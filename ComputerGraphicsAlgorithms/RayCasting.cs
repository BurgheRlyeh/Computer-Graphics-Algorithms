#include "SceneCB.h"

struct ModelBuffer
{
    float4x4 mModel;
    float4x4 mNormTang;
    float4 settings;
    float4 posAngle;
};

cbuffer ModelBufferInst: register(b1) {
    ModelBuffer modelBuffer[100];
};

struct Vertex
{
    float4 position;
    float4 tangent;
    float4 norm;
    float4 uv;
};

cbuffer VIBuffer: register(b2) {
    Vertex vertices[24];
    int4 indices[36];
}

cbuffer RTBuffer: register(b3) {
    float4 whnf;
    float4x4 pvInv;
}

struct Ray
{
    int modelID;
    int triangleID;
    float3 origin;
    float3 direction;
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

    float4 ndc = float4(x, y, 0.f, 1.f);
    float4 worldPos = mul(pvInv, ndc);

    ray.origin = cameraPos.xyz;
    ray.direction = worldPos.xyz;
    return ray;
}

// Moller-Trumbore Intersection Algorithm
Intsec rayIntsecTriangle(Ray ray, float3 v0, float3 v1, float3 v2)
{
    Intsec intsec;
    intsec.t = whnf.w;

    // edges
    float3 e1 = v1 - v0;
    float3 e2 = v2 - v0;

    float3 h = cross(ray.direction, e2);
    float a = dot(e1, h);

    // check is parallel
    if (abs(a) < 1e-8)
        return intsec;

    float3 s = ray.origin - v0;
    intsec.u = dot(s, h) / a;

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

    const int primitives = 12;

    for (int m = 0; m < 2; ++m)
    {
        for (int i = 0; i < primitives; ++i)
        {
            float3 v0 = getVertexWorldPos(indices[i].x, m);
            float3 v1 = getVertexWorldPos(indices[i].y, m);
            float3 v2 = getVertexWorldPos(indices[i].z, m);

            Intsec curr = rayIntsecTriangle(ray, v0.xyz, v1.xyz, v2.xyz);

            if (curr.t < whnf.z || whnf.w < curr.t)
                continue;

            if (curr.t < best.t)
            {
                //best = curr;
                best.t = curr.t;
                best.u = curr.u;
                best.v = curr.v;
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

    float3 v0 = getVertexWorldPos(indices[tId].x, mId);
    float3 v1 = getVertexWorldPos(indices[tId].y, mId);
    float3 v2 = getVertexWorldPos(indices[tId].z, mId);

    return v0 + intsec.u * (v1 - v0) + intsec.v * (v2 - v0);
}


[numthreads(1, 1, 1)]
void cs(uint3 DTid: SV_DispatchThreadID)
{
    Ray ray = GenerateRay(DTid);
    Intsec best = CalculateIntersection(ray);

    float t = best.t;

    texOutput[DTid.xy] = normalize(
        float4(DTid.x / whnf.x, DTid.y / whnf.y, DTid.z, 1.f)
    );


    //float intens = exp(best.t);
    //float3 cl = float3(intens, intens, intens);
    //cl = normalize(cl);

    //texOutput[DTid.xy] = float4(cl, 0.f);
    //texOutput[DTid.xy] = normalize(float4(ray.direction - ray.origin, 1.f));
}