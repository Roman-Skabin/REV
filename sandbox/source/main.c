#include "cengine.h"

#define WINDOW_TITLE "Sandbox"

typedef struct Sandbox
{
    Logger      logger;
    Triangle    t1;
    Rect        ground;
    AudioBuffer audio;
    v4          camera;
    m4          proj;
} Sandbox;

USER_CALLBACK(User_OnInit)
{
    Sandbox *sandbox    = PushToPA(Sandbox, engine->memory, 1);
    engine->user_ponter = sandbox;

    CreateLogger(&sandbox->logger, "Sandbox Logger", "../sandbox/sandbox.log", LOG_TO_FILE);
    DebugResult(SetWindowTextA(engine->window.handle, WINDOW_TITLE));
#if RELEASE
    // SetFullscreen(engine, true);
#endif

    v2 size = v2s_to_v2(engine->window.size);

    sandbox->ground.vertices[0] = v4_1(-size.w, -size.h, 0.1f, 1.0f);
    sandbox->ground.vertices[1] = v4_1(-size.w, -size.h, 1.5f, 1.0f);
    sandbox->ground.vertices[2] = v4_1( size.w, -size.h, 1.5f, 1.0f);
    sandbox->ground.vertices[3] = v4_1( size.w, -size.h, 0.1f, 1.0f);
    sandbox->ground.model       = m4_identity();
    sandbox->ground.view        = m4_identity();

    sandbox->t1.vertices[0] = v4_1(-size.w*0.5f, -size.h*0.75f, 0.13f, 1.0f);
    sandbox->t1.vertices[1] = v4_1(        0.0f, -size.h*0.50f, 0.20f, 1.0f);
    sandbox->t1.vertices[2] = v4_1( size.w*0.5f, -size.h*0.75f, 0.40f, 1.0f);
    sandbox->t1.model       = m4_identity();
    sandbox->t1.view        = m4_identity();

    sandbox->proj = m4_persp_lh(-size.w, size.w, -size.h, size.h, 0.1f, 1.0f);

    sandbox->audio = LoadAudioFile(engine, "../sandbox/assets/audio.wav");
    // SoundPlay(&engine->sound);
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

        sandbox->ground.vertices[0] = v4_1(-size.w, -size.h, 0.1f, 1.0f);
        sandbox->ground.vertices[1] = v4_1(-size.w, -size.h, 1.5f, 1.0f);
        sandbox->ground.vertices[2] = v4_1( size.w, -size.h, 1.5f, 1.0f);
        sandbox->ground.vertices[3] = v4_1( size.w, -size.h, 0.1f, 1.0f);

        sandbox->t1.vertices[0] = v4_1(-size.w*0.5f, -size.h*0.75f, 0.13f, 1.0f);
        sandbox->t1.vertices[1] = v4_1(        0.0f,  size.h*0.50f, 0.20f, 1.0f);
        sandbox->t1.vertices[2] = v4_1( size.w*0.5f, -size.h*0.75f, 0.40f, 1.0f);

        sandbox->proj = m4_persp_lh(-size.w, size.w, -size.h, size.h, 0.1f, 1.0f);
    }
    
    v4 dpos = v4_0(0.0f);

    if (engine->input.mouse.dpos.x || engine->input.mouse.dpos.y)
    {
        dpos.x = 0.2f * -engine->input.mouse.dpos.x * engine->timer.delta_seconds;
        dpos.y = 0.2f * -engine->input.mouse.dpos.y * engine->timer.delta_seconds;
    }
    else if (engine->input.keys[KEY_UP].down   || engine->input.keys[KEY_DOWN].down
         ||  engine->input.keys[KEY_LEFT].down || engine->input.keys[KEY_RIGHT].down)
    {
        /**/ if (engine->input.keys[KEY_UP].down)    dpos.y = -0.5f*engine->timer.delta_seconds;
        else if (engine->input.keys[KEY_DOWN].down)  dpos.y =  0.5f*engine->timer.delta_seconds;
        /**/ if (engine->input.keys[KEY_LEFT].down)  dpos.x =  0.5f*engine->timer.delta_seconds;
        else if (engine->input.keys[KEY_RIGHT].down) dpos.x = -0.5f*engine->timer.delta_seconds;
    }
    sandbox->t1.view = m4_mul(sandbox->t1.view, m4_translation_v(dpos));

    /**/ if (engine->input.keys[KEY_A].down) dpos.x = -engine->timer.delta_seconds;
    else if (engine->input.keys[KEY_D].down) dpos.x =  engine->timer.delta_seconds;
    /**/ if (engine->input.keys[KEY_W].down) dpos.z =  0.1f*engine->timer.delta_seconds;
    else if (engine->input.keys[KEY_S].down) dpos.z = -0.1f*engine->timer.delta_seconds;

    if (engine->input.keys[KEY_Q].down)
    {
        sandbox->t1.view = m4_mul(sandbox->t1.view, m4_rotation_z(f32_PI / 12.0f));
    }
    else if (engine->input.keys[KEY_E].down)
    {
        sandbox->t1.view = m4_mul(sandbox->t1.view, m4_rotation_z(-f32_PI / 12.0f));
    }

    if (engine->input.mouse.dwheel > 0)
    {
        sandbox->t1.view = m4_mul(sandbox->t1.view, m4_scaling(2.0f, 2.0f, 1.0f));
    }
    else if (engine->input.mouse.dwheel < 0)
    {
        sandbox->t1.view = m4_mul(sandbox->t1.view, m4_scaling(0.5f, 0.5f, 1.0f));
    }

    sandbox->t1.view = m4_mul(sandbox->t1.view, m4_translation_v(dpos));
}

USER_CALLBACK(User_OnRender)
{
    Sandbox *sandbox = cast(Sandbox *, engine->user_ponter);

    Rect *rects[] =
    {
        &sandbox->ground
    };
    DrawOpaqueRects(engine, rects, ArrayCount(rects));

    Triangle *triangles[] =
    {
        &sandbox->t1
    };
    DrawTranslucentTriangles(engine, triangles, ArrayCount(triangles));
}

SOUND_CALLBACK(User_SoundCallback)
{
    Sandbox     *sandbox = cast(Sandbox *, engine->user_ponter);
    AudioBuffer *audio   = &sandbox->audio;

    for (u32 i = 0; i < buffer->samples_count; ++i)
    {
        if (audio->samples_index >= audio->samples_count)
            audio->samples_index = 0;

        buffer->samples[i * buffer->channels_count    ] = audio->samples[audio->samples_index++];
        buffer->samples[i * buffer->channels_count + 1] = audio->samples[audio->samples_index++];
    }
}
