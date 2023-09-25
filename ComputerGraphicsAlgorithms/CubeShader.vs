#include "SceneCB.h"

struct ModelBuffer {
    float4x4 model;
    float4x4 norm;
    // x - shininess
    // y - rotation speed
    // z - texture id
    // w - normal map presence
    float4 settings;
    // xyz - position
    // w - current angle
    float4 posAngle;
};

cbuffer ModelBufferInst : register (b1) {
    ModelBuffer modelBuffer[50];
};

cbuffer ModelBufferInstVis : register (b2) {
    uint4 ids[50];
};

struct VSInput {
    float3 pos : POSITION;
    float3 tang : TANGENT;
    float3 norm : NORMAL;
    float2 uv : TEXCOORD;

    unsigned int instanceId : SV_InstanceID;
};

struct VSOutput {
    float4 pos : SV_Position;
    float4 worldPos : POSITION;
    float3 tang : TANGENT;
    float3 norm : NORMAL;
    float2 uv : TEXCOORD;

    nointerpolation unsigned int instanceId : SV_InstanceID;
};

VSOutput vs(VSInput vertex) {
    VSOutput result;

    unsigned int idx = lightsBumpNormsCull.w == 1 ? ids[vertex.instanceId].x : vertex.instanceId;

    float4 worldPos = mul(modelBuffer[idx].model, float4(vertex.pos, 1.0));

    result.pos = mul(vp, worldPos);
    result.worldPos = worldPos;
    result.uv = vertex.uv;
    result.tang = mul(modelBuffer[idx].norm, float4(vertex.tang, 0)).xyz;
    result.norm = mul(modelBuffer[idx].norm, float4(vertex.norm, 0)).xyz;
    result.instanceId = vertex.instanceId;

    return result;
}
