//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"

#ifndef ENGINE_DEFINED
#define ENGINE_DEFINED
    typedef struct Engine Engine;
#endif

typedef struct SoundBuffer
{
    f32 *samples;
    u32  samples_count;
    u32  samples_per_second;
    u16  channels_count;
} SoundBuffer;

#define SOUND_CALLBACK(name) void name(Engine *engine, SoundBuffer *buffer)
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

INLINE void SoundPlay(SoundStream *stream)  { stream->pause = false; }
INLINE void SoundPause(SoundStream *stream) { stream->pause = true; }

typedef struct AudioBuffer
{
    f32 *samples;
    u32  samples_count;
    u32  samples_index;
    u16  channels_count;
} AudioBuffer;

CENGINE_FUN AudioBuffer LoadAudioFile(Engine *engine, const char *filename);
