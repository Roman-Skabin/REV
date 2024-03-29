// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#include "core/pch.h"

//
// Import libs
//

#if REV_PLATFORM_WIN64
    #pragma comment(lib, "ntdll.lib")       // Windows
    #pragma comment(lib, "kernel32.lib")    // Windows
    #pragma comment(lib, "user32.lib")      // Windows
    #pragma comment(lib, "shlwapi.lib")     // Windows
    #pragma comment(lib, "DbgHelp.lib")     // Windows
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
