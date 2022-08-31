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

    REV_INLINE bool operator==(const ShaderHandle& left, const ShaderHandle& right) { return left.index == right.index && left._static == right._static; }
    REV_INLINE bool operator!=(const ShaderHandle& left, const ShaderHandle& right) { return left.index != right.index || left._static != right._static; }

    enum DSV_CLEAR_FLAG : u32
    {
        DSV_CLEAR_FLAG_NONE    = 0,
        DSV_CLEAR_FLAG_DEPTH   = 1 << 0,
        DSV_CLEAR_FLAG_STENCIL = 1 << 1,
    };
    REV_ENUM_OPERATORS(DSV_CLEAR_FLAG);

    enum UAV_CLEAR_OP : u32
    {
        UAV_CLEAR_OP_NONE,
        UAV_CLEAR_OP_FLOAT,
        UAV_CLEAR_OP_UINT,
    };

    struct ShaderInputResources
    {
        ResourceHandle resource;
        u32            shader_register;
        u32            register_space;
    };

    struct ShaderOutputResources
    {
        ResourceHandle resource;
    };

    // @NOTE(Roman): This is NOT a descriptor for a SRV.
    //               It's just a descriptor for all the resoucres used in a shader.
    //               e.g. constant buffers, read(-write) buffers, read(-write) textures, samplers, render targets.
    struct ShaderResourceDesc
    {
        ResourceHandle resource;
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
                bool     rtv_clear;
            };
            struct
            {
                DSV_CLEAR_FLAG dsv_clear_flags;
                f32            dsv_depth_clear_value;
                u8             dsv_stencil_clear_value;
            };
            struct
            {
                u32          uav_shader_register;
                u32          uav_register_space;
                UAV_CLEAR_OP uav_clear_op;
                union
                {
                    Math::v4  uav_float_clear_color;
                    Math::v4u uav_uint_clear_color;
                };
            };
        };

    private:
        REV_INLINE ShaderResourceDesc() {}

    public:
        REV_INLINE ShaderResourceDesc(const ShaderResourceDesc& other) { CopyMemory(this, &other, sizeof(ShaderResourceDesc)); }

        static REV_INLINE ShaderResourceDesc CBV(const ResourceHandle& resource, u32 shader_register, u32 register_space)
        {
            ShaderResourceDesc desc;
            desc.resource                        = resource;
            desc.cbv_srv_sampler_shader_register = shader_register;
            desc.cbv_srv_sampler_register_space  = register_space;
            return desc;
        }

        static REV_INLINE ShaderResourceDesc SRV(const ResourceHandle& resource, u32 shader_register, u32 register_space)
        {
            ShaderResourceDesc desc;
            desc.resource                        = resource;
            desc.cbv_srv_sampler_shader_register = shader_register;
            desc.cbv_srv_sampler_register_space  = register_space;
            return desc;
        }

        static REV_INLINE ShaderResourceDesc Sampler(const ResourceHandle& resource, u32 shader_register, u32 register_space)
        {
            ShaderResourceDesc desc;
            desc.resource                        = resource;
            desc.cbv_srv_sampler_shader_register = shader_register;
            desc.cbv_srv_sampler_register_space  = register_space;
            return desc;
        }

        static REV_INLINE ShaderResourceDesc RTV(const ResourceHandle& resource)
        {
            ShaderResourceDesc desc;
            desc.resource  = resource;
            desc.rtv_clear = false;
            return desc;
        }

        static REV_INLINE ShaderResourceDesc REV_VECTORCALL RTV(const ResourceHandle& resource, Math::v4 clear_color)
        {
            ShaderResourceDesc desc;
            desc.resource        = resource;
            desc.rtv_clear_color = clear_color;
            desc.rtv_clear       = true;
            return desc;
        }

        static REV_INLINE ShaderResourceDesc DSV(const ResourceHandle& resource)
        {
            ShaderResourceDesc desc;
            desc.resource        = resource;
            desc.dsv_clear_flags = DSV_CLEAR_FLAG_NONE;
            return desc;
        }

        static REV_INLINE ShaderResourceDesc DSV(const ResourceHandle& resource, f32 depth_clear_value)
        {
            ShaderResourceDesc desc;
            desc.resource              = resource;
            desc.dsv_clear_flags       = DSV_CLEAR_FLAG_DEPTH;
            desc.dsv_depth_clear_value = depth_clear_value;
            return desc;
        }

        static REV_INLINE ShaderResourceDesc DSV(const ResourceHandle& resource, u8 stencil_clear_value)
        {
            ShaderResourceDesc desc;
            desc.resource                = resource;
            desc.dsv_clear_flags         = DSV_CLEAR_FLAG_STENCIL;
            desc.dsv_stencil_clear_value = stencil_clear_value;
            return desc;
        }

        static REV_INLINE ShaderResourceDesc DSV(const ResourceHandle& resource, f32 depth_clear_value, u8 stencil_clear_value)
        {
            ShaderResourceDesc desc;
            desc.resource                = resource;
            desc.dsv_clear_flags         = DSV_CLEAR_FLAG_DEPTH | DSV_CLEAR_FLAG_STENCIL;
            desc.dsv_depth_clear_value   = depth_clear_value;
            desc.dsv_stencil_clear_value = stencil_clear_value;
            return desc;
        }

        static REV_INLINE ShaderResourceDesc UAV(const ResourceHandle& resource, u32 shader_register, u32 register_space)
        {
            ShaderResourceDesc desc;
            desc.resource            = resource;
            desc.uav_shader_register = shader_register;
            desc.uav_register_space  = register_space;
            desc.uav_clear_op        = UAV_CLEAR_OP_NONE;
            return desc;
        }

        static REV_INLINE ShaderResourceDesc REV_VECTORCALL UAV(const ResourceHandle& resource, u32 shader_register, u32 register_space, Math::v4 clear_color)
        {
            ShaderResourceDesc desc;
            desc.resource              = resource;
            desc.uav_shader_register   = shader_register;
            desc.uav_register_space    = register_space;
            desc.uav_clear_op          = UAV_CLEAR_OP_FLOAT;
            desc.uav_float_clear_color = clear_color;
            return desc;
        }

        static REV_INLINE ShaderResourceDesc REV_VECTORCALL UAV(const ResourceHandle& resource, u32 shader_register, u32 register_space, Math::v4u clear_color)
        {
            ShaderResourceDesc desc;
            desc.resource             = resource;
            desc.uav_shader_register  = shader_register;
            desc.uav_register_space   = register_space;
            desc.uav_clear_op         = UAV_CLEAR_OP_UINT;
            desc.uav_uint_clear_color = clear_color;
            return desc;
        }

        REV_INLINE operator bool() const { return resource; }
    };

    struct CompileShaderResult
    {
        void             *blob     = null;
        ConstArray<byte>  bytecode;
    };

    class REV_API REV_NOVTABLE ShaderManager final
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

        ConstArray<ResourceHandle> GetLoadableResources(const ShaderHandle& graphics_shader);
        ConstArray<ResourceHandle> GetStorableResources(const ShaderHandle& graphics_shader);

        void Draw(const ShaderHandle& graphics_shader);

        CompileShaderResult CompileShader(const ConstString& code, const ConstString& name, SHADER_KIND kind);
        void ReleaseCompiledShader(CompileShaderResult& shader_bytecode);

    private:
        REV_REMOVE_OOP_STUFF(ShaderManager);

    private:
        friend class GraphicsAPI;
    };
}
