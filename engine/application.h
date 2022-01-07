// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "core/allocator.h"
#include "core/work_queue.h"
#include "core/window.h"
#include "core/input.h"
#include "core/scene.h"
#include "core/settings.h"

#include "graphics/graphics_api.h"

#include "tools/logger.h"
#include "tools/timer.h"
#include "tools/static_string.hpp"

#include "asset_manager/asset_manager.h"

int REV_CDECL main(int argc, char **argv);

namespace REV
{
    class REV_API Application
    {
    public:
        static Application *Get();

    protected:
        explicit Application(const ConstString& name, const ConstString& ini_filename);

    public:
        virtual ~Application();

        void SetCurrentScene(SceneBase *scene);

        REV_INLINE const Logger&       GetLogger()       const { return m_Logger;       }
        REV_INLINE const Allocator&    GetAllocator()    const { return m_Allocator;    }
        REV_INLINE const WorkQueue&    GetWorkQueue()    const { return m_WorkQueue;    }
        REV_INLINE const Window&       GetWindow()       const { return m_Window;       }
        REV_INLINE const Input        *GetInput()        const { return m_Input;        }
        REV_INLINE const Timer&        GetTimer()        const { return m_Timer;        }
        REV_INLINE const SceneBase    *GetCurrentScene() const { return m_CurrentScene; }
        REV_INLINE const AssetManager *GetAssetManager() const { return m_AssetManager; }
        REV_INLINE const Settings     *GetSettings()     const { return m_Settings;     }

        REV_INLINE Logger&       GetLogger()       { return m_Logger;       }
        REV_INLINE Allocator&    GetAllocator()    { return m_Allocator;    }
        REV_INLINE WorkQueue&    GetWorkQueue()    { return m_WorkQueue;    }
        REV_INLINE Window&       GetWindow()       { return m_Window;       }
        REV_INLINE Input        *GetInput()        { return m_Input;        }
        REV_INLINE Timer&        GetTimer()        { return m_Timer;        }
        REV_INLINE SceneBase    *GetCurrentScene() { return m_CurrentScene; }
        REV_INLINE AssetManager *GetAssetManager() { return m_AssetManager; }
        REV_INLINE Settings     *GetSettings()     { return m_Settings;     }

    private:
        void Run(SceneBase *scene);

        REV_DELETE_CONSTRS_AND_OPS(Application);

    protected:
        Allocator     m_Allocator;
        Settings     *m_Settings;
        Logger        m_Logger;
        WorkQueue     m_WorkQueue;
        Window        m_Window;
        Input        *m_Input;
        Timer         m_Timer;
        SceneBase    *m_CurrentScene;
        AssetManager *m_AssetManager;
        ForwardPlusPipeline m_ForwardPlusPipeline;

    private:
        static Application *s_Application;

        friend int REV_CDECL ::main(int argc, char **argv);
    };
}
