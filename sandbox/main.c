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

typedef struct SandboxState
{
    Logger   logger;
    Triangle t1;
    Triangle t2;
} SandboxState;

internal VERTEX_SHADER(TriangleVS)
{
    Triangle *triangle = state->user_ponter;
    return m4_mul_v(triangle->proj, pos);
}

internal PIXEL_SHADER(Triangle1PS)
{
    return v4_1(1.0f, 0.0f, 0.0f, 1.0f);
}

internal PIXEL_SHADER(Triangle2PS)
{
    return v4_1(0.0f, 0.0f, 1.0f, 1.0f);
}

USER_CALLBACK(User_OnInit)
{
    SandboxState *sandbox_state = PushToPA(SandboxState, &engine_state->memory, 1);
    engine_state->user_ponter   = sandbox_state;

    sandbox_state->logger = CreateLogger("Sandbox Logger", "sandbox.log", LOG_TO_FILE | LOG_TO_DEBUG);
    Check(SetWindowTextA(engine_state->window.handle, WINDOW_TITLE));
#if RELEASE
    SetFullscreen(engine_state, true);
#endif
    SetViewport(engine_state, 0.1f, 1.0f);

    v2 size = v2s_to_v2(engine_state->window.size);

    sandbox_state->t1.p1   = v4_1(-size.w*0.5f, -size.h*0.5f, 0.13f, 1.0f);
    sandbox_state->t1.p2   = v4_1(        0.0f,  size.h*0.5f, 0.20f, 1.0f);
    sandbox_state->t1.p3   = v4_1( size.w*0.5f, -size.h*0.5f, 0.40f, 1.0f);
    sandbox_state->t1.VS   = TriangleVS;
    sandbox_state->t1.PS   = Triangle1PS;
    sandbox_state->t1.proj = m4_persp_lh_n0(-size.w, size.w, -size.h, size.h, 0.1f, 1.0f);
    // sandbox_state->t1.proj = m4_ortho_lh_n0(-size.w, size.w, -size.h, size.h, 0.1f, 1.0f);

    sandbox_state->t2.p1   = v4_1(-size.w*0.5f, -size.h*0.185f, 0.10f, 1.0f);
    sandbox_state->t2.p2   = v4_1(        0.0f,  size.h*0.185f, 0.23f, 1.0f);
    sandbox_state->t2.p3   = v4_1( size.w*0.5f, -size.h*0.185f, 0.27f, 1.0f);
    sandbox_state->t2.VS   = TriangleVS;
    sandbox_state->t2.PS   = Triangle2PS;
    sandbox_state->t2.proj = sandbox_state->t1.proj;

    float volumes[2] = { 1.0f, 1.0f };
    engine_state->audio.error = engine_state->audio.volume->lpVtbl->SetAllVolumes(engine_state->audio.volume, ArrayCount(volumes), volumes);
    Check(SUCCEEDED(engine_state->audio.error));
}

USER_CALLBACK(User_OnDestroy)
{
    SandboxState *sandbox_state = cast(SandboxState *, engine_state->user_ponter);
    DestroyLogger(&sandbox_state->logger);
}

USER_CALLBACK(User_OnUpdate)
{
    SandboxState *sandbox_state = cast(SandboxState *, engine_state->user_ponter);

    local f32 last_print_time;
    if (engine_state->timer.seconds - last_print_time >= 0.1)
    {
        f32 FPS = engine_state->timer.ticks_per_second / cast(f32, engine_state->timer.delta_ticks);

        char buffer[64];
        sprintf(buffer, WINDOW_TITLE" - FPS: %f - MSPF: %f", FPS, 1000.0f / FPS);

        Check(SetWindowTextA(engine_state->window.handle, buffer));

        last_print_time = engine_state->timer.seconds;
    }

    if (engine_state->input.keys[KEY_F11].pressed)
    {
        SetFullscreen(engine_state, !engine_state->window.fullscreened);
    }

    if (engine_state->window.resized)
    {
        v2 size = v2s_to_v2(engine_state->window.size);

        sandbox_state->t1.p1   = v4_1(-size.w*0.5f, -size.h*0.5f, 0.13f, 1.0f);
        sandbox_state->t1.p2   = v4_1(        0.0f,  size.h*0.5f, 0.20f, 1.0f);
        sandbox_state->t1.p3   = v4_1( size.w*0.5f, -size.h*0.5f, 0.40f, 1.0f);
        sandbox_state->t1.proj = m4_persp_lh_n0(-size.w, size.w, -size.h, size.h, 0.1f, 1.0f);

        sandbox_state->t2.p1   = v4_1(-size.w*0.5f, -size.h*0.185f, 0.10f, 1.0f);
        sandbox_state->t2.p2   = v4_1(        0.0f,  size.h*0.185f, 0.23f, 1.0f);
        sandbox_state->t2.p3   = v4_1( size.w*0.5f, -size.h*0.185f, 0.27f, 1.0f);
        sandbox_state->t2.proj = sandbox_state->t1.proj;
    }
}

USER_CALLBACK(User_OnRender)
{
    SandboxState *sandbox_state = cast(SandboxState *, engine_state->user_ponter);

    engine_state->user_ponter = &sandbox_state->t1;
    RenderTriangle(engine_state, sandbox_state->t1.p1, sandbox_state->t1.p2, sandbox_state->t1.p3, sandbox_state->t1.VS, sandbox_state->t1.PS);

    engine_state->user_ponter = &sandbox_state->t2;
    RenderTriangle(engine_state, sandbox_state->t2.p1, sandbox_state->t2.p2, sandbox_state->t2.p3, sandbox_state->t2.VS, sandbox_state->t2.PS);

    engine_state->user_ponter = sandbox_state;
}

AUDIO_CALLBACK(User_OnAudioPlay)
{
          u32 *      cur_sample  = samples;
	const u32 *const last_sample = samples + num_samples;

    enum { C4 = 262 };

	while (cur_sample < last_sample)
	{
        // first channel
        *cur_sample++ = C4;

        // second channel
        *cur_sample++ = C4;
	}
}
