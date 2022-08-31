// Copyright (c) 2020-2022, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "graphics/memory_manager.h"
#include "tools/function.hpp"

namespace REV
{
    struct TextureSurface
    {
        byte *data          = null;
        u64   size_in_bytes = 0;
        u32   row_bytes     = 0;
    };

    struct TextureDataDesc
    {
        TEXTURE_DIMENSION dimension   = TEXTURE_DIMENSION_UNKNOWN;
        TEXTURE_FORMAT    format      = TEXTURE_FORMAT_UNKNOWN;
        u16               width       = 1;
        u16               height      = 1;
        u16               depth       = 1;
        u16               mip_levels  = 1;
        u16               array_size  = 1;
        const byte       *data        = null;
    };

    struct TextureDataHeader
    {
        TEXTURE_DIMENSION dimension;
        TEXTURE_FORMAT    format;
        u16               width;
        u16               height;
        u16               depth;
        u16               mip_levels;
        u16               array_size;
        u16               planes_count;

        u16            surfaces_count;
        TextureSurface surfaces[0];
    };

    class REV_API TextureData final
    {
    public:
        // @NOTE(Roman): return true means break loop.
        using ForEachSurfaceConstCallback = Function<bool(const TextureSurface *surface, u16 mip_level, u16 subtexture, u16 plane)>;
        using ForEachSurfaceCallback      = Function<bool(TextureSurface *surface, u16 mip_level, u16 subtexture, u16 plane)>;

        using ForEachPixelXYZConstCallback = Function<bool(const void *pixel, TEXTURE_FORMAT format, Math::vec3 uvw)>;
        using ForEachPixelXYZCallback      = Function<bool(void *pixel, TEXTURE_FORMAT format, Math::vec3 uvw)>;

        using ForEachPixelUVWConstCallback = Function<bool(const void *pixel, TEXTURE_FORMAT format, Math::vec3 uvw)>;
        using ForEachPixelUVWCallback      = Function<bool(void *pixel, TEXTURE_FORMAT format, Math::vec3 uvw)>;

        using InitFromNonLinearDataCallback = Function<void(TextureDataHeader *header)>;

    public:
        void Init(const TextureDataDesc& desc);
        void InitFromNonLinearData(u64 surfaces_count, const InitFromNonLinearDataCallback& callback);

        const void *GetPixelData(Math::v2u xy, u16 mip_level = 0, u16 subtexture = 0, u16 plane = 0) const;
              void *GetPixelData(Math::v2u xy, u16 mip_level = 0, u16 subtexture = 0, u16 plane = 0);

        void ForEachSurface(const ForEachSurfaceConstCallback& callback) const;
        void ForEachSurface(const ForEachSurfaceCallback& callback);

        void ForEachPixelXYZ(const TextureSurface *surface, const ForEachSurfaceConstCallback& callback) const;
        void ForEachPixelXYZ(TextureSurface *surface, const ForEachSurfaceCallback& callback);

        bool Empty() const;
        u16 PlanesCount() const;
        u16 SurfacesCount() const;

    private:
        static u16 GetBitsPerPixelOrBitsPerBlock(TEXTURE_FORMAT format);
        static void GetSurfaceSizes(u16 width, u16 height, u16 bpp_or_bpb, TEXTURE_FORMAT format, TextureSurface& surface);

    private:
        TextureDataHeader *m_Header;
    };
}

#include "graphics/texture_data.inl"
