#cengine pipeline(blending, enabled)

#cengine shader(vertex, "Internal_WeightedBlendedOIT_OnRender_VS")

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

#cengine shader(pixel, "Internal_WeightedBlendedOIT_OnRender_PS")

cbuffer Constants : register(b0)
{
    float cFar;
};

struct PSOutput
{
    float4 sum : SV_Target1;
    float  mul : SV_Target2;
};

PSOutput PSMain(float4 pos : SV_Position, float4 tex : TEXCOORD)
{
    float4 color;
    // ...

    PSOutput output;
    output.sum = color * (cFar - tex.z);
    output.mul = color.a;
    return output;
}
