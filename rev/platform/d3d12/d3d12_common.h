// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "core/common.h"

namespace REV::D3D12
{
    class DeviceContext;

    REV_INLINE bool Succeeded(HRESULT hr) { return hr >= S_OK; }
    REV_INLINE bool Failed(HRESULT hr)    { return hr <  S_OK; }

    template<typename T, typename = RTTI::enable_if_t<RTTI::is_base_of_v<IUnknown, T>>>
    REV_INLINE void SafeRelease(T *& unknown)
    {
        if (unknown)
        {
            unknown->Release();
            unknown = null;
        }
    }

    #define REV_FORCE_PRINT_MESSAGES _HRESULT_TYPEDEF_(0x8000'0000)

    bool CheckResultAndPrintMessages(HRESULT hr, DeviceContext *device_context);
    bool CheckResultAndPrintMessages(HRESULT hr);
}
