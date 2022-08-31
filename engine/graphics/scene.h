// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "asset_manager/asset_manager.h"

namespace REV
{
    using EntityID = u32;

    #define REV_INVALID_ENTITY_ID REV_U32_MAX

    // @TODO(Roman): Store vertices and indeces in RAM so we can batch them together on GPU
    class REV_API Entity
    {
    public:
        Entity(const ConstString& name);
        Entity(Entity&& other);

        ~Entity();

        void SetVertices(const void *vertices, u64 count, u64 stride);
        void SetIndices(const void *indices, u64 count, u64 stride);

        template<typename T> REV_INLINE void SetVertices(const ConstArray<T>& vertices) { SetVertices(vertices.Data(), vertices.Count(), vertices.Stride()); }
        template<typename T> REV_INLINE void SetIndices(const ConstArray<T>& indices)   { SetIndices(indices.Data(),   indices.Count(),  indices.Stride());  }

        REV_INLINE const ResourceHandle& VertexBuffer() const { return m_VertexBuffer; }
        REV_INLINE const ResourceHandle& IndexBuffer()  const { return m_IndexBuffer;  }
        REV_INLINE const ConstString&    Name()         const { return m_Name;         }
        REV_INLINE EntityID              ID()           const { return m_ID;           }

        REV_INLINE bool Valid() const { return m_ID != REV_INVALID_ENTITY_ID; }

        Entity& operator=(Entity&& other);

        REV_INLINE operator bool() { return Valid(); }

    private:
        Entity(const Entity&) = delete;
        Entity& operator=(const Entity&) = delete;

    private:
        ResourceHandle m_VertexBuffer;
        ResourceHandle m_IndexBuffer;
        ConstString    m_Name;
        EntityID       m_ID;
    };

    REV_INLINE bool operator==(const Entity& left, const Entity& right) { return left.ID() == right.ID(); }
    REV_INLINE bool operator!=(const Entity& left, const Entity& right) { return left.ID() != right.ID(); }

    class REV_API Scene
    {
    protected:
        Scene(const ConstString& name);

    public:
        virtual ~Scene() {}

        virtual void OnSetCurrent()   = 0;
        virtual void OnUnsetCurrent() = 0;

        virtual void OnUpdate() = 0;

        void OnUnsetCurrentEx();

    private:
        REV_DELETE_CONSTRS_AND_OPS(Scene);
        Scene() = delete;
    
    protected:
        ConstString m_Name;
    };
}
