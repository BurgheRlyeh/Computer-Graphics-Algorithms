#include "SceneCB.h"

struct ModelBuffer {
    float4x4 mModel;
    float4x4 mNormTang;
    float4 settings;
    float4 posAngle;
};

cbuffer ModelBufferInst: register(b1) {
    ModelBuffer modelBuffer[100];
};

struct Vertex {
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
    float4x4 vpInv;
}

struct Ray {
    float3 origin;
    float3 direction;
};

struct Intsec {
    int modelID;
    int triangleID;
    float t;
    float u;
    float v;
};

RWTexture2D<float4> texOutput: register(u0);

// transforms vector & projecting back into w = 1
float4 transform(float4 v, float4x4 m) {
    m = transpose(m);

    float4 res =
        v.xxxx * m[0] +
        v.yyyy * m[1] +
        v.zzzz * m[2] + m[3];

    return res / res.w;
}

// Projects a 3D vector from screen space into world space
float3 unproject(float4 v) {
    // to ndc
    float4 scale = float4(whnf.x / 2, -whnf.y / 2, whnf.w - whnf.z, 1.f);
    scale = rcp(scale); // per-component reciprocal

    // near offset
    float4 offset = float4(0.f, 0.f, -whnf.z, 0.f);
    offset = scale * offset + float4(-1.f, 1.f, 0.f, 0.f);

    return transform(v * scale + offset, vpInv).xyz;
}

Ray rayGen(uint3 DTid: SV_DispatchThreadID) {
    float4 pixelNear = float4(DTid.x, DTid.y, 0.f, 0.f);
    float4 pixelFar = float4(DTid.x, DTid.y, 1.f, 0.f);
    float3 worldDir = unproject(pixelFar) - unproject(pixelNear);

    Ray ray;
    ray.origin = cameraPos.xyz;
    ray.direction = normalize(worldDir);
    return ray;
}

//Ray rayGen(uint3 DTid: SV_DispatchThreadID) {
//    float2 xy = 2.f * (DTid.xy) / whnf.xy - 1.f;
//    xy.y *= -1;
//    float4 ndc = float4(xy, 1.f, 1.f);
//    float4 worldDir = mul(vpInv, ndc);

//    Ray ray;
//    ray.origin = cameraPos.xyz;
//    ray.direction = normalize(worldDir);
//    return ray;
//}

// Moller-Trumbore Intersection Algorithm
Intsec rayIntsecTriangle(Ray ray, float3 v0, float3 v1, float3 v2) {
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

    // ray.origin + t * ray.direction

    intsec.u = u;
    intsec.v = v;
    return intsec;
}

float3 getVertexWorldPos(int id, int mID) {
    float4 v = float4(vertices[id].position.xyz, 1.f);
    float4 mv = mul(modelBuffer[mID].mModel, v);
    return mv.xyz / mv.w;
}

Intsec CalculateIntersection(Ray ray)
{
    Intsec best;
    best.t = whnf.w;

    for (int m = 0; m < 10; ++m)
    {
        for (int i = 0; i < 12; ++i)
        {
            float3 v0 = getVertexWorldPos(indices[i].x, m);
            float3 v1 = getVertexWorldPos(indices[i].y, m);
            float3 v2 = getVertexWorldPos(indices[i].z, m);

            Intsec curr = rayIntsecTriangle(ray, v0, v1, v2);

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

[numthreads(1, 1, 1)]
void cs(uint3 DTid: SV_DispatchThreadID) {
    Ray ray = rayGen(DTid);
    Intsec best = CalculateIntersection(ray);

    if (best.t >= whnf.w) {
        return;
    }

    float4 colorNear = float4(1.f, 1.f, 1.f, 1.f);
    float4 colorFar = float4(0.f, 0.f, 0.f, 1.f);

    texOutput[DTid.xy] = lerp(colorNear, colorFar, 1.f - 1 / best.t);
}