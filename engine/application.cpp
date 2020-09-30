//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "application.h"

Application *Application::s_Application = null;

Application *Application::Get()
{
    CheckM(s_Application, "Application is not created yet");
    return s_Application;
}

Application::Application(const char *name, GRAPHICS_API api)
    : m_Logger("Engine logger", "../log/cengine.log", Logger::TARGET::FILE),
      m_Memory(Memory::Get()),
      m_Allocator(m_Memory->PushToPermanentArea(GB(1)), GB(1), false),
      m_WorkQueue(WorkQueue::Create(m_Logger)),
      m_Window(m_Logger,
               name,
               v2s(960, 540),
               v2s(10, 10)),
      m_Input(Input::Create(m_Window, m_Logger)),
      m_Timer("EngineMainTimer", CSTRLEN("EngineMainTimer")),
      m_Levels(&m_Allocator),
      m_GPUManager(CreateGPUManager(api, &m_Window, m_Logger))
{
    CheckM(!s_Application,
           "Only one application alowed. "
           "Application \"%s\" is already created",
           s_Application->m_Window.m_Title);
    s_Application = this;
}

Application::~Application()
{
    if (m_GPUManager) m_GPUManager->Destroy();
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

        if (!m_Window.Closed() && !m_Window.Minimized())
        {
            m_Input->Update(m_Logger);
            m_Timer.Tick();
            m_Window.ApplyFullscreenRequst();

            m_GPUManager->StartFrame();
            for (Level *level : m_Levels)
            {
                level->OnUpdateAndRender();
            }
            m_GPUManager->EndFrame();
        }
    }

    m_Timer.Stop();
}
