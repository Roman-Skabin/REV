// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#include "core/pch.h"
#include "asset_manager/asset_manager.h"
#include "graphics/graphics_api.h"
#include "core/settings.h"
#include "memory/memory.h"
#include "math/math.h"

// @TODO(Roman): #CrossPlatform
#include "platform/d3d12/d3d12_memory_manager.h"
#include "platform/d3d12/d3d12_common.h"

#pragma pack(push, 1)
namespace REV
{

enum DDS_FLAG : u32
{
    DDS_FLAG_CAPS             = 0x00000001,
    DDS_FLAG_HEIGHT           = 0x00000002,
    DDS_FLAG_WIDTH            = 0x00000004,
    DDS_FLAG_PITCH            = 0x00000008,
    DDS_FLAG_BACKBUFFERCOUNT  = 0x00000020,
    DDS_FLAG_ZBUFFERBITDEPTH  = 0x00000040,
    DDS_FLAG_ALPHABITDEPTH    = 0x00000080,
    DDS_FLAG_LPSURFACE        = 0x00000800,
    DDS_FLAG_PIXELFORMAT      = 0x00001000,
    DDS_FLAG_CKDESTOVERLAY    = 0x00002000,
    DDS_FLAG_CKDESTBLT        = 0x00004000,
    DDS_FLAG_CKSRCOVERLAY     = 0x00008000,
    DDS_FLAG_CKSRCBLT         = 0x00010000,
    DDS_FLAG_MIPMAPCOUNT      = 0x00020000,
    DDS_FLAG_REFRESHRATE      = 0x00040000,
    DDS_FLAG_LINEARSIZE       = 0x00080000,
    DDS_FLAG_TEXTURESTAGE     = 0x00100000,
    DDS_FLAG_FVF              = 0x00200000,
    DDS_FLAG_SRCVBHANDLE      = 0x00400000,
    DDS_FLAG_DEPTH            = 0x00800000,
    DDS_FLAG_ALL              = 0x00FFF9EE
};
REV_ENUM_OPERATORS(DDS_FLAG);
static_assert(sizeof(DDS_FLAG) == 4);

enum PIXEL_FORMAT_FLAG : u32
{
    PIXEL_FORMAT_FLAG_ALPHAPIXELS       = 0x00000001,
    PIXEL_FORMAT_FLAG_ALPHA             = 0x00000002,
    PIXEL_FORMAT_FLAG_FOURCC            = 0x00000004,
    PIXEL_FORMAT_FLAG_PALETTEINDEXED4   = 0x00000008,
    PIXEL_FORMAT_FLAG_PALETTEINDEXEDTO8 = 0x00000010,
    PIXEL_FORMAT_FLAG_PALETTEINDEXED8   = 0x00000020,
    PIXEL_FORMAT_FLAG_RGB               = 0x00000040,
    PIXEL_FORMAT_FLAG_COMPRESSED        = 0x00000080,
    PIXEL_FORMAT_FLAG_RGBTOYUV          = 0x00000100,
    PIXEL_FORMAT_FLAG_YUV               = 0x00000200,
    PIXEL_FORMAT_FLAG_ZBUFFER           = 0x00000400,
    PIXEL_FORMAT_FLAG_PALETTEINDEXED1   = 0x00000800,
    PIXEL_FORMAT_FLAG_PALETTEINDEXED2   = 0x00001000,
    PIXEL_FORMAT_FLAG_ZPIXELS           = 0x00002000,
    PIXEL_FORMAT_FLAG_STENCILBUFFER     = 0x00004000,
    PIXEL_FORMAT_FLAG_ALPHAPREMULT      = 0x00008000,
    PIXEL_FORMAT_FLAG_LUMINANCE         = 0x00020000,
    PIXEL_FORMAT_FLAG_BUMPLUMINANCE     = 0x00040000,
    PIXEL_FORMAT_FLAG_BUMPDUDV          = 0x00080000,
};
REV_ENUM_OPERATORS(PIXEL_FORMAT_FLAG);
static_assert(sizeof(PIXEL_FORMAT_FLAG) == 4);

enum CAPS1 : u32
{
    CAPS1_RESERVED1          = 0x00000001,
    CAPS1_ALPHA              = 0x00000002,
    CAPS1_BACKBUFFER         = 0x00000004,
    CAPS1_COMPLEX            = 0x00000008,
    CAPS1_FLIP               = 0x00000010,
    CAPS1_FRONTBUFFER        = 0x00000020,
    CAPS1_OFFSCREENPLAIN     = 0x00000040,
    CAPS1_OVERLAY            = 0x00000080,
    CAPS1_PALETTE            = 0x00000100,
    CAPS1_PRIMARYSURFACE     = 0x00000200,
    CAPS1_RESERVED3          = 0x00000400,
    CAPS1_PRIMARYSURFACELEFT = 0x00000000,
    CAPS1_SYSTEMMEMORY       = 0x00000800,
    CAPS1_TEXTURE            = 0x00001000,
    CAPS1_3DDEVICE           = 0x00002000,
    CAPS1_VIDEOMEMORY        = 0x00004000,
    CAPS1_VISIBLE            = 0x00008000,
    CAPS1_WRITEONLY          = 0x00010000,
    CAPS1_ZBUFFER            = 0x00020000,
    CAPS1_OWNDC              = 0x00040000,
    CAPS1_LIVEVIDEO          = 0x00080000,
    CAPS1_HWCODEC            = 0x00100000,
    CAPS1_MODEX              = 0x00200000,
    CAPS1_MIPMAP             = 0x00400000,
    CAPS1_RESERVED2          = 0x00800000,
    CAPS1_ALLOCONLOAD        = 0x04000000,
    CAPS1_VIDEOPORT          = 0x08000000,
    CAPS1_LOCALVIDMEM        = 0x10000000,
    CAPS1_NONLOCALVIDMEM     = 0x20000000,
    CAPS1_STANDARDVGAMODE    = 0x40000000,
    CAPS1_OPTIMIZED          = 0x80000000,
};
REV_ENUM_OPERATORS(CAPS1);
static_assert(sizeof(CAPS1) == 4);

enum CAPS2 : u32
{
    CAPS2_RESERVED4             = 0x00000002,
    CAPS2_HARDWAREDEINTERLACE   = 0x00000000,
    CAPS2_HINTDYNAMIC           = 0x00000004,
    CAPS2_HINTSTATIC            = 0x00000008,
    CAPS2_TEXTUREMANAGE         = 0x00000010,
    CAPS2_RESERVED1             = 0x00000020,
    CAPS2_RESERVED2             = 0x00000040,
    CAPS2_OPAQUE                = 0x00000080,
    CAPS2_HINTANTIALIASING      = 0x00000100,
    CAPS2_CUBEMAP               = 0x00000200,
    CAPS2_CUBEMAP_POSITIVEX     = 0x00000400,
    CAPS2_CUBEMAP_NEGATIVEX     = 0x00000800,
    CAPS2_CUBEMAP_POSITIVEY     = 0x00001000,
    CAPS2_CUBEMAP_NEGATIVEY     = 0x00002000,
    CAPS2_CUBEMAP_POSITIVEZ     = 0x00004000,
    CAPS2_CUBEMAP_NEGATIVEZ     = 0x00008000,
    CAPS2_CUBEMAP_ALLFACES      = CAPS2_CUBEMAP_POSITIVEX
                                | CAPS2_CUBEMAP_NEGATIVEX
                                | CAPS2_CUBEMAP_POSITIVEY
                                | CAPS2_CUBEMAP_NEGATIVEY
                                | CAPS2_CUBEMAP_POSITIVEZ
                                | CAPS2_CUBEMAP_NEGATIVEZ,
    CAPS2_MIPMAPSUBLEVEL        = 0x00010000,
    CAPS2_D3DTEXTUREMANAGE      = 0x00020000,
    CAPS2_DONOTPERSIST          = 0x00040000,
    CAPS2_STEREOSURFACELEFT     = 0x00080000,
    CAPS2_VOLUME                = 0x00200000,
    CAPS2_NOTUSERLOCKABLE       = 0x00400000,
    CAPS2_POINTS                = 0x00800000,
    CAPS2_RTPATCHES             = 0x01000000,
    CAPS2_NPATCHES              = 0x02000000,
    CAPS2_RESERVED3             = 0x04000000,
    CAPS2_DISCARDBACKBUFFER     = 0x10000000,
    CAPS2_ENABLEALPHACHANNEL    = 0x20000000,
    CAPS2_EXTENDEDFORMATPRIMARY = 0x40000000,
    CAPS2_ADDITIONALPRIMARY     = 0x80000000,
};
REV_ENUM_OPERATORS(CAPS2);
static_assert(sizeof(CAPS2) == 4);

enum CAPS3 : u32
{
    CAPS3_MULTISAMPLE_MASK          = 0x0000001F,
    CAPS3_MULTISAMPLE_QUALITY_MASK  = 0x000000E0,
    CAPS3_MULTISAMPLE_QUALITY_SHIFT = 5,
    CAPS3_RESERVED1                 = 0x00000100,
    CAPS3_RESERVED2                 = 0x00000200,
    CAPS3_LIGHTWEIGHTMIPMAP         = 0x00000400,
    CAPS3_AUTOGENMIPMAP             = 0x00000800,
    CAPS3_DMAP                      = 0x00001000,
    CAPS3_CREATESHAREDRESOURCE      = 0x00002000,
    CAPS3_READONLYRESOURCE          = 0x00004000,
    CAPS3_OPENSHAREDRESOURCE        = 0x00008000,
};
REV_ENUM_OPERATORS(CAPS3);
static_assert(sizeof(CAPS3) == 4);

#define REV_MAKE_FOURCC(str) ((str[3] << 24) | (str[2] << 16) | (str[1] << 8) | str[0])

enum FOURCC : u32
{
    FOURCC_DXT1 = REV_MAKE_FOURCC("DXT1"),
    FOURCC_DXT2 = REV_MAKE_FOURCC("DXT2"),
    FOURCC_DXT3 = REV_MAKE_FOURCC("DXT3"),
    FOURCC_DXT4 = REV_MAKE_FOURCC("DXT4"),
    FOURCC_DXT5 = REV_MAKE_FOURCC("DXT5"),

    FOURCC_DX10 = REV_MAKE_FOURCC("DX10"),
};
static_assert(sizeof(FOURCC) == 4);

enum : u32
{
    DDS_MAGIC = REV_MAKE_FOURCC("DDS "),
};

enum MISC_FLAG : u32
{
    MISC_FLAG_GENERATE_MIPS                   = 0x00001,
    MISC_FLAG_SHARED                          = 0x00002,
    MISC_FLAG_TEXTURECUBE                     = 0x00004,
    MISC_FLAG_DRAWINDIRECT_ARGS               = 0x00010,
    MISC_FLAG_BUFFER_ALLOW_RAW_VIEWS          = 0x00020,
    MISC_FLAG_BUFFER_STRUCTURED               = 0x00040,
    MISC_FLAG_RESOURCE_CLAMP                  = 0x00080,
    MISC_FLAG_SHARED_KEYEDMUTEX               = 0x00100,
    MISC_FLAG_GDI_COMPATIBLE                  = 0x00200,
    MISC_FLAG_SHARED_NTHANDLE                 = 0x00800,
    MISC_FLAG_RESTRICTED_CONTENT              = 0x01000,
    MISC_FLAG_RESTRICT_SHARED_RESOURCE        = 0x02000,
    MISC_FLAG_RESTRICT_SHARED_RESOURCE_DRIVER = 0x04000,
    MISC_FLAG_GUARDED                         = 0x08000,
    MISC_FLAG_TILE_POOL                       = 0x20000,
    MISC_FLAG_TILED                           = 0x40000,
    MISC_FLAG_HW_PROTECTED                    = 0x80000,
};
REV_ENUM_OPERATORS(MISC_FLAG);
static_assert(sizeof(MISC_FLAG) == 4);

enum MISC_FLAG2 : u32
{
    MISC_FLAG2_ALPHA_MODE_MASK = 0x7,
};
REV_ENUM_OPERATORS(MISC_FLAG2);
static_assert(sizeof(MISC_FLAG2) == 4);

struct PixelFormat
{
    u32               self_size;
    PIXEL_FORMAT_FLAG flags;
    union
    {
        char          str[4];
        FOURCC        _enum;
    } fourcc;
    union
    {
        u32           rgb_bit_count;
        u32           yuv_bit_count;
        u32           zbuffer_bit_depth;
        u32           alpha_bit_depth;
        u32           luminance_bit_count;
        u32           bump_bit_count;
        u32           private_format_bit_count;
    };
    union
    {
        u32           r_bit_mask;
        u32           y_bit_mask;
        u32           stencil_bit_depth;
        u32           luminance_bit_mask;
        u32           bump_du_bit_mask;
        u32           operations;
    };
    union
    {
        u32           g_bit_mask;
        u32           u_bit_mask;
        u32           z_bit_mask;
        u32           bump_dv_bit_mask;
        struct
        {
            u16       flip_ms_types;
            u16       blt_ms_types;
        } multisample_caps;
    };
    union
    {
        u32           b_bit_mask;
        u32           v_bit_mask;
        u32           stencil_bit_mask;
        u32           bump_luminance_bit_mask;
    };
    union
    {
        u32           a_bit_mask;
        u32           z_bit_mask;
    };
};

struct Caps2
{
    CAPS1   caps1;
    CAPS2   caps2;
    CAPS3   caps3;
    union
    {
        u32 caps4;
        u32 volume_depth;
    };
};

struct ColorKey
{
    u32 color_space_low_value;
    u32 color_space_high_value;
};

struct DDSHeader
{
    u32             self_size;
    DDS_FLAG        flags;
    u32             height;
    u32             width;
    union
    {
        s32         pitch;
        u32         linear_size;
    };
    union
    {
        u32         back_buffer_count;
        u32         depth;
    };
    union
    {
        u32         mipmap_count;
        u32         refresh_rate;
        u32         src_vb_handle;
    };
    u32             alpha_bit_depth;
    u32             reserved;
    intptr32        surface;
    union
    {
        ColorKey    ck_dest_overlay;
        u32         empty_face_color;
    };
    ColorKey        ck_dest_blt;
    ColorKey        ck_src_overlay;
    ColorKey        ck_src_blt;
    union
    {
        PixelFormat pixel_format;
        u32         vertex_format; // @NOTE(Roman): for vertex buffers only
    };
    Caps2           caps;
    u32             texture_stage;
};

struct DX10Header
{
    DXGI_FORMAT              format;
    D3D12_RESOURCE_DIMENSION dimension;
    MISC_FLAG                misc_flags;
    u32                      array_size;
    MISC_FLAG2               misc_flags2;
};

void AssetManager::LoadDDSTexture(Asset *asset, const ConstArray<byte>& data, const ConstString& name, bool permanent)
{
    MemoryManager *memory_manager = GraphicsAPI::GetMemoryManager();

    u32 magic = *cast(u32 *, data.Data());
    REV_CHECK_M(magic == DDS_MAGIC, "This is not a DDS file");

    DDSHeader *dds_header = cast(DDSHeader *, data.Data() + sizeof(u32));
    REV_CHECK_M(dds_header->self_size == sizeof(DDSHeader) && dds_header->pixel_format.self_size == sizeof(PixelFormat), "Invalid DDS file layout");

    DX10Header     *dx10_header    = null;
    u32             data_offset    = sizeof(u32) + sizeof(DDSHeader);
    TEXTURE_FORMAT  texture_format = TEXTURE_FORMAT_UNKNOWN;

    if (dds_header->pixel_format.flags & PIXEL_FORMAT_FLAG_FOURCC)
    {
        switch (dds_header->pixel_format.fourcc._enum)
        {
            case FOURCC_DXT1:
            {
                texture_format = TEXTURE_FORMAT_BC1;
            } break;

            case FOURCC_DXT2:
            case FOURCC_DXT3:
            {
                texture_format = TEXTURE_FORMAT_BC2;
            } break;

            case FOURCC_DXT4:
            case FOURCC_DXT5:
            {
                texture_format = TEXTURE_FORMAT_BC3;
            } break;

            case FOURCC_DX10:
            {
                dx10_header     = cast(DX10Header *, data.Data() + sizeof(u32) + sizeof(DDSHeader));
                data_offset    += sizeof(DX10Header);
                texture_format  = D3D12::DXGIToREVTextureFormat(dx10_header->format);
            } break;

            default:
            {
                // @TODO(Roman): #OtherFormats
                REV_ERROR_M("Unhandled DDS texture format: %.4s", dds_header->pixel_format.fourcc.str);
            } break;
        };
    }
    else
    {
        // @TODO(Roman): #OtherFormats: BC4, BC5, Uncompressed RGB(A), YCrCb, ...
        REV_ERROR_M("Unhandled DDS texture format");
    }

    // @TODO(Roman): Rewrite
    RESOURCE_FLAG   resource_flags = RESOURCE_FLAG_CPU_WRITE_ONCE
                                   | RESOURCE_FLAG_GPU_READ
                                   | (permanent ? RESOURCE_FLAG_PERMANENT : RESOURCE_FLAG_NONE);
    u16             mipmap_count   = (dds_header->caps.caps1 & CAPS1_MIPMAP) ? cast(u16, dds_header->mipmap_count) : 1;

    TextureDataDesc texture_data_desc;
    texture_data_desc.data              = data.Data() + data_offset;
    texture_data_desc.format            = texture_format;
    texture_data_desc.width             = dds_header->width;
    texture_data_desc.height            = dds_header->height;
    texture_data_desc.depth             = dds_header->depth;
    texture_data_desc.mip_levels_count  = mipmap_count;
    texture_data_desc.subtextures_count = 1;

    if (dx10_header)                            texture_data_desc.subtextures_count  = dx10_header->array_size;
    if (dds_header->caps.caps2 & CAPS2_CUBEMAP) texture_data_desc.subtextures_count *= 6;

    TextureData texture_data;
    texture_data.Init(texture_data_desc);

    // @TODO(Roman): Rewrite
    if (dx10_header)
    {
        switch (dx10_header->dimension)
        {
            case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
            {
                REV_CHECK(dds_header->caps.caps2 & CAPS2_VOLUME);
                REV_CHECK(dx10_header->array_size == 1);

                asset->texture = memory_manager->AllocateTexture3D(cast(u16, dds_header->width),
                                                                   cast(u16, dds_header->height),
                                                                   cast(u16, dds_header->depth),
                                                                   mipmap_count,
                                                                   texture_format,
                                                                   resource_flags,
                                                                   name);
            } break;

            case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
            {
                if (dx10_header->array_size > 1)
                {
                    if (dds_header->caps.caps2 & CAPS2_CUBEMAP)
                    {
                        REV_CHECK(dx10_header->misc_flags & MISC_FLAG_TEXTURECUBE);
                        REV_CHECK_M(dds_header->caps.caps2 & CAPS2_CUBEMAP_ALLFACES, "Currently only DDS cube textures with ALL faces are allowed");

                        asset->texture = memory_manager->AllocateTextureCubeArray(cast(u16, dds_header->width),
                                                                                  cast(u16, dds_header->height),
                                                                                  cast(u16, dx10_header->array_size),
                                                                                  mipmap_count,
                                                                                  texture_format,
                                                                                  resource_flags,
                                                                                  name);
                    }
                    else
                    {
                        asset->texture = memory_manager->AllocateTexture2DArray(cast(u16, dds_header->width),
                                                                                cast(u16, dds_header->height),
                                                                                cast(u16, dx10_header->array_size),
                                                                                mipmap_count,
                                                                                texture_format,
                                                                                resource_flags,
                                                                                name);
                    }
                }
                else
                {
                    if (dds_header->caps.caps2 & CAPS2_CUBEMAP)
                    {
                        REV_CHECK(dx10_header->misc_flags & MISC_FLAG_TEXTURECUBE);
                        REV_CHECK_M(dds_header->caps.caps2 & CAPS2_CUBEMAP_ALLFACES, "Currently only DDS cube textures with ALL faces are allowed");

                        asset->texture = memory_manager->AllocateTextureCube(cast(u16, dds_header->width),
                                                                             cast(u16, dds_header->height),
                                                                             mipmap_count,
                                                                             texture_format,
                                                                             resource_flags,
                                                                             name);
                    }
                    else
                    {
                        asset->texture = memory_manager->AllocateTexture2D(cast(u16, dds_header->width),
                                                                           cast(u16, dds_header->height),
                                                                           mipmap_count,
                                                                           texture_format,
                                                                           resource_flags,
                                                                           name);
                    }
                }
            } break;

            case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
            {
                if (dx10_header->array_size > 1)
                {
                    asset->texture = memory_manager->AllocateTexture1DArray(cast(u16, dds_header->width),
                                                                            cast(u16, dx10_header->array_size),
                                                                            mipmap_count,
                                                                            texture_format,
                                                                            resource_flags,
                                                                            name);
                }
                else
                {
                    asset->texture = memory_manager->AllocateTexture1D(cast(u16, dds_header->width),
                                                                       mipmap_count,
                                                                       texture_format,
                                                                       resource_flags,
                                                                       name);
                }
            } break;

            default:
            {
                REV_ERROR_M("Incorrect dimension");
            } break;
        }
    }
    else
    {
        if (dds_header->flags & DDS_FLAG_DEPTH)
        {
            REV_CHECK(dds_header->caps.caps2 & CAPS2_VOLUME);

            asset->texture = memory_manager->AllocateTexture3D(cast(u16, dds_header->width),
                                                               cast(u16, dds_header->height),
                                                               cast(u16, dds_header->depth),
                                                               mipmap_count,
                                                               texture_format,
                                                               resource_flags,
                                                               name);
        }
        else if (dds_header->caps.caps2 & CAPS2_CUBEMAP)
        {
            REV_CHECK_M(dds_header->caps.caps2 & CAPS2_CUBEMAP_ALLFACES, "Currently only DDS cube textures with ALL faces are allowed");

            asset->texture = memory_manager->AllocateTextureCube(cast(u16, dds_header->width),
                                                                 cast(u16, dds_header->height),
                                                                 mipmap_count,
                                                                 texture_format,
                                                                 resource_flags,
                                                                 name);
        }
        else
        {
            asset->texture = memory_manager->AllocateTexture2D(cast(u16, dds_header->width),
                                                               cast(u16, dds_header->height),
                                                               mipmap_count,
                                                               texture_format,
                                                               resource_flags,
                                                               name);
        }
    }

    memory_manager->SetTextureData(asset->texture, texture_data);
}

}
#pragma pack(pop)
