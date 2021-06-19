//
// Copyright 2020-2021 Roman Skabin
//

#include "core/pch.h"
#include "asset_manager/asset_manager.h"
#include "core/memory.h"
#include "math/math.h"
#include <vcruntime_new.h>

namespace REV
{

REV_GLOBAL AssetManager *g_AssetManager = null;

// @NOTE(Roman): Return need to be freed
REV_INTERNAL const char *ReadEntireFile(const char *filename, Allocator *allocator, u64 *length)
{
    // @TODO(Roman): #CrossPlatform. tools/file
    HANDLE file = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, null, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY | FILE_FLAG_SEQUENTIAL_SCAN, null);
    REV_CHECK(file != INVALID_HANDLE_VALUE);

    u64 size = 0;
    REV_DEBUG_RESULT(GetFileSizeEx(file, cast<LARGE_INTEGER *>(&size)));
    *length = size;

    char *buffer = allocator->Alloc<char>(size + 1);
    for (u64 bytes_read = 0; size; )
    {
        u32 bytes_to_read = Math::clamp<u32, u64>(size, 0, REV_U32_MAX);
        REV_DEBUG_RESULT(ReadFile(file, buffer + bytes_read, bytes_to_read, null, null));

        bytes_read += bytes_to_read;
        size       -= bytes_to_read;
    }
    buffer[*length] = '\0';

    REV_DEBUG_RESULT(CloseHandle(file));
    return buffer;
}

AssetManager *AssetManager::Create(Allocator *allocator, const StaticString<REV_PATH_CAPACITY>& REVAM_filename)
{
    REV_CHECK_M(!g_AssetManager, "Asset manager is already created. Use AssetManager::Get() function instead");
    g_AssetManager = new (Memory::Get()->PushToPA<AssetManager>()) AssetManager(allocator, REVAM_filename);
    return g_AssetManager;
}

AssetManager *AssetManager::Get()
{
    REV_CHECK_M(g_AssetManager, "Asset manager is not created yet");
    return g_AssetManager;
}

AssetManager::AssetManager(Allocator *allocator, const StaticString<REV_PATH_CAPACITY>& REVAM_filename)
    : m_Allocator(allocator),
      m_UserREVAMFileName(REVAM_filename),
      m_UserREVAMStream(null),
      m_StaticArea(allocator),
      m_SceneArea(allocator)
{
    u64 length = 0;
    m_UserREVAMStream = ReadEntireFile(m_UserREVAMFileName.Data(), m_Allocator, &length);
    ParseREVAMFile();
}

AssetManager::~AssetManager()
{
    m_Allocator->DeAlloc(m_UserREVAMStream);

    for (Asset& asset : m_StaticArea)
    {
        m_Allocator->DeAlloc(asset.names);
    }
}

void AssetManager::ParseTexture(Asset *asset, const char *_filename, u64 filename_length)
{
    Memory *memory = Memory::Get();

    const char *filename = memory->PushToTA<const char>(filename_length + 1);
    CopyMemory(cast<char *>(filename), _filename, filename_length);

    u64 dot_index = ConstString(filename, filename_length).RFind('.');
    if (!memcmp(filename + dot_index, ".dds", REV_CSTRLEN(".dds")))
    {
        // @TODO(Roman): #CrossPlatform
        HANDLE file = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, null, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY | FILE_FLAG_SEQUENTIAL_SCAN, null);
        REV_CHECK(file != INVALID_HANDLE_VALUE);

        u64 data_size = 0;
        REV_DEBUG_RESULT(GetFileSizeEx(file, cast<LARGE_INTEGER *>(&data_size)));

        byte *data = memory->PushToTA<byte>(data_size);
        for (u64 bytes_read = 0, size = data_size; size; )
        {
            u32 bytes_to_read = Math::clamp<u32, u64>(size, 0, REV_U32_MAX);
            REV_DEBUG_RESULT(ReadFile(file, data + bytes_read, bytes_to_read, null, null));

            bytes_read += bytes_to_read;
            size       -= bytes_to_read;
        }

        REV_DEBUG_RESULT(CloseHandle(file));

        CreateDDSTexture(asset, data, data_size);
    }
    // @TODO(Roman): Other texture file formats
    else
    {
        u64   texture_format_length = filename_length - dot_index;
        char *texture_format        = memory->PushToTA<char>(texture_format_length);

        for (u64 i = 0; i < texture_format_length; ++i)
        {
            char c = (filename + dot_index)[i];
            if ('a' <= c && c <= 'z') texture_format[i] = c - 'a' + 'A';
            else                      texture_format[i] = c;
        }

        REV_FAILED_M("Unsupported texture format: %.*s", texture_format_length, texture_format);
    }
}

void AssetManager::FreeSceneAssets()
{
    for (Asset& asset : m_SceneArea)
    {
        m_Allocator->DeAlloc(asset.names);
        // @TODO(Roman): Release scene specific assets
    }
}

}
