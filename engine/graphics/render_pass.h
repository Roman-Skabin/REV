// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "graphics/shader_manager.h"

namespace REV
{
    enum RENDER_PASS_KIND : u32
    {
        // Common
        RENDER_PASS_KIND_OPAQUE,
        RENDER_PASS_KIND_TRANSPARENT,
        RENDER_PASS_KIND_SEA,
        RENDER_PASS_KIND_SKYBOX,
        RENDER_PASS_KIND_DEPTH,
        RENDER_PASS_KIND_UI,

        // Forward+ pipeline
        RENDER_PASS_KIND_COMPUTE_LIGHT_GRID,

        // @TODO(Roman): More passes
        RENDER_PASS_KIND_MAX
    };

    enum RENDER_PASS_FLAG : u32
    {
        RENDER_PASS_FLAG_NONE    = 0,
        RENDER_PASS_FLAG_ENABLED = 1 << 0,
    };
    REV_ENUM_OPERATORS(RENDER_PASS_FLAG);

    // @TODO(Roman): RT & DS load actions:
    //                   LOAD_ACTION_NONE
    //                   LOAD_ACTION_LOAD
    //                   LOAD_ACTION_CLEAR
    //               RT & DS store actions:
    //                   STORE_ACTION_NONE
    //                   STORE_ACTION_STORE

    struct RenderSubPass
    {
        GPU::ShaderHandle   shader_handle;
        GPU::ResourceHandle vbuffer_handle;
        GPU::ResourceHandle ibuffer_handle;
    };

    class REV_API RenderPass
    {
    public:
        RenderPass(Allocator *allocator, RENDER_PASS_KIND kind);
        ~RenderPass();

        void LoadResources();
        void StoreResources();

        void Render();

        void AddSubPass(
            const GPU::ShaderHandle&   shader_handle,
            const GPU::ResourceHandle& vbuffer_handle,
            const GPU::ResourceHandle& ibuffer_handle
        );

        REV_INLINE void ResetSubPasses() { m_SubPasses.Clear(); }

        REV_INLINE RENDER_PASS_KIND Kind() const { return m_Kind; }
        ConstString KindName() const;

        REV_INLINE void Enable()  const { m_Flags |=  RENDER_PASS_FLAG_ENABLED; }
        REV_INLINE void Disable() const { m_Flags &= ~RENDER_PASS_FLAG_ENABLED; }

        REV_INLINE bool Enabled() const { return m_Flags & RENDER_PASS_FLAG_ENABLED; }

        REV_INLINE const Array<RenderSubPass>&       SubPasses() const { return m_SubPasses; }
        REV_INLINE const Array<GPU::ResourceHandle>& Input()     const { return m_Input;     }
        REV_INLINE const Array<GPU::ResourceHandle>& Output()    const { return m_Output;    }

    private:
        REV_DELETE_CONSTRS_AND_OPS(RenderPass);

    private:
        Array<RenderSubPass>       m_SubPasses;
        Array<GPU::ResourceHandle> m_Input;
        Array<GPU::ResourceHandle> m_Output;
        RENDER_PASS_KIND           m_Kind;
        RENDER_PASS_FLAG           m_Flags;
    };
}
