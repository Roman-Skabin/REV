//
// Pipeline state settings
//
#cengine blending(disable)  // default: disable
#cengine depth_test(enable) // default: enable

//
// Vertex Shader settings (must be before hlsl shader code)
//
#cengine shader(vertex, "Rect_VS")               // means vertex shader below (must be first)
                                                 // name is optional
#cengine shader(entry_point, CustomVSEntryPoint) // optional. default: VSMain

// @CleanUp(Roman): remove when we'll be able to use SRVs and CBVs
float4x4 m4_persp_lh(float left, float right, float bottom, float top, float near, float far)
{
    return float4x4(
        2.0f*near/(right-left),                   0.0f,                  0.0f,                        0.0f,
                          0.0f, 2.0f*near/(top-bottom),                  0.0f,                        0.0f,
                          0.0f,                   0.0f, (far+near)/(far-near), -(2.0f*near*far)/(far-near),
                          0.0f,                   0.0f,                  1.0f,                        0.0f);
}

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
float4x4 m4_translation(float4 v)
{
    return float4x4(
        1.0f, 0.0f, 0.0f, v.x,
        0.0f, 1.0f, 0.0f, v.y,
        0.0f, 0.0f, 1.0f, v.z,
        0.0f, 0.0f, 0.0f, 1.0f);
}

// @CleanUp(Roman): remove when we'll be able to use SRVs and CBVs
float4x4 m4_translation(float x, float y, float z)
{
    return float4x4(
        1.0f, 0.0f, 0.0f, x,
        0.0f, 1.0f, 0.0f, y,
        0.0f, 0.0f, 1.0f, z,
        0.0f, 0.0f, 0.0f, 1.0f);
}

// @CleanUp(Roman): remove when we'll be able to use SRVs and CBVs
float4x4 m4_rotation_x(float angle_deg)
{
    float angle_rad = angle_deg * 3.14f / 180.0f;

    float s, c;
    sincos(angle_rad, s, c);

    return float4x4(1.0f, 0.0f, 0.0f, 0.0f,
                    0.0f,    c,   -s, 0.0f,
                    0.0f,    s,    c, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f);
}

// @CleanUp(Roman): remove when we'll be able to use SRVs and CBVs
float4x4 m4_rotation_y(float angle_deg)
{
    float angle_rad = angle_deg * 3.14f / 180.0f;

    float s, c;
    sincos(angle_rad, s, c);

    return float4x4(   c, 0.0f,    s, 0.0f,
                    0.0f, 1.0f, 0.0f, 0.0f,
                      -s, 0.0f,    c, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f);
}

// @CleanUp(Roman): remove when we'll be able to use SRVs and CBVs
float4x4 m4_rotation_z(float angle_deg)
{
    float angle_rad = angle_deg * 3.14f / 180.0f;

    float s, c;
    sincos(angle_rad, s, c);

    return float4x4(   c,   -s, 0.0f, 0.0f,
                       s,    c, 0.0f, 0.0f,
                    0.0f, 0.0f, 1.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f);
}

// @CleanUp(Roman): remove when we'll be able to use SRVs and CBVs
float4x4 m4_identity()
{
    return float4x4(1.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 1.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 1.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f);
}

// @CleanUp(Roman): remove when we'll be able to use SRVs and CBVs
float4x4 m4_scale(float x, float y, float z)
{
    return float4x4(   x, 0.0f, 0.0f, 0.0f,
                    0.0f,    y, 0.0f, 0.0f,
                    0.0f, 0.0f,    z, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f);
}

// @CleanUp(Roman): remove when we'll be able to use SRVs and CBVs
float4x4 CameraToWorld(float3 camera : POSITION, float3 center : POSITION)
{
    float3 up = float3(0.0f, 1.0f, 0.0f);

    float3 z_axis    = normalize(center - camera);
    float3 x_axis    = normalize(cross(up, z_axis));
    float3 y_axis    = normalize(cross(z_axis, x_axis));
    float3 translate = float3( dot(x_axis, camera),
                               dot(y_axis, camera),
                              -dot(z_axis, camera));

    return float4x4(x_axis.x, y_axis.x, z_axis.x, translate.x,
                    x_axis.y, y_axis.y, z_axis.y, translate.y,
                    x_axis.z, y_axis.z, z_axis.z, translate.z,
                        0.0f,     0.0f,     0.0f,        1.0f);
}

// row major multiplication
float4x4 m4_mul(float4x4 l, float4x4 r)
{
    float4x4 res = float4x4(0.0f, 0.0f, 0.0f, 0.0f,
                            0.0f, 0.0f, 0.0f, 0.0f,
                            0.0f, 0.0f, 0.0f, 0.0f,
                            0.0f, 0.0f, 0.0f, 0.0f);

    res[0][0] = l[0][0] * r[0][0] + l[0][1] * r[1][0] + l[0][2] * r[2][0] + l[0][3] * r[3][0];
    res[0][1] = l[0][0] * r[0][1] + l[0][1] * r[1][1] + l[0][2] * r[2][1] + l[0][3] * r[3][1];
    res[0][2] = l[0][0] * r[0][2] + l[0][1] * r[1][2] + l[0][2] * r[2][2] + l[0][3] * r[3][2];
    res[0][3] = l[0][0] * r[0][3] + l[0][1] * r[1][3] + l[0][2] * r[2][3] + l[0][3] * r[3][3];

    res[1][0] = l[1][0] * r[0][0] + l[1][1] * r[1][0] + l[1][2] * r[2][0] + l[1][3] * r[3][0];
    res[1][1] = l[1][0] * r[0][1] + l[1][1] * r[1][1] + l[1][2] * r[2][1] + l[1][3] * r[3][1];
    res[1][2] = l[1][0] * r[0][2] + l[1][1] * r[1][2] + l[1][2] * r[2][2] + l[1][3] * r[3][2];
    res[1][3] = l[1][0] * r[0][3] + l[1][1] * r[1][3] + l[1][2] * r[2][3] + l[1][3] * r[3][3];

    res[2][0] = l[2][0] * r[0][0] + l[2][1] * r[1][0] + l[2][2] * r[2][0] + l[2][3] * r[3][0];
    res[2][1] = l[2][0] * r[0][1] + l[2][1] * r[1][1] + l[2][2] * r[2][1] + l[2][3] * r[3][1];
    res[2][2] = l[2][0] * r[0][2] + l[2][1] * r[1][2] + l[2][2] * r[2][2] + l[2][3] * r[3][2];
    res[2][3] = l[2][0] * r[0][3] + l[2][1] * r[1][3] + l[2][2] * r[2][3] + l[2][3] * r[3][3];

    res[3][0] = l[3][0] * r[0][0] + l[3][1] * r[1][0] + l[3][2] * r[2][0] + l[3][3] * r[3][0];
    res[3][1] = l[3][0] * r[0][1] + l[3][1] * r[1][1] + l[3][2] * r[2][1] + l[3][3] * r[3][1];
    res[3][2] = l[3][0] * r[0][2] + l[3][1] * r[1][2] + l[3][2] * r[2][2] + l[3][3] * r[3][2];
    res[3][3] = l[3][0] * r[0][3] + l[3][1] * r[1][3] + l[3][2] * r[2][3] + l[3][3] * r[3][3];

    return res;
}

// row major multiplication
float4 m4_mul(float4x4 l, float4 r)
{
    return float4(l[0][0] * r.x + l[0][1] * r.y + l[0][2] * r.z + l[0][3] * r.w,
                  l[1][0] * r.x + l[1][1] * r.y + l[1][2] * r.z + l[1][3] * r.w,
                  l[2][0] * r.x + l[2][1] * r.y + l[2][2] * r.z + l[2][3] * r.w,
                  l[3][0] * r.x + l[3][1] * r.y + l[3][2] * r.z + l[3][3] * r.w);
}

// @CleanUp(Roman): remove most of stuff when we'll be able to use SRVs and CBVs
struct VSOutput
{
    float4 pos    : SV_Position;
    float4 col    : COLOR;
    float4 tex    : TEXCOORD;
    float4 camera : POSITION0;
    float4 light  : POSITION1;
    float4 sphere : POSITION2;
};

VSOutput CustomVSEntryPoint(float4 pos : POSITION, float4 col : COLOR)
{
    float4 camera = float4( 0.0f, 0.0f, -0.5f, 1.0f);
    float4 light  = float4(-0.4f, 0.4f, -0.4f, 1.0f);
    float3 center = float3( 0.0f, 0.0f,  0.7f);

    float4x4 proj  = m4_persp_lh_fov(1.0f / 1.0f, 90.0f, 0.1f, 10.0f);
    float4x4 view  = CameraToWorld(camera.xyz, center);
    float4x4 model = m4_identity();

    float4x4 MVP  = m4_mul(proj, m4_mul(view, model));

    VSOutput output;
    output.pos    = m4_mul(MVP, pos);
    output.col    = col;
    output.tex    = output.pos;
    output.camera = camera;
    output.light  = light;
    output.sphere = m4_mul(MVP, float4(center, 0.5f));
    return output;
}

//
// Pixel Shader settings (must be before hlsl shader code)
//
#cengine shader(pixel) // means pixel shader below (must be first)
                       // name is optional

#define STEPS_COUNT 64
#define EPSILON     0.001f

float GetDist(float3 ray, float4 sphere)
{
    float dist_to_sphere = length(ray - sphere.xyz) - sphere.w;
    return dist_to_sphere;
}

float3 GetNormal(float3 ray, float4 sphere)
{
    float  dist    = GetDist(ray, sphere);
    float2 epsilon = float2(EPSILON, 0.0f);
    float3 normal  = dist - float3(GetDist(ray - epsilon.xyy, sphere),
                                   GetDist(ray - epsilon.yxy, sphere),
                                   GetDist(ray - epsilon.yyx, sphere));
    return normalize(normal);
}

float3 GetLight(float3 ray, float4 sphere, float3 light_pos)
{
    float3 light  = normalize(light_pos - ray);
    float3 normal = GetNormal(ray, sphere);
    float3 diff   = dot(normal, light);
    return diff;
}

float3 RayMarch(float3 ray_origin, float3 ray_destination, float3 light_pos, float4 sphere)
{
    float3 ray_pos = ray_origin;
 
    for (int i = 0; i < STEPS_COUNT; ++i)
    {
        float dist = GetDist(ray_pos, sphere);

        if (dist > -ray_origin.z) break;

        if (dist < EPSILON)
        {
            return GetLight(ray_pos, sphere, light_pos);
        }

        ray_pos += dist * ray_destination;
    }

    return 0.0f;
}

struct PSInput
{
    float4 pos    : SV_Position;
    float4 col    : COLOR;
    float4 tex    : TEXCOORD;
    float4 camera : POSITION0;
    float4 light  : POSITION1;
    float4 sphere : POSITION2;
};

float4 PSMain(PSInput input) : SV_Target
{
#if 1
    float3 ray_origin = input.camera.xyz;
    float3 ray_dest   = normalize(input.tex.xyz);
    float3 color      = RayMarch(ray_origin, ray_dest, input.light.xyz, input.sphere);
    return float4(color, 1.0f);
#else
    return input.col;
#endif
}
