//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "core/id.h"

#define DEFINE_ID(Type, name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    const Type name = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

#define DEFINE_IID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    DEFINE_ID(IID, IID_ ## name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)

#undef DEFINE_GUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    DEFINE_ID(GUID, GUID_ ## name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)

#define DEFINE_CLSID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    DEFINE_ID(CLSID, CLSID_ ## name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)

//
// Common
//

DEFINE_ID(GUID, GUID_NULL, 0x00000000, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

//
// WASAPI
//

DEFINE_CLSID(MMDeviceEnumerator, 0xBCDE0395, 0xE52F, 0x467C, 0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E);
DEFINE_IID(IMMDeviceEnumerator, 0xA95664D2, 0x9614, 0x4F35, 0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6);
DEFINE_IID(IMMDevice, 0xD666063F, 0x1587, 0x4E43, 0x81, 0xF1, 0xB9, 0x48, 0xE8, 0x07, 0x36, 0x3F);
DEFINE_IID(IAudioClient, 0x1CB9AD4C, 0xDBFA, 0x4C32, 0xB1, 0x78, 0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2);
DEFINE_IID(IAudioRenderClient, 0xF294ACFC, 0x3146, 0x4483, 0xA7, 0xBF, 0xAD, 0xDC, 0xA7, 0xC2, 0x60, 0xE2);
DEFINE_IID(ISimpleAudioVolume, 0x87CE5498, 0x68D6, 0x44E5, 0x92, 0x15, 0x6D, 0xA4, 0x7E, 0xF8, 0x83, 0xD8);
DEFINE_IID(IAudioSessionEvents, 0x24918ACC, 0x64B3, 0x37C1, 0x8C, 0xA9, 0x74, 0xA6, 0x6E, 0x99, 0x57, 0xA8);

//
// MFAPI
//

DEFINE_IID(IMFSourceReader, 0x70AE66F2, 0xC809, 0x4E4F, 0x89, 0x15, 0xBD, 0xCB, 0x40, 0x6B, 0x79, 0x93);
DEFINE_IID(IMFMediaType, 0x44AE0FA8, 0xEA31, 0x4109, 0x8D, 0x2E, 0x4C, 0xAE, 0x49, 0x97, 0xC5, 0x55);
DEFINE_IID(IMFSample, 0xC40A00F2, 0xB93A, 0x4D80, 0xAE, 0x8C, 0x5A, 0x1C, 0x63, 0x4F, 0x58, 0xE4);
DEFINE_IID(IMFMediaBuffer, 0x045FA593, 0x8799, 0x42B8, 0xBC, 0x8D, 0x89, 0x68, 0xC6, 0x45, 0x35, 0x07);