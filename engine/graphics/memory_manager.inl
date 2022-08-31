// Copyright (c) 2020-2022, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

namespace REV
{
    //
    // ResourceHandle
    //

    REV_INLINE ResourceHandle::operator bool() const
    {
        return ptr && kind != RESOURCE_KIND_NONE;
    }

    REV_INLINE bool operator==(const ResourceHandle& left, const ResourceHandle& right)
    {
        return left.ptr  == right.ptr
            && left.kind == right.kind;
    }

    REV_INLINE bool operator!=(const ResourceHandle& left, const ResourceHandle& right)
    {
        return left.ptr  != right.ptr
            || left.kind != right.kind;
    }

    //
    // MemoryManager
    //
}
