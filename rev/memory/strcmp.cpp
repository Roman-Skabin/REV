//
// Copyright 2020-2021 Roman Skabin
//

#include "core/pch.h"
#include "memory/memlow.h"

namespace REV
{

COMPARE_RESULT REV_VECTORCALL CompareStrings(const char *left, u64 left_length, const char *right, u64 right_length)
{
    if (left_length < right_length) return COMPARE_RESULT_LT;
    if (left_length > right_length) return COMPARE_RESULT_GT;

    u64 chunk_length = left_length > 16 ? 16 : left_length;

    while (left_length)
    {
        __m128i mm_left  = _mm_lddqu_si128(reinterpret_cast<const __m128i *>(left));
        __m128i mm_right = _mm_lddqu_si128(reinterpret_cast<const __m128i *>(right));

        int index = _mm_cmpestri(mm_left,  static_cast<int>(chunk_length),
                                 mm_right, static_cast<int>(chunk_length),
                                 _SIDD_SBYTE_OPS | _SIDD_CMP_EQUAL_EACH | _SIDD_MASKED_NEGATIVE_POLARITY | _SIDD_LEAST_SIGNIFICANT);

        if (index < chunk_length)
        {
            char l = mm_left.m128i_i8[index];
            char r = mm_right.m128i_i8[index];

            if (l < r) return COMPARE_RESULT_LT;
            if (l > r) return COMPARE_RESULT_GT;
        }

        left_length  -= chunk_length;
        chunk_length  = left_length > 16 ? 16 : left_length;

        left  += chunk_length;
        right += chunk_length;
    }

    return COMPARE_RESULT_EQ;
}

COMPARE_RESULT REV_VECTORCALL CompareUnicodeStrings(const wchar_t *left, u64 left_length, const wchar_t *right, u64 right_length)
{
    if (left_length < right_length) return COMPARE_RESULT_LT;
    if (left_length > right_length) return COMPARE_RESULT_GT;

    u64 chunk_length = left_length > 8 ? 8 : left_length;

    while (left_length)
    {
        __m128i mm_left  = _mm_lddqu_si128(reinterpret_cast<const __m128i *>(left));
        __m128i mm_right = _mm_lddqu_si128(reinterpret_cast<const __m128i *>(right));

        int index = _mm_cmpestri(mm_left,  static_cast<int>(chunk_length),
                                 mm_right, static_cast<int>(chunk_length),
                                 _SIDD_SWORD_OPS | _SIDD_CMP_EQUAL_EACH | _SIDD_MASKED_NEGATIVE_POLARITY | _SIDD_LEAST_SIGNIFICANT);

        if (index < chunk_length)
        {
            wchar_t l = mm_left.m128i_i16[index];
            wchar_t r = mm_right.m128i_i16[index];

            if (l < r) return COMPARE_RESULT_LT;
            if (l > r) return COMPARE_RESULT_GT;
        }

        left_length  -= chunk_length;
        chunk_length  = left_length > 8 ? 8 : left_length;

        left  += chunk_length;
        right += chunk_length;
    }

    return COMPARE_RESULT_EQ;
}

COMPARE_RESULT REV_VECTORCALL CompareStringsAligned(const char *left, u64 left_length, const char *right, u64 right_length)
{
    if (left_length < right_length) return COMPARE_RESULT_LT;
    if (left_length > right_length) return COMPARE_RESULT_GT;

    REV_CHECK_M(left_length % 16 == 0, "Strings must be 16-byte aligned!");

    while (left_length)
    {
        __m128i mm_left  = _mm_lddqu_si128(reinterpret_cast<const __m128i *>(left));
        __m128i mm_right = _mm_lddqu_si128(reinterpret_cast<const __m128i *>(right));

        int index = _mm_cmpestri(mm_left,  16,
                                 mm_right, 16,
                                 _SIDD_SBYTE_OPS | _SIDD_CMP_EQUAL_EACH | _SIDD_MASKED_NEGATIVE_POLARITY | _SIDD_LEAST_SIGNIFICANT);

        if (index < 16)
        {
            char l = mm_left.m128i_i8[index];
            char r = mm_right.m128i_i8[index];

            if (l < r) return COMPARE_RESULT_LT;
            if (l > r) return COMPARE_RESULT_GT;
        }

        left_length -= 16;
        left        += 16;
        right       += 16;
    }

    return COMPARE_RESULT_EQ;
}

COMPARE_RESULT REV_VECTORCALL CompareUnicodeStringsAligned(const wchar_t *left, u64 left_length, const wchar_t *right, u64 right_length)
{
    if (left_length < right_length) return COMPARE_RESULT_LT;
    if (left_length > right_length) return COMPARE_RESULT_GT;

    REV_CHECK_M(left_length % 8 == 0, "Strings must be 16-byte aligned!");

    while (left_length)
    {
        __m128i mm_left  = _mm_lddqu_si128(reinterpret_cast<const __m128i *>(left));
        __m128i mm_right = _mm_lddqu_si128(reinterpret_cast<const __m128i *>(right));

        int index = _mm_cmpestri(mm_left,  8,
                                 mm_right, 8,
                                 _SIDD_SWORD_OPS | _SIDD_CMP_EQUAL_EACH | _SIDD_MASKED_NEGATIVE_POLARITY | _SIDD_LEAST_SIGNIFICANT);

        if (index < 8)
        {
            wchar_t l = mm_left.m128i_i16[index];
            wchar_t r = mm_right.m128i_i16[index];

            if (l < r) return COMPARE_RESULT_LT;
            if (l > r) return COMPARE_RESULT_GT;
        }

        left_length -= 8;
        left        += 8;
        right       += 8;
    }

    return COMPARE_RESULT_EQ;
}

}
