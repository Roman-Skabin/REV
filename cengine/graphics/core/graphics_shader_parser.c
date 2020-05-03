//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "graphics/core/shader_parser.h"
#include "cengine.h"

#define TokenEquals(str, len)  ((parser->lexer.token.end - parser->lexer.token.start == (len)) && !memcmp(parser->lexer.token.start, str, len))
#define TokenEqualsCSTR(cstr)  TokenEquals(cstr, CSTRLEN(cstr))

#define SyntaxError(format, ...) MessageF(MESSAGE_TYPE_ERROR, CSTRCAT("Shader syntax error: ", format), __VA_ARGS__)

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

internal void ParsePreprocessor(ShaderParser *parser, GraphicsProgramDesc *gpd)
{
    GetNextGraphicsShaderToken(&parser->lexer);
    CheckToken(&parser->lexer, TOKEN_KIND_KEYWORD);

    if (TokenEqualsCSTR("cengine"))
    {
        GetNextGraphicsShaderToken(&parser->lexer);
        CheckToken(&parser->lexer, TOKEN_KIND_KEYWORD);

        if (TokenEqualsCSTR("blending"))
        {
            GetNextGraphicsShaderToken(&parser->lexer);
            CheckToken(&parser->lexer, TOKEN_KIND_LPAREN);

            GetNextGraphicsShaderToken(&parser->lexer);
            CheckToken(&parser->lexer, TOKEN_KIND_KEYWORD);

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
                SyntaxError("undefined blending option: %s", parser->lexer.token.name);
            }

            GetNextGraphicsShaderToken(&parser->lexer);
            CheckToken(&parser->lexer, TOKEN_KIND_RPAREN);
        }
        else if (TokenEqualsCSTR("depth_test"))
        {
            GetNextGraphicsShaderToken(&parser->lexer);
            CheckToken(&parser->lexer, TOKEN_KIND_LPAREN);

            GetNextGraphicsShaderToken(&parser->lexer);
            CheckToken(&parser->lexer, TOKEN_KIND_KEYWORD);

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
                SyntaxError("undefined depth test option: %s", parser->lexer.token.name);
            }

            GetNextGraphicsShaderToken(&parser->lexer);
            CheckToken(&parser->lexer, TOKEN_KIND_RPAREN);
        }
        else if (TokenEqualsCSTR("shader"))
        {
            ShaderDesc *shader_desc = gpd->sd.descs + gpd->sd.count;

            GetNextGraphicsShaderToken(&parser->lexer);
            CheckToken(&parser->lexer, TOKEN_KIND_LPAREN);

            GetNextGraphicsShaderToken(&parser->lexer);
            CheckToken(&parser->lexer, TOKEN_KIND_KEYWORD);

            if (TokenEqualsCSTR("vertex"))
            {
                if (shader_desc->code_start)
                {
                    ++gpd->sd.count;

                    const char *shader_code_end = parser->lexer.stream;
                    while (*shader_code_end != '#') --shader_code_end;

                    shader_desc->code_end = shader_code_end;

                    shader_desc = gpd->sd.descs + gpd->sd.count;
                }

                shader_desc->target      = "vs_5_0";
                shader_desc->entry_point = "VSMain";

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
            else if (TokenEqualsCSTR("hull"))
            {
                if (shader_desc->code_start)
                {
                    ++gpd->sd.count;
    
                    const char *shader_code_end = parser->lexer.stream;
                    while (*shader_code_end != '#') --shader_code_end;
    
                    shader_desc->code_end = shader_code_end;
    
                    shader_desc = gpd->sd.descs + gpd->sd.count;
                }

                shader_desc->target      = "hs_5_0";
                shader_desc->entry_point = "HSMain";

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
            else if (TokenEqualsCSTR("domain"))
            {
                if (shader_desc->code_start)
                {
                    ++gpd->sd.count;
    
                    const char *shader_code_end = parser->lexer.stream;
                    while (*shader_code_end != '#') --shader_code_end;
    
                    shader_desc->code_end = shader_code_end;
    
                    shader_desc = gpd->sd.descs + gpd->sd.count;
                }

                shader_desc->target      = "ds_5_0";
                shader_desc->entry_point = "DSMain";

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
            else if (TokenEqualsCSTR("geometry"))
            {
                if (shader_desc->code_start)
                {
                    ++gpd->sd.count;
    
                    const char *shader_code_end = parser->lexer.stream;
                    while (*shader_code_end != '#') --shader_code_end;
    
                    shader_desc->code_end = shader_code_end;
    
                    shader_desc = gpd->sd.descs + gpd->sd.count;
                }

                shader_desc->target      = "gs_5_0";
                shader_desc->entry_point = "GSMain";

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
            else if (TokenEqualsCSTR("pixel"))
            {
                if (shader_desc->code_start)
                {
                    ++gpd->sd.count;
    
                    const char *shader_code_end = parser->lexer.stream;
                    while (*shader_code_end != '#') --shader_code_end;
    
                    shader_desc->code_end = shader_code_end;

                    shader_desc = gpd->sd.descs + gpd->sd.count;
                }

                shader_desc->target      = "ps_5_0";
                shader_desc->entry_point = "PSMain";

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
            else if (TokenEqualsCSTR("entry_point"))
            {
                GetNextGraphicsShaderToken(&parser->lexer);
                CheckToken(&parser->lexer, TOKEN_KIND_COMMA);

                GetNextGraphicsShaderToken(&parser->lexer);
                CheckToken(&parser->lexer, TOKEN_KIND_NAME);

                shader_desc->entry_point = parser->lexer.token.name;

                GetNextGraphicsShaderToken(&parser->lexer);
                CheckToken(&parser->lexer, TOKEN_KIND_RPAREN);

                shader_desc->code_start = parser->lexer.stream + 1;
            }
            else
            {
                SyntaxError("undefined shader type: %s", parser->lexer.token.name);
            }
        }
    }
}

internal void ParseInputLayoutFromArgs(Engine *engine, ShaderParser *parser, GraphicsProgramDesc *gpd)
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
            GetNextGraphicsShaderToken(&parser->lexer);
        }
        CheckToken(&parser->lexer, TOKEN_KIND_NAME);

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

        else SyntaxError("unsupported arg type: %s", parser->lexer.token.name);

        GetNextGraphicsShaderToken(&parser->lexer);
        CheckToken(&parser->lexer, TOKEN_KIND_NAME);

        GetNextGraphicsShaderToken(&parser->lexer);
        CheckToken(&parser->lexer, TOKEN_KIND_COLON);

        GetNextGraphicsShaderToken(&parser->lexer);
        CheckToken(&parser->lexer, TOKEN_KIND_NAME);

        const char *digit_end   = parser->lexer.token.end - 1;
        const char *digit_start = digit_end;

        while (isdigit(*digit_start)) --digit_start;

        if (isdigit(*digit_end))
        {
            ++digit_start;

            char buffer[8] = {0};
            sprintf(buffer, "%.*s", cast(s32, digit_end - digit_start), digit_start);

            element->SemanticIndex = atoi(buffer);
            element->SemanticName  = InternStringRange(&parser->lexer.interns, parser->lexer.token.start, digit_start);
        }
        else
        {
            element->SemanticIndex = 0;
            element->SemanticName  = parser->lexer.token.name;
        }

        GetNextGraphicsShaderToken(&parser->lexer);

        if (i < gpd->psd.input_layout.NumElements - 1)
        {
            CheckToken(&parser->lexer, TOKEN_KIND_COMMA);
            GetNextGraphicsShaderToken(&parser->lexer);
        }
        else
        {
            CheckToken(&parser->lexer, TOKEN_KIND_RPAREN);
        }
    }

    gpd->psd.input_layout.pInputElementDescs = ied;
}

internal void ParseInputLayoutFromStruct(Engine *engine, ShaderParser *parser, GraphicsProgramDesc *gpd)
{
    ShaderDesc *shader_desc = gpd->sd.descs + gpd->sd.count;

    if (TokenEqualsCSTR("in")  || TokenEqualsCSTR("inout")
    ||  TokenEqualsCSTR("out") || TokenEqualsCSTR("uniform"))
    {
        GetNextGraphicsShaderToken(&parser->lexer);
    }
    CheckToken(&parser->lexer, TOKEN_KIND_NAME);

    Token       first_arg_token  = parser->lexer.token;
    const char *first_arg_stream = parser->lexer.stream;

    parser->lexer.stream = shader_desc->code_start;
    GetNextGraphicsShaderToken(&parser->lexer);

    find_next_struct:
    {
        while (!TokenEqualsCSTR("struct") && parser->lexer.token.kind != TOKEN_KIND_EOF)
        {
            GetNextGraphicsShaderToken(&parser->lexer);
        }

        GetNextGraphicsShaderToken(&parser->lexer);
        CheckToken(&parser->lexer, TOKEN_KIND_NAME);

        if (!TokenEquals(first_arg_token.start, first_arg_token.end - first_arg_token.start)
        &&  parser->lexer.token.kind != TOKEN_KIND_EOF)
        {
            goto find_next_struct;
        }
    }

    if (parser->lexer.token.kind == TOKEN_KIND_EOF)
    {
        SyntaxError("unexpected end of file in vertex shader");
    }

    GetNextGraphicsShaderToken(&parser->lexer);
    CheckToken(&parser->lexer, TOKEN_KIND_LBRACE);

    GetNextGraphicsShaderToken(&parser->lexer);
    CheckToken(&parser->lexer, TOKEN_KIND_NAME);

    if (TokenEqualsCSTR("uniform"))
    {
        GetNextGraphicsShaderToken(&parser->lexer);
        CheckToken(&parser->lexer, TOKEN_KIND_NAME);
    }

    Token       first_struct_member_token  = parser->lexer.token;
    const char *first_struct_member_stream = parser->lexer.stream;

    while (parser->lexer.token.kind != TOKEN_KIND_RBRACE && parser->lexer.token.kind != TOKEN_KIND_EOF)
    {
        if (parser->lexer.token.kind == TOKEN_KIND_SEMICOLON)
        {
            ++gpd->psd.input_layout.NumElements;
        }
        GetNextGraphicsShaderToken(&parser->lexer);
    }

    if (parser->lexer.token.kind == TOKEN_KIND_EOF)
    {
        SyntaxError("unexpected end of line in vertex shader's entry struct's members");
    }

    parser->lexer.token  = first_struct_member_token;
    parser->lexer.stream = first_struct_member_stream;

    D3D12_INPUT_ELEMENT_DESC *ied = PushToTA(D3D12_INPUT_ELEMENT_DESC, engine->memory, gpd->psd.input_layout.NumElements);

    for (u32 i = 0; i < gpd->psd.input_layout.NumElements; ++i)
    {
        if (i > 0) GetNextGraphicsShaderToken(&parser->lexer);

        D3D12_INPUT_ELEMENT_DESC *element = ied + i;
        element->InputSlot                = 0;
        element->AlignedByteOffset        = i ? D3D12_APPEND_ALIGNED_ELEMENT : 0;
        element->InputSlotClass           = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        element->InstanceDataStepRate     = 0;

        if (TokenEqualsCSTR("uniform"))
        {
            GetNextGraphicsShaderToken(&parser->lexer);
        }
        CheckToken(&parser->lexer, TOKEN_KIND_NAME);

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

        else SyntaxError("unsupported struct member type: %s", parser->lexer.token.name);

        GetNextGraphicsShaderToken(&parser->lexer);
        CheckToken(&parser->lexer, TOKEN_KIND_NAME);

        GetNextGraphicsShaderToken(&parser->lexer);
        CheckToken(&parser->lexer, TOKEN_KIND_COLON);

        GetNextGraphicsShaderToken(&parser->lexer);
        CheckToken(&parser->lexer, TOKEN_KIND_NAME);

        const char *digit_end   = parser->lexer.token.end - 1;
        const char *digit_start = digit_end;

        while (isdigit(*digit_start)) --digit_start;

        if (isdigit(*digit_end))
        {
            ++digit_start;

            char buffer[8] = {0};
            sprintf(buffer, "%.*s", cast(s32, digit_end - digit_start), digit_start);

            element->SemanticIndex = atoi(buffer);
            element->SemanticName  = InternStringRange(&parser->lexer.interns, parser->lexer.token.start, digit_start);
        }
        else
        {
            element->SemanticIndex = 0;
            element->SemanticName  = parser->lexer.token.name;
        }

        GetNextGraphicsShaderToken(&parser->lexer);
        CheckToken(&parser->lexer, TOKEN_KIND_SEMICOLON);
    }

    gpd->psd.input_layout.pInputElementDescs = ied;
}

internal void ParseInputLayout(Engine *engine, ShaderParser *parser, GraphicsProgramDesc *gpd)
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

        while (parser->lexer.token.kind != TOKEN_KIND_RPAREN && parser->lexer.token.kind != TOKEN_KIND_EOF)
        {
            if (parser->lexer.token.kind == TOKEN_KIND_LPAREN)
            {
                GetNextGraphicsShaderToken(&parser->lexer);
                first_arg_token  = parser->lexer.token;
                first_arg_stream = parser->lexer.stream;
            }
            else if (parser->lexer.token.kind == TOKEN_KIND_COMMA)
            {
                ++gpd->psd.input_layout.NumElements;
                GetNextGraphicsShaderToken(&parser->lexer);
            }
            else
            {
                GetNextGraphicsShaderToken(&parser->lexer);
            }
        }

        if (first_arg_token.kind == TOKEN_KIND_RPAREN)
        {
            gpd->psd.input_layout.pInputElementDescs = PushToTA(D3D12_INPUT_ELEMENT_DESC, engine->memory, 1);
        }
        else if (gpd->psd.input_layout.NumElements)
        {
            parser->lexer.token  = first_arg_token;
            parser->lexer.stream = first_arg_stream;

            ParseInputLayoutFromArgs(engine, parser, gpd);
        }
        else if (parser->lexer.token.kind != TOKEN_KIND_EOF)
        {
            parser->lexer.token  = first_arg_token;
            parser->lexer.stream = first_arg_stream;

            ParseInputLayoutFromStruct(engine, parser, gpd);
        }
        else
        {
            SyntaxError("unexpected end of file in vertex shader's entry point's arguments");
        }
    }
}

void ParseGraphicsShaders(Engine *engine, const char *file_with_shaders, GraphicsProgramDesc *gpd)
{
    char *shader_code = ReadEntireShaderFile(engine, file_with_shaders, 0);

    ShaderParser parser = {0};
    CreateGraphicsShaderLexer(engine, shader_code, &parser.lexer);

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
                if (!gpd->psd.input_layout.pInputElementDescs)
                {
                    ParseInputLayout(engine, &parser, gpd);
                }
            } break;

            case TOKEN_KIND_KEYWORD:
            {
                // @TODO(Roman): parse CBV, SRV, UAV, ...
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

    DestroyInterns(&parser.lexer.interns);
}
