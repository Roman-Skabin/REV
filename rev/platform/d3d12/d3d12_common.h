//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/common.h"

namespace REV::D3D12
{
    class Renderer;

    constexpr bool Succeeded(HRESULT hr) { return hr >= S_OK; }
    constexpr bool Failed(HRESULT hr)    { return hr <  S_OK; }

    template<typename T, typename = RTTI::enable_if_t<RTTI::is_base_of_v<IUnknown, T>>>
    REV_INLINE void SafeRelease(T *& unknown)
    {
        if (unknown)
        {
            unknown->Release();
            unknown = null;
        }
    }

    REV_API bool CheckResultAndPrintMessages(HRESULT hr, Renderer *renderer);
    REV_API bool CheckResultAndPrintMessages(HRESULT hr);
}
