cbuffer MVPMatrix : register(b0, space0)
{
    float4x4 cMVP;
    float3 center;
};

struct VSOutput
{
    float4 pos : SV_Position;
    float4 col : COLOR;
    float3 tex : TEXCOORD;
};

VSOutput VSMain(float4 pos : POSITION, float4 col : COLOR)
{
    VSOutput output;
    output.pos    = mul(cMVP, pos);
    output.col    = col;
    output.tex    = output.pos;
    return output;
}
