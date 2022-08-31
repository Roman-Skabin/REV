//
// Types
//
cbuffer DemoSceneCB // : register(b0, space0)
{
    float4x4 cMVP;
    float4   cSunColor;
    float3   cCenter;
    uint     cEntityID;
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
    float2 tex_coord : TEXCOORD0;
};

struct PSOutput
{
    float4 color      : SV_Target0;
    uint   mouse_pick : SV_Target1;
};

//
// Textures
//
Texture2D WoodTexture; // : register(t0, space0);

//
// Samplers
//
SamplerState WoodTextureSampler; // : register(s0, space0);

//
// Functions
//
VSOutput VSMain(VSInput input)
{
    VSOutput output;
    output.position  = mul(cMVP, input.position);
    output.normal    = input.normal;
    output.tex_coord = input.tex_coord;
    return output;
}

PSOutput PSMain(VSOutput input)
{
    PSOutput output;
    output.color      = WoodTexture.Sample(WoodTextureSampler, input.tex_coord);
    output.mouse_pick = cEntityID;
    return output;
}
