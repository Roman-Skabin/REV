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
