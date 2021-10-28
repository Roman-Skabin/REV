// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

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
        u64  index   = REV_INVALID_U64_INDEX;
        bool _static = false;

        REV_INLINE operator bool() const { return index != REV_INVALID_U64_INDEX; }
    };
#endif

    namespace GPU
    {
        enum SHADER_KIND : u32
        {
            SHADER_KIND_UNKNOWN  = 0,
            SHADER_KIND_VERTEX   = 1 << 0,
            SHADER_KIND_HULL     = 1 << 1,
            SHADER_KIND_DOMAIN   = 1 << 2,
            SHADER_KIND_GEOMETRY = 1 << 3,
            SHADER_KIND_PIXEL    = 1 << 4,
            SHADER_KIND_COMPUTE  = 1 << 5,

            SHADER_KIND_COUNT
        };
        REV_ENUM_OPERATORS(SHADER_KIND);

        struct ShaderHandle
        {
            u64  index   = REV_INVALID_U64_INDEX;
            bool _static = false;

            REV_INLINE operator bool() const { return index != REV_INVALID_U64_INDEX; }
        };

        // @NOTE(Roman): This is NOT a descriptor for a SRV.
        //               It's just a descriptor for all the resoucres used in a shader.
        //               e.g. constant buffers, read(-write) buffers, read(-write) textures, samplers.
        struct ShaderResourceDesc
        {
            GPU::ResourceHandle resource;
            u32                 shader_register = 0;
            u32                 register_space  = 0;
        };

        struct CompileShaderResult
        {
            void             *blob     = null;
            ConstArray<byte>  bytecode;
        };

        class REV_API ShaderManager final
        {
        public:
            ShaderHandle CreateGraphicsShader(
                const ConstString&                    shader_cache_filename,
                const ConstArray<ShaderResourceDesc>& resources,
                bool                                  _static
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
            byte platform[0];

            friend class ::REV::GraphicsAPI;
        };
    }
}
