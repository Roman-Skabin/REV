//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "graphics/shader_parser/interns.h"
#include "graphics/shader_parser/ast.h"
#include "graphics/core/core_renderer.h"
#include "math/vec.h"

typedef enum TOKEN_KIND
{
    TOKEN_KIND_EOF,

    TOKEN_KIND_HASH,
    TOKEN_KIND_LPAREN,   // (
    TOKEN_KIND_RPAREN,   // )
    TOKEN_KIND_LBRACE,   // {
    TOKEN_KIND_RBRACE,   // }
    TOKEN_KIND_LBRACKET, // [
    TOKEN_KIND_RBRACKET, // ]
    TOKEN_KIND_COMMA,
    TOKEN_KIND_COLON,
    TOKEN_KIND_SEMICOLON,

    TOKEN_KIND_NAME,
    TOKEN_KIND_INT,
    TOKEN_KIND_FLOAT,
    TOKEN_KIND_STRING,
    TOKEN_KIND_KEYWORD,
} TOKEN_KIND;

typedef struct Token
{
    TOKEN_KIND  kind;
    v2u         pos;
    const char *start;
    const char *end;
    union
    {
        const char *name;
        const char *str_val;
        s64         s64_val;
        u64         u64_val;
        f64         f64_val;
    };
} Token;

typedef struct ShaderLexer ShaderLexer;

CENGINE_FUN const char *TokenKindName(ShaderLexer *lexer, TOKEN_KIND kind);
CENGINE_FUN const char *TokenInfo(ShaderLexer *lexer);
CENGINE_FUN void CheckToken(ShaderLexer *lexer, TOKEN_KIND kind);

struct ShaderLexer
{
    const char  *stream;
    const char  *line_start;
    Token        token;
    Interns      interns;

    const char  *first_keyword;
    const char  *last_keyword;

    const char **token_kind_names;
    u64          token_kind_names_count;
};

CENGINE_FUN void CreateGraphicsShaderLexer(Engine *engine, const char *stream, ShaderLexer *lexer);
CENGINE_FUN void GetNextGraphicsShaderToken(ShaderLexer *lexer);

typedef struct ShaderParser
{
    ShaderLexer  lexer;
    ASTType     *types;
} ShaderParser;

CENGINE_FUN void ParseGraphicsShaders(Engine *engine, const char *file_with_shaders, GraphicsProgramDesc *gpd);
