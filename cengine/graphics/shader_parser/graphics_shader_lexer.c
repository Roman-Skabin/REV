//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "graphics/shader_parser/shader_parser.h"

global const char *token_kind_names[] =
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

global const char *keywords[] =
{
    "cengine",

    "pipeline",
    "shader",

    "blending",
    "depth_test",
    "cull_mode",
    "entry_point",

    "enabled",
    "disabled",

    "none",
    "front",
    "back",

    "vertex",
    "hull",
    "domain",
    "geometry",
    "pixel",

    "typedef",
    "struct",
    "cbuffer",
    "tbuffer", // @TODO(Roman): texture buffer  SRV/UAV

    "register",
    "packoffset",
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

    lexer->token_kind_names       = token_kind_names;
    lexer->token_kind_names_count = ArrayCount(token_kind_names);

    // init interns
    CreateInterns(engine, &lexer->interns);

    // init keywords
    lexer->first_keyword = InternString(&lexer->interns, *keywords, strlen(*keywords));
    u32 last_keyword_index = ArrayCount(keywords) - 1;
    for (u32 i = 1; i < last_keyword_index; ++i)
    {
        InternString(&lexer->interns, keywords[i], strlen(keywords[i]));
    }
    lexer->last_keyword = InternString(&lexer->interns, keywords[last_keyword_index], strlen(keywords[last_keyword_index]));

    // init stream
    lexer->stream     = stream;
    lexer->line_start = lexer->stream;
    GetNextGraphicsShaderToken(lexer);
}

internal void TokenizeFloat(ShaderLexer *lexer)
{
    // @NOTE(Roman): Minus, digits before a dot and dot itself have to be already passed.
    lexer->token.kind = TOKEN_KIND_FLOAT;
    while (isdigit(*lexer->stream)) ++lexer->stream;
    if (*lexer->stream == 'e')
    {
        ++lexer->stream;
        if (*lexer->stream == '+' || *lexer->stream == '-') ++lexer->stream;
        while (isdigit(*lexer->stream)) ++lexer->stream;
        _snscanf(lexer->token.start, lexer->stream - lexer->token.start, "%le", &lexer->token.f64_val);
        if (*lexer->stream == 'f') ++lexer->stream;
    }
    else if (*lexer->stream == 'E')
    {
        ++lexer->stream;
        if (*lexer->stream == '+' || *lexer->stream == '-') ++lexer->stream;
        while (isdigit(*lexer->stream)) ++lexer->stream;
        _snscanf(lexer->token.start, lexer->stream - lexer->token.start, "%lE", &lexer->token.f64_val);
        if (*lexer->stream == 'f') ++lexer->stream;
    }
    else
    {
        _snscanf(lexer->token.start, lexer->stream - lexer->token.start, "%lf", &lexer->token.f64_val);
        if (*lexer->stream == 'f') ++lexer->stream;
    }
}

internal void TokenizeInt(ShaderLexer *lexer)
{
    // @NOTE(Roman): Minus and digits have to be already passed.
    lexer->token.kind = TOKEN_KIND_INT;
    if (*lexer->token.start == '-')
    {
        _snscanf(lexer->token.start + 1,
                 lexer->stream - lexer->token.start - 1,
                 "%I64i",
                 &lexer->token.s64_val);
        lexer->token.s64_val = -lexer->token.s64_val;
    }
    else
    {
        _snscanf(lexer->token.start,
                 lexer->stream - lexer->token.start,
                 "%I64i",
                 &lexer->token.u64_val);
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
            TokenizeInt(lexer);
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
            TokenizeInt(lexer);
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
