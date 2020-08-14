//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "graphics/shader_parser/shader_parser.h"
#include "cengine.h"

#define TokenEquals(token, str, len) (((token).end - (token).start == (len)) && RtlEqualMemory((token).start, str, len))
#define TokenEqualsCSTR(token, cstr) TokenEquals(token, cstr, CSTRLEN(cstr))

#define SyntaxError(lexer, format, ...)                              \
{                                                                    \
    MessageF(MESSAGE_TYPE_ERROR,                                     \
             CSTRCAT("Shader syntax error (%I32u:%I32u): ", format), \
             (lexer).token.pos.r, (lexer).token.pos.c, __VA_ARGS__); \
}

#define PassModifiers(_parser)                                        \
{                                                                     \
    while (TokenEqualsCSTR((_parser)->lexer.token, "linear")          \
       ||  TokenEqualsCSTR((_parser)->lexer.token, "centroid")        \
       ||  TokenEqualsCSTR((_parser)->lexer.token, "nointerpolation") \
       ||  TokenEqualsCSTR((_parser)->lexer.token, "noperspective")   \
       ||  TokenEqualsCSTR((_parser)->lexer.token, "sample")          \
       ||  TokenEqualsCSTR((_parser)->lexer.token, "const")           \
       ||  TokenEqualsCSTR((_parser)->lexer.token, "in")              \
       ||  TokenEqualsCSTR((_parser)->lexer.token, "out")             \
       ||  TokenEqualsCSTR((_parser)->lexer.token, "inout")           \
       ||  TokenEqualsCSTR((_parser)->lexer.token, "uniform"))        \
    {                                                                 \
        GetNextGraphicsShaderToken(&(_parser)->lexer);                \
    }                                                                 \
}

internal char *ReadEntireShaderFile(
    in  Engine     *engine,
    in  const char *filename,
    opt u32        *size)
{
    Check(engine)
    Check(filename)

    HANDLE file = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, null, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, null);
    CheckM(file != INVALID_HANDLE_VALUE, "File does not exist: %s", filename);

    u32   filesize = GetFileSize(file, null);
    char *buffer   = PushToTransientArea(engine->memory, filesize + 1);

    DebugResult(ReadFile(file, buffer, filesize, null, null));

    DebugResult(CloseHandle(file));

    if (size) *size = filesize;
    return buffer;
}

#define InitBuiltinScalarType(_name, _format, _flag, _size_in_bytes)     \
{                                                                        \
    cur_type->next          = PushToTA(ASTType, engine->memory, 1);      \
    cur_type->base_type     = null;                                      \
    cur_type->name          = InternCSTR(&parser->lexer.interns, _name); \
    cur_type->kind          = AST_TYPE_KIND_SCALAR;                      \
    cur_type->format        = _format;                                   \
    cur_type->flag          = _flag;                                     \
    cur_type->builtin       = true;                                      \
    cur_type->size_in_bytes = _size_in_bytes;                            \
    cur_type                = cur_type->next;                            \
}

#define InitBuiltinVectorType(_name, _format, _flag, _type_size, _dimension) \
{                                                                            \
    cur_type->next          = PushToTA(ASTType, engine->memory, 1);          \
    cur_type->base_type     = null;                                          \
    cur_type->name          = InternCSTR(&parser->lexer.interns, _name);     \
    cur_type->kind          = AST_TYPE_KIND_VECTOR;                          \
    cur_type->format        = _format;                                       \
    cur_type->flag          = _flag;                                         \
    cur_type->builtin       = true;                                          \
    cur_type->size_in_bytes = (_dimension) * (_type_size);                   \
    cur_type->dimension     = _dimension;                                    \
    cur_type                = cur_type->next;                                \
}

#define InitBuiltinMatrixType(_name, _format, _flag, _type_size, _rows, _cols) \
{                                                                              \
    cur_type->next          = PushToTA(ASTType, engine->memory, 1);            \
    cur_type->base_type     = null;                                            \
    cur_type->name          = InternCSTR(&parser->lexer.interns, _name);       \
    cur_type->kind          = AST_TYPE_KIND_MATRIX;                            \
    cur_type->format        = _format;                                         \
    cur_type->flag          = _flag;                                           \
    cur_type->builtin       = true;                                            \
    cur_type->size_in_bytes = (_rows) * (_cols) * (_type_size);                \
    cur_type->mat_size.r    = _rows;                                           \
    cur_type->mat_size.c    = _cols;                                           \
    cur_type                = cur_type->next;                                  \
}

internal void InitBuiltinTypes(Engine *engine, ShaderParser *parser)
{
    // @NOTE(Roman): DXGI has no enum constants for every HLSL builtin type.

    parser->types     = PushToTA(ASTType, engine->memory, 1);
    ASTType *cur_type = parser->types;

    InitBuiltinScalarType("bool",    DXGI_FORMAT_R32_UINT,          AST_TYPE_FLAG_UINT, 4);
    InitBuiltinVectorType("bool1",   DXGI_FORMAT_R32_UINT,          AST_TYPE_FLAG_UINT, 4, 1);
    InitBuiltinVectorType("bool2",   DXGI_FORMAT_R32G32_UINT,       AST_TYPE_FLAG_UINT, 4, 2);
    InitBuiltinVectorType("bool3",   DXGI_FORMAT_R32G32B32_UINT,    AST_TYPE_FLAG_UINT, 4, 3);
    InitBuiltinVectorType("bool4",   DXGI_FORMAT_R32G32B32A32_UINT, AST_TYPE_FLAG_UINT, 4, 4);
    InitBuiltinMatrixType("bool1x1", DXGI_FORMAT_R32_UINT,          AST_TYPE_FLAG_UINT, 4, 1, 1);
    InitBuiltinMatrixType("bool1x2", DXGI_FORMAT_R32G32_UINT,       AST_TYPE_FLAG_UINT, 4, 1, 2);
    InitBuiltinMatrixType("bool1x3", DXGI_FORMAT_R32G32B32_UINT,    AST_TYPE_FLAG_UINT, 4, 1, 3);
    InitBuiltinMatrixType("bool1x4", DXGI_FORMAT_R32G32B32A32_UINT, AST_TYPE_FLAG_UINT, 4, 1, 4);
    InitBuiltinMatrixType("bool2x1", DXGI_FORMAT_R32G32_UINT,       AST_TYPE_FLAG_UINT, 4, 2, 1);
    InitBuiltinMatrixType("bool2x2", DXGI_FORMAT_R32G32B32A32_UINT, AST_TYPE_FLAG_UINT, 4, 2, 2);
    InitBuiltinMatrixType("bool2x3", DXGI_FORMAT_UNKNOWN,           AST_TYPE_FLAG_NONE, 4, 2, 3);
    InitBuiltinMatrixType("bool2x4", DXGI_FORMAT_UNKNOWN,           AST_TYPE_FLAG_NONE, 4, 2, 4);
    InitBuiltinMatrixType("bool3x1", DXGI_FORMAT_UNKNOWN,           AST_TYPE_FLAG_NONE, 4, 3, 1);
    InitBuiltinMatrixType("bool3x2", DXGI_FORMAT_UNKNOWN,           AST_TYPE_FLAG_NONE, 4, 3, 2);
    InitBuiltinMatrixType("bool3x3", DXGI_FORMAT_UNKNOWN,           AST_TYPE_FLAG_NONE, 4, 3, 3);
    InitBuiltinMatrixType("bool3x4", DXGI_FORMAT_UNKNOWN,           AST_TYPE_FLAG_NONE, 4, 3, 4);
    InitBuiltinMatrixType("bool4x1", DXGI_FORMAT_UNKNOWN,           AST_TYPE_FLAG_NONE, 4, 4, 1);
    InitBuiltinMatrixType("bool4x2", DXGI_FORMAT_UNKNOWN,           AST_TYPE_FLAG_NONE, 4, 4, 2);
    InitBuiltinMatrixType("bool4x3", DXGI_FORMAT_UNKNOWN,           AST_TYPE_FLAG_NONE, 4, 4, 3);
    InitBuiltinMatrixType("bool4x4", DXGI_FORMAT_UNKNOWN,           AST_TYPE_FLAG_NONE, 4, 4, 4);

    InitBuiltinScalarType("int",    DXGI_FORMAT_R32_SINT,          AST_TYPE_FLAG_SINT, 4);
    InitBuiltinVectorType("int1",   DXGI_FORMAT_R32_SINT,          AST_TYPE_FLAG_SINT, 4, 1);
    InitBuiltinVectorType("int2",   DXGI_FORMAT_R32G32_SINT,       AST_TYPE_FLAG_SINT, 4, 2);
    InitBuiltinVectorType("int3",   DXGI_FORMAT_R32G32B32_SINT,    AST_TYPE_FLAG_SINT, 4, 3);
    InitBuiltinVectorType("int4",   DXGI_FORMAT_R32G32B32A32_SINT, AST_TYPE_FLAG_SINT, 4, 4);
    InitBuiltinMatrixType("int1x1", DXGI_FORMAT_R32_SINT,          AST_TYPE_FLAG_SINT, 4, 1, 1);
    InitBuiltinMatrixType("int1x2", DXGI_FORMAT_R32G32_SINT,       AST_TYPE_FLAG_SINT, 4, 1, 2);
    InitBuiltinMatrixType("int1x3", DXGI_FORMAT_R32G32B32_SINT,    AST_TYPE_FLAG_SINT, 4, 1, 3);
    InitBuiltinMatrixType("int1x4", DXGI_FORMAT_R32G32B32A32_SINT, AST_TYPE_FLAG_SINT, 4, 1, 4);
    InitBuiltinMatrixType("int2x1", DXGI_FORMAT_R32G32_SINT,       AST_TYPE_FLAG_SINT, 4, 2, 1);
    InitBuiltinMatrixType("int2x2", DXGI_FORMAT_R32G32B32A32_SINT, AST_TYPE_FLAG_SINT, 4, 2, 2);
    InitBuiltinMatrixType("int2x3", DXGI_FORMAT_UNKNOWN,           AST_TYPE_FLAG_NONE, 4, 2, 3);
    InitBuiltinMatrixType("int2x4", DXGI_FORMAT_UNKNOWN,           AST_TYPE_FLAG_NONE, 4, 2, 4);
    InitBuiltinMatrixType("int3x1", DXGI_FORMAT_UNKNOWN,           AST_TYPE_FLAG_NONE, 4, 3, 1);
    InitBuiltinMatrixType("int3x2", DXGI_FORMAT_UNKNOWN,           AST_TYPE_FLAG_NONE, 4, 3, 2);
    InitBuiltinMatrixType("int3x3", DXGI_FORMAT_UNKNOWN,           AST_TYPE_FLAG_NONE, 4, 3, 3);
    InitBuiltinMatrixType("int3x4", DXGI_FORMAT_UNKNOWN,           AST_TYPE_FLAG_NONE, 4, 3, 4);
    InitBuiltinMatrixType("int4x1", DXGI_FORMAT_UNKNOWN,           AST_TYPE_FLAG_NONE, 4, 4, 1);
    InitBuiltinMatrixType("int4x2", DXGI_FORMAT_UNKNOWN,           AST_TYPE_FLAG_NONE, 4, 4, 2);
    InitBuiltinMatrixType("int4x3", DXGI_FORMAT_UNKNOWN,           AST_TYPE_FLAG_NONE, 4, 4, 3);
    InitBuiltinMatrixType("int4x4", DXGI_FORMAT_UNKNOWN,           AST_TYPE_FLAG_NONE, 4, 4, 4);

    InitBuiltinScalarType("uint",    DXGI_FORMAT_R32_UINT,          AST_TYPE_FLAG_UINT, 4);
    InitBuiltinVectorType("uint1",   DXGI_FORMAT_R32_UINT,          AST_TYPE_FLAG_UINT, 4, 1);
    InitBuiltinVectorType("uint2",   DXGI_FORMAT_R32G32_UINT,       AST_TYPE_FLAG_UINT, 4, 2);
    InitBuiltinVectorType("uint3",   DXGI_FORMAT_R32G32B32_UINT,    AST_TYPE_FLAG_UINT, 4, 3);
    InitBuiltinVectorType("uint4",   DXGI_FORMAT_R32G32B32A32_UINT, AST_TYPE_FLAG_UINT, 4, 4);
    InitBuiltinMatrixType("uint1x1", DXGI_FORMAT_R32_UINT,          AST_TYPE_FLAG_UINT, 4, 1, 1);
    InitBuiltinMatrixType("uint1x2", DXGI_FORMAT_R32G32_UINT,       AST_TYPE_FLAG_UINT, 4, 1, 2);
    InitBuiltinMatrixType("uint1x3", DXGI_FORMAT_R32G32B32_UINT,    AST_TYPE_FLAG_UINT, 4, 1, 3);
    InitBuiltinMatrixType("uint1x4", DXGI_FORMAT_R32G32B32A32_UINT, AST_TYPE_FLAG_UINT, 4, 1, 4);
    InitBuiltinMatrixType("uint2x1", DXGI_FORMAT_R32G32_UINT,       AST_TYPE_FLAG_UINT, 4, 2, 1);
    InitBuiltinMatrixType("uint2x2", DXGI_FORMAT_R32G32B32A32_UINT, AST_TYPE_FLAG_UINT, 4, 2, 2);
    InitBuiltinMatrixType("uint2x3", DXGI_FORMAT_UNKNOWN,           AST_TYPE_FLAG_NONE, 4, 2, 3);
    InitBuiltinMatrixType("uint2x4", DXGI_FORMAT_UNKNOWN,           AST_TYPE_FLAG_NONE, 4, 2, 4);
    InitBuiltinMatrixType("uint3x1", DXGI_FORMAT_UNKNOWN,           AST_TYPE_FLAG_NONE, 4, 3, 1);
    InitBuiltinMatrixType("uint3x2", DXGI_FORMAT_UNKNOWN,           AST_TYPE_FLAG_NONE, 4, 3, 2);
    InitBuiltinMatrixType("uint3x3", DXGI_FORMAT_UNKNOWN,           AST_TYPE_FLAG_NONE, 4, 3, 3);
    InitBuiltinMatrixType("uint3x4", DXGI_FORMAT_UNKNOWN,           AST_TYPE_FLAG_NONE, 4, 3, 4);
    InitBuiltinMatrixType("uint4x1", DXGI_FORMAT_UNKNOWN,           AST_TYPE_FLAG_NONE, 4, 4, 1);
    InitBuiltinMatrixType("uint4x2", DXGI_FORMAT_UNKNOWN,           AST_TYPE_FLAG_NONE, 4, 4, 2);
    InitBuiltinMatrixType("uint4x3", DXGI_FORMAT_UNKNOWN,           AST_TYPE_FLAG_NONE, 4, 4, 3);
    InitBuiltinMatrixType("uint4x4", DXGI_FORMAT_UNKNOWN,           AST_TYPE_FLAG_NONE, 4, 4, 4);
    
    InitBuiltinScalarType("dword", DXGI_FORMAT_R32_UINT, AST_TYPE_FLAG_UINT, 4);

    InitBuiltinScalarType("half",    DXGI_FORMAT_R16_FLOAT,          AST_TYPE_FLAG_HALF, 2);
    InitBuiltinVectorType("half1",   DXGI_FORMAT_R16_FLOAT,          AST_TYPE_FLAG_HALF, 2, 1);
    InitBuiltinVectorType("half2",   DXGI_FORMAT_R16G16_FLOAT,       AST_TYPE_FLAG_HALF, 2, 2);
    InitBuiltinVectorType("half3",   DXGI_FORMAT_UNKNOWN,            AST_TYPE_FLAG_HALF, 2, 3);
    InitBuiltinVectorType("half4",   DXGI_FORMAT_R16G16B16A16_FLOAT, AST_TYPE_FLAG_HALF, 2, 4);
    InitBuiltinMatrixType("half1x1", DXGI_FORMAT_R16_FLOAT,          AST_TYPE_FLAG_HALF, 2, 1, 1);
    InitBuiltinMatrixType("half1x2", DXGI_FORMAT_R16G16_FLOAT,       AST_TYPE_FLAG_HALF, 2, 1, 2);
    InitBuiltinMatrixType("half1x3", DXGI_FORMAT_UNKNOWN,            AST_TYPE_FLAG_HALF, 2, 1, 3);
    InitBuiltinMatrixType("half1x4", DXGI_FORMAT_R16G16B16A16_FLOAT, AST_TYPE_FLAG_HALF, 2, 1, 4);
    InitBuiltinMatrixType("half2x1", DXGI_FORMAT_R16G16_FLOAT,       AST_TYPE_FLAG_HALF, 2, 2, 1);
    InitBuiltinMatrixType("half2x2", DXGI_FORMAT_R16G16B16A16_FLOAT, AST_TYPE_FLAG_HALF, 2, 2, 2);
    InitBuiltinMatrixType("half2x3", DXGI_FORMAT_UNKNOWN,            AST_TYPE_FLAG_NONE, 2, 2, 3);
    InitBuiltinMatrixType("half2x4", DXGI_FORMAT_UNKNOWN,            AST_TYPE_FLAG_NONE, 2, 2, 4);
    InitBuiltinMatrixType("half3x1", DXGI_FORMAT_UNKNOWN,            AST_TYPE_FLAG_NONE, 2, 3, 1);
    InitBuiltinMatrixType("half3x2", DXGI_FORMAT_UNKNOWN,            AST_TYPE_FLAG_NONE, 2, 3, 2);
    InitBuiltinMatrixType("half3x3", DXGI_FORMAT_UNKNOWN,            AST_TYPE_FLAG_NONE, 2, 3, 3);
    InitBuiltinMatrixType("half3x4", DXGI_FORMAT_UNKNOWN,            AST_TYPE_FLAG_NONE, 2, 3, 4);
    InitBuiltinMatrixType("half4x1", DXGI_FORMAT_UNKNOWN,            AST_TYPE_FLAG_NONE, 2, 4, 1);
    InitBuiltinMatrixType("half4x2", DXGI_FORMAT_UNKNOWN,            AST_TYPE_FLAG_NONE, 2, 4, 2);
    InitBuiltinMatrixType("half4x3", DXGI_FORMAT_UNKNOWN,            AST_TYPE_FLAG_NONE, 2, 4, 3);
    InitBuiltinMatrixType("half4x4", DXGI_FORMAT_UNKNOWN,            AST_TYPE_FLAG_NONE, 2, 4, 4);

    InitBuiltinScalarType("float",    DXGI_FORMAT_R32_FLOAT,          AST_TYPE_FLAG_FLOAT, 4);
    InitBuiltinVectorType("float1",   DXGI_FORMAT_R32_FLOAT,          AST_TYPE_FLAG_FLOAT, 4, 1);
    InitBuiltinVectorType("float2",   DXGI_FORMAT_R32G32_FLOAT,       AST_TYPE_FLAG_FLOAT, 4, 2);
    InitBuiltinVectorType("float3",   DXGI_FORMAT_R32G32B32_FLOAT,    AST_TYPE_FLAG_FLOAT, 4, 3);
    InitBuiltinVectorType("float4",   DXGI_FORMAT_R32G32B32A32_FLOAT, AST_TYPE_FLAG_FLOAT, 4, 4);
    InitBuiltinMatrixType("float1x1", DXGI_FORMAT_R32_FLOAT,          AST_TYPE_FLAG_FLOAT, 4, 1, 1);
    InitBuiltinMatrixType("float1x2", DXGI_FORMAT_R32G32_FLOAT,       AST_TYPE_FLAG_FLOAT, 4, 1, 2);
    InitBuiltinMatrixType("float1x3", DXGI_FORMAT_R32G32B32_FLOAT,    AST_TYPE_FLAG_FLOAT, 4, 1, 3);
    InitBuiltinMatrixType("float1x4", DXGI_FORMAT_R32G32B32A32_FLOAT, AST_TYPE_FLAG_FLOAT, 4, 1, 4);
    InitBuiltinMatrixType("float2x1", DXGI_FORMAT_R32G32_FLOAT,       AST_TYPE_FLAG_FLOAT, 4, 2, 1);
    InitBuiltinMatrixType("float2x2", DXGI_FORMAT_R32G32B32A32_FLOAT, AST_TYPE_FLAG_FLOAT, 4, 2, 2);
    InitBuiltinMatrixType("float2x3", DXGI_FORMAT_UNKNOWN,            AST_TYPE_FLAG_NONE,  4, 2, 3);
    InitBuiltinMatrixType("float2x4", DXGI_FORMAT_UNKNOWN,            AST_TYPE_FLAG_NONE,  4, 2, 4);
    InitBuiltinMatrixType("float3x1", DXGI_FORMAT_UNKNOWN,            AST_TYPE_FLAG_NONE,  4, 3, 1);
    InitBuiltinMatrixType("float3x2", DXGI_FORMAT_UNKNOWN,            AST_TYPE_FLAG_NONE,  4, 3, 2);
    InitBuiltinMatrixType("float3x3", DXGI_FORMAT_UNKNOWN,            AST_TYPE_FLAG_NONE,  4, 3, 3);
    InitBuiltinMatrixType("float3x4", DXGI_FORMAT_UNKNOWN,            AST_TYPE_FLAG_NONE,  4, 3, 4);
    InitBuiltinMatrixType("float4x1", DXGI_FORMAT_UNKNOWN,            AST_TYPE_FLAG_NONE,  4, 4, 1);
    InitBuiltinMatrixType("float4x2", DXGI_FORMAT_UNKNOWN,            AST_TYPE_FLAG_NONE,  4, 4, 2);
    InitBuiltinMatrixType("float4x3", DXGI_FORMAT_UNKNOWN,            AST_TYPE_FLAG_NONE,  4, 4, 3);
    InitBuiltinMatrixType("float4x4", DXGI_FORMAT_UNKNOWN,            AST_TYPE_FLAG_NONE,  4, 4, 4);

    InitBuiltinScalarType("double",    DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8);
    InitBuiltinVectorType("double1",   DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8, 1);
    InitBuiltinVectorType("double2",   DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8, 2);
    InitBuiltinVectorType("double3",   DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8, 3);
    InitBuiltinVectorType("double4",   DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8, 4);
    InitBuiltinMatrixType("double1x1", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8, 1, 1);
    InitBuiltinMatrixType("double1x2", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8, 1, 2);
    InitBuiltinMatrixType("double1x3", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8, 1, 3);
    InitBuiltinMatrixType("double1x4", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8, 1, 4);
    InitBuiltinMatrixType("double2x1", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8, 2, 1);
    InitBuiltinMatrixType("double2x2", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8, 2, 2);
    InitBuiltinMatrixType("double2x3", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8, 2, 3);
    InitBuiltinMatrixType("double2x4", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8, 2, 4);
    InitBuiltinMatrixType("double3x1", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8, 3, 1);
    InitBuiltinMatrixType("double3x2", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8, 3, 2);
    InitBuiltinMatrixType("double3x3", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8, 3, 3);
    InitBuiltinMatrixType("double3x4", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8, 3, 4);
    InitBuiltinMatrixType("double4x1", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8, 4, 1);
    InitBuiltinMatrixType("double4x2", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8, 4, 2);
    InitBuiltinMatrixType("double4x3", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8, 4, 3);
    InitBuiltinMatrixType("double4x4", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8, 4, 4);
}

internal void ParsePreprocessor(
    in  ShaderParser        *parser,
    out GraphicsProgramDesc *gpd)
{
    GetNextGraphicsShaderToken(&parser->lexer);
    if (TokenEqualsCSTR(parser->lexer.token, "cengine"))
    {
        GetNextGraphicsShaderToken(&parser->lexer);
        CheckToken(&parser->lexer, TOKEN_KIND_KEYWORD);
        
        if (TokenEqualsCSTR(parser->lexer.token, "pipeline"))
        {
            GetNextGraphicsShaderToken(&parser->lexer);
            CheckToken(&parser->lexer, TOKEN_KIND_LPAREN);

            GetNextGraphicsShaderToken(&parser->lexer);
            CheckToken(&parser->lexer, TOKEN_KIND_KEYWORD);

            if (TokenEqualsCSTR(parser->lexer.token, "blending"))
            {
                GetNextGraphicsShaderToken(&parser->lexer);
                CheckToken(&parser->lexer, TOKEN_KIND_COMMA);

                GetNextGraphicsShaderToken(&parser->lexer);
                CheckToken(&parser->lexer, TOKEN_KIND_KEYWORD);

                if (TokenEqualsCSTR(parser->lexer.token, "enabled"))
                {
                    gpd->psd.blending_enabled = true;
                }
                else if (TokenEqualsCSTR(parser->lexer.token, "disabled"))
                {
                    gpd->psd.blending_enabled = false;
                }
                else
                {
                    SyntaxError(parser->lexer,
                                "undefined blending option: '%s', expected: 'enabled' or 'disabled'",
                                parser->lexer.token.name);
                }
            }
            else if (TokenEqualsCSTR(parser->lexer.token, "depth_test"))
            {
                GetNextGraphicsShaderToken(&parser->lexer);
                CheckToken(&parser->lexer, TOKEN_KIND_COMMA);

                GetNextGraphicsShaderToken(&parser->lexer);
                CheckToken(&parser->lexer, TOKEN_KIND_KEYWORD);

                if (TokenEqualsCSTR(parser->lexer.token, "enabled"))
                {
                    gpd->psd.depth_test_enabled = true;
                }
                else if (TokenEqualsCSTR(parser->lexer.token, "disabled"))
                {
                    gpd->psd.depth_test_enabled = false;
                }
                else
                {
                    SyntaxError(parser->lexer,
                                "undefined depth test option: '%s', expected: 'enabled' or 'disabled'",
                                parser->lexer.token.name);
                }
            }
            else if (TokenEqualsCSTR(parser->lexer.token, "cull_mode"))
            {
                GetNextGraphicsShaderToken(&parser->lexer);
                CheckToken(&parser->lexer, TOKEN_KIND_COMMA);

                GetNextGraphicsShaderToken(&parser->lexer);
                CheckToken(&parser->lexer, TOKEN_KIND_KEYWORD);

                if (TokenEqualsCSTR(parser->lexer.token, "none"))
                {
                    gpd->psd.cull_mode = D3D12_CULL_MODE_NONE;
                }
                else if (TokenEqualsCSTR(parser->lexer.token, "front"))
                {
                    gpd->psd.cull_mode = D3D12_CULL_MODE_FRONT;
                }
                else if (TokenEqualsCSTR(parser->lexer.token, "back"))
                {
                    gpd->psd.cull_mode = D3D12_CULL_MODE_BACK;
                }
                else
                {
                    SyntaxError(parser->lexer,
                                "undefined cull mode option: '%s', expected: 'none' or 'front' or 'back'",
                                parser->lexer.token.name);
                }
            }
            else
            {
                SyntaxError(parser->lexer,
                            "undefined pipeline setting: '%s', expected: 'blending' or 'depth_test' or 'cull_mode'",
                            parser->lexer.token.name);
            }

            GetNextGraphicsShaderToken(&parser->lexer);
            CheckToken(&parser->lexer, TOKEN_KIND_RPAREN);
        }
        else if (TokenEqualsCSTR(parser->lexer.token, "shader"))
        {
            ShaderDesc *shader_desc = gpd->sd.descs + gpd->sd.count;

            GetNextGraphicsShaderToken(&parser->lexer);
            CheckToken(&parser->lexer, TOKEN_KIND_LPAREN);

            GetNextGraphicsShaderToken(&parser->lexer);
            CheckToken(&parser->lexer, TOKEN_KIND_KEYWORD);

            if (TokenEqualsCSTR(parser->lexer.token, "vertex"))
            {
                if (shader_desc->code_start)
                {
                    ++gpd->sd.count;

                    const char *shader_code_end = parser->lexer.stream;
                    while (*shader_code_end != '#') --shader_code_end;

                    shader_desc->code_end = shader_code_end;

                    shader_desc = gpd->sd.descs + gpd->sd.count;
                }

                shader_desc->name            = gpd->sd.filename;
                shader_desc->entry_point     = "VSMain";
                shader_desc->entry_point_len = CSTRLEN("VSMain");

                GetNextGraphicsShaderToken(&parser->lexer);

                if (parser->lexer.token.kind == TOKEN_KIND_COMMA)
                {
                    GetNextGraphicsShaderToken(&parser->lexer);
                    CheckToken(&parser->lexer, TOKEN_KIND_FLOAT);

                    int len = sprintf(shader_desc->target, "vs_%1.1f", parser->lexer.token.f64_val);
                    char *target_end = shader_desc->target + len;
                    for (char *it = shader_desc->target; it != target_end; ++it)
                    {
                        if (*it == '.')
                        {
                            *it = '_';
                            break;
                        }
                    }

                    GetNextGraphicsShaderToken(&parser->lexer);
                }
                else
                {
                    CopyMemory(shader_desc->target, "vs_5_0", CSTRLEN("vs_5_0"));
                }
            }
            else if (TokenEqualsCSTR(parser->lexer.token, "hull"))
            {
                if (shader_desc->code_start)
                {
                    ++gpd->sd.count;

                    const char *shader_code_end = parser->lexer.stream;
                    while (*shader_code_end != '#') --shader_code_end;

                    shader_desc->code_end = shader_code_end;

                    shader_desc = gpd->sd.descs + gpd->sd.count;
                }

                shader_desc->name            = gpd->sd.filename;
                shader_desc->entry_point     = "HSMain";
                shader_desc->entry_point_len = CSTRLEN("HSMain");

                GetNextGraphicsShaderToken(&parser->lexer);

                if (parser->lexer.token.kind == TOKEN_KIND_COMMA)
                {
                    GetNextGraphicsShaderToken(&parser->lexer);
                    CheckToken(&parser->lexer, TOKEN_KIND_FLOAT);

                    int len = sprintf(shader_desc->target, "hs_%1.1f", parser->lexer.token.f64_val);
                    char *target_end = shader_desc->target + len;
                    for (char *it = shader_desc->target; it != target_end; ++it)
                    {
                        if (*it == '.')
                        {
                            *it = '_';
                            break;
                        }
                    }

                    GetNextGraphicsShaderToken(&parser->lexer);
                }
                else
                {
                    CopyMemory(shader_desc->target, "hs_5_0", CSTRLEN("hs_5_0"));
                }
            }
            else if (TokenEqualsCSTR(parser->lexer.token, "domain"))
            {
                if (shader_desc->code_start)
                {
                    ++gpd->sd.count;

                    const char *shader_code_end = parser->lexer.stream;
                    while (*shader_code_end != '#') --shader_code_end;

                    shader_desc->code_end = shader_code_end;

                    shader_desc = gpd->sd.descs + gpd->sd.count;
                }

                shader_desc->name            = gpd->sd.filename;
                shader_desc->entry_point     = "DSMain";
                shader_desc->entry_point_len = CSTRLEN("DSMain");

                GetNextGraphicsShaderToken(&parser->lexer);

                if (parser->lexer.token.kind == TOKEN_KIND_COMMA)
                {
                    GetNextGraphicsShaderToken(&parser->lexer);
                    CheckToken(&parser->lexer, TOKEN_KIND_FLOAT);

                    int len = sprintf(shader_desc->target, "ds_%1.1f", parser->lexer.token.f64_val);
                    char *target_end = shader_desc->target + len;
                    for (char *it = shader_desc->target; it != target_end; ++it)
                    {
                        if (*it == '.')
                        {
                            *it = '_';
                            break;
                        }
                    }

                    GetNextGraphicsShaderToken(&parser->lexer);
                }
                else
                {
                    CopyMemory(shader_desc->target, "ds_5_0", CSTRLEN("ds_5_0"));
                }
            }
            else if (TokenEqualsCSTR(parser->lexer.token, "geometry"))
            {
                if (shader_desc->code_start)
                {
                    ++gpd->sd.count;

                    const char *shader_code_end = parser->lexer.stream;
                    while (*shader_code_end != '#') --shader_code_end;

                    shader_desc->code_end = shader_code_end;

                    shader_desc = gpd->sd.descs + gpd->sd.count;
                }

                shader_desc->name            = gpd->sd.filename;
                shader_desc->entry_point     = "GSMain";
                shader_desc->entry_point_len = CSTRLEN("GSMain");

                GetNextGraphicsShaderToken(&parser->lexer);

                if (parser->lexer.token.kind == TOKEN_KIND_COMMA)
                {
                    GetNextGraphicsShaderToken(&parser->lexer);
                    CheckToken(&parser->lexer, TOKEN_KIND_FLOAT);

                    int len = sprintf(shader_desc->target, "gs_%1.1f", parser->lexer.token.f64_val);
                    char *target_end = shader_desc->target + len;
                    for (char *it = shader_desc->target; it != target_end; ++it)
                    {
                        if (*it == '.')
                        {
                            *it = '_';
                            break;
                        }
                    }

                    GetNextGraphicsShaderToken(&parser->lexer);
                }
                else
                {
                    CopyMemory(shader_desc->target, "gs_5_0", CSTRLEN("gs_5_0"));
                }
            }
            else if (TokenEqualsCSTR(parser->lexer.token, "pixel"))
            {
                if (shader_desc->code_start)
                {
                    ++gpd->sd.count;

                    const char *shader_code_end = parser->lexer.stream;
                    while (*shader_code_end != '#') --shader_code_end;

                    shader_desc->code_end = shader_code_end;

                    shader_desc = gpd->sd.descs + gpd->sd.count;
                }

                shader_desc->name            = gpd->sd.filename;
                shader_desc->entry_point     = "PSMain";
                shader_desc->entry_point_len = CSTRLEN("PSMain");

                GetNextGraphicsShaderToken(&parser->lexer);

                if (parser->lexer.token.kind == TOKEN_KIND_COMMA)
                {
                    GetNextGraphicsShaderToken(&parser->lexer);
                    CheckToken(&parser->lexer, TOKEN_KIND_FLOAT);

                    int len = sprintf(shader_desc->target, "ps_%1.1f", parser->lexer.token.f64_val);
                    char *target_end = shader_desc->target + len;
                    for (char *it = shader_desc->target; it != target_end; ++it)
                    {
                        if (*it == '.')
                        {
                            *it = '_';
                            break;
                        }
                    }

                    GetNextGraphicsShaderToken(&parser->lexer);
                }
                else
                {
                    CopyMemory(shader_desc->target, "ps_5_0", CSTRLEN("ps_5_0"));
                }
            }
            else if (TokenEqualsCSTR(parser->lexer.token, "entry_point"))
            {
                GetNextGraphicsShaderToken(&parser->lexer);
                CheckToken(&parser->lexer, TOKEN_KIND_COMMA);

                GetNextGraphicsShaderToken(&parser->lexer);
                CheckToken(&parser->lexer, TOKEN_KIND_NAME);

                shader_desc->entry_point     = parser->lexer.token.name;
                shader_desc->entry_point_len = parser->lexer.token.end - parser->lexer.token.start;

                GetNextGraphicsShaderToken(&parser->lexer);
            }
            else
            {
                SyntaxError(parser->lexer,
                            "undefined shader type: '%s', expected 'vertex' or 'hull' or 'domain' or 'geometry' or 'pixel'",
                            parser->lexer.token.name);
            }

            CheckToken(&parser->lexer, TOKEN_KIND_RPAREN);

            shader_desc->code_start = parser->lexer.stream + 1;
        }
    }
}

internal u32 CountTypesInStructRecursively(
    in ASTType *struct_type)
{
    u32 count = 0;
    for (u64 i = 0; i < struct_type->_struct->fields_count; ++i)
    {
        ASTStructField *field = struct_type->_struct->fields + i;
        if (field->type->kind == AST_TYPE_KIND_STRUCT)
        {
            count += CountTypesInStructRecursively(field->type);
        }
        else
        {
            ++count;
        }
    }
    return count;
}

internal u32 CollectStructFiedlsRecursivelyForInputLayout(
    in ShaderParser             *parser,
    in ASTType                  *struct_type,
    in u32                       outer_index,
    in D3D12_INPUT_ELEMENT_DESC *desc)
{
    for (u64 i = 0; i < struct_type->_struct->fields_count; ++i)
    {
        ASTStructField *field = struct_type->_struct->fields + i;

        if (field->type->kind == AST_TYPE_KIND_STRUCT)
        {
            outer_index = CollectStructFiedlsRecursivelyForInputLayout(parser,
                                                                       field->type,
                                                                       outer_index,
                                                                       desc);
            desc += outer_index;
        }
        else
        {
            desc->InputSlot            = 0;
            desc->AlignedByteOffset    = outer_index > 0 ? D3D12_APPEND_ALIGNED_ELEMENT : 0;
            desc->InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            desc->InstanceDataStepRate = 0;

            desc->SemanticName         = field->semantic_name;
            desc->SemanticIndex        = field->semantic_index;
            desc->Format               = field->type->format;

            ++desc;
            ++outer_index;
        }
    }

    return outer_index;
}

internal void ParseInputLayout(
    Engine              *engine,
    ShaderParser        *parser,
    GraphicsProgramDesc *gpd)
{
    GetNextGraphicsShaderToken(&parser->lexer);
    CheckToken(&parser->lexer, TOKEN_KIND_LPAREN);

    GetNextGraphicsShaderToken(&parser->lexer);
    if (parser->lexer.token.kind == TOKEN_KIND_RPAREN)
    {
        gpd->psd.input_layout.pInputElementDescs = PushToTA(D3D12_INPUT_ELEMENT_DESC, engine->memory, 1);
        return;
    }

    PassModifiers(parser);

    const char *stream_first_arg     = parser->lexer.stream;
    Token       token_first_arg      = parser->lexer.token;
    const char *line_start_first_arg = parser->lexer.line_start;

    // Count args
    while (parser->lexer.token.kind != TOKEN_KIND_RPAREN
       &&  parser->lexer.token.kind != TOKEN_KIND_EOF)
    {
        PassModifiers(parser);

        b32 found_in_types = false;

        for (ASTType *it = parser->types; it; it = it->next)
        {
            if (it->name == parser->lexer.token.name)
            {
                GetNextGraphicsShaderToken(&parser->lexer);
                CheckToken(&parser->lexer, TOKEN_KIND_NAME);

                GetNextGraphicsShaderToken(&parser->lexer);
                if (parser->lexer.token.kind == TOKEN_KIND_LBRACKET)
                {
                    SyntaxError(parser->lexer, "arrays are not supported in vertex shader's entry point's args");
                }

                if (it->kind == AST_TYPE_KIND_STRUCT)
                {
                    gpd->psd.input_layout.NumElements += CountTypesInStructRecursively(it);
                }
                else
                {
                    ++gpd->psd.input_layout.NumElements;
                }

                found_in_types = true;
                break;
            }
        }

        if (!found_in_types)
        {
            SyntaxError(parser->lexer, "undefined type: '%s'", parser->lexer.token.name);
        }

        while (parser->lexer.token.kind != TOKEN_KIND_COMMA
           &&  parser->lexer.token.kind != TOKEN_KIND_RPAREN
           &&  parser->lexer.token.kind != TOKEN_KIND_EOF)
        {
            GetNextGraphicsShaderToken(&parser->lexer);
        }

        if (parser->lexer.token.kind != TOKEN_KIND_RPAREN
        &&  parser->lexer.token.kind != TOKEN_KIND_EOF)
        {
            GetNextGraphicsShaderToken(&parser->lexer);
        }
    }

    if (parser->lexer.token.kind == TOKEN_KIND_EOF)
    {
        SyntaxError(parser->lexer, "unexpected end of file in vertex shader's entry point's arguments");
    }

    parser->lexer.stream     = stream_first_arg;
    parser->lexer.token      = token_first_arg;
    parser->lexer.line_start = line_start_first_arg;

    // D3D12_INPUT_LAYOUT_DESC filling
    gpd->psd.input_layout.pInputElementDescs = PushToTA(D3D12_INPUT_ELEMENT_DESC, engine->memory, gpd->psd.input_layout.NumElements);

    for (u32 i = 0; i < gpd->psd.input_layout.NumElements; ++i)
    {
        PassModifiers(parser);

        #pragma warning(suppress: 4090) // different const qualifiers
        D3D12_INPUT_ELEMENT_DESC *desc = gpd->psd.input_layout.pInputElementDescs + i;

        ASTType *type = null;
        for (ASTType *it = parser->types; it; it = it->next)
        {
            if (it->name == parser->lexer.token.name)
            {
                type = it;
                break;
            }
        }
        if (!type)
        {
            SyntaxError(parser->lexer, "undefined type: '%s'", parser->lexer.token.name);
        }

        if (type->kind == AST_TYPE_KIND_STRUCT)
        {
            // @NOTE(Roman): -1 cuz we have ++i in for loop;
            i = CollectStructFiedlsRecursivelyForInputLayout(parser, type, i, desc) - 1;

            GetNextGraphicsShaderToken(&parser->lexer);
            CheckToken(&parser->lexer, TOKEN_KIND_NAME);
        }
        else
        {
            desc->InputSlot            = 0;
            desc->AlignedByteOffset    = D3D12_APPEND_ALIGNED_ELEMENT;
            desc->InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            desc->InstanceDataStepRate = 0;

            GetNextGraphicsShaderToken(&parser->lexer);
            CheckToken(&parser->lexer, TOKEN_KIND_NAME);

            GetNextGraphicsShaderToken(&parser->lexer);
            CheckToken(&parser->lexer, TOKEN_KIND_COLON);

            GetNextGraphicsShaderToken(&parser->lexer);
            CheckToken(&parser->lexer, TOKEN_KIND_NAME);

            const char *index_start = parser->lexer.token.end;
            while (isdigit(index_start[-1])) --index_start;

            desc->SemanticName  = InternStringRange(&parser->lexer.interns, parser->lexer.token.start, index_start);
            desc->SemanticIndex = strtoul(index_start, null, 10);
            desc->Format        = type->format;
        }

        GetNextGraphicsShaderToken(&parser->lexer);
        if (parser->lexer.token.kind == TOKEN_KIND_COMMA)
        {
            GetNextGraphicsShaderToken(&parser->lexer);
        }
        else
        {
            CheckToken(&parser->lexer, TOKEN_KIND_RPAREN);
        }
    }
}

internal void ParseStruct(
    in     Engine       *engine,
    in out ShaderParser *parser)
{
    // struct name
    // {
    //     [modifiers...] type[RxC] name[index] [: semantic] [= expr];
    //     ...
    // };

    ASTStruct *_struct     = PushToTA(ASTStruct, engine->memory, 1);
    u64        struct_size = 0;

    GetNextGraphicsShaderToken(&parser->lexer);
    CheckToken(&parser->lexer, TOKEN_KIND_NAME);
    
    _struct->name = parser->lexer.token.name;

    // @NOTE(Roman): Incomplete types are not alowed in HLSL.
    GetNextGraphicsShaderToken(&parser->lexer);
    CheckToken(&parser->lexer, TOKEN_KIND_LBRACE);

    const char *stream_lbrace     = parser->lexer.stream;
    Token       token_lbrace      = parser->lexer.token;
    const char *line_start_lbrace = parser->lexer.line_start;

    // Count, how many fields we have.
    while (parser->lexer.token.kind != TOKEN_KIND_RBRACE
       &&  parser->lexer.token.kind != TOKEN_KIND_EOF)
    {
        if (parser->lexer.token.kind == TOKEN_KIND_SEMICOLON)
        {
            ++_struct->fields_count;
        }
        GetNextGraphicsShaderToken(&parser->lexer);
    }
    if (parser->lexer.token.kind == TOKEN_KIND_EOF)
    {
        SyntaxError(parser->lexer, "unexpected end of file");
    }

    _struct->fields = PushToTA(ASTStructField, engine->memory, _struct->fields_count);
    {
        ASTStructField *it = _struct->fields;
        for (u64 i = 1; i < _struct->fields_count; ++i)
        {
            it->next = _struct->fields + i;
            it       = it->next;
        }
    }

    parser->lexer.stream     = stream_lbrace;
    parser->lexer.token      = token_lbrace;
    parser->lexer.line_start = line_start_lbrace;

    GetNextGraphicsShaderToken(&parser->lexer);
    CheckToken(&parser->lexer, TOKEN_KIND_NAME);

    for (ASTStructField *field = _struct->fields; field; field = field->next)
    {
        // [modifiers...]
        PassModifiers(parser);

        // Type[RxC]
        CheckToken(&parser->lexer, TOKEN_KIND_NAME);
        for (ASTType *it = parser->types; it; it = it->next)
        {
            if (it->name == parser->lexer.token.name)
            {
                field->type = it;
                break;
            }
        }
        if (!field->type)
        {
            SyntaxError(parser->lexer, "undefined type: '%s'", parser->lexer.token.name);
        }

        GetNextGraphicsShaderToken(&parser->lexer);
        CheckToken(&parser->lexer, TOKEN_KIND_NAME);

        GetNextGraphicsShaderToken(&parser->lexer);

        // Array
        if (parser->lexer.token.kind == TOKEN_KIND_LBRACKET)
        {
            GetNextGraphicsShaderToken(&parser->lexer);
            CheckToken(&parser->lexer, TOKEN_KIND_INT);

            struct_size += field->type->size_in_bytes * parser->lexer.token.u64_val;

            GetNextGraphicsShaderToken(&parser->lexer);
            CheckToken(&parser->lexer, TOKEN_KIND_RBRACKET);

            GetNextGraphicsShaderToken(&parser->lexer);
            if (parser->lexer.token.kind == TOKEN_KIND_COLON)
            {
                SyntaxError(parser->lexer, "arrays with semantics are not supported");
            }
        }
        else
        {
            struct_size += field->type->size_in_bytes;
        }

        // Semantic
        if (parser->lexer.token.kind == TOKEN_KIND_COLON)
        {
            GetNextGraphicsShaderToken(&parser->lexer);
            CheckToken(&parser->lexer, TOKEN_KIND_NAME);

            const char *index_start = parser->lexer.token.end;
            while (isdigit(index_start[-1])) --index_start;

            field->semantic_index = strtoul(index_start, null, 10);
            field->semantic_name  = InternStringRange(&parser->lexer.interns, parser->lexer.token.start, index_start);
        }

        // Pass other stuff we don't care about
        while (parser->lexer.token.kind != TOKEN_KIND_SEMICOLON
           &&  parser->lexer.token.kind != TOKEN_KIND_EOF)
        {
            GetNextGraphicsShaderToken(&parser->lexer);    
        }
        if (parser->lexer.token.kind == TOKEN_KIND_EOF)
        {
            SyntaxError(parser->lexer, "unexpected end of file");
        }

        GetNextGraphicsShaderToken(&parser->lexer);
    }

    CheckToken(&parser->lexer, TOKEN_KIND_RBRACE);

    GetNextGraphicsShaderToken(&parser->lexer);
    CheckToken(&parser->lexer, TOKEN_KIND_SEMICOLON);

    // Done
    ASTType *it = parser->types;
    while (it->next) it = it->next;
    it->next                = PushToTA(ASTType, engine->memory, 1);
    it->next->next          = null;
    it->next->base_type     = null;
    it->next->name          = _struct->name;
    it->next->kind          = AST_TYPE_KIND_STRUCT;
    it->next->format        = DXGI_FORMAT_UNKNOWN;
    it->next->flag          = AST_TYPE_FLAG_NONE;
    it->next->builtin       = false;
    it->next->size_in_bytes = struct_size;
    it->next->_struct       = _struct;
}

internal void ParseTypedef(
    in     Engine       *engine,
    in out ShaderParser *parser)
{
    // typedef [const] type name ['['index']'];

    ASTType *type = PushToTA(ASTType, engine->memory, 1);

    GetNextGraphicsShaderToken(&parser->lexer);
    if (TokenEqualsCSTR(parser->lexer.token, "const"))
    {
        GetNextGraphicsShaderToken(&parser->lexer);
    }
    CheckToken(&parser->lexer, TOKEN_KIND_NAME);

    ASTType *base_type = null;
    for (ASTType *it = parser->types; it; it = it->next)
    {
        if (it->name == parser->lexer.token.name)
        {
            base_type = it;
            break;
        }
    }
    if (!base_type) SyntaxError(parser->lexer, "unknown type: '%s'", parser->lexer.token.name);

    type->base_type = base_type;
    type->flag      = base_type->flag;

    GetNextGraphicsShaderToken(&parser->lexer);
    CheckToken(&parser->lexer, TOKEN_KIND_NAME);

    type->name = parser->lexer.token.name;

    GetNextGraphicsShaderToken(&parser->lexer);
    if (parser->lexer.token.kind == TOKEN_KIND_LBRACKET)
    {
        type->kind = AST_TYPE_KIND_ARRAY;

        GetNextGraphicsShaderToken(&parser->lexer);
        CheckToken(&parser->lexer, TOKEN_KIND_INT);

        type->dimension     = cast(u32, parser->lexer.token.u64_val);
        type->size_in_bytes = type->dimension * base_type->size_in_bytes;

        if (type->size_in_bytes <= 16)
        {
            switch (type->flag)
            {
                case AST_TYPE_FLAG_SINT:
                {
                    /**/ if (type->size_in_bytes ==  4) type->format = DXGI_FORMAT_R32_SINT;
                    else if (type->size_in_bytes ==  8) type->format = DXGI_FORMAT_R32G32_SINT;
                    else if (type->size_in_bytes == 12) type->format = DXGI_FORMAT_R32G32B32_SINT;
                    else if (type->size_in_bytes == 16) type->format = DXGI_FORMAT_R32G32B32A32_SINT;
                    else                                type->format = DXGI_FORMAT_UNKNOWN;
                } break;

                case AST_TYPE_FLAG_UINT:
                {
                    /**/ if (type->size_in_bytes ==  4) type->format = DXGI_FORMAT_R32_UINT;
                    else if (type->size_in_bytes ==  8) type->format = DXGI_FORMAT_R32G32_UINT;
                    else if (type->size_in_bytes == 12) type->format = DXGI_FORMAT_R32G32B32_UINT;
                    else if (type->size_in_bytes == 16) type->format = DXGI_FORMAT_R32G32B32A32_UINT;
                    else                                type->format = DXGI_FORMAT_UNKNOWN;
                } break;

                case AST_TYPE_FLAG_HALF:
                {
                    /**/ if (type->size_in_bytes == 2) type->format = DXGI_FORMAT_R16_FLOAT;
                    else if (type->size_in_bytes == 4) type->format = DXGI_FORMAT_R16G16_FLOAT;
                    else if (type->size_in_bytes == 6) type->format = DXGI_FORMAT_UNKNOWN;
                    else if (type->size_in_bytes == 8) type->format = DXGI_FORMAT_R16G16B16A16_FLOAT;
                    else                               type->format = DXGI_FORMAT_UNKNOWN;
                } break;

                case AST_TYPE_FLAG_FLOAT:
                {
                    /**/ if (type->size_in_bytes ==  4) type->format = DXGI_FORMAT_R32_FLOAT;
                    else if (type->size_in_bytes ==  8) type->format = DXGI_FORMAT_R32G32_FLOAT;
                    else if (type->size_in_bytes == 12) type->format = DXGI_FORMAT_R32G32B32_FLOAT;
                    else if (type->size_in_bytes == 16) type->format = DXGI_FORMAT_R32G32B32A32_FLOAT;
                    else                                type->format = DXGI_FORMAT_UNKNOWN;
                } break;

                default:
                {
                    type->format = DXGI_FORMAT_UNKNOWN;
                } break;
            }
        }
        else
        {
            type->format = DXGI_FORMAT_UNKNOWN;
        }

        GetNextGraphicsShaderToken(&parser->lexer);
        CheckToken(&parser->lexer, TOKEN_KIND_RBRACKET);

        GetNextGraphicsShaderToken(&parser->lexer);
    }
    else // not an array
    {
        type->kind          = base_type->kind;
        type->format        = base_type->format;
        type->size_in_bytes = base_type->size_in_bytes;
        switch (type->kind)
        {
            case AST_TYPE_KIND_VECTOR:
            {
                type->dimension = base_type->dimension;
            } break;
            
            case AST_TYPE_KIND_MATRIX:
            {
                type->mat_size = base_type->mat_size;
            } break;

            case AST_TYPE_KIND_STRUCT:
            {
                type->_struct = base_type->_struct;
            } break;
        }
    }
    CheckToken(&parser->lexer, TOKEN_KIND_SEMICOLON);

    ASTType *it = parser->types;
    while (it->next) it = it->next;
    it->next = type;
}

internal void ParseCBuffer(
    in     Engine              *engine,
    in out ShaderParser        *parser,
    in out GraphicsProgramDesc *gpd)
{
    // cbuffer name [: register(b# [, space#])]
    // {
    //     ...
    // };

    ASTCBuffer *cbuffer = PushToTA(ASTCBuffer, engine->memory, 1);

    GetNextGraphicsShaderToken(&parser->lexer);
    CheckToken(&parser->lexer, TOKEN_KIND_NAME);
    
    cbuffer->name_len = parser->lexer.token.end - parser->lexer.token.start;
    cbuffer->name     = parser->lexer.token.name;

    GetNextGraphicsShaderToken(&parser->lexer);

    // [: register(b# [, space#])]
    if (parser->lexer.token.kind == TOKEN_KIND_COLON)
    {
        GetNextGraphicsShaderToken(&parser->lexer);
        CheckToken(&parser->lexer, TOKEN_KIND_KEYWORD);

        GetNextGraphicsShaderToken(&parser->lexer);
        CheckToken(&parser->lexer, TOKEN_KIND_LPAREN);

        GetNextGraphicsShaderToken(&parser->lexer);
        CheckToken(&parser->lexer, TOKEN_KIND_NAME);

        const char *register_val = parser->lexer.token.start;
        if (*register_val != 'b')
        {
            SyntaxError(parser->lexer,
                        "invaid register: '%s'. The only valid register for cbuffer is b#",
                        parser->lexer.token.name);
        }
        ++register_val;

        cbuffer->_register = strtoul(register_val, null, 10);

        GetNextGraphicsShaderToken(&parser->lexer);
        if (parser->lexer.token.kind == TOKEN_KIND_COMMA)
        {
            GetNextGraphicsShaderToken(&parser->lexer);
            CheckToken(&parser->lexer, TOKEN_KIND_NAME);

            // @NOTE(Roman): Not TokenEquals cuz token.end - token.start > CSTRLEN("space")
            if (!RtlEqualMemory(parser->lexer.token.start, "space", CSTRLEN("space")))
            {
                SyntaxError(parser->lexer,
                            "expected 'space', got: '%.s'",
                            parser->lexer.token.name);
            }

            const char *space_val = parser->lexer.token.start + CSTRLEN("space");
            cbuffer->space        = strtoul(space_val, null, 10);

            GetNextGraphicsShaderToken(&parser->lexer);
        }
        CheckToken(&parser->lexer, TOKEN_KIND_RPAREN);

        GetNextGraphicsShaderToken(&parser->lexer);
    }
    CheckToken(&parser->lexer, TOKEN_KIND_LBRACE);

    GetNextGraphicsShaderToken(&parser->lexer);
    while (parser->lexer.token.kind != TOKEN_KIND_RBRACE
       &&  parser->lexer.token.kind != TOKEN_KIND_EOF)
    {
        PassModifiers(parser);
        CheckToken(&parser->lexer, TOKEN_KIND_NAME);

        // @TODO(Roman): We're only parsing structs, typedefs and built-in types.
        //               Add arrays parsing.

        b32 type_found = false;
        for (ASTType *it = parser->types; it; it = it->next)
        {
            if (it->name == parser->lexer.token.name)
            {
                cbuffer->size_in_bytes += cast(u32, it->size_in_bytes);
                type_found = true;
                break;
            }
        }

        if (!type_found)
        {
            SyntaxError(parser->lexer,
                        "undefined type: '%s'",
                        parser->lexer.token.name);
        }

        while (parser->lexer.token.kind != TOKEN_KIND_SEMICOLON
           &&  parser->lexer.token.kind != TOKEN_KIND_RBRACE
           &&  parser->lexer.token.kind != TOKEN_KIND_EOF)
        {
            GetNextGraphicsShaderToken(&parser->lexer);
        }

        if (parser->lexer.token.kind == TOKEN_KIND_SEMICOLON)
        {
            GetNextGraphicsShaderToken(&parser->lexer);
        }
    }
    if (parser->lexer.token.kind == TOKEN_KIND_EOF)
    {
        SyntaxError(parser->lexer, "unexpected end of file");
    }

    GetNextGraphicsShaderToken(&parser->lexer);
    CheckToken(&parser->lexer, TOKEN_KIND_SEMICOLON);

    // Done
    if (!parser->cbuffers)
    {
        parser->cbuffers = cbuffer;
    }
    else
    {
        ASTCBuffer *it = parser->cbuffers;
        while (it->next) it = it->next;
        it->next = cbuffer;
    }

    if (gpd->rsd.Version == D3D_ROOT_SIGNATURE_VERSION_1_0)
        ++gpd->rsd.Desc_1_0.NumParameters;
    else
        ++gpd->rsd.Desc_1_1.NumParameters;
}

internal void InitRootSignatureParameters(
    in     Engine              *engine,
    in     ShaderParser        *parser,
    in out GraphicsProgram     *program,
    out    GraphicsProgramDesc *gpd)
{
    if (gpd->rsd.Version == D3D_ROOT_SIGNATURE_VERSION_1_0)
    {
        gpd->rsd.Desc_1_0.pParameters = PushToTA(D3D12_ROOT_PARAMETER, engine->memory, gpd->rsd.Desc_1_0.NumParameters);

        #pragma warning(suppress: 4090) // different const qualifiers
        D3D12_ROOT_PARAMETER *parameter = gpd->rsd.Desc_1_0.pParameters;

        for (ASTCBuffer *cbuffer = parser->cbuffers;
             cbuffer;
             cbuffer = cbuffer->next, ++parameter)
        {
            parameter->ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
            parameter->Descriptor.ShaderRegister = cbuffer->_register;
            parameter->Descriptor.RegisterSpace  = cbuffer->space;
            parameter->ShaderVisibility          = D3D12_SHADER_VISIBILITY_ALL;

            cbuffer->size_in_bytes = ALIGN_UP(cbuffer->size_in_bytes, 256);
            GPUResource *resource  = PushConstantBuffer(engine, cbuffer->size_in_bytes, cbuffer->name, cast(u32, cbuffer->name_len));
            BufferPushBack(program->resources, resource);
        }

        // @TODO(Roman): Init SRVs and UAVs
    }
    else
    {
        gpd->rsd.Desc_1_1.pParameters = PushToTA(D3D12_ROOT_PARAMETER1, engine->memory, gpd->rsd.Desc_1_1.NumParameters);
        
        #pragma warning(suppress: 4090) // different const qualifiers
        D3D12_ROOT_PARAMETER1 *parameter = gpd->rsd.Desc_1_1.pParameters;

        for (ASTCBuffer *cbuffer = parser->cbuffers;
             cbuffer;
             cbuffer = cbuffer->next, ++parameter)
        {
            parameter->ParameterType             = D3D12_ROOT_PARAMETER_TYPE_CBV;
            parameter->Descriptor.ShaderRegister = cbuffer->_register;
            parameter->Descriptor.RegisterSpace  = cbuffer->space;
            parameter->Descriptor.Flags          = cbuffer->flags;
            parameter->ShaderVisibility          = D3D12_SHADER_VISIBILITY_ALL;

            cbuffer->size_in_bytes = ALIGN_UP(cbuffer->size_in_bytes, 256);
            GPUResource *resource = PushConstantBuffer(engine, cbuffer->size_in_bytes, cbuffer->name, cast(u32, cbuffer->name_len));
            BufferPushBack(program->resources, resource);
        }

        // @TODO(Roman): Init SRVs and UAVs
    }
}

void ParseGraphicsShaders(
    in  Engine              *engine,
    in  const char          *file_with_shaders,
    out GraphicsProgram     *program,
    out GraphicsProgramDesc *gpd)
{
    char *shader_code = ReadEntireShaderFile(engine, file_with_shaders, null);
    gpd->sd.filename = file_with_shaders;

    ShaderParser parser = {0};
    CreateGraphicsShaderLexer(engine, shader_code, &parser.lexer);
    InitBuiltinTypes(engine, &parser);

    while (parser.lexer.token.kind != TOKEN_KIND_EOF)
    {
        switch (parser.lexer.token.kind)
        {
            case TOKEN_KIND_HASH:
            {
                ParsePreprocessor(&parser, gpd);
            } break;

            case TOKEN_KIND_NAME:
            {
                ShaderDesc *shader_desc = gpd->sd.descs + gpd->sd.count;
                if (shader_desc->target
                &&  *shader_desc->target == 'v' // @NOTE(Roman): Hack for not to use RtlEqualMemory
                                                //               for checking is it a vertex shader.
                &&  TokenEquals(parser.lexer.token, shader_desc->entry_point, shader_desc->entry_point_len))
                {
                    ParseInputLayout(engine, &parser, gpd);
                }
            } break;

            case TOKEN_KIND_KEYWORD:
            {
                if (TokenEqualsCSTR(parser.lexer.token, "struct"))
                {
                    ParseStruct(engine, &parser);
                }
                else if (TokenEqualsCSTR(parser.lexer.token, "typedef"))
                {
                    ParseTypedef(engine, &parser);
                }
                else if (TokenEqualsCSTR(parser.lexer.token, "cbuffer"))
                {
                    ParseCBuffer(engine, &parser, gpd);
                }
                // @TODO(Roman): parse SRV, UAV, ...
            } break;
        };

        GetNextGraphicsShaderToken(&parser.lexer);
    }

    ShaderDesc *last_shader = gpd->sd.descs + gpd->sd.count;
    if (last_shader->code_start)
    {
        if (!last_shader->code_end)
        {
            last_shader->code_end = parser.lexer.stream;
        }
        ++gpd->sd.count;
    }
    program->shaders_count = gpd->sd.count;

    InitRootSignatureParameters(engine, &parser, program, gpd);
}
