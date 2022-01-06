// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#include "core/common.h"

namespace REV::Windows
{
REV_EXTERN_START

    //
    // Common
    //

    // @NOTE(Roman): Some of the NT status codes are in winnt.h.
    //               I don't use nt*.h and/or wdf*.h headers
    //               because the include order of NT and WDF
    //               headers are not obvious at all.
    //               Especially when you use Windows.h as well.
    //               NT status codes will be added as they are needed.
    typedef _Return_type_success_(return >= 0) LONG NTSTATUS;

    #undef STATUS_WAIT_0
    #undef STATUS_ABANDONED_WAIT_0

    #define STATUS_SUCCESS                  0x00000000
    #define STATUS_WAIT_0                   0x00000000
    #define STATUS_WAIT_63                  0x0000003F
    #define STATUS_ABANDONED                0x00000080
    #define STATUS_ABANDONED_WAIT_0         0x00000080
    #define STATUS_ABANDONED_WAIT_63        0x000000BF
    #define STATUS_SEMAPHORE_LIMIT_EXCEEDED 0xC0000047

    typedef struct _UNICODE_STRING {
        USHORT Length;
        USHORT MaximumLength;
        PWSTR  Buffer;
    } UNICODE_STRING, *PUNICODE_STRING;

    typedef struct _OBJECT_ATTRIBUTES {
        ULONG           Length;
        HANDLE          RootDirectory;
        PUNICODE_STRING ObjectName;
        ULONG           Attributes;
        PVOID           SecurityDescriptor;
        PVOID           SecurityQualityOfService;
    } OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

    typedef struct _CLIENT_ID {
        HANDLE UniqueProcess;
        HANDLE UniqueThread;
    } CLIENT_ID, *PCLIENT_ID;

    NTSYSAPI NTSTATUS NTAPI NtClose(
        _In_ HANDLE ObjectHandle
    );

    NTSYSAPI NTSTATUS NTAPI NtWaitForSingleObject(
        _In_     HANDLE         ObjectHandle,
        _In_     BOOLEAN        Alertable,
        _In_opt_ PLARGE_INTEGER TimeOut
    );

    //
    // Semaphore
    //

    typedef enum _SEMAPHORE_INFORMATION_CLASS {
        SemaphoreBasicInformation = 0
    } SEMAPHORE_INFORMATION_CLASS, *PSEMAPHORE_INFORMATION_CLASS;

    typedef struct _SEMAPHORE_BASIC_INFORMATION {
        ULONG CurrentCount;
        ULONG MaximumCount;
    } SEMAPHORE_BASIC_INFORMATION, *PSEMAPHORE_BASIC_INFORMATION;

    NTSYSAPI NTSTATUS NTAPI NtCreateSemaphore(
        _Out_    PHANDLE            SemaphoreHandle,
        _In_     ACCESS_MASK        DesiredAccess,
        _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
        _In_     ULONG              InitialCount,
        _In_     ULONG              MaximumCount
    );

    NTSYSAPI NTSTATUS NTAPI NtOpenSemaphore(
        _Out_ PHANDLE            SemaphoreHandle,
        _In_  ACCESS_MASK        DesiredAccess,
        _In_  POBJECT_ATTRIBUTES ObjectAttributes
    );

    NTSYSAPI NTSTATUS NTAPI NtQuerySemaphore(
        _In_                                           HANDLE                       SemaphoreHandle,
        _In_                                           SEMAPHORE_INFORMATION_CLASS  SemaphoreInformationClass,
        _Out_writes_bytes_(SemaphoreInformationLength) PSEMAPHORE_BASIC_INFORMATION SemaphoreInformation,
        _In_                                           ULONG                        SemaphoreInformationLength,
        _Out_opt_                                      PULONG                       ReturnLength
    );

    NTSYSAPI NTSTATUS NTAPI NtReleaseSemaphore(
        _In_      HANDLE SemaphoreHandle,
        _In_      ULONG  ReleaseCount,
        _Out_opt_ PULONG PreviousCount
    );

REV_EXTERN_END
}
