//
// Copyright 2020 Roman Skabin
//

#include "core/pch.h"
#include "graphics/core/interns.h"
#include "cengine.h"

void CreateInterns(Engine *engine, Interns *interns)
{
    Check(engine);
    Check(interns);

    interns->memory  = engine->memory;
    interns->interns = CreateBuffer(&engine->allocator, sizeof(const char *));
}

void DestroyInterns(Interns *interns)
{
    DestroyBuffer(interns->interns);
    interns->memory = 0;
}

const char *InternStringRange(Interns *interns, const char *start, const char *end)
{
    u64 len = end - start;

    Intern *first_intern = interns->interns;
    Intern *last_intern  = interns->interns + BufferGetCount(interns->interns) - 1;

	for (Intern *it = first_intern; it <= last_intern; ++it)
    {
        if (it->len == len && !memcmp(it->str, start, len))
        {
            return it->str;
        }
    }

    char *new_str = PushToTransientArea(interns->memory, len + 1);
    CopyMemory(new_str, start, len);
    new_str[len] = '\0';

    Intern new_intern;
    new_intern.str = new_str;
    new_intern.len = len;
	BufferPushBack(interns->interns, new_intern);
	
	return new_str;
}
