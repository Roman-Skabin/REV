// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#include "core/pch.h"
#include "asset_manager/asset_manager.h"
#include "core/settings.h"
#include "math/math.h"
#include "tools/static_string.hpp"
#include "core/work_queue.h"
#include "memory/memory.h"
#include "tools/file.h"

namespace REV
{

REV_GLOBAL AssetManager *g_AssetManager = null;

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
      m_Logger(logger, ConstString(REV_CSTR_ARGS("AssetManager")), Logger::TARGET_CONSOLE | Logger::TARGET_FILE),
      m_WorkQueue(m_Logger, Memory::Get()->PermanentArena())
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

AssetHandle AssetManager::LoadTexture(const ConstString& name, bool _static)
{
    Array<Asset> *assets = _static ? &m_StaticAssets : &m_SceneAssets;

    StaticString<REV_PATH_CAPACITY> filename;
    MakeFilename(filename, ASSET_KIND_TEXTURE, name);

    if (filename.Empty())
    {
        AssetHandle handle;
        handle._static = _static;
        return handle;
    }

    ConstArray<byte> data;
    {
        File texture_file(filename, FILE_FLAG_RES);

        u64   filedata_size = texture_file.Size();
        byte *filedata      = Memory::Get()->PushToFA<byte>(filedata_size + 1);

        texture_file.Read(filedata, filedata_size);

        data = ConstArray(filedata, filedata_size);
    }

    Asset *asset = assets->PushBack();
    asset->kind = ASSET_KIND_TEXTURE;
    asset->name = name;

    u64 dot_index = filename.RFind('.');
    REV_CHECK(dot_index != filename.npos);

    ConstString extension(filename.Data() + dot_index, filename.Length() - dot_index);

    if (extension == ConstString(REV_CSTR_ARGS(".dds")))
    {
        LoadDDSTexture(asset, data, name, _static);
    }
    // @TODO(Roman): Other texture file formats
    else
    {
        ConstString format = extension.SubString(1);
        REV_ERROR_M("Unsupported texture format: %.*s", format.Length(), format.Data());
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

    bool cache_exists = File::Exists(cache_filename);

    File file(filename, FILE_FLAG_RES);
    File cache_file(cache_filename, FILE_FLAG_RW | FILE_FLAG_SEQ | (cache_exists ? FILE_FLAG_EXISTS : FILE_FLAG_NEW));

    GPU::CompileShaderResult vs_cache;
    GPU::CompileShaderResult hs_cache;
    GPU::CompileShaderResult ds_cache;
    GPU::CompileShaderResult gs_cache;
    GPU::CompileShaderResult ps_cache;

    if (!cache_exists || file.LastWriteTime() > cache_file.LastWriteTime())
    {
        char *data = Memory::Get()->PushToFA<char>(file.Size() + 1);
        file.Read(data, file.Size());

        ConstString code(data, file.Size());
        ConstString name(filename.Data(), filename.Length());

        volatile u64              shaders_count = 0;
        volatile GPU::SHADER_KIND shader_kind   = GPU::SHADER_KIND_UNKNOWN;
        m_Logger.LogInfo('"', name, "\" shaders are being compiled...");

        m_WorkQueue.AddWork([&vs_cache, shader_manager, &code, &name, &shaders_count, &shader_kind]
        {
            vs_cache = shader_manager->CompileShader(code, name, GPU::SHADER_KIND_VERTEX);
            if (vs_cache.blob)
            {
                _InterlockedIncrement64(cast(s64 *, &shaders_count));
                _InterlockedOr(cast(volatile long *, &shader_kind), GPU::SHADER_KIND_VERTEX);
            }
        });
        m_WorkQueue.AddWork([&hs_cache, shader_manager, &code, &name, &shaders_count, &shader_kind]
        {
            hs_cache = shader_manager->CompileShader(code, name, GPU::SHADER_KIND_HULL);
            if (hs_cache.blob)
            {
                _InterlockedIncrement64(cast(s64 *, &shaders_count));
                _InterlockedOr(cast(volatile long *, &shader_kind), GPU::SHADER_KIND_HULL);
            }
        });
        m_WorkQueue.AddWork([&ds_cache, shader_manager, &code, &name, &shaders_count, &shader_kind]
        {
            ds_cache = shader_manager->CompileShader(code, name, GPU::SHADER_KIND_DOMAIN);
            if (ds_cache.blob)
            {
                _InterlockedIncrement64(cast(s64 *, &shaders_count));
                _InterlockedOr(cast(volatile long *, &shader_kind), GPU::SHADER_KIND_DOMAIN);
            }
        });
        m_WorkQueue.AddWork([&gs_cache, shader_manager, &code, &name, &shaders_count, &shader_kind]
        {
            gs_cache = shader_manager->CompileShader(code, name, GPU::SHADER_KIND_GEOMETRY);
            if (gs_cache.blob)
            {
                _InterlockedIncrement64(cast(s64 *, &shaders_count));
                _InterlockedOr(cast(volatile long *, &shader_kind), GPU::SHADER_KIND_GEOMETRY);
            }
        });
        m_WorkQueue.AddWork([&ps_cache, shader_manager, &code, &name, &shaders_count, &shader_kind]
        {
            ps_cache = shader_manager->CompileShader(code, name, GPU::SHADER_KIND_PIXEL);
            if (ps_cache.blob)
            {
                _InterlockedIncrement64(cast(s64 *, &shaders_count));
                _InterlockedOr(cast(volatile long *, &shader_kind), GPU::SHADER_KIND_PIXEL);
            }
        });
        m_WorkQueue.Wait();

        m_Logger.LogInfo('"', name, "\" shaders' compilation has been done. ", shaders_count, "/5 shaders has been successfully compiled.");

        cache_file.Write(cast(GPU::SHADER_KIND *, &shader_kind), sizeof(GPU::SHADER_KIND));

        u32 count = cast(u32, vs_cache.bytecode.Count());
        {
            cache_file.Write(&count, sizeof(count));
            cache_file.Write(vs_cache.bytecode.Data(), count);
        }
        if (count = cast(u32, hs_cache.bytecode.Count()))
        {
            cache_file.Write(&count, sizeof(count));
            cache_file.Write(hs_cache.bytecode.Data(), count);
        }
        if (count = cast(u32, ds_cache.bytecode.Count()))
        {
            cache_file.Write(&count, sizeof(count));
            cache_file.Write(ds_cache.bytecode.Data(), count);
        }
        if (count = cast(u32, gs_cache.bytecode.Count()))
        {
            cache_file.Write(&count, sizeof(count));
            cache_file.Write(gs_cache.bytecode.Data(), count);
        }
        count = cast(u32, ps_cache.bytecode.Count());
        {
            cache_file.Write(&count, sizeof(count));
            cache_file.Write(ps_cache.bytecode.Data(), count);
        }
    }

    cache_file.Close();
    file.Close();

    m_WorkQueue.AddWork([shader_manager, &vs_cache] { shader_manager->ReleaseCompiledShader(vs_cache); });
    m_WorkQueue.AddWork([shader_manager, &hs_cache] { shader_manager->ReleaseCompiledShader(hs_cache); });
    m_WorkQueue.AddWork([shader_manager, &ds_cache] { shader_manager->ReleaseCompiledShader(ds_cache); });
    m_WorkQueue.AddWork([shader_manager, &gs_cache] { shader_manager->ReleaseCompiledShader(gs_cache); });
    m_WorkQueue.AddWork([shader_manager, &ps_cache] { shader_manager->ReleaseCompiledShader(ps_cache); });
    m_WorkQueue.Wait();

    Array<Asset> *assets = _static ? &m_StaticAssets : &m_SceneAssets;

    Asset *asset = assets->PushBack();
    asset->kind   = ASSET_KIND_SHADER;
    asset->shader = shader_manager->CreateGraphicsShader(cache_filename.ToConstString(), desc.resources, _static);
    asset->name   = desc.name;

    AssetHandle handle;
    handle.index   = assets->Count() - 1;
    handle._static = _static;

    return handle;
}

ConstArray<AssetHandle> AssetManager::LoadTextures(const ConstArray<ConstString>& names, bool _static)
{
    Array<Asset> *assets           = _static ? &m_StaticAssets : &m_SceneAssets;
    u64           new_assets_index = assets->Count();
    Asset        *new_assets       = assets->PushBack(names.Count());
    AssetHandle  *asset_handles    = Memory::Get()->PushToFA<AssetHandle>(names.Count());

    u64 index = 0;
    for (const ConstString& name : names)
    {
        m_WorkQueue.AddWork([this,
                             name,
                             _static,
                             asset        = new_assets       + index,
                             asset_handle = asset_handles    + index,
                             asset_index  = new_assets_index + index]
        {
            StaticString<REV_PATH_CAPACITY> filename;
            MakeFilename(filename, ASSET_KIND_TEXTURE, name);

            if (filename.Empty())
            {
                asset_handle->index   = REV_U64_MAX;
                asset_handle->_static = _static;
                return;
            }

            ConstArray<byte> data;
            {
                File texture_file(filename, FILE_FLAG_RES);

                u64   filedata_size = texture_file.Size();
                byte *filedata      = Memory::Get()->PushToFA<byte>(filedata_size + 1);

                texture_file.Read(filedata, filedata_size);

                data = ConstArray(filedata, filedata_size);
            }

            asset->kind = ASSET_KIND_TEXTURE;
            asset->name = name;

            u64 dot_index = filename.RFind('.');
            REV_CHECK(dot_index != filename.npos);

            ConstString extension(filename.Data() + dot_index, filename.Length() - dot_index);

            if (extension == ConstString(REV_CSTR_ARGS(".dds")))
            {
                LoadDDSTexture(asset, data, name, _static);
            }
            // @TODO(Roman): Other texture file formats
            else
            {
                ConstString format = extension.SubString(1);
                REV_ERROR_M("Unsupported texture format: %.*s", format.Length(), format.Data());
            }

            asset_handle->index   = asset_index;
            asset_handle->_static = _static;
        });

        ++index;
    }

    m_WorkQueue.Wait();

    return ConstArray(asset_handles, names.Count());
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

            bool cache_exists = File::Exists(cache_filename);

            File file(filename, FILE_FLAG_RES);
            File cache_file(cache_filename, FILE_FLAG_RW | FILE_FLAG_SEQ | (cache_exists ? FILE_FLAG_EXISTS : FILE_FLAG_NEW));

            GPU::CompileShaderResult vs_cache;
            GPU::CompileShaderResult hs_cache;
            GPU::CompileShaderResult ds_cache;
            GPU::CompileShaderResult gs_cache;
            GPU::CompileShaderResult ps_cache;

            if (!cache_exists || file.LastWriteTime() > cache_file.LastWriteTime())
            {
                char *data = Memory::Get()->PushToFA<char>(file.Size() + 1);
                file.Read(data, file.Size());

                ConstString code(data, file.Size());
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

                cache_file.Write(&shader_kind, sizeof(GPU::SHADER_KIND));

                u32 count = cast(u32, vs_cache.bytecode.Count());
                {
                    cache_file.Write(&count, sizeof(count));
                    cache_file.Write(vs_cache.bytecode.Data(), count);
                }
                if (count = cast(u32, hs_cache.bytecode.Count()))
                {
                    cache_file.Write(&count, sizeof(count));
                    cache_file.Write(hs_cache.bytecode.Data(), count);
                }
                if (count = cast(u32, ds_cache.bytecode.Count()))
                {
                    cache_file.Write(&count, sizeof(count));
                    cache_file.Write(ds_cache.bytecode.Data(), count);
                }
                if (count = cast(u32, gs_cache.bytecode.Count()))
                {
                    cache_file.Write(&count, sizeof(count));
                    cache_file.Write(gs_cache.bytecode.Data(), count);
                }
                count = cast(u32, ps_cache.bytecode.Count());
                {
                    cache_file.Write(&count, sizeof(count));
                    cache_file.Write(ps_cache.bytecode.Data(), count);
                }
            }

            cache_file.Close();
            file.Close();

            shader_manager->ReleaseCompiledShader(vs_cache);
            shader_manager->ReleaseCompiledShader(hs_cache);
            shader_manager->ReleaseCompiledShader(ds_cache);
            shader_manager->ReleaseCompiledShader(gs_cache);
            shader_manager->ReleaseCompiledShader(ps_cache);

            asset->kind   = ASSET_KIND_SHADER;
            asset->shader = shader_manager->CreateGraphicsShader(cache_filename.ToConstString(), desc.resources, _static);
            asset->name   = desc.name;

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

const Asset *AssetManager::GetAsset(const GPU::ResourceHandle& resource) const
{
    if (GPU::MemoryManager::IsTexture(resource))
    {
        const Array<Asset>& assets = GPU::MemoryManager::IsStatic(resource) ? m_StaticAssets : m_SceneAssets;
        for (const Asset& asset : assets)
        {
            if (asset.texture == resource)
            {
                return &asset;
            }
        }
    }
    return null;
}

Asset *AssetManager::GetAsset(const GPU::ResourceHandle& resource)
{
    if (GPU::MemoryManager::IsTexture(resource))
    {
        Array<Asset>& assets = GPU::MemoryManager::IsStatic(resource) ? m_StaticAssets : m_SceneAssets;
        for (Asset& asset : assets)
        {
            if (asset.texture == resource)
            {
                return &asset;
            }
        }
    }
    return null;
}

REV_INTERNAL void ChangeExtension(StaticString<REV_PATH_CAPACITY>& filename, const ConstString& found_filename)
{
    u64 found_filename_dot_index = found_filename.RFind('.');
    REV_CHECK(found_filename_dot_index != ConstString::npos);

    ConstString& extension = found_filename.SubString(found_filename_dot_index + 1);

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
        default:                 REV_ERROR_M("Wrong ASSET_KIND: %I32u", kind); break;
    }

    filename += asset_name;
    filename.PushBack(REV_CSTR_ARGS(".*"));

    ConstString expected_extension;
    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:  expected_extension.AssignCSTR(REV_CSTR_ARGS(".hlsl")); break;
        case GraphicsAPI::API::VULKAN: expected_extension.AssignCSTR(REV_CSTR_ARGS(".glsl")); break;
    }

    bool file_found = File::Find(filename, [&filename, kind, &expected_extension](const ConstString& found_filename)
    {
        ChangeExtension(filename, found_filename);

        if (kind == ASSET_KIND_SHADER)
        {
            u64 dot_index = filename.RFind('.');
            REV_CHECK(dot_index != filename.npos);

            ConstString got_extension(filename.Data() + dot_index, filename.Length() - dot_index);
            if (got_extension != expected_extension)
            {
                filename.Replace(dot_index + 1, filename.Length(), '*');
                return false;
            }
        }

        return true;
    }, true);

    if (!file_found)
    {
        filename.Clear();
        switch (kind)
        {
            case ASSET_KIND_TEXTURE: m_Logger.LogError("Texture \"", asset_name, "\" is not found"); break;
            case ASSET_KIND_SHADER:  m_Logger.LogError("Shader \"", asset_name, "\" is not found");  break;
        }
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
        default:                REV_ERROR_M("Cache exists only for shaders", kind); break;
    }

    Path path(filename);
    path.Create();

    switch (GraphicsAPI::GetAPI())
    {
        case GraphicsAPI::API::D3D12:  filename.PushBack(REV_CSTR_ARGS("hlsl_")); break;
        case GraphicsAPI::API::VULKAN: filename.PushBack(REV_CSTR_ARGS("glsl_")); break;
    }
    filename += asset_name;
    filename.PushBack(REV_CSTR_ARGS(".cso"));
}

}
