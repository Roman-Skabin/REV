// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "tools/critical_section.hpp"

namespace REV
{
    template<typename SyncObjectType>
    class ScopedLock final
    {
    public:
        REV_INLINE ScopedLock(SyncObjectType& sync_object)
            : m_SyncObject(sync_object)
        {
            /**/ if constexpr (RTTI::is_same_v<RTTI::remove_cv_t<RTTI::remove_ref_t<SyncObjectType>>, CriticalSection<false>>) m_SyncObject.Enter();
            else if constexpr (RTTI::is_same_v<RTTI::remove_cv_t<RTTI::remove_ref_t<SyncObjectType>>, CriticalSection<true>>)  m_SyncObject.Enter();
            else static_assert("Invalid, unsupported or unhandled syncronization object type");
        }

        REV_INLINE ~ScopedLock()
        {
            /**/ if constexpr (RTTI::is_same_v<RTTI::remove_cv_t<RTTI::remove_ref_t<SyncObjectType>>, CriticalSection<false>>) m_SyncObject.Leave();
            else if constexpr (RTTI::is_same_v<RTTI::remove_cv_t<RTTI::remove_ref_t<SyncObjectType>>, CriticalSection<true>>)  m_SyncObject.Leave();
        }

    private:
        ScopedLock() = delete;
        REV_DELETE_CONSTRS_AND_OPS(ScopedLock);

    private:
        SyncObjectType& m_SyncObject;
    };

    template<typename SyncObjectType> ScopedLock(SyncObjectType&) -> ScopedLock<SyncObjectType>;

    #define REV_SCOPED_LOCK(sync_object) ScopedLock REV_CSTRCAT(scoped_lock_, __COUNTER__)(sync_object)
}
