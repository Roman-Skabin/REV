//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "application.h"

Application *Application::s_Application = null;

Application::Application(const char *name)
    : m_Logger("Engine logger", "../log/cengine.log", Logger::TARGET::FILE),
      m_Memory(Memory::Get()),
      m_Allocator(m_Memory->PushToPermanentArea(GB(1)), GB(1), false),
      m_WorkQueue(WorkQueue::Create(m_Logger)),
      m_Window(m_Logger,
               null,
               name,
               v2s(960, 540),
               v2s(10, 10)),
      m_Input(Input::Create(m_Window, m_Logger)),
      m_Timer("EngineMainTimer", CSTRLEN("EngineMainTimer"))
{
    CheckM(!s_Application,
           "Only one application alowed. "
           "Application \"%s\" is already created",
           s_Application->m_Window.m_Title);
    s_Application = this;
}

Application::~Application()
{
    s_Application = null;
}

void Application::Run()
{
    m_Timer.Start();

    m_Window.Show();
    while (!m_Window.Closed())
    {
        m_Memory->ResetTransientArea();
        m_Window.Resset();
        m_Input->Reset();

        m_Window.PollEvents();

        if (!m_Window.Closed())
        {
            m_Input->Update(m_Logger);
            m_Timer.Tick();
            m_Window.ApplyFullscreenRequst();

            // Start Frame
            // for (auto& level : m_Levels) level.OnUpdate(...);
            // End Frame
        }
    }

    m_Timer.Stop();
}
