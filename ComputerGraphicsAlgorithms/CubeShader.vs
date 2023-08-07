cbuffer SceneBuffer : register (b0)
{
    float4x4 vp;
};

cbuffer GeomBuffer : register (b1)
{
    float4x4 m;
};

struct VSInput
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
};

struct VSOutput
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
};

VSOutput vs(VSInput vertex)
{
    VSOutput result;

    result.pos = mul(vp, mul(m, float4(vertex.pos, 1.0)));
    result.uv = vertex.uv;

    return result;
}
