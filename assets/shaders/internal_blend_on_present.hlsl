// @NOTE(Roman): Currently dummy shaders.
//               Not used anywhere.

#cengine pipeline(blending, enabled)

#cengine shader(vertex, 5.0)

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

#cengine shader(pixel, 5.0)

Texture2D<float4> tSumRT : register(t0);
Texture2D<float4> tMulRT : register(t1);

float4 PSMain(float4 pos : SV_Position, float4 tex : TEXCOORD) : SV_Target0
{
    float4 sum = tSumRT.Load(tex.x, tex.y);
    float4 mul = tMulRT.Load(tex.x, tex.y);
    return float4(sum.rgb / sum.a, mul.a);
}
