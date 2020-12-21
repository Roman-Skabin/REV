
cbuffer CB_DemoScene : register(b0, space0)
{
    float4x4 cMVP;
    float4   cSunColor;
    float3   cCenter;
};

struct VSInput
{
    float4 position  : POSITION;
    float4 normal    : NORMAL;
    float2 tex_coord : TEXCOORD;
};

struct VSOutput
{
    float4 position  : SV_Position;
    float4 normal    : NORMAL;
    float3 world_pos : TEXCOORD0;
    float2 tex_coord : TEXCOORD1;
};
typedef VSOutput PSInput;

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    output.position  = mul(cMVP, input.position);
    output.normal    = input.normal;
    output.world_pos = output.position.xyz;
    output.tex_coord = input.tex_coord;
    return output;
}

float4 PSMain(PSInput input) : SV_Target
{
    float  intensity = 1.0f / (8 * length(input.world_pos - cCenter));
    return intensity * cSunColor;
}
