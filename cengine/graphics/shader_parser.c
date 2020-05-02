//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "tools/buffer.h"
#include "graphics/renderer.h"
#include "cengine.h"

//
// Interns
//

typedef struct Intern
{
    const char *str;
    u64 len;
} Intern;

global BUF Intern *gInterns;
global     Memory *gInternsMemory;

internal INLINE void InitInterns(Engine *engine)
{
    gInterns       = CreateBuffer(&engine->allocator, sizeof(void *));
    gInternsMemory = engine->memory;
}

internal INLINE void DestroyInterns()
{
    DestroyBuffer(gInterns);
    gInternsMemory = 0;
}

internal const char *InternStringRange(const char *start, const char *end)
{
    u64 len = end - start;

    Intern *first_intern = gInterns;
    Intern *last_intern  = gInterns + BufferGetCount(gInterns) - 1;

	for (Intern *it = first_intern; it <= last_intern; ++it)
    {
        if (it->len == len && !memcmp(it->str, start, len))
        {
            return it->str;
        }
    }

    char *new_str = PushToTransientArea(gInternsMemory, len + 1);
    CopyMemory(new_str, start, len);
    new_str[len] = '\0';

    Intern new_intern;
    new_intern.str = new_str;
    new_intern.len = len;
	BufferPushBack(gInterns, new_intern);
	
	return new_str;
}

#define InternString(string, length) InternStringRange(string, (string) + (length))
#define InternCSTR(cstr)             InternString(cstr, CSTRLEN(cstr))

//
// Lexer
//

#define PARSE_INT_FLOAT 0

typedef enum TOKEN_KIND
{
    TOKEN_KIND_EOF,
    TOKEN_KIND_DO_NOT_CARE,

    TOKEN_KIND_HASH,
    TOKEN_KIND_LPAREN,
    TOKEN_KIND_RPAREN,
    TOKEN_KIND_LBRACE,
    TOKEN_KIND_RBRACE,
    TOKEN_KIND_COMMA,
    TOKEN_KIND_COLON,
    TOKEN_KIND_SEMICOLON,

    TOKEN_KIND_NAME,
    TOKEN_KIND_STRING,
    TOKEN_KIND_KEYWORD,
#if PARSE_INT_FLOAT
    TOKEN_KIND_INT,
    TOKEN_KIND_FLOAT,
#endif
} TOKEN_KIND;

global const char *token_kind_names[] =
{
    [TOKEN_KIND_EOF]         = "EOF",
    [TOKEN_KIND_DO_NOT_CARE] = "Parset doesn't care",

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
#if PARSE_INT_FLOAT
    [TOKEN_KIND_INT]         = "<int immediate>",
    [TOKEN_KIND_FLOAT]       = "<float immediate>",
#endif
};

typedef struct Token
{
    TOKEN_KIND kind;
    const char *start;
    const char *end;
    union
    {
        const char *name;
        const char *str_val;
        char        char_val; // @CleanUp(Roman): remove?
    #if PARSE_INT_FLOAT
        s32         int_val;
        f32         float_val;
    #endif
    };
} Token;

global const char *stream;
global Token token;

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
global const char *first_keyword;
global const char *last_keyword;

internal void InitKeywords()
{
    first_keyword = InternString(*keywords, strlen(*keywords));

    u32 last_index = ArrayCount(keywords) - 1;
    for (u32 i = 1; i < last_index; ++i)
    {
        InternString(keywords[i], strlen(keywords[i]));
    }

    last_keyword = InternString(keywords[last_index], strlen(keywords[last_index]));
}

#define IsKeyword(name) (first_keyword <= (name) && (name) <= last_keyword)

internal INLINE const char *token_kind_name(TOKEN_KIND kind)
{
    if (token.kind == TOKEN_KIND_NAME || token.kind == TOKEN_KIND_KEYWORD)
    {
        return token.name;
    }
    else if (kind < ArrayCount(token_kind_names))
    {
        const char *name = token_kind_names[kind];
        return name ? name : "<unknown>";
    }
    else
    {
        return "<unknown>";
    }
}

#define TokenEquals(str, len)  ((token.end - token.start == (len)) && !memcmp(token.start, str, len))
#define TokenEqualsCSTR(cstr)  TokenEquals(cstr, CSTRLEN(cstr))
#define TokenEqualsChar(_char) (*token.start == (_char))

#define SyntaxError(format, ...) MessageF(MESSAGE_TYPE_ERROR, CSTRCAT("Shader syntax error: ", format), __VA_ARGS__)

internal INLINE void CheckToken(TOKEN_KIND kind)
{
    if (token.kind != kind)
    {
        SyntaxError("expected '%s', got '%s'.",
                    token_kind_name(kind),
                    token_kind_name(token.kind));
    }
}

internal INLINE void CheckSyntax(const char *expected, u32 expected_len)
{
    if ((token.end - token.start != expected_len)
    ||  memcmp(token.start, expected, token.end - token.start))
    {
        SyntaxError("exected '%s', got '%.*s'.",
                    expected,
                    token.end - token.start, token.start);
    }
}
#define CheckSyntaxCSTR(expected) CheckSyntax(expected, CSTRLEN(expected))
#define CheckSyntaxChar(expected) CheckSyntax(expected, 1)

internal void GetNextToken();

internal INLINE void InitStream(const char *str)
{
    stream = str;
    ZeroMemory(&token, sizeof(Token));
    GetNextToken();
}

#define ELSE_IF_STREAM_1(c1, k1) else if (*stream == (c1)) { token.char_val = (c1); token.kind = (k1); ++stream; }

internal void GetNextToken()
{
try_again:

    while (isspace(*stream)) ++stream;

    token.start = stream;

    if (isalpha(*stream) || *stream == '_')
    {
        while (isalnum(*stream) || *stream == '_') ++stream;
        token.name = InternStringRange(token.start, stream);
        token.kind = IsKeyword(token.name) ? TOKEN_KIND_KEYWORD : TOKEN_KIND_NAME;
    }
#if PARSE_INT_FLOAT
    else if (isdigit(*stream) || (*stream == '-' && isdigit(stream[1])))
    {
        if (*stream == '-') ++stream;
        while (isdigit(*stream)) ++stream;
        token.kind = TOKEN_KIND_INT;

        if (*stream == '.' || *stream == 'e')
        {
            ++stream;
            token.kind = TOKEN_KIND_FLOAT;
            if (*stream == 'e') ++stream;
            if (*stream == '+' || *stream == '-') ++stream;
            while (isdigit(*stream)) ++stream;
            if (*stream == 'f') ++stream;
        }
    }
#endif
    ELSE_IF_STREAM_1('\0', TOKEN_KIND_EOF)
    ELSE_IF_STREAM_1('#', TOKEN_KIND_HASH)
    ELSE_IF_STREAM_1('(', TOKEN_KIND_LPAREN)
    ELSE_IF_STREAM_1(')', TOKEN_KIND_RPAREN)
    ELSE_IF_STREAM_1('{', TOKEN_KIND_LBRACE)
    ELSE_IF_STREAM_1('}', TOKEN_KIND_RBRACE)
    ELSE_IF_STREAM_1(',', TOKEN_KIND_COMMA)
    ELSE_IF_STREAM_1(':', TOKEN_KIND_COLON)
    ELSE_IF_STREAM_1(';', TOKEN_KIND_SEMICOLON)
    else if (*stream == '/' && stream[1] == '/')
    {
        while (*stream != '\n') ++stream;
        goto try_again;
    }
    else if (*stream == '"')
    {
        ++stream;
        while (*stream && *stream != '"')
        {
            ++stream;
            if (*stream == '\n')
            {
                SyntaxError("no new lines in shader names");
            }
            else if (*stream == '\\')
            {
                SyntaxError("no escape characters in shader names");
            }
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
        token.str_val = InternStringRange(token.start, stream);
        token.kind    = TOKEN_KIND_STRING;
    }
    else
    {
        token.kind = TOKEN_KIND_DO_NOT_CARE;
        ++stream;
    }

    token.end = stream;
}

//
// Parser
//

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

internal void ParsePreprocessor(GraphicsProgramDesc *gpd)
{
    GetNextToken();
    CheckToken(TOKEN_KIND_KEYWORD);

    if (TokenEqualsCSTR("cengine"))
    {
        GetNextToken();
        CheckToken(TOKEN_KIND_KEYWORD);

        if (TokenEqualsCSTR("blending"))
        {
            GetNextToken();
            CheckToken(TOKEN_KIND_LPAREN);

            GetNextToken();
            CheckToken(TOKEN_KIND_KEYWORD);

            if (TokenEqualsCSTR("enable"))
            {
                gpd->psd.blending_enabled = true;
            }
            else if (TokenEqualsCSTR("disable"))
            {
                gpd->psd.blending_enabled = false;
            }
            else
            {
                SyntaxError("undefined blending option: %s", token.name);
            }

            GetNextToken();
            CheckToken(TOKEN_KIND_RPAREN);
        }
        else if (TokenEqualsCSTR("depth_test"))
        {
            GetNextToken();
            CheckToken(TOKEN_KIND_LPAREN);

            GetNextToken();
            CheckToken(TOKEN_KIND_KEYWORD);

            if (TokenEqualsCSTR("enable"))
            {
                gpd->psd.depth_test_enabled = true;
            }
            else if (TokenEqualsCSTR("disable"))
            {
                gpd->psd.depth_test_enabled = false;
            }
            else
            {
                SyntaxError("undefined depth test option: %s", token.name);
            }

            GetNextToken();
            CheckToken(TOKEN_KIND_RPAREN);
        }
        else if (TokenEqualsCSTR("shader"))
        {
            ShaderDesc *shader_desc = gpd->sd.descs + gpd->sd.count;

            GetNextToken();
            CheckToken(TOKEN_KIND_LPAREN);

            GetNextToken();
            CheckToken(TOKEN_KIND_KEYWORD);

            if (TokenEqualsCSTR("vertex"))
            {
                if (shader_desc->code_start)
                {
                    ++gpd->sd.count;

                    const char *shader_code_end = stream;
                    while (*shader_code_end != '#') --shader_code_end;

                    shader_desc->code_end = shader_code_end;

                    shader_desc = gpd->sd.descs + gpd->sd.count;
                }

                shader_desc->target      = "vs_5_0";
                shader_desc->entry_point = "VSMain";

                GetNextToken();

                if (token.kind == TOKEN_KIND_COMMA)
                {
                    GetNextToken();
                    CheckToken(TOKEN_KIND_STRING);

                    shader_desc->name = token.str_val;

                    GetNextToken();
                }

                CheckToken(TOKEN_KIND_RPAREN);

                shader_desc->code_start = stream + 1;
            }
            else if (TokenEqualsCSTR("hull"))
            {
                if (shader_desc->code_start)
                {
                    ++gpd->sd.count;
    
                    const char *shader_code_end = stream;
                    while (*shader_code_end != '#') --shader_code_end;
    
                    shader_desc->code_end = shader_code_end;
    
                    shader_desc = gpd->sd.descs + gpd->sd.count;
                }

                shader_desc->target      = "hs_5_0";
                shader_desc->entry_point = "HSMain";

                GetNextToken();

                if (token.kind == TOKEN_KIND_COMMA)
                {
                    GetNextToken();
                    CheckToken(TOKEN_KIND_STRING);

                    shader_desc->name = token.str_val;

                    GetNextToken();
                }

                CheckToken(TOKEN_KIND_RPAREN);

                shader_desc->code_start = stream + 1;
            }
            else if (TokenEqualsCSTR("domain"))
            {
                if (shader_desc->code_start)
                {
                    ++gpd->sd.count;
    
                    const char *shader_code_end = stream;
                    while (*shader_code_end != '#') --shader_code_end;
    
                    shader_desc->code_end = shader_code_end;
    
                    shader_desc = gpd->sd.descs + gpd->sd.count;
                }

                shader_desc->target      = "ds_5_0";
                shader_desc->entry_point = "DSMain";

                GetNextToken();

                if (token.kind == TOKEN_KIND_COMMA)
                {
                    GetNextToken();
                    CheckToken(TOKEN_KIND_STRING);

                    shader_desc->name = token.str_val;

                    GetNextToken();
                }

                CheckToken(TOKEN_KIND_RPAREN);

                shader_desc->code_start = stream + 1;
            }
            else if (TokenEqualsCSTR("geometry"))
            {
                if (shader_desc->code_start)
                {
                    ++gpd->sd.count;
    
                    const char *shader_code_end = stream;
                    while (*shader_code_end != '#') --shader_code_end;
    
                    shader_desc->code_end = shader_code_end;
    
                    shader_desc = gpd->sd.descs + gpd->sd.count;
                }

                shader_desc->target      = "gs_5_0";
                shader_desc->entry_point = "GSMain";

                GetNextToken();

                if (token.kind == TOKEN_KIND_COMMA)
                {
                    GetNextToken();
                    CheckToken(TOKEN_KIND_STRING);

                    shader_desc->name = token.str_val;

                    GetNextToken();
                }

                CheckToken(TOKEN_KIND_RPAREN);

                shader_desc->code_start = stream + 1;
            }
            else if (TokenEqualsCSTR("pixel"))
            {
                if (shader_desc->code_start)
                {
                    ++gpd->sd.count;
    
                    const char *shader_code_end = stream;
                    while (*shader_code_end != '#') --shader_code_end;
    
                    shader_desc->code_end = shader_code_end;

                    shader_desc = gpd->sd.descs + gpd->sd.count;
                }

                shader_desc->target      = "ps_5_0";
                shader_desc->entry_point = "PSMain";

                GetNextToken();

                if (token.kind == TOKEN_KIND_COMMA)
                {
                    GetNextToken();
                    CheckToken(TOKEN_KIND_STRING);

                    shader_desc->name = token.str_val;

                    GetNextToken();
                }

                CheckToken(TOKEN_KIND_RPAREN);

                shader_desc->code_start = stream + 1;
            }
            else if (TokenEqualsCSTR("entry_point"))
            {
                GetNextToken();
                CheckToken(TOKEN_KIND_COMMA);

                GetNextToken();
                CheckToken(TOKEN_KIND_NAME);

                shader_desc->entry_point = token.name;

                GetNextToken();
                CheckToken(TOKEN_KIND_RPAREN);

                shader_desc->code_start = stream + 1;
            }
            else
            {
                SyntaxError("undefined shader type: %s", token.name);
            }
        }
    }
}

internal void ParseInputLayoutFromArgs(Engine *engine, GraphicsProgramDesc *gpd)
{
    D3D12_INPUT_ELEMENT_DESC *ied = PushToTA(D3D12_INPUT_ELEMENT_DESC, engine->memory, gpd->psd.input_layout.NumElements);

    for (u32 i = 0; i < gpd->psd.input_layout.NumElements; ++i)
    {
        D3D12_INPUT_ELEMENT_DESC *element = ied + i;
        element->InputSlot                = 0;
        element->AlignedByteOffset        = i ? D3D12_APPEND_ALIGNED_ELEMENT : 0;
        element->InputSlotClass           = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        element->InstanceDataStepRate     = 0;

        if (TokenEqualsCSTR("in")  || TokenEqualsCSTR("inout")
        ||  TokenEqualsCSTR("out") || TokenEqualsCSTR("uniform"))
        {
            GetNextToken();
        }
        CheckToken(TOKEN_KIND_NAME);

        /**/ if (TokenEqualsCSTR("float"))  element->Format = DXGI_FORMAT_R32_FLOAT;
        else if (TokenEqualsCSTR("float1")) element->Format = DXGI_FORMAT_R32_FLOAT;
        else if (TokenEqualsCSTR("float2")) element->Format = DXGI_FORMAT_R32G32_FLOAT;
        else if (TokenEqualsCSTR("float3")) element->Format = DXGI_FORMAT_R32G32B32_FLOAT;
        else if (TokenEqualsCSTR("float4")) element->Format = DXGI_FORMAT_R32G32B32A32_FLOAT;

        else if (TokenEqualsCSTR("int"))  element->Format = DXGI_FORMAT_R32_SINT;
        else if (TokenEqualsCSTR("int1")) element->Format = DXGI_FORMAT_R32_SINT;
        else if (TokenEqualsCSTR("int2")) element->Format = DXGI_FORMAT_R32G32_SINT;
        else if (TokenEqualsCSTR("int3")) element->Format = DXGI_FORMAT_R32G32B32_SINT;
        else if (TokenEqualsCSTR("int4")) element->Format = DXGI_FORMAT_R32G32B32A32_SINT;

        else if (TokenEqualsCSTR("uint"))  element->Format = DXGI_FORMAT_R32_UINT;
        else if (TokenEqualsCSTR("uint1")) element->Format = DXGI_FORMAT_R32_UINT;
        else if (TokenEqualsCSTR("uint2")) element->Format = DXGI_FORMAT_R32G32_UINT;
        else if (TokenEqualsCSTR("uint3")) element->Format = DXGI_FORMAT_R32G32B32_UINT;
        else if (TokenEqualsCSTR("uint4")) element->Format = DXGI_FORMAT_R32G32B32A32_UINT;

        else if (TokenEqualsCSTR("dword")) element->Format = DXGI_FORMAT_R32_UINT;

        else if (TokenEqualsCSTR("half"))  element->Format = DXGI_FORMAT_R16_FLOAT;
        else if (TokenEqualsCSTR("half1")) element->Format = DXGI_FORMAT_R16_FLOAT;
        else if (TokenEqualsCSTR("half2")) element->Format = DXGI_FORMAT_R16G16_FLOAT;
        // else if (TokenEqualsCSTR("half3")) element->Format = DXGI_FORMAT_R16G16B16_FLOAT; // @NOTE(Roman): DirectX missing
        else if (TokenEqualsCSTR("half4")) element->Format = DXGI_FORMAT_R16G16B16A16_FLOAT;

        // @NOTE(Roman): DirectX missing
        // else if (TokenEqualsCSTR("double"))  element->Format = DXGI_FORMAT_R64_TYPELESS;
        // else if (TokenEqualsCSTR("double1")) element->Format = DXGI_FORMAT_R64_TYPELESS;
        // else if (TokenEqualsCSTR("double2")) element->Format = DXGI_FORMAT_R64G64_TYPELESS;
        // else if (TokenEqualsCSTR("double3")) element->Format = DXGI_FORMAT_R64G64B64_TYPELESS;
        // else if (TokenEqualsCSTR("double4")) element->Format = DXGI_FORMAT_R64G64B64A64_TYPELESS;

        else SyntaxError("unsupported arg type: %s", token.name);

        GetNextToken();
        CheckToken(TOKEN_KIND_NAME);

        GetNextToken();
        CheckToken(TOKEN_KIND_COLON);

        GetNextToken();
        CheckToken(TOKEN_KIND_NAME);

        const char *digit_end   = token.end - 1;
        const char *digit_start = digit_end;

        while (isdigit(*digit_start)) --digit_start;

        if (isdigit(*digit_end))
        {
            ++digit_start;

            char buffer[8] = {0};
            sprintf(buffer, "%.*s", cast(s32, digit_end - digit_start), digit_start);

            element->SemanticIndex = atoi(buffer);
            element->SemanticName  = InternStringRange(token.start, digit_start);
        }
        else
        {
            element->SemanticIndex = 0;
            element->SemanticName  = token.name;
        }

        GetNextToken();

        if (i < gpd->psd.input_layout.NumElements - 1)
        {
            CheckToken(TOKEN_KIND_COMMA);
            GetNextToken();
        }
        else
        {
            CheckToken(TOKEN_KIND_RPAREN);
        }
    }

    gpd->psd.input_layout.pInputElementDescs = ied;
}

internal void ParseInputLayoutFromStruct(Engine *engine, GraphicsProgramDesc *gpd)
{
    ShaderDesc *shader_desc = gpd->sd.descs + gpd->sd.count;

    if (TokenEqualsCSTR("in")  || TokenEqualsCSTR("inout")
    ||  TokenEqualsCSTR("out") || TokenEqualsCSTR("uniform"))
    {
        GetNextToken();
    }
    CheckToken(TOKEN_KIND_NAME);

    Token       first_arg_token  = token;
    const char *first_arg_stream = stream;

    stream = shader_desc->code_start;
    GetNextToken();

    find_next_struct:
    {
        while (!TokenEqualsCSTR("struct") && token.kind != TOKEN_KIND_EOF)
        {
            GetNextToken();
        }

        GetNextToken();
        CheckToken(TOKEN_KIND_NAME);

        if (!TokenEquals(first_arg_token.start, first_arg_token.end - first_arg_token.start)
        &&  token.kind != TOKEN_KIND_EOF)
        {
            goto find_next_struct;
        }
    }

    if (token.kind == TOKEN_KIND_EOF)
    {
        SyntaxError("unexpected end of file in vertex shader");
    }

    GetNextToken();
    CheckToken(TOKEN_KIND_LBRACE);

    GetNextToken();
    CheckToken(TOKEN_KIND_NAME);

    if (TokenEqualsCSTR("uniform"))
    {
        GetNextToken();
        CheckToken(TOKEN_KIND_NAME);
    }

    Token       first_struct_member_token  = token;
    const char *first_struct_member_stream = stream;

    while (token.kind != TOKEN_KIND_RBRACE && token.kind != TOKEN_KIND_EOF)
    {
        if (token.kind == TOKEN_KIND_SEMICOLON)
        {
            ++gpd->psd.input_layout.NumElements;
        }
        GetNextToken();
    }

    if (token.kind == TOKEN_KIND_EOF)
    {
        SyntaxError("unexpected end of line in vertex shader's entry struct's members");
    }

    token  = first_struct_member_token;
    stream = first_struct_member_stream;

    D3D12_INPUT_ELEMENT_DESC *ied = PushToTA(D3D12_INPUT_ELEMENT_DESC, engine->memory, gpd->psd.input_layout.NumElements);

    for (u32 i = 0; i < gpd->psd.input_layout.NumElements; ++i)
    {
        if (i > 0) GetNextToken();

        D3D12_INPUT_ELEMENT_DESC *element = ied + i;
        element->InputSlot                = 0;
        element->AlignedByteOffset        = i ? D3D12_APPEND_ALIGNED_ELEMENT : 0;
        element->InputSlotClass           = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        element->InstanceDataStepRate     = 0;

        if (TokenEqualsCSTR("uniform"))
        {
            GetNextToken();
        }
        CheckToken(TOKEN_KIND_NAME);

        /**/ if (TokenEqualsCSTR("float"))  element->Format = DXGI_FORMAT_R32_FLOAT;
        else if (TokenEqualsCSTR("float1")) element->Format = DXGI_FORMAT_R32_FLOAT;
        else if (TokenEqualsCSTR("float2")) element->Format = DXGI_FORMAT_R32G32_FLOAT;
        else if (TokenEqualsCSTR("float3")) element->Format = DXGI_FORMAT_R32G32B32_FLOAT;
        else if (TokenEqualsCSTR("float4")) element->Format = DXGI_FORMAT_R32G32B32A32_FLOAT;

        else if (TokenEqualsCSTR("int"))  element->Format = DXGI_FORMAT_R32_SINT;
        else if (TokenEqualsCSTR("int1")) element->Format = DXGI_FORMAT_R32_SINT;
        else if (TokenEqualsCSTR("int2")) element->Format = DXGI_FORMAT_R32G32_SINT;
        else if (TokenEqualsCSTR("int3")) element->Format = DXGI_FORMAT_R32G32B32_SINT;
        else if (TokenEqualsCSTR("int4")) element->Format = DXGI_FORMAT_R32G32B32A32_SINT;

        else if (TokenEqualsCSTR("uint"))  element->Format = DXGI_FORMAT_R32_UINT;
        else if (TokenEqualsCSTR("uint1")) element->Format = DXGI_FORMAT_R32_UINT;
        else if (TokenEqualsCSTR("uint2")) element->Format = DXGI_FORMAT_R32G32_UINT;
        else if (TokenEqualsCSTR("uint3")) element->Format = DXGI_FORMAT_R32G32B32_UINT;
        else if (TokenEqualsCSTR("uint4")) element->Format = DXGI_FORMAT_R32G32B32A32_UINT;

        else if (TokenEqualsCSTR("dword")) element->Format = DXGI_FORMAT_R32_UINT;

        else if (TokenEqualsCSTR("half"))  element->Format = DXGI_FORMAT_R16_FLOAT;
        else if (TokenEqualsCSTR("half1")) element->Format = DXGI_FORMAT_R16_FLOAT;
        else if (TokenEqualsCSTR("half2")) element->Format = DXGI_FORMAT_R16G16_FLOAT;
        // else if (TokenEqualsCSTR("half3")) element->Format = DXGI_FORMAT_R16G16B16_FLOAT; // @NOTE(Roman): DirectX missing
        else if (TokenEqualsCSTR("half4")) element->Format = DXGI_FORMAT_R16G16B16A16_FLOAT;

        // @NOTE(Roman): DirectX missing
        // else if (TokenEqualsCSTR("double"))  element->Format = DXGI_FORMAT_R64_TYPELESS;
        // else if (TokenEqualsCSTR("double1")) element->Format = DXGI_FORMAT_R64_TYPELESS;
        // else if (TokenEqualsCSTR("double2")) element->Format = DXGI_FORMAT_R64G64_TYPELESS;
        // else if (TokenEqualsCSTR("double3")) element->Format = DXGI_FORMAT_R64G64B64_TYPELESS;
        // else if (TokenEqualsCSTR("double4")) element->Format = DXGI_FORMAT_R64G64B64A64_TYPELESS;

        else SyntaxError("unsupported struct member type: %s", token.name);

        GetNextToken();
        CheckToken(TOKEN_KIND_NAME);

        GetNextToken();
        CheckToken(TOKEN_KIND_COLON);

        GetNextToken();
        CheckToken(TOKEN_KIND_NAME);

        const char *digit_end   = token.end - 1;
        const char *digit_start = digit_end;

        while (isdigit(*digit_start)) --digit_start;

        if (isdigit(*digit_end))
        {
            ++digit_start;

            char buffer[8] = {0};
            sprintf(buffer, "%.*s", cast(s32, digit_end - digit_start), digit_start);

            element->SemanticIndex = atoi(buffer);
            element->SemanticName  = InternStringRange(token.start, digit_start);
        }
        else
        {
            element->SemanticIndex = 0;
            element->SemanticName  = token.name;
        }

        GetNextToken();
        CheckToken(TOKEN_KIND_SEMICOLON);
    }

    gpd->psd.input_layout.pInputElementDescs = ied;
}

internal void ParseInputLayout(Engine *engine, GraphicsProgramDesc *gpd)
{
    ShaderDesc *shader_desc = gpd->sd.descs + gpd->sd.count;

    u64 entry_point_len = shader_desc->entry_point ? strlen(shader_desc->entry_point) : 0;

    if (shader_desc->target
    &&  *shader_desc->target == 'v' // hack for checking is it a vertex shader
    &&  TokenEquals(shader_desc->entry_point, entry_point_len))
    {
        Token       first_arg_token       = {0};
        const char *first_arg_stream      = 0;
        gpd->psd.input_layout.NumElements = 0;

        while (token.kind != TOKEN_KIND_RPAREN && token.kind != TOKEN_KIND_EOF)
        {
            if (token.kind == TOKEN_KIND_LPAREN)
            {
                GetNextToken();
                first_arg_token  = token;
                first_arg_stream = stream;
            }
            else if (token.kind == TOKEN_KIND_COMMA)
            {
                ++gpd->psd.input_layout.NumElements;
                GetNextToken();
            }
            else
            {
                GetNextToken();
            }
        }

        if (first_arg_token.kind == TOKEN_KIND_RPAREN)
        {
            gpd->psd.input_layout.pInputElementDescs = PushToTA(D3D12_INPUT_ELEMENT_DESC, engine->memory, 1);
        }
        else if (gpd->psd.input_layout.NumElements)
        {
            token  = first_arg_token;
            stream = first_arg_stream;

            ParseInputLayoutFromArgs(engine, gpd);
        }
        else if (token.kind != TOKEN_KIND_EOF)
        {
            token  = first_arg_token;
            stream = first_arg_stream;

            ParseInputLayoutFromStruct(engine, gpd);
        }
        else
        {
            SyntaxError("unexpected end of file in vertex shader's entry point's arguments");
        }
    }
}

// @Important(Roman): NOT INTERNAL!!!
// Used in: "renderer.h"
void ParseShaders(Engine *engine, const char *file_with_shaders, GraphicsProgramDesc *gpd)
{
    char *shader_code = ReadEntireShaderFile(engine, file_with_shaders, 0);

    InitInterns(engine);
    InitKeywords();
    InitStream(shader_code);

    while (token.kind != TOKEN_KIND_EOF)
    {
        switch (token.kind)
        {
            case TOKEN_KIND_HASH:
            {
                ParsePreprocessor(gpd);
            } break;

            case TOKEN_KIND_NAME:
            {
                if (!gpd->psd.input_layout.pInputElementDescs)
                {
                    ParseInputLayout(engine, gpd);
                }
            } break;

            case TOKEN_KIND_KEYWORD:
            {
                // @TODO(Roman): parse CBV, SRV, UAV, ...
            } break;
        };

        GetNextToken();
    }

    ShaderDesc *last_shader = gpd->sd.descs + gpd->sd.count;
    if (last_shader->code_start)
    {
        if (!last_shader->code_end)
        {
            last_shader->code_end = stream;
        }
        ++gpd->sd.count;
    }

    DestroyInterns();
}
