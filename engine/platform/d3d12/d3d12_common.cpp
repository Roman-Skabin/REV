// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#include "core/pch.h"
#include "graphics/graphics_api.h"
#include "tools/static_string_builder.hpp"
#include "memory/memory.h"

#include "platform/d3d12/d3d12_device_context.h"
#include "platform/d3d12/d3d12_common.h"

namespace REV::D3D12
{

template<u64 cstr_len>
REV_INTERNAL REV_INLINE void AddCSTR(char *message, u64& message_len, const char (&cstr)[cstr_len])
{
    CopyMemory(message + message_len, cstr, cstr_len - 1);
    message_len += cstr_len - 1;
}

template<u64 cat_len>
REV_INTERNAL REV_INLINE void AddCategory(char *message, u64& message_len, const char (&category)[cat_len], int id)
{
    message_len += sprintf(message + message_len,
                           " [%.*s MESSAGE, MESSAGE_ID = %I32d]",
                           cast(int, cat_len - 1),
                           category,
                           id);
}

bool CheckResultAndPrintMessages(HRESULT hr, DeviceContext *device_context)
{
#if REV_DEBUG
    if (Failed(hr) && device_context->InfoQueue())
    {
        Memory *memory = Memory::Get();

        u64 num_messages = device_context->InfoQueue()->GetNumStoredMessages(DXGI_DEBUG_ALL);
        if (num_messages)
        {
            StaticStringBuilder<1024> builder;

            for (u64 i = 0; i < num_messages; ++i)
            {
                u64 message_length = 0;
                HRESULT error = device_context->InfoQueue()->GetMessage(DXGI_DEBUG_ALL, i, null, &message_length);
                REV_CHECK(Succeeded(error));

                DXGI_INFO_QUEUE_MESSAGE *message = cast(DXGI_INFO_QUEUE_MESSAGE *, memory->PushToFrameArena(message_length));
                error = device_context->InfoQueue()->GetMessage(DXGI_DEBUG_ALL, i, message, &message_length);
                REV_CHECK(Succeeded(error));

                /**/ if (Math::mm_equals(&message->Producer, &DXGI_DEBUG_D3D12)) builder.Build("D3D12 ");
                else if (Math::mm_equals(&message->Producer, &DXGI_DEBUG_DXGI))  builder.Build("DXGI ");
                else if (Math::mm_equals(&message->Producer, &DXGI_DEBUG_APP))   builder.Build("APP ");
                else                                                             builder.Build("<UNKNOWN> ");

                switch (message->Severity)
                {
                    case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION: builder.Build("CORRUPTION: "); break;
                    case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR:      builder.Build("ERROR: ");      break;
                    case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING:    builder.Build("WARNING: ");    break;
                    case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_INFO:       builder.Build("INFO: ");       break;
                    case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_MESSAGE:    builder.Build("MESSAGE: ");    break;
                }

                while (isspace(*message->pDescription))
                {
                    ++message->pDescription;
                    --message->DescriptionByteLength;
                }

                builder.Build(ConstString(message->pDescription, message->DescriptionByteLength));

                switch (message->Category)
                {
                    case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_UNKNOWN:               builder.Build("[UNKNOWN MESSAGE, MESSAGE_ID = ",               message->ID, "]"); break;
                    case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_MISCELLANEOUS:         builder.Build("[MISCELLANEOUS MESSAGE, MESSAGE_ID = ",         message->ID, "]"); break;
                    case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_INITIALIZATION:        builder.Build("[INITIALIZATION MESSAGE, MESSAGE_ID = ",        message->ID, "]"); break;
                    case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_CLEANUP:               builder.Build("[CLEANUP MESSAGE, MESSAGE_ID = ",               message->ID, "]"); break;
                    case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_COMPILATION:           builder.Build("[COMPILATION MESSAGE, MESSAGE_ID = ",           message->ID, "]"); break;
                    case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_STATE_CREATION:        builder.Build("[STATE CREATION MESSAGE, MESSAGE_ID = ",        message->ID, "]"); break;
                    case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_STATE_SETTING:         builder.Build("[STATE SETTING MESSAGE, MESSAGE_ID = ",         message->ID, "]"); break;
                    case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_STATE_GETTING:         builder.Build("[STATE GETTING MESSAGE, MESSAGE_ID = ",         message->ID, "]"); break;
                    case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_RESOURCE_MANIPULATION: builder.Build("[RESOURCE MANIPULATION MESSAGE, MESSAGE_ID = ", message->ID, "]"); break;
                    case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_EXECUTION:             builder.Build("[EXECUTION MESSAGE, MESSAGE_ID = ",             message->ID, "]"); break;
                    case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_SHADER:                builder.Build("[SHADER MESSAGE, MESSAGE_ID = ",                message->ID, "]"); break;
                };

                switch (message->Severity)
                {
                    case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR:   device_context->GetLogger().LogError(  builder.ToString()); break;
                    case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING: device_context->GetLogger().LogWarning(builder.ToString()); break;
                    default:                                       device_context->GetLogger().LogInfo(   builder.ToString()); break;
                }

                builder.Clear();
            }

            device_context->InfoQueue()->ClearStoredMessages(DXGI_DEBUG_ALL);
        }

        return hr == REV_FORCE_PRINT_MESSAGES;
    }
    else if (!device_context->InfoQueue())
    {
        return false;
    }
#endif
    return true;
}

bool CheckResultAndPrintMessages(HRESULT hr)
{
#if REV_DEBUG
    return CheckResultAndPrintMessages(hr, cast(DeviceContext *, GraphicsAPI::GetDeviceContext()));
#endif
    return true;
}

DXGI_FORMAT REVToDXGITextureFormat(TEXTURE_FORMAT format)
{
    switch (format)
    {
        case TEXTURE_FORMAT_S32:         return DXGI_FORMAT_R32_SINT;
        case TEXTURE_FORMAT_U32:         return DXGI_FORMAT_R32_UINT;

        case TEXTURE_FORMAT_R32:         return DXGI_FORMAT_R32_UINT;
        case TEXTURE_FORMAT_RG32:        return DXGI_FORMAT_R32G32_UINT;
        case TEXTURE_FORMAT_RGB32:       return DXGI_FORMAT_R32G32B32_UINT;
        case TEXTURE_FORMAT_RGBA32:      return DXGI_FORMAT_R32G32B32A32_UINT;

        case TEXTURE_FORMAT_BGRA8:       return DXGI_FORMAT_B8G8R8A8_UNORM;
        case TEXTURE_FORMAT_RGBA8:       return DXGI_FORMAT_R8G8B8A8_UNORM;
        case TEXTURE_FORMAT_A8:          return DXGI_FORMAT_A8_UNORM;
        case TEXTURE_FORMAT_B5G6R5:      return DXGI_FORMAT_B5G6R5_UNORM;
        case TEXTURE_FORMAT_BGR5A1:      return DXGI_FORMAT_B5G5R5A1_UNORM;
        case TEXTURE_FORMAT_BGRA4:       return DXGI_FORMAT_B4G4R4A4_UNORM;
        case TEXTURE_FORMAT_R10G10B10A2: return DXGI_FORMAT_R10G10B10A2_UNORM;
        case TEXTURE_FORMAT_RGBA16:      return DXGI_FORMAT_R16G16B16A16_UNORM;

        case TEXTURE_FORMAT_D16:         return DXGI_FORMAT_D16_UNORM;
        case TEXTURE_FORMAT_D32:         return DXGI_FORMAT_D32_FLOAT;
        case TEXTURE_FORMAT_D24S8:       return DXGI_FORMAT_D24_UNORM_S8_UINT;

        case TEXTURE_FORMAT_BC1:         return DXGI_FORMAT_BC1_UNORM;
        case TEXTURE_FORMAT_BC2:         return DXGI_FORMAT_BC2_UNORM;
        case TEXTURE_FORMAT_BC3:         return DXGI_FORMAT_BC3_UNORM;
        case TEXTURE_FORMAT_BC4:         return DXGI_FORMAT_BC4_UNORM;
        case TEXTURE_FORMAT_BC5:         return DXGI_FORMAT_BC5_UNORM;
        case TEXTURE_FORMAT_BC7:         return DXGI_FORMAT_BC7_UNORM;
    }

    REV_ERROR_M("Invalid TEXTURE_FORMAT: %hhu", format);
    return DXGI_FORMAT_UNKNOWN;
}

TEXTURE_FORMAT DXGIToREVTextureFormat(DXGI_FORMAT format)
{
    switch (format)
    {
        case DXGI_FORMAT_R32_SINT:           return TEXTURE_FORMAT_S32;
        case DXGI_FORMAT_R32_UINT:           return TEXTURE_FORMAT_U32;

        case DXGI_FORMAT_R32_UINT:           return TEXTURE_FORMAT_R32;
        case DXGI_FORMAT_R32G32_UINT:        return TEXTURE_FORMAT_RG32;
        case DXGI_FORMAT_R32G32B32_UINT:     return TEXTURE_FORMAT_RGB32;
        case DXGI_FORMAT_R32G32B32A32_UINT:  return TEXTURE_FORMAT_RGBA32;

        case DXGI_FORMAT_B8G8R8A8_UNORM:     return TEXTURE_FORMAT_BGRA8;
        case DXGI_FORMAT_R8G8B8A8_UNORM:     return TEXTURE_FORMAT_RGBA8;
        case DXGI_FORMAT_A8_UNORM:           return TEXTURE_FORMAT_A8;
        case DXGI_FORMAT_B5G6R5_UNORM:       return TEXTURE_FORMAT_B5G6R5;
        case DXGI_FORMAT_B5G5R5A1_UNORM:     return TEXTURE_FORMAT_BGR5A1;
        case DXGI_FORMAT_B4G4R4A4_UNORM:     return TEXTURE_FORMAT_BGRA4;
        case DXGI_FORMAT_R10G10B10A2_UNORM:  return TEXTURE_FORMAT_R10G10B10A2;
        case DXGI_FORMAT_R16G16B16A16_UNORM: return TEXTURE_FORMAT_RGBA16;

        case DXGI_FORMAT_D16_UNORM:          return TEXTURE_FORMAT_D16;
        case DXGI_FORMAT_D32_FLOAT:          return TEXTURE_FORMAT_D32;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:  return TEXTURE_FORMAT_D24S8;

        case DXGI_FORMAT_BC1_UNORM:          return TEXTURE_FORMAT_BC1;
        case DXGI_FORMAT_BC2_UNORM:          return TEXTURE_FORMAT_BC2;
        case DXGI_FORMAT_BC3_UNORM:          return TEXTURE_FORMAT_BC3;
        case DXGI_FORMAT_BC4_UNORM:          return TEXTURE_FORMAT_BC4;
        case DXGI_FORMAT_BC5_UNORM:          return TEXTURE_FORMAT_BC5;
        case DXGI_FORMAT_BC7_UNORM:          return TEXTURE_FORMAT_BC7;
    }

    REV_ERROR_M("Invalid or unsupported DXGI_FORMAT: %I32u", format);
    return TEXTURE_FORMAT_UNKNOWN;
}

D3D12_FILTER RevToD3D12SamplerFilter(SAMPLER_FILTER filter, u32& anisotropy)
{
    switch (filter)
    {
        case SAMPLER_FILTER_POINT:           anisotropy = 0;  return D3D12_FILTER_MIN_MAG_MIP_POINT;
        case SAMPLER_FILTER_BILINEAR:        anisotropy = 0;  return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        case SAMPLER_FILTER_TRILINEAR:       anisotropy = 0;  return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        case SAMPLER_FILTER_ANISOTROPIC_1X:  anisotropy = 1;
        case SAMPLER_FILTER_ANISOTROPIC_2X:  anisotropy = 2;
        case SAMPLER_FILTER_ANISOTROPIC_4X:  anisotropy = 4;
        case SAMPLER_FILTER_ANISOTROPIC_8X:  anisotropy = 8;
        case SAMPLER_FILTER_ANISOTROPIC_16X: anisotropy = 16; return D3D12_FILTER_ANISOTROPIC;
    }

    anisotropy = 0;
    REV_ERROR_M("Invalid or unsupported SAMPLER_FILTER: %hhu", filter);
    return D3D12_FILTER_MIN_MAG_MIP_POINT;
}

D3D12_TEXTURE_ADDRESS_MODE RevToD3D12SamplerAddressMode(SAMPLER_ADRESS_MODE address_mode)
{
    switch (address_mode)
    {
        case SAMPLER_ADRESS_MODE_CLAMP:  return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        case SAMPLER_ADRESS_MODE_WRAP:   return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        case SAMPLER_ADRESS_MODE_MIRROR: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
    }

    REV_ERROR_M("Invalid or unsupported SAMPLER_ADRESS_MODE: %hhu", address_mode);
    return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
}

}
