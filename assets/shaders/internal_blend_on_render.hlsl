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

cbuffer Constants : register(b0)
{
    float cFar;
};

struct PSOutput
{
    float4 sum : SV_Target1;
    float4 mul : SV_Target2;
};

PSOutput PSMain(float4 pos : SV_Position, float4 tex : TEXCOORD)
{
    float4 color;
    // ...

    PSOutput output;
    output.sum = color * (cFar - tex.z);
    output.mul = float4(0.0f, 0.0f, 0.0f, color.a);
    return output;
}
