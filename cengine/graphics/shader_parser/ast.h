//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "tools/buffer.h"
#include "math/vec.h"

typedef struct ASTType        ASTType;
typedef struct ASTStruct      ASTStruct;
typedef struct ASTStructField ASTStructField;
typedef struct ASTCBuffer     ASTCBuffer;

typedef enum AST_TYPE_KIND
{
    AST_TYPE_KIND_SCALAR,
    AST_TYPE_KIND_VECTOR,
    AST_TYPE_KIND_MATRIX,
    AST_TYPE_KIND_ARRAY,
    AST_TYPE_KIND_STRUCT,
} AST_TYPE_KIND;

typedef enum AST_TYPE_FLAG
{
    AST_TYPE_FLAG_NONE,
    AST_TYPE_FLAG_SINT,
    AST_TYPE_FLAG_UINT,
    AST_TYPE_FLAG_HALF,
    AST_TYPE_FLAG_FLOAT,
} AST_TYPE_FLAG;

struct ASTType
{
    ExtendsList(ASTType);
    ASTType       *base_type;
    const char    *name;
    AST_TYPE_KIND  kind;
    DXGI_FORMAT    format;
    AST_TYPE_FLAG  flag;
    b32            builtin;
    u64            size_in_bytes;
    union
    {
        u32        dimension;
        v2u        mat_size;
        ASTStruct *_struct;
    };
};

struct ASTStructField
{
    ExtendsList(ASTStructField);
    ASTType    *type;
    const char *semantic_name;
    u32         semantic_index;
};

struct ASTStruct
{
    ExtendsList(ASTStruct);
    const char          *name;
    LIST ASTStructField *fields;
    u64                  fields_count;
};

struct ASTCBuffer
{
    ExtendsList(ASTCBuffer);
    const char *name;
    u64         name_len;
    u32         _register;
    u32         space;
    u32         size_in_bytes;
    // @TODO(Roman): How to parse 'em?
    D3D12_ROOT_DESCRIPTOR_FLAGS flags; // @NOTE(Roman): Only for Root Signature v1.1
};
