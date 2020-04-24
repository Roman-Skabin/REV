#include "cengine.h"

#define WINDOW_TITLE "Sandbox"

typedef struct Sandbox
{
    Logger        logger;
    AudioBuffer   audio;
    VertexBuffer  vertex_buffer;
    IndexBuffer   index_buffer;
    Shader        vertex_shader;
    Shader        pixel_shader;
    PipelineStage rect_stage;
} Sandbox;

typedef struct Vertex
{
    v4 pos;
    v4 col;
} Vertex;

USER_CALLBACK(User_OnInit)
{
    Sandbox *sandbox    = PushToPA(Sandbox, engine->memory, 1);
    engine->user_ponter = sandbox;

    CreateLogger(&sandbox->logger, "Sandbox Logger", "../sandbox/sandbox.log", LOG_TO_FILE);
    DebugResult(SetWindowTextA(engine->window.handle, WINDOW_TITLE));
#if RELEASE
    SetFullscreen(engine, true);
#endif

    sandbox->audio = LoadAudioFile(engine, "../sandbox/assets/audio.wav");
    // SoundPlay(&engine->sound);

    sandbox->vertex_shader = CreateShader(engine, "../sandbox/shaders/vertex.hlsl", "main", SHADER_KIND_VERTEX);
    sandbox->pixel_shader  = CreateShader(engine, "../sandbox/shaders/pixel.hlsl", "main", SHADER_KIND_PIXEL);

    v2  size   = v2s_to_v2(engine->window.size);
    m4  proj   = m4_persp_lh(-size.w, size.w, -size.h, size.h, 0.1f, 1.0f);

    Vertex vertices[] =
    {
        // @Remove(Roman): hard-coded matrix multiplication yet we're not supporting SRVs
        //                 to pass projection matrix to the shader
        { m4_mul_v(proj, v4_1(-1.0f * size.w, -1.0f * size.h, 0.1f, 1.0f)), v4_1(1.0f, 0.0f, 0.0f, 1.0f) },
        { m4_mul_v(proj, v4_1(-1.0f * size.w, -1.0f * size.h, 1.0f, 1.0f)), v4_1(0.0f, 1.0f, 0.0f, 1.0f) },
        { m4_mul_v(proj, v4_1( 1.0f * size.w, -1.0f * size.h, 1.0f, 1.0f)), v4_1(0.0f, 0.0f, 1.0f, 1.0f) },
        { m4_mul_v(proj, v4_1( 1.0f * size.w, -1.0f * size.h, 0.1f, 1.0f)), v4_1(1.0f, 1.0f, 0.0f, 1.0f) }
    };
    sandbox->vertex_buffer = CreateVertexBuffer(engine, vertices, ArrayCount(vertices), sizeof(Vertex));

    u32 indecies[] =
    {
        0, 1, 2,
        0, 2, 3
    };
    sandbox->index_buffer = CreateIndexBuffer(engine, indecies, ArrayCount(indecies));

    ShaderArg shader_args[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0 },
        { "COLOR"   , 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0 }
    };

    CreatePipelineStage(engine,
                        &sandbox->vertex_shader,
                        0, 0, 0,
                        &sandbox->pixel_shader,
                        false,
                        ArrayCount(shader_args),
                        shader_args,
                        &sandbox->vertex_buffer,
                        &sandbox->index_buffer,
                        &sandbox->rect_stage);
}

USER_CALLBACK(User_OnDestroy)
{
    Sandbox *sandbox = cast(Sandbox *, engine->user_ponter);
    DestroyPipelineStage(&sandbox->rect_stage);
    DestroyIndexBuffer(&sandbox->index_buffer);
    DestroyVertexBuffer(&sandbox->vertex_buffer);
    DestroyShader(&sandbox->pixel_shader);
    DestroyShader(&sandbox->vertex_shader);
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

    // @Remove(Roman): Destraction of the vertex buffer yet we're not supporing SRVs
    //                 to pass projection matrix to the shader
    if (engine->window.resized)
    {
        DestroyPipelineStage(&sandbox->rect_stage);
        DestroyVertexBuffer(&sandbox->vertex_buffer);

        v2 size = v2s_to_v2(engine->window.size);
        m4 proj = m4_persp_lh(-size.w, size.w, -size.h, size.h, 0.1f, 1.0f);

        Vertex vertices[] =
        {
            // @Remove(Roman): hard-coded matrix multiplication yet we're not supporting SRVs
            //                 to pass projection matrix to the shader
            { m4_mul_v(proj, v4_1(-1.0f * size.w, -1.0f * size.h, 0.1f, 1.0f)), v4_1(1.0f, 0.0f, 0.0f, 1.0f) },
            { m4_mul_v(proj, v4_1(-1.0f * size.w, -1.0f * size.h, 1.0f, 1.0f)), v4_1(0.0f, 1.0f, 0.0f, 1.0f) },
            { m4_mul_v(proj, v4_1( 1.0f * size.w, -1.0f * size.h, 1.0f, 1.0f)), v4_1(0.0f, 0.0f, 1.0f, 1.0f) },
            { m4_mul_v(proj, v4_1( 1.0f * size.w, -1.0f * size.h, 0.1f, 1.0f)), v4_1(1.0f, 1.0f, 0.0f, 1.0f) }
        };
        sandbox->vertex_buffer = CreateVertexBuffer(engine, vertices, ArrayCount(vertices), sizeof(Vertex));

        ShaderArg shader_args[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0 },
            { "COLOR"   , 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0 }
        };

        CreatePipelineStage(engine,
                            &sandbox->vertex_shader,
                            0, 0, 0,
                            &sandbox->pixel_shader,
                            false,
                            ArrayCount(shader_args),
                            shader_args,
                            &sandbox->vertex_buffer,
                            &sandbox->index_buffer,
                            &sandbox->rect_stage);

    }
}

USER_CALLBACK(User_OnRender)
{
    Sandbox *sandbox = cast(Sandbox *, engine->user_ponter);
    RenderPipelineStage(engine, &sandbox->rect_stage);
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
