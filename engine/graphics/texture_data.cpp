// Copyright (c) 2020-2022, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#include "graphics/memory_manager.h"
#include "memory/memory.h"
#include "math/math.h"

namespace REV
{

void TextureData::Init(const TextureDataDesc& desc)
{
    REV_CHECK(desc.dimension != TEXTURE_DIMENSION_UNKNOWN);
    REV_CHECK(TEXTURE_FORMAT_FIRST_UNCOMPRESSED <= desc.format && desc.format <= TEXTURE_FORMAT_LAST_COMPRESSED);

    u16 bpp_or_bpb   = GetBitsPerPixelOrBitsPerBlock(desc.format);
    u16 planes_count = GraphicsAPI::GetDeviceContext()->GetFormatPlanesCount(desc.format);

    u64 surfaces_count = desc.mip_levels * desc.array_size * planes_count;
    if (desc.dimension == TEXTURE_DIMENSION_CUBE || desc.dimension == TEXTURE_DIMENSION_CUBE_ARRAY)
        surfaces_count *= 6;

    u64 bytes_to_allocate = REV_StructFieldOffset(Header, surfaces) + sizeof(TextureSurface) * surfaces_count;

    m_Header = cast(Header *, Memory::Get()->PushToFrameArena(bytes_to_allocate));
    m_Header->dimension         = desc.dimension;
    m_Header->format            = desc.format;
    m_Header->width             = Math::max(1, desc.width);
    m_Header->height            = Math::max(1, desc.height);
    m_Header->depth             = Math::max(1, desc.depth);
    m_Header->mip_levels        = Math::max(1, desc.mip_levels);
    m_Header->array_size        = Math::max(1, desc.array_size);
    m_Header->planes_count      = Math::max(1, planes_count);
    m_Header->surfaces_count    = surfaces_count;

    for (u16 plane = 0; plane < m_Header->planes_count; ++plane)
    {
        for (u16 subtexture = 0; subtexture < m_Header->array_size; ++subtexture)
        {
            u16 mip_width  = m_Header->width;
            u16 mip_height = m_Header->height;
            u16 mip_depth  = m_Header->depth;

            for (u16 mip_level = 0; mip_level < m_Header->mip_levels; ++mip_level)
            {
                TextureSurface *surface = m_Header->surfaces
                                        + plane      * m_Header->mip_levels * m_Header->array_size
                                        + subtexture * m_Header->mip_levels
                                        + mip_level;

                surface->data = desc.data;
                GetSurfaceSizes(mip_width, mip_height, bpp_or_bpb, m_Header->format, *surface);

                mip_width  = Math::max(1, mip_width  >> 1);
                mip_height = Math::max(1, mip_height >> 1);
                mip_depth  = Math::max(1, mip_depth  >> 1);

                desc.data += surface->size_in_bytes;
            }
        }
    }
}

void TextureData::InitFromNonLinearData(u64 surfaces_count, const InitFromNonLinearDataCallback& callback)
{
    u64 bytes_to_allocate = REV_StructFieldOffset(Header, surfaces) + sizeof(TextureSurface) * surfaces_count;

    m_Header = cast(Header *, Memory::Get()->PushToFrameArena(bytes_to_allocate));
    m_Header->surfaces_count = surfaces_count;

    callback(m_Header);
}

const void *TextureData::GetPixelData(Math::v2u xy, u16 mip_level, u16 subtexture, u16 plane) const
{
    REV_CHECK(mip_level  < m_Header->mip_levels);
    REV_CHECK(subtexture < m_Header->array_size);
    REV_CHECK(plane      < m_Header->planes_count);

    REV_CHECK_M(TEXTURE_FORMAT_FIRST_UNCOMPRESSED <= m_Header->format && m_Header->format <= TEXTURE_FORMAT_LAST_UNCOMPRESSED,
                "Uncompressed textures support GetPixelData function only");

    const TextureSurface *surface = m_Header->surfaces
                                  + plane      * m_Header->mip_levels * m_Header->array_size
                                  + subtexture * m_Header->mip_levels
                                  + mip_level;

    u16 bits_per_pixel  = GetBitsPerPixelOrBitsPerBlock(m_Header->format);
    u16 bytes_per_pixel = (bits_per_pixel + 7) / 8;

    u32 last_pixel_x = surface->row_bytes     / bytes_per_pixel;
    u64 last_pixel_y = surface->size_in_bytes / surface->row_bytes;

    if (last_pixel_x) --last_pixel_x;
    if (last_pixel_y) --last_pixel_y;

    Math::v2u clamped_uv = Math::v2u::clamp(xy, Math::v2u(), Math::v2u(last_pixel_x, cast(u32, last_pixel_y)));

    return surface->data
         + clamped_uv.y * surface->row_bytes
         + clamped_uv.x * bytes_per_pixel;
}

void *TextureData::GetPixelData(Math::v2u xy, u16 mip_level, u16 subtexture, u16 plane)
{
    REV_CHECK(mip_level  < m_Header->mip_levels);
    REV_CHECK(subtexture < m_Header->array_size);
    REV_CHECK(plane      < m_Header->planes_count);

    REV_CHECK_M(TEXTURE_FORMAT_FIRST_UNCOMPRESSED <= m_Header->format && m_Header->format <= TEXTURE_FORMAT_LAST_UNCOMPRESSED,
                "Uncompressed textures support GetPixelData function only");

    TextureSurface *surface = m_Header->surfaces
                            + plane      * m_Header->mip_levels * m_Header->array_size
                            + subtexture * m_Header->mip_levels
                            + mip_level;

    u16 bits_per_pixel  = GetBitsPerPixelOrBitsPerBlock(m_Header->format);
    u16 bytes_per_pixel = (bits_per_pixel + 7) / 8;

    u32 last_pixel_x = surface->row_bytes     / bytes_per_pixel;
    u64 last_pixel_y = surface->size_in_bytes / surface->row_bytes;

    if (last_pixel_x) --last_pixel_x;
    if (last_pixel_y) --last_pixel_y;

    Math::v2u clamped_uv = Math::v2u::clamp(xy, Math::v2u(), Math::v2u(last_pixel_x, cast(u32, last_pixel_y)));

    return surface->data
         + clamped_uv.y * surface->row_bytes
         + clamped_uv.x * bytes_per_pixel;
}

void TextureData::ForEachSurface(const ForEachPixelConstCallback& callback) const
{
    u16 surfaces_on_plane = m_Header->mip_levels * m_Header->array_size;

    for (u16 i = 0; i < m_Header->surfaces_count; i++)
    {
        const TextureSurface *surface    = m_Header->surfaces + i;
        u16                   plane      = i / surfaces_on_plane;
        u16                   subtexture = i % surfaces_on_plane / m_Header->mip_levels;
        u16                   mip_level  = i % surfaces_on_plane % m_Header->mip_levels;

        if (callback(surface, mip_level, subtexture, plane))
        {
            break;
        }
    }
}

void TextureData::ForEachSurface(const ForEachPixelCallback& callback)
{
    u16 surfaces_on_plane = m_Header->mip_levels * m_Header->array_size;

    for (u16 i = 0; i < m_Header->surfaces_count; i++)
    {
        TextureSurface *surface    = m_Header->surfaces + i;
        u16             plane      = i / surfaces_on_plane;
        u16             subtexture = i % surfaces_on_plane / m_Header->mip_levels;
        u16             mip_level  = i % surfaces_on_plane % m_Header->mip_levels;

        if (callback(surface, mip_level, subtexture, plane))
        {
            break;
        }
    }
}

void TextureData::ForEachPixelXYZ(const TextureSurface *surface, const ForEachSurfaceConstCallback& callback) const
{
    // @TODO, @Incomplete(Roman)
    REV_ERROR_M("Not implemented");
}

void TextureData::ForEachPixelXYZ(const ForEachSurfaceCallback& callback)
{
    // @TODO, @Incomplete(Roman)
    REV_ERROR_M("Not implemented");
}

u16 TextureData::GetBitsPerPixelOrBitsPerBlock(TEXTURE_FORMAT format)
{
    switch (format)
    {
        case TEXTURE_FORMAT_S32:         return 32;
        case TEXTURE_FORMAT_U32:         return 32;

        case TEXTURE_FORMAT_R32:         return 32;
        case TEXTURE_FORMAT_RG32:        return 64;
        case TEXTURE_FORMAT_RGB32:       return 96;
        case TEXTURE_FORMAT_RGBA32:      return 128;

        case TEXTURE_FORMAT_BGRA8:       return 32;
        case TEXTURE_FORMAT_RGBA8:       return 32;
        case TEXTURE_FORMAT_A8:          return 8;
        case TEXTURE_FORMAT_B5G6R5:      return 16;
        case TEXTURE_FORMAT_BGR5A1:      return 16;
        case TEXTURE_FORMAT_BGRA4:       return 16;
        case TEXTURE_FORMAT_R10G10B10A2: return 32;
        case TEXTURE_FORMAT_RGBA16:      return 64;

        case TEXTURE_FORMAT_D16:         return 16;
        case TEXTURE_FORMAT_D32:         return 32;
        case TEXTURE_FORMAT_D24S8:       return 32;

        case TEXTURE_FORMAT_BC1:         return 8;
        case TEXTURE_FORMAT_BC2:         return 16;
        case TEXTURE_FORMAT_BC3:         return 16;
        case TEXTURE_FORMAT_BC4:         return 8;
        case TEXTURE_FORMAT_BC5:         return 16;
        case TEXTURE_FORMAT_BC7:         return 16;
    }

    REV_ERROR_M("Invalid or unsupported TEXTURE_FORMAT: 0x%X", format);
    return 0;
}

void TextureData::GetSurfaceSizes(u16 width, u16 height, u16 bpp_or_bpb, TEXTURE_FORMAT format, TextureSurface& surface)
{
    if (TEXTURE_FORMAT_FIRST_UNCOMPRESSED <= format && format <= TEXTURE_FORMAT_LAST_UNCOMPRESSED)
    {
        surface.row_bytes     = (width * bpp_or_bpb + 7) / 8;
        surface.size_in_bytes = height * surface.row_bytes;
    }
    else if (TEXTURE_FORMAT_FIRST_COMPRESSED <= format && format <= TEXTURE_FORMAT_LAST_COMPRESSED)
    {
        surface.row_bytes     = ((width  + 3) / 4) * bpp_or_bpb;
        surface.size_in_bytes = ((height + 3) / 4) * surface.row_bytes;
    }
    else
    {
        REV_ERROR_M("Invalid or unsupported TEXTURE_FORMAT: 0x%X", format);
    }
}

}
