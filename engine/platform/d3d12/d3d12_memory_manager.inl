// Copyright (c) 2020-2022, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

namespace REV
{
    namespace D3D12
    {
        //
        // MemoryManager
        //

        REV_INLINE ConstString MemoryManager::GetResourceName(const ResourceHandle& resource)
        {
            REV_CHECK_M(resource, "Handle is not valid");

            if (resource.kind == RESOURCE_KIND_BUFFER)
            {
                return cast(Buffer *, resource.ptr)->name.ToConstString();
            }

            if (resource.kind == RESOURCE_KIND_TEXTURE)
            {
                return cast(Texture *, resource.ptr)->name.ToConstString();
            }

            REV_CHECK(resource.kind == RESOURCE_KIND_SAMPLER);
            return cast(Sampler *, resource.ptr)->name.ToConstString();
        }
        
        REV_INLINE ConstString MemoryManager::GetBufferName(const ResourceHandle& resource)
        {
            return GetBuffer(resource)->name.ToConstString();
        }
        
        REV_INLINE ConstString MemoryManager::GetTextureName(const ResourceHandle& resource)
        {
            return GetTexture(resource)->name.ToConstString();
        }

        REV_INLINE ConstString MemoryManager::GetSamplerName(const ResourceHandle& resource)
        {
            return GetSampler(resource)->name.ToConstString();
        }

        REV_INLINE D3D12_GPU_VIRTUAL_ADDRESS MemoryManager::GetBufferGPUVirtualAddress(const ResourceHandle& resource)
        {
            REV_CHECK_M(resource, "Handle is not valid");
            REV_CHECK_M(resource.kind == RESOURCE_KIND_BUFFER || resource.kind == RESOURCE_KIND_TEXTURE, "Only buffers and textures have gpu virtual address");

            if (resource.kind == RESOURCE_KIND_BUFFER)
            {
                return GetBufferGPUVirtualAddress(resource);
            }

            return GetTextureGPUVirtualAddress(resource);
        }

        REV_INLINE D3D12_GPU_VIRTUAL_ADDRESS MemoryManager::GetBufferGPUVirtualAddress(const ResourceHandle& resource)
        {
            Buffer *buffer = GetBuffer(resource);
            return buffer->default_page->gpu_mem->GetGPUVirtualAddress() + buffer->default_page_offset;
        }

        REV_INLINE D3D12_GPU_VIRTUAL_ADDRESS MemoryManager::GetTextureGPUVirtualAddress(const ResourceHandle& resource)
        {
            return GetTexture(resource)->default_gpu_mem->GetGPUVirtualAddress();
        }

        REV_INLINE ID3D12Resource *MemoryManager::GetBufferDefaultGPUMem(const ResourceHandle& resource)
        {
            return GetBuffer(resource)->default_page->gpu_mem;
        }

        REV_INLINE Buffer *MemoryManager::GetBuffer(const ResourceHandle& resource)
        {
            REV_CHECK_M(resource, "Handle is not valid");
            REV_CHECK_M(resource.kind == RESOURCE_KIND_BUFFER, "Resource is not a buffer");

            return cast(Buffer *, resource.ptr);
        }

        REV_INLINE Texture *MemoryManager::GetTexture(const ResourceHandle& resource)
        {
            REV_CHECK_M(resource, "Handle is not valid");
            REV_CHECK_M(resource.kind == RESOURCE_KIND_TEXTURE, "Resource is not a texture");

            return cast(Texture *, resource.ptr);
        }

        REV_INLINE Sampler *MemoryManager::GetSampler(const ResourceHandle& resource)
        {
            REV_CHECK_M(resource, "Handle is not valid");
            REV_CHECK_M(resource.kind == RESOURCE_KIND_SAMPLER, "Resource is not a sampler");

            return cast(Sampler *, resource.ptr);
        }

        REV_INLINE D3D12_CLEAR_VALUE MemoryManager::GetOptimizedClearValue(RESOURCE_FLAG flags, TEXTURE_FORMAT format)
        {
            D3D12_CLEAR_VALUE clear_value{};

            if (flags & RESOURCE_FLAG_ALLOW_DEPTH_STENCIL_TARGET)
            {
                switch (format)
                {
                    case TEXTURE_FORMAT_D16:   clear_value.Format = DXGI_FORMAT_D16_UNORM; break;
                    case TEXTURE_FORMAT_D32:   clear_value.Format = DXGI_FORMAT_D32_FLOAT; break;
                    case TEXTURE_FORMAT_D24S8: clear_value.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; break;
                    default: REV_ERROR_M("Unhandled or unknown depth-stencil format: %hhu", format);
                }

                clear_value.DepthStencil.Depth   = 0.0f; // zw far depth
                clear_value.DepthStencil.Stencil = 0;
            }
            else if (flags & RESOURCE_FLAG_ALLOW_COLOR_TARGET)
            {
                // @TODO(Roman): Handle other formats.
                clear_value.Format   = DXGI_FORMAT_R8G8B8A8_UNORM;
                clear_value.Color[0] = 0.2f;
                clear_value.Color[1] = 0.2f;
                clear_value.Color[2] = 0.2f;
                clear_value.Color[3] = 1.0f;
            }

            return clear_value;
        }

        REV_INLINE ResourceMemory *MemoryManager::GetResourceMemory(RESOURCE_FLAG flags, const ConstString& name)
        {
            if (flags & RESOURCE_FLAG_PER_FRAME)
            {
                return &m_PerFrameMemory;
            }

            if (flags & RESOURCE_FLAG_PER_SCENE)
            {
                return &m_PerSceneMemory;
            }

            if (flags & RESOURCE_FLAG_PERMANENT)
            {
                return &m_PermanentMemory;
            }

            REV_ERROR_M("Resource \"%.*s\" is not bound to any memory. "
                        "Use RESOURCE_FLAG_PER_FRAME, RESOURCE_FLAG_PER_SCENE or RESOURCE_FLAG_PERMANENT",
                        name.Length(), name.Data());

            return null;
        }

        REV_INLINE D3D12_RESOURCE_STATES MemoryManager::GetResourceDefaultState(RESOURCE_KIND kind, RESOURCE_FLAG flags)
        {
            REV_CHECK_M(kind == RESOURCE_KIND_BUFFER || kind == RESOURCE_KIND_TEXTURE, "Samplers have no resource states");

            if (!(flags & RESOURCE_FLAG_ALLOW_SHADER_READ))
            {
                if (kind == RESOURCE_KIND_BUFFER)
                {
                    if (flags & (RESOURCE_FLAG_VERTEX_BUFFER | RESOURCE_FLAG_CONSTANT_BUFFER))
                    {
                        return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
                    }

                    if (flags & RESOURCE_FLAG_INDEX_BUFFER)
                    {
                        return D3D12_RESOURCE_STATE_INDEX_BUFFER;
                    }
                }

                if (kind == RESOURCE_KIND_TEXTURE)
                {
                    if (flags & RESOURCE_FLAG_ALLOW_COLOR_TARGET)
                    {
                        REV_CHECK_M(!(flags & RESOURCE_FLAG_ALLOW_DEPTH_STENCIL_TARGET),
                                    "You cannot combine RESOURCE_FLAG_ALLOW_COLOR_TARGET and RESOURCE_FLAG_ALLOW_DEPTH_STENCIL_TARGET");
                        return D3D12_RESOURCE_STATE_RENDER_TARGET;
                    }

                    if (flags & RESOURCE_FLAG_ALLOW_DEPTH_STENCIL_TARGET)
                    {
                        return D3D12_RESOURCE_STATE_DEPTH_READ;
                    }
                }

                // @TODO(Roman): What if we make D3D12_RESOURCE_STATE_UNORDERED_ACCESS state default state
                //               event if RESOURCE_FLAG_ALLOW_SHADER_READ persists.
                if (flags & RESOURCE_FLAG_ALLOW_NONCOHERENT_GPU_WRITES)
                {
                    return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                }
            }

            // @TODO(Roman): Do we want to separate these default states with flags.
            return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
                 | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        }

        REV_INLINE D3D12_RESOURCE_FLAGS MemoryManager::GetResourceFlags(RESOURCE_KIND kind, RESOURCE_FLAG flags)
        {
            REV_CHECK_M(kind == RESOURCE_KIND_BUFFER || kind == RESOURCE_KIND_TEXTURE, "Samplers have no resource states");

            D3D12_RESOURCE_FLAGS d3d12_flags = D3D12_RESOURCE_FLAG_NONE;

            if (flags & RESOURCE_FLAG_ALLOW_COLOR_TARGET)
            {
                REV_CHECK(kind == RESOURCE_KIND_TEXTURE);
                REV_CHECK(!(flags & RESOURCE_FLAG_ALLOW_DEPTH_STENCIL_TARGET));
                d3d12_flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            }

            if (flags & RESOURCE_FLAG_ALLOW_DEPTH_STENCIL_TARGET)
            {
                REV_CHECK(kind == RESOURCE_KIND_TEXTURE);
                REV_CHECK(!(flags & RESOURCE_FLAG_ALLOW_COLOR_TARGET));
                d3d12_flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
            }

            if (flags & RESOURCE_FLAG_ALLOW_NONCOHERENT_GPU_WRITES)
            {
                d3d12_flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            }

            if (!(flags & RESOURCE_FLAG_ALLOW_SHADER_READ))
            {
                d3d12_flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
            }

            return d3d12_flags;
        }
    }
}
