//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "tools/string.hpp"
#include "math/mat.h"

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
#endif

template<typename Allocator_t = Allocator>
class StringBuilder final
{
public:
    StringBuilder(Allocator_t *allocator, u64 initial_capacity = 16, u64 alignment_in_bytes = REV::DEFAULT_ALIGNMENT)
        : Base(BASE::DEC),
          Width(0),
          Precision(0),
          TextAlignment(SBTA::RIGHT),
          Fill(' '),
          DecorateBase(true),
          ForceSign(false),
          m_String(allocator, initial_capacity, alignment_in_bytes)
    {
    }

    StringBuilder(const String<Allocator_t>& string)
        : Base(BASE::DEC),
          Width(0),
          Precision(0),
          TextAlignment(SBTA::RIGHT),
          Fill(' '),
          DecorateBase(true),
          ForceSign(false),
          m_String(string)
    {
    }

    StringBuilder(String<Allocator_t>&& string)
        : Base(BASE::DEC),
          Width(0),
          Precision(0),
          TextAlignment(SBTA::RIGHT),
          Fill(' '),
          DecorateBase(true),
          ForceSign(false),
          m_String(RTTI::move(string))
    {
    }

    StringBuilder(const StringBuilder& other)
        : Base(other.Base),
          Width(other.Width),
          Precision(other.Precision),
          TextAlignment(other.TextAlignment),
          Fill(other.Fill),
          DecorateBase(other.DecorateBase),
          ForceSign(other.ForceSign),
          m_String(other.m_String)
    {
    }

    StringBuilder(StringBuilder&& other)
        : Base(other.Base),
          Width(other.Width),
          Precision(other.Precision),
          TextAlignment(other.TextAlignment),
          Fill(other.Fill),
          DecorateBase(other.DecorateBase),
          ForceSign(other.ForceSign),
          m_String(RTTI::move(other.m_String))
    {
    }

    ~StringBuilder()
    {
    }

    constexpr const String<Allocator_t>& ToString() const { return m_String; }
    constexpr       String<Allocator_t>& ToString()       { return m_String; }

    template<typename ...Args>
    void Build(const Args&... args)
    {
        (..., BuildOne(args));
        ResetSpecs();
    }

    template<typename ...Args>
    void BuildLn(const Args&... args)
    {
        (..., BuildOne(args));
        BuildOne('\n');
        ResetSpecs();
    }

    void ResetSpecs()
    {
        Base          = BASE::DEC;
        Width         = 0;
        Precision     = 0;
        TextAlignment = SBTA::RIGHT;
        Fill          = ' ';
        DecorateBase  = true;
        ForceSign     = false;
    }

    void Clear()
    {
        m_String.Clear();
    }

    StringBuilder& operator=(const StringBuilder& other)
    {
        if (this != &other)
        {
            Base          = other.Base;
            Width         = other.Width;
            Precision     = other.Precision;
            TextAlignment = other.TextAlignment;
            Fill          = other.Fill;
            DecorateBase  = other.DecorateBase;
            ForceSign     = other.ForceSign;
            m_String      = other.m_String;
        }
        return *this;
    }

    StringBuilder& operator=(StringBuilder&& other)
    {
        if (this != &other)
        {
            Base          = other.Base;
            Width         = other.Width;
            Precision     = other.Precision;
            TextAlignment = other.TextAlignment;
            Fill          = other.Fill;
            DecorateBase  = other.DecorateBase;
            ForceSign     = other.ForceSign;
            m_String      = RTTI::move(other.m_String);
        }
        return *this;
    }

private:
    template<typename T>
    void BuildOne(const T& arg)
    {
        if constexpr (RTTI::is_trivially_buildable_v<T>)
        {
            AppendTrivial(arg);
        }
        else if constexpr (RTTI::has_to_string_v<T>)
        {
            m_String.PushBack(arg.ToString());
        }
        else
        {
            StaticString<128> message;
            message.PushBack('<');
            message.PushBack(typeid(T).name());
            message.PushBack(" has no method ToString>", REV_CSTRLEN(" has no method ToString>"));

            m_String.PushBack(message);
        }
    }

    template<typename T, typename = RTTI::enable_if_t<RTTI::is_integral_v<T> && !RTTI::is_bool_v<T>>>
    void ParseInt(char *buffer, T val)
    {
        if (val < 0)
        {
            val = RTTI::abs(val);
            *buffer++ = '-';
        }
        else if (ForceSign)
        {
            *buffer++ = '+';
        }

        if (DecorateBase)
        {
            switch (Base)
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
                *buffer++ = int_to_char[val % cast<T>(Base)];
                val /= cast<T>(Base);
            }

            --buffer;

            while (start < buffer)
            {
                char temp = *start;
                *start++  = *buffer;
                *buffer-- = temp;
            }
        }
        else
        {
            *buffer = '0';
        }
    }

    void ParsePointer(char *buffer, u64 val)
    {
        if (val > 0)
        {
            if (DecorateBase)
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
        }
        else
        {
            *buffer++ = 'n';
            *buffer++ = 'u';
            *buffer++ = 'l';
            *buffer   = 'l';
        }
    }

    template<typename T, typename = RTTI::enable_if_t<RTTI::is_floating_point_v<T>>>
    void ParseFloatingPoint(char *buffer, T val)
    {
        if (val < 0)
        {
            val       = RTTI::abs(val);
            *buffer++ = '-';
        }
        else if (ForceSign)
        {
            *buffer++ = '+';
        }

        T exponent = 0.0;
        T fraction = modf(val, &exponent);

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
                T expexp = 0.0f;
                modff(exponent, &expexp);

                while (expexp > 0.0f)
                {
                    T mod      = fmodf(exponent, 10.0f);
                    *buffer++  = int_to_char[cast<u64>(mod)];
                    exponent  /= 10.0f;
                    modff(exponent, &expexp);
                }
            }
            else
            {
                T expexp = 0.0;
                modf(exponent, &expexp);

                while (expexp > 0.0)
                {
                    T mod      = fmod(exponent, 10.0);
                    *buffer++  = int_to_char[cast<u64>(mod)];
                    exponent  /= 10.0;
                    modf(exponent, &expexp);
                }
            }

            for (s32 i = -1; start < buffer; --i)
            {
                char buffer_char = buffer[i];

                char temp   = *start;
                *start++    = buffer_char;
                buffer_char = temp;
            }
        }

        *buffer++ = '.';

        if (fraction == 0.0)
        {
            if (Precision) memset_char(buffer, '0', Precision);
            else           *buffer = '0';
        }
        else
        {
            u32 precision = Precision ? Precision : 5;

            start = buffer;

            if constexpr (RTTI::is_same_v<RTTI::remove_cv_t<T>, f32>)
            {
                for (u32 i = 0; i < precision; ++i)
                {
                    fraction  *= 10.0f;
                    T mod      = fmodf(fraction, 10.0f);
                    *buffer++  = int_to_char[cast<u64>(mod)];
                }
            }
            else
            {
                for (u32 i = 0; i < precision; ++i)
                {
                    fraction  *= 10.0;
                    T mod      = fmod(fraction, 10.0);
                    *buffer++  = int_to_char[cast<u64>(mod)];
                }
            }

            --buffer;

            while (start < buffer)
            {
                char temp = *start;
                *start++  = *buffer;
                *buffer-- = temp;
            }
        }
    }

    template<typename T, typename = RTTI::enable_if_t<RTTI::is_vec_v<T>>>
    void __vectorcall ParseVec(char *buffer, T val)
    {
        if constexpr (RTTI::is_same_v<T, Math::v2>)
        {
            *buffer++ = '{';
            *buffer++ = ' ';
            ParseFloatingPoint(buffer, val.x);
            while (*buffer) ++buffer;
            *buffer++ = ',';
            *buffer++ = ' ';
            ParseFloatingPoint(buffer, val.y);
            while (*buffer) ++buffer;
            *buffer++ = ' ';
            *buffer++ = '}';
        }
        else if constexpr (RTTI::is_any_of_v<T, Math::v2s, Math::v2u>)
        {
            *buffer++ = '{';
            *buffer++ = ' ';
            ParseInt(buffer, val.x);
            while (*buffer) ++buffer;
            *buffer++ = ',';
            *buffer++ = ' ';
            ParseInt(buffer, val.y);
            while (*buffer) ++buffer;
            *buffer++ = ' ';
            *buffer++ = '}';
        }
        else if constexpr (RTTI::is_same_v<T, Math::v3>)
        {
            *buffer++ = '{';
            *buffer++ = ' ';
            ParseFloatingPoint(buffer, val.x);
            while (*buffer) ++buffer;
            *buffer++ = ',';
            *buffer++ = ' ';
            ParseFloatingPoint(buffer, val.y);
            while (*buffer) ++buffer;
            *buffer++ = ',';
            *buffer++ = ' ';
            ParseFloatingPoint(buffer, val.z);
            while (*buffer) ++buffer;
            *buffer++ = ' ';
            *buffer++ = '}';
        }
        else if constexpr (RTTI::is_any_of_v<T, Math::v3s, Math::v3u>)
        {
            *buffer++ = '{';
            *buffer++ = ' ';
            ParseInt(buffer, val.x);
            while (*buffer) ++buffer;
            *buffer++ = ',';
            *buffer++ = ' ';
            ParseInt(buffer, val.y);
            while (*buffer) ++buffer;
            *buffer++ = ',';
            *buffer++ = ' ';
            ParseInt(buffer, val.z);
            while (*buffer) ++buffer;
            *buffer++ = ' ';
            *buffer++ = '}';
        }
        else if constexpr (RTTI::is_same_v<T, Math::v4>)
        {
            *buffer++ = '{';
            *buffer++ = ' ';
            ParseFloatingPoint(buffer, val.x);
            while (*buffer) ++buffer;
            *buffer++ = ',';
            *buffer++ = ' ';
            ParseFloatingPoint(buffer, val.y);
            while (*buffer) ++buffer;
            *buffer++ = ',';
            *buffer++ = ' ';
            ParseFloatingPoint(buffer, val.z);
            while (*buffer) ++buffer;
            *buffer++ = ',';
            *buffer++ = ' ';
            ParseFloatingPoint(buffer, val.w);
            while (*buffer) ++buffer;
            *buffer++ = ' ';
            *buffer++ = '}';
        }
        else if constexpr (RTTI::is_any_of_v<T, Math::v4s, Math::v4u>)
        {
            *buffer++ = '{';
            *buffer++ = ' ';
            ParseInt(buffer, val.x);
            while (*buffer) ++buffer;
            *buffer++ = ',';
            *buffer++ = ' ';
            ParseInt(buffer, val.y);
            while (*buffer) ++buffer;
            *buffer++ = ',';
            *buffer++ = ' ';
            ParseInt(buffer, val.z);
            while (*buffer) ++buffer;
            *buffer++ = ',';
            *buffer++ = ' ';
            ParseInt(buffer, val.w);
            while (*buffer) ++buffer;
            *buffer++ = ' ';
            *buffer++ = '}';
        }
        else
        {
            CopyMemory(buffer, "<Undefined vector type>", REV_CSTRLEN("<Undefined vector type>"));
        }
    }

    template<typename T, typename = RTTI::enable_if_t<RTTI::is_mat_v<T>>>
    void __vectorcall ParseMat(char *buffer, T val)
    {
        if constexpr (RTTI::is_same_v<T, Math::m2>)
        {
            *buffer++ = '\n';
            *buffer++ = '[';
            *buffer++ = ' ';
            ParseFloatingPoint(buffer, val.e00);
            while (*buffer) ++buffer;
            *buffer++ = ',';
            *buffer++ = ' ';
            ParseFloatingPoint(buffer, val.e01);
            while (*buffer) ++buffer;
            *buffer++ = ' ';
            *buffer++ = ']';
            *buffer++ = '\n';

            *buffer++ = '[';
            *buffer++ = ' ';
            ParseFloatingPoint(buffer, val.e10);
            while (*buffer) ++buffer;
            *buffer++ = ',';
            *buffer++ = ' ';
            ParseFloatingPoint(buffer, val.e11);
            while (*buffer) ++buffer;
            *buffer++ = ' ';
            *buffer++ = ']';
            *buffer++ = '\n';
        }
        else if constexpr (RTTI::is_same_v<T, Math::m3>)
        {
            *buffer++ = '\n';
            *buffer++ = '[';
            *buffer++ = ' ';
            ParseFloatingPoint(buffer, val.e00);
            while (*buffer) ++buffer;
            *buffer++ = ',';
            *buffer++ = ' ';
            ParseFloatingPoint(buffer, val.e01);
            while (*buffer) ++buffer;
            *buffer++ = ',';
            *buffer++ = ' ';
            ParseFloatingPoint(buffer, val.e02);
            while (*buffer) ++buffer;
            *buffer++ = ' ';
            *buffer++ = ']';
            *buffer++ = '\n';

            *buffer++ = '[';
            *buffer++ = ' ';
            ParseFloatingPoint(buffer, val.e10);
            while (*buffer) ++buffer;
            *buffer++ = ',';
            *buffer++ = ' ';
            ParseFloatingPoint(buffer, val.e11);
            while (*buffer) ++buffer;
            *buffer++ = ',';
            *buffer++ = ' ';
            ParseFloatingPoint(buffer, val.e12);
            while (*buffer) ++buffer;
            *buffer++ = ' ';
            *buffer++ = ']';
            *buffer++ = '\n';

            *buffer++ = '[';
            *buffer++ = ' ';
            ParseFloatingPoint(buffer, val.e20);
            while (*buffer) ++buffer;
            *buffer++ = ',';
            *buffer++ = ' ';
            ParseFloatingPoint(buffer, val.e21);
            while (*buffer) ++buffer;
            *buffer++ = ',';
            *buffer++ = ' ';
            ParseFloatingPoint(buffer, val.e22);
            while (*buffer) ++buffer;
            *buffer++ = ' ';
            *buffer++ = ']';
            *buffer++ = '\n';
        }
        else if constexpr (RTTI::is_same_v<T, Math::m4>)
        {
            *buffer++ = '\n';
            *buffer++ = '[';
            *buffer++ = ' ';
            ParseFloatingPoint(buffer, val.e00);
            while (*buffer) ++buffer;
            *buffer++ = ',';
            *buffer++ = ' ';
            ParseFloatingPoint(buffer, val.e01);
            while (*buffer) ++buffer;
            *buffer++ = ',';
            *buffer++ = ' ';
            ParseFloatingPoint(buffer, val.e02);
            while (*buffer) ++buffer;
            *buffer++ = ',';
            *buffer++ = ' ';
            ParseFloatingPoint(buffer, val.e03);
            while (*buffer) ++buffer;
            *buffer++ = ' ';
            *buffer++ = ']';
            *buffer++ = '\n';

            *buffer++ = '[';
            *buffer++ = ' ';
            ParseFloatingPoint(buffer, val.e10);
            while (*buffer) ++buffer;
            *buffer++ = ',';
            *buffer++ = ' ';
            ParseFloatingPoint(buffer, val.e11);
            while (*buffer) ++buffer;
            *buffer++ = ',';
            *buffer++ = ' ';
            ParseFloatingPoint(buffer, val.e12);
            while (*buffer) ++buffer;
            *buffer++ = ',';
            *buffer++ = ' ';
            ParseFloatingPoint(buffer, val.e13);
            while (*buffer) ++buffer;
            *buffer++ = ' ';
            *buffer++ = ']';
            *buffer++ = '\n';

            *buffer++ = '[';
            *buffer++ = ' ';
            ParseFloatingPoint(buffer, val.e20);
            while (*buffer) ++buffer;
            *buffer++ = ',';
            *buffer++ = ' ';
            ParseFloatingPoint(buffer, val.e21);
            while (*buffer) ++buffer;
            *buffer++ = ',';
            *buffer++ = ' ';
            ParseFloatingPoint(buffer, val.e22);
            while (*buffer) ++buffer;
            *buffer++ = ',';
            *buffer++ = ' ';
            ParseFloatingPoint(buffer, val.e23);
            while (*buffer) ++buffer;
            *buffer++ = ' ';
            *buffer++ = ']';
            *buffer++ = '\n';

            *buffer++ = '[';
            *buffer++ = ' ';
            ParseFloatingPoint(buffer, val.e30);
            while (*buffer) ++buffer;
            *buffer++ = ',';
            *buffer++ = ' ';
            ParseFloatingPoint(buffer, val.e31);
            while (*buffer) ++buffer;
            *buffer++ = ',';
            *buffer++ = ' ';
            ParseFloatingPoint(buffer, val.e32);
            while (*buffer) ++buffer;
            *buffer++ = ',';
            *buffer++ = ' ';
            ParseFloatingPoint(buffer, val.e33);
            while (*buffer) ++buffer;
            *buffer++ = ' ';
            *buffer++ = ']';
            *buffer++ = '\n';
        }
        else
        {
            CopyMemory(buffer, "<Undefined matrix type>", REV_CSTRLEN("<Undefined matrix type>"));
        }
    }

    template<typename T, typename = RTTI::enable_if_t<RTTI::is_trivially_buildable_v<T>>>
    void AppendTrivial(T val)
    {
        u64 start_length = m_String.Length();

        char buffer[1024] = {'\0'};

        if constexpr (RTTI::is_bool_v<T>)
        {
            if (val) m_String.PushBack("true", 4);
            else     m_String.PushBack("false", 5);
        }
        else if constexpr (RTTI::is_char_v<T>)
        {
            m_String.PushBack(val);
        }
        else if constexpr (RTTI::is_cstring_v<T>)
        {
            m_String.PushBack(val);
        }
        else if constexpr (RTTI::is_pointer_v<T>)
        {
            ParsePointer(buffer, cast<u64>(val));
            m_String.PushBack(buffer);
        }
        else if constexpr (RTTI::is_nullptr_t_v<T>)
        {
            m_String.PushBack("null");
        }
        else if constexpr (RTTI::is_arithmetic_v<T> && !RTTI::is_floating_point_v<T>)
        {
            ParseInt(buffer, val);
            m_String.PushBack(buffer);
        }
        else if constexpr (RTTI::is_floating_point_v<T>)
        {
            ParseFloatingPoint(buffer, val);
            m_String.PushBack(buffer);
        }
        else if constexpr (RTTI::is_vec_v<T>)
        {
            ParseVec(buffer, val);
            m_String.PushBack(buffer);
        }
        else if constexpr (RTTI::is_mat_v<T>)
        {
            ParseMat(buffer, val);
            m_String.PushBack(buffer);
        }

        u64 appended_length = m_String.Length() - start_length;
        
        if (appended_length < Width)
        {
            if (TextAlignment == SBTA::RIGHT)
            {
                m_String.Insert(start_length, Fill, Width - appended_length);
            }
            else
            {
                m_String.Insert(start_length + appended_length, Fill, Width - appended_length);
            }
        }
    }

public:
    BASE Base;
    u32  Width;
    u32  Precision;
    SBTA TextAlignment;
    char Fill;
    bool DecorateBase;
    bool ForceSign;

private:
    String<Allocator_t> m_String;
};

template<typename Allocator_t> StringBuilder(Allocator_t *, u64, u64) -> StringBuilder<Allocator_t>;

template<typename Allocator_t> StringBuilder(const String<Allocator_t>&) -> StringBuilder<Allocator_t>;
template<typename Allocator_t> StringBuilder(String<Allocator_t>&&)      -> StringBuilder<Allocator_t>;

template<typename Allocator_t> StringBuilder(const StringBuilder<Allocator_t>&) -> StringBuilder<Allocator_t>;
template<typename Allocator_t> StringBuilder(StringBuilder<Allocator_t>&&)      -> StringBuilder<Allocator_t>;

}
