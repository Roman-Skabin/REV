#include "sandbox.h"

Sandbox::Sandbox()
    : Application("Sandbox"),
      m_Logger("Sandbox logger", "../log/sandbox.log", Logger::TARGET::FILE | Logger::TARGET::CONSOLE),
      m_DemoLevel(m_Logger)
{
    PushLevels(&m_DemoLevel);
}

Sandbox::~Sandbox()
{
    PopLevels(&m_DemoLevel);
}

int main(int argc, char **argv)
{
    Memory::Create(GB(1ui64), GB(3ui64));
    Sandbox().Run();
    return 0;
}
