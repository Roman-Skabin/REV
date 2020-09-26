#pragma once

#include "core/level.h"
#include "tools/logger.h"

class DemoLevel : public Level
{
public:
    DemoLevel(const Logger& logger);
    ~DemoLevel();

    virtual void OnAttach() override;
    virtual void OnDetach() override;

    virtual void OnUpdateAndRender() override;

private:
    Application *m_Application;
    Logger       m_Logger;
};
