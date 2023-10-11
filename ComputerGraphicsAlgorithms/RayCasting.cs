#include "Light.h"

struct ModelBuffer {
    float4x4 modelMatrix;
    float4x4 normTangMatrix;
    float4 shineSpeedTexIdNM;
    float4 posAngle;
};
cbuffer ModelBufferInst: register(b1)
{
    ModelBuffer modelBuffer[25];
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
    float4 instances;
    float4 camDir;
}

cbuffer ModelBufferInv: register(b4) {
    float4x4 modelBufferInv[25];
}

struct AABB {
    float4 bmin, bmax;
};

struct BVHNode {
    AABB aabb;
    int4 leftFirstCntParent;
};

cbuffer BVH: register(b5) {
    BVHNode bvhNode[2 * 12 * 25 - 1];
    int4 triIdx[12 * 25];
}

struct Ray {
    float4 orig;
    float4 dest;
    float4 dir;
};

struct Intsec {
    int mId;
    int tId;
    float t;
    float u;
    float v;
};

RWTexture2D<float4> texOutput: register(u0);

Texture2DArray colorTexture: register(t0);
Texture2D normalMapTexture: register(t1);

SamplerState colorSampler: register(s0);

float4 pixelToWorld(float2 pixel, float depth) {
    float2 ndc = 2.f * pixel / whnf.xy - 1.f;
    ndc.y *= -1;

    depth = (1.f - depth) / (whnf.z - whnf.w);

    float4 res = mul(vpInv, float4(ndc, depth, 1.f));
    return res / res.w;
}

Ray generateRay(float2 screenPoint) {
    Ray ray;

    ray.orig = pixelToWorld(screenPoint.xy + 0.5f, 0.f);
    ray.dest = pixelToWorld(screenPoint.xy + 0.5f, 1.f);
    ray.dir = normalize(ray.dest - ray.orig);

    return ray;
}

// Moller-Trumbore Intersection Algorithm
Intsec rayTriangleIntersection(Ray ray, float4 v0, float4 v1, float4 v2)
{
    Intsec intsec;
    intsec.mId = intsec.tId = -1;
    intsec.t = intsec.u = intsec.v = -1.f;

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

// naive intersection part
Intsec naiveIntersection(Ray ray) {
    Intsec best;
    best.mId = best.tId = -1;
    best.t = whnf.w;
    best.u = best.v = -1.f;

    for (int m = 0; m < instances.x; ++m)
    {
        Ray mRay;
        mRay.orig = mul(modelBufferInv[m], ray.orig);
        mRay.dest = mul(modelBufferInv[m], ray.dest);
        mRay.dir = normalize(mRay.dest - mRay.orig);

        for (int i = 0; i < 12; ++i)
        {
            float4 v0 = vertices[indices[i].x].position;
            float4 v1 = vertices[indices[i].y].position;
            float4 v2 = vertices[indices[i].z].position;

            Intsec curr = rayTriangleIntersection(mRay, v0, v1, v2);

            if (whnf.z < curr.t && curr.t < best.t) {
                best = curr;
                best.mId = m;
                best.tId = i;
            }
        }
    }

    return best;
}

// bvh intersection part
bool isRayIntersectsAABB(Ray ray, AABB aabb) {
    float4 v1 = (aabb.bmin - ray.orig) / ray.dir;
    float4 v2 = (aabb.bmax - ray.orig) / ray.dir;

    float3 vmin = min(v1.xyz, v2.xyz);
    float3 vmax = max(v1.xyz, v2.xyz);

    float tmin = max(max(vmin.x, vmin.y), vmin.z);
    float tmax = min(min(vmax.x, vmax.y), vmax.z);

    return whnf.z < tmax && tmin <= tmax && tmin < whnf.w;
}

Intsec bestBVHLeafIntersection(Ray ray, BVHNode node)
{
    Intsec best;
    best.mId = best.tId = -1;
    best.t = whnf.w;
    best.u = best.v = -1.f;

    for (int i = 0; i < node.leftFirstCntParent.z; ++i) {
        int mId = triIdx[node.leftFirstCntParent.y + i].x / 12;
        int tId = triIdx[node.leftFirstCntParent.y + i].x % 12;

        Ray mRay;
        mRay.orig = mul(modelBufferInv[mId], ray.orig);
        mRay.dest = mul(modelBufferInv[mId], ray.dest);
        mRay.dir = normalize(mRay.dest - mRay.orig);

        float4 v0 = vertices[indices[tId].x].position;
        float4 v1 = vertices[indices[tId].y].position;
        float4 v2 = vertices[indices[tId].z].position;

        Intsec curr = rayTriangleIntersection(mRay, v0, v1, v2);

        if (whnf.z < curr.t && curr.t < best.t) {
            best = curr;
            best.mId = mId;
            best.tId = tId;
        }
    }

    return best;
}

Intsec bvhIntersection(Ray ray)
{
    Intsec best;
    best.mId = best.tId = -1;
    best.t = whnf.w;
    best.u = best.v = -1.f;

    // Create a stack to store the nodes to be processed.
    int stack[599];
    int stackSize = 0;
    stack[stackSize++] = 0;

    while (stackSize > 0) {
        BVHNode node = bvhNode[stack[--stackSize]];

        if (!isRayIntersectsAABB(ray, node.aabb))
            continue;

        if (node.leftFirstCntParent.z == 0) {
            stack[stackSize++] = node.leftFirstCntParent.x;
            stack[stackSize++] = node.leftFirstCntParent.x + 1;
            continue;
        }

        Intsec curr = bestBVHLeafIntersection(ray, node);
        if (curr.t < best.t)
            best = curr;
    }

    return best;
}

// stack-less

int parent(int node) {
    return bvhNode[node].leftFirstCntParent.w;
}

int leftChild(int node) {
    return bvhNode[node].leftFirstCntParent.x;
}

int sibling(int node) {
    return bvhNode[parent(node)].leftFirstCntParent.x + 1;
}

bool isLeaf(int node) {
    return bvhNode[node].leftFirstCntParent.z > 0;
}

Intsec bvhStacklessIntersection(Ray ray)
{
    Intsec best;
    best.mId = best.tId = -1;
    best.t = whnf.w;
    best.u = best.v = -1.f;

    if (!isRayIntersectsAABB(ray, bvhNode[0].aabb))
        return best;

    for (int2 nodeState = int2(leftChild(0), 0); nodeState.x != 0;) {
        // from parent
        if (nodeState.y == 0) {
            if (!isRayIntersectsAABB(ray, bvhNode[nodeState.x].aabb))
                nodeState = int2(sibling(nodeState.x), 1);
            else if (!isLeaf(nodeState.x))
                nodeState = int2(leftChild(nodeState.x), 0);
            else {
                Intsec intsec = bestBVHLeafIntersection(ray, bvhNode[nodeState.x]);
                if (intsec.t < best.t)
                    best = intsec;

                nodeState = int2(sibling(nodeState.x), 1);
            }
        }
        // from sibling
        else if (nodeState.y == 1) {
            if (!isRayIntersectsAABB(ray, bvhNode[nodeState.x].aabb))
                nodeState = int2(parent(nodeState.x), 2);
            else if (!isLeaf(nodeState.x))
                nodeState = int2(leftChild(nodeState.x), 0);
            else {
                Intsec intsec = bestBVHLeafIntersection(ray, bvhNode[nodeState.x]);
                if (intsec.t < best.t)
                    best = intsec;

                nodeState = int2(parent(nodeState.x), 2);
            }
        }
        // from child
        else if (nodeState.y == 2) {
            if (nodeState.x == leftChild(parent(nodeState.x)))
                nodeState = int2(sibling(nodeState.x), 1);
            else
                nodeState = int2(parent(nodeState.x), 2);
        }
    }

    return best;
}

[numthreads(1, 1, 1)]
void cs(uint3 DTid: SV_DispatchThreadID)
{
    Ray ray = generateRay(DTid.xy);

    //Intsec best = naiveIntersection(ray);
    //Intsec best = bvhIntersection(ray);
    Intsec best = bvhStacklessIntersection(ray);

    if (best.t <= whnf.z || whnf.w <= best.t)
        return;

    // depth view
    {
        float depth = best.t * dot(ray.dir, camDir);

        float4 colorNear = float4(1.f, 1.f, 1.f, 1.f);
        float4 colorFar = float4(0.f, 0.f, 0.f, 1.f);

        float4 finalCl = lerp(colorNear, colorFar, 1.f - 1.f / depth);
    }

    Vertex v0 = vertices[indices[best.tId].x];
    Vertex v1 = vertices[indices[best.tId].y];
    Vertex v2 = vertices[indices[best.tId].z];

    float2 uv = float2(best.u, best.v);
    uv = (1 - uv.x - uv.y) * v0.uv.xy + uv.x * v1.uv.xy + uv.y * v2.uv.xy;

    float4 color = colorTexture.SampleLevel(
        colorSampler,
        float3(uv, modelBuffer[best.mId].shineSpeedTexIdNM.z),
        0
    );

    texOutput[DTid.xy] = color;

    //uint idx = best.mId;
    //uint flags = asuint(modelBuffer[idx].shineSpeedTexIdNM.w);

    //float4 cl = colorTexture.SampleLevel(
    //    colorSampler,
    //    float3(uv, modelBuffer[idx].shineSpeedTexIdNM.z)
    //    0
    //);
    //float4 finalCl = ambientColor * cl;

    //float3 normal = float3(0.f, 0.f, 0.f);
    //if (lightsBumpNormsCull.y > 0 && flags == 1)
    //{
    //    float4 tang0 = mul(modelBuffer[idx].normTangMatrix, v0.tangent);
    //    float4 tang1 = mul(modelBuffer[idx].normTangMatrix, v1.tangent);
    //    float4 tang2 = mul(modelBuffer[idx].normTangMatrix, v2.tangent);
    //    float4 tang = lerp(tang0, tang1, uv.x);
    //    tang = lerp(tang, tang2, uv.y);

    //    float4 norm0 = mul(modelBuffer[idx].normTangMatrix, v0.tangent);
    //    float4 norm1 = mul(modelBuffer[idx].normTangMatrix, v1.tangent);
    //    float4 norm2 = mul(modelBuffer[idx].normTangMatrix, v2.tangent);
    //    float4 norm = lerp(norm0, norm1, uv.x);
    //    norm = lerp(norm, norm2, uv.y);


    //    float3 binorm = 
    //}
}