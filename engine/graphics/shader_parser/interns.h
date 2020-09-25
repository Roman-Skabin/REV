//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/memory.h"
#include "tools/list.hpp"

typedef struct Intern Intern;

struct Intern
{
    const char *str;
    u64         len;
};

struct Interns
{
    List<Intern>  interns;
    Memory       *memory;
};

ENGINE_FUN void CreateInterns(
    in  Engine  *engine,
    out Interns *interns
);

ENGINE_FUN const char *InternStringRange(
    in Interns    *interns,
    in const char *start,
    in const char *end
);

ENGINE_NOINLINE
ENGINE_FUN const char *InternString(
    in Interns    *interns,
    in const char *string,
    in u64         length
);

#define InternCSTR(interns, cstr) InternString(interns, cstr, CSTRLEN(cstr))
