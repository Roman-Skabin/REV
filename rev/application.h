//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/common.h"
#include "core/memory.h"
#include "core/allocator.h"
#include "core/work_queue.h"
#include "core/window.h"
#include "core/input.h"
#include "core/scene.h"

#include "graphics/graphics_api.h"

#include "tools/logger.h"
#include "tools/timer.h"
#include "tools/array.hpp"
#include "tools/static_string.hpp"

int main(int argc, char **argv);

namespace REV
{
    class REV_API Application
    {
    public:
        static Application *Get();

    protected:
        explicit Application(
            const StaticString<128>& name = StaticString<128>("Application", REV_CSTRLEN("Application")),
            GraphicsAPI::API         api  = GraphicsAPI::API::D3D12
        );

    public:
        virtual ~Application();

        const Memory       *GetMemory()       const { return m_Memory;       }
        const Allocator&    GetAllocator()    const { return m_Allocator;    }
        const WorkQueue    *GetWorkQueue()    const { return m_WorkQueue;    }
        const Window&       GetWindow()       const { return m_Window;       }
        const Input        *GetInput()        const { return m_Input;        }
        const Timer&        GetTimer()        const { return m_Timer;        }
        const SceneManager *GetSceneManager() const { return m_SceneManager; }

        Memory       *GetMemory()       { return m_Memory;       }
        Allocator&    GetAllocator()    { return m_Allocator;    }
        WorkQueue    *GetWorkQueue()    { return m_WorkQueue;    }
        Window&       GetWindow()       { return m_Window;       }
        Input        *GetInput()        { return m_Input;        }
        Timer&        GetTimer()        { return m_Timer;        }
        SceneManager *GetSceneManager() { return m_SceneManager; }

    private:
        void Run();

        Application(const Application&)  = delete;
        Application(Application&&)       = delete;

        Application& operator=(const Application&) = delete;
        Application& operator=(Application&&)      = delete;

    private:
        Logger        m_Logger;

    protected:
        Memory       *m_Memory;
        Allocator     m_Allocator;
        WorkQueue    *m_WorkQueue;
        Window        m_Window;
        Input        *m_Input;
        Timer         m_Timer;
        SceneManager *m_SceneManager;

    private:
        static Application *s_Application;

        friend int ::main(int argc, char **argv);
    };
}
