//
// Copyright 2020-2021 Roman Skabin
//

#include "core/pch.h"
#include "asset_manager/asset_manager.h"
#include "graphics/graphics_api.h"
#include "core/settings.h"
#include "math/math.h"

// @TODO(Roman): #CrossPlatform
#include "platform/d3d12/d3d12_memory_manager.h"
#include <dxgiformat.h>
#include <d3d12.h>

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

REV_INTERNAL u64 BitsPerPixel(GPU::TEXTURE_FORMAT format)
{
    if (cast<bool>(format & GPU::TEXTURE_FORMAT::_DDS_DX10))
    {
        switch (cast<u32>(format & ~GPU::TEXTURE_FORMAT::_DDS_DX10))
        {
            /*   1 */ case DXGI_FORMAT_R32G32B32A32_TYPELESS:      return 128;
            /*   2 */ case DXGI_FORMAT_R32G32B32A32_FLOAT:         return 128;
            /*   3 */ case DXGI_FORMAT_R32G32B32A32_UINT:          return 128;
            /*   4 */ case DXGI_FORMAT_R32G32B32A32_SINT:          return 128;
            /*   5 */ case DXGI_FORMAT_R32G32B32_TYPELESS:         return 96;
            /*   6 */ case DXGI_FORMAT_R32G32B32_FLOAT:            return 96;
            /*   7 */ case DXGI_FORMAT_R32G32B32_UINT:             return 96;
            /*   8 */ case DXGI_FORMAT_R32G32B32_SINT:             return 96;
            /*   9 */ case DXGI_FORMAT_R16G16B16A16_TYPELESS:      return 64;
            /*  10 */ case DXGI_FORMAT_R16G16B16A16_FLOAT:         return 64;
            /*  11 */ case DXGI_FORMAT_R16G16B16A16_UNORM:         return 64;
            /*  12 */ case DXGI_FORMAT_R16G16B16A16_UINT:          return 64;
            /*  13 */ case DXGI_FORMAT_R16G16B16A16_SNORM:         return 64;
            /*  14 */ case DXGI_FORMAT_R16G16B16A16_SINT:          return 64;
            /*  15 */ case DXGI_FORMAT_R32G32_TYPELESS:            return 64;
            /*  16 */ case DXGI_FORMAT_R32G32_FLOAT:               return 64;
            /*  17 */ case DXGI_FORMAT_R32G32_UINT:                return 64;
            /*  18 */ case DXGI_FORMAT_R32G32_SINT:                return 64;
            /*  19 */ case DXGI_FORMAT_R32G8X24_TYPELESS:          return 64;
            /*  20 */ case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:       return 64;
            /*  21 */ case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:   return 64;
            /*  22 */ case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:    return 64;
            /*  23 */ case DXGI_FORMAT_R10G10B10A2_TYPELESS:       return 32;
            /*  24 */ case DXGI_FORMAT_R10G10B10A2_UNORM:          return 32;
            /*  25 */ case DXGI_FORMAT_R10G10B10A2_UINT:           return 32;
            /*  26 */ case DXGI_FORMAT_R11G11B10_FLOAT:            return 32;
            /*  27 */ case DXGI_FORMAT_R8G8B8A8_TYPELESS:          return 32;
            /*  28 */ case DXGI_FORMAT_R8G8B8A8_UNORM:             return 32;
            /*  29 */ case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:        return 32;
            /*  30 */ case DXGI_FORMAT_R8G8B8A8_UINT:              return 32;
            /*  31 */ case DXGI_FORMAT_R8G8B8A8_SNORM:             return 32;
            /*  32 */ case DXGI_FORMAT_R8G8B8A8_SINT:              return 32;
            /*  33 */ case DXGI_FORMAT_R16G16_TYPELESS:            return 32;
            /*  34 */ case DXGI_FORMAT_R16G16_FLOAT:               return 32;
            /*  35 */ case DXGI_FORMAT_R16G16_UNORM:               return 32;
            /*  36 */ case DXGI_FORMAT_R16G16_UINT:                return 32;
            /*  37 */ case DXGI_FORMAT_R16G16_SNORM:               return 32;
            /*  38 */ case DXGI_FORMAT_R16G16_SINT:                return 32;
            /*  39 */ case DXGI_FORMAT_R32_TYPELESS:               return 32;
            /*  40 */ case DXGI_FORMAT_D32_FLOAT:                  return 32;
            /*  41 */ case DXGI_FORMAT_R32_FLOAT:                  return 32;
            /*  42 */ case DXGI_FORMAT_R32_UINT:                   return 32;
            /*  43 */ case DXGI_FORMAT_R32_SINT:                   return 32;
            /*  44 */ case DXGI_FORMAT_R24G8_TYPELESS:             return 32;
            /*  45 */ case DXGI_FORMAT_D24_UNORM_S8_UINT:          return 32;
            /*  46 */ case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:      return 32;
            /*  47 */ case DXGI_FORMAT_X24_TYPELESS_G8_UINT:       return 32;
            /*  48 */ case DXGI_FORMAT_R8G8_TYPELESS:              return 16;
            /*  49 */ case DXGI_FORMAT_R8G8_UNORM:                 return 16;
            /*  50 */ case DXGI_FORMAT_R8G8_UINT:                  return 16;
            /*  51 */ case DXGI_FORMAT_R8G8_SNORM:                 return 16;
            /*  52 */ case DXGI_FORMAT_R8G8_SINT:                  return 16;
            /*  53 */ case DXGI_FORMAT_R16_TYPELESS:               return 16;
            /*  54 */ case DXGI_FORMAT_R16_FLOAT:                  return 16;
            /*  55 */ case DXGI_FORMAT_D16_UNORM:                  return 16;
            /*  56 */ case DXGI_FORMAT_R16_UNORM:                  return 16;
            /*  57 */ case DXGI_FORMAT_R16_UINT:                   return 16;
            /*  58 */ case DXGI_FORMAT_R16_SNORM:                  return 16;
            /*  59 */ case DXGI_FORMAT_R16_SINT:                   return 16;
            /*  60 */ case DXGI_FORMAT_R8_TYPELESS:                return 8;
            /*  61 */ case DXGI_FORMAT_R8_UNORM:                   return 8;
            /*  62 */ case DXGI_FORMAT_R8_UINT:                    return 8;
            /*  63 */ case DXGI_FORMAT_R8_SNORM:                   return 8;
            /*  64 */ case DXGI_FORMAT_R8_SINT:                    return 8;
            /*  65 */ case DXGI_FORMAT_A8_UNORM:                   return 8;
            /*  66 */ case DXGI_FORMAT_R1_UNORM:                   return 1;
            /*  67 */ case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:         return 32;
            /*  68 */ case DXGI_FORMAT_R8G8_B8G8_UNORM:            return 32;
            /*  69 */ case DXGI_FORMAT_G8R8_G8B8_UNORM:            return 32;
            /*  70 */ case DXGI_FORMAT_BC1_TYPELESS:               return 4;
            /*  71 */ case DXGI_FORMAT_BC1_UNORM:                  return 4;
            /*  72 */ case DXGI_FORMAT_BC1_UNORM_SRGB:             return 4;
            /*  73 */ case DXGI_FORMAT_BC2_TYPELESS:               return 8;
            /*  74 */ case DXGI_FORMAT_BC2_UNORM:                  return 8;
            /*  75 */ case DXGI_FORMAT_BC2_UNORM_SRGB:             return 8;
            /*  76 */ case DXGI_FORMAT_BC3_TYPELESS:               return 8;
            /*  77 */ case DXGI_FORMAT_BC3_UNORM:                  return 8;
            /*  78 */ case DXGI_FORMAT_BC3_UNORM_SRGB:             return 8;
            /*  79 */ case DXGI_FORMAT_BC4_TYPELESS:               return 4;
            /*  80 */ case DXGI_FORMAT_BC4_UNORM:                  return 4;
            /*  81 */ case DXGI_FORMAT_BC4_SNORM:                  return 4;
            /*  82 */ case DXGI_FORMAT_BC5_TYPELESS:               return 8;
            /*  83 */ case DXGI_FORMAT_BC5_UNORM:                  return 8;
            /*  84 */ case DXGI_FORMAT_BC5_SNORM:                  return 8;
            /*  85 */ case DXGI_FORMAT_B5G6R5_UNORM:               return 16;
            /*  86 */ case DXGI_FORMAT_B5G5R5A1_UNORM:             return 16;
            /*  87 */ case DXGI_FORMAT_B8G8R8A8_UNORM:             return 32;
            /*  88 */ case DXGI_FORMAT_B8G8R8X8_UNORM:             return 32;
            /*  89 */ case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM: return 32;
            /*  90 */ case DXGI_FORMAT_B8G8R8A8_TYPELESS:          return 32;
            /*  91 */ case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:        return 32;
            /*  92 */ case DXGI_FORMAT_B8G8R8X8_TYPELESS:          return 32;
            /*  93 */ case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:        return 32;
            /*  94 */ case DXGI_FORMAT_BC6H_TYPELESS:              return 8;
            /*  95 */ case DXGI_FORMAT_BC6H_UF16:                  return 8;
            /*  96 */ case DXGI_FORMAT_BC6H_SF16:                  return 8;
            /*  97 */ case DXGI_FORMAT_BC7_TYPELESS:               return 8;
            /*  98 */ case DXGI_FORMAT_BC7_UNORM:                  return 8;
            /*  99 */ case DXGI_FORMAT_BC7_UNORM_SRGB:             return 8;
            /* 100 */ case DXGI_FORMAT_AYUV:                       return 32;
            /* 101 */ case DXGI_FORMAT_Y410:                       return 32;
            /* 102 */ case DXGI_FORMAT_Y416:                       return 64;
            /* 103 */ case DXGI_FORMAT_NV12:                       return 12;
            /* 104 */ case DXGI_FORMAT_P010:                       return 24;
            /* 105 */ case DXGI_FORMAT_P016:                       return 24;
            /* 106 */ case DXGI_FORMAT_420_OPAQUE:                 return 12;
            /* 107 */ case DXGI_FORMAT_YUY2:                       return 32;
            /* 108 */ case DXGI_FORMAT_Y210:                       return 64;
            /* 109 */ case DXGI_FORMAT_Y216:                       return 64;
            /* 110 */ case DXGI_FORMAT_NV11:                       return 12;
            /* 111 */ case DXGI_FORMAT_AI44:                       return 8;
            /* 112 */ case DXGI_FORMAT_IA44:                       return 8;
            /* 113 */ case DXGI_FORMAT_P8:                         return 8;
            /* 114 */ case DXGI_FORMAT_A8P8:                       return 16;
            /* 115 */ case DXGI_FORMAT_B4G4R4A4_UNORM:             return 16;
            /* 130 */ case DXGI_FORMAT_P208:                       return 16;
            /* 131 */ case DXGI_FORMAT_V208:                       return 16;
            /* 132 */ case DXGI_FORMAT_V408:                       return 24;
            default:                                               return 0;
        }
    }
    else
    {
        switch (format)
        {
            /* 1 */ case GPU::TEXTURE_FORMAT::RGBA8: return 32;
            /* 2 */ case GPU::TEXTURE_FORMAT::BGRA8: return 32;
            /* 3 */ case GPU::TEXTURE_FORMAT::DXT1:  return 4;
            /* 4 */ case GPU::TEXTURE_FORMAT::DXT2:  return 8;
            /* 5 */ case GPU::TEXTURE_FORMAT::DXT3:  return 8;
            /* 6 */ case GPU::TEXTURE_FORMAT::DXT4:  return 4;
            /* 7 */ case GPU::TEXTURE_FORMAT::DXT5:  return 8;
            default:                                 return 0;
        }
    }
}

void AssetManager::CreateDDSTexture(Asset *asset, byte *data, u64 data_size)
{
    GPU::MemoryManager *memory_manager = GraphicsAPI::GetMemoryManager();

    u32 magic = *cast<u32 *>(data);
    REV_CHECK_M(magic == DDS_MAGIC, "This is not a DDS file");

    DDSHeader *dds_header = cast<DDSHeader *>(data + sizeof(u32));
    REV_CHECK_M(dds_header->self_size == sizeof(DDSHeader) && dds_header->pixel_format.self_size == sizeof(PixelFormat), "Invalid DDS file layout");

    DX10Header          *dx10_header    = null;
    u32                  data_offset    = sizeof(u32) + sizeof(DDSHeader);
    GPU::TEXTURE_FORMAT  texture_format = GPU::TEXTURE_FORMAT::UNKNOWN;

    u32  compression_ratio = 1;
    bool block_compression = false;
    bool packed            = false; // @TODO(Roman): #OtherFormats
    bool planar            = false; // @TODO(Roman): #OtherFormats

    if (dds_header->pixel_format.flags & PIXEL_FORMAT_FLAG_FOURCC)
    {
        switch (dds_header->pixel_format.fourcc._enum)
        {
            case FOURCC_DXT1:
            {
                texture_format    = GPU::TEXTURE_FORMAT::DXT1;
                compression_ratio = 8;
                block_compression = true;
            } break;

            case FOURCC_DXT2:
            {
                texture_format    = GPU::TEXTURE_FORMAT::DXT2;
                compression_ratio = 4;
                block_compression = true;
            } break;

            case FOURCC_DXT3:
            {
                texture_format    = GPU::TEXTURE_FORMAT::DXT3;
                compression_ratio = 4;
                block_compression = true;
            } break;

            case FOURCC_DXT4:
            {
                texture_format    = GPU::TEXTURE_FORMAT::DXT4;
                compression_ratio = 4;
                block_compression = true;
            } break;

            case FOURCC_DXT5:
            {
                texture_format    = GPU::TEXTURE_FORMAT::DXT5;
                compression_ratio = 4;
                block_compression = true;
            } break;

            case FOURCC_DX10:
            {
                dx10_header     = cast<DX10Header *>(data + sizeof(u32) + sizeof(DDSHeader));
                data_offset    += sizeof(DX10Header);
                texture_format  = cast<GPU::TEXTURE_FORMAT>(dx10_header->format) | GPU::TEXTURE_FORMAT::_DDS_DX10;
            } break;

            default:
            {
                // @TODO(Roman): #OtherFormats
                REV_FAILED_M("Unhandled DDS texture format: %.4s", dds_header->pixel_format.fourcc.str);
            } break;
        };
    }
    else
    {
        // @TODO(Roman): #OtherFormats: BC4, BC5, Uncompressed RGB(A), YCrCb
        REV_FAILED_M("Unhandled DDS texture format");
    }

    if (dx10_header)
    {
        if (dx10_header->dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
        {
            REV_CHECK(dds_header->caps.caps2 & CAPS2_VOLUME);
            REV_CHECK(dx10_header->array_size == 1);
            asset->resource = memory_manager->AllocateTexture3D(cast<u16>(dds_header->width),
                                                                cast<u16>(dds_header->height),
                                                                cast<u16>(dds_header->depth),
                                                                cast<u16>(dds_header->mipmap_count),
                                                                texture_format,
                                                                "DDSTexutre3D");
        }
        else if (dx10_header->dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
        {
            if (dx10_header->array_size > 1)
            {
                if (dds_header->caps.caps2 & CAPS2_CUBEMAP)
                {
                    REV_CHECK(dx10_header->misc_flags & MISC_FLAG_TEXTURECUBE);
                    REV_CHECK_M(dds_header->caps.caps2 & CAPS2_CUBEMAP_ALLFACES, "Currently only DDS cube textures with ALL faces are allowed");
                    asset->resource = memory_manager->AllocateTextureCubeArray(cast<u16>(dds_header->width),
                                                                               cast<u16>(dds_header->height),
                                                                               cast<u16>(dx10_header->array_size),
                                                                               cast<u16>(dds_header->mipmap_count),
                                                                               texture_format,
                                                                               "DDSTexutreCubeArray");
                }
                else
                {
                    asset->resource = memory_manager->AllocateTexture2DArray(cast<u16>(dds_header->width),
                                                                             cast<u16>(dds_header->height),
                                                                             cast<u16>(dx10_header->array_size),
                                                                             cast<u16>(dds_header->mipmap_count),
                                                                             texture_format,
                                                                             "DDSTexutre2DArray");
                }
            }
            else
            {
                if (dds_header->caps.caps2 & CAPS2_CUBEMAP)
                {
                    REV_CHECK(dx10_header->misc_flags & MISC_FLAG_TEXTURECUBE);
                    REV_CHECK_M(dds_header->caps.caps2 & CAPS2_CUBEMAP_ALLFACES, "Currently only DDS cube textures with ALL faces are allowed");
                    asset->resource = memory_manager->AllocateTextureCube(cast<u16>(dds_header->width),
                                                                          cast<u16>(dds_header->height),
                                                                          cast<u16>(dds_header->mipmap_count),
                                                                          texture_format,
                                                                          "DDSTexutreCube");
                }
                else
                {
                    asset->resource = memory_manager->AllocateTexture2D(cast<u16>(dds_header->width),
                                                                        cast<u16>(dds_header->height),
                                                                        cast<u16>(dds_header->mipmap_count),
                                                                        texture_format,
                                                                        "DDSTexutre2D");
                }
            }
        }
        else if (dx10_header->dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D)
        {
            asset->resource = memory_manager->AllocateTexture1D(cast<u16>(dds_header->width),
                                                                texture_format,
                                                                "DDSTexutre1D");
        }
        else
        {
            REV_FAILED_M("Incorrect dimension");
        }
    }
    else
    {
        if (dds_header->flags & DDS_FLAG_DEPTH)
        {
            REV_CHECK(dds_header->caps.caps2 & CAPS2_VOLUME);
            asset->resource = memory_manager->AllocateTexture3D(cast<u16>(dds_header->width),
                                                                cast<u16>(dds_header->height),
                                                                cast<u16>(dds_header->depth),
                                                                cast<u16>(dds_header->mipmap_count),
                                                                texture_format,
                                                                "DDSTexutre3D");
        }
        else if (dds_header->caps.caps2 & CAPS2_CUBEMAP)
        {
            REV_CHECK_M(dds_header->caps.caps2 & CAPS2_CUBEMAP_ALLFACES, "Currently only DDS cube textures with ALL faces are allowed");
            asset->resource = memory_manager->AllocateTextureCube(cast<u16>(dds_header->width),
                                                                  cast<u16>(dds_header->height),
                                                                  cast<u16>(dds_header->mipmap_count),
                                                                  texture_format,
                                                                  "DDSTexutreCube");
        }
        else
        {
            REV_CHECK(dds_header->width && dds_header->height);
            asset->resource = memory_manager->AllocateTexture2D(cast<u16>(dds_header->width),
                                                                cast<u16>(dds_header->height),
                                                                cast<u16>(dds_header->mipmap_count),
                                                                texture_format,
                                                                "DDSTexutre2D");
        }
    }

    GPU::TextureDesc texture_desc;

    // @NOTE(Roman): compressed:   DDS_FLAG_LINEARSIZE, linear_size = number of bytes for the main image.
    //               uncompressed: DDS_FLAG_PITCH,      pitch       = number of bytes per row.
    if (dds_header->flags & DDS_FLAG_LINEARSIZE)
    {
        texture_desc.subtexture_desc   = Memory::Get()->PushToTA<GPU::SubTextureDesc>(dds_header->mipmap_count);
        texture_desc.subtextures_count = dds_header->mipmap_count;

        byte *mipmap_level_data = data + data_offset;

        u64 width  = dds_header->width;
        u64 height = dds_header->height;

        for (u64 i = 0; i < dds_header->mipmap_count; ++i)
        {
            s64 row_bytes   = 0;
            s64 slice_bytes = 0;

            if (block_compression)
            {
                row_bytes   = Math::max(1ui64, (width  + 3) / 4) * compression_ratio;
                slice_bytes = Math::max(1ui64, (height + 3) / 4) * row_bytes;
            }
            else if (packed)
            {
                row_bytes   = ((width + 1) >> 1) * compression_ratio;
                slice_bytes = row_bytes * height;
            }
            else if (planar)
            {
                row_bytes   = ((width + 1) >> 1) * compression_ratio;
                slice_bytes = (3 * row_bytes * height + 1) >> 1;
            }
            else
            {
                row_bytes   = (width * BitsPerPixel(texture_format) + 7) / 8;
                slice_bytes = row_bytes * height;
            }

            texture_desc.subtexture_desc[i].data            = mipmap_level_data;
            texture_desc.subtexture_desc[i].bytes_per_row   = row_bytes;
            texture_desc.subtexture_desc[i].bytes_per_tex2d = slice_bytes;

            mipmap_level_data += slice_bytes;

            width  >>= 1;
            height >>= 1;
        }
    }
    else if (dds_header->flags & DDS_FLAG_PITCH)
    {
        REV_FAILED_M("Only BC1-BC5 foramts are supported. Any format supported only with DDS_HEADER_DXT10.");
    }

    memory_manager->SetTextureData(asset->resource, &texture_desc);

    // @TODO(Roman): #CrossPlatform, #Hardcoded.
    if (GraphicsAPI::GetAPI() == GraphicsAPI::API::D3D12)
    {
        D3D12::MemoryManager *d3d12_memory_manager = cast<D3D12::MemoryManager *>(memory_manager);
        Settings             *settings             = Settings::Get();

        D3D12_FILTER d3d12_filter;
        switch (settings->filtering)
        {
            case FILTERING::POINT:       d3d12_filter = D3D12_FILTER_MIN_MAG_MIP_POINT;        break;
            case FILTERING::BILINEAR:    d3d12_filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT; break;
            case FILTERING::TRILINEAR:   d3d12_filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;       break;
            case FILTERING::ANISOTROPIC: d3d12_filter = D3D12_FILTER_ANISOTROPIC;              break;
        }

        D3D12_SAMPLER_DESC d3d12_sampler_desc{};
        d3d12_sampler_desc.Filter         = d3d12_filter;
        d3d12_sampler_desc.AddressU       = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        d3d12_sampler_desc.AddressV       = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        d3d12_sampler_desc.AddressW       = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        d3d12_sampler_desc.MipLODBias     = 0.0f;
        d3d12_sampler_desc.MaxAnisotropy  = settings->anisotropy;
        d3d12_sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        d3d12_sampler_desc.MinLOD         = 0.0f;
        d3d12_sampler_desc.MaxLOD         = 100.0f;

        asset->sampler = d3d12_memory_manager->AllocateSampler(d3d12_sampler_desc);
    }
    else
    {
        REV_FAILED_M("Not implemented yet");
    }
}

}
#pragma pack(pop)
