#include "SceneCB.h"

struct ModelBuffer
{
    float4x4 mModel;
    float4x4 mNormTang;
    float4 settings;
    float4 posAngle;
};

cbuffer ModelBufferInst: register(b1)
{
    ModelBuffer modelBuffer[100];
};

struct Vertex
{
    float4 position;
    float4 tangent;
    float4 norm;
    float4 uv;
};

cbuffer VIBuffer: register(b2)
{
    Vertex vertices[24];
    int4 indices[36];
}

cbuffer RTBuffer: register(b3)
{
    float4 whnf;
    float4x4 pvInv;
}

struct Ray
{
    int modelID;
    int triangleID;
    float3 origin;
    float3 direction;
    float dist;
};

struct Intsec
{
    int modelID;
    int triangleID;
    float t; // dist
    // barycentric cs
    float u;
    float v;
};

RWTexture2D<float4> texOutput: register(u0);

Ray GenerateRay(uint3 DTid: SV_DispatchThreadID)
{
    Ray ray;

    // Calculate NDC in range [-1, 1]
    float x = 2.f * DTid.x / whnf.x - 1.f;
    float y = 2.f * DTid.y / whnf.y - 1.f;

    //if (x < 0 || width < x) {
    //    return
    //}
    //if (y < 0 || height < y) {

    //}

    float4 ndc = float4(x, y, 0.f, 1.f);
    float4 worldPos = mul(pvInv, ndc);

    ray.origin = cameraPos.xyz;
    ray.direction = worldPos.xyz;
    ray.dist = 0.f;
    ray.modelID = -1;
    ray.triangleID = -1;
    return ray;
}

// Moller-Trumbore Intersection Algorithm
Intsec rayIntsecTriangle(Ray ray, float3 v0, float3 v1, float3 v2)
{
    Intsec intsec;
    intsec.t = 0.f;

    // edges
    float3 e1 = v1 - v0;
    float3 e2 = v2 - v0;

    float3 n = cross(ray.direction, e2);
    float a = dot(e1, n);

    // check is parallel
    if (abs(a) < 1e-8)
        return intsec;

    float3 s = ray.origin - v0;
    intsec.u = dot(s, n) / a;

    // check u range
    if (intsec.u < 0.0 || 1.0 < intsec.u)
        return intsec;

    float3 q = cross(s, e1);
    intsec.v = dot(ray.direction, q) / a;

    // check v + u rang
    if (intsec.v < 0.0 || 1.0 < intsec.u + intsec.v)
        return intsec;

    intsec.t = dot(e2, q) / a;
    return intsec;
}

float3 getVertexWorldPos(int id, int mID)
{
    float4 v = float4(vertices[id].position.xyz, 1.f);
    return mul(modelBuffer[mID].mModel, v).xyz;
}

Intsec CalculateIntersection(Ray ray)
{
    Intsec best;
    best.t = whnf.w;

    const int primitives = 12; // 12

    for (int m = 0; m < 10; ++m)
    {
        for (int i = 0; i < primitives; ++i)
        {
            float3 v0 = getVertexWorldPos(indices[3 * i].x, m);
            float3 v1 = getVertexWorldPos(indices[3 * i].y, m);
            float3 v2 = getVertexWorldPos(indices[3 * i].z, m);

            Intsec curr = rayIntsecTriangle(ray, v0.xyz, v1.xyz, v2.xyz);

            if (curr.t < whnf.z || whnf.w < curr.t)
                continue;

            if (curr.t < best.t)
            {
                best = curr;
                best.modelID = m;
                best.triangleID = i;
            }
        }
    }

    return best;
}

float3 getIntsecPoint(Intsec intsec)
{
    int mId = intsec.modelID;
    int tId = intsec.triangleID;

    float3 v0 = getVertexWorldPos(3 * tId, mId);
    float3 v1 = getVertexWorldPos(3 * tId + 1, mId);
    float3 v2 = getVertexWorldPos(3 * tId + 2, mId);

    return v0 + intsec.u * (v1 - v0) + intsec.v * (v2 - v0);
}


[numthreads(1, 1, 1)]
void main(uint3 DTid: SV_DispatchThreadID)
{
    Ray ray = GenerateRay(DTid);
    Intsec best = CalculateIntersection(ray);

    texOutput[DTid.xy] = float4(best.t, best.t, best.t, 1.f);
}

//#include "SceneCB.h"

//struct ModelBuffer {
//    float4x4 mModel;
//    float4x4 mNormTang;
//    float4 settings;
//    float4 posAngle;
//};

//cbuffer ModelBufferInst: register(b1) {
//    ModelBuffer modelBuffer[100];
//};

//struct Vertex {
//    float3 position;
//    float3 tangent;
//    float3 norm;
//    float2 uv;
//};

//cbuffer VIBuffer: register(b2) {
//    Vertex vertices[24];
//    int indices[36];
//}

//cbuffer RTBuffer: register(b3) {
//    float width;
//    float height;
//    float near;
//    float far;
//    float4x4 pvInv;
//}

//struct Ray {
//    int modelID;
//    int triangleID;
//    float3 origin;
//    float3 direction;
//    float dist;
//};

//struct Intsec {
//    int modelID;
//    int triangleID;
//    float t; // dist
//    // barycentric cs
//    float u;
//    float v;
//};

//RWStructuredBuffer<Ray> rays;
//RWStructuredBuffer<Intsec> intersections;

//Ray GenerateRay(uint3 DTid: SV_DispatchThreadID)
//{
//    Ray ray;
    
//    // Calculate NDC in range [-1, 1]
//    float x = 2.f * DTid.x / width - 1.f;
//    float y = 2.f * DTid.y / height - 1.f;
    
//    //if (x < 0 || width < x) {
//    //    return
//    //}
//    //if (y < 0 || height < y) {
        
//    //}
    
//    float4 ndc = float4(x, y, 0.f, 1.f);
//    float4 worldPos = mul(pvInv, ndc);
    
//    ray.origin = cameraPos;
//    ray.direction = worldPos;
//    ray.dist = 0.f;
//    ray.modelID = -1;
//    ray.triangleID = -1;
//    return ray;
//}

//// Moller-Trumbore Intersection Algorithm
//Intsec rayIntsecTriangle(Ray ray, float3 v0, float3 v1, float3 v2) {
//    Intsec intsec;
//    intsec.t = 0.f;
    
//    // edges
//    float3 e1 = v1 - v0;
//    float3 e2 = v2 - v0;
    
//    float3 n = cross(ray.direction, e2);
//    float a = dot(e1, n);
    
//    // check is parallel
//    if (abs(a) < 1e-8)
//        return intsec;
    
//    float3 s = ray.origin - v0;
//    intsec.u = dot(s, n) / a;
    
//    // check u range
//    if (intsec.u < 0.0 || 1.0 < intsec.u)
//        return intsec;
    
//    float3 q = cross(s, e1);
//    intsec.v = dot(ray.direction, q) / a;
    
//    // check v + u rang
//    if (intsec.v < 0.0 || 1.0 < intsec.u + intsec.v)
//        return intsec;
    
//    intsec.t = dot(e2, q) / a;
//    return intsec;
//}

//float3 getVertexWorldPos(int id, int mID) {
//    float4 v = float4(vertices[indices[id]].position, 1.f);
//    return mul(modelBuffer[mID].mModel, v).xyz;
//}

//Intsec CalculateIntersection(Ray ray) {
//    Intsec best;
//    best.t = far;
    
//    const int primitives = 24; // 12
    
//    for (int m = 0; m < 100; ++m) {
//        for (int i = 0; i < primitives; ++i) {
//            float3 v0 = getVertexWorldPos(3 * i, m);
//            float3 v1 = getVertexWorldPos(3 * i + 1, m);
//            float3 v2 = getVertexWorldPos(3 * i + 2, m);
            
//            Intsec curr = rayIntsecTriangle(ray, v0.xyz, v1.xyz, v2.xyz);
        
//            if (curr.t < near || far < curr.t)
//                continue;
        
//            if (curr.t < best.t) {
//                best = curr;
//                best.modelID = m;
//                best.triangleID = i;
//            }
//        }
//    }
    
//    return best;
//}

//float3 getIntsecPoint(Intsec intsec)
//{
//    int mId = intsec.modelID; 
//    int tId = intsec.triangleID;
    
//    float3 v0 = getVertexWorldPos(3 * tId, mId);
//    float3 v1 = getVertexWorldPos(3 * tId + 1, mId);
//    float3 v2 = getVertexWorldPos(3 * tId + 2, mId);
    
//    return v0 + intsec.u * (v1 - v0) + intsec.v * (v2 - v0);
//}


//[numthreads(1, 1, 1)]
//void main(uint3 DTid: SV_DispatchThreadID)
//{
//}

//Intersection getIntersection(Ray ray, float3 a, float3 b, float3 v) {
//    Intersection intersection;
//    intersection.triangleID = -1;
	
//    float3 e1 = b - a;
//    float3 e2 = v - a;
    
//    float3 p = cross(ray.direction, e2);
//    float det = 1.f / dot(e1, p);
    
//    float3 t = ray.origin - a;
//    intersection.u = dot(t, p) * det;
    
//    float3 q = cross(t, e1);
//    intersection.v = dot(ray.direction, q) * det;
//    intersection.t = dot(e2, q) * det;
	
//    return intersection;
//}

//bool RayTriangleTest(Intersection intersection) {
//    return intersection.u >= 0.0f
//		&& intersection.v >= 0.0f
//		&& intersection.u + intersection.v <= 1.0f
//		&& intersection.t >= 0.0f;
//}

//void CalculateIntersection(Ray ray)
//{
//    Intersection cIntersection;
    
//    Intersection bIntersection;
//    bIntersection.triangleID = -1;
//    bIntersection.t = 10000.f;
//    bIntersection.u = -1;
//    bIntersection.v = -1;
    
//    const int iNumPrimitives = 10;
    
//    for (int i = 0; i < iNumPrimitives; ++i) {
//        unsigned int offset = i * 3;
//        float3 A = g_sVertices[g_sIndices[offset]].position;
//        float3 B = g_sVertices[g_sIndices[offset + 1]].position;
//        float3 C = g_sVertices[g_sIndices[offset + 2]].position;
//        cIntersection = getIntersection(ray, A, B, C);
        
//        if (ray.triangleID == i)
//            continue;
        
//        if (!RayTriangleTest(cIntersection))
//            continue;
        
//        if (cIntersection.t < bIntersection.t)
//        {
//            bIntersection = cIntersection;
//            bIntersection.triangleID = i;
//        }
//    }
//}



//void GenerateRays(uint3 DTid: SV_DispatchThreadID)
//{
//    float x = 1.f + float(2.f * DTid.x + 1.f) / width;
//    float y = 1.f - float(2.f * DTid.y + 1.f) / width;
//    float z = 2.f;
    
//    //float4 aux = mul(float4(0, 0, 0, 1.f), model);
//    //float4 pixelPos = mul(float4(x, y, z, 1.f), g_mfWorld);
    
//    Ray ray;
//    ray.origin = cameraPos.xyz;
//    ray.direction = float3(0, 0, 0); //    normalize(pixelPos.xyz - ray.origin);
//    ray.dist = far + 1.f;
//    ray.triangleID = -1;
    
//    unsigned int index = DTid.y * width + DTid.x;
    
//    g_uRays[index] = ray;
//    g_uAccumulation[index] = 0.0f;
//}