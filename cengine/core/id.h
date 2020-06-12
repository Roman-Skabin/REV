//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"

#ifndef GUID_DEFINED
#define GUID_DEFINED
typedef struct // *ID
{
    u32 Data1;
    u16 Data2;
    u16 Data3;
    u8  Data4[8];
} IID, GUID, CLSID;
#endif

#define DECLARE_ID(Type, name) CENGINE_DATA const Type __declspec(selectany) name

#define DECLARE_IID(name)   DECLARE_ID(IID, IID_ ## name)
#define DECLARE_GUID(name)  DECLARE_ID(GUID, GUID_ ## name)
#define DECLARE_CLSID(name) DECLARE_ID(CLSID, CLSID_ ## name)

#undef GUID_NULL
