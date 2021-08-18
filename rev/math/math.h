//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/common.h"

namespace REV::Math
{
    inline constexpr const f64 g_f64_E        = 2.71828182845904523536;  // e
    inline constexpr const f64 g_f64_LOG2E    = 1.44269504088896340736;  // log2(e)
    inline constexpr const f64 g_f64_LOG10E   = 0.434294481903251827651; // log10(e)
    inline constexpr const f64 g_f64_LN2      = 0.693147180559945309417; // ln(2)
    inline constexpr const f64 g_f64_LN10     = 2.30258509299404568402;  // ln(10)
    inline constexpr const f64 g_f64_PI       = 3.141592653589793238463; // pi
    inline constexpr const f64 g_f64_PI_2     = 1.57079632679489661923;  // pi/2
    inline constexpr const f64 g_f64_PI_4     = 0.785398163397448309616; // pi/4
    inline constexpr const f64 g_f64_1_PI     = 0.318309886183790671538; // 1/pi
    inline constexpr const f64 g_f64_2_PI     = 0.636619772367581343076; // 2/pi
    inline constexpr const f64 g_f64_2_SQRTPI = 1.12837916709551257390;  // 2/sqrt(pi)
    inline constexpr const f64 g_f64_SQRT2    = 1.41421356237309504880;  // sqrt(2)
    inline constexpr const f64 g_f64_1_SQRT_2 = 0.707106781186547524401; // 1/sqrt(2)
    inline constexpr const f64 g_f64_TAU      = 6.283185307179586476925; // TAU = 2pi

    inline constexpr const f32 g_f32_E        = 2.71828182845904523536f;  // e
    inline constexpr const f32 g_f32_LOG2E    = 1.44269504088896340736f;  // log2(e)
    inline constexpr const f32 g_f32_LOG10E   = 0.434294481903251827651f; // log10(e)
    inline constexpr const f32 g_f32_LN2      = 0.693147180559945309417f; // ln(2)
    inline constexpr const f32 g_f32_LN10     = 2.30258509299404568402f;  // ln(10)
    inline constexpr const f32 g_f32_PI       = 3.141592653589793238463f; // pi
    inline constexpr const f32 g_f32_PI_2     = 1.57079632679489661923f;  // pi/2
    inline constexpr const f32 g_f32_PI_4     = 0.785398163397448309616f; // pi/4
    inline constexpr const f32 g_f32_1_PI     = 0.318309886183790671538f; // 1/pi
    inline constexpr const f32 g_f32_2_PI     = 0.636619772367581343076f; // 2/pi
    inline constexpr const f32 g_f32_2_SQRTPI = 1.12837916709551257390f;  // 2/sqrt(pi)
    inline constexpr const f32 g_f32_SQRT2    = 1.41421356237309504880f;  // sqrt(2)
    inline constexpr const f32 g_f32_1_SQRT_2 = 0.707106781186547524401f; // 1/sqrt(2)
    inline constexpr const f32 g_f32_TAU      = 6.283185307179586476925f; // TAU = 2pi

    template<typename T, typename = RTTI::enable_if_t<RTTI::is_floating_point_v<T>>>
    REV_INLINE T REV_VECTORCALL lerp(T start, T end, T percent)
    {
        return (end - start) * percent + start;
    }

    template<typename T, typename = RTTI::enable_if_t<RTTI::is_floating_point_v<T>>>
    REV_INLINE T REV_VECTORCALL invlerp(T start, T end, T value) // ret = [0, 1]
    {
        return (value - start) / (end - start);
    }

    template<typename T, typename = RTTI::enable_if_t<RTTI::is_floating_point_v<T>>>
    REV_INLINE T REV_VECTORCALL invlerp_n(T start, T end, T value) // ret = [-1, 1]
    {
        return ((value - start) / (end - start) - 0.5f) * 2.0f;
    }

    // value <= 20
    REV_API u64 fact(u8 value);

    REV_INLINE void REV_VECTORCALL sincos(rad32 a, f32& s, f32& c)
    {
        __m128 cos_res;
        s = _mm_cvtss_f32(_mm_sincos_ps(&cos_res, _mm_load_ss(&a)));
        c = _mm_cvtss_f32(cos_res);
    }

    REV_INLINE void REV_VECTORCALL sincos(rad64 a, f64& s, f64& c)
    {
        __m128d cos_res;
        s = _mm_cvtsd_f64(_mm_sincos_pd(&cos_res, _mm_load_sd(&a)));
        c = _mm_cvtsd_f64(cos_res);
    }

    //
    // @NOTE(Roman): Branchless functions
    //

    template<typename T, typename = RTTI::enable_if_t<RTTI::is_arithmetic_v<T>>>
    REV_INLINE T REV_VECTORCALL abs(T val)
    {
        if constexpr (sizeof(T) == 1)
        {
            if (RTTI::is_signed_v<T>)
            {
                __m128i mm_abs = _mm_abs_epi8(_mm_set1_epi8(*cast<char *>(&val)));
                return *cast<T *>(mm_abs.m128i_i8);
            }
            return val;
        }
        else if constexpr (sizeof(T) == 2)
        {
            if (RTTI::is_signed_v<T>)
            {
                __m128i mm_abs = _mm_abs_epi16(_mm_set1_epi16(*cast<short *>(&val)));
                return *cast<T *>(mm_abs.m128i_i16);
            }
            return val;
        }
        else if constexpr (sizeof(T) == 4)
        {
            if constexpr (RTTI::is_floating_point_v<T>)
            {
                return fabsf(val);
            }
            else if constexpr (RTTI::is_signed_v<T>)
            {
                __m128i mm_abs = _mm_abs_epi32(_mm_set1_epi32(*cast<int *>(&val)));
                return *cast<T *>(mm_abs.m128i_i32);
            }
            return val;
        }
        else if constexpr (sizeof(T) == 8)
        {
            if constexpr (RTTI::is_floating_point_v<T>)
            {
                return fabs(val);
            }
            else if constexpr (RTTI::is_signed_v<T>)
            {
                #if REV_ISA >= REV_ISA_AVX512
                    __m128i mm_abs = _mm_abs_epi64(_mm_set1_epi64(cast<int *>(&val)));
                    return *cast<T *>(mm_abs.m128i_i32);
                #else
                    return val < 0 ? -val : val;
                #endif
            }
            return val;
        }
        else
        {
            return val < 0 ? -val : val;
        }
    }

    template<typename T, typename = RTTI::enable_if_t<RTTI::is_arithmetic_v<T>>>
    REV_INLINE bool REV_VECTORCALL is_negative(T val)
    {
        /**/ if constexpr (RTTI::is_unsigned_v<T>) return false;
        else if constexpr (sizeof(T) == 1) return *cast<u8  *>(&val) & 0x80;
        else if constexpr (sizeof(T) == 2) return *cast<u16 *>(&val) & 0x8000;
        else if constexpr (sizeof(T) == 4) return *cast<u32 *>(&val) & 0x8000'0000;
        else if constexpr (sizeof(T) == 8) return *cast<u64 *>(&val) & 0x8000'0000'0000'0000;
        else return val < 0;
    }

    template<typename T>
    REV_INLINE T REV_VECTORCALL min(T l, T r)
    {
    #if REV_RELEASE
        return l < r ? l : r;
    #else
        if constexpr (sizeof(T) == 1)
        {
            if constexpr (RTTI::is_signed_v<T>)
            {
                __m128i mm_min = _mm_min_epi8(_mm_set1_epi8(*cast<char *>(&l)), _mm_set1_epi8(*cast<char *>(&r)));
                return *cast<T *>(mm_min.m128i_i8);
            }
            else
            {
                __m128i mm_min = _mm_min_epu8(_mm_set1_epi8(*cast<char *>(&l)), _mm_set1_epi8(*cast<char *>(&r)));
                return *cast<T *>(mm_min.m128i_u8);
            }
        }
        else if constexpr (sizeof(T) == 2)
        {
            if constexpr (RTTI::is_signed_v<T>)
            {
                __m128i mm_min = _mm_min_epi16(_mm_cvtsi32_si128(*cast<s16 *>(&l)), _mm_cvtsi32_si128(*cast<s16 *>(&r)));
                return *cast<T *>(mm_min.m128i_i16);
            }
            else
            {
                __m128i mm_min = _mm_min_epu16(_mm_cvtsi32_si128(*cast<u16 *>(&l)), _mm_cvtsi32_si128(*cast<u16 *>(&r)));
                return *cast<T *>(mm_min.m128i_u16);
            }
        }
        else if constexpr (sizeof(T) == 4)
        {
            if constexpr (RTTI::is_floating_point_v<T>)
            {
                __m128 mm_min = _mm_min_ss(_mm_load_ss(&l), _mm_load_ss(&r));
                return *cast<T *>(mm_min.m128_f32);
            }
            else if constexpr (RTTI::is_signed_v<T>)
            {
                __m128i mm_min = _mm_min_epi32(_mm_cvtsi32_si128(*cast<int *>(&l)), _mm_cvtsi32_si128(*cast<int *>(&r)));
                return *cast<T *>(mm_min.m128i_i32);
            }
            else
            {
                __m128i mm_min = _mm_min_epu32(_mm_cvtsi32_si128(*cast<int *>(&l)), _mm_cvtsi32_si128(*cast<int *>(&r)));
                return *cast<T *>(mm_min.m128i_u32);
            }
        }
        else if constexpr (sizeof(T) == 8)
        {
            if constexpr (RTTI::is_floating_point_v<T>)
            {
                __m128d mm_min = _mm_min_sd(_mm_load_sd(&l), _mm_load_sd(&r));
                return *cast<T *>(mm_min.m128d_f64);
            }
            else if constexpr (RTTI::is_signed_v<T>)
            {
                #if REV_ISA >= REV_ISA_AVX512
                    __m128i mm_min = _mm_min_epi64(_mm_cvtsi64_si128(*cast<s64 *>(&l)), _mm_cvtsi64_si128(*cast<s64 *>(&r)));
                    return *cast<T *>(mm_min.m128i_i64);
                #else
                    __m128i mm_l    = _mm_cvtsi64_si128(*cast<s64 *>(&l));
                    __m128i mm_r    = _mm_cvtsi64_si128(*cast<s64 *>(&r));
                    __m128i gt_mask = _mm_cmpgt_epi64(mm_l, mm_r);
                    __m128i mm_min  = _mm_blendv_epi8(mm_l, mm_r, gt_mask); // @NOTE(Roman): dest[i+7:i] = mask[i+7:i] ? b[i+7:i] : a[i+7:i];
                    return *cast<T *>(mm_min.m128i_i64);
                #endif
            }
            else
            {
                #if REV_ISA >= REV_ISA_AVX512
                    __m128i mm_min = _mm_min_epu64(_mm_cvtsi64_si128(*cast<s64 *>(&l)), _mm_cvtsi64_si128(*cast<s64 *>(&r)));
                    return *cast<T *>(mm_min.m128i_u64);
                #else
                    return l < r ? l : r;
                #endif
            }
        }
        else
        {
            return l < r ? l : r;
        }
    #endif
    }

    template<typename T>
    REV_INLINE T REV_VECTORCALL max(T l, T r)
    {
    #if REV_RELEASE
        return l > r ? l : r;
    #else
        if constexpr (sizeof(T) == 1)
        {
            if constexpr (RTTI::is_signed_v<T>)
            {
                __m128i mm_max = _mm_max_epi8(_mm_set1_epi8(*cast<char *>(&l)), _mm_set1_epi8(*cast<char *>(&r)));
                return *cast<T *>(mm_max.m128i_i8);
            }
            else
            {
                __m128i mm_max = _mm_max_epu8(_mm_set1_epi8(*cast<char *>(&l)), _mm_set1_epi8(*cast<char *>(&r)));
                return *cast<T *>(mm_max.m128i_u8);
            }
        }
        else if constexpr (sizeof(T) == 2)
        {
            if constexpr (RTTI::is_signed_v<T>)
            {
                __m128i mm_max = _mm_max_epi16(_mm_cvtsi32_si128(*cast<s16 *>(&l)), _mm_cvtsi32_si128(*cast<s16 *>(&r)));
                return *cast<T *>(mm_max.m128i_i16);
            }
            else
            {
                __m128i mm_max = _mm_max_epu16(_mm_cvtsi32_si128(*cast<u16 *>(&l)), _mm_cvtsi32_si128(*cast<u16 *>(&r)));
                return *cast<T *>(mm_max.m128i_u16);
            }
        }
        else if constexpr (sizeof(T) == 4)
        {
            if constexpr (RTTI::is_floating_point_v<T>)
            {
                __m128 mm_max = _mm_max_ss(_mm_load_ss(&l), _mm_load_ss(&r));
                return *cast<T *>(mm_max.m128_f32);
            }
            else if constexpr (RTTI::is_signed_v<T>)
            {
                __m128i mm_max = _mm_max_epi32(_mm_cvtsi32_si128(*cast<int *>(&l)), _mm_cvtsi32_si128(*cast<int *>(&r)));
                return *cast<T *>(mm_max.m128i_i32);
            }
            else
            {
                __m128i mm_max = _mm_max_epu32(_mm_cvtsi32_si128(*cast<int *>(&l)), _mm_cvtsi32_si128(*cast<int *>(&r)));
                return *cast<T *>(mm_max.m128i_u32);
            }
        }
        else if constexpr (sizeof(T) == 8)
        {
            if constexpr (RTTI::is_floating_point_v<T>)
            {
                __m128d mm_max = _mm_max_sd(_mm_load_sd(&l), _mm_load_sd(&r));
                return *cast<T *>(mm_max.m128d_f64);
            }
            else if constexpr (RTTI::is_signed_v<T>)
            {
                #if REV_ISA >= REV_ISA_AVX512
                    __m128i mm_min = _mm_max_epi64(_mm_cvtsi64_si128(*cast<s64 *>(&l)), _mm_cvtsi64_si128(*cast<s64 *>(&r)));
                    return *cast<T *>(mm_min.m128i_i64);
                #else
                    __m128i mm_l    = _mm_cvtsi64_si128(*cast<s64 *>(&l));
                    __m128i mm_r    = _mm_cvtsi64_si128(*cast<s64 *>(&r));
                    __m128i gt_mask = _mm_cmpgt_epi64(mm_l, mm_r);
                    __m128i mm_max  = _mm_blendv_epi8(mm_r, mm_l, gt_mask); // @NOTE(Roman): dest[i+7:i] = mask[i+7:i] ? b[i+7:i] : a[i+7:i];
                    return *cast<T *>(mm_max.m128i_i64);
                #endif
            }
            else
            {
                #if REV_ISA >= REV_ISA_AVX512
                    __m128i mm_min = _mm_max_epu64(_mm_cvtsi64_si128(*cast<s64 *>(&l)), _mm_cvtsi64_si128(*cast<s64 *>(&r)));
                    return *cast<T *>(mm_min.m128i_u64);
                #else
                    return l > r ? l : r;
                #endif
            }
        }
        else
        {
            return l > r ? l : r;
        }
    #endif
    }

    template<typename T, typename = RTTI::enable_if_t<RTTI::is_arithmetic_v<T>>>
    REV_INLINE T REV_VECTORCALL clamp(T val, T min_val, T max_val)
    {
    #if REV_RELEASE
        T min_res = val < max_val ? val : max_val;
        return min_val > min_res ? min_val : min_res;
    #else
        if constexpr (sizeof(T) == 1)
        {
            __m128i mm_val = _mm_set1_epi8(*cast<char *>(&val));
            __m128i mm_min = _mm_set1_epi8(*cast<char *>(&min_val));
            __m128i mm_max = _mm_set1_epi8(*cast<char *>(&max_val));

            if constexpr (RTTI::is_signed_v<T>)
            {
                __m128i mm_res = _mm_max_epi8(mm_min, _mm_min_epi8(mm_val, mm_max));
                return *cast<T *>(mm_res.m128i_i8);
            }
            else
            {
                __m128i mm_res = _mm_max_epu8(mm_min, _mm_min_epu8(mm_val, mm_max));
                return *cast<T *>(mm_res.m128i_u8);
            }
        }
        else if constexpr (sizeof(T) == 2)
        {
            __m128i mm_val = _mm_cvtsi32_si128(*cast<short *>(&val));
            __m128i mm_min = _mm_cvtsi32_si128(*cast<short *>(&min_val));
            __m128i mm_max = _mm_cvtsi32_si128(*cast<short *>(&max_val));

            if constexpr (RTTI::is_signed_v<T>)
            {
                __m128i mm_res = _mm_max_epi16(mm_min, _mm_min_epi16(mm_val, mm_max));
                return *cast<T *>(mm_res.m128i_i16);
            }
            else
            {
                __m128i mm_res = _mm_max_epu16(mm_min, _mm_min_epu16(mm_val, mm_max));
                return *cast<T *>(mm_res.m128i_u16);
            }
        }
        else if constexpr (sizeof(T) == 4)
        {
            if constexpr (RTTI::is_floating_point_v<T>)
            {
                __m128 mm_val = _mm_load_ss(&val);
                __m128 mm_min = _mm_load_ss(&min_val);
                __m128 mm_max = _mm_load_ss(&max_val);
                __m128 mm_res = _mm_max_ss(mm_min, _mm_min_ss(mm_val, mm_max));
                return *cast<T *>(mm_res.m128_f32);
            }
            else
            {
                __m128i mm_val = _mm_cvtsi32_si128(*cast<int *>(&val));
                __m128i mm_min = _mm_cvtsi32_si128(*cast<int *>(&min_val));
                __m128i mm_max = _mm_cvtsi32_si128(*cast<int *>(&max_val));

                if constexpr (RTTI::is_signed_v<T>)
                {
                    __m128i mm_res = _mm_max_epi32(mm_min, _mm_min_epi32(mm_val, mm_max));
                    return *cast<T *>(mm_res.m128i_i32);
                }
                else
                {
                    __m128i mm_res = _mm_max_epu32(mm_min, _mm_min_epu32(mm_val, mm_max));
                    return *cast<T *>(mm_res.m128i_u32);
                }
            }
        }
        else if constexpr (sizeof(T) == 8)
        {
            if constexpr (RTTI::is_floating_point_v<T>)
            {
                __m128d mm_val = _mm_load_sd(&val);
                __m128d mm_min = _mm_load_sd(&min_val);
                __m128d mm_max = _mm_load_sd(&max_val);
                __m128d mm_res = _mm_max_sd(mm_min, _mm_min_sd(mm_val, mm_max));
                return *cast<T *>(mm_res.m128d_f64);
            }
            else if constexpr (RTTI::is_signed_v<T>)
            {
                __m128i mm_val = _mm_cvtsi64_si128(*cast<s64 *>(&val));
                __m128i mm_min = _mm_cvtsi64_si128(*cast<s64 *>(&min_val));
                __m128i mm_max = _mm_cvtsi64_si128(*cast<s64 *>(&max_val));

                #if REV_ISA >= REV_ISA_AVX512
                    __m128i mm_res = _mm_max_epi64(mm_min, _mm_min_epi64(mm_val, mm_max));
                    return *cast<T *>(mm_res.m128i_i64);
                #else
                    __m128i min_res = _mm_blendv_epi8(mm_val,  mm_max, _mm_cmpgt_epi64(mm_val, mm_max));
                    __m128i mm_res  = _mm_blendv_epi8(min_res, mm_min, _mm_cmpgt_epi64(mm_min, min_res));
                    return *cast<T *>(mm_res.m128i_i64);
                #endif
            }
            else
            {
                #if REV_ISA >= REV_ISA_AVX512
                    __m128i mm_val = _mm_cvtsi64_si128(*cast<s64 *>(&val));
                    __m128i mm_min = _mm_cvtsi64_si128(*cast<s64 *>(&min_val));
                    __m128i mm_max = _mm_cvtsi64_si128(*cast<s64 *>(&max_val));
                    __m128i mm_res = _mm_max_epu64(mm_min, _mm_min_epu64(mm_val, mm_max));
                    return *cast<T *>(mm_res.m128i_u64);
                #else
                    T min_res = val < max_val ? val : max_val;
                    return min_val > min_res ? min_val : min_res;
                #endif
            }
        }
        else
        {
            T min_res = val < max_val ? val : max_val;
            return min_val > min_res ? min_val : min_res;
        }
    #endif
    }

    template<typename Ret, typename T,
             typename = RTTI::enable_if_t<RTTI::is_arithmetic_v<T>>,
             typename = RTTI::enable_if_t<RTTI::is_arithmetic_v<Ret>>>
    REV_INLINE Ret REV_VECTORCALL clamp(T val, T min_val, T max_val)
    {
        if constexpr (RTTI::is_same_v<Ret, T>)
        {
            return clamp(val, min_val, max_val);
        }
        else
        {
            return cast<Ret>(clamp(val, min_val, max_val));
        }
    }
}
