// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#include "core/pch.h"
#include "application.h"
#include "memory/memory.h"

namespace REV
{

Application *Application::s_Application = null;

Application *Application::Get()
{
    REV_CHECK_M(s_Application, "Application is not created yet");
    return s_Application;
}

Application::Application(const ConstString& name, const ConstString& ini_filename)
    : m_Logger(ConstString(REV_CSTR_ARGS("REV logger")), ConstString(REV_CSTR_ARGS("../../log/rev.log")), Logger::TARGET_FILE),
      m_Allocator(Memory::Get()->PushToPermanentArena(GB(1)), GB(1), false, ConstString(REV_CSTR_ARGS("Default"))),
      m_WorkQueue(m_Logger, Memory::Get()->PermanentArena()),
      m_Settings(Settings::Init(ini_filename)),
      m_Window(m_Logger, name),
      m_Input(Input::Create(m_Window, m_Logger)),
      m_Timer(ConstString(REV_CSTR_ARGS("REVMainTimer"))),
      m_CurrentScene(null),
      m_AssetManager(null)
{
    REV_CHECK_M(!s_Application,
                "Only one application alowed. "
                "Application \"%s\" is already created",
                s_Application->m_Window.m_Title);
    s_Application = this;

    GraphicsAPI::SetGraphicsAPI(m_Settings->graphics_api);
    GraphicsAPI::Init(&m_Window, &m_Allocator, m_Logger);

    // @NOTE(Roman): AssetManager must be created after Graphics API initializeation.
    m_AssetManager = AssetManager::Create(&m_Allocator, m_Logger);
}

Application::~Application()
{
    m_AssetManager->~AssetManager();
    GraphicsAPI::Destroy();
    s_Application = null;
}

void Application::SetCurrentScene(SceneBase *scene)
{
    if (m_CurrentScene) m_CurrentScene->OnUnsetCurrentEx();
    m_CurrentScene = scene;
    m_CurrentScene->OnSetCurrentEx();
}

void Application::Run(SceneBase *scene)
{
    GPU::DeviceContext *device_context = GraphicsAPI::GetDeviceContext();

    m_Timer.Start();

    m_Window.Show();
    while (!m_Window.Closed())
    {
        m_WorkQueue.AddWork([    ] { Memory::Get()->ResetFrameArena(); });
        m_WorkQueue.AddWork([this] { m_Window.Resset();                });
        m_WorkQueue.AddWork([this] { m_Input->Reset();                 });
        m_WorkQueue.Wait();

        m_Window.PollEvents();

        if (!m_Window.Closed() && !m_Window.Minimized())
        {
            m_Input->Update(m_Logger);
            m_Timer.Tick();
            m_Window.ApplyFullscreenRequest();

            device_context->StartFrame();
            if (!m_CurrentScene)
            {
                SetCurrentScene(scene);
            }
            m_CurrentScene->OnUpdate();
            m_CurrentScene->FlushBatch();
            
            device_context->EndFrame();
        }
    }

    m_WorkQueue.AddWork([this          ]{ m_Timer.Stop();               });
    m_WorkQueue.AddWork([device_context]{ device_context->WaitForGPU(); });
    m_WorkQueue.Wait();

    m_CurrentScene->OnUnsetCurrentEx();
}

}
