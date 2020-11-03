#include "components/demo.h"
#include "application.h"
#include "math/vec.h"
#include "math/color.h"
#include "tools/static_string_builder.hpp"

#pragma pack(push, 1)
struct Vertex
{
    v4 pos;
    v4 col;
};
#pragma pack(pop)

DemoComponent::DemoComponent()
    : AppComponent("DemoComponent"),
      m_Application(Application::Get()),
      m_GraphicsProgram(GraphicsAPI::GetGPUProgramManager()->CreateGraphicsProgram("../sandbox/assets/shaders/shader.vert",
                                                                                   "../sandbox/assets/shaders/shader.pixel")),
      m_VertexBuffer(null),
      m_IndexBuffer(null),
      m_OriginalWindowTitle(m_Application->GetWindow().Title())
{
    IGPUMemoryManager *gpu_memory_manager = GraphicsAPI::GetGPUMemoryManager();

    Vertex vertices[] =
    {
        { v4(-0.5f, -0.5f, 0.0f, 1.0f), gRedA1    },
        { v4(-0.5f,  0.5f, 0.0f, 1.0f), gGreenA1  },
        { v4( 0.5f,  0.5f, 0.0f, 1.0f), gBlueA1   },
        { v4( 0.5f, -0.5f, 0.0f, 1.0f), gYellowA1 }
    };
    m_VertexBuffer = gpu_memory_manager->AllocateVB(cast<u32>(ArrayCount(vertices)), sizeof(Vertex));
    m_VertexBuffer->SetDataImmediate(vertices);

    u32 indices[] =
    {
        0, 1, 2,
        0, 2, 3
    };
    m_IndexBuffer = gpu_memory_manager->AllocateIB(cast<u32>(ArrayCount(indices)));
    m_IndexBuffer->SetDataImmediate(indices);
}

DemoComponent::~DemoComponent()
{
}

#include "tools/string_builder.hpp"

void DemoComponent::OnAttach()
{
    Allocator *allocator = &Application::Get()->GetAllocator();

    char ch = 'c';
    const char *cstring = "Hello, world!";
    v4 rotation_vector = v4(0.5f, -0.75f, 0.25, 0.0f).normalize();

    StringBuilder sb(allocator, 1024);

    sb.BuildLn("\nchar: ", ch, ", s8: ", cast<s8>(S8_MAX), ", u8: ", U8_MAX, ", s16: ", S16_MAX, ", u16: ", U16_MAX,
               ", int: ", INT_MAX, ", s32: ", S32_MAX, ", u32: ", U32_MAX, ", s64: ", S64_MAX, ", u64: ", U64_MAX);
    sb.BuildLn("f32: ", F32_MAX, " f64: ", F64_MAX);
    sb.BuildLn("pointer: ", allocator);
    sb.BuildLn("allocator: ", *allocator);
    sb.BuildLn("nullptr: ", null);
    sb.BuildLn("cstring: ", cstring);
    sb.BuildLn("rotation vector: ", rotation_vector);
    sb.Precision = 5;
    sb.ForceSign = true;
    sb.BuildLn("rotation matrix: ", m4::rotation(rotation_vector, f32_PI_4));

    WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), sb.ToString().Data(), cast<u32>(sb.ToString().Length()), null, null);
}

void DemoComponent::OnDetach()
{
}

void DemoComponent::OnUpdate()
{
    const Keyboard& keyboard = m_Application->GetInput()->GetKeyboard();
    Window&         window   = m_Application->GetWindow();
    Timer&          timer    = m_Application->GetTimer();

    local f32 last_print_time;
    if (!window.Fullscreened() && timer.Seconds() - last_print_time >= 0.1)
    {
        f32 FPS = timer.TicksPerSecond() / cast<f32>(timer.DeltaTicks());

        StaticStringBuilder<128> ssb;
        ssb.Precision = 2;
        ssb.Build(m_OriginalWindowTitle, " - FPS: ", FPS, " - MSPF: ", 1000.0f / FPS);

        window.SetTitle(ssb.ToStaticString());

        last_print_time = timer.Seconds();
    }

    if (keyboard[KEY::F11].Pressed())
    {
        window.RequstFullscreen(!window.Fullscreened());
    }
    else if (keyboard[KEY::V].Pressed())
    {
        IRenderer *renderer = GraphicsAPI::GetRenderer();
        renderer->SetVSync(!renderer->VSyncEnabled());
    }

    GraphicsAPI::GetGPUProgramManager()->SetCurrentGraphicsProgram(m_GraphicsProgram);

    m_GraphicsProgram->BindVertexBuffer(m_VertexBuffer);
    m_GraphicsProgram->BindIndexBuffer(m_IndexBuffer);
    m_GraphicsProgram->DrawIndices();
}
