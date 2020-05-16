//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "gpu/core/interns.h"
#include "cengine.h"

void CreateInterns(Engine *engine, Interns *interns)
{
    Check(engine);
    Check(interns);

    interns->memory  = engine->memory;
    interns->interns = 0;
}

const char *InternStringRange(Interns *interns, const char *start, const char *end)
{
    u64 len = end - start;

    if (!interns->interns)
    {
        char *new_str = PushToTransientArea(interns->memory, len + 1);
        CopyMemory(new_str, start, len);
        new_str[len] = '\0';

        interns->interns       = PushToTA(Intern, interns->memory, 1);
        interns->interns->next = 0;
        interns->interns->str  = new_str;
        interns->interns->len  = len;

        return new_str;
    }
    else
    {
        Intern *it = interns->interns;
	    while (it)
        {
            if (it->len == len && RtlEqualMemory(it->str, start, len))
            {
                return it->str;
            }

            if (it->next) it = it->next;
            else          break;
        };

        char *new_str = PushToTransientArea(interns->memory, len + 1);
        CopyMemory(new_str, start, len);
        new_str[len] = '\0';

        it->next       = PushToTA(Intern, interns->memory, 1);
        it->next->next = 0;
        it->next->str  = new_str;
        it->next->len  = len;
	
	    return new_str;
    }
}
