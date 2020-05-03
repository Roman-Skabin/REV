//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "graphics/core/shader_parser.h"

global const char *token_kind_names[] =
{
    [TOKEN_KIND_EOF]         = "EOF",
    [TOKEN_KIND_DO_NOT_CARE] = "ShaderParser doesn't care about this token",

    [TOKEN_KIND_HASH]        = "#",
    [TOKEN_KIND_LPAREN]      = "(",
    [TOKEN_KIND_RPAREN]      = ")",
    [TOKEN_KIND_LBRACE]      = "{",
    [TOKEN_KIND_RBRACE]      = "}",
    [TOKEN_KIND_COMMA]       = ",",
    [TOKEN_KIND_COLON]       = ":",
    [TOKEN_KIND_SEMICOLON]   = ";",

    [TOKEN_KIND_NAME]        = "<name>",
    [TOKEN_KIND_STRING]      = "<string>",
    [TOKEN_KIND_KEYWORD]     = "<keyword>",
};

global const char *keywords[] =
{
    "cengine",
    "define",
    "elif",
    "else",
    "endif",
    "error",
    "if",
    "ifdef",
    "ifndef",
    "include",
    "line",
    "pragma",
    "undef",

    "blending",
    "depth_test",
    "shader",
    "entry_point",

    "enable",
    "disable",

    "vertex",
    "hull",
    "domain",
    "geometry",
    "pixel",

    "struct",
};

#define IsKeyword(lexer, name) ((lexer)->first_keyword <= (name) && (name) <= (lexer)->last_keyword)

#define SyntaxError(format, ...) MessageF(MESSAGE_TYPE_ERROR, CSTRCAT("Shader syntax error: ", format), __VA_ARGS__)

void CreateGraphicsShaderLexer(Engine *engine, const char *stream, ShaderLexer *lexer)
{
    lexer->token_kind_names       = token_kind_names;
    lexer->token_kind_names_count = ArrayCount(token_kind_names);

    // init interns
    CreateInterns(engine, &lexer->interns);

    // init keywords
    lexer->first_keyword = InternString(&lexer->interns, *keywords, strlen(*keywords));
    u32 last_index = ArrayCount(keywords) - 1;
    for (u32 i = 1; i < last_index; ++i)
    {
        InternString(&lexer->interns, keywords[i], strlen(keywords[i]));
    }
    lexer->last_keyword = InternString(&lexer->interns, keywords[last_index], strlen(keywords[last_index]));

    // init stream
    lexer->stream = stream;
    GetNextGraphicsShaderToken(lexer);
}

#define ELSE_IF_STREAM_1(c1, k1) else if (*lexer->stream == (c1)) { lexer->token.kind = (k1); ++lexer->stream; }

void GetNextGraphicsShaderToken(ShaderLexer *lexer)
{
comment_passed:

    while (isspace(*lexer->stream)) ++lexer->stream;

    lexer->token.start = lexer->stream;

    if (isalpha(*lexer->stream) || *lexer->stream == '_')
    {
        while (isalnum(*lexer->stream) || *lexer->stream == '_') ++lexer->stream;
        lexer->token.name = InternStringRange(&lexer->interns, lexer->token.start, lexer->stream);
        lexer->token.kind = IsKeyword(lexer, lexer->token.name) ? TOKEN_KIND_KEYWORD : TOKEN_KIND_NAME;
    }
    ELSE_IF_STREAM_1('\0', TOKEN_KIND_EOF)
    ELSE_IF_STREAM_1('#', TOKEN_KIND_HASH)
    ELSE_IF_STREAM_1('(', TOKEN_KIND_LPAREN)
    ELSE_IF_STREAM_1(')', TOKEN_KIND_RPAREN)
    ELSE_IF_STREAM_1('{', TOKEN_KIND_LBRACE)
    ELSE_IF_STREAM_1('}', TOKEN_KIND_RBRACE)
    ELSE_IF_STREAM_1(',', TOKEN_KIND_COMMA)
    ELSE_IF_STREAM_1(':', TOKEN_KIND_COLON)
    ELSE_IF_STREAM_1(';', TOKEN_KIND_SEMICOLON)
    else if (*lexer->stream == '/' && lexer->stream[1] == '*')
    {
        while (!(*lexer->stream == '*' && lexer->stream[1] == '/')) ++lexer->stream;
        lexer->stream += 2;
        goto comment_passed;
    }
    else if (*lexer->stream == '/' && lexer->stream[1] == '/')
    {
        while (*lexer->stream != '\n') ++lexer->stream;
        ++lexer->stream;
        goto comment_passed;
    }
    else if (*lexer->stream == '"')
    {
        ++lexer->stream;
        while (*lexer->stream && *lexer->stream != '"')
        {
            ++lexer->stream;
            if (*lexer->stream == '\n')
            {
                SyntaxError("no new lines in shader names");
            }
            else if (*lexer->stream == '\\')
            {
                SyntaxError("no escape characters in shader names");
            }
        }
        if (*lexer->stream)
        {
            if (*lexer->stream != '"') SyntaxError("expected '\"', got '%c'", *lexer->stream);
            ++lexer->stream;
        }
        else
        {
            SyntaxError("unexpected end of file");
        }
        lexer->token.str_val = InternStringRange(&lexer->interns, lexer->token.start, lexer->stream);
        lexer->token.kind    = TOKEN_KIND_STRING;
    }
    else
    {
        lexer->token.kind = TOKEN_KIND_DO_NOT_CARE;
        ++lexer->stream;
    }

    lexer->token.end = lexer->stream;
}
