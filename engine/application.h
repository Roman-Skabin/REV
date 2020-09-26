//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"
#include "core/memory.h"
#include "core/allocator.h"
#include "core/window.h"
#include "core/input.h"

#include "tools/logger.h"
#include "tools/work_queue.h"
#include "tools/timer.h"

class ENGINE_IMPEXP Application
{
protected:
    explicit Application(const char *name);

public:
    virtual ~Application();

private:
    void Run();

    Application(const Application&)  = delete;
    Application(Application&&)       = delete;

    Application& operator=(const Application&) = delete;
    Application& operator=(Application&&)      = delete;

private:
    Logger     m_Logger;

protected:
    Memory    *m_Memory;
    Allocator  m_Allocator;
    WorkQueue *m_WorkQueue;
    Window     m_Window;
    Input     *m_Input;
    Timer      m_Timer;

    static Application *s_Application;

private:
    friend int main(int argc, char **argv);
};
