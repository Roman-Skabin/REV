//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "graphics/shader_parser/shader_parser.h"

#define TokenEquals(token, str, len) (((token).end - (token).start == (len)) && RtlEqualMemory((token).start, str, len))
#define TokenEqualsCSTR(token, cstr) TokenEquals(token, cstr, CSTRLEN(cstr))

#define SyntaxError(lexer, format, ...)                                \
{                                                                      \
    MessageF(MESSAGE_TYPE_ERROR,                                       \
             CSTRCAT("Shader syntax error (%I32u:%I32u): ", format),   \
             (lexer)->token.pos.r, (lexer)->token.pos.c, __VA_ARGS__); \
}

const char *TokenKindName(
    IN ShaderLexer *lexer,
    IN TOKEN_KIND   kind)
{
    if (kind < lexer->token_kind_names_count)
    {
        const char *name = lexer->token_kind_names[kind];
        return name ? name : "<unknown>";
    }
    else
    {
        return "<unknown>";
    }
}

const char *TokenInfo(
    IN ShaderLexer *lexer)
{
    if (lexer->token.kind == TOKEN_KIND_NAME
    ||  lexer->token.kind == TOKEN_KIND_KEYWORD)
    {
        return lexer->token.name;
    }
    else
    {
        return TokenKindName(lexer, lexer->token.kind);
    }
}

void CheckToken(
    IN ShaderLexer *lexer,
    IN TOKEN_KIND   kind)
{
    if (lexer->token.kind != kind)
    {
        SyntaxError(lexer, "expected '%s', got '%s'.",
                    TokenKindName(lexer, kind),
                    TokenInfo(lexer));
    }
}
