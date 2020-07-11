//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"
#include "math/simd.h"

#ifndef GUID_DEFINED
#define GUID_DEFINED
#pragma pack(push, 1)
typedef struct // *ID
{
    u32 Data1;
    u16 Data2;
    u16 Data3;
    u8  Data4[8];
} IID, GUID, CLSID;
#pragma pack(pop)
#endif

#define DECLARE_ID(Type, name) CENGINE_DATA const Type name

#define DECLARE_IID(name)   DECLARE_ID(IID, IID_ ## name)
#define DECLARE_GUID(name)  DECLARE_ID(GUID, GUID_ ## name)
#define DECLARE_CLSID(name) DECLARE_ID(CLSID, CLSID_ ## name)

#define IID_EQUALS(_iid1, _iid2)       mm_equals(&(_iid1), &(_iid2))
#define GUID_EQUALS(_guid1, _guid2)    mm_equals(&(_guid1), &(_guid2))
#define CLSID_EQUALS(_clsid1, _clsid2) mm_equals(&(_clsid1), &(_clsid2))

#undef GUID_NULL
