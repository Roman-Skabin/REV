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
        //               e.g. constant buffers, read(-write) buffers, read(-write) textures, samplers, render targets.
        struct ShaderResourceDesc
        {
            GPU::ResourceHandle resource;
            union
            {
                struct
                {
                    u32 cbv_srv_sampler_shader_register;
                    u32 cbv_srv_sampler_register_space;
                };
                struct
                {
                    Math::v4 rtv_clear_color;
                };
                struct
                {
                    u32  uav_shader_register;
                    u32  uav_register_space;
                    bool uav_is_cleared_with_floats;
                    union
                    {
                        Math::v4  uav_float_clear_color;
                        Math::v4u uav_uint_clear_color;
                    };
                };
            };

            REV_INLINE ShaderResourceDesc(const GPU::ResourceHandle& resource, u32 shader_register, u32 register_space)
                : resource(resource), cbv_srv_sampler_shader_register(shader_register), cbv_srv_sampler_register_space(register_space)
            {}

            REV_INLINE ShaderResourceDesc(const GPU::ResourceHandle& resource, Math::v4 clear_color)
                : resource(resource), rtv_clear_color(clear_color)
            {}

            REV_INLINE ShaderResourceDesc(const GPU::ResourceHandle& resource, u32 shader_register, u32 register_space, Math::v4 clear_color)
                : resource(resource), uav_shader_register(shader_register), uav_register_space(register_space), uav_is_cleared_with_floats(true), uav_float_clear_color(clear_color)
            {}

            REV_INLINE ShaderResourceDesc(const GPU::ResourceHandle& resource, u32 shader_register, u32 register_space, Math::v4u clear_color)
                : resource(resource), uav_shader_register(shader_register), uav_register_space(register_space), uav_is_cleared_with_floats(false), uav_uint_clear_color(clear_color)
            {}

            REV_INLINE ShaderResourceDesc(const ShaderResourceDesc& other)
            {
                CopyMemory(this, &other, sizeof(GPU::ResourceHandle));
            }

            REV_INLINE ShaderResourceDesc& operator=(const ShaderResourceDesc& other)
            {
                if (this != &other)
                {
                    CopyMemory(this, &other, sizeof(GPU::ResourceHandle));
                }
                return *this;
            }
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

            void SetCurrentGraphicsShader(const ShaderHandle& graphics_shader);

            void BindVertexBuffer(const ShaderHandle& graphics_shader, const ResourceHandle& resource_handle);
            void BindIndexBuffer(const ShaderHandle& graphics_shader, const ResourceHandle& resource_handle);

            void Draw(const ShaderHandle& graphics_shader);

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
