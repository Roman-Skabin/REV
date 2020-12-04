#include "sandbox.h"

Sandbox::Sandbox()
    : Application(StaticString<128>("Sandbox", CSTRLEN("Sandbox"))),
      m_Logger("Sandbox logger", "../log/sandbox.log", Logger::TARGET::FILE | Logger::TARGET::CONSOLE)
{
    m_SceneManager->SetCurrentScene(m_SceneManager->PushScene<DemoScene>());
}

Sandbox::~Sandbox()
{
}

int main(int argc, char **argv)
{
    Memory::Create(GB(1ui64), GB(3ui64));
    Sandbox().Run();
    return 0;
}
