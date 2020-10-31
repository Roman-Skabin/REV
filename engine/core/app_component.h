//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"

class Application;

class ENGINE_API AppComponent
{
protected:
    explicit AppComponent(const char *name = "Level") : m_Name(name) {}

public:
    virtual ~AppComponent() {}

    virtual void OnAttach() = 0;
    virtual void OnDetach() = 0;

    virtual void OnUpdate() = 0;

    const char *GetName() const { return m_Name; }

protected:
    const char *m_Name;
};
