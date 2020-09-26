//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/core.h"

class Application;

class ENGINE_IMPEXP Level
{
protected:
    explicit Level(const char *name = "Level");

public:
    virtual ~Level();

    virtual void OnAttach() = 0;
    virtual void OnDetach() = 0;

    virtual void OnUpdateAndRender() = 0;

    const char *GetName() const { return m_Name; }

protected:
    const char *m_Name;
};
