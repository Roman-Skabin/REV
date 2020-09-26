//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"
#include "core/memory.h"
#include "core/allocator.h"
#include "core/window.h"
#include "core/input.h"
#include "core/level.h"

#include "tools/logger.h"
#include "tools/work_queue.h"
#include "tools/timer.h"
#include "tools/buffer.hpp"

class ENGINE_IMPEXP Application
{
public:
    static Application *Get();

protected:
    explicit Application(const char *name = "Application");

public:
    virtual ~Application();

    template<typename ...LevelsPointers, typename = RTTI::enable_if_t<RTTI::are_base_of_v<Level, RTTI::remove_pointer_t<LevelsPointers>...>>>
    void PushLevels(LevelsPointers... levels)
    {
        m_Levels.PushBack(cast<Level *>(levels)...);
        (..., levels->OnAttach());
    }

    template<typename ...LevelsPointers, typename = RTTI::enable_if_t<RTTI::are_base_of_v<Level, RTTI::remove_pointer_t<LevelsPointers>...>>>
    void PopLevels(LevelsPointers... levels)
    {
        (..., m_Levels.Erase(m_Levels.Find(levels)));
        (..., levels->OnDetach());
    }

    const Memory           *GetMemory()    const { return m_Memory;    }
    const Allocator&        GetAllocator() const { return m_Allocator; }
    const WorkQueue        *GetWorkQueue() const { return m_WorkQueue; }
    const Window&           GetWindow()    const { return m_Window;    }
    const Input            *GetInput()     const { return m_Input;     }
    const Timer&            GetTimer()     const { return m_Timer;     }
    const Buffer<Level *>&  GetLevels()    const { return m_Levels;    }

    Memory           *GetMemory()    { return m_Memory;    }
    Allocator&        GetAllocator() { return m_Allocator; }
    WorkQueue        *GetWorkQueue() { return m_WorkQueue; }
    Window&           GetWindow()    { return m_Window;    }
    Input            *GetInput()     { return m_Input;     }
    Timer&            GetTimer()     { return m_Timer;     }
    Buffer<Level *>&  GetLevels()    { return m_Levels;    }

private:
    void Run();

    Application(const Application&)  = delete;
    Application(Application&&)       = delete;

    Application& operator=(const Application&) = delete;
    Application& operator=(Application&&)      = delete;

private:
    Logger            m_Logger;

protected:
    Memory          *m_Memory;
    Allocator        m_Allocator;
    WorkQueue       *m_WorkQueue;
    Window           m_Window;
    Input           *m_Input;
    Timer            m_Timer;
    Buffer<Level *>  m_Levels;

    static Application *s_Application;

private:
    friend int main(int argc, char **argv);
};
