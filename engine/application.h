//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"
#include "core/memory.h"
#include "core/allocator.h"
#include "core/work_queue.h"
#include "core/window.h"
#include "core/input.h"
#include "core/app_component.h"

#include "renderer/graphics_api.h"

#include "tools/logger.h"
#include "tools/timer.h"
#include "tools/buffer.hpp"
#include "tools/static_string.hpp"

class ENGINE_IMPEXP Application
{
public:
    static Application *Get();

protected:
    explicit Application(const StaticString<128>& name = "Application", GraphicsAPI::API api = GraphicsAPI::API::D3D12);

public:
    virtual ~Application();

    template<typename ...AppComponentPointers, typename = RTTI::enable_if_t<RTTI::are_base_of_v<AppComponent, RTTI::remove_pointer_t<AppComponentPointers>...>>>
    void AttacComponents(AppComponentPointers... components)
    {
        m_AppComponents.PushBack(cast<AppComponent *>(components)...);
        (..., components->OnAttach());
        (..., m_Logger.LogInfo("%s component has been attached", components->GetName()));
    }

    template<typename ...AppComponentPointers, typename = RTTI::enable_if_t<RTTI::are_base_of_v<AppComponent, RTTI::remove_pointer_t<AppComponentPointers>...>>>
    void DetachComponents(AppComponentPointers... components)
    {
        (..., components->OnDetach());
        (..., m_Logger.LogInfo("%s component has been detached", components->GetName()));
        (..., m_AppComponents.Erase(m_AppComponents.Find(components)));
    }

    void DetachAllComponents()
    {
        for (AppComponent *component : m_AppComponents)
        {
            component->OnDetach();
            m_Logger.LogInfo("%s component has been detached", component->GetName());
        }
        m_AppComponents.Clear();
    }

    const Memory                  *GetMemory()        const { return m_Memory;        }
    const Allocator&               GetAllocator()     const { return m_Allocator;     }
    const WorkQueue               *GetWorkQueue()     const { return m_WorkQueue;     }
    const Window&                  GetWindow()        const { return m_Window;        }
    const Input                   *GetInput()         const { return m_Input;         }
    const Timer&                   GetTimer()         const { return m_Timer;         }
    const Buffer<AppComponent *>&  GetAppComponents() const { return m_AppComponents; }

    Memory                  *GetMemory()        { return m_Memory;        }
    Allocator&               GetAllocator()     { return m_Allocator;     }
    WorkQueue               *GetWorkQueue()     { return m_WorkQueue;     }
    Window&                  GetWindow()        { return m_Window;        }
    Input                   *GetInput()         { return m_Input;         }
    Timer&                   GetTimer()         { return m_Timer;         }
    Buffer<AppComponent *>&  GetAppComponents() { return m_AppComponents; }

private:
    void Run();

    Application(const Application&)  = delete;
    Application(Application&&)       = delete;

    Application& operator=(const Application&) = delete;
    Application& operator=(Application&&)      = delete;

private:
    Logger                  m_Logger;

protected:
    Memory                 *m_Memory;
    Allocator               m_Allocator;
    WorkQueue              *m_WorkQueue;
    Window                  m_Window;
    Input                  *m_Input;
    Timer                   m_Timer;
    Buffer<AppComponent *>  m_AppComponents;

private:
    static Application *s_Application;

    friend int main(int argc, char **argv);
};
