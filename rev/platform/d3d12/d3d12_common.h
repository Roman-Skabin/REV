//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/common.h"

namespace REV::D3D12
{
    class Renderer;

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

    bool CheckResultAndPrintMessages(HRESULT hr, Renderer *renderer);
    bool CheckResultAndPrintMessages(HRESULT hr);
}
