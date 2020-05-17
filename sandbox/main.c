#include "cengine.h"
#include "math/color.h"

#define WINDOW_TITLE "Sandbox"

typedef struct Sandbox
{
    Logger       logger;
    AudioBuffer  audio;

    VertexBuffer vertex_buffer;
    IndexBuffer  index_buffer;

    GraphicsProgram program;
} Sandbox;

typedef struct Vertex
{
    v4 pos;
    v4 col;
} Vertex;

USER_CALLBACK(OnInit)
{
    Sandbox *sandbox    = PushToPA(Sandbox, engine->memory, 1);
    engine->user_ponter = sandbox;

    CreateLogger(&sandbox->logger, "Sandbox Logger", "../log/sandbox.log", LOG_TO_FILE);
    DebugResult(SetWindowTextA(engine->window.handle, WINDOW_TITLE));
#if RELEASE
    SetFullscreen(engine, true);
#endif

#if RELEASE
    sandbox->audio = LoadAudioFile(engine, "../assets/audio/audio.wav");
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
    sandbox->vertex_buffer = CreateVertexBuffer(engine, vertices, ArrayCount(vertices), sizeof(Vertex));

    GraphicsProgram_Create(engine, "../assets/shaders/shader.hlsl", 0, &sandbox->program);
}

USER_CALLBACK(OnDestroy)
{
    Sandbox *sandbox = cast(Sandbox *, engine->user_ponter);
    GraphicsProgram_Destroy(&sandbox->program);
    DestroyIndexBuffer(&sandbox->index_buffer);
    DestroyVertexBuffer(&sandbox->vertex_buffer);
    DestroyLogger(&sandbox->logger);
}

USER_CALLBACK(OnUpdate)
{
    Sandbox *sandbox = cast(Sandbox *, engine->user_ponter);

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

    if (engine->window.resized)
    {
        // ...
    }
}

USER_CALLBACK(OnRender)
{
    Sandbox *sandbox = cast(Sandbox *, engine->user_ponter);

    SetVertexBuffer(engine, &sandbox->vertex_buffer);
    GraphicsProgram_Bind(engine, &sandbox->program);
    // Set Root Signature stuff
    DrawVertices(engine, &sandbox->vertex_buffer);
}

SOUND_CALLBACK(OnSound)
{
    Sandbox     *sandbox = cast(Sandbox *, engine->user_ponter);
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

USER_MAIN()
{
    return EngineRun(instance, OnInit, OnDestroy, OnUpdate, OnRender, OnSound);
}
