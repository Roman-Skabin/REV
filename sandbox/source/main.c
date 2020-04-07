#include "cengine.h"

#define WINDOW_TITLE "Sandbox"

typedef struct Triangle
{
    v4            p1;
    v4            p2;
    v4            p3;
    VertexShader *VS;
    PixelShader  *PS;
    m4            proj;
} Triangle;

typedef struct Sandbox
{
    Logger      logger;
    Triangle    t1;
    Triangle    t2;
    AudioBuffer audio;
} Sandbox;

// @CleanUp(Roman): after making sandbox thread safety delete gSandbox
global Sandbox *gSandbox;

internal VERTEX_SHADER(TriangleVS)
{
    Triangle *triangle = engine->user_ponter;
    return m4_mul_v(triangle->proj, pos);
}

internal PIXEL_SHADER(Triangle1PS)
{
    return v4_1(1.0f, 0.0f, 0.0f, 0.5f);
}

internal PIXEL_SHADER(Triangle2PS)
{
    return v4_1(0.0f, 0.0f, 1.0f, 0.5f);
}

USER_CALLBACK(User_OnInit)
{
    // @TODO(Roman): make sandbox thread safety
    Sandbox *sandbox    = PushToPA(Sandbox, engine->memory, 1);
    engine->user_ponter = sandbox;
    gSandbox            = sandbox;

    CreateLogger(&sandbox->logger, "Sandbox Logger", "../sandbox/sandbox.log", LOG_TO_FILE | LOG_TO_DEBUG);
    DebugResult(SetWindowTextA(engine->window.handle, WINDOW_TITLE));
#if RELEASE
    SetFullscreen(engine, true);
#endif
    SetViewport(engine, 0.1f, 1.0f);

    v2 size = v2s_to_v2(engine->window.size);

    sandbox->t1.p1   = v4_1(-size.w*0.5f, -size.h*0.5f, 0.13f, 1.0f);
    sandbox->t1.p2   = v4_1(        0.0f,  size.h*0.5f, 0.20f, 1.0f);
    sandbox->t1.p3   = v4_1( size.w*0.5f, -size.h*0.5f, 0.40f, 1.0f);
    sandbox->t1.VS   = TriangleVS;
    sandbox->t1.PS   = Triangle1PS;
    sandbox->t1.proj = m4_persp_lh_n0(-size.w, size.w, -size.h, size.h, 0.1f, 1.0f);
    // sandbox->t1.proj = m4_ortho_lh_n0(-size.w, size.w, -size.h, size.h, 0.1f, 1.0f);

    sandbox->t2.p1   = v4_1(-size.w*0.5f, -size.h*0.185f, 0.10f, 1.0f);
    sandbox->t2.p2   = v4_1(        0.0f,  size.h*0.185f, 0.30f, 1.0f);
    sandbox->t2.p3   = v4_1( size.w*0.5f, -size.h*0.185f, 0.27f, 1.0f);
    sandbox->t2.VS   = TriangleVS;
    sandbox->t2.PS   = Triangle2PS;
    sandbox->t2.proj = sandbox->t1.proj;

    sandbox->audio = LoadAudioFile(engine, "../sandbox/assets/audio.wav");
    SoundPlay(&engine->sound);
}

USER_CALLBACK(User_OnDestroy)
{
    Sandbox *sandbox = cast(Sandbox *, engine->user_ponter);
    DestroyLogger(&sandbox->logger);
}

USER_CALLBACK(User_OnUpdate)
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
        v2 size = v2s_to_v2(engine->window.size);

        sandbox->t1.p1   = v4_1(-size.w*0.5f, -size.h*0.5f, 0.13f, 1.0f);
        sandbox->t1.p2   = v4_1(        0.0f,  size.h*0.5f, 0.20f, 1.0f);
        sandbox->t1.p3   = v4_1( size.w*0.5f, -size.h*0.5f, 0.40f, 1.0f);
        sandbox->t1.proj = m4_persp_lh_n0(-size.w, size.w, -size.h, size.h, 0.1f, 1.0f);

        sandbox->t2.p1   = v4_1(-size.w*0.5f, -size.h*0.185f, 0.10f, 1.0f);
        sandbox->t2.p2   = v4_1(        0.0f,  size.h*0.185f, 0.30f, 1.0f);
        sandbox->t2.p3   = v4_1( size.w*0.5f, -size.h*0.185f, 0.27f, 1.0f);
        sandbox->t2.proj = sandbox->t1.proj;
    }
}

USER_CALLBACK(User_OnRender)
{
    Sandbox *sandbox = cast(Sandbox *, engine->user_ponter);
    
    engine->user_ponter = &sandbox->t1;
    // RenderOpaqueTriangle(engine, sandbox->t1.p1, sandbox->t1.p2, sandbox->t1.p3, sandbox->t1.VS, sandbox->t1.PS);
    RenderTranslucentTriangle(engine, sandbox->t1.p1, sandbox->t1.p2, sandbox->t1.p3, sandbox->t1.VS, sandbox->t1.PS);

    engine->user_ponter = &sandbox->t2;
    // RenderOpaqueTriangle(engine, sandbox->t2.p1, sandbox->t2.p2, sandbox->t2.p3, sandbox->t2.VS, sandbox->t2.PS);
    RenderTranslucentTriangle(engine, sandbox->t2.p1, sandbox->t2.p2, sandbox->t2.p3, sandbox->t2.VS, sandbox->t2.PS);

    engine->user_ponter = sandbox;
}

SOUND_CALLBACK(User_SoundCallback)
{
    Sandbox     *sandbox = gSandbox; // cast(Sandbox *, engine->user_ponter);
    AudioBuffer *audio   = &sandbox->audio;

    for (u32 i = 0; i < buffer->samples_count; ++i)
    {
        if (audio->samples_index >= audio->samples_count)
            audio->samples_index = 0;

        buffer->samples[i * buffer->channels_count    ] = audio->samples[audio->samples_index++];
        buffer->samples[i * buffer->channels_count + 1] = audio->samples[audio->samples_index++];
    }
}
