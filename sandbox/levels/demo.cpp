#include "levels/demo.h"
#include "application.h"

DemoLevel::DemoLevel(const Logger& logger)
    : Level("DemoLevel"),
      m_Application(Application::Get()),
      m_Logger(logger, "DemoLevel logger")
{
}

DemoLevel::~DemoLevel()
{
}

void DemoLevel::OnAttach()
{
    m_Logger.LogInfo("%s has been attached", m_Name);
}

void DemoLevel::OnDetach()
{
    m_Logger.LogInfo("%s has been detached", m_Name);
}

void DemoLevel::OnUpdateAndRender()
{
    const Keyboard& keyboard = m_Application->GetInput()->GetKeyboard();
    Window&         window   = m_Application->GetWindow();

    if (keyboard[KEY::F11].Pressed())
    {
        window.RequstFullscreen(!window.Fullscreened());
    }
}
