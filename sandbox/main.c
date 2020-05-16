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

    Vertex vertices[] =
    {
        // bottom
        { v4_1(-0.5f, -0.5f, 0.2f, 1.0f), gRedA1 },
        { v4_1(-0.5f, -0.5f, 1.2f, 1.0f), gRedA1 },
        { v4_1( 0.5f, -0.5f, 1.2f, 1.0f), gRedA1 },
        { v4_1(-0.5f, -0.5f, 0.2f, 1.0f), gRedA1 },
        { v4_1( 0.5f, -0.5f, 1.2f, 1.0f), gRedA1 },
        { v4_1( 0.5f, -0.5f, 0.2f, 1.0f), gRedA1 },

        // top
        { v4_1(-0.5f,  0.5f, 0.2f, 1.0f), gGreenA1 },
        { v4_1(-0.5f,  0.5f, 1.2f, 1.0f), gGreenA1 },
        { v4_1( 0.5f,  0.5f, 1.2f, 1.0f), gGreenA1 },
        { v4_1(-0.5f,  0.5f, 0.2f, 1.0f), gGreenA1 },
        { v4_1( 0.5f,  0.5f, 1.2f, 1.0f), gGreenA1 },
        { v4_1( 0.5f,  0.5f, 0.2f, 1.0f), gGreenA1 },

        // left
        { v4_1(-0.5f, -0.5f, 0.2f, 1.0f), gBlueA1 },
        { v4_1(-0.5f,  0.5f, 0.2f, 1.0f), gBlueA1 },
        { v4_1(-0.5f,  0.5f, 1.2f, 1.0f), gBlueA1 },
        { v4_1(-0.5f, -0.5f, 0.2f, 1.0f), gBlueA1 },
        { v4_1(-0.5f,  0.5f, 1.2f, 1.0f), gBlueA1 },
        { v4_1(-0.5f, -0.5f, 1.2f, 1.0f), gBlueA1 },

        // right
        { v4_1( 0.5f, -0.5f, 0.2f, 1.0f), gYellowA1 },
        { v4_1( 0.5f,  0.5f, 0.2f, 1.0f), gYellowA1 },
        { v4_1( 0.5f,  0.5f, 1.2f, 1.0f), gYellowA1 },
        { v4_1( 0.5f, -0.5f, 0.2f, 1.0f), gYellowA1 },
        { v4_1( 0.5f,  0.5f, 1.2f, 1.0f), gYellowA1 },
        { v4_1( 0.5f, -0.5f, 1.2f, 1.0f), gYellowA1 },

        // front
        { v4_1(-0.5f, -0.5f, 0.2f, 1.0f), gMagentaA1 },
        { v4_1(-0.5f,  0.5f, 0.2f, 1.0f), gMagentaA1 },
        { v4_1( 0.5f,  0.5f, 0.2f, 1.0f), gMagentaA1 },
        { v4_1(-0.5f, -0.5f, 0.2f, 1.0f), gMagentaA1 },
        { v4_1( 0.5f,  0.5f, 0.2f, 1.0f), gMagentaA1 },
        { v4_1( 0.5f, -0.5f, 0.2f, 1.0f), gMagentaA1 },

        // rear
        { v4_1(-0.5f, -0.5f, 1.2f, 1.0f), gCyanA1 },
        { v4_1(-0.5f,  0.5f, 1.2f, 1.0f), gCyanA1 },
        { v4_1( 0.5f,  0.5f, 1.2f, 1.0f), gCyanA1 },
        { v4_1(-0.5f, -0.5f, 1.2f, 1.0f), gCyanA1 },
        { v4_1( 0.5f,  0.5f, 1.2f, 1.0f), gCyanA1 },
        { v4_1( 0.5f, -0.5f, 1.2f, 1.0f), gCyanA1 },
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
