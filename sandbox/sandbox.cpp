#include "sandbox.h"

Sandbox::Sandbox()
    : Application("Sandbox"),
      m_Logger("Sandbox logger", "../log/sandbox.log", Logger::TARGET::FILE | Logger::TARGET::CONSOLE),
      m_DemoComponent()
{
    // GraphicsAPI::GetRenderer()->SetVSync(true);
    AttacComponents(&m_DemoComponent);
}

Sandbox::~Sandbox()
{
    DetachAllComponents();
}

int main(int argc, char **argv)
{
    Memory::Create(GB(1ui64), GB(3ui64));
    Sandbox().Run();
    return 0;
}
