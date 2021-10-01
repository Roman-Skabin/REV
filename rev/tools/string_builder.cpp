// Copyright (c) 2020-2021, Roman-Skabin
// All rights reserved.
// 
// This source code is licensed under the BSD-style license found in the
// LICENSE.md file in the root directory of this source tree. 

#pragma once

#include "core/pch.h"
#include "tools/string_builder.h"

namespace REV
{

StringBuilder::StringBuilder(Allocator *allocator, u64 initial_capacity, u64 alignment_in_bytes)
    : m_IntFormat(),
      m_FloatFormat(),
      m_TextFormat(),
      m_PointerFormat(),
      m_String(allocator, initial_capacity, alignment_in_bytes)
{
}

StringBuilder::StringBuilder(const String& string)
    : m_IntFormat(),
      m_FloatFormat(),
      m_TextFormat(),
      m_PointerFormat(),
      m_String(string)
{
}

StringBuilder::StringBuilder(String&& string)
    : m_IntFormat(),
      m_FloatFormat(),
      m_TextFormat(),
      m_PointerFormat(),
      m_String(RTTI::move(string))
{
}

StringBuilder::StringBuilder(const StringBuilder& other)
    : m_IntFormat(other.m_IntFormat),
      m_FloatFormat(other.m_FloatFormat),
      m_TextFormat(other.m_TextFormat),
      m_PointerFormat(other.m_PointerFormat),
      m_String(other.m_String)
{
}

StringBuilder::StringBuilder(StringBuilder&& other)
    : m_IntFormat(RTTI::move(other.m_IntFormat)),
      m_FloatFormat(RTTI::move(other.m_FloatFormat)),
      m_TextFormat(RTTI::move(other.m_TextFormat)),
      m_PointerFormat(RTTI::move(other.m_PointerFormat)),
      m_String(RTTI::move(other.m_String))
{
}

StringBuilder::~StringBuilder()
{
}

StringBuilder& REV_CDECL StringBuilder::BuildF(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    BuildVA(format, args);
    va_end(args);
    return *this;
}

StringBuilder& REV_CDECL StringBuilder::BuildVA(const char *format, va_list args)
{
    u64 old_length = m_String.m_Header->length;

    m_String.m_Header->length += vsnprintf(null, 0, format, args);
    m_String.Expand();

    vsnprintf(m_String.m_Header->data     + old_length,
              m_String.m_Header->capacity - old_length,
              format,
              args);

    return *this;
}

StringBuilder& StringBuilder::ResetFormats()
{
    m_IntFormat     = IntFormat();
    m_FloatFormat   = FloatFormat();
    m_TextFormat    = TextFormat();
    m_PointerFormat = PointerFormat();
    return *this;
}

StringBuilder& StringBuilder::Clear()
{
    m_String.Clear();
    return *this;
}

StringBuilder& StringBuilder::operator=(const StringBuilder& other)
{
    if (this != &other)
    {
        m_IntFormat     = other.m_IntFormat;
        m_FloatFormat   = other.m_FloatFormat;
        m_TextFormat    = other.m_TextFormat;
        m_PointerFormat = other.m_PointerFormat;
        m_String        = other.m_String;
    }
    return *this;
}

StringBuilder& StringBuilder::operator=(StringBuilder&& other)
{
    if (this != &other)
    {
        m_IntFormat     = other.m_IntFormat;
        m_FloatFormat   = other.m_FloatFormat;
        m_TextFormat    = other.m_TextFormat;
        m_PointerFormat = other.m_PointerFormat;
        m_String        = RTTI::move(other.m_String);
    }
    return *this;
}

}
