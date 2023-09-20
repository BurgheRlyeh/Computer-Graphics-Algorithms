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
    int4 indices[12];
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
    float3 vec;
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

float4 transform(float4 v, float4x4 m)
{

    //m = transpose(m);

    float4 x = float4(v.xxxx);
    float4 y = float4(v.yyyy);
    float4 z = float4(v.zzzz);

    float4 c0 = float4(m._11, m._21, m._31, m._41);
    float4 c1 = float4(m._12, m._22, m._32, m._42);
    float4 c2 = float4(m._13, m._23, m._33, m._43);
    float4 c3 = float4(m._14, m._24, m._34, m._44);


    float4 res = z * c2 + c3;
    res = res + y * c1;
    res = res + x * c0;

    //float4 res = x * c0 + y * c1 + z * c2 + c3;

    res /= res.w;

    return res;
}

float4 unproject(float4 v) {
    float4 d = float4(-1.f, 1.f, 0.f, 0.f);

    float4 scale = float4(0.5f * whnf.x, -0.5f * whnf.y, whnf.w - whnf.z, 1.f);
    scale = rcp(scale);

    float4 offset = float4(-0.f, -0.f, -whnf.z, 0.f);
    offset = scale * offset + d;

    float4x4 trans = pvInv;

    float4 res = v * scale + offset;

    return transform(res, trans);
}

Ray rayGen(uint3 DTid: SV_DispatchThreadID) {
    float4 mouseNear = float4(DTid.x, DTid.y, 0.f, 0.f);
    float4 mouseFar = float4(DTid.x, DTid.y, 1.f, 0.f);
    //float4 mouseNear = float4(whnf.x / 2.f, whnf.y / 2.f, 0.f, 0.f);
    //float4 mouseFar = float4(whnf.x / 2.f, whnf.y / 2.f, 1.f, 0.f);

    float4 unprojNear = unproject(mouseNear);
    float4 unprojFar = unproject(mouseFar);

    float4 res = normalize(unprojFar - unprojNear);

    Ray ray;
    ray.origin = (cameraPos.xyz);
    ray.direction = res;
    ray.vec = ray.direction;
    return ray;
}

Ray GenerateRay(uint3 DTid: SV_DispatchThreadID)
{
    Ray ray;

    // Calculate NDC in range [-1, 1]
    float2 xy = 2.f * (DTid.xy + 0.f) / whnf.xy - 1.f;
    xy.y *= -1;

    float4 ndc = float4(xy, 0.f, 1.f);
    float4 worldPos = mul(pvInv, ndc);
    //worldPos.xyz /= worldPos.w;
    worldPos = normalize(worldPos);

    ray.origin = cameraPos.xyz;
    ray.direction = worldPos.xyz;
    ray.vec = /*normalize*/(worldPos.xyz - ray.origin);
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
    float u = dot(s, h) / a;

    // check u range
    if (u < 0.0 || 1.0 < u)
        return intsec;

    float3 q = cross(s, e1);
    float v = dot(ray.direction, q) / a;

    // check v + u rang
    if (v < 0.0 || 1.0 < u + v)
        return intsec;

    intsec.t = dot(e2, q) / a;
    intsec.u = u;
    intsec.v = v;
    return intsec;
}

float3 getVertexWorldPos(int id, int mID)
{
    float4 v = float4(vertices[id].position.xyz, 1.f);
    float4 mv = mul(modelBuffer[mID].mModel, v);
    mv.xyz /= mv.w;
    return mv.xyz;
}

Intsec CalculateIntersection(Ray ray)
{
    Intsec best;
    best.t = whnf.w;

    const int primitives = 12;

    for (int m = 0; m < 10; ++m)
    {
        for (int i = 0; i < 12; ++i)
        {
            float3 v0 = getVertexWorldPos(indices[i].x, m);
            float3 v1 = getVertexWorldPos(indices[i].y, m);
            float3 v2 = getVertexWorldPos(indices[i].z, m);

            Intsec curr = rayIntsecTriangle(ray, v0, v1, v2);
            float t = curr.t;
            float u = curr.u;
            float v = curr.v;

            if (whnf.z < curr.t && curr.t < best.t)
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

    float3 v0 = getVertexWorldPos(indices[tId].x, mId);
    float3 v1 = getVertexWorldPos(indices[tId].y, mId);
    float3 v2 = getVertexWorldPos(indices[tId].z, mId);

    return v0 + intsec.u * (v1 - v0) + intsec.v * (v2 - v0);
}


[numthreads(1, 1, 1)]
void cs(uint3 DTid: SV_DispatchThreadID)
{
    //Ray ray0 = GenerateRay(DTid);
    //Intsec best0 = CalculateIntersection(ray0);

    Ray ray1 = rayGen(DTid);
    Intsec best1 = CalculateIntersection(ray1);

    if (best1.t < whnf.w)
    {
        float4 colorNear = float4(1.f, 1.f, 1.f, 1.0f);
        float4 colorFar = float4(0.f, 0.f, 0.f, 1.0f);

        float depth = 1.f - 1 / best1.t;

        float4 finalColor = lerp(colorNear, colorFar, depth);

        texOutput[DTid.xy] = finalColor;
    }
}