#pragma once

#include "application.h"
#include "scenes/demo.h"

class Sandbox final : public Application
{
public:
    Sandbox();
    ~Sandbox();

private:
    Sandbox(const Sandbox&) = delete;
    Sandbox(Sandbox&&)      = delete;

    Sandbox& operator=(const Sandbox&) = delete;
    Sandbox& operator=(Sandbox&&)      = delete;

    const Logger& GetLogger() const { return m_Logger; }
          Logger& GetLogger()       { return m_Logger; }

private:
    Logger m_Logger;
};
