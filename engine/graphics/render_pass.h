// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "graphics/shader_manager.h"
#include "tools/static_string.hpp"
#include "tools/chunk_list.hpp"

namespace REV
{
    enum RT_DS_LOAD_ACTION : u32
    {
        RT_DS_LOAD_ACTION_NONE,  // D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD
        RT_DS_LOAD_ACTION_LOAD,  // D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE
        RT_DS_LOAD_ACTION_CLEAR, // D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR
    };

    enum RT_DS_STORE_ACTION : u32
    {
        RT_DS_STORE_ACTION_NONE,    // D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD
        RT_DS_STORE_ACTION_STORE,   // D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE
        RT_DS_STORE_ACTION_RESOLVE, // D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE
    };

    class REV_API RenderPass final
    {
    public:
        struct RTDesc
        {
            ResourceHandle     resource_handle;
            RT_DS_LOAD_ACTION  load_action  = RT_DS_LOAD_ACTION_NONE;
            RT_DS_STORE_ACTION store_action = RT_DS_STORE_ACTION_NONE;
            Math::v4           clear_value;
        };

        struct DSDesc
        {
            ResourceHandle     resource_handle;
            RT_DS_LOAD_ACTION  depth_load_action    = RT_DS_LOAD_ACTION_NONE;
            RT_DS_STORE_ACTION depth_store_action   = RT_DS_STORE_ACTION_NONE;
            f32                depth_clear_value    = 0.0f;
            RT_DS_LOAD_ACTION  stencil_load_action  = RT_DS_LOAD_ACTION_NONE;
            RT_DS_STORE_ACTION stencil_store_action = RT_DS_STORE_ACTION_NONE;
            u8                 stencil_clear_value  = 0;
        };

        struct Output
        {
            ConstArray<RTDesc> render_target_descs;
            ConstArray<DSDesc> depth_stencil_descs;
        };

    public:
        RenderPass(const ConstString& name);
        ~RenderPass();

        void Render();

        REV_INLINE const ConstString& GetName()   const { return m_Name;   }
        REV_INLINE const Output&      GetOutput() const { return m_Output; }

    private:
        REV_DELETE_CONSTRS_AND_OPS(RenderPass);

    private:
        ConstString m_Name;
        Output      m_Output;
    };
}
