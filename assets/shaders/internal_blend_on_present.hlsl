#cengine pipeline(blending, enabled)

#cengine shader(vertex, "Internal_WeightedBlendedOIT_OnPresent_VS")

struct VSOutput
{
    float4 pos : SV_Position;
    float4 tex : TEXCOORD;
};

VSOutput VSMain(float4 pos : POSITION, float4 tex : TEXCOORD)
{
    VSOutput output;
    output.pos = pos;
    output.tex = tex;
    return output;
}

#cengine shader(pixel, "Internal_WeightedBlendedOIT_OnPresent_PS")

Texture2D<float4> tSumRT : register(t0);
Texture2D<float4> tMulRT : register(t1);

float4 PSMain(float4 pos : SV_Position, float4 tex : TEXCOORD) : SV_Target0
{
    float4 sum = tSumRT.Load(tex.x, tex.y);
    float4 mul = tMulRT.Load(tex.x, tex.y);
    return float4(sum.rgb / sum.a, mul.a);
}
