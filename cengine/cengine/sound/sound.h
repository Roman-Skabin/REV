//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"

#ifndef ENGINE_STATE_DEFINED
#define ENGINE_STATE_DEFINED
    typedef struct EngineState EngineState;
#endif

typedef struct SoundBuffer
{
    f32 *samples;
    u32  samples_count;
    u32  samples_per_second;
    u16  channels_count;
} SoundBuffer;

#define SOUND_CALLBACK(name) void name(EngineState *engine_state, SoundBuffer *buffer)
typedef SOUND_CALLBACK(SoundCallback);

typedef struct SoundStream
{
    IMMDevice            *device;
    IAudioClient         *client;
    IAudioRenderClient   *renderer;
    ISimpleAudioVolume   *volume;
    WAVEFORMATEXTENSIBLE  wave_format;
    HRESULT               error;
    b32                   pause;
} SoundStream;

inline void SoundPlay(SoundStream *stream)  { stream->pause = false; }
inline void SoundPause(SoundStream *stream) { stream->pause = true; }

typedef struct AudioBuffer
{
    f32 *samples;
    u32  samples_count;
    u32  samples_index;
    u16  channels_count;
} AudioBuffer;

CEXTERN AudioBuffer LoadAudioFile(EngineState *state, const char *filename);
