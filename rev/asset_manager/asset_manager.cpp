//
// Copyright 2020-2021 Roman Skabin
//

#include "core/pch.h"
#include "asset_manager/asset_manager.h"
#include "core/settings.h"
#include "math/math.h"
#include "tools/static_string.hpp"
#include "core/work_queue.h"
#include "memory/memory.h"

namespace REV
{

REV_GLOBAL AssetManager *g_AssetManager = null;

// @TODO(Roman): #Tools, #MakePublic.
REV_INTERNAL ConstArray<byte> ReadEntireFileToTA(const char *filename)
{
    // @TODO(Roman): #CrossPlatform
    HANDLE file = CreateFileA(filename, GENERIC_READ, 0, null, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY | FILE_FLAG_SEQUENTIAL_SCAN, null);
    if (file != INVALID_HANDLE_VALUE)
    {
        u64 file_size = 0;
        REV_DEBUG_RESULT(GetFileSizeEx(file, cast<LARGE_INTEGER *>(&file_size)));

        byte *data = Memory::Get()->PushToFA<byte>(file_size + 1);

        u32 bytes_read = 0;
        for (u64 offset = 0; offset < file_size; offset += bytes_read)
        {
            u32 bytes_to_read = Math::clamp<u32, u64>(file_size - offset, 0, REV_U32_MAX);
            REV_DEBUG_RESULT(ReadFile(file, data + offset, bytes_to_read, &bytes_read, null));
        }

        REV_DEBUG_RESULT(CloseHandle(file));
        return ConstArray(data, file_size);
    }
    return null;
}

AssetManager *AssetManager::Create(Allocator *allocator, const Logger& logger)
{
    REV_CHECK_M(!g_AssetManager, "Asset manager is already created. Use AssetManager::Get() function instead");
    g_AssetManager = new (Memory::Get()->PushToPA<AssetManager>()) AssetManager(allocator, logger);
    return g_AssetManager;
}

AssetManager *AssetManager::Get()
{
    REV_CHECK_M(g_AssetManager, "Asset manager is not created yet");
    return g_AssetManager;
}

AssetManager::AssetManager(Allocator *allocator, const Logger& logger)
    : m_Allocator(allocator),
      m_StaticAssets(allocator),
      m_SceneAssets(allocator),
      m_Logger(logger, ConstString(REV_CSTR_ARGS("AssetManager logger")), Logger::TARGET::CONSOLE | Logger::TARGET::FILE),
      m_WorkQueue(m_Logger)
{
    m_Logger.LogSuccess("AssetManager has been created");
}

AssetManager::~AssetManager()
{
    FreeSceneAssets();

    for (Asset& asset : m_StaticAssets)
    {
        // @TODO(Roman): Release assets resoucres
    }

    m_Logger.LogInfo("AssetManager has been destroyed");
}

void AssetManager::FreeSceneAssets()
{
    for (Asset& asset : m_SceneAssets)
    {
        // @TODO(Roman): Release assets resoucres
    }
}

AssetHandle AssetManager::LoadTexture(const LoadTextureDesc& desc, bool _static)
{
    Array<Asset> *assets = _static ? &m_StaticAssets : &m_SceneAssets;

    StaticString<REV_PATH_CAPACITY> filename;
    MakeFilename(filename, ASSET_KIND_TEXTURE, desc.name);

    if (filename.Empty())
    {
        AssetHandle handle;
        handle._static = _static;
        return handle;
    }

    ConstArray<byte> data = ReadEntireFileToTA(filename.Data());

    Asset *asset = assets->PushBack();
    asset->kind                    = ASSET_KIND_TEXTURE;
    asset->texture.shader_register = desc.shader_register;
    asset->texture.register_space  = desc.register_space;
    asset->name                    = desc.name;

    u64 dot_index = filename.RFind('.');
    REV_CHECK(dot_index != filename.npos);

    ConstString extension(filename.Data() + dot_index, filename.Length() - dot_index);

    if (extension == ConstString(REV_CSTR_ARGS(".dds")))
    {
        LoadDDSTexture(asset, data, desc.name, _static);
    }
    // @TODO(Roman): Other texture file formats
    else
    {
        ConstString format = extension.SubString(1);
        REV_FAILED_M("Unsupported texture format: %.*s", format.Length(), format.Data());
    }

    AssetHandle handle;
    handle.index   = assets->Count() - 1;
    handle._static = _static;

    return handle;
}

AssetHandle AssetManager::LoadShader(const LoadShaderDesc& desc, bool _static)
{
    GPU::ShaderManager *shader_manager = GraphicsAPI::GetShaderManager();

    StaticString<REV_PATH_CAPACITY> filename;
    MakeFilename(filename, ASSET_KIND_SHADER, desc.name);

    StaticString<REV_PATH_CAPACITY> cache_filename;
    MakeCacheFilename(cache_filename, ASSET_KIND_SHADER, desc.name);

    if (filename.Empty() || cache_filename.Empty())
    {
        AssetHandle handle;
        handle._static = _static;
        return handle;
    }

    bool compile_shaders = false;

    // @TODO(Roman): #CrossPlatform
    HANDLE file = CreateFileA(filename.Data(), GENERIC_READ | GENERIC_WRITE, 0, null, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, null);
    REV_CHECK_M(file != INVALID_HANDLE_VALUE, "file '%.*s' does not exist", filename.Length(), filename.Data());

    HANDLE cache_file = CreateFileA(cache_filename.Data(), GENERIC_READ | GENERIC_WRITE, 0, null, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, null);
    if (cache_file == INVALID_HANDLE_VALUE)
    {
        cache_file = CreateFileA(cache_filename.Data(), GENERIC_READ | GENERIC_WRITE, 0, null, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, null);
        REV_CHECK(cache_file != INVALID_HANDLE_VALUE);

        compile_shaders = true;
    }
    else
    {
        FILETIME last_file_write_time = {0};
        REV_DEBUG_RESULT(GetFileTime(file, null, null, &last_file_write_time));

        FILETIME last_cache_file_write_time = {0};
        REV_DEBUG_RESULT(GetFileTime(cache_file, null, null, &last_cache_file_write_time));

        compile_shaders = *cast<u64 *>(&last_file_write_time) > *cast<u64 *>(&last_cache_file_write_time);
    }

    GPU::CompileShaderResult vs_cache;
    GPU::CompileShaderResult hs_cache;
    GPU::CompileShaderResult ds_cache;
    GPU::CompileShaderResult gs_cache;
    GPU::CompileShaderResult ps_cache;

    if (compile_shaders)
    {
        u64 file_size = 0;
        REV_DEBUG_RESULT(GetFileSizeEx(file, cast<LARGE_INTEGER *>(&file_size)));

        char *data = Memory::Get()->PushToFA<char>(file_size + 1);

        u32 bytes_read = 0;
        for (u64 offset = 0; offset < file_size; offset += bytes_read)
        {
            u64 bytes_rest    = file_size - offset;
            u32 bytes_to_read = Math::clamp<u32, u64>(bytes_rest, 0, REV_U32_MAX);

            REV_DEBUG_RESULT(ReadFile(file, data + offset, bytes_to_read, &bytes_read, null));
        }

        ConstString code(data, file_size);
        ConstString name(filename.Data(), filename.Length());

        volatile u64              shaders_count = 0;
        volatile GPU::SHADER_KIND shader_kind   = GPU::SHADER_KIND_UNKNOWN;
        m_Logger.LogInfo('"', name, "\" shaders are being compiled...");

        m_WorkQueue.AddWork([&vs_cache, shader_manager, &code, &name, &shaders_count, &shader_kind]
        {
            vs_cache = shader_manager->CompileShader(code, name, GPU::SHADER_KIND_VERTEX);
            if (vs_cache.blob)
            {
                _InterlockedIncrement64(cast<s64 *>(&shaders_count));
                _InterlockedOr((volatile long *)&shader_kind, GPU::SHADER_KIND_VERTEX);
            }
        });
        m_WorkQueue.AddWork([&hs_cache, shader_manager, &code, &name, &shaders_count, &shader_kind]
        {
            hs_cache = shader_manager->CompileShader(code, name, GPU::SHADER_KIND_HULL);
            if (hs_cache.blob)
            {
                _InterlockedIncrement64(cast<s64 *>(&shaders_count));
                _InterlockedOr((volatile long *)&shader_kind, GPU::SHADER_KIND_HULL);
            }
        });
        m_WorkQueue.AddWork([&ds_cache, shader_manager, &code, &name, &shaders_count, &shader_kind]
        {
            ds_cache = shader_manager->CompileShader(code, name, GPU::SHADER_KIND_DOMAIN);
            if (ds_cache.blob)
            {
                _InterlockedIncrement64(cast<s64 *>(&shaders_count));
                _InterlockedOr((volatile long *)&shader_kind, GPU::SHADER_KIND_DOMAIN);
            }
        });
        m_WorkQueue.AddWork([&gs_cache, shader_manager, &code, &name, &shaders_count, &shader_kind]
        {
            gs_cache = shader_manager->CompileShader(code, name, GPU::SHADER_KIND_GEOMETRY);
            if (gs_cache.blob)
            {
                _InterlockedIncrement64(cast<s64 *>(&shaders_count));
                _InterlockedOr((volatile long *)&shader_kind, GPU::SHADER_KIND_GEOMETRY);
            }
        });
        m_WorkQueue.AddWork([&ps_cache, shader_manager, &code, &name, &shaders_count, &shader_kind]
        {
            ps_cache = shader_manager->CompileShader(code, name, GPU::SHADER_KIND_PIXEL);
            if (ps_cache.blob)
            {
                _InterlockedIncrement64(cast<s64 *>(&shaders_count));
                _InterlockedOr((volatile long *)&shader_kind, GPU::SHADER_KIND_PIXEL);
            }
        });
        m_WorkQueue.Wait();

        m_Logger.LogInfo('"', name, "\" shaders' compilation has been done. ", shaders_count, "/5 shaders has been successfully compiled.");

        REV_DEBUG_RESULT(WriteFile(cache_file, (GPU::SHADER_KIND *)&shader_kind, sizeof(GPU::SHADER_KIND), null, null));

        u32 count = cast<u32>(vs_cache.bytecode.Count());
        {
            REV_DEBUG_RESULT(WriteFile(cache_file, &count,                   sizeof(count), null, null));
            REV_DEBUG_RESULT(WriteFile(cache_file, vs_cache.bytecode.Data(), count,         null, null));
        }
        if (count = cast<u32>(hs_cache.bytecode.Count()))
        {
            REV_DEBUG_RESULT(WriteFile(cache_file, &count,                   sizeof(count), null, null));
            REV_DEBUG_RESULT(WriteFile(cache_file, hs_cache.bytecode.Data(), count,         null, null));
        }
        if (count = cast<u32>(ds_cache.bytecode.Count()))
        {
            REV_DEBUG_RESULT(WriteFile(cache_file, &count,                   sizeof(count), null, null));
            REV_DEBUG_RESULT(WriteFile(cache_file, ds_cache.bytecode.Data(), count,         null, null));
        }
        if (count = cast<u32>(gs_cache.bytecode.Count()))
        {
            REV_DEBUG_RESULT(WriteFile(cache_file, &count,                   sizeof(count), null, null));
            REV_DEBUG_RESULT(WriteFile(cache_file, gs_cache.bytecode.Data(), count,         null, null));
        }
        count = cast<u32>(ps_cache.bytecode.Count());
        {
            REV_DEBUG_RESULT(WriteFile(cache_file, &count,                   sizeof(count), null, null));
            REV_DEBUG_RESULT(WriteFile(cache_file, ps_cache.bytecode.Data(), count,         null, null));
        }
    }

    REV_DEBUG_RESULT(CloseHandle(cache_file));
    REV_DEBUG_RESULT(CloseHandle(file));

    m_WorkQueue.AddWork([shader_manager, &vs_cache] { shader_manager->ReleaseCompiledShader(vs_cache); });
    m_WorkQueue.AddWork([shader_manager, &hs_cache] { shader_manager->ReleaseCompiledShader(hs_cache); });
    m_WorkQueue.AddWork([shader_manager, &ds_cache] { shader_manager->ReleaseCompiledShader(ds_cache); });
    m_WorkQueue.AddWork([shader_manager, &gs_cache] { shader_manager->ReleaseCompiledShader(gs_cache); });
    m_WorkQueue.AddWork([shader_manager, &ps_cache] { shader_manager->ReleaseCompiledShader(ps_cache); });
    m_WorkQueue.Wait();

    Array<Asset> *assets = _static ? &m_StaticAssets : &m_SceneAssets;

    Asset *asset = assets->PushBack();
    asset->kind          = ASSET_KIND_SHADER;
    asset->shader_handle = shader_manager->CreateGraphicsShader(ConstString(cache_filename.Data(), cache_filename.Length()), desc.textures, desc.cbuffers, desc.samplers, _static);
    asset->name          = desc.name;

    AssetHandle handle;
    handle.index   = assets->Count() - 1;
    handle._static = _static;

    return handle;
}

ConstArray<AssetHandle> AssetManager::LoadTextures(const ConstArray<LoadTextureDesc>& descs, bool _static)
{
    Array<Asset>        *assets             = _static ? &m_StaticAssets : &m_SceneAssets;
    u64                  new_assets_index   = assets->Count();
    Asset               *new_assets         = assets->PushBack(descs.Count());
    AssetHandle         *asset_handles      = Memory::Get()->PushToFA<AssetHandle>(descs.Count());

    u64 index = 0;
    for (const LoadTextureDesc& desc : descs)
    {
        m_WorkQueue.AddWork([this,
                             desc,
                             _static,
                             asset        = new_assets       + index,
                             asset_handle = asset_handles    + index,
                             asset_index  = new_assets_index + index]
        {
            StaticString<REV_PATH_CAPACITY> filename;
            MakeFilename(filename, ASSET_KIND_TEXTURE, desc.name);

            if (filename.Empty())
            {
                asset_handle->index   = REV_U64_MAX;
                asset_handle->_static = _static;
                return;
            }

            ConstArray<byte> data = ReadEntireFileToTA(filename.Data());

            asset->kind                    = ASSET_KIND_TEXTURE;
            asset->texture.shader_register = desc.shader_register;
            asset->texture.register_space  = desc.register_space;
            asset->name                    = desc.name;

            u64 dot_index = filename.RFind('.');
            REV_CHECK(dot_index != filename.npos);

            ConstString extension(filename.Data() + dot_index, filename.Length() - dot_index);

            if (extension == ConstString(REV_CSTR_ARGS(".dds")))
            {
                LoadDDSTexture(asset, data, desc.name, _static);
            }
            // @TODO(Roman): Other texture file formats
            else
            {
                ConstString format = extension.SubString(1);
                REV_FAILED_M("Unsupported texture format: %.*s", format.Length(), format.Data());
            }

            asset_handle->index   = asset_index;
            asset_handle->_static = _static;
        });

        ++index;
    }

    m_WorkQueue.Wait();

    return ConstArray(asset_handles, descs.Count());
}

ConstArray<AssetHandle> AssetManager::LoadShaders(const ConstArray<LoadShaderDesc>& descs, bool _static)
{
    GPU::ShaderManager  *shader_manager     = GraphicsAPI::GetShaderManager();
    Array<Asset>        *assets             = _static ? &m_StaticAssets : &m_SceneAssets;
    u64                  new_assets_index   = assets->Count();
    Asset               *new_assets         = assets->PushBack(descs.Count());
    AssetHandle         *asset_handles      = Memory::Get()->PushToFA<AssetHandle>(descs.Count());

    u64 index = 0;
    for (const LoadShaderDesc& desc : descs)
    {
        m_WorkQueue.AddWork([this,
                             desc,
                             shader_manager,
                             _static,
                             asset        = new_assets       + index,
                             asset_handle = asset_handles    + index,
                             asset_index  = new_assets_index + index]
        {
            StaticString<REV_PATH_CAPACITY> filename;
            MakeFilename(filename, ASSET_KIND_SHADER, desc.name);

            StaticString<REV_PATH_CAPACITY> cache_filename;
            MakeCacheFilename(cache_filename, ASSET_KIND_SHADER, desc.name);

            if (filename.Empty() || cache_filename.Empty())
            {
                asset_handle->index   = REV_U64_MAX;
                asset_handle->_static = _static;
                return;
            }

            bool compile_shaders = false;

            // @TODO(Roman): #CrossPlatform
            HANDLE file = CreateFileA(filename.Data(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, null, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, null);
            REV_CHECK_M(file != INVALID_HANDLE_VALUE, "file '%.*s' does not exist", filename.Length(), filename.Data());

            HANDLE cache_file = CreateFileA(cache_filename.Data(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, null, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, null);
            if (cache_file == INVALID_HANDLE_VALUE)
            {
                cache_file = CreateFileA(cache_filename.Data(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, null, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, null);
                REV_CHECK(cache_file != INVALID_HANDLE_VALUE);

                compile_shaders = true;
            }
            else
            {
                FILETIME last_file_write_time = {0};
                REV_DEBUG_RESULT(GetFileTime(file, null, null, &last_file_write_time));

                FILETIME last_cache_file_write_time = {0};
                REV_DEBUG_RESULT(GetFileTime(cache_file, null, null, &last_cache_file_write_time));

                compile_shaders = *cast<u64 *>(&last_file_write_time) > *cast<u64 *>(&last_cache_file_write_time);
            }

            GPU::CompileShaderResult vs_cache;
            GPU::CompileShaderResult hs_cache;
            GPU::CompileShaderResult ds_cache;
            GPU::CompileShaderResult gs_cache;
            GPU::CompileShaderResult ps_cache;

            if (compile_shaders)
            {
                u64 file_size = 0;
                REV_DEBUG_RESULT(GetFileSizeEx(file, cast<LARGE_INTEGER *>(&file_size)));

                char *data = Memory::Get()->PushToFA<char>(file_size + 1);

                u32 bytes_read = 0;
                for (u64 offset = 0; offset < file_size; offset += bytes_read)
                {
                    u64 bytes_rest    = file_size - offset;
                    u32 bytes_to_read = Math::clamp<u32, u64>(bytes_rest, 0, REV_U32_MAX);

                    REV_DEBUG_RESULT(ReadFile(file, data + offset, bytes_to_read, &bytes_read, null));
                }

                ConstString code(data, file_size);
                ConstString name(filename.Data(), filename.Length());

                u64              shaders_count = 0;
                GPU::SHADER_KIND shader_kind   = GPU::SHADER_KIND_UNKNOWN;

                m_Logger.LogInfo('"', name, "\" shaders are being compiled...");
                {
                    vs_cache = shader_manager->CompileShader(code, name, GPU::SHADER_KIND_VERTEX);
                    ++shaders_count;
                    shader_kind |= GPU::SHADER_KIND_VERTEX;
                }
                {
                    hs_cache = shader_manager->CompileShader(code, name, GPU::SHADER_KIND_HULL);
                    ++shaders_count;
                    shader_kind |= GPU::SHADER_KIND_HULL;
                }
                {
                    ds_cache = shader_manager->CompileShader(code, name, GPU::SHADER_KIND_DOMAIN);
                    ++shaders_count;
                    shader_kind |= GPU::SHADER_KIND_DOMAIN;
                }
                {
                    gs_cache = shader_manager->CompileShader(code, name, GPU::SHADER_KIND_GEOMETRY);
                    ++shaders_count;
                    shader_kind |= GPU::SHADER_KIND_GEOMETRY;
                }
                {
                    ps_cache = shader_manager->CompileShader(code, name, GPU::SHADER_KIND_PIXEL);
                    ++shaders_count;
                    shader_kind |= GPU::SHADER_KIND_PIXEL;
                }
                m_Logger.LogInfo('"', name, "\" shaders' compilation has been done. ", shaders_count, "/5 shaders has been successfully compiled.");

                REV_DEBUG_RESULT(WriteFile(cache_file, &shader_kind, sizeof(GPU::SHADER_KIND), null, null));

                u32 count = cast<u32>(vs_cache.bytecode.Count());
                {
                    REV_DEBUG_RESULT(WriteFile(cache_file, &count,                   sizeof(count), null, null));
                    REV_DEBUG_RESULT(WriteFile(cache_file, vs_cache.bytecode.Data(), count,         null, null));
                }
                if (count = cast<u32>(hs_cache.bytecode.Count()))
                {
                    REV_DEBUG_RESULT(WriteFile(cache_file, &count,                   sizeof(count), null, null));
                    REV_DEBUG_RESULT(WriteFile(cache_file, hs_cache.bytecode.Data(), count,         null, null));
                }
                if (count = cast<u32>(ds_cache.bytecode.Count()))
                {
                    REV_DEBUG_RESULT(WriteFile(cache_file, &count,                   sizeof(count), null, null));
                    REV_DEBUG_RESULT(WriteFile(cache_file, ds_cache.bytecode.Data(), count,         null, null));
                }
                if (count = cast<u32>(gs_cache.bytecode.Count()))
                {
                    REV_DEBUG_RESULT(WriteFile(cache_file, &count,                   sizeof(count), null, null));
                    REV_DEBUG_RESULT(WriteFile(cache_file, gs_cache.bytecode.Data(), count,         null, null));
                }
                count = cast<u32>(ps_cache.bytecode.Count());
                {
                    REV_DEBUG_RESULT(WriteFile(cache_file, &count,                   sizeof(count), null, null));
                    REV_DEBUG_RESULT(WriteFile(cache_file, ps_cache.bytecode.Data(), count,         null, null));
                }
            }

            REV_DEBUG_RESULT(CloseHandle(cache_file));
            REV_DEBUG_RESULT(CloseHandle(file));

            shader_manager->ReleaseCompiledShader(vs_cache);
            shader_manager->ReleaseCompiledShader(hs_cache);
            shader_manager->ReleaseCompiledShader(ds_cache);
            shader_manager->ReleaseCompiledShader(gs_cache);
            shader_manager->ReleaseCompiledShader(ps_cache);

            asset->kind          = ASSET_KIND_SHADER;
            asset->shader_handle = shader_manager->CreateGraphicsShader(ConstString(cache_filename.Data(), cache_filename.Length()), desc.textures, desc.cbuffers, desc.samplers, _static);
            asset->name          = desc.name;

            asset_handle->index   = asset_index;
            asset_handle->_static = _static;
        });

        ++index;
    }

    m_WorkQueue.Wait();

    return ConstArray(asset_handles, descs.Count());
}

AssetHandle AssetManager::FindAsset(const ConstString& name, bool _static) const
{
    const Array<Asset>& assets = _static ? m_StaticAssets : m_SceneAssets;

    AssetHandle handle;
    handle.index   = 0;
    handle._static = _static;

    for (const Asset& asset : assets)
    {
        if (asset.name == name)
        {
            return handle;
        }
        ++handle.index;
    }

    handle.index = REV_U64_MAX;
    return handle;
}

const Asset *AssetManager::GetAsset(const ConstString& name, bool _static) const
{
    const Array<Asset>& assets = _static ? m_StaticAssets : m_SceneAssets;
    for (const Asset& asset : assets)
    {
        if (asset.name == name)
        {
            return &asset;
        }
    }
    return null;
}

Asset *AssetManager::GetAsset(const ConstString& name, bool _static)
{
    Array<Asset>& assets = _static ? m_StaticAssets : m_SceneAssets;

    for (Asset& asset : assets)
    {
        if (asset.name == name)
        {
            return &asset;
        }
    }

    return null;
}

REV_INTERNAL void ChangeExtension(StaticString<REV_PATH_CAPACITY>& filename, const char *found_filename)
{
    ConstString found_name(found_filename, strlen(found_filename));

    u64 found_name_dot_index = found_name.RFind('.');
    REV_CHECK(found_name_dot_index != ConstString::npos);

    ConstString& extension = found_name.SubString(found_name_dot_index + 1);

    u64 filename_dot_index = filename.RFind('.');
    REV_CHECK(filename_dot_index != ConstString::npos);

    filename.Replace(filename_dot_index + 1, filename.Length(), extension);
};

void AssetManager::MakeFilename(StaticString<REV_PATH_CAPACITY>& filename, ASSET_KIND kind, const ConstString& asset_name)
{
    // @NOTE(Roman): asset_folder/asset_type/asset_name.extension
    filename = Settings::Get()->assets_folder;
    if (filename.Last() != '/') filename += '/';
    switch (kind)
    {
        case ASSET_KIND_TEXTURE: filename.PushBack(REV_CSTR_ARGS("textures/")); break;
        case ASSET_KIND_SHADER:  filename.PushBack(REV_CSTR_ARGS("shaders/"));  break;
        default:                 REV_FAILED_M("Wrong ASSET_KIND: %I32u", kind); break;
    }

    filename += asset_name;
    filename.PushBack(REV_CSTR_ARGS(".*"));

    // @TODO(Roman): #CrossPlatform
    WIN32_FIND_DATAA find_data   = {};
    HANDLE           find_handle = FindFirstFileA(filename.Data(), &find_data);
    if (find_handle == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_NOT_FOUND)
    {
        filename.Clear();
        switch (kind)
        {
            case ASSET_KIND_TEXTURE: m_Logger.LogError("Texture \"", asset_name, "\" is not found"); break;
            case ASSET_KIND_SHADER:  m_Logger.LogError("Shader \"", asset_name, "\" is not found");  break;
        }
    }
    else
    {
        ChangeExtension(filename, find_data.cFileName);

        if (kind == ASSET_KIND_SHADER)
        {
            ConstString expected_extension;
            switch (GraphicsAPI::GetAPI())
            {
                case GraphicsAPI::API::D3D12:  expected_extension = ConstString(REV_CSTR_ARGS(".hlsl")); break;
                case GraphicsAPI::API::VULKAN: expected_extension = ConstString(REV_CSTR_ARGS(".glsl")); break;
            }

            u32 last_error = 0;
            while (true)
            {
                u64 dot_index = filename.RFind('.');
                ConstString got_extension(filename.Data() + dot_index, filename.Length() - dot_index);

                if (got_extension != expected_extension)
                {
                    REV_DEBUG_RESULT(FindNextFileA(find_handle, &find_data));
                    last_error = GetLastError();

                    if (last_error == ERROR_SUCCESS)
                    {
                        ChangeExtension(filename, find_data.cFileName);
                        continue;
                    }

                    REV_CHECK(last_error == ERROR_NO_MORE_FILES);
                }

                break;
            }
        }
        REV_DEBUG_RESULT(FindClose(find_handle));
    }
}

void AssetManager::MakeCacheFilename(StaticString<REV_PATH_CAPACITY>& filename, ASSET_KIND kind, const ConstString& asset_name)
{
    // @NOTE(Roman): asset_folder/asset_type/cache/asset_name.extension
    filename = Settings::Get()->assets_folder;
    if (filename.Last() != '/') filename += '/';
    switch (kind)
    {
        case ASSET_KIND_SHADER: filename.PushBack(REV_CSTR_ARGS("shaders/cache/"));  break;
        default:                REV_FAILED_M("Cache exists only for shaders", kind); break;
    }

    // @TODO(Roman): #CrossPlatform
    BOOL result = CreateDirectoryA(filename.Data(), null);
    REV_CHECK_M(result || GetLastError() == ERROR_ALREADY_EXISTS, "One or more intermediate directories do not exist. Final directory path: \"%.*s\"", filename.Length(), filename.Data());

    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:  filename.PushBack(REV_CSTR_ARGS("hlsl_")); break;
        case GraphicsAPI::API::VULKAN: filename.PushBack(REV_CSTR_ARGS("glsl_")); break;
    }
    filename += asset_name;
    filename.PushBack(REV_CSTR_ARGS(".cso"));
}

}
