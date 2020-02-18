//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"

#ifndef ENGINE_STATE_DEFINED
#define ENGINE_STATE_DEFINED
    typedef struct EngineState EngineState;
#endif

typedef enum SAMPLE_TYPE
{
    SAMPLE_TYPE_U32,
    SAMPLE_TYPE_F32,
} SAMPLE_TYPE;

typedef struct SoundBuffer
{
    void         *samples;
    u32           samples_count;
    SAMPLE_TYPE   sample_type;
    u16           channels_count;
} SoundBuffer;

#define SOUND_CALLBACK(name) void name(EngineState *state, SoundBuffer *buffer)
typedef SOUND_CALLBACK(SoundCallback);

typedef struct SoundStream
{
    IMMDevice           *device;
    IAudioClient3       *client;
    IAudioRenderClient  *renderer;
    ISimpleAudioVolume  *volume;
    HRESULT              error;
    b32                  pause;
} SoundStream;

inline void SoundPlay(SoundStream *stream)  { stream->pause = false; }
inline void SoundPause(SoundStream *stream) { stream->pause = true; }
