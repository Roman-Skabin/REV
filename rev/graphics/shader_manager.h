//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/common.h"
#include "graphics/memory_manager.h"
#include "tools/static_string.hpp"
#include "math/vec.h"
#include "tools/const_array.hpp"

namespace REV
{
    class GraphicsAPI;

    #pragma pack(push, 1)
    struct REV_ALIGN(1) Vertex
    {
        Math::v4 position;
        Math::v4 normal;
        Math::v2 tex_coord;
    };
    #pragma pack(pop)

    typedef u32 Index;

#ifndef ASSET_HANDLE_DEFINED
#define ASSET_HANDLE_DEFINED
    struct AssetHandle
    {
        u64  index   = REV_U64_MAX;
        bool _static = false;
    };
#endif

    namespace GPU
    {
        enum SHADER_KIND : u32
        {
            SHADER_KIND_UNKNOWN  = 0,
            SHADER_KIND_VERTEX   = BIT(0),
            SHADER_KIND_HULL     = BIT(1),
            SHADER_KIND_DOMAIN   = BIT(2),
            SHADER_KIND_GEOMETRY = BIT(3),
            SHADER_KIND_PIXEL    = BIT(4),
            SHADER_KIND_COMPUTE  = BIT(5),

            SHADER_KIND_COUNT
        };
        REV_ENUM_OPERATORS(SHADER_KIND);

        struct ShaderHandle
        {
            u64  index   = REV_U64_MAX;
            bool _static = false;
        };

        struct CBufferDesc
        {
            GPU::ResourceHandle resource;
            u32                 shader_register;
            u32                 register_space;
        };

        struct SamplerDesc
        {
            GPU::ResourceHandle resource;
            u32                 shader_register;
            u32                 register_space;
        };

        struct CompileShaderResult
        {
            void             *blob     = null;
            ConstArray<byte>  bytecode;
        };

        // @TODO, @Optimize(Roman): Make some hash table or something that will store texture-shader mapping:
        //                          key = shader, data = array of AssetIDs (or some "AssetDescs").
        class REV_API ShaderManager final
        {
        public:
            // @Optimize, #Tools(Roman): Pass ConstList.
            ShaderHandle CreateGraphicsShader(
                const ConstString&              shader_cache_filename,
                const ConstArray<AssetHandle>&  textures,
                const ConstArray<CBufferDesc>&  cbuffers,
                const ConstArray<SamplerDesc>&  samplers,
                bool                            _static
            );

            void SetCurrentGraphicsShader(ShaderHandle graphics_shader);

            void BindVertexBuffer(ShaderHandle graphics_shader, ResourceHandle resource_handle);
            void BindIndexBuffer(ShaderHandle graphics_shader, ResourceHandle resource_handle);

            void Draw(ShaderHandle graphics_shader);

            CompileShaderResult CompileShader(const ConstString& code, const ConstString& name, SHADER_KIND kind);
            void ReleaseCompiledShader(CompileShaderResult& shader_bytecode);

        private:
            REV_REMOVE_OOP_STUFF(ShaderManager);

        private:
            #pragma warning(suppress: 4200)
            byte platform[0];

            friend class ::REV::GraphicsAPI;
        };
    }
}
