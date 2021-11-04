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
    REV::GPU::DeviceContext *device_context = REV::GraphicsAPI::GetDeviceContext();

    REV::Math::v2s rt_dimension = device_context->RTSize();

    m_StaticMousePickTexture = memory_manager->AllocateTexture2D(cast(u16, rt_dimension.w),
                                                                 cast(u16, rt_dimension.h),
                                                                 1,
                                                                 REV::GPU::TEXTURE_FORMAT_U32,
                                                                 REV::GPU::RESOURCE_FLAG_STATIC | REV::GPU::RESOURCE_FLAG_CPU_READ | REV::GPU::RESOURCE_FLAG_RENDER_TARGET,
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
