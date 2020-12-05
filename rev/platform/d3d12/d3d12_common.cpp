//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "platform/d3d12/d3d12_common.h"
#include "graphics/graphics_api.h"
#include "platform/d3d12/d3d12_renderer.h"

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
                           cast<int>(cat_len - 1),
                           category,
                           id);
}

bool CheckResultAndPrintMessages(HRESULT hr, Renderer *renderer)
{
#if REV_DEBUG
    if (Failed(hr) && renderer->InfoQueue())
    {
        Memory *memory = Memory::Get();

        u64 num_messages = renderer->InfoQueue()->GetNumStoredMessages(DXGI_DEBUG_ALL);
        if (num_messages)
        {
            char composed_message[1024] = {0};
            u64  composed_message_len   = 0;

            for (u64 i = 0; i < num_messages; ++i)
            {
                u64 message_length = 0;
                HRESULT error = renderer->InfoQueue()->GetMessage(DXGI_DEBUG_ALL, i, null, &message_length);
                REV_CHECK(Succeeded(error));

                DXGI_INFO_QUEUE_MESSAGE *message = cast<DXGI_INFO_QUEUE_MESSAGE *>(memory->PushToTransientArea(message_length));
                error = renderer->InfoQueue()->GetMessage(DXGI_DEBUG_ALL, i, message, &message_length);
                REV_CHECK(Succeeded(error));

                composed_message_len = 0;

                /**/ if (Math::mm_equals(&message->Producer, &DXGI_DEBUG_D3D12)) AddCSTR(composed_message, composed_message_len, "D3D12 ");
                else if (Math::mm_equals(&message->Producer, &DXGI_DEBUG_DXGI))  AddCSTR(composed_message, composed_message_len, "DXGI ");
                else if (Math::mm_equals(&message->Producer, &DXGI_DEBUG_APP))   AddCSTR(composed_message, composed_message_len, "APP ");
                else                                                             AddCSTR(composed_message, composed_message_len, "<UNKNOWN> ");

                switch (message->Severity)
                {
                    case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION: AddCSTR(composed_message, composed_message_len, "CORRUPTION: "); break;
                    case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR:      AddCSTR(composed_message, composed_message_len, "ERROR: ");      break;
                    case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING:    AddCSTR(composed_message, composed_message_len, "WARNING: ");    break;
                    case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_INFO:       AddCSTR(composed_message, composed_message_len, "INFO: ");       break;
                    case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_MESSAGE:    AddCSTR(composed_message, composed_message_len, "MESSAGE: ");    break;
                }

                while (isspace(*message->pDescription))
                {
                    ++message->pDescription;
                    --message->DescriptionByteLength;
                }

                CopyMemory(composed_message + composed_message_len, message->pDescription, message->DescriptionByteLength);
                composed_message_len += message->DescriptionByteLength;

                switch (message->Category)
                {
                    case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_UNKNOWN:               AddCategory(composed_message, composed_message_len, "UNKNOWN", message->ID);               break;
                    case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_MISCELLANEOUS:         AddCategory(composed_message, composed_message_len, "MISCELLANEOUS", message->ID);         break;
                    case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_INITIALIZATION:        AddCategory(composed_message, composed_message_len, "INITIALIZATION", message->ID);        break;
                    case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_CLEANUP:               AddCategory(composed_message, composed_message_len, "CLEANUP", message->ID);               break;
                    case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_COMPILATION:           AddCategory(composed_message, composed_message_len, "COMPILATION", message->ID);           break;
                    case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_STATE_CREATION:        AddCategory(composed_message, composed_message_len, "STATE CREATION", message->ID);        break;
                    case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_STATE_SETTING:         AddCategory(composed_message, composed_message_len, "STATE SETTING", message->ID);         break;
                    case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_STATE_GETTING:         AddCategory(composed_message, composed_message_len, "STATE GETTING", message->ID);         break;
                    case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_RESOURCE_MANIPULATION: AddCategory(composed_message, composed_message_len, "RESOURCE MANIPULATION", message->ID); break;
                    case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_EXECUTION:             AddCategory(composed_message, composed_message_len, "EXECUTION", message->ID);             break;
                    case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_SHADER:                AddCategory(composed_message, composed_message_len, "SHADER", message->ID);                break;
                };

                switch (message->Severity)
                {
                    case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR:   renderer->GetLogger().LogError(  "%.*s", composed_message_len, composed_message); break;
                    case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING: renderer->GetLogger().LogWarning("%.*s", composed_message_len, composed_message); break;
                    default:                                       renderer->GetLogger().LogInfo(   "%.*s", composed_message_len, composed_message); break;
                }

                ZeroMemory(composed_message, composed_message_len);
            }

            renderer->InfoQueue()->ClearStoredMessages(DXGI_DEBUG_ALL);
        }

        return false;
    }
    else if (!renderer->InfoQueue())
    {
        return false;
    }
#endif
    return true;
}

bool CheckResultAndPrintMessages(HRESULT hr)
{
    return CheckResultAndPrintMessages(hr, cast<Renderer *>(GraphicsAPI::GetRenderer()));
}

}
