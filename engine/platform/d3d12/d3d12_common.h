//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"

namespace D3D12
{
    class Renderer;

    constexpr bool Succeeded(HRESULT hr) { return hr >= S_OK; }
    constexpr bool Failed(HRESULT hr)    { return hr <  S_OK; }

    template<typename T, typename = RTTI::enable_if_t<RTTI::is_base_of_v<IUnknown, T>>>
    INLINE void SafeRelease(T *& unknown)
    {
        if (unknown)
        {
            unknown->Release();
            unknown = null;
        }
    }

    ENGINE_API bool CheckResultAndPrintMessages(HRESULT hr, Renderer *renderer);
    ENGINE_API bool CheckResultAndPrintMessages(HRESULT hr);
}
