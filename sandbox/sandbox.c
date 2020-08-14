#include "cengine.h"
#include "math/mat.h"
#include "math/color.h"

#define WINDOW_TITLE "Sandbox"

// @TODO(Roman): Move to engine
typedef struct Camera
{
    v4 pos;
    v4 target;
    v4 up;
    m4 view;
    m4 proj;
} Camera;

typedef struct CENGINE_ALIGN(256) CBufferData
{
    m4 MVP;
} CBufferData;

typedef struct Sandbox
{
    Logger           logger;
    AudioBuffer      audio;
    Camera           camera;
    GraphicsProgram *graphics_program;
    GPUResource     *vertex_buffer;
} Sandbox;

typedef struct Vertex
{
    v4 pos;
    v4 col;
} Vertex;

internal USER_CALLBACK(OnInit)
{
    Timer profile_timer;
    StartProfiling("OnInitProfileTimer", CSTRLEN("OnInitProfileTimer"), &profile_timer);

    Sandbox *sandbox     = PushToPA(Sandbox, engine->memory, 1);
    engine->user_pointer = sandbox;

    CreateLogger("Sandbox Logger", "../log/sandbox.log", LOG_TO_FILE | LOG_TO_CONSOLE, &sandbox->logger);
    DebugResult(SetWindowTextA(engine->window.handle, WINDOW_TITLE));
#if RELEASE
    SetFullscreen(engine, true);
#endif

#if RELEASE
    CreateAudioBuffer(engine, "../assets/audio/audio.wav", &sandbox->audio);
    SoundPlay(&engine->sound);
#endif

    v4 cube_v0 = v4_1(-0.5, -0.5f, 0.2f, 1.0f);
    v4 cube_v1 = v4_1(-0.5, -0.5f, 1.2f, 1.0f);
    v4 cube_v2 = v4_1( 0.5, -0.5f, 1.2f, 1.0f);
    v4 cube_v3 = v4_1( 0.5, -0.5f, 0.2f, 1.0f);
    v4 cube_v4 = v4_1(-0.5,  0.5f, 0.2f, 1.0f);
    v4 cube_v5 = v4_1(-0.5,  0.5f, 1.2f, 1.0f);
    v4 cube_v6 = v4_1( 0.5,  0.5f, 1.2f, 1.0f);
    v4 cube_v7 = v4_1( 0.5,  0.5f, 0.2f, 1.0f);

    Vertex vertices[] =
    {
        // bottom
        { cube_v0, gYellowA1 },
        { cube_v1, gYellowA1 },
        { cube_v2, gYellowA1 },
        { cube_v0, gYellowA1 },
        { cube_v2, gYellowA1 },
        { cube_v3, gYellowA1 },

        // top
        { cube_v4, gGreenA1 },
        { cube_v5, gGreenA1 },
        { cube_v6, gGreenA1 },
        { cube_v4, gGreenA1 },
        { cube_v6, gGreenA1 },
        { cube_v7, gGreenA1 },

        // left
        { cube_v0, gRedA1 },
        { cube_v4, gRedA1 },
        { cube_v5, gRedA1 },
        { cube_v0, gRedA1 },
        { cube_v5, gRedA1 },
        { cube_v1, gRedA1 },

        // right
        { cube_v3, gMagentaA1 },
        { cube_v7, gMagentaA1 },
        { cube_v6, gMagentaA1 },
        { cube_v3, gMagentaA1 },
        { cube_v6, gMagentaA1 },
        { cube_v2, gMagentaA1 },

        // front
        { cube_v0, gBlueA1 },
        { cube_v4, gBlueA1 },
        { cube_v7, gBlueA1 },
        { cube_v0, gBlueA1 },
        { cube_v7, gBlueA1 },
        { cube_v3, gBlueA1 },

        // rear
        { cube_v1, gCyanA1 },
        { cube_v5, gCyanA1 },
        { cube_v6, gCyanA1 },
        { cube_v1, gCyanA1 },
        { cube_v6, gCyanA1 },
        { cube_v2, gCyanA1 },
    };
    sandbox->vertex_buffer = PushVertexBuffer(engine, ArrayCount(vertices), sizeof(Vertex));
    SetGPUResourceData(engine, sandbox->vertex_buffer, vertices);

    sandbox->graphics_program = AddGraphicsProgram(engine, "../assets/shaders/shader.hlsl", null);

    sandbox->camera.pos    = v4_1(0.0f, 0.0f, -1.0f, 1.0f);
    sandbox->camera.target = v4_1(0.0f, 0.0f,  0.7f, 1.0f);
    sandbox->camera.up     = v4_1(0.0f, 1.0f,  0.0f, 0.0f);
    sandbox->camera.view   = CameraToWorldLH(sandbox->camera.pos,
                                             sandbox->camera.target,
                                             sandbox->camera.up);
    sandbox->camera.proj   = m4_persp_lh_fov(16.0f / 9.0f, 90.0f, 0.1f, 10.0f);

    CBufferData cbuffer_data;
    cbuffer_data.MVP = m4_mul(sandbox->camera.proj, sandbox->camera.view);
    SetGPUResourceDataByName(engine, "MVPMatrix", CSTRLEN("MVPMatrix"), &cbuffer_data);

    StopProfiling(&profile_timer, &sandbox->logger);
}

internal USER_CALLBACK(OnDestroy)
{
    Sandbox *sandbox = cast(Sandbox *, engine->user_pointer);
    DestroyGraphicsProgram(sandbox->graphics_program);
#if RELEASE
    DestroyAudioBuffer(engine, &sandbox->audio);
#endif
    DestroyLogger(&sandbox->logger);
}

internal USER_CALLBACK(OnUpdateAndRender)
{
    Sandbox *sandbox = cast(Sandbox *, engine->user_pointer);

    local f32 last_print_time;
    if (engine->timer.seconds - last_print_time >= 0.1)
    {
        f32 FPS = engine->timer.ticks_per_second / cast(f32, engine->timer.delta_ticks);

        char buffer[64];
        sprintf(buffer, WINDOW_TITLE" - FPS: %f - MSPF: %f", FPS, 1000.0f / FPS);

        DebugResult(SetWindowTextA(engine->window.handle, buffer));

        last_print_time = engine->timer.seconds;
    }

    if (engine->input.keys[KEY_F11].pressed)
    {
        SetFullscreen(engine, !engine->window.fullscreened);
    }

    if (engine->input.keys[KEY_LEFT].pressed)
    {
        sandbox->camera.pos.x += 100.0f * engine->timer.delta_seconds;
        sandbox->camera.view = CameraToWorldLH(sandbox->camera.pos,
                                               sandbox->camera.target,
                                               sandbox->camera.up);
        CBufferData cbuffer_data;
        cbuffer_data.MVP = m4_mul(sandbox->camera.proj, sandbox->camera.view);
        SetGPUResourceDataByName(engine, "MVPMatrix", CSTRLEN("MVPMatrix"), &cbuffer_data);
    }
    else if (engine->input.keys[KEY_RIGHT].down)
    {
        sandbox->camera.pos.x += 100.0f * engine->timer.delta_seconds;
        sandbox->camera.view = CameraToWorldLH(sandbox->camera.pos,
                                               sandbox->camera.target,
                                               sandbox->camera.up);
        CBufferData cbuffer_data;
        cbuffer_data.MVP = m4_mul(sandbox->camera.proj, sandbox->camera.view);
        SetGPUResourceDataByName(engine, "MVPMatrix", CSTRLEN("MVPMatrix"), &cbuffer_data);
    }
    if (engine->input.keys[KEY_DOWN].down)
    {
        sandbox->camera.pos.y -= 100.0f * engine->timer.delta_seconds;
        sandbox->camera.view = CameraToWorldLH(sandbox->camera.pos,
                                               sandbox->camera.target,
                                               sandbox->camera.up);
        CBufferData cbuffer_data;
        cbuffer_data.MVP = m4_mul(sandbox->camera.proj, sandbox->camera.view);
        SetGPUResourceDataByName(engine, "MVPMatrix", CSTRLEN("MVPMatrix"), &cbuffer_data);
    }
    else if (engine->input.keys[KEY_UP].down)
    {
        sandbox->camera.pos.y += 100.0f * engine->timer.delta_seconds;
        sandbox->camera.view = CameraToWorldLH(sandbox->camera.pos,
                                               sandbox->camera.target,
                                               sandbox->camera.up);
        CBufferData cbuffer_data;
        cbuffer_data.MVP = m4_mul(sandbox->camera.proj, sandbox->camera.view);
        SetGPUResourceDataByName(engine, "MVPMatrix", CSTRLEN("MVPMatrix"), &cbuffer_data);
    }

    BindVertexBuffer(engine, sandbox->vertex_buffer);
    BindGraphicsProgram(engine, sandbox->graphics_program);

    DrawVertices(engine, sandbox->vertex_buffer);
}

internal SOUND_CALLBACK(OnSound)
{
    Sandbox     *sandbox = cast(Sandbox *, engine->user_pointer);
    AudioBuffer *audio   = &sandbox->audio;

    if (audio)
    {
        for (u32 i = 0; i < buffer->samples_count; ++i)
        {
            if (audio->samples_index >= audio->samples_count)
                audio->samples_index = 0;

            buffer->samples[i * buffer->channels_count    ] = audio->samples[audio->samples_index++];
            buffer->samples[i * buffer->channels_count + 1] = audio->samples[audio->samples_index++];
        }
    }
}

USER_MAIN(OnInit, OnDestroy, OnUpdateAndRender, OnSound);
