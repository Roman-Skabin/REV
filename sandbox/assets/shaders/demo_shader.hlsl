cbuffer DemoSceneCB : register(b0, space0)
{
    float4x4 cMVP;
    float4   cSunColor;
    float3   cCenter;
    uint     cEntityID;
};

struct DemoSceneVB
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
typedef VSOutput PSInput;

Texture2D    WoodTexture    : register(t0, space0);
SamplerState TextureSampler : register(s0, space0);

VSOutput VSMain(DemoSceneVB vbuffer)
{
    VSOutput output;
    output.position  = mul(cMVP, vbuffer.position);
    output.normal    = vbuffer.normal;
    output.tex_coord = vbuffer.tex_coord;
    return output;
}

struct PSOutput
{
    float4 color      : SV_Target0;
    uint   mouse_pick : SV_Target1;
};

PSOutput PSMain(PSInput input)
{
    PSOutput output;
    output.color      = WoodTexture.Sample(TextureSampler, input.tex_coord);
    output.mouse_pick = cEntityID;
    return output;
}
