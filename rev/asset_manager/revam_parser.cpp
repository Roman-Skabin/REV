//
// Copyright 2020-2021 Roman Skabin
//

#include "core/pch.h"
#include "asset_manager/asset_manager.h"
#include "math/vec.h"
#include "core/memory.h"

namespace REV
{

//
// Token
//

enum class TOKEN_KIND : u32
{
    EOS,
    UNKNOWN      = EOS,

    COMMA        = ',', // 44
    COLON        = ':', // 58
    SEMICOLON    = ';', // 59

    LAST_LITERAL = 0xFF,

    KEYWORD,
    NAME,
    STRING,
};

struct Token
{
    TOKEN_KIND  kind  = TOKEN_KIND::UNKNOWN;
    Math::v2u   pos   = Math::v2u(1, 0);
    const char *start = null;
    const char *end   = null;
    const char *name  = null;
    // @TODO(Roman): Interns or hash table or mix or nothing?
    // const char *str_val = null;
};

//
// Keyword
//

enum KEYWORD_ID : u32
{
    KEYWORD_ID_STATIC   = 0,
    KEYWORD_ID_SCENE    = 1,
    KEYWORD_ID_TEXTURES = 2,
    KEYWORD_ID_SFX      = 3,
    KEYWORD_ID_MUSIC    = 4,

    KEYWORD_ID_UNKNOWN  = REV_U32_MAX
};

struct Keyword
{
    const char *name;
    u64         name_len;
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
    Keyword     keywords[5];

    Lexer(const char *filename, const char *stream);

    void NextToken();
    void REV_CDECL SyntaxError(const char *format, ...);

    bool IsKeyword(const char *str, u64 length);

    const char *GetKeywordName(const char *str, u64 length);
    KEYWORD_ID GetKeywordID();

    bool TokenEquals(const char *str, u64 length);
    bool TokenEqualsKeyword(KEYWORD_ID id);
    
    void CheckToken(TOKEN_KIND token_kind);
};

Lexer::Lexer(const char *filename, const char *stream)
    : filename(filename),
      stream(stream),
      line_start(stream),
      token(),
      keywords{{ "static",   REV_CSTRLEN("static")   },
               { "scene",    REV_CSTRLEN("scene")    },
               { "textures", REV_CSTRLEN("textures") },
               { "sfx",      REV_CSTRLEN("sfx")      },
               { "music",    REV_CSTRLEN("music")    }}
{
    NextToken();
}

void Lexer::NextToken()
{
    #define ELSE_IF_CHAR(c1, k1) else if (*stream == (c1)) { token.kind = (k1); ++stream; }

tokenize_again:
    while (isspace(*stream))
    {
        if (*stream++ == '\n')
        {
            ++token.pos.r;
            line_start = stream;
        }
    }

    token.start = stream;

    if (isalpha(*stream))
    {
        while (isalnum(*stream) || *stream == '_') ++stream;
        token.name = GetKeywordName(token.start, stream - token.start);
        token.kind = token.name ? TOKEN_KIND::KEYWORD : TOKEN_KIND::NAME;
    }
    ELSE_IF_CHAR('\0', TOKEN_KIND::EOS)
    ELSE_IF_CHAR(',', TOKEN_KIND::COMMA)
    ELSE_IF_CHAR(':', TOKEN_KIND::COLON)
    ELSE_IF_CHAR(';', TOKEN_KIND::SEMICOLON)
    else if (*stream == '"')
    {
        ++stream;
        while (*stream && *stream != '"')
        {
            if (*stream == '\\' && stream[1] == '"') ++stream;
            ++stream;
        }
        if (*stream)
        {
            if (*stream != '"') SyntaxError("expected '\"', got '%c'", *stream);
            ++stream;
        }
        else
        {
            SyntaxError("unexpected end of file");
        }
        token.kind = TOKEN_KIND::STRING;
    }
    else if (*stream == '/' && stream[1] == '/')
    {
        // @NOTE: No back slashes in the end of one-line comment
        while (*stream && *stream != '\n') ++stream;
        ++stream;
        ++token.pos.r;
        line_start = stream;
        goto tokenize_again;
    }
    else if (*stream == '/' && stream[1] == '*')
    {
        while (*stream && !(*stream == '*' && stream[1] == '/'))
        {
            if (*stream++ == '\n')
            {
                ++token.pos.r;
                line_start = stream;
            }
        }
        stream += 2;
        goto tokenize_again;
    }
    else
    {
        SyntaxError("undefined token");
    }

    token.end   = stream;
    token.pos.c = cast<u32>(token.start - line_start + 1);

    #undef ELSE_IF_CHAR
}

void REV_CDECL Lexer::SyntaxError(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    
    char buffer[1024];
    vsprintf(buffer, format, args);
    
    va_end(args);

    token.pos.c = cast<u32>(stream - line_start);

    DebugFC(DEBUG_COLOR::ERROR, "%s(%I32u:%I32u): RAF syntax error: %s.", filename, token.pos.r, token.pos.c, buffer);
    ExitProcess(1);
}

bool Lexer::IsKeyword(const char *str, u64 length)
{
    Keyword *last = keywords + ArrayCount(keywords) - 1;
    for (Keyword *it = keywords; it <= last; ++it)
    {
        if (it->name_len == length && !memcmp(it->name, str, length))
        {
            return true;
        }
    }
    return false;
}

const char *Lexer::GetKeywordName(const char *str, u64 length)
{
    Keyword *last = keywords + ArrayCount(keywords) - 1;
    for (Keyword *it = keywords; it <= last; ++it)
    {
        if (it->name_len == length && !memcmp(it->name, str, length))
        {
            return it->name;
        }
    }
    return null;
}

bool Lexer::TokenEquals(const char *str, u64 length)
{
    return token.end - token.start == length
        && !memcmp(token.start, str, length);
}

bool Lexer::TokenEqualsKeyword(KEYWORD_ID id)
{
    return token.name == keywords[id].name;
}

KEYWORD_ID Lexer::GetKeywordID()
{
    for (u32 i = 0; i < ArrayCount(keywords); ++i)
    {
        if (token.name == keywords[i].name)
        {
            return cast<KEYWORD_ID>(i);
        }
    }
    return KEYWORD_ID_UNKNOWN;
}

void Lexer::CheckToken(TOKEN_KIND token_kind)
{
    if (token.kind != token_kind)
    {
        #if 0 // Argh C++...
            const char *token_kind_names[256] =
            {
                [TOKEN_KIND::EOS]       = "EOS",
                [TOKEN_KIND::COMMA]     = ",",
                [TOKEN_KIND::COLON]     = ":",
                [TOKEN_KIND::SEMICOLON] = ";",
                [TOKEN_KIND::KEYWORD]   = "<keyword>",
                [TOKEN_KIND::STRING]    = "<string>",
            };
        #else
            const char *token_kind_names[256] = {'\0'};
            token_kind_names[cast<u64>(TOKEN_KIND::EOS)]       = "EOS";
            token_kind_names[cast<u64>(TOKEN_KIND::COMMA)]     = ",";
            token_kind_names[cast<u64>(TOKEN_KIND::COLON)]     = ":";
            token_kind_names[cast<u64>(TOKEN_KIND::SEMICOLON)] = ";";
            token_kind_names[cast<u64>(TOKEN_KIND::KEYWORD)]   = "<keyword>";
            token_kind_names[cast<u64>(TOKEN_KIND::STRING)]    = "<string>";
        #endif

        auto& TokenKindName = [&token_kind_names](TOKEN_KIND token_kind)
        {
            if (cast<u64>(token_kind) < ArrayCount(token_kind_names))
            {
                const char *name = token_kind_names[cast<u64>(token_kind)];
                return name ? name : "<unknown>";
            }
            return "<unknown>";
        };

        auto& TokenInfo = [&TokenKindName](Token *token)
        {
            return token->kind == TOKEN_KIND::KEYWORD
                 ? token->name
                 : TokenKindName(token->kind);
        };

        SyntaxError("expected '%s', got '%s'", TokenKindName(token_kind), TokenInfo(&token));
    }
}

void AssetManager::ParseExternalBlock(void *_lexer, Array<Asset> *area)
{
    Lexer *lexer = cast<Lexer *>(_lexer);
    lexer->CheckToken(TOKEN_KIND::COLON);

    lexer->NextToken();
    lexer->CheckToken(TOKEN_KIND::KEYWORD);

    while (true)
    {
        if (lexer->TokenEqualsKeyword(KEYWORD_ID_TEXTURES))
        {
            lexer->NextToken();
            lexer->CheckToken(TOKEN_KIND::COLON);

            lexer->NextToken();
            lexer->CheckToken(TOKEN_KIND::STRING);

            do
            {
                Asset *asset = area->PushBack();

                // @NOTE(Roman): filename
                {
                    const char *filename     = lexer->token.start + 1;
                    u64         filename_len = lexer->token.end - lexer->token.start - 2;

                    ParseTexture(asset, filename, filename_len);
                }

                lexer->NextToken();
                lexer->CheckToken(TOKEN_KIND::COLON);

                lexer->NextToken();
                lexer->CheckToken(TOKEN_KIND::NAME);

                // @NOTE(Roman): names
                {
                    u64 names_count            = 0;
                    u64 names_data_bytes_total = 0;

                    Lexer lexer_save = *lexer;
                    {
                        names_count            = 1;
                        names_data_bytes_total = lexer->token.end - lexer->token.start;

                        lexer->NextToken();
                        while (lexer->token.kind == TOKEN_KIND::COMMA)
                        {
                            lexer->NextToken();
                            lexer->CheckToken(TOKEN_KIND::NAME);

                            ++names_count;
                            names_data_bytes_total += lexer->token.end - lexer->token.start;

                            lexer->NextToken();
                        }
                        lexer->CheckToken(TOKEN_KIND::SEMICOLON);
                    }

                    asset->names        = cast<AssetNames *>(m_Allocator->Allocate(sizeof(u64) + sizeof(u64) * names_count + names_data_bytes_total));
                    asset->names->count = names_count;

                    *lexer = lexer_save;
                    {
                        AssetName *name = cast<AssetName *>(asset->names->names);

                        name->length = lexer->token.end - lexer->token.start;
                        memcpy(name->data, lexer->token.start, name->length);

                        lexer->NextToken();
                        for (u64 i = 0; lexer->token.kind == TOKEN_KIND::COMMA; ++i)
                        {
                            lexer->NextToken();
                            lexer->CheckToken(TOKEN_KIND::NAME);

                            name = cast<AssetName *>(name->data + name->length);

                            name->length = lexer->token.end - lexer->token.start;
                            memcpy(name->data, lexer->token.start, name->length);

                            lexer->NextToken();
                        }
                        lexer->CheckToken(TOKEN_KIND::SEMICOLON);
                    }
                }

                lexer->NextToken();
            } while (lexer->token.kind == TOKEN_KIND::STRING);
        }
        else if (lexer->TokenEqualsKeyword(KEYWORD_ID_SFX))
        {
            REV_FAILED_M("SFX blocks are not supported yet.");
        }
        else if (lexer->TokenEqualsKeyword(KEYWORD_ID_MUSIC))
        {
            REV_FAILED_M("Music blocks are not supported yet.");
        }
        else
        {
            break;
        }
    }
}

void AssetManager::ParseREVAMFile()
{
    Lexer lexer(m_UserREVAMFileName, m_UserREVAMStream);

    while (lexer.token.kind != TOKEN_KIND::EOS)
    {
        if (lexer.token.kind == TOKEN_KIND::KEYWORD
        &&  lexer.TokenEquals("static", REV_CSTRLEN("static")))
        {
            lexer.NextToken();
            ParseExternalBlock(&lexer, &m_StaticArea);
            break;
        }
        lexer.NextToken();
    }
}

void AssetManager::ParseREVAMFile(const ConstString& scene_name)
{
    Lexer lexer(m_UserREVAMFileName, m_UserREVAMStream);

    while (lexer.token.kind != TOKEN_KIND::EOS)
    {
        if (lexer.token.kind == TOKEN_KIND::KEYWORD
        &&  lexer.TokenEquals("scene", REV_CSTRLEN("scene")))
        {
            lexer.NextToken();
            lexer.CheckToken(TOKEN_KIND::NAME);

            if (lexer.TokenEquals(scene_name.Data(), scene_name.Length()))
            {
                lexer.NextToken();
                ParseExternalBlock(&lexer, &m_SceneArea);
                break;
            }
        }
        lexer.NextToken();
    }
}

}
