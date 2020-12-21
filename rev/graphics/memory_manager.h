//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/common.h"
#include "tools/static_string.hpp"

namespace REV { class GraphicsAPI; }

namespace REV::GPU
{
    enum class RESOURCE_KIND : u64
    {
        UNKNOWN,
        VB,
        IB,
        CB,
        SR,
        UA,
    };

    struct ResourceHandle final
    {
        u64           index = REV_U64_MAX;
        RESOURCE_KIND kind  = RESOURCE_KIND::UNKNOWN;
    };

    class REV_API MemoryManager final
    {
    public:
        // @Optimize(Roman): Use (upload pointer + offset) as a CPU data storage for buffers?
        ResourceHandle AllocateVertexBuffer(u32 vertex_count, const StaticString<64>& name = null); // stride = sizeof(REV::Vertex)
        ResourceHandle AllocateIndexBuffer(u32 index_count, const StaticString<64>& name = null); // stride = sizeof(REV::Index)
        ResourceHandle AllocateConstantBuffer(u32 bytes, const StaticString<64>& name /* required */);

        void SetResourceData(ResourceHandle resource, const void *data);

        void SetResourceDataImmediately(ResourceHandle resource, const void *data);

        void StartImmediateExecution();
        void EndImmediateExecution();

        void FreeMemory();

    private:
        MemoryManager()                     = delete;
        MemoryManager(const MemoryManager&) = delete;
        MemoryManager(MemoryManager&&)      = delete;

        ~MemoryManager() = delete;

        MemoryManager& operator=(const MemoryManager&) = delete;
        MemoryManager& operator=(MemoryManager&&)      = delete;

    private:
        #pragma warning(suppress: 4200)
        byte platform[0];

        friend class ::REV::GraphicsAPI;
    };
}
