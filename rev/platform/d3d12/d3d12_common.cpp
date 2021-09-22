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

}
