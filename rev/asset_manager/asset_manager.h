//
// Copyright 2020-2021 Roman Skabin
//

#pragma once

#include "tools/array.hpp"
#include "tools/const_string.h"
#include "graphics/memory_manager.h"

namespace REV
{
    #pragma warning(push)
    #pragma warning(disable: 4200)
    struct AssetName
    {
        u64  length;
        char data[0];
    };

    // @NOTE(Roman): layout: [count|length_1|data_1|length_2|data_2|...|length_count|data_count]
    struct AssetNames
    {
        u64  count;
        byte names[0]; // cast to AssetName *
    };
    #pragma warning(pop)

    struct Asset
    {
        // @NOTE(Roman): names need to be freed.
        AssetNames *names;
        GPU::ResourceHandle resource;
    };

    class REV_API AssetManager final
    {
    public:
        static AssetManager *Create(Allocator *allocator, const char *REVAM_filename);
        static AssetManager *Get();

        ~AssetManager();

        void ParseREVAMFile(const ConstString& scene_name);
        void FreeSceneAssets();

    private:
        AssetManager(Allocator *allocator, const char *RAF);

        void ParseREVAMFile();
        void ParseExternalBlock(void *, Array<Asset> *area);

        void ParseTexture(Asset *asset, const char *filename, u64 filename_length);
        void CreateDDSTexture(Asset *asset, byte *data, u64 data_size);

        REV_DELETE_CONSTRS_AND_OPS(AssetManager);

    private:
        Allocator    *m_Allocator;
        const char   *m_UserREVAMFileName;
        const char   *m_UserREVAMStream;
        Array<Asset>  m_StaticArea;
        Array<Asset>  m_SceneArea;
    };
}
