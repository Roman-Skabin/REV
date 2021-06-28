//
// Copyright 2020-2021 Roman Skabin
//

#include "core/pch.h"
#include "core/settings.h"
#include "core/memory.h"

namespace REV
{

//
// Token
//

enum TOKEN_KIND : u32
{
    TOKEN_KIND_EOS          = '\0',
    TOKEN_KIND_UNKNOWN      = TOKEN_KIND_EOS,

    TOKEN_KIND_ASSIGN       = '=', // 61

    TOKEN_KIND_LAST_LITERAL = 0xFF,

    TOKEN_KIND_NAME,
    TOKEN_KIND_INT,

    TOKEN_KIND_MAX
};

struct Token
{
    TOKEN_KIND  kind  = TOKEN_KIND_UNKNOWN;
    Math::v2u   pos   = Math::v2u(1, 0);
    const char *start = null;
    const char *end   = null;
    union
    {
        u32 u32_val;
        s32 s32_val;
    };
};

//
// Lexer
//

struct Lexer
{
    const char *filename;
    const char *stream;
    const char *line_start;
    Token       token;
};

REV_INTERNAL void REV_CDECL SyntaxError(Lexer *lexer, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    
    char buffer[1024];
    vsprintf(buffer, format, args);
    
    va_end(args);

    lexer->token.pos.c = cast<u32>(lexer->stream - lexer->line_start);

    DebugFC(DEBUG_COLOR::ERROR, "%s(%I32u:%I32u): INI syntax error: %s.", lexer->filename, lexer->token.pos.r, lexer->token.pos.c, buffer);
    // @TODO(Roman): #CrossPlatform
    ExitProcess(1);
}

REV_INTERNAL void NextToken(Lexer *lexer)
{
    #define ELSE_IF_CHAR(c1, k1) else if (*lexer->stream == (c1)) { lexer->token.kind = (k1); ++lexer->stream; }

tokenize_again:
    while (isspace(*lexer->stream))
    {
        if (*lexer->stream++ == '\n')
        {
            ++lexer->token.pos.r;
            lexer->line_start = lexer->stream;
        }
    }

    lexer->token.start = lexer->stream;

    if (isalpha(*lexer->stream))
    {
        while (isalnum(*lexer->stream) || *lexer->stream == '_') ++lexer->stream;
        lexer->token.kind = TOKEN_KIND_NAME;
    }
    else if (isdigit(*lexer->stream))
    {
        lexer->token.u32_val = *lexer->stream++ - '0';
        while (isdigit(*lexer->stream))
        {
            lexer->token.u32_val *= 10;
            lexer->token.u32_val += *lexer->stream++ - '0';
        }
        lexer->token.kind = TOKEN_KIND_INT;
    }
    else if (*lexer->stream == '-' && isdigit(lexer->stream[1]))
    {
        ++lexer->stream;
        lexer->token.s32_val = *lexer->stream++ - '0';
        while (isdigit(*lexer->stream))
        {
            lexer->token.s32_val *= 10;
            lexer->token.s32_val += *lexer->stream++ - '0';
        }
        lexer->token.s32_val |= REV_S32_MIN;
        lexer->token.kind     = TOKEN_KIND_INT;
    }
    ELSE_IF_CHAR('\0', TOKEN_KIND_EOS)
    ELSE_IF_CHAR('=', TOKEN_KIND_ASSIGN)
    else if (*lexer->stream == '#' || *lexer->stream == ';')
    {
        while (*lexer->stream && *lexer->stream != '\n') ++lexer->stream;
        ++lexer->stream;
        ++lexer->token.pos.r;
        lexer->line_start = lexer->stream;
        goto tokenize_again;
    }
    else
    {
        SyntaxError(lexer, "undefined token: %.*s", cast<int>(lexer->stream - lexer->token.start), lexer->token.start);
    }

    lexer->token.end   = lexer->stream;
    lexer->token.pos.c = cast<u32>(lexer->token.start - lexer->line_start + 1);

    #undef ELSE_IF_CHAR
}

REV_INTERNAL REV_INLINE bool TokenEquals(Lexer *lexer, const char *str, u64 length)
{
    return lexer->token.end - lexer->token.start == length
        && !memcmp(lexer->token.start, str, length);
}

REV_INTERNAL void CheckToken(Lexer *lexer, TOKEN_KIND token_kind)
{
    if (lexer->token.kind != token_kind)
    {
        const char *token_kind_names[TOKEN_KIND_MAX] = {'\0'};
        token_kind_names[TOKEN_KIND_EOS]       = "<EOS>";
        token_kind_names[TOKEN_KIND_ASSIGN]    = "=";

        auto& TokenKindName = [&token_kind_names](TOKEN_KIND token_kind) -> const char *
        {
            if (token_kind < ArrayCount(token_kind_names))
            {
                const char *name = token_kind_names[token_kind];
                return name ? name : "<unknown>";
            }
            return "<unknown>";
        };

        SyntaxError(lexer, "expected '%s', got '%s'", TokenKindName(token_kind), TokenKindName(lexer->token.kind));
    }
}

REV_INTERNAL REV_INLINE bool IsToken(Lexer *lexer, TOKEN_KIND token_kind)
{
    return lexer->token.kind == token_kind;
}

// @TODO(Roman): #Tools, #MakePublic.
REV_INTERNAL byte *TryReadEntireFileToTA(const char *filename, u64& data_size)
{
    // @TODO(Roman): #CrossPlatform
    HANDLE file = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, null, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY | FILE_FLAG_SEQUENTIAL_SCAN, null);
    if (file != INVALID_HANDLE_VALUE)
    {
        data_size = 0;
        REV_DEBUG_RESULT(GetFileSizeEx(file, cast<LARGE_INTEGER *>(&data_size)));

        byte *data = Memory::Get()->PushToTA<byte>(data_size + 1);
        for (u64 bytes_read = 0, size = data_size; size; )
        {
            u32 bytes_to_read = Math::clamp<u32, u64>(size, 0, REV_U32_MAX);
            REV_DEBUG_RESULT(ReadFile(file, data + bytes_read, bytes_to_read, null, null));

            bytes_read += bytes_to_read;
            size       -= bytes_to_read;
        }
        data[data_size] = '\0';

        REV_DEBUG_RESULT(CloseHandle(file));
        return data;
    }
    return null;
}

REV_INTERNAL void WriteDefaultSettingsToFile(const char *filename)
{
    // @TODO(Roman): #CrossPlatform.
    HANDLE file = CreateFileA(filename, GENERIC_WRITE, FILE_SHARE_WRITE, null, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, null);
    REV_CHECK(file != INVALID_HANDLE_VALUE);

    char default_settings[] =
    "window_x             = 10\n"
    "window_y             = 10\n"
    "window_width         = 960\n"
    "window_height        = 540\n"
    "fullscreen           = false\n"
    "render_target_width  = 1920\n"
    "render_target_height = 1080\n"
    "graphics_api         = D3D12\n"
    "filtering            = TRILINEAR\n"
    "anisotropy           = 1\n"
    "vsync                = false\n";

    REV_DEBUG_RESULT(WriteFile(file, default_settings, REV_CSTRLEN(default_settings), null, null));
    REV_DEBUG_RESULT(CloseHandle(file));
}

REV_GLOBAL Settings *g_Settings = null;

Settings *Settings::Init(const char *ini_filename)
{
    REV_CHECK_M(!g_Settings, "Settings is already created. Use Settings::Get() function instead");
    g_Settings = Memory::Get()->PushToPA<Settings>();
    g_Settings->window_xywh      = Math::v4s(10, 10, 960, 540);
    g_Settings->render_target_wh = Math::v2s(1920, 1080);
    g_Settings->graphics_api     = GraphicsAPI::API::D3D12;
    g_Settings->filtering        = FILTERING::TRILINEAR;
    g_Settings->anisotropy       = 1;
    g_Settings->fullscreen       = false;
    g_Settings->vsync            = false;

    u64         stream_length = 0;
    const char *stream        = cast<const char *>(TryReadEntireFileToTA(ini_filename, stream_length));

    if (stream)
    {
        Lexer lexer;
        lexer.filename   = ini_filename;
        lexer.stream     = stream;
        lexer.line_start = stream;
        NextToken(&lexer);

        for (; !IsToken(&lexer, TOKEN_KIND_EOS); NextToken(&lexer))
        {
            if (IsToken(&lexer, TOKEN_KIND_NAME))
            {
                if (TokenEquals(&lexer, "window_x", REV_CSTRLEN("window_x")))
                {
                    NextToken(&lexer);
                    CheckToken(&lexer, TOKEN_KIND_ASSIGN);

                    NextToken(&lexer);
                    CheckToken(&lexer, TOKEN_KIND_INT);

                    g_Settings->window_xywh.x = lexer.token.s32_val;
                }
                else if (TokenEquals(&lexer, "window_y", REV_CSTRLEN("window_y")))
                {
                    NextToken(&lexer);
                    CheckToken(&lexer, TOKEN_KIND_ASSIGN);

                    NextToken(&lexer);
                    CheckToken(&lexer, TOKEN_KIND_INT);

                    g_Settings->window_xywh.y = lexer.token.s32_val;
                }
                else if (TokenEquals(&lexer, "window_width", REV_CSTRLEN("window_width")))
                {
                    NextToken(&lexer);
                    CheckToken(&lexer, TOKEN_KIND_ASSIGN);

                    NextToken(&lexer);
                    CheckToken(&lexer, TOKEN_KIND_INT);

                    g_Settings->window_xywh.wh.w = lexer.token.s32_val;
                }
                else if (TokenEquals(&lexer, "window_height", REV_CSTRLEN("window_height")))
                {
                    NextToken(&lexer);
                    CheckToken(&lexer, TOKEN_KIND_ASSIGN);

                    NextToken(&lexer);
                    CheckToken(&lexer, TOKEN_KIND_INT);

                    g_Settings->window_xywh.wh.h = lexer.token.s32_val;
                }
                else if (TokenEquals(&lexer, "fullscreen", REV_CSTRLEN("fullscreen")))
                {
                    NextToken(&lexer);
                    CheckToken(&lexer, TOKEN_KIND_ASSIGN);

                    NextToken(&lexer);
                    CheckToken(&lexer, TOKEN_KIND_NAME);

                    if (TokenEquals(&lexer, "true",  REV_CSTRLEN("true")))
                    {
                        g_Settings->fullscreen = true;
                    }
                    // NOTE(Roman): false is default
                }
                else if (TokenEquals(&lexer, "render_target_width", REV_CSTRLEN("render_target_width")))
                {
                    NextToken(&lexer);
                    CheckToken(&lexer, TOKEN_KIND_ASSIGN);

                    NextToken(&lexer);
                    CheckToken(&lexer, TOKEN_KIND_INT);

                    g_Settings->render_target_wh.w = lexer.token.s32_val;
                }
                else if (TokenEquals(&lexer, "render_target_height", REV_CSTRLEN("render_target_height")))
                {
                    NextToken(&lexer);
                    CheckToken(&lexer, TOKEN_KIND_ASSIGN);

                    NextToken(&lexer);
                    CheckToken(&lexer, TOKEN_KIND_INT);

                    g_Settings->render_target_wh.h = lexer.token.s32_val;
                }
                else if (TokenEquals(&lexer, "graphics_api", REV_CSTRLEN("graphics_api")))
                {
                    NextToken(&lexer);
                    CheckToken(&lexer, TOKEN_KIND_ASSIGN);

                    NextToken(&lexer);
                    CheckToken(&lexer, TOKEN_KIND_NAME);

                    // NOTE(Roman): D3D12 is default
                    if (TokenEquals(&lexer, "Vulkan", REV_CSTRLEN("Vulkan")))
                    {
                        g_Settings->graphics_api = GraphicsAPI::API::VULKAN;
                    }
                    // @TODO(Roman): More Graphics APIs.
                }
                else if (TokenEquals(&lexer, "filtering", REV_CSTRLEN("filtering")))
                {
                    NextToken(&lexer);
                    CheckToken(&lexer, TOKEN_KIND_ASSIGN);

                    NextToken(&lexer);
                    CheckToken(&lexer, TOKEN_KIND_NAME);

                    if (TokenEquals(&lexer, "POINT", REV_CSTRLEN("POINT")))
                    {
                        g_Settings->filtering = FILTERING::POINT;
                    }
                    else if (TokenEquals(&lexer, "BILINEAR", REV_CSTRLEN("BILINEAR")))
                    {
                        g_Settings->filtering = FILTERING::BILINEAR;
                    }
                    // @NOTE(Roman): TRILINEAR is default
                    else if (TokenEquals(&lexer, "ANISOTROPIC", REV_CSTRLEN("ANISOTROPIC")))
                    {
                        g_Settings->filtering = FILTERING::ANISOTROPIC;
                    }
                }
                else if (TokenEquals(&lexer, "anisotropy", REV_CSTRLEN("anisotropy")))
                {
                    NextToken(&lexer);
                    CheckToken(&lexer, TOKEN_KIND_ASSIGN);

                    NextToken(&lexer);
                    CheckToken(&lexer, TOKEN_KIND_INT);

                    g_Settings->anisotropy = cast<u8>(lexer.token.u32_val);
                }
                else if (TokenEquals(&lexer, "vsync", REV_CSTRLEN("vsync")))
                {
                    NextToken(&lexer);
                    CheckToken(&lexer, TOKEN_KIND_ASSIGN);

                    NextToken(&lexer);
                    CheckToken(&lexer, TOKEN_KIND_NAME);

                    if (TokenEquals(&lexer, "true",  REV_CSTRLEN("true")))
                    {
                        g_Settings->vsync = true;
                    }
                    // NOTE(Roman): false is default
                }
                else
                {
                    SyntaxError(&lexer, "unexpected setting: %.*s", cast<int>(lexer.stream - lexer.token.start), lexer.token.start);
                }
            }
        }
    }
    else
    {
        WriteDefaultSettingsToFile(ini_filename);
    }

    return g_Settings;
}

Settings *Settings::Get()
{
    REV_CHECK_M(g_Settings, "Settings are not initialized yet");
    return g_Settings;
}

}
