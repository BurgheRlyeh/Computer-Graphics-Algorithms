//#include "SceneCB.h"

struct ModelBuffer
{
    float4x4 mModel;
    float4x4 mNormTang;
    float4 settings;
    float4 posAngle;
};

cbuffer ModelBufferInst: register(b0) {
    ModelBuffer modelBuffer[50];
};

struct Vertex
{
    float4 position;
    float4 tangent;
    float4 norm;
    float4 uv;
};

cbuffer VIBuffer: register(b1) {
    Vertex vertices[24];
    int4 indices[12];
}

cbuffer RTBuffer: register(b2) {
    float4 whnf;
    float4x4 vpInv;
    float4 instances;
}

struct ModelBufferInv
{
    float4x4 mInv;
};

cbuffer ModelBufferInv: register(b3) {
    ModelBufferInv modelBufferInv[50];
}

struct Ray
{
    float4 orig;
    float4 dest;
    float4 dir;
};

struct Intsec
{
    Ray ray;
    int modelID;
    int triangleID;
    float t;
    float u;
    float v;
};

RWTexture2D<float4> texOutput: register(u0);

Texture2DArray colorTexture : register(t0);
Texture2D normalMapTexture : register(t1);

SamplerState colorSampler : register(s0);

// Projects a 3D vector from screen space into world space
float4 screenToWorld(float2 pixel, float depth)
{
    float4 v = float4(2.f * pixel / whnf.xy - 1.f, (1.f - depth) / (whnf.z - whnf.w), 1.f);
    v.y *= -1;
    v = mul(vpInv, v);
    return v / v.w;
}

Ray rayGen(uint3 DTid: SV_DispatchThreadID) {
    Ray ray;

    ray.orig = screenToWorld(DTid.xy + 0.5f, 0.f);
    ray.dest = screenToWorld(DTid.xy + 0.5f, 1.f);
    ray.dir = normalize(ray.dest - ray.orig);

    return ray;
}

// Moller-Trumbore Intersection Algorithm
Intsec rayIntsecTriangle(Ray ray, float4 v0, float4 v1, float4 v2)
{
    Intsec intsec;
    intsec.t = whnf.w;

    // edges
    float3 e1 = v1 - v0;
    float3 e2 = v2 - v0;

    float3 h = cross(ray.dir, e2);
    float a = dot(e1, h);

    // check is parallel
    if (abs(a) < 1e-8)
        return intsec;

    float3 s = ray.orig - v0;
    intsec.u = dot(s, h) / a;

    // check u range
    if (intsec.u < 0.0 || 1.0 < intsec.u)
        return intsec;

    float3 q = cross(s, e1);
    intsec.v = dot(ray.dir, q) / a;

    // check v + u range
    if (intsec.v < 0.0 || 1.0 < intsec.u + intsec.v)
        return intsec;

    intsec.t = dot(e2, q) / a;
    return intsec;
}

Intsec CalculateIntersection(Ray ray)
{
    Intsec best;
    best.t = whnf.w;

    for (int m = 0; m < instances.x; ++m)
    {
        Ray mray;
        mray.orig = mul(modelBufferInv[m].mInv, ray.orig);
        mray.dest = mul(modelBufferInv[m].mInv, ray.dest);
        mray.dir = normalize(mray.dest - mray.orig);

        for (int i = 0; i < 12; ++i)
        {
            float4 v0 = vertices[indices[i].x].position;
            float4 v1 = vertices[indices[i].y].position;
            float4 v2 = vertices[indices[i].z].position;

            Intsec curr = rayIntsecTriangle(mray, v0, v1, v2);

            if (whnf.z < curr.t && curr.t < best.t)
            {
                best = curr;
                best.ray = ray;
                best.modelID = m;
                best.triangleID = i;
            }
        }
    }

    return best;
}

[numthreads(1, 1, 1)]
void cs(uint3 DTid: SV_DispatchThreadID) {
    Ray ray = rayGen(DTid);
    Intsec best = CalculateIntersection(ray);

    if (best.t >= whnf.w)
    {
        return;
    }

    Ray dir = rayGen(float3(whnf.xy / 2.f, 1.f));
    float depth = best.t;// * dot(ray.direction, dir.direction);

    float4 colorNear = float4(1.f, 1.f, 1.f, 1.f);
    float4 colorFar = float4(0.f, 0.f, 0.f, 1.f);

    float4 finalCl = lerp(colorNear, colorFar, 1.f - 1.f / depth);
    //texOutput[DTid.xy] = finalCl;

    float2 uv = float2(best.u, best.v);
    //if (best.triangleID % 2 == 0)
    //{
    //    uv = 1 - uv;
    //}

    float4 color = colorTexture.SampleLevel(
        colorSampler,
        float3(uv, modelBuffer[best.modelID].settings.z),
        0
    );

    texOutput[DTid.xy] = finalCl * color;
}