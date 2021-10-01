#include "sandbox.h"
#include "memory/memory.h"

Sandbox::Sandbox()
    : Application(REV::ConstString(REV_CSTR_ARGS("Sandbox")),
                  REV::ConstString(REV_CSTR_ARGS("../../sandbox/sandbox.ini"))),
      m_Logger(REV::ConstString(REV_CSTR_ARGS("Sandbox")),
               REV::Logger::TARGET_FILE | REV::Logger::TARGET_CONSOLE),
      m_DemoScene(&m_Allocator)
{
}

Sandbox::~Sandbox()
{
}

int REV_CDECL main(int argc, char **argv)
{
    REV::Memory::Create(MB(512), MB(512), GB(3ui64));
    Sandbox sandbox;
    sandbox.Run(&sandbox.GetDemoScene());
    return 0;
}
