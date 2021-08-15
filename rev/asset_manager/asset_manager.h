//
// Copyright 2020-2021 Roman Skabin
//

#pragma once

#include "tools/array.hpp"
#include "tools/const_string.h"
#include "graphics/memory_manager.h"
#include "graphics/shader_manager.h"
#include "core/work_queue.h"

namespace REV
{
    enum ASSET_KIND : u32
    {
        ASSET_KIND_UNKNOWN = 0,
        ASSET_KIND_TEXTURE,
        ASSET_KIND_SHADER,
        ASSET_KIND_MESH,     // @Incomplete(Roman): Meshes are not supported yet.
    };

    #ifndef ASSET_HANDLE_DEFINED
    #define ASSET_HANDLE_DEFINED
        struct AssetHandle
        {
            u64  index;
            bool _static;
        };
    #endif

    // @Cleanup(Roman)
    struct Asset
    {
        ASSET_KIND kind;
        union
        {
            struct
            {
                GPU::ResourceHandle resource;
                u32                 shader_register;
                u32                 register_space;
            } texture;
            GPU::ShaderHandle shader_handle;
            struct
            {
                GPU::ResourceHandle vbuffer;
                GPU::ResourceHandle ibuffer;
            } mesh;
        };
        StaticString<64> name;
    };

    struct LoadTextureDesc final
    {
        const ConstString& name;
        u32                shader_register;
        u32                register_space;

        REV_INLINE LoadTextureDesc(const ConstString& name, u32 shader_register, u32 register_space)
            : name(name), shader_register(shader_register), register_space(register_space)
        {}
    };

    struct LoadShaderDesc final
    {
        const ConstString&                  name;
        const ConstArray<AssetHandle>&      textures;
        const ConstArray<GPU::CBufferDesc>& cbuffers;
        const ConstArray<GPU::SamplerDesc>& samplers;

        REV_INLINE LoadShaderDesc(
            const ConstString&                  name,
            const ConstArray<AssetHandle>&      textures,
            const ConstArray<GPU::CBufferDesc>& cbuffers,
            const ConstArray<GPU::SamplerDesc>& samplers)
            : name(name),
              textures(textures),
              cbuffers(cbuffers),
              samplers(samplers)
        {}
    };

    class REV_API AssetManager final
    {
    public:
        static AssetManager *Create(Allocator *allocator, const Logger& logger);
        static AssetManager *Get();

        ~AssetManager();

        void FreeSceneAssets();

        AssetHandle LoadTexture(const LoadTextureDesc& desc, bool _static);
        AssetHandle LoadShader(const LoadShaderDesc& desc, bool _static);

        // @NOTE(Roman): Batch loaders. Return number of load assets
        Array<AssetHandle> LoadTextures(const ConstArray<LoadTextureDesc>& descs, bool _static);
        Array<AssetHandle> LoadShaders(const ConstArray<LoadShaderDesc>& descs, bool _static);

        REV_INLINE const Array<Asset>& GetStaticAssets() const { return m_StaticAssets; }
        REV_INLINE const Array<Asset>& GetSceneAssets()  const { return m_SceneAssets;  }

        REV_INLINE Array<Asset>& GetStaticAssets() { return m_StaticAssets; }
        REV_INLINE Array<Asset>& GetSceneAssets()  { return m_SceneAssets;  }

        AssetHandle FindAsset(const ConstString& name, bool _static) const;

        const Asset *GetAsset(const ConstString& name, bool _static) const;
              Asset *GetAsset(const ConstString& name, bool _static);

        REV_INLINE const Asset *GetAsset(AssetHandle handle)  const { return handle._static ? m_StaticAssets.GetPointer(handle.index) : m_SceneAssets.GetPointer(handle.index);  }
        REV_INLINE       Asset *GetAsset(AssetHandle handle)        { return handle._static ? m_StaticAssets.GetPointer(handle.index) : m_SceneAssets.GetPointer(handle.index);  }

        void MakeFilename(StaticString<REV_PATH_CAPACITY>& filename, ASSET_KIND kind, const ConstString& asset_name);
        void MakeCacheFilename(StaticString<REV_PATH_CAPACITY>& filename, ASSET_KIND kind, const ConstString& asset_name);

    private:
        AssetManager(Allocator *allocator, const Logger& logger);
        
        void LoadDDSTexture(Asset *asset, const ConstArray<byte>& data, bool _static);

        REV_DELETE_CONSTRS_AND_OPS(AssetManager);

    private:
        Allocator    *m_Allocator;
        Array<Asset>  m_StaticAssets;          // @TODO, #Tools(Roman): Use HashTables instead (cuz we can search 'em by names)
        Array<Asset>  m_SceneAssets;           // @TODO, #Tools(Roman): Use HashTables instead (cuz we can search 'em by names)
        Logger        m_Logger;
        WorkQueue     m_WorkQueue;
    };
}
