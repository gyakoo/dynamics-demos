#define PIXEL_SHADER
#include "ShaderCommon.hlsli"

Texture2D texDiffuse;
SamplerState texSampler;

cbuffer ModulateParamsCB
{
    float4 modulate;
};

// Basic PS for models. (half) Lambertian lighting model, Alpha Blending and Ambient
float4 main(PSInput_PositionNormalTexture input) : SV_TARGET
{
    const float3 toLight = normalize(float3(0, 1, 1));
    const float4 ambient = float4(0.01f,0.01f,0.01f,0.0f);
    const float numShads = 5.0f;

    float4 texColor = texDiffuse.Sample(texSampler, input.uv);
    const float halfL = 0.5f*(dot(input.normal, toLight) + 1.0f);
    texColor.rgb *= halfL;
    texColor.a = 1.0f;
    return float4(1, 1, 1, 1);// texColor;// *modulate + ambient;
}