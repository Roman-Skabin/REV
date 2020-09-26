#include "sandbox.h"

Sandbox::Sandbox()
    : Application("Sandbox"),
      m_Logger("Sandbox logger", "../log/sandbox.log", Logger::TARGET::FILE | Logger::TARGET::CONSOLE)
{
}

Sandbox::~Sandbox()
{
}

int main(int argc, char **argv)
{
    Memory::Create(GB(1ui64), GB(2ui64));
    Sandbox().Run();
    return 0;
}