#include "SceneCB.h"

cbuffer CullParams : register(b1) {
    uint4 numShapes; // x - objects count
    float4 bbMin[50];
    float4 bbMax[50];
};

RWStructuredBuffer<uint> indirectArgs : register(u0);
RWStructuredBuffer<uint4> objectIds : register(u1);

bool IsBoxInside(in float4 frustum[6], in float3 bbMin, in float3 bbMax) {
    for (int i = 0; i < 6; i++) {
        float4 p = float4(
            frustum[i].x < 0 ? bbMin.x : bbMax.x,
            frustum[i].y < 0 ? bbMin.y : bbMax.y,
            frustum[i].z < 0 ? bbMin.z : bbMax.z,
            1.0
        );
        if (dot(p, frustum[i]) < 0.0f) {
            return false;
        }
    }

    return true;
}

[numthreads(64, 1, 1)]
void cs(uint3 globalThreadId : SV_DispatchThreadID) {
    if (globalThreadId.x >= numShapes.x) {
        return;
    }

    if (IsBoxInside(frustum, bbMin[globalThreadId.x].xyz, bbMax[globalThreadId.x].xyz)) {
        uint id = 0;
        InterlockedAdd(indirectArgs[1], 1, id); // Corresponds to instanceCount in DrawIndexedIndirect

        objectIds[id] = uint4(globalThreadId.x, 0, 0, 0);
    }
}
