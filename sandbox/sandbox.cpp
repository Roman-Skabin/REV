#include "sandbox.h"
#include "memory/memory.h"

Sandbox::Sandbox()
    : Application(REV::ConstString(REV_CSTR_ARGS("Sandbox")),
                  REV::ConstString(REV_CSTR_ARGS("../../../sandbox/sandbox.ini"))),
      m_Logger(REV::ConstString(REV_CSTR_ARGS("Sandbox")),
               REV::Logger::TARGET_FILE | REV::Logger::TARGET_CONSOLE),
      m_DemoScene(&m_Allocator)
{
    REV::GPU::MemoryManager *memory_manager = REV::GraphicsAPI::GetMemoryManager();

    REV::Math::v4s window_dimension = m_Window.XYWH();

    m_StaticMousePickTexture = memory_manager->AllocateTexture(cast(u16, window_dimension.wh.w),
                                                               cast(u16, window_dimension.wh.h),
                                                               0,
                                                               1,
                                                               REV::GPU::RESOURCE_KIND_READ_WRITE_TEXTURE | REV::GPU::RESOURCE_KIND_STATIC,
                                                               REV::GPU::TEXTURE_FORMAT_U32,
                                                               REV::ConstString(REV_CSTR_ARGS("StaticMousePickTexture")));
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
