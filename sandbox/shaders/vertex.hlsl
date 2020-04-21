struct VS_OUTPUT
{
    float4 pos : SV_Position;
    float4 col : COLOR;
};

VS_OUTPUT main(float4 pos : POSITION, float4 col : COLOR)
{
    VS_OUTPUT output;
    output.pos = pos;
    output.col = col;
    return output;
}
