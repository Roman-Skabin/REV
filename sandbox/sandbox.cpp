#include "sandbox.h"

Sandbox::Sandbox()
    : Application("Sandbox"),
      m_Logger("Sandbox logger", "../log/sandbox.log", Logger::TARGET::FILE | Logger::TARGET::CONSOLE),
      m_DemoLevel()
{
    m_Renderer->SetVSync(true);
    m_Window.RequstFullscreen(true);
    AttachLevels(&m_DemoLevel);
}

Sandbox::~Sandbox()
{
    DetachAllLevels();
}

int main(int argc, char **argv)
{
    Memory::Create(GB(1ui64), GB(3ui64));
    Sandbox().Run();
    return 0;
}
