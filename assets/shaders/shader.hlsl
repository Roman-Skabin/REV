//
// Pipeline state settings
//
#cengine blending(disable)  // default: disable. options: enable, disable.
#cengine depth_test(enable) // default: enable.  options: enable, disable.
#cengine cull_mode(none)    // default: none.    options: none, front, back.

//
// Vertex Shader settings (must be before hlsl shader code)
//
#cengine shader(vertex, "Rect_VS")               // means vertex shader below (must be first)
                                                 // name is optional
#cengine shader(entry_point, CustomVSEntryPoint) // optional. default: VSMain

// @CleanUp(Roman): remove when we'll be able to use SRVs and CBVs
float4x4 m4_persp_lh_fov(float aspect, float fov, float near, float far)
{
    float tanfov2 = tan(fov / 2.0f * 3.14f / 180.0f);
    return float4x4(
        1.0f/(aspect*tanfov2),         0.0f,                  0.0f,                        0.0f,
                         0.0f, 1.0f/tanfov2,                  0.0f,                        0.0f,
                         0.0f,         0.0f, (far+near)/(far-near), -(2.0f*near*far)/(far-near),
                         0.0f,         0.0f,                  1.0f,                        0.0f);
}

// @CleanUp(Roman): remove when we'll be able to use SRVs and CBVs
float4x4 WorldToCameraLH(float3 camera, float3 target, float3 up)
{
    float3 forward   = normalize(target - camera);
    float3 right     = normalize(cross(up, forward));
    float3 y_axis    = cross(forward, right);
    float3 translate = -float3(dot(right, camera),
                               dot(y_axis, camera),
                               dot(forward, camera));

    return float4x4(right.x, y_axis.x, forward.x, translate.x,
                    right.y, y_axis.y, forward.y, translate.y,
                    right.z, y_axis.z, forward.z, translate.z,
                       0.0f,     0.0f,      0.0f,        1.0f);
}

// @CleanUp(Roman): remove most of stuff when we'll be able to use SRVs and CBVs
struct VSOutput
{
    float4 pos : SV_Position;
    float4 col : COLOR;
};

VSOutput CustomVSEntryPoint(float4 pos : POSITION, float4 col : COLOR)
{
    float3 camera = float3( 0.35f, -0.4f, -1.0f);
    float3 target = float3(-0.80f,  0.2f,  0.0f);
    float3 up     = float3(-0.51f,  1.0f,  0.0f);

    float4x4 proj  = m4_persp_lh_fov(16.0f / 9.0f, 90.0f, 0.1f, 10.0f);
    float4x4 view  = WorldToCameraLH(camera, target, up);

    float4x4 VP  = mul(proj, view);

    VSOutput output;
    output.pos = mul(VP, pos);
    output.col = col;
    return output;
}

//
// Pixel Shader settings (must be before hlsl shader code)
//
#cengine shader(pixel) // means pixel shader below (must be first)
                       // name is optional

float4 PSMain(float4 pos : SV_Position, float4 col : COLOR) : SV_Target
{
    return col;
}
