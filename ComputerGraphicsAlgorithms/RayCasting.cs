//#include "SceneCB.h"

struct ModelBuffer
{
    float4x4 mModel;
    float4x4 mNormTang;
    float4 settings;
    float4 posAngle;
};

cbuffer ModelBufferInst: register(b0)
{
    ModelBuffer modelBuffer[25];
};

struct Vertex
{
    float4 position;
    float4 tangent;
    float4 norm;
    float4 uv;
};

cbuffer VIBuffer: register(b1)
{
    Vertex vertices[24];
    int4 indices[12];
}

cbuffer RTBuffer: register(b2)
{
    float4 whnf;
    float4x4 vpInv;
    float4 instances;
    float4 camDir;
}

struct ModelBufferInv
{
    float4x4 mInv;
};

cbuffer ModelBufferInv: register(b3)
{
    ModelBufferInv modelBufferInv[25];
}

struct BVHNode
{
    float4 bbMin, bbMax;
    uint4 leftFirstCnt;
};

cbuffer BVH: register(b4)
{
    BVHNode bvhNode[2 * 12 * 25 - 1];
    uint4 triIdx[12 * 25];
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

Texture2DArray colorTexture: register(t0);
Texture2D normalMapTexture: register(t1);

SamplerState colorSampler: register(s0);

float4 pixelToWorld(float2 pixel, float depth)
{
    float2 ndc = 2.f * pixel / whnf.xy - 1.f;
    ndc.y *= -1;

    depth = (1.f - depth) / (whnf.z - whnf.w);

    float4 res = mul(vpInv, float4(ndc, depth, 1.f));
    return res / res.w;
}

Ray generateRay(float2 screenPoint)
{
    Ray ray;

    ray.orig = pixelToWorld(screenPoint.xy + 0.5f, 0.f);
    ray.dest = pixelToWorld(screenPoint.xy + 0.5f, 1.f);
    ray.dir = normalize(ray.dest - ray.orig);

    return ray;
}

// Moller-Trumbore Intersection Algorithm
Intsec rayIntsecTriangle(Ray ray, float4 v0, float4 v1, float4 v2)
{
    Intsec intsec;
    intsec.t = whnf.w;

    // edges
    float3 e1 = v1.xyz - v0.xyz;
    float3 e2 = v2.xyz - v0.xyz;

    float3 h = cross(ray.dir.xyz, e2);
    float a = dot(e1, h);

    // check is parallel
    if (abs(a) < 1e-8)
        return intsec;

    float3 s = ray.orig.xyz - v0.xyz;
    intsec.u = dot(s, h) / a;

    // check u range
    if (intsec.u < 0.0 || 1.0 < intsec.u)
        return intsec;

    float3 q = cross(s, e1);
    intsec.v = dot(ray.dir.xyz, q) / a;

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

bool IntersectAABB(Ray ray, float3 bmin, float3 bmax)
{
    float3 t1 = (bmin - ray.orig) / ray.dir;
    float3 t2 = (bmax - ray.orig) / ray.dir;

    float3 tmin = min(t1, t2);
    float3 tmax = max(t1, t2);

    float tmin_max = max(tmin.x, max(tmin.y, tmin.z));
    float tmax_min = min(tmax.x, min(tmax.y, tmax.z));

    return tmax_min >= tmin_max && 0 < tmax_min && tmin_max < whnf.w;
}

Intsec bvhIntersection(Ray ray) {
    Intsec best;
    best.t = whnf.w;

    // Create a stack to store the nodes to be processed.
    uint stack[599];
    uint stackSize = 0;
    stack[stackSize++] = 0;

    while (stackSize > 0) {
        BVHNode node = bvhNode[stack[--stackSize]];

        if (!IntersectAABB(ray, node.bbMin, node.bbMax))
            continue;

        if (node.leftFirstCnt.z == 0) {
            stack[stackSize++] = node.leftFirstCnt.x;
            stack[stackSize++] = node.leftFirstCnt.x + 1;
            continue;
        }

        for (uint i = 0; i < node.leftFirstCnt.z; ++i) {
            uint mId = triIdx[node.leftFirstCnt.y + i].x / 12;
            uint tId = triIdx[node.leftFirstCnt.y + i].x % 12;

            Ray mray;
            mray.orig = mul(modelBufferInv[mId].mInv, ray.orig);
            mray.dest = mul(modelBufferInv[mId].mInv, ray.dest);
            mray.dir = normalize(mray.dest - mray.orig);

            float4 v0 = vertices[indices[tId].x].position;
            float4 v1 = vertices[indices[tId].y].position;
            float4 v2 = vertices[indices[tId].z].position;

            Intsec curr = rayIntsecTriangle(mray, v0, v1, v2);

            if (whnf.z < curr.t && curr.t < best.t)
            {
                best = curr;
                best.ray = ray;
                best.modelID = mId;
                best.triangleID = tId;
            }
        }
    }

    return best;
}

[numthreads(1, 1, 1)]
void cs(uint3 DTid: SV_DispatchThreadID)
{
    Ray ray = generateRay(DTid.xy);

    //best.t = whnf.w;
    //Intsec best = CalculateIntersection(ray);
    Intsec best = bvhIntersection(ray);

    if (best.t >= whnf.w)
        return;

    float depth = best.t * dot(ray.dir, camDir);

    float4 colorNear = float4(1.f, 1.f, 1.f, 1.f);
    float4 colorFar = float4(0.f, 0.f, 0.f, 1.f);

    float4 finalCl = lerp(colorNear, colorFar, 1.f - 1.f / depth);

    //texOutput[DTid.xy] = finalCl;
    //return;

    Vertex v0 = vertices[indices[best.triangleID].x];
    Vertex v1 = vertices[indices[best.triangleID].y];
    Vertex v2 = vertices[indices[best.triangleID].z];

    float u = best.u;
    float v = best.v;
    float2 uv = (1 - u - v) * v0.uv.xy + u * v1.uv.xy + v * v2.uv.xy;

    float4 color = colorTexture.SampleLevel(
        colorSampler,
        float3(uv, modelBuffer[best.modelID].settings.z),
        0
    );

    texOutput[DTid.xy] = color;
}