//
// Copyright 2020 Roman Skabin
//

#pragma once

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <immintrin.h> // AVX
#include <Xinput.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#undef near
#undef far
