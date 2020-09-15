//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "graphics/shader_parser/shader_parser.h"

global const char *gTokenKindNames[] =
{
    [TOKEN_KIND_EOF]       = "EOF",

    [TOKEN_KIND_HASH]      = "#",
    [TOKEN_KIND_LPAREN]    = "(",
    [TOKEN_KIND_RPAREN]    = ")",
    [TOKEN_KIND_LBRACE]    = "{",
    [TOKEN_KIND_RBRACE]    = "}",
    [TOKEN_KIND_LBRACKET]  = "[",
    [TOKEN_KIND_RBRACKET]  = "]",
    [TOKEN_KIND_COMMA]     = ",",
    [TOKEN_KIND_COLON]     = ":",
    [TOKEN_KIND_SEMICOLON] = ";",

    [TOKEN_KIND_NAME]      = "<name>",
    [TOKEN_KIND_INT]       = "<integer>",
    [TOKEN_KIND_STRING]    = "<string>",
    [TOKEN_KIND_KEYWORD]   = "<keyword>",
};

typedef struct Keyword
{
    const char *name;
    u64         name_len;
} Keyword;

global Keyword gKeywords[] =
{
    { "cengine",     CSTRLEN("cengine")     },

    { "pipeline",    CSTRLEN("pipeline")    },
    { "shader",      CSTRLEN("shader")      },

    { "blending",    CSTRLEN("blending")    },
    { "depth_test",  CSTRLEN("depth_test")  },
    { "cull_mode",   CSTRLEN("cull_mode")   },
    { "entry_point", CSTRLEN("entry_point") },

    { "enabled",     CSTRLEN("enabled")     },
    { "disabled",    CSTRLEN("disabled")    },

    { "none",        CSTRLEN("none")        },
    { "front",       CSTRLEN("front")       },
    { "back",        CSTRLEN("back")        },

    { "vertex",      CSTRLEN("vertex")      },
    { "hull",        CSTRLEN("hull")        },
    { "domain",      CSTRLEN("domain")      },
    { "geometry",    CSTRLEN("geometry")    },
    { "pixel",       CSTRLEN("pixel")       },

    { "typedef",     CSTRLEN("typedef")     },
    { "struct",      CSTRLEN("struct")      },
    { "cbuffer",     CSTRLEN("cbuffer")     },
    { "tbuffer",     CSTRLEN("tbuffer")     }, // @TODO(Roman): texture buffer  SRV/UAV

    { "register",    CSTRLEN("register")    },
    { "packoffset",  CSTRLEN("packoffset")  },
};

#define IsKeyword(lexer, name) ((lexer)->first_keyword <= (name) && (name) <= (lexer)->last_keyword)

#define SyntaxError(lexer, format, ...)                                      \
{                                                                            \
    (lexer)->token.pos.c = cast(u32, (lexer)->stream - (lexer)->line_start); \
    MessageF(MESSAGE_TYPE_ERROR,                                             \
             CSTRCAT("Shader syntax error (%I32u:%I32u): ", format),         \
             (lexer)->token.pos.r, (lexer)->token.pos.c, __VA_ARGS__);       \
}

void CreateGraphicsShaderLexer(Engine *engine, const char *stream, ShaderLexer *lexer)
{
    lexer->token.pos.r = 1;

    lexer->token_kind_names       = gTokenKindNames;
    lexer->token_kind_names_count = ArrayCount(gTokenKindNames);

    // init interns
    CreateInterns(engine, &lexer->interns);

    // init keywords
    lexer->first_keyword   = InternString(&lexer->interns, gKeywords->name, gKeywords->name_len);
    u32 last_keyword_index = ArrayCount(gKeywords) - 1;
    for (u32 i = 1; i < last_keyword_index; ++i)
    {
        Keyword *keyword = gKeywords + i;
        InternString(&lexer->interns, keyword->name, keyword->name_len);
    }
    Keyword *last_keyword = gKeywords + last_keyword_index;
    lexer->last_keyword   = InternString(&lexer->interns, last_keyword->name, last_keyword->name_len);

    // init stream
    lexer->stream     = stream;
    lexer->line_start = lexer->stream;
    GetNextGraphicsShaderToken(lexer);
}

internal void TokenizeFloat(ShaderLexer *lexer)
{
    // [whitespace][sign][digits][.digits][{e|E}[sign]digits]

    // @NOTE(Roman): Sign, digits before a dot and dot itself have to be already passed.

    lexer->token.kind = TOKEN_KIND_FLOAT;

    lexer->token.f64_val = atof(lexer->token.start);

    while (isdigit(*lexer->stream)) ++lexer->stream;
    if (*lexer->stream == 'e' || *lexer->stream == 'E')
    {
        ++lexer->stream;
        if (*lexer->stream == '+' || *lexer->stream == '-') ++lexer->stream;
        while (isdigit(*lexer->stream)) ++lexer->stream;
    }
}

internal void TokenizeInt(ShaderLexer *lexer, u32 base)
{
    // [whitespace][sign][digits]

    // @NOTE(Roman): Minus and digits have to be already passed.

    lexer->token.kind = TOKEN_KIND_INT;

    if (*lexer->token.start == '-')
    {
        lexer->token.s64_val = strtoll(lexer->token.start, null, base);
    }
    else
    {
        lexer->token.u64_val = strtoull(lexer->token.start, null, base);
    }
}

#define ELSE_IF_CHAR(c1, k1) else if (*lexer->stream == (c1)) { lexer->token.kind = (k1); ++lexer->stream; }

void GetNextGraphicsShaderToken(ShaderLexer *lexer)
{
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

    if (isalpha(*lexer->stream) || *lexer->stream == '_')
    {
        while (isalnum(*lexer->stream) || *lexer->stream == '_') ++lexer->stream;
        lexer->token.name = InternStringRange(&lexer->interns, lexer->token.start, lexer->stream);
        lexer->token.kind = IsKeyword(lexer, lexer->token.name) ? TOKEN_KIND_KEYWORD : TOKEN_KIND_NAME;
    }
    else if (isdigit(*lexer->stream))
    {
        while (isdigit(*lexer->stream)) ++lexer->stream;

        if (*lexer->stream == '.')
        {
            ++lexer->stream;
            TokenizeFloat(lexer);
        }
        else
        {
            u32 base = 10;

            if (*lexer->token.start == '0')
            {
                char base_identifier = lexer->token.start[1];

                /**/ if (base_identifier == 'x' || base_identifier == 'X') base = 16;
                else if (base_identifier == 'b' || base_identifier == 'B') base = 2;
                else if (isdigit(base_identifier))                         base = 8;
                else SyntaxError(lexer, "unrecognized base identifier: expected 'x' or 'X' or 'b' or 'B', got '%c'", base_identifier);
            }

            TokenizeInt(lexer, base);
        }
    }
    else if (*lexer->stream == '-')
    {
        const char *digit_start = ++lexer->stream;
        while (isdigit(*lexer->stream)) ++lexer->stream;
        if (*lexer->stream == '.')
        {
            ++lexer->stream;
            TokenizeFloat(lexer);
        }
        else if (lexer->stream - digit_start)
        {
            u32 base = 10;

            if (*digit_start == '0')
            {
                char base_identifier = digit_start[1];

                /**/ if (base_identifier == 'x' || base_identifier == 'X') base = 16;
                else if (base_identifier == 'b' || base_identifier == 'B') base = 2;
                else if (isdigit(base_identifier))                         base = 8;
                else SyntaxError(lexer, "unrecognized base identifier: expected 'x' or 'X' or 'b' or 'B', got '%c'", base_identifier);
            }

            TokenizeInt(lexer, base);
        }
        else
        {
            goto tokenize_again;
        }
    }
    else if (*lexer->stream == '.' && isdigit(lexer->stream[1]))
    {
        ++lexer->stream;
        TokenizeFloat(lexer);
    }
    ELSE_IF_CHAR('\0', TOKEN_KIND_EOF)
    ELSE_IF_CHAR('#', TOKEN_KIND_HASH)
    ELSE_IF_CHAR('(', TOKEN_KIND_LPAREN)
    ELSE_IF_CHAR(')', TOKEN_KIND_RPAREN)
    ELSE_IF_CHAR('{', TOKEN_KIND_LBRACE)
    ELSE_IF_CHAR('}', TOKEN_KIND_RBRACE)
    ELSE_IF_CHAR('[', TOKEN_KIND_LBRACKET)
    ELSE_IF_CHAR(']', TOKEN_KIND_RBRACKET)
    ELSE_IF_CHAR(',', TOKEN_KIND_COMMA)
    ELSE_IF_CHAR(':', TOKEN_KIND_COLON)
    ELSE_IF_CHAR(';', TOKEN_KIND_SEMICOLON)
    else if (*lexer->stream == '/' && lexer->stream[1] == '*')
    {
        while (!(*lexer->stream == '*' && lexer->stream[1] == '/'))
        {
            if (*lexer->stream++ == '\n')
            {
                ++lexer->token.pos.r;
                lexer->line_start = lexer->stream;
            }
        }
        lexer->stream += 2;
        goto tokenize_again;
    }
    else if (*lexer->stream == '/' && lexer->stream[1] == '/')
    {
        while (*lexer->stream != '\n') ++lexer->stream;
        ++lexer->stream;
        ++lexer->token.pos.r;
        lexer->line_start = lexer->stream;
        goto tokenize_again;
    }
    else if (*lexer->stream == '"')
    {
        ++lexer->stream;
        while (*lexer->stream && *lexer->stream != '"')
        {
            ++lexer->stream;
        }
        if (*lexer->stream)
        {
            if (*lexer->stream != '"') SyntaxError(lexer, "expected '\"', got '%c'", *lexer->stream);
            ++lexer->stream;
        }
        else
        {
            SyntaxError(lexer, "unexpected end of file");
        }
        lexer->token.str_val = InternStringRange(&lexer->interns, lexer->token.start, lexer->stream);
        lexer->token.kind    = TOKEN_KIND_STRING;
    }
    else
    {
        ++lexer->stream;
        goto tokenize_again;
    }

    lexer->token.end   = lexer->stream;
    lexer->token.pos.c = cast(u32, lexer->token.start - lexer->line_start + 1);
}

#undef ELSE_IF_CHAR
