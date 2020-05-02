#include "cengine.h"

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

    sandbox->audio = LoadAudioFile(engine, "../assets/audio/audio.wav");
#if RELEASE
    SoundPlay(&engine->sound);
#endif

    Vertex vertices[] =
    {
        { v4_1(-0.5f, -0.5f, 0.0f, 1.0f), v4_1(1.0f, 0.0f, 0.0f, 1.0f) },
        { v4_1(-0.5f,  0.5f, 0.0f, 1.0f), v4_1(0.0f, 1.0f, 0.0f, 1.0f) },
        { v4_1( 0.5f,  0.5f, 0.0f, 1.0f), v4_1(0.0f, 0.0f, 1.0f, 1.0f) },
        { v4_1( 0.5f, -0.5f, 0.0f, 1.0f), v4_1(0.0f, 0.0f, 0.0f, 1.0f) }
    };
    sandbox->vertex_buffer = CreateVertexBuffer(engine, vertices, ArrayCount(vertices), sizeof(Vertex));

    u32 indices[] =
    {
        0, 1, 2,
        0, 2, 3
    };
    sandbox->index_buffer = CreateIndexBuffer(engine, indices, ArrayCount(indices));

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

    GraphicsProgram_Bind(engine, &sandbox->program);
    SetVertexBuffer(engine, &sandbox->vertex_buffer);
    SetIndexBuffer(engine, &sandbox->index_buffer);
    // Set Root Signature stuff
    DrawIndices(engine, &sandbox->index_buffer);
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
