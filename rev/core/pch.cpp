//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"

//
// Import libs
//

#if REV_PLATFORM_WIN64
    #pragma comment(lib, "User32.lib")      // Windows
    #pragma comment(lib, "d3d12.lib")       // DirectX
    #pragma comment(lib, "dxgi.lib")        // DirectX
    #pragma comment(lib, "d3dcompiler.lib") // DirectX
//  #pragma comment(lib, "mfplat.lib")      // MFAPI
//  #pragma comment(lib, "mfreadwrite.lib") // MFAPI
#elif REV_PLATFORM_MACOS
    // @TODO(Roman): Platform-specific links
#elif REV_PLATFORM_LINUX
    // @TODO(Roman): Platform-specific links
#endif
