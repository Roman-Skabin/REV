#include "sandbox.h"

Sandbox::Sandbox()
    : Application(REV::StaticString<128>("Sandbox", REV_CSTRLEN("Sandbox"))),
      m_Logger("Sandbox logger", "../log/sandbox.log", REV::Logger::TARGET::FILE | REV::Logger::TARGET::CONSOLE),
      m_DemoScene(&m_Allocator)
{
    SetCurrentScene(&m_DemoScene);
}

Sandbox::~Sandbox()
{
}

int REV_CDECL main(int argc, char **argv)
{
    REV::Memory::Create(GB(1ui64), GB(3ui64));
    Sandbox().Run();
    return 0;
}
