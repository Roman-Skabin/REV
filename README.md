# REV

## What I am working on right now:
* Extending asset manager with more texture formats to load
* Writing loader for mesh files (some Blender's output files)
* Fixing bags
* Removing/rewriting "bad code"
* Also working on tools

## More detailed version:
* [see here](TODO.txt)

## How to build:
1. Open console in the build directory.
2. Run [generate.bat](build/generate.bat).
3. Open generated solution in MSVS.
4. Press Build button.

## How to use:
1. Create your own project like [sandbox](sandbox) ([premake](build/premake/sandbox.lua) generation file example)
2. Link it with [rev](rev)
3. Build it!

## Minimal sandbox startup code (will be changed later):
```C++
#include "application.h"
#include "memory/memory.h"

class Scene : public REV::SceneBase
{
    Scene(REV::Allocator *allocator)
        : SceneBase(allocator, REV::ConstString(REV_CSTR_ARGS("DemoScene")), 1024, 36*1024)
    {
    }

    ~Scene()
    {
    }

    virtual void OnSetCurrent()       override {}
    virtual void OnUnsetCurrent()     override {}
    virtual void OnSetResourcesData() override {}
    virtual void OnUpdate()           override {}
};

class Sandbox : public REV::Application
{
public:
    Sandbox()
        : Application(REV::ConstString(REV_CSTR_ARGS("Sandbox")),
                      REV::ConstString(REV_CSTR_ARGS("../../sandbox/sandbox.ini"))),
          m_Scene(&m_Allocator)
    {
    }

    ~Sandbox()
    {
    }

    REV_INLINE Scene& GetScene() { return m_Scene; }

private:
    Scene m_Scene;
};

int REV_CDECL main(int argc, char **argv)
{
    REV::Memory::Create(MB(512), MB(512), GB(2ui64));
    Sandbox sandbox;
    sandbox.Run(&sandbox.GetScene());
    return 0;
}
```
