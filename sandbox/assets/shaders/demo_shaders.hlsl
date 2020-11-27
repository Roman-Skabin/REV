
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

float4 PSMain(VSOutput vs_output) : SV_Target
{
    float intense = 1.0f / (5*length(vs_output.tex - center));

    // return intense * float4(1.0f, 0.0f, 0.0f, 1.0f);
    return intense * vs_output.col;
}
