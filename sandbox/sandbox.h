#pragma once

#include "application.h"
#include "scenes/demo.h"

// @Cleanup(Roman): The idea about some "sandbox" class that inherits from the Application class is maybe useless.
//                  We can just pass a scene we want to draw first to the Application constructor or something.
//                  Anyway, the point is Sandbox abstraction over Application is useless.

class Sandbox final : public REV::Application
{
public:
    Sandbox();
    ~Sandbox();

    REV_INLINE const REV::Logger& GetLogger() const { return m_Logger; }
    REV_INLINE       REV::Logger& GetLogger()       { return m_Logger; }

    REV_INLINE const DemoScene& GetDemoScene() const { return m_DemoScene; }
    REV_INLINE       DemoScene& GetDemoScene()       { return m_DemoScene; }

    REV_INLINE const REV::ResourceHandle& GetMousePickTexture() const { return m_StaticMousePickTexture; }
    REV_INLINE const REV::ResourceHandle& GetColorTarget()      const { return m_ColorTarget; }
    REV_INLINE const REV::ResourceHandle& GetDepthTarget()      const { return m_DepthTarget; }

private:
    REV_DELETE_CONSTRS_AND_OPS(Sandbox);

private:
    REV::Logger         m_Logger;
    REV::ResourceHandle m_StaticMousePickTexture;
    REV::ResourceHandle m_ColorTarget;
    REV::ResourceHandle m_DepthTarget;

    DemoScene m_DemoScene;
};
