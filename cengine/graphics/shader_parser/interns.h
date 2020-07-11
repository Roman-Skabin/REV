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

CENGINE_FUN void CreateInterns(
    IN  Engine  *engine,
    OUT Interns *interns
);

CENGINE_FUN const char *InternStringRange(
    IN Interns    *interns,
    IN const char *start,
    IN const char *end
);

CENGINE_NOINLINE
CENGINE_FUN const char *InternString(
    IN Interns    *interns,
    IN const char *string,
    IN u64         length
);

#define InternCSTR(interns, cstr) InternString(interns, cstr, CSTRLEN(cstr))
