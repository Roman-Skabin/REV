// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "core/common.h"

namespace REV
{
    enum TEXTURE_FORMAT : u32;

    namespace D3D12
    {
        class DeviceContext;

        REV_INLINE bool Succeeded(HRESULT hr) { return hr >= S_OK; }
        REV_INLINE bool Failed(HRESULT hr)    { return hr <  S_OK; }

        template<typename T>
        REV_INLINE void SafeRelease(T *& unknown)
        {
            static_assert(RTTI::is_base_of_v<IUnknown, T>, "IUnknown must be the base of T");

            if (unknown)
            {
                unknown->Release();
                unknown = null;
            }
        }

        #define REV_FORCE_PRINT_MESSAGES _HRESULT_TYPEDEF_(0x8000'0000)

        bool CheckResultAndPrintMessages(HRESULT hr, DeviceContext *device_context);
        bool CheckResultAndPrintMessages(HRESULT hr);

        // @NOTE(Roman): Textures
        DXGI_FORMAT REVToDXGITextureFormat(TEXTURE_FORMAT format);
        TEXTURE_FORMAT DXGIToREVTextureFormat(DXGI_FORMAT format);

        // @NOTE(Roman): Samplers
        D3D12_FILTER RevToD3D12SamplerFilter(SAMPLER_FILTER filter, u32& anisotropy);
        D3D12_TEXTURE_ADDRESS_MODE RevToD3D12SamplerAddressMode(SAMPLER_ADRESS_MODE address_mode);
    }
}
