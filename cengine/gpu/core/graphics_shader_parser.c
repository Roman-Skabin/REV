//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "gpu/core/shader_parser.h"
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

internal char *ReadEntireShaderFile(Engine *engine, const char *filename, u32 *size)
{
    if (filename)
    {
        HANDLE file = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, 0);
        CheckM(file != INVALID_HANDLE_VALUE, "File does not exist");

        u32   filesize = GetFileSize(file, 0);
        char *buffer   = PushToTransientArea(engine->memory, filesize + 1);

        DebugResult(ReadFile(file, buffer, filesize, 0, 0));

        DebugResult(CloseHandle(file));

        if (size) *size = filesize;
        return buffer;
    }
    if (size) *size = 0;
    return 0;
}

#define InitBuilinScalarType(_name, _format, _flag, _size_in_bytes)      \
{                                                                        \
    cur_type->next          = PushToTA(ASTType, engine->memory, 1);      \
    cur_type->base_type     = 0;                                         \
    cur_type->name          = InternCSTR(&parser->lexer.interns, _name); \
    cur_type->kind          = AST_TYPE_KIND_SCALAR;                      \
    cur_type->format        = _format;                                   \
    cur_type->flag          = _flag;                                     \
    cur_type->builtin       = true;                                      \
    cur_type->size_in_bytes = _size_in_bytes;                            \
    cur_type                = cur_type->next;                            \
}

#define InitBuilinVectorType(_name, _format, _flag, _size_in_bytes, _dimension) \
{                                                                               \
    cur_type->next          = PushToTA(ASTType, engine->memory, 1);             \
    cur_type->base_type     = 0;                                                \
    cur_type->name          = InternCSTR(&parser->lexer.interns, _name);        \
    cur_type->kind          = AST_TYPE_KIND_VECTOR;                             \
    cur_type->format        = _format;                                          \
    cur_type->flag          = _flag;                                            \
    cur_type->builtin       = true;                                             \
    cur_type->size_in_bytes = _size_in_bytes;                                   \
    cur_type->dimension     = _dimension;                                       \
    cur_type                = cur_type->next;                                   \
}

#define InitBuilinMatrixType(_name, _format, _flag, _size_in_bytes, _rows, _cols) \
{                                                                                 \
    cur_type->next          = PushToTA(ASTType, engine->memory, 1);               \
    cur_type->base_type     = 0;                                                  \
    cur_type->name          = InternCSTR(&parser->lexer.interns, _name);          \
    cur_type->kind          = AST_TYPE_KIND_MATRIX;                               \
    cur_type->format        = _format;                                            \
    cur_type->flag          = _flag;                                              \
    cur_type->builtin       = true;                                               \
    cur_type->size_in_bytes = _size_in_bytes;                                     \
    cur_type->mat_size.r    = _rows;                                              \
    cur_type->mat_size.c    = _cols;                                              \
    cur_type                = cur_type->next;                                     \
}

internal void InitBuiltinTypes(Engine *engine, ShaderParser *parser)
{
    // @NOTE(Roman): DXGI has no enum constants for every HLSL builtin type.

    parser->types     = PushToTA(ASTType, engine->memory, 1);
    ASTType *cur_type = parser->types;

    InitBuilinScalarType("bool", DXGI_FORMAT_R32_UINT, AST_TYPE_FLAG_UINT, 4);
    InitBuilinVectorType("bool1", DXGI_FORMAT_R32_UINT, AST_TYPE_FLAG_UINT, 4, 1);
    InitBuilinVectorType("bool2", DXGI_FORMAT_R32G32_UINT, AST_TYPE_FLAG_UINT, 8, 2);
    InitBuilinVectorType("bool3", DXGI_FORMAT_R32G32B32_UINT, AST_TYPE_FLAG_UINT, 12, 3);
    InitBuilinVectorType("bool4", DXGI_FORMAT_R32G32B32A32_UINT, AST_TYPE_FLAG_UINT, 16, 4);
    InitBuilinMatrixType("bool1x1", DXGI_FORMAT_R32_UINT, AST_TYPE_FLAG_UINT, 4, 1, 1);
    InitBuilinMatrixType("bool1x2", DXGI_FORMAT_R32G32_UINT, AST_TYPE_FLAG_UINT, 8, 1, 2);
    InitBuilinMatrixType("bool1x3", DXGI_FORMAT_R32G32B32_UINT, AST_TYPE_FLAG_UINT, 12, 1, 3);
    InitBuilinMatrixType("bool1x4", DXGI_FORMAT_R32G32B32A32_UINT, AST_TYPE_FLAG_UINT, 16, 1, 4);
    InitBuilinMatrixType("bool2x1", DXGI_FORMAT_R32G32_UINT, AST_TYPE_FLAG_UINT, 8, 2, 1);
    InitBuilinMatrixType("bool2x2", DXGI_FORMAT_R32G32B32A32_UINT, AST_TYPE_FLAG_UINT, 16, 2, 2);
    InitBuilinMatrixType("bool2x3", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*2*3, 2, 3);
    InitBuilinMatrixType("bool2x4", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*2*4, 2, 4);
    InitBuilinMatrixType("bool3x1", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*3*1, 3, 1);
    InitBuilinMatrixType("bool3x2", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*3*2, 3, 2);
    InitBuilinMatrixType("bool3x3", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*3*3, 3, 3);
    InitBuilinMatrixType("bool3x4", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*3*4, 3, 4);
    InitBuilinMatrixType("bool4x1", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*4*1, 4, 1);
    InitBuilinMatrixType("bool4x2", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*4*2, 4, 2);
    InitBuilinMatrixType("bool4x3", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*4*3, 4, 3);
    InitBuilinMatrixType("bool4x4", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*4*4, 4, 4);

    InitBuilinScalarType("int", DXGI_FORMAT_R32_SINT, AST_TYPE_FLAG_SINT, 4);
    InitBuilinVectorType("int1", DXGI_FORMAT_R32_SINT, AST_TYPE_FLAG_SINT, 4, 1);
    InitBuilinVectorType("int2", DXGI_FORMAT_R32G32_SINT, AST_TYPE_FLAG_SINT, 8, 2);
    InitBuilinVectorType("int3", DXGI_FORMAT_R32G32B32_SINT, AST_TYPE_FLAG_SINT, 12, 3);
    InitBuilinVectorType("int4", DXGI_FORMAT_R32G32B32A32_SINT, AST_TYPE_FLAG_SINT, 16, 4);
    InitBuilinMatrixType("int1x1", DXGI_FORMAT_R32_SINT, AST_TYPE_FLAG_SINT, 4, 1, 1);
    InitBuilinMatrixType("int1x2", DXGI_FORMAT_R32G32_SINT, AST_TYPE_FLAG_SINT, 8, 1, 2);
    InitBuilinMatrixType("int1x3", DXGI_FORMAT_R32G32B32_SINT, AST_TYPE_FLAG_SINT, 12, 1, 3);
    InitBuilinMatrixType("int1x4", DXGI_FORMAT_R32G32B32A32_SINT, AST_TYPE_FLAG_SINT, 16, 1, 4);
    InitBuilinMatrixType("int2x1", DXGI_FORMAT_R32G32_SINT, AST_TYPE_FLAG_SINT, 8, 2, 1);
    InitBuilinMatrixType("int2x2", DXGI_FORMAT_R32G32B32A32_SINT, AST_TYPE_FLAG_SINT, 16, 2, 2);
    InitBuilinMatrixType("int2x3", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*2*3, 2, 3);
    InitBuilinMatrixType("int2x4", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*2*4, 2, 4);
    InitBuilinMatrixType("int3x1", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*3*1, 3, 1);
    InitBuilinMatrixType("int3x2", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*3*2, 3, 2);
    InitBuilinMatrixType("int3x3", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*3*3, 3, 3);
    InitBuilinMatrixType("int3x4", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*3*4, 3, 4);
    InitBuilinMatrixType("int4x1", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*4*1, 4, 1);
    InitBuilinMatrixType("int4x2", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*4*2, 4, 2);
    InitBuilinMatrixType("int4x3", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*4*3, 4, 3);
    InitBuilinMatrixType("int4x4", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*4*4, 4, 4);

    InitBuilinScalarType("uint", DXGI_FORMAT_R32_UINT, AST_TYPE_FLAG_UINT, 4);
    InitBuilinVectorType("uint1", DXGI_FORMAT_R32_UINT, AST_TYPE_FLAG_UINT, 4, 1);
    InitBuilinVectorType("uint2", DXGI_FORMAT_R32G32_UINT, AST_TYPE_FLAG_UINT, 8, 2);
    InitBuilinVectorType("uint3", DXGI_FORMAT_R32G32B32_UINT, AST_TYPE_FLAG_UINT, 12, 3);
    InitBuilinVectorType("uint4", DXGI_FORMAT_R32G32B32A32_UINT, AST_TYPE_FLAG_UINT, 16, 4);
    InitBuilinMatrixType("uint1x1", DXGI_FORMAT_R32_UINT, AST_TYPE_FLAG_UINT, 4, 1, 1);
    InitBuilinMatrixType("uint1x2", DXGI_FORMAT_R32G32_UINT, AST_TYPE_FLAG_UINT, 8, 1, 2);
    InitBuilinMatrixType("uint1x3", DXGI_FORMAT_R32G32B32_UINT, AST_TYPE_FLAG_UINT, 12, 1, 3);
    InitBuilinMatrixType("uint1x4", DXGI_FORMAT_R32G32B32A32_UINT, AST_TYPE_FLAG_UINT, 16, 1, 4);
    InitBuilinMatrixType("uint2x1", DXGI_FORMAT_R32G32_UINT, AST_TYPE_FLAG_UINT, 8, 2, 1);
    InitBuilinMatrixType("uint2x2", DXGI_FORMAT_R32G32B32A32_UINT, AST_TYPE_FLAG_UINT, 16, 2, 2);
    InitBuilinMatrixType("uint2x3", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*2*3, 2, 3);
    InitBuilinMatrixType("uint2x4", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*2*4, 2, 4);
    InitBuilinMatrixType("uint3x1", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*3*1, 3, 1);
    InitBuilinMatrixType("uint3x2", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*3*2, 3, 2);
    InitBuilinMatrixType("uint3x3", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*3*3, 3, 3);
    InitBuilinMatrixType("uint3x4", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*3*4, 3, 4);
    InitBuilinMatrixType("uint4x1", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*4*1, 4, 1);
    InitBuilinMatrixType("uint4x2", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*4*2, 4, 2);
    InitBuilinMatrixType("uint4x3", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*4*3, 4, 3);
    InitBuilinMatrixType("uint4x4", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*4*4, 4, 4);
    
    InitBuilinScalarType("dword", DXGI_FORMAT_R32_UINT, AST_TYPE_FLAG_UINT, 4);

    InitBuilinScalarType("half", DXGI_FORMAT_R16_FLOAT, AST_TYPE_FLAG_HALF, 2);
    InitBuilinVectorType("half1", DXGI_FORMAT_R16_FLOAT, AST_TYPE_FLAG_HALF, 2, 1);
    InitBuilinVectorType("half2", DXGI_FORMAT_R16G16_FLOAT, AST_TYPE_FLAG_HALF, 4, 2);
    InitBuilinVectorType("half3", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_HALF, 6, 3);
    InitBuilinVectorType("half4", DXGI_FORMAT_R16G16B16A16_FLOAT, AST_TYPE_FLAG_HALF, 8, 4);
    InitBuilinMatrixType("half1x1", DXGI_FORMAT_R16_FLOAT, AST_TYPE_FLAG_HALF, 2, 1, 1);
    InitBuilinMatrixType("half1x2", DXGI_FORMAT_R16G16_FLOAT, AST_TYPE_FLAG_HALF, 4, 1, 2);
    InitBuilinMatrixType("half1x3", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_HALF, 6, 1, 3);
    InitBuilinMatrixType("half1x4", DXGI_FORMAT_R16G16B16A16_FLOAT, AST_TYPE_FLAG_HALF, 8, 1, 4);
    InitBuilinMatrixType("half2x1", DXGI_FORMAT_R16G16_FLOAT, AST_TYPE_FLAG_HALF, 4, 2, 1);
    InitBuilinMatrixType("half2x2", DXGI_FORMAT_R16G16B16A16_FLOAT, AST_TYPE_FLAG_HALF, 8, 2, 2);
    InitBuilinMatrixType("half2x3", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 2*2*3, 2, 3);
    InitBuilinMatrixType("half2x4", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 2*2*4, 2, 4);
    InitBuilinMatrixType("half3x1", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 2*3*1, 3, 1);
    InitBuilinMatrixType("half3x2", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 2*3*2, 3, 2);
    InitBuilinMatrixType("half3x3", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 2*3*3, 3, 3);
    InitBuilinMatrixType("half3x4", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 2*3*4, 3, 4);
    InitBuilinMatrixType("half4x1", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 2*4*1, 4, 1);
    InitBuilinMatrixType("half4x2", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 2*4*2, 4, 2);
    InitBuilinMatrixType("half4x3", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 2*4*3, 4, 3);
    InitBuilinMatrixType("half4x4", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 2*4*4, 4, 4);

    InitBuilinScalarType("float", DXGI_FORMAT_R32_FLOAT, AST_TYPE_FLAG_FLOAT, 4);
    InitBuilinVectorType("float1", DXGI_FORMAT_R32_FLOAT, AST_TYPE_FLAG_FLOAT, 4, 1);
    InitBuilinVectorType("float2", DXGI_FORMAT_R32G32_FLOAT, AST_TYPE_FLAG_FLOAT, 8, 2);
    InitBuilinVectorType("float3", DXGI_FORMAT_R32G32B32_FLOAT, AST_TYPE_FLAG_FLOAT, 12, 3);
    InitBuilinVectorType("float4", DXGI_FORMAT_R32G32B32A32_FLOAT, AST_TYPE_FLAG_FLOAT, 16, 4);
    InitBuilinMatrixType("float1x1", DXGI_FORMAT_R32_FLOAT, AST_TYPE_FLAG_FLOAT, 4, 1, 1);
    InitBuilinMatrixType("float1x2", DXGI_FORMAT_R32G32_FLOAT, AST_TYPE_FLAG_FLOAT, 8, 1, 2);
    InitBuilinMatrixType("float1x3", DXGI_FORMAT_R32G32B32_FLOAT, AST_TYPE_FLAG_FLOAT, 12, 1, 3);
    InitBuilinMatrixType("float1x4", DXGI_FORMAT_R32G32B32A32_FLOAT, AST_TYPE_FLAG_FLOAT, 16, 1, 4);
    InitBuilinMatrixType("float2x1", DXGI_FORMAT_R32G32_FLOAT, AST_TYPE_FLAG_FLOAT, 8, 2, 1);
    InitBuilinMatrixType("float2x2", DXGI_FORMAT_R32G32B32A32_FLOAT, AST_TYPE_FLAG_FLOAT, 16, 2, 2);
    InitBuilinMatrixType("float2x3", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*2*3, 2, 3);
    InitBuilinMatrixType("float2x4", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*2*4, 2, 4);
    InitBuilinMatrixType("float3x1", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*3*1, 3, 1);
    InitBuilinMatrixType("float3x2", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*3*2, 3, 2);
    InitBuilinMatrixType("float3x3", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*3*3, 3, 3);
    InitBuilinMatrixType("float3x4", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*3*4, 3, 4);
    InitBuilinMatrixType("float4x1", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*4*1, 4, 1);
    InitBuilinMatrixType("float4x2", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*4*2, 4, 2);
    InitBuilinMatrixType("float4x3", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*4*3, 4, 3);
    InitBuilinMatrixType("float4x4", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 4*4*4, 4, 4);

    InitBuilinScalarType("double", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8);
    InitBuilinVectorType("double1", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8*1, 1);
    InitBuilinVectorType("double2", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8*2, 2);
    InitBuilinVectorType("double3", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8*3, 3);
    InitBuilinVectorType("double4", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8*4, 4);
    InitBuilinMatrixType("double1x1", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8*1*1, 1, 1);
    InitBuilinMatrixType("double1x2", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8*1*2, 1, 2);
    InitBuilinMatrixType("double1x3", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8*1*3, 1, 3);
    InitBuilinMatrixType("double1x4", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8*1*4, 1, 4);
    InitBuilinMatrixType("double2x1", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8*2*1, 2, 1);
    InitBuilinMatrixType("double2x2", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8*2*2, 2, 2);
    InitBuilinMatrixType("double2x3", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8*2*3, 2, 3);
    InitBuilinMatrixType("double2x4", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8*2*4, 2, 4);
    InitBuilinMatrixType("double3x1", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8*3*1, 3, 1);
    InitBuilinMatrixType("double3x2", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8*3*2, 3, 2);
    InitBuilinMatrixType("double3x3", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8*3*3, 3, 3);
    InitBuilinMatrixType("double3x4", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8*3*4, 3, 4);
    InitBuilinMatrixType("double4x1", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8*4*1, 4, 1);
    InitBuilinMatrixType("double4x2", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8*4*2, 4, 2);
    InitBuilinMatrixType("double4x3", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8*4*3, 4, 3);
    InitBuilinMatrixType("double4x4", DXGI_FORMAT_UNKNOWN, AST_TYPE_FLAG_NONE, 8*4*4, 4, 4);
}

internal void ParsePreprocessor(ShaderParser *parser, GraphicsProgramDesc *gpd)
{
    GetNextGraphicsShaderToken(&parser->lexer);
    if (TokenEqualsCSTR(parser->lexer.token, "cengine"))
    {
        GetNextGraphicsShaderToken(&parser->lexer);
        CheckToken(&parser->lexer, TOKEN_KIND_KEYWORD);

        if (TokenEqualsCSTR(parser->lexer.token, "blending"))
        {
            GetNextGraphicsShaderToken(&parser->lexer);
            CheckToken(&parser->lexer, TOKEN_KIND_LPAREN);

            GetNextGraphicsShaderToken(&parser->lexer);
            CheckToken(&parser->lexer, TOKEN_KIND_KEYWORD);

            if (TokenEqualsCSTR(parser->lexer.token, "enable"))
            {
                gpd->psd.blending_enabled = true;
            }
            else if (TokenEqualsCSTR(parser->lexer.token, "disable"))
            {
                gpd->psd.blending_enabled = false;
            }
            else
            {
                SyntaxError(parser->lexer, "undefined blending option: %s", parser->lexer.token.name);
            }

            GetNextGraphicsShaderToken(&parser->lexer);
            CheckToken(&parser->lexer, TOKEN_KIND_RPAREN);
        }
        else if (TokenEqualsCSTR(parser->lexer.token, "depth_test"))
        {
            GetNextGraphicsShaderToken(&parser->lexer);
            CheckToken(&parser->lexer, TOKEN_KIND_LPAREN);

            GetNextGraphicsShaderToken(&parser->lexer);
            CheckToken(&parser->lexer, TOKEN_KIND_KEYWORD);

            if (TokenEqualsCSTR(parser->lexer.token, "enable"))
            {
                gpd->psd.depth_test_enabled = true;
            }
            else if (TokenEqualsCSTR(parser->lexer.token, "disable"))
            {
                gpd->psd.depth_test_enabled = false;
            }
            else
            {
                SyntaxError(parser->lexer, "undefined depth test option: %s", parser->lexer.token.name);
            }

            GetNextGraphicsShaderToken(&parser->lexer);
            CheckToken(&parser->lexer, TOKEN_KIND_RPAREN);
        }
        else if (TokenEqualsCSTR(parser->lexer.token, "cull_mode"))
        {
            GetNextGraphicsShaderToken(&parser->lexer);
            CheckToken(&parser->lexer, TOKEN_KIND_LPAREN);

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
                SyntaxError(parser->lexer, "undefined cull mode option: %s", parser->lexer.token.name);
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

                shader_desc->target          = "vs_5_0";
                shader_desc->entry_point     = "VSMain";
                shader_desc->entry_point_len = CSTRLEN("VSMain");

                GetNextGraphicsShaderToken(&parser->lexer);

                if (parser->lexer.token.kind == TOKEN_KIND_COMMA)
                {
                    GetNextGraphicsShaderToken(&parser->lexer);
                    CheckToken(&parser->lexer, TOKEN_KIND_STRING);

                    shader_desc->name = parser->lexer.token.str_val;

                    GetNextGraphicsShaderToken(&parser->lexer);
                }

                CheckToken(&parser->lexer, TOKEN_KIND_RPAREN);

                shader_desc->code_start = parser->lexer.stream + 1;
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

                shader_desc->target          = "hs_5_0";
                shader_desc->entry_point     = "HSMain";
                shader_desc->entry_point_len = CSTRLEN("HSMain");

                GetNextGraphicsShaderToken(&parser->lexer);

                if (parser->lexer.token.kind == TOKEN_KIND_COMMA)
                {
                    GetNextGraphicsShaderToken(&parser->lexer);
                    CheckToken(&parser->lexer, TOKEN_KIND_STRING);

                    shader_desc->name = parser->lexer.token.str_val;

                    GetNextGraphicsShaderToken(&parser->lexer);
                }

                CheckToken(&parser->lexer, TOKEN_KIND_RPAREN);

                shader_desc->code_start = parser->lexer.stream + 1;
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

                shader_desc->target          = "ds_5_0";
                shader_desc->entry_point     = "DSMain";
                shader_desc->entry_point_len = CSTRLEN("DSMain");

                GetNextGraphicsShaderToken(&parser->lexer);

                if (parser->lexer.token.kind == TOKEN_KIND_COMMA)
                {
                    GetNextGraphicsShaderToken(&parser->lexer);
                    CheckToken(&parser->lexer, TOKEN_KIND_STRING);

                    shader_desc->name = parser->lexer.token.str_val;

                    GetNextGraphicsShaderToken(&parser->lexer);
                }

                CheckToken(&parser->lexer, TOKEN_KIND_RPAREN);

                shader_desc->code_start = parser->lexer.stream + 1;
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

                shader_desc->target          = "gs_5_0";
                shader_desc->entry_point     = "GSMain";
                shader_desc->entry_point_len = CSTRLEN("GSMain");

                GetNextGraphicsShaderToken(&parser->lexer);

                if (parser->lexer.token.kind == TOKEN_KIND_COMMA)
                {
                    GetNextGraphicsShaderToken(&parser->lexer);
                    CheckToken(&parser->lexer, TOKEN_KIND_STRING);

                    shader_desc->name = parser->lexer.token.str_val;

                    GetNextGraphicsShaderToken(&parser->lexer);
                }

                CheckToken(&parser->lexer, TOKEN_KIND_RPAREN);

                shader_desc->code_start = parser->lexer.stream + 1;
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

                shader_desc->target          = "ps_5_0";
                shader_desc->entry_point     = "PSMain";
                shader_desc->entry_point_len = CSTRLEN("PSMain");

                GetNextGraphicsShaderToken(&parser->lexer);

                if (parser->lexer.token.kind == TOKEN_KIND_COMMA)
                {
                    GetNextGraphicsShaderToken(&parser->lexer);
                    CheckToken(&parser->lexer, TOKEN_KIND_STRING);

                    shader_desc->name = parser->lexer.token.str_val;

                    GetNextGraphicsShaderToken(&parser->lexer);
                }

                CheckToken(&parser->lexer, TOKEN_KIND_RPAREN);

                shader_desc->code_start = parser->lexer.stream + 1;
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
                CheckToken(&parser->lexer, TOKEN_KIND_RPAREN);

                shader_desc->code_start = parser->lexer.stream + 1;
            }
            else
            {
                SyntaxError(parser->lexer, "undefined shader type: %s", parser->lexer.token.name);
            }
        }
    }
}

internal u32 CountTypesInStructRecursively(ASTType *struct_type)
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

internal u32 CollectStructFiedlsRecursively(ShaderParser *parser, ASTType *struct_type, u32 outer_index, D3D12_INPUT_ELEMENT_DESC *desc)
{
    for (u64 i = 0; i < struct_type->_struct->fields_count; ++i)
    {
        ASTStructField *field = struct_type->_struct->fields + i;

        if (field->type->kind == AST_TYPE_KIND_STRUCT)
        {
            outer_index  = CollectStructFiedlsRecursively(parser, field->type, outer_index, desc);
            desc        += outer_index;
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

internal void ParseInputLayout(Engine *engine, ShaderParser *parser, ShaderDesc *shader_desc, GraphicsProgramDesc *gpd)
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
                if (it->kind == AST_TYPE_KIND_STRUCT)
                {
                    gpd->psd.input_layout.NumElements += CountTypesInStructRecursively(it);
                }
                else
                {
                    // @TODO(Roman): what if we have an array in args?
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

        #pragma warning(suppress: 4090)
        D3D12_INPUT_ELEMENT_DESC *desc = gpd->psd.input_layout.pInputElementDescs + i;

        ASTType *type = 0;
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
            i = CollectStructFiedlsRecursively(parser, type, i, desc) - 1;

            GetNextGraphicsShaderToken(&parser->lexer);
            CheckToken(&parser->lexer, TOKEN_KIND_NAME);
        }
        else
        {
            desc->InputSlot            = 0;
            desc->AlignedByteOffset    = i > 0 ? D3D12_APPEND_ALIGNED_ELEMENT : 0;
            desc->InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            desc->InstanceDataStepRate = 0;

            GetNextGraphicsShaderToken(&parser->lexer);
            CheckToken(&parser->lexer, TOKEN_KIND_NAME);

            GetNextGraphicsShaderToken(&parser->lexer);
            if (parser->lexer.token.kind == TOKEN_KIND_LBRACKET)
            {
                // @TODO(Roman): Array support
                SyntaxError(parser->lexer, "Arrays in entry's point arguments are currently not supported. Make a typedef to be able to work with them.");
            }
            CheckToken(&parser->lexer, TOKEN_KIND_COLON);

            GetNextGraphicsShaderToken(&parser->lexer);
            CheckToken(&parser->lexer, TOKEN_KIND_NAME);

            const char *index_start = parser->lexer.token.end;
            while (isdigit(index_start[-1])) --index_start;

            desc->SemanticName  = InternStringRange(&parser->lexer.interns, parser->lexer.token.start, index_start);
            desc->SemanticIndex = strtoul(index_start, 0, 10);
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

internal void ParseStruct(Engine *engine, ShaderParser *parser)
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
        ASTStructField *temp_it = _struct->fields;
        for (u64 i = 1; i < _struct->fields_count; ++i)
        {
            temp_it->next = _struct->fields + i;
            temp_it       = temp_it->next;
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
            // @TODO(Roman): Array support
            SyntaxError(parser->lexer, "Arrays in structs are currently not supported. Make a typedef to be able to work with them.");

            GetNextGraphicsShaderToken(&parser->lexer);
            CheckToken(&parser->lexer, TOKEN_KIND_INT);

            struct_size += field->type->size_in_bytes * parser->lexer.token.u64_val;

            GetNextGraphicsShaderToken(&parser->lexer);
            CheckToken(&parser->lexer, TOKEN_KIND_RBRACKET);

            GetNextGraphicsShaderToken(&parser->lexer);
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

            field->semantic_index = strtoul(index_start, 0, 10);
            field->semantic_name  = InternStringRange(&parser->lexer.interns, parser->lexer.token.start, index_start);
        }

        // Other stuff we don't care about
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

    // done
    ASTType *last_type = parser->types;
    while (last_type->next) last_type = last_type->next;
    last_type->next                = PushToTA(ASTType, engine->memory, 1);
    last_type->next->next          = 0;
    last_type->next->base_type     = 0;
    last_type->next->name          = _struct->name;
    last_type->next->kind          = AST_TYPE_KIND_STRUCT;
    last_type->next->format        = DXGI_FORMAT_UNKNOWN;
    last_type->next->flag          = AST_TYPE_FLAG_NONE;
    last_type->next->builtin       = false;
    last_type->next->size_in_bytes = struct_size;
    last_type->next->_struct       = _struct;
}

internal void ParseTypedef(Engine *engine, ShaderParser *parser)
{
    // typedef [const] type name ['['index']'];

    ASTType type;
    type.next    = 0;
    type.builtin = false;

    GetNextGraphicsShaderToken(&parser->lexer);
    if (TokenEqualsCSTR(parser->lexer.token, "const"))
    {
        GetNextGraphicsShaderToken(&parser->lexer);
    }
    CheckToken(&parser->lexer, TOKEN_KIND_NAME);

    ASTType *base_type = 0;
    for (ASTType *it = parser->types; it; it = it->next)
    {
        if (it->name == parser->lexer.token.name)
        {
            base_type = it;
            break;
        }
    }
    if (!base_type) SyntaxError(parser->lexer, "unknown type: '%s'", parser->lexer.token.name);

    type.base_type = base_type;
    type.flag      = base_type->flag;

    GetNextGraphicsShaderToken(&parser->lexer);
    CheckToken(&parser->lexer, TOKEN_KIND_NAME);

    type.name = parser->lexer.token.name;

    GetNextGraphicsShaderToken(&parser->lexer);
    if (parser->lexer.token.kind == TOKEN_KIND_LBRACKET)
    {
        type.kind = AST_TYPE_KIND_ARRAY;

        GetNextGraphicsShaderToken(&parser->lexer);
        CheckToken(&parser->lexer, TOKEN_KIND_INT);

        type.dimension     = cast(u32, parser->lexer.token.u64_val);
        type.size_in_bytes = type.dimension * base_type->size_in_bytes;

        if (type.size_in_bytes <= 16)
        {
            switch (type.flag)
            {
                case AST_TYPE_FLAG_SINT:
                {
                    /**/ if (type.size_in_bytes ==  4) type.format = DXGI_FORMAT_R32_SINT;
                    else if (type.size_in_bytes ==  8) type.format = DXGI_FORMAT_R32G32_SINT;
                    else if (type.size_in_bytes == 12) type.format = DXGI_FORMAT_R32G32B32_SINT;
                    else if (type.size_in_bytes == 16) type.format = DXGI_FORMAT_R32G32B32A32_SINT;
                    else                               type.format = DXGI_FORMAT_UNKNOWN;
                } break;

                case AST_TYPE_FLAG_UINT:
                {
                    /**/ if (type.size_in_bytes ==  4) type.format = DXGI_FORMAT_R32_UINT;
                    else if (type.size_in_bytes ==  8) type.format = DXGI_FORMAT_R32G32_UINT;
                    else if (type.size_in_bytes == 12) type.format = DXGI_FORMAT_R32G32B32_UINT;
                    else if (type.size_in_bytes == 16) type.format = DXGI_FORMAT_R32G32B32A32_UINT;
                    else                               type.format = DXGI_FORMAT_UNKNOWN;
                } break;

                case AST_TYPE_FLAG_HALF:
                {
                    /**/ if (type.size_in_bytes == 2) type.format = DXGI_FORMAT_R16_FLOAT;
                    else if (type.size_in_bytes == 4) type.format = DXGI_FORMAT_R16G16_FLOAT;
                    else if (type.size_in_bytes == 6) type.format = DXGI_FORMAT_UNKNOWN;
                    else if (type.size_in_bytes == 8) type.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
                    else                              type.format = DXGI_FORMAT_UNKNOWN;
                } break;

                case AST_TYPE_FLAG_FLOAT:
                {
                    /**/ if (type.size_in_bytes ==  4) type.format = DXGI_FORMAT_R32_FLOAT;
                    else if (type.size_in_bytes ==  8) type.format = DXGI_FORMAT_R32G32_FLOAT;
                    else if (type.size_in_bytes == 12) type.format = DXGI_FORMAT_R32G32B32_FLOAT;
                    else if (type.size_in_bytes == 16) type.format = DXGI_FORMAT_R32G32B32A32_FLOAT;
                    else                               type.format = DXGI_FORMAT_UNKNOWN;
                } break;

                default:
                {
                    type.format = DXGI_FORMAT_UNKNOWN;
                } break;
            }
        }
        else
        {
            type.format = DXGI_FORMAT_UNKNOWN;
        }

        GetNextGraphicsShaderToken(&parser->lexer);
        CheckToken(&parser->lexer, TOKEN_KIND_RBRACKET);

        GetNextGraphicsShaderToken(&parser->lexer);
    }
    else // not an array
    {
        type.kind          = base_type->kind;
        type.format        = base_type->format;
        type.size_in_bytes = base_type->size_in_bytes;
        switch (type.kind)
        {
            case AST_TYPE_KIND_VECTOR:
            {
                type.dimension = base_type->dimension;
            } break;
            
            case AST_TYPE_KIND_MATRIX:
            {
                type.mat_size = base_type->mat_size;
            } break;

            case AST_TYPE_KIND_STRUCT:
            {
                type._struct = base_type->_struct;
            } break;
        }
    }
    CheckToken(&parser->lexer, TOKEN_KIND_SEMICOLON);

    ASTType *last_type = parser->types;
    while (last_type->next) last_type = last_type->next;
    last_type->next  = PushToTA(ASTType, engine->memory, 1);
    *last_type->next = type;
}

void ParseGraphicsShaders(Engine *engine, const char *file_with_shaders, GraphicsProgramDesc *gpd)
{
    char *shader_code = ReadEntireShaderFile(engine, file_with_shaders, 0);

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
                &&  *shader_desc->target == 'v'
                &&  TokenEquals(parser.lexer.token, shader_desc->entry_point, shader_desc->entry_point_len))
                {
                    ParseInputLayout(engine, &parser, shader_desc, gpd);
                }
            } break;

            case TOKEN_KIND_KEYWORD:
            {
                // @TODO(Roman): parse CBV, SRV, UAV, ...
                if (TokenEqualsCSTR(parser.lexer.token, "struct"))
                {
                    ParseStruct(engine, &parser);
                }
                else if (TokenEqualsCSTR(parser.lexer.token, "typedef"))
                {
                    ParseTypedef(engine, &parser);
                }
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
}
