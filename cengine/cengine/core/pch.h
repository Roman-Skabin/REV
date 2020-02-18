//
// Copyright 2020 Roman Skabin
//

#pragma once

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS
#define INITGUID

#include <Windows.h>
#include <immintrin.h> // AVX
#include <Xinput.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <AudioClient.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#undef near
#undef far
