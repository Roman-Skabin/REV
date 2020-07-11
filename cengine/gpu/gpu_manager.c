//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "gpu/gpu_manager.h"
#include "cengine.h"
#include "core/id.h"

#define SafeRelease(directx_interface)                           \
{                                                                \
    if (directx_interface)                                       \
    {                                                            \
        (directx_interface)->lpVtbl->Release(directx_interface); \
        (directx_interface) = 0;                                 \
    }                                                            \
}

void SetVSync(Engine *engine, b32 enable)
{
    Check(engine);
    if (engine->gpu_manager.vsync != enable)
    {
        engine->gpu_manager.vsync = enable;
    }
}

#define AddCSTR(_message, _message_len, _cstr)                        \
{                                                                     \
    CopyMemory((_message) + (_message_len), (_cstr), CSTRLEN(_cstr)); \
    (_message_len) += CSTRLEN(_cstr);                                 \
}

#define AddCategory(_message, _message_len, _category, _id)       \
{                                                                 \
    (_message_len) += sprintf((_message) + (_message_len),        \
                              " [%.*s MESSAGE, MESSAGE_ID = %d]", \
                              cast(int, CSTRLEN(_category)),      \
                              (_category),                        \
                              _id);                               \
}

#if DEBUG
void LogDirectXMessages(Engine *engine)
{
    Check(engine);

    u64 num_messages = engine->gpu_manager.dxgi_info_queue->lpVtbl->GetNumStoredMessages(engine->gpu_manager.dxgi_info_queue,
                                                                                         DXGI_DEBUG_ALL);
    if (num_messages)
    {
        char composed_message[1024] = {0};
        u64  composed_message_len   = 0;

        for (u64 i = 0; i < num_messages; ++i)
        {
            u64 message_length = 0;
            engine->gpu_manager.error = engine->gpu_manager.dxgi_info_queue->lpVtbl->GetMessage(engine->gpu_manager.dxgi_info_queue,
                                                                                                DXGI_DEBUG_ALL,
                                                                                                i,
                                                                                                0,
                                                                                                &message_length);
            Check(SUCCEEDED(engine->gpu_manager.error));

            DXGI_INFO_QUEUE_MESSAGE *message = PushToTransientArea(engine->memory, message_length);
            engine->gpu_manager.error = engine->gpu_manager.dxgi_info_queue->lpVtbl->GetMessage(engine->gpu_manager.dxgi_info_queue,
                                                                                                DXGI_DEBUG_ALL,
                                                                                                i,
                                                                                                message,
                                                                                                &message_length);
            Check(SUCCEEDED(engine->gpu_manager.error));

            composed_message_len = 0;

            /**/ if (GUID_EQUALS(message->Producer, DXGI_DEBUG_D3D12)) AddCSTR(composed_message, composed_message_len, "D3D12 ")
            else if (GUID_EQUALS(message->Producer, DXGI_DEBUG_DXGI))  AddCSTR(composed_message, composed_message_len, "DXGI ")
            else if (GUID_EQUALS(message->Producer, DXGI_DEBUG_APP))   AddCSTR(composed_message, composed_message_len, "APP ")
            else                                                       AddCSTR(composed_message, composed_message_len, "<UNKNOWN> ")

            switch (message->Severity)
            {
                case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION: AddCSTR(composed_message, composed_message_len, "CORRUPTION: ") break;
                case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR:      AddCSTR(composed_message, composed_message_len, "ERROR: ")      break;
                case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING:    AddCSTR(composed_message, composed_message_len, "WARNING: ")    break;
                case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_INFO:       AddCSTR(composed_message, composed_message_len, "INFO: ")       break;
                case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_MESSAGE:    AddCSTR(composed_message, composed_message_len, "MESSAGE: ")    break;
            }

            CopyMemory(composed_message + composed_message_len, message->pDescription, message->DescriptionByteLength);
            composed_message_len += message->DescriptionByteLength;

            switch (message->Category)
            {
                case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_UNKNOWN:               AddCategory(composed_message, composed_message_len, "UNKNOWN", message->ID)               break;
                case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_MISCELLANEOUS:         AddCategory(composed_message, composed_message_len, "MISCELLANEOUS", message->ID)         break;
                case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_INITIALIZATION:        AddCategory(composed_message, composed_message_len, "INITIALIZATION", message->ID)        break;
                case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_CLEANUP:               AddCategory(composed_message, composed_message_len, "CLEANUP", message->ID)               break;
                case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_COMPILATION:           AddCategory(composed_message, composed_message_len, "COMPILATION", message->ID)           break;
                case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_STATE_CREATION:        AddCategory(composed_message, composed_message_len, "STATE CREATION", message->ID)        break;
                case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_STATE_SETTING:         AddCategory(composed_message, composed_message_len, "STATE SETTING", message->ID)         break;
                case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_STATE_GETTING:         AddCategory(composed_message, composed_message_len, "STATE GETTING", message->ID)         break;
                case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_RESOURCE_MANIPULATION: AddCategory(composed_message, composed_message_len, "RESOURCE MANIPULATION", message->ID) break;
                case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_EXECUTION:             AddCategory(composed_message, composed_message_len, "EXECUTION", message->ID)             break;
                case DXGI_INFO_QUEUE_MESSAGE_CATEGORY_SHADER:                AddCategory(composed_message, composed_message_len, "SHADER", message->ID)                break;
            }

            switch (message->Severity)
            {
                case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR:   LogError(&engine->gpu_manager.debug_logger,  "%.*s", composed_message_len, composed_message); break;
                case DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING: LogWarning(&engine->gpu_manager.debug_logger,"%.*s", composed_message_len, composed_message); break;
                default:                                       LogInfo(&engine->gpu_manager.debug_logger,   "%.*s", composed_message_len, composed_message); break;
            }

            ZeroMemory(composed_message, composed_message_len);
        }

        engine->gpu_manager.dxgi_info_queue->lpVtbl->ClearStoredMessages(engine->gpu_manager.dxgi_info_queue, DXGI_DEBUG_ALL);
    }

    if (num_messages = engine->gpu_manager.info_queue->lpVtbl->GetNumStoredMessages(engine->gpu_manager.info_queue))
    {
        char composed_message[1024] = {0};
        u64  composed_message_len   = 0;

        for (u64 i = 0; i < num_messages; ++i)
        {
            u64 message_length = 0;
            engine->gpu_manager.error = engine->gpu_manager.info_queue->lpVtbl->GetMessage(engine->gpu_manager.info_queue,
                                                                                           i,
                                                                                           0,
                                                                                           &message_length);
            Check(SUCCEEDED(engine->gpu_manager.error));

            D3D12_MESSAGE *message = PushToTransientArea(engine->memory, message_length);
            engine->gpu_manager.error = engine->gpu_manager.info_queue->lpVtbl->GetMessage(engine->gpu_manager.info_queue,
                                                                                           i,
                                                                                           message,
                                                                                           &message_length);
            Check(SUCCEEDED(engine->gpu_manager.error));

            composed_message_len = 0;

            switch (message->Severity)
            {
                case D3D12_MESSAGE_SEVERITY_CORRUPTION: AddCSTR(composed_message, composed_message_len, "D3D12 CORRUPTION: ") break;
                case D3D12_MESSAGE_SEVERITY_ERROR:      AddCSTR(composed_message, composed_message_len, "D3D12 ERROR: ")      break;
                case D3D12_MESSAGE_SEVERITY_WARNING:    AddCSTR(composed_message, composed_message_len, "D3D12 WARNING: ")    break;
                case D3D12_MESSAGE_SEVERITY_INFO:       AddCSTR(composed_message, composed_message_len, "D3D12 INFO: ")       break;
                case D3D12_MESSAGE_SEVERITY_MESSAGE:    AddCSTR(composed_message, composed_message_len, "D3D12 MESSAGE: ")    break;
            }

            CopyMemory(composed_message + composed_message_len, message->pDescription, message->DescriptionByteLength);
            composed_message_len += message->DescriptionByteLength;

            switch (message->Category)
            {
                case D3D12_MESSAGE_CATEGORY_APPLICATION_DEFINED:   AddCategory(composed_message, composed_message_len, "APPLICATION DEFINED", message->ID)   break;
                case D3D12_MESSAGE_CATEGORY_MISCELLANEOUS:         AddCategory(composed_message, composed_message_len, "MISCELLANEOUS", message->ID)         break;
                case D3D12_MESSAGE_CATEGORY_INITIALIZATION:        AddCategory(composed_message, composed_message_len, "INITIALIZATION", message->ID)        break;
                case D3D12_MESSAGE_CATEGORY_CLEANUP:               AddCategory(composed_message, composed_message_len, "CLEANUP", message->ID)               break;
                case D3D12_MESSAGE_CATEGORY_COMPILATION:           AddCategory(composed_message, composed_message_len, "COMPILATION", message->ID)           break;
                case D3D12_MESSAGE_CATEGORY_STATE_CREATION:        AddCategory(composed_message, composed_message_len, "STATE CREATION", message->ID)        break;
                case D3D12_MESSAGE_CATEGORY_STATE_SETTING:         AddCategory(composed_message, composed_message_len, "STATE SETTING", message->ID)         break;
                case D3D12_MESSAGE_CATEGORY_STATE_GETTING:         AddCategory(composed_message, composed_message_len, "STATE GETTING", message->ID)         break;
                case D3D12_MESSAGE_CATEGORY_RESOURCE_MANIPULATION: AddCategory(composed_message, composed_message_len, "RESOURCE MANIPULATION", message->ID) break;
                case D3D12_MESSAGE_CATEGORY_EXECUTION:             AddCategory(composed_message, composed_message_len, "EXECUTION", message->ID)             break;
                case D3D12_MESSAGE_CATEGORY_SHADER:                AddCategory(composed_message, composed_message_len, "SHADER", message->ID)                break;
            }

            switch (message->Severity)
            {
                case D3D12_MESSAGE_SEVERITY_ERROR:   LogError(&engine->gpu_manager.debug_logger,  "%.*s", composed_message_len, composed_message); break;
                case D3D12_MESSAGE_SEVERITY_WARNING: LogWarning(&engine->gpu_manager.debug_logger,"%.*s", composed_message_len, composed_message); break;
                default:                             LogInfo(&engine->gpu_manager.debug_logger,   "%.*s", composed_message_len, composed_message); break;
            }

            ZeroMemory(composed_message, composed_message_len);
        }

        engine->gpu_manager.info_queue->lpVtbl->ClearStoredMessages(engine->gpu_manager.info_queue);
    }
}
#endif
