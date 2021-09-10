//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "tools/static_string.hpp"
#include "math/mat.h"
#include "memory/memory.h"

namespace REV
{

#ifndef SB_RTTI_AND_TYPES_DEFINED
#define SB_RTTI_AND_TYPES_DEFINED
namespace RTTI
{
    template<typename T> inline constexpr bool is_vec_v = is_any_of_v<remove_cv_t<T>, Math::v2, Math::v2s, Math::v2u,
                                                                                      Math::v3, Math::v3s, Math::v3u,
                                                                                      Math::v4, Math::v4s, Math::v4u>;

    template<typename T> inline constexpr bool is_mat_v = is_any_of_v<remove_cv_t<T>, Math::m2, Math::m3, Math::m4>;

    template<typename T>
    inline constexpr bool is_trivially_buildable_v  = is_integral_v<T>
                                                   || is_floating_point_v<T>
                                                   || is_vec_v<T>
                                                   || is_mat_v<T>
                                                   || is_pointer_v<T>
                                                   || is_wcstring_array_v<T>
                                                   || is_wcstring_v<T>
                                                   || is_cstring_array_v<T>
                                                   || is_cstring_v<T>
                                                   || is_nullptr_t_v<T>;

    template<typename T>
    using is_trivially_buildable = bool_type<is_trivially_buildable_v<T>>;
}

enum class SBTA
{
    RIGHT,
    LEFT,
};

enum class BASE
{
    BIN = 2,
    OCT = 8,
    DEC = 10,
    HEX = 16
};

struct IntFormat final
{
    BASE Base          = BASE::DEC;
    u32  Width         = 0;
    SBTA TextAlignment = SBTA::RIGHT;
    char Fill          = ' ';
    bool DecorateBase  = true;
    bool ForceSign     = false;
};

struct FloatFormat final
{
    u32  Width         = 0;
    u32  Precision     = 0;
    SBTA TextAlignment = SBTA::RIGHT;
    char Fill          = ' ';
    bool ForceSign     = false;
};

struct TextFormat final
{
    u32  Width         = 0;
    SBTA TextAlignment = SBTA::RIGHT;
    char Fill          = ' ';
};

struct PointerFormat final
{
    bool Decorate = true;
};
#endif

template<u64 capacity, u64 aligned_capacity = AlignUp(capacity, CACHE_LINE_SIZE)>
class StaticStringBuilder final
{
public:
    REV_INLINE StaticStringBuilder()
        : m_IntFormat(),
          m_FloatFormat(),
          m_TextFormat(),
          m_PointerFormat(),
          m_StaticString()
    {
    }

    REV_INLINE StaticStringBuilder(const StaticStringBuilder& other)
        : m_IntFormat(other.m_IntFormat),
          m_FloatFormat(other.m_FloatFormat),
          m_TextFormat(other.m_TextFormat),
          m_PointerFormat(other.m_PointerFormat),
          m_StaticString(other.m_StaticString)
    {
    }

    REV_INLINE ~StaticStringBuilder()
    {
    }

    REV_INLINE const StaticString<capacity, aligned_capacity>& ToString() const { return m_StaticString; }
    REV_INLINE       StaticString<capacity, aligned_capacity>& ToString()       { return m_StaticString; }

    REV_INLINE const StaticString<capacity, aligned_capacity>& Buffer() const { return m_StaticString; }
    REV_INLINE       StaticString<capacity, aligned_capacity>& Buffer()       { return m_StaticString; }

    REV_INLINE const char *BufferData() const { return m_StaticString.Data(); }
    REV_INLINE       char *BufferData()       { return m_StaticString.Data(); }

    REV_INLINE u64 BufferLength() const { return m_StaticString.Length(); }

    template<typename ...Args>
    REV_INLINE StaticStringBuilder& Build(const Args&... args)
    {
        (..., BuildOne(args));
        return *this;
    }

    template<typename ...Args>
    REV_INLINE StaticStringBuilder& BuildLn(const Args&... args)
    {
        (..., BuildOne(args));
        BuildOne('\n');
        return *this;
    }

    REV_INLINE StaticStringBuilder& REV_CDECL BuildF(const char *format, ...)
    {
        va_list args;
        va_start(args, format);
        m_StaticString.m_Length += vsnprintf(m_StaticString.m_Data + m_StaticString.m_Length,
                                             m_StaticString.Capacity() - m_StaticString.m_Length,
                                             format,
                                             args);
        va_end(args);
        return *this;
    }

    REV_INLINE StaticStringBuilder& REV_CDECL BuildVA(const char *format, va_list args)
    {
        m_StaticString.m_Length += vsnprintf(m_StaticString.m_Data + m_StaticString.m_Length,
                                             m_StaticString.Capacity() - m_StaticString.m_Length,
                                             format,
                                             args);
        return *this;
    }

    REV_INLINE StaticStringBuilder& ResetSpecs()
    {
        m_IntFormat     = IntFormat();
        m_FloatFormat   = FloatFormat();
        m_TextFormat    = TextFormat();
        m_PointerFormat = PointerFormat();
        return *this;
    }

    REV_INLINE StaticStringBuilder& Clear()
    {
        m_StaticString.Clear();
        return *this;
    }

    StaticStringBuilder& operator=(const StaticStringBuilder& other)
    {
        if (this != &other)
        {
            m_IntFormat     = other.m_IntFormat;
            m_FloatFormat   = other.m_FloatFormat;
            m_TextFormat    = other.m_TextFormat;
            m_PointerFormat = other.m_PointerFormat;
            m_StaticString  = other.m_StaticString;
        }
        return *this;
    }

private:
    StaticStringBuilder(StaticStringBuilder&&)            = delete;
    StaticStringBuilder& operator=(StaticStringBuilder&&) = delete;

    template<typename T>
    REV_INLINE void BuildOne(const T& arg)
    {
        if constexpr (RTTI::is_trivially_buildable_v<T>)
        {
            AppendTrivial(arg);
        }
        else if constexpr (RTTI::has_to_string_v<T>)
        {
            m_StaticString.PushBack(arg.ToString());
        }
        else
        {
            m_StaticString.PushBack('<');
            m_StaticString.PushBack(typeid(T).name());
            m_StaticString.PushBack(REV_CSTR_ARGS(" has no method ToString>"));
        }
    }

    // @NOTE(Roman): All Parse* functions return buffer_length (appended_length).

    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T> && !RTTI::is_bool_v<T>>>
    u64 ParseInt(char *buffer, T val)
    {
        char *buffer_start = buffer;
        char *buffer_end   = buffer;

        if (val < 0)
        {
            val       = Math::abs(val);
            *buffer++ = '-';
        }
        else if (m_IntFormat.ForceSign)
        {
            *buffer++ = '+';
        }

        if (m_IntFormat.DecorateBase)
        {
            switch (m_IntFormat.Base)
            {
                case BASE::BIN:
                {
                    *buffer++ = '0';
                    *buffer++ = 'b';
                } break;

                case BASE::OCT:
                {
                    *buffer++ = '0';
                } break;

                case BASE::HEX:
                {
                    *buffer++ = '0';
                    *buffer++ = 'x';
                } break;
            }
        }

        if (val > 0)
        {
            const char *int_to_char = "0123456789ABCDEF";

            char *start = buffer;

            while (val > 0)
            {
                *buffer++ = int_to_char[val % cast(T, m_IntFormat.Base)];
                val /= cast(T, m_IntFormat.Base);
            }

            buffer_end = buffer--;

            while (start < buffer)
            {
                char temp = *start;
                *start++  = *buffer;
                *buffer-- = temp;
            }
        }
        else
        {
            *buffer++  = '0';
            buffer_end = buffer;
        }

        return buffer_end - buffer_start;
    }

    u64 ParsePointer(char *buffer, u64 val)
    {
        u64 buffer_start = buffer;
        u64 buffer_end   = buffer;

        if (val > 0)
        {
            if (m_PointerFormat.Decorate)
            {
                *buffer++ = '0';
                *buffer++ = 'x';
            }

            const char *int_to_char = "0123456789ABCDEF";

            char *start = buffer;
            char *end   = buffer + 16;
            char *it    = end - 1;

            while (it >= start && val > 0)
            {
                *it-- = int_to_char[val % 16];
                val /= 16;
            }

            while (it >= start)
            {
                *it-- = '0';
            }

            buffer_end = end;
        }
        else
        {
            *buffer++ = 'n';
            *buffer++ = 'u';
            *buffer++ = 'l';
            *buffer++ = 'l';

            buffer_end = buffer;
        }

        return buffer_end - buffer_start;
    }

    template<typename T, typename = RTTI::enable_if_t<RTTI::is_floating_point_v<T>>>
    u64 ParseFloatingPoint(char *buffer, T val)
    {
        char *buffer_start = buffer;
        char *buffer_end   = buffer;

        if (val < 0)
        {
            val       = Math::abs(val);
            *buffer++ = '-';
        }
        else if (m_FloatFormat.ForceSign)
        {
            *buffer++ = '+';
        }

        T exponent = 0.0;
        T fraction = 0.0;

        if constexpr (RTTI::is_same_v<RTTI::remove_cv_t<T>, f32>)
        {
            fraction = modff(val, &exponent);
        }
        else
        {
            fraction = modf(val, &exponent);
        }

        const char *int_to_char = "0123456789ABCDEF";

        char *start = buffer;

        if (exponent == 0.0)
        {
            *buffer++ = '0';
        }
        else
        {
            if constexpr (RTTI::is_same_v<RTTI::remove_cv_t<T>, f32>)
            {
                T expexp = exponent;

                while (expexp > 0.0f)
                {
                    T mod      = fmodf(exponent, 10.0f);
                    *buffer++  = int_to_char[cast(u64, mod)];
                    exponent  /= 10.0f;
                    modff(exponent, &expexp);
                }
            }
            else
            {
                T expexp = exponent;

                while (expexp > 0.0)
                {
                    T mod      = fmod(exponent, 10.0);
                    *buffer++  = int_to_char[cast(u64, mod)];
                    exponent  /= 10.0;
                    modf(exponent, &expexp);
                }
            }

            for (s32 i = -1; start < buffer; --i)
            {
                char *buffer_char = buffer + i;

                char temp    = *start;
                *start++     = *buffer_char;
                *buffer_char = temp;
            }
        }

        *buffer++ = '.';

        if (fraction == 0.0)
        {
            if (m_FloatFormat.Precision) FillMemoryChar(buffer, '0', m_FloatFormat.Precision);
            else                         *buffer++ = '0';

            buffer_end = buffer + m_FloatFormat.Precision;
        }
        else
        {
            u32 precision = m_FloatFormat.Precision ? m_FloatFormat.Precision : 5;

            start = buffer;

            if constexpr (RTTI::is_same_v<RTTI::remove_cv_t<T>, f32>)
            {
                for (u32 i = 0; i < precision; ++i)
                {
                    fraction  *= 10.0f;
                    T mod      = fmodf(fraction, 10.0f);
                    *buffer++  = int_to_char[cast(u64, mod)];
                }
            }
            else
            {
                for (u32 i = 0; i < precision; ++i)
                {
                    fraction  *= 10.0;
                    T mod      = fmod(fraction, 10.0);
                    *buffer++  = int_to_char[cast(u64, mod)];
                }
            }

            buffer_end = buffer--;

            while (start < buffer)
            {
                char temp = *start;
                *start++  = *buffer;
                *buffer-- = temp;
            }
        }

        return buffer_end - buffer_start;
    }

    template<typename T, typename = RTTI::enable_if_t<RTTI::is_vec_v<T>>>
    u64 REV_VECTORCALL ParseVec(char *buffer, T val)
    {
        char *buffer_start = buffer;

        if constexpr (RTTI::is_same_v<T, Math::v2>)
        {
            *buffer++ = '{';
            *buffer++ = ' ';
            buffer += ParseFloatingPoint(buffer, val.x);;
            *buffer++ = ',';
            *buffer++ = ' ';
            buffer += ParseFloatingPoint(buffer, val.y);
            *buffer++ = ' ';
            *buffer++ = '}';
        }
        else if constexpr (RTTI::is_any_of_v<T, Math::v2s, Math::v2u>)
        {
            *buffer++ = '{';
            *buffer++ = ' ';
            buffer += ParseInt(buffer, val.x);
            *buffer++ = ',';
            *buffer++ = ' ';
            buffer += ParseInt(buffer, val.y);
            *buffer++ = ' ';
            *buffer++ = '}';
        }
        else if constexpr (RTTI::is_same_v<T, Math::v3>)
        {
            *buffer++ = '{';
            *buffer++ = ' ';
            buffer += ParseFloatingPoint(buffer, val.x);
            *buffer++ = ',';
            *buffer++ = ' ';
            buffer += ParseFloatingPoint(buffer, val.y);
            *buffer++ = ',';
            *buffer++ = ' ';
            buffer += ParseFloatingPoint(buffer, val.z);
            *buffer++ = ' ';
            *buffer++ = '}';
        }
        else if constexpr (RTTI::is_any_of_v<T, Math::v3s, Math::v3u>)
        {
            *buffer++ = '{';
            *buffer++ = ' ';
            buffer += ParseInt(buffer, val.x);
            *buffer++ = ',';
            *buffer++ = ' ';
            buffer += ParseInt(buffer, val.y);
            *buffer++ = ',';
            *buffer++ = ' ';
            buffer += ParseInt(buffer, val.z);
            *buffer++ = ' ';
            *buffer++ = '}';
        }
        else if constexpr (RTTI::is_same_v<T, Math::v4>)
        {
            *buffer++ = '{';
            *buffer++ = ' ';
            buffer += ParseFloatingPoint(buffer, val.x);
            *buffer++ = ',';
            *buffer++ = ' ';
            buffer += ParseFloatingPoint(buffer, val.y);
            *buffer++ = ',';
            *buffer++ = ' ';
            buffer += ParseFloatingPoint(buffer, val.z);
            *buffer++ = ',';
            *buffer++ = ' ';
            buffer += ParseFloatingPoint(buffer, val.w);
            *buffer++ = ' ';
            *buffer++ = '}';
        }
        else if constexpr (RTTI::is_any_of_v<T, Math::v4s, Math::v4u>)
        {
            *buffer++ = '{';
            *buffer++ = ' ';
            buffer += ParseInt(buffer, val.x);
            *buffer++ = ',';
            *buffer++ = ' ';
            buffer += ParseInt(buffer, val.y);
            *buffer++ = ',';
            *buffer++ = ' ';
            buffer += ParseInt(buffer, val.z);
            *buffer++ = ',';
            *buffer++ = ' ';
            buffer += ParseInt(buffer, val.w);
            *buffer++ = ' ';
            *buffer++ = '}';
        }
        else
        {
            CopyMemory(buffer, REV_CSTR_ARGS("<Undefined vector type>"));
            buffer += REV_CSTRLEN("<Undefined vector type>");
        }

        return buffer - buffer_start;
    }

    template<typename T, typename = RTTI::enable_if_t<RTTI::is_mat_v<T>>>
    u64 REV_VECTORCALL ParseMat(char *buffer, T val)
    {
        char *buffer_start = buffer;

        if constexpr (RTTI::is_same_v<T, Math::m2>)
        {
            *buffer++ = '\n';
            *buffer++ = '[';
            *buffer++ = ' ';
            buffer += ParseFloatingPoint(buffer, val.e00);
            *buffer++ = ',';
            *buffer++ = ' ';
            buffer += ParseFloatingPoint(buffer, val.e01);
            *buffer++ = ' ';
            *buffer++ = ']';
            *buffer++ = '\n';

            *buffer++ = '[';
            *buffer++ = ' ';
            buffer += ParseFloatingPoint(buffer, val.e10);
            *buffer++ = ',';
            *buffer++ = ' ';
            buffer += ParseFloatingPoint(buffer, val.e11);
            *buffer++ = ' ';
            *buffer++ = ']';
            *buffer++ = '\n';
        }
        else if constexpr (RTTI::is_same_v<T, Math::m3>)
        {
            *buffer++ = '\n';
            *buffer++ = '[';
            *buffer++ = ' ';
            buffer += ParseFloatingPoint(buffer, val.e00);
            *buffer++ = ',';
            *buffer++ = ' ';
            buffer += ParseFloatingPoint(buffer, val.e01);
            *buffer++ = ',';
            *buffer++ = ' ';
            buffer += ParseFloatingPoint(buffer, val.e02);
            *buffer++ = ' ';
            *buffer++ = ']';
            *buffer++ = '\n';

            *buffer++ = '[';
            *buffer++ = ' ';
            buffer += ParseFloatingPoint(buffer, val.e10);
            *buffer++ = ',';
            *buffer++ = ' ';
            buffer += ParseFloatingPoint(buffer, val.e11);
            *buffer++ = ',';
            *buffer++ = ' ';
            buffer += ParseFloatingPoint(buffer, val.e12);
            *buffer++ = ' ';
            *buffer++ = ']';
            *buffer++ = '\n';

            *buffer++ = '[';
            *buffer++ = ' ';
            buffer += ParseFloatingPoint(buffer, val.e20);
            *buffer++ = ',';
            *buffer++ = ' ';
            buffer += ParseFloatingPoint(buffer, val.e21);
            *buffer++ = ',';
            *buffer++ = ' ';
            buffer += ParseFloatingPoint(buffer, val.e22);
            *buffer++ = ' ';
            *buffer++ = ']';
            *buffer++ = '\n';
        }
        else if constexpr (RTTI::is_same_v<T, Math::m4>)
        {
            *buffer++ = '\n';
            *buffer++ = '[';
            *buffer++ = ' ';
            buffer += ParseFloatingPoint(buffer, val.e00);
            *buffer++ = ',';
            *buffer++ = ' ';
            buffer += ParseFloatingPoint(buffer, val.e01);
            *buffer++ = ',';
            *buffer++ = ' ';
            buffer += ParseFloatingPoint(buffer, val.e02);
            *buffer++ = ',';
            *buffer++ = ' ';
            buffer += ParseFloatingPoint(buffer, val.e03);
            *buffer++ = ' ';
            *buffer++ = ']';
            *buffer++ = '\n';

            *buffer++ = '[';
            *buffer++ = ' ';
            buffer += ParseFloatingPoint(buffer, val.e10);
            *buffer++ = ',';
            *buffer++ = ' ';
            buffer += ParseFloatingPoint(buffer, val.e11);
            *buffer++ = ',';
            *buffer++ = ' ';
            buffer += ParseFloatingPoint(buffer, val.e12);
            *buffer++ = ',';
            *buffer++ = ' ';
            buffer += ParseFloatingPoint(buffer, val.e13);
            *buffer++ = ' ';
            *buffer++ = ']';
            *buffer++ = '\n';

            *buffer++ = '[';
            *buffer++ = ' ';
            buffer += ParseFloatingPoint(buffer, val.e20);
            *buffer++ = ',';
            *buffer++ = ' ';
            buffer += ParseFloatingPoint(buffer, val.e21);
            *buffer++ = ',';
            *buffer++ = ' ';
            buffer += ParseFloatingPoint(buffer, val.e22);
            *buffer++ = ',';
            *buffer++ = ' ';
            buffer += ParseFloatingPoint(buffer, val.e23);
            *buffer++ = ' ';
            *buffer++ = ']';
            *buffer++ = '\n';

            *buffer++ = '[';
            *buffer++ = ' ';
            buffer += ParseFloatingPoint(buffer, val.e30);
            *buffer++ = ',';
            *buffer++ = ' ';
            buffer += ParseFloatingPoint(buffer, val.e31);
            *buffer++ = ',';
            *buffer++ = ' ';
            buffer += ParseFloatingPoint(buffer, val.e32);
            *buffer++ = ',';
            *buffer++ = ' ';
            buffer += ParseFloatingPoint(buffer, val.e33);
            *buffer++ = ' ';
            *buffer++ = ']';
            *buffer++ = '\n';
        }
        else
        {
            CopyMemory(buffer, REV_CSTR_ARGS("<Undefined matrix type>"));
            buffer += REV_CSTRLEN("<Undefined matrix type>");
        }

        return buffer - buffer_start;
    }

    template<typename T, typename = RTTI::enable_if_t<RTTI::is_trivially_buildable_v<T>>>
    void AppendTrivial(const T& val)
    {
        u64 start_length = m_StaticString.Length();

        char buffer[1024] = {'\0'};

        u32  width          = 0;
        char fill           = ' ';
        SBTA text_alignment = SBTA::RIGHT;

        if constexpr (RTTI::is_bool_v<T>)
        {
            if (val) m_StaticString.PushBack(REV_CSTR_ARGS("true"));
            else     m_StaticString.PushBack(REV_CSTR_ARGS("false"));
        }
        else if constexpr (RTTI::is_char_v<T>)
        {
            width          = m_TextFormat.Width;
            fill           = m_TextFormat.Fill;
            text_alignment = m_TextFormat.TextAlignment;
            m_StaticString.PushBack(val);
        }
        else if constexpr (RTTI::is_cstring_array_v<T>)
        {
            width          = m_TextFormat.Width;
            fill           = m_TextFormat.Fill;
            text_alignment = m_TextFormat.TextAlignment;
            m_StaticString.PushBack(REV_CSTR_ARGS(val));
        }
        else if constexpr (RTTI::is_cstring_v<T>)
        {
            width          = m_TextFormat.Width;
            fill           = m_TextFormat.Fill;
            text_alignment = m_TextFormat.TextAlignment;
            m_StaticString.PushBack(val, strlen(val));
        }
        else if constexpr (RTTI::is_wcstring_array_v<T>)
        {
            width          = m_TextFormat.Width;
            fill           = m_TextFormat.Fill;
            text_alignment = m_TextFormat.TextAlignment;

            char buffer[sizeof(val) / sizeof(wchar_t)] = {'\0'};

            // @TODO(Roman): #CrossPlatform
            WideCharToMultiByte(CP_ACP, 0, val, sizeof(val) - sizeof(wchar_t), REV_CSTR_ARGS(buffer), null, null);

            m_StaticString.PushBack(REV_CSTR_ARGS(buffer));
        }
        else if constexpr (RTTI::is_wcstring_v<T>)
        {
            width          = m_TextFormat.Width;
            fill           = m_TextFormat.Fill;
            text_alignment = m_TextFormat.TextAlignment;

            int wlength = cast(int, wcslen(val));

            // @TODO(Roman): #CrossPlatform
            int   length = WideCharToMultiByte(CP_ACP, 0, val, wlength, null, 0, null, null);
            char *buffer = Memory::Get()->PushToFA<char>(length + 1);

            WideCharToMultiByte(CP_ACP, 0, val, wlength, buffer, length, null, null);

            m_StaticString.PushBack(buffer, length);
        }
        else if constexpr (RTTI::is_pointer_v<T>)
        {
            u64 length = ParsePointer(buffer, cast(u64, val));
            m_StaticString.PushBack(buffer, length);
        }
        else if constexpr (RTTI::is_nullptr_t_v<T>)
        {
            m_StaticString.PushBack(REV_CSTR_ARGS("null"));
        }
        else if constexpr (RTTI::is_arithmetic_v<T> && !RTTI::is_floating_point_v<T>)
        {
            width          = m_IntFormat.Width;
            fill           = m_IntFormat.Fill;
            text_alignment = m_IntFormat.TextAlignment;
            u64 length     = ParseInt(buffer, val);
            m_StaticString.PushBack(buffer, length);
        }
        else if constexpr (RTTI::is_floating_point_v<T>)
        {
            width          = m_FloatFormat.Width;
            fill           = m_FloatFormat.Fill;
            text_alignment = m_FloatFormat.TextAlignment;
            u64 length     = ParseFloatingPoint(buffer, val);
            m_StaticString.PushBack(buffer, length);
        }
        else if constexpr (RTTI::is_vec_v<T>)
        {
            u64 length = ParseVec(buffer, val);
            m_StaticString.PushBack(buffer, length);
        }
        else if constexpr (RTTI::is_mat_v<T>)
        {
            u64 length = ParseMat(buffer, val);
            m_StaticString.PushBack(buffer, length);
        }

        u64 appended_length = m_StaticString.Length() - start_length;
        if (appended_length < width)
        {
            if (text_alignment == SBTA::RIGHT)
            {
                m_StaticString.Insert(start_length, fill, width - appended_length);
            }
            else
            {
                m_StaticString.Insert(start_length + appended_length, fill, width - appended_length);
            }
        }
    }

public:
    IntFormat     m_IntFormat;
    FloatFormat   m_FloatFormat;
    TextFormat    m_TextFormat;
    PointerFormat m_PointerFormat;

private:
    StaticString<capacity, aligned_capacity> m_StaticString;
};

template<u64 capacity, u64 aligned_capacity> StaticStringBuilder(const StaticString<capacity, aligned_capacity>&)        -> StaticStringBuilder<capacity, aligned_capacity>;
template<u64 capacity, u64 aligned_capacity> StaticStringBuilder(const StaticStringBuilder<capacity, aligned_capacity>&) -> StaticStringBuilder<capacity, aligned_capacity>;

}
