//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "sound/sound.h"
#include "cengine.h"

void CreateAudioBuffer(
    in     Engine      *engine,
    in     const char  *filename,
    in_opt u64          filename_len,
    out    AudioBuffer *buffer)
{
    Check(engine);
    Check(filename);
    Check(buffer);

    if (!filename_len) filename_len = strlen(filename);

    wchar_t wfilename[MAX_PATH] = {0};
    DebugResult(MultiByteToWideChar(CP_ACP, 0, filename, cast<s32>(filename_len), wfilename, sizeof(wfilename)));

    IMFSourceReader *reader = 0;
    engine->sound.error = MFCreateSourceReaderFromURL(wfilename, 0, &reader);
    Check(SUCCEEDED(engine->sound.error));

    IMFMediaType *type = 0;
    engine->sound.error = MFCreateMediaType(&type);
    Check(SUCCEEDED(engine->sound.error));

    engine->sound.error = type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    Check(SUCCEEDED(engine->sound.error));

    engine->sound.error = type->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_Float);
    Check(SUCCEEDED(engine->sound.error));

    engine->sound.error = type->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, engine->sound.wave_format.Format.nChannels);
    Check(SUCCEEDED(engine->sound.error));

    engine->sound.error = type->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, engine->sound.wave_format.Format.nSamplesPerSec);
    Check(SUCCEEDED(engine->sound.error));

    engine->sound.error = type->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, engine->sound.wave_format.Format.nAvgBytesPerSec);
    Check(SUCCEEDED(engine->sound.error));

    engine->sound.error = type->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, engine->sound.wave_format.Format.nBlockAlign);
    Check(SUCCEEDED(engine->sound.error));

    engine->sound.error = type->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, engine->sound.wave_format.Format.wBitsPerSample);
    Check(SUCCEEDED(engine->sound.error));

    engine->sound.error = type->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, true);
    Check(SUCCEEDED(engine->sound.error));

    engine->sound.error = reader->SetCurrentMediaType(0, 0, type);
    Check(SUCCEEDED(engine->sound.error));

    u32  sample_buffer_cap  = engine->sound.wave_format.Format.nSamplesPerSec * (engine->sound.wave_format.Format.wBitsPerSample / 8);
    u32  sample_buffer_size = 0;
    u8  *sample_buffer      = engine->allocator.Alloc<u8>(sample_buffer_cap);

    while (true)
    {
        u32        stream_index = 0;
        u32        stream_flags = 0;
        s64        time_stamp   = 0;
        IMFSample *sample       = 0;

        engine->sound.error = reader->ReadSample(MF_SOURCE_READER_ANY_STREAM, 0, &stream_index, &stream_flags, &time_stamp, &sample);
        Check(SUCCEEDED(engine->sound.error));

        if (stream_flags & MF_SOURCE_READERF_ENDOFSTREAM) break;

        IMFMediaBuffer *media_buffer = 0;
        engine->sound.error = sample->ConvertToContiguousBuffer(&media_buffer);
        Check(SUCCEEDED(engine->sound.error));

        u32 media_buffer_size = 0;
        engine->sound.error = media_buffer->GetCurrentLength(&media_buffer_size);

        u32 new_buffer_size = sample_buffer_size + media_buffer_size;

        if (new_buffer_size > sample_buffer_cap)
        {
            sample_buffer_cap *= 2;

            if (sample_buffer_cap < new_buffer_size)
                sample_buffer_cap = new_buffer_size;

            sample_buffer = engine->allocator.ReAlloc(sample_buffer, sample_buffer_cap);
        }

        u8 *media_buffer_ptr = 0;
        engine->sound.error = media_buffer->Lock(&media_buffer_ptr, 0, 0);
        Check(SUCCEEDED(engine->sound.error));

        CopyMemory(sample_buffer + sample_buffer_size, media_buffer_ptr, media_buffer_size);
        sample_buffer_size += media_buffer_size;

        engine->sound.error = media_buffer->Unlock();
        Check(SUCCEEDED(engine->sound.error));

        media_buffer->Release();
        sample->Release();
    }
    
    buffer->samples        = cast<f32 *>(sample_buffer);
    buffer->samples_count  = sample_buffer_size / (engine->sound.wave_format.Format.wBitsPerSample / 8);
    buffer->channels_count = engine->sound.wave_format.Format.nChannels;

    reader->Release();
    type->Release();

    engine->logger.LogSuccess("AudioBuffer was created");
}

void DestroyAudioBuffer(
    in Engine      *engine,
    in AudioBuffer *buffer)
{
    Check(engine);
    Check(buffer);

    engine->allocator.DeAlloc(buffer->samples);
    buffer->samples_count  = 0;
    buffer->samples_index  = 0;
    buffer->channels_count = 0;

    engine->logger.LogInfo("AudioBuffer was destroyed");
}
