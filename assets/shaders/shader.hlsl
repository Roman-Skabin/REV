//
// Pipeline state settings
//
#cengine pipeline(blending, disabled)  // default: disabled. options: enabled, disabled.
#cengine pipeline(depth_test, enabled) // default: enabled.  options: enabled, disabled.
#cengine pipeline(cull_mode, none)     // default: none.     options: none, front, back.

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
    float3 z_axis    = normalize(target - camera);
    float3 x_axis    = normalize(cross(up, z_axis));
    float3 y_axis    = cross(z_axis, x_axis);
    float3 translate = -float3(dot(x_axis, camera),
                               dot(y_axis, camera),
                               dot(z_axis, camera));

    return float4x4(x_axis.x, y_axis.x, z_axis.x, translate.x,
                    x_axis.y, y_axis.y, z_axis.y, translate.y,
                    x_axis.z, y_axis.z, z_axis.z, translate.z,
                        0.0f,     0.0f,     0.0f,        1.0f);
}

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

float4 PSMain(float4 pos : SV_Position, float4 col : COLOR) : SV_Target0
{
    return col;
}
