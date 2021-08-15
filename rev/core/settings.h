//
// Copyright 2020-2021 Roman Skabin
//

#pragma once

#include "math/vec.h"
#include "graphics/graphics_api.h"
#include "tools/static_string.hpp"

namespace REV
{
    enum class FILTERING : u16
    {
        POINT,
        BILINEAR,
        TRILINEAR,
        ANISOTROPIC
    };

    struct REV_API Settings final
    {
        Math::v4s                       window_xywh; // x, y, width, height
        Math::v2s                       render_target_wh;
        GraphicsAPI::API                graphics_api;
        FILTERING                       filtering;
        u8                              anisotropy : 6;
        u8                              fullscreen : 1;
        u8                              vsync      : 1;
        StaticString<REV_PATH_CAPACITY> assets_folder;

        static Settings *Get();

    private:
        static Settings *Init(const char *ini_filename);

        friend class ::REV::Application;
    };
}
