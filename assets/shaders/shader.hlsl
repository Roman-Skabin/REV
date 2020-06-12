//
// Pipeline state settings
//
#cengine pipeline(blending, disabled)  // default: disabled. options: enabled, disabled.
#cengine pipeline(depth_test, enabled) // default: enabled.  options: enabled, disabled.
#cengine pipeline(cull_mode, none)     // default: none.     options: none, front, back.

//
// Vertex Shader settings (must be before hlsl shader code)
//
#cengine shader(vertex, 5.1)                     // means vertex shader below (must be first), default target is 5.0
#cengine shader(entry_point, CustomVSEntryPoint) // optional. default: VSMain

cbuffer MVPMatrix : register(b0, space0)
{
    float4x4 cMVP;
};

struct VSOutput
{
    float4 pos : SV_Position;
    float4 col : COLOR;
};

VSOutput CustomVSEntryPoint(float4 pos : POSITION, float4 col : COLOR)
{
    VSOutput output;
    output.pos = mul(cMVP, pos);
    output.col = col;
    return output;
}

//
// Pixel Shader settings (must be before hlsl shader code)
//
#cengine shader(pixel, 5.1) // means pixel shader below (must be first), default target is 5.0

float4 PSMain(float4 pos : SV_Position, float4 col : COLOR) : SV_Target0
{
    return col;
}
