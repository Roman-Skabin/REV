#pragma once

#include "application.h"

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

private:
    Logger m_Logger;
};
