//
// PNT
//
struct VSInput_PositionNormalTexture
{
    float3 pos      : SV_POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD0;
};

struct PSInput_PositionNormalTexture
{
    float4 pos      : SV_POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD0;
    float3 opos     : TEXCOORD1;
};
