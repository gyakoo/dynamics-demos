#define VERTEX_SHADER
#include "ShaderCommon.hlsli"

cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
    matrix model;
    matrix view;
    matrix projection;
};

PSInput_PositionNormalTexture main(VSInput_PositionNormalTexture input)
{
    PSInput_PositionNormalTexture output;
    float4 pos = float4(input.pos, 1.0f);

    // Transform the vertex position into projected space.
    pos = mul(pos, model);
    output.opos = pos.xyz;
    pos = mul(pos, view);
    pos = mul(pos, projection);
    output.pos = pos;
    output.normal = mul(input.normal, (float3x3)model);
    output.uv = input.uv;

    return output;
}