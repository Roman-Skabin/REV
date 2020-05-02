#pragma type(vertex)
#pragma name("Rect_VS")

struct VSOutput
{
    float4 pos : SV_Position;
    float4 col : COLOR;
};

VSOutput VSMain(float4 pos : POSITION, float4 col : COLOR)
{
    VSOutput output;
    output.pos = pos;
    output.col = col;
    return output;
}

#pragma type(pixel)
#pragma name("Rect_PS")

#define STEP_COUNT 64
#define STEP       0.01

float4 PSMain(float4 pos : SV_Position, float4 col : COLOR) : SV_Target
{
    return col;
}
