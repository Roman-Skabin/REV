//
// Pipeline state settings
//
#cengine blending(disable)  // default: disable
/* #cengine depth_test(enable) // default: enable
*/
//
// Vertex Shader settings (must be before hlsl shader code)
//
#cengine shader(vertex, "Rect_VS")               // means vertex shader below (must be first)
                                                 // name is optional
#cengine shader(entry_point, CustomVSEntryPoint) // optional. default: VSMain

struct VSOutput
{
    float4 pos : SV_Position;
    float4 col : COLOR;
};

struct VSInput
{
    float4 pos : POSITION;
    float4 col : COLOR;
};

VSOutput CustomVSEntryPoint(VSInput input)
{
    VSOutput output;
    output.pos = input.pos;
    output.col = input.col;
    return output;
}

//
// Pixel Shader settings (must be before hlsl shader code)
//
#cengine shader(pixel) // means pixel shader below (must be first)
                       // name is optional

float4 safe_lerp(float4 start, float4 end, float4 percent)
{
    percent = max(float4(0, 0, 0, 0), min(percent, float4(1, 1, 1, 1)));
    return (start + end) / percent;
}

#define INT_DEFINE    64
#define FLOAT_DEFINE  0.01
#define STRING_DEFINE "qwerty"

float4 PSMain(float4 pos : SV_Position, float4 col : COLOR) : SV_Target
{
    // return safe_lerp(0.0f, pos, col);
    return col;
}