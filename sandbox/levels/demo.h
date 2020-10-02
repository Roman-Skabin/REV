#pragma once

#include "core/level.h"
#include "tools/logger.h"

class DemoLevel final : public Level
{
public:
    DemoLevel();
    ~DemoLevel();

    virtual void OnAttach() override;
    virtual void OnDetach() override;

    virtual void OnUpdateAndRender() override;

private:
    Application *m_Application;
    char         m_OriginalWindowTitle[128];
};
