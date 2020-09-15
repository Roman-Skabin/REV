//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/memory.h"

typedef struct Intern Intern;

struct Intern
{
    ExtendsList(Intern);
    const char *str;
    u64         len;
};

typedef struct Interns
{
    LIST Intern *interns;
    Memory *memory;
} Interns;

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
