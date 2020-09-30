#include "levels/demo.h"
#include "application.h"

DemoLevel::DemoLevel(const Logger& logger)
    : Level("DemoLevel"),
      m_Application(Application::Get()),
      m_Logger(logger, "DemoLevel logger")
{
    strcpy(m_OriginalWindowTitle, m_Application->GetWindow().Title());
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
    Timer&          timer    = m_Application->GetTimer();

    local f32 last_print_time;
    if (!window.Fullscreened() && timer.Seconds() - last_print_time >= 0.1)
    {
        f32 FPS = timer.TicksPerSecond() / cast<f32>(timer.DeltaTicks());

        char buffer[64];
        sprintf(buffer, "%s - FPS: %f - MSPF: %f", m_OriginalWindowTitle, FPS, 1000.0f / FPS);

        window.SetTitle(buffer);

        last_print_time = timer.Seconds();
    }

    if (keyboard[KEY::F11].Pressed())
    {
        window.RequstFullscreen(!window.Fullscreened());
    }
}
