//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/pch.h"
#include "platform/d3d12_gpu_memory_manager.h"

namespace D3D12 {

//
// GPUResource
//

GPUResource::GPUResource(const char *name, u32 name_length)
    : m_DefaultResource(null),
      m_UploadResources{null},
      m_UploadPointers{null},
      m_Kind(KIND::UNKNOWN),
      m_AllocationState(ALLOCATION_STATE::NONE),
      m_DescHeap(null),
      m_VertexIndexCount(0),
      m_VertexIndexStride(0),
      m_InitialState(D3D12_RESOURCE_STATE_COMMON),
      m_NameLen(name_length ? name_length : cast<u32>(strlen(name))),
      m_ResourceDesc()
{
    CopyMemory(m_Name, name, m_NameLen);
}

GPUResource::~GPUResource()
{
}

//
// GPUDescHeap
//

GPUDescHeap::GPUDescHeap()
    : m_Handle(null),
      m_Resource(null),
      m_DescHeapDesc(),
      m_AllocationState(ALLOCATION_STATE::NONE)
{
}

GPUDescHeap::~GPUDescHeap()
{
}

//
// GPUResourceMemory
//

GPUResourceMemory::GPUResourceMemory(Allocator *allocator)
    : m_Resources(allocator),
      m_DefaultHeap(null),
      m_DefaultOffset(0),
      m_DefaultCapacity(0),
      m_UploadHeap{null},
      m_UploadOffset{0},
      m_UploadCapacity{0}
{
}

GPUResourceMemory::~GPUResourceMemory()
{
}

//
// GPUDescHeapMemory
//

GPUDescHeapMemory::GPUDescHeapMemory(Allocator *allocator)
    : m_DescHeaps(allocator)
{
}

GPUDescHeapMemory::~GPUDescHeapMemory()
{
}

//
// GPUMemoryManager
//

GPUMemoryManager::GPUMemoryManager(Allocator *allocator)
    : m_Allocator(allocator),
      m_DescHeapMemory(allocator),
      m_BufferMemory(allocator),
      m_TextureMemory(allocator),
      m_Error(S_OK)
{
}

GPUMemoryManager::~GPUMemoryManager()
{
}

void GPUMemoryManager::Destroy()
{
    this->~GPUMemoryManager();
}

};
