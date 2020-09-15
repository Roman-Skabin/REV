//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "graphics/shader_parser/interns.h"
#include "graphics/shader_parser/ast.h"
#include "graphics/gpu_program_manager.h"
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

ENGINE_FUN const char *TokenKindName(
    in ShaderLexer *lexer,
    in TOKEN_KIND   kind
);

ENGINE_FUN const char *TokenInfo(
    in ShaderLexer *lexer
);

ENGINE_FUN void CheckToken(
    in ShaderLexer *lexer,
    in TOKEN_KIND   kind
);

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

ENGINE_FUN void CreateGraphicsShaderLexer(
    in  Engine      *engine,
    in  const char  *stream,
    out ShaderLexer *lexer
);

ENGINE_FUN void GetNextGraphicsShaderToken(
    in ShaderLexer *lexer
);

typedef struct ShaderParser
{
    ShaderLexer      lexer;
    LIST ASTType    *types;
    LIST ASTCBuffer *cbuffers;
} ShaderParser;

ENGINE_FUN void ParseGraphicsShaders(
    in  Engine              *engine,
    in  const char          *file_with_shaders,
    in  GraphicsProgram     *program,
    out GraphicsProgramDesc *gpd
);
