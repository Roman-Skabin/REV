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
// Keyword
//

enum KEYWORD_ID : u32
{
    KEYWORD_ID_STATIC,
    KEYWORD_ID_SCENE,
    KEYWORD_ID_SHADERS,
    KEYWORD_ID_MESHES,
    KEYWORD_ID_CBUFFERS,
    KEYWORD_ID_TEXTURES,
    KEYWORD_ID_NAME,
    KEYWORD_ID_NAMES,

    KEYWORD_ID_COUNT,
    KEYWORD_ID_UNKNOWN  = REV_U32_MAX
};

struct Keyword
{
    const char *name;
    u64         name_len;
};

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
    LBRACE       = '{', // 123
    RBRACE       = '}', // 125

    LAST_LITERAL = 0xFF,

    KEYWORD,
    NAME,
    STRING,

    MAX
};

struct Token
{
    TOKEN_KIND  kind       = TOKEN_KIND::UNKNOWN;
    KEYWORD_ID  keyword_id = KEYWORD_ID_UNKNOWN;
    Math::v2u   pos        = Math::v2u(1, 0);
    const char *start      = null;
    const char *end        = null;
    const char *name       = null;
    // @TODO(Roman): Interns or hash table or mix or nothing?
    // const char *str_val = null;
};

//
// Lexer
//

struct LexerState
{
    const char *stream     = null;
    const char *line_start = null;
    Token       token;
};

struct Lexer
{
    const char *filename;
    const char *stream;
    const char *line_start;
    Token       token;
    Keyword     keywords[KEYWORD_ID_COUNT];

    Lexer(const char *filename, const char *stream);

    void NextToken();
    void REV_CDECL SyntaxError(const char *format, ...);

    bool IsKeyword(const char *str, u64 length);

    const char *GetKeywordName(const char *str, u64 length);

    bool TokenEquals(const char *str, u64 length);
    bool TokenEqualsKeyword(KEYWORD_ID id);
    
    void CheckToken(TOKEN_KIND token_kind);

    void SaveState(LexerState *state);
    void LoadState(LexerState *state);
};

Lexer::Lexer(const char *filename, const char *stream)
    : filename(filename),
      stream(stream),
      line_start(stream),
      token(),
      keywords{{ "static",   REV_CSTRLEN("static")   },
               { "scene",    REV_CSTRLEN("scene")    },
               { "shaders",  REV_CSTRLEN("shaders")  },
               { "meshes",   REV_CSTRLEN("meshes")   },
               { "cbuffers", REV_CSTRLEN("cbuffers") },
               { "textures", REV_CSTRLEN("textures") },
               { "name",     REV_CSTRLEN("name")     },
               { "names",    REV_CSTRLEN("names")    }}
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

    token.keyword_id = KEYWORD_ID_UNKNOWN;
    token.start      = stream;

    if (isalpha(*stream) || *stream == '_')
    {
        while (isalnum(*stream) || *stream == '_') ++stream;
        token.name = GetKeywordName(token.start, stream - token.start);
        token.kind = token.name ? TOKEN_KIND::KEYWORD : TOKEN_KIND::NAME;
    }
    ELSE_IF_CHAR('\0', TOKEN_KIND::EOS)
    ELSE_IF_CHAR(',', TOKEN_KIND::COMMA)
    ELSE_IF_CHAR(':', TOKEN_KIND::COLON)
    ELSE_IF_CHAR(';', TOKEN_KIND::SEMICOLON)
    ELSE_IF_CHAR('{', TOKEN_KIND::LBRACE)
    ELSE_IF_CHAR('}', TOKEN_KIND::RBRACE)
    else if (*stream == '"')
    {
        ++stream;
        while (*stream && *stream != '"')
        {
            if (*stream == '\n') SyntaxError("unexpected end of line, expected '\"'");
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

    DebugFC(DEBUG_COLOR::ERROR, "%s(%I32u:%I32u): REVAM syntax error: %s.", filename, token.pos.r, token.pos.c, buffer);
    // @TODO(Roman): #CrossPlatform
    ExitProcess(1);
}

bool Lexer::IsKeyword(const char *str, u64 length)
{
    if (token.keyword_id < KEYWORD_ID_COUNT)
    {
        Keyword *keyword = keywords + token.keyword_id;
        return keyword->name_len == length
            && !memcmp(keyword->name, str, length);
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
            token.keyword_id = cast<KEYWORD_ID>(it - keywords);
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

void Lexer::CheckToken(TOKEN_KIND token_kind)
{
    if (token.kind != token_kind)
    {
        const char *token_kind_names[cast<u64>(TOKEN_KIND::MAX)] = {'\0'};
        token_kind_names[cast<u64>(TOKEN_KIND::EOS)]       = "<EOS>";
        token_kind_names[cast<u64>(TOKEN_KIND::COMMA)]     = ",";
        token_kind_names[cast<u64>(TOKEN_KIND::COLON)]     = ":";
        token_kind_names[cast<u64>(TOKEN_KIND::SEMICOLON)] = ";";
        token_kind_names[cast<u64>(TOKEN_KIND::LBRACE)]    = "{";
        token_kind_names[cast<u64>(TOKEN_KIND::RBRACE)]    = "}";
        token_kind_names[cast<u64>(TOKEN_KIND::KEYWORD)]   = "<keyword>";
        token_kind_names[cast<u64>(TOKEN_KIND::NAME)]      = "<name>";
        token_kind_names[cast<u64>(TOKEN_KIND::STRING)]    = "<string>";

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

void Lexer::SaveState(LexerState *state)
{
    state->stream     = stream;
    state->line_start = line_start;
    state->token      = token;
}

void Lexer::LoadState(LexerState *state)
{
    stream     = state->stream;
    line_start = state->line_start;
    token      = state->token;
}

void AssetManager::ParseExternalBlock(void *_lexer, Array<Asset> *area)
{
    Lexer *lexer = cast<Lexer *>(_lexer);

    lexer->NextToken();
    lexer->CheckToken(TOKEN_KIND::KEYWORD);

    while (true)
    {
        if (lexer->TokenEqualsKeyword(KEYWORD_ID_SHADERS))
        {
            REV_FAILED_M("SFX blocks are not supported yet.");
        }
        else if (lexer->TokenEqualsKeyword(KEYWORD_ID_MESHES))
        {
            REV_FAILED_M("SFX blocks are not supported yet.");
        }
        else if (lexer->TokenEqualsKeyword(KEYWORD_ID_CBUFFERS))
        {
            REV_FAILED_M("SFX blocks are not supported yet.");
        }
        else if (lexer->TokenEqualsKeyword(KEYWORD_ID_TEXTURES))
        {
            lexer->NextToken();
            lexer->CheckToken(TOKEN_KIND::LBRACE);

            lexer->NextToken();
            lexer->CheckToken(TOKEN_KIND::STRING);

            do
            {
                Asset *asset = area->PushBack();

                // @NOTE(Roman): filename without quotes
                {
                    const char *filename     = lexer->token.start + 1;
                    u64         filename_len = lexer->token.end - lexer->token.start - 2;

                    ParseTexture(asset, filename, filename_len);
                }

                lexer->NextToken();
                lexer->CheckToken(TOKEN_KIND::LBRACE);

                lexer->NextToken();
                lexer->CheckToken(TOKEN_KIND::KEYWORD);

                if (!lexer->TokenEqualsKeyword(KEYWORD_ID_NAMES))
                {
                    lexer->SyntaxError("The only allowed keyword here is 'names', got %s", lexer->token.name);
                }

                lexer->NextToken();
                lexer->CheckToken(TOKEN_KIND::COLON);

                lexer->NextToken();
                lexer->CheckToken(TOKEN_KIND::NAME);

                // @NOTE(Roman): names
                {
                    u64 names_count            = 1;
                    u64 names_data_bytes_total = lexer->token.end - lexer->token.start;

                    LexerState lexer_save;
                    lexer->SaveState(&lexer_save);
                    {
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

                    lexer->LoadState(&lexer_save);
                    {
                        AssetName *name = cast<AssetName *>(asset->names->names);

                        name->length = lexer->token.end - lexer->token.start;
                        memcpy(name->data, lexer->token.start, name->length);

                        lexer->NextToken();
                        while (lexer->token.kind == TOKEN_KIND::COMMA)
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
                lexer->CheckToken(TOKEN_KIND::RBRACE);

                lexer->NextToken();
            } while (lexer->token.kind == TOKEN_KIND::STRING);

            lexer->CheckToken(TOKEN_KIND::RBRACE);
        }
        else
        {
            break;
        }
    }
}

void AssetManager::ParseREVAMFile()
{
    Lexer lexer(m_UserREVAMFileName.Data(), m_UserREVAMStream);

    while (lexer.token.kind != TOKEN_KIND::EOS)
    {
        if (lexer.token.kind == TOKEN_KIND::KEYWORD
        &&  lexer.TokenEquals("static", REV_CSTRLEN("static")))
        {
            lexer.NextToken();
            lexer.CheckToken(TOKEN_KIND::LBRACE);

            ParseExternalBlock(&lexer, &m_StaticArea);

            lexer.NextToken();
            lexer.CheckToken(TOKEN_KIND::RBRACE);

            break;
        }
        lexer.NextToken();
    }
}

void AssetManager::ParseREVAMFile(const ConstString& scene_name)
{
    Lexer lexer(m_UserREVAMFileName.Data(), m_UserREVAMStream);

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
                lexer.CheckToken(TOKEN_KIND::LBRACE);

                ParseExternalBlock(&lexer, &m_SceneArea);

                lexer.NextToken();
                lexer.CheckToken(TOKEN_KIND::RBRACE);

                break;
            }
        }
        lexer.NextToken();
    }
}

}