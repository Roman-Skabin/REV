//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "math/math.h"

#define MM(Type, mm, index) (cast(Type *, mm)[index])

#define MMf(m128_src, index)  ((m128_src).m128_f32[index])
#define MMi(m128i_src, index) ((m128i_src).m128i_i32[index])
#define MMu(m128i_src, index) ((m128i_src).m128i_u32[index])

#define MM128f(m128_src, index) MMf(m128_src, index)
#define MM128i(m128i_src, index) MMi(m128i_src, index)
#define MM128u(m128i_src, index) MMu(m128i_src, index)

#define MM256f(m256_src, index)  ((m256_src).m256_f32[index])
#define MM256i(m256i_src, index) ((m256i_src).m256i_i32[index])
#define MM256u(m256i_src, index) ((m256i_src).m256i_u32[index])

#define MM512f(m256_src, index)  ((m512_src).m512_f32[index])
#define MM512i(m256i_src, index) ((m512i_src).m512i_i32[index])
#define MM512u(m256i_src, index) ((m512i_src).m512i_u32[index])

#undef _MM_SHUFFLE
typedef enum MM_SHUFFLE // for _mm_shuffle_*
{
    MM_SHUFFLE_XXXX, MM_SHUFFLE_YXXX, MM_SHUFFLE_ZXXX, MM_SHUFFLE_WXXX,
    MM_SHUFFLE_XYXX, MM_SHUFFLE_YYXX, MM_SHUFFLE_ZYXX, MM_SHUFFLE_WYXX,
    MM_SHUFFLE_XZXX, MM_SHUFFLE_YZXX, MM_SHUFFLE_ZZXX, MM_SHUFFLE_WZXX,
    MM_SHUFFLE_XWXX, MM_SHUFFLE_YWXX, MM_SHUFFLE_ZWXX, MM_SHUFFLE_WWXX,
    MM_SHUFFLE_XXYX, MM_SHUFFLE_YXYX, MM_SHUFFLE_ZXYX, MM_SHUFFLE_WXYX,
    MM_SHUFFLE_XYYX, MM_SHUFFLE_YYYX, MM_SHUFFLE_ZYYX, MM_SHUFFLE_WYYX,
    MM_SHUFFLE_XZYX, MM_SHUFFLE_YZYX, MM_SHUFFLE_ZZYX, MM_SHUFFLE_WZYX,
    MM_SHUFFLE_XWYX, MM_SHUFFLE_YWYX, MM_SHUFFLE_ZWYX, MM_SHUFFLE_WWYX,
    MM_SHUFFLE_XXZX, MM_SHUFFLE_YXZX, MM_SHUFFLE_ZXZX, MM_SHUFFLE_WXZX,
    MM_SHUFFLE_XYZX, MM_SHUFFLE_YYZX, MM_SHUFFLE_ZYZX, MM_SHUFFLE_WYZX,
    MM_SHUFFLE_XZZX, MM_SHUFFLE_YZZX, MM_SHUFFLE_ZZZX, MM_SHUFFLE_WZZX,
    MM_SHUFFLE_XWZX, MM_SHUFFLE_YWZX, MM_SHUFFLE_ZWZX, MM_SHUFFLE_WWZX,
    MM_SHUFFLE_XXWX, MM_SHUFFLE_YXWX, MM_SHUFFLE_ZXWX, MM_SHUFFLE_WXWX,
    MM_SHUFFLE_XYWX, MM_SHUFFLE_YYWX, MM_SHUFFLE_ZYWX, MM_SHUFFLE_WYWX,
    MM_SHUFFLE_XZWX, MM_SHUFFLE_YZWX, MM_SHUFFLE_ZZWX, MM_SHUFFLE_WZWX,
    MM_SHUFFLE_XWWX, MM_SHUFFLE_YWWX, MM_SHUFFLE_ZWWX, MM_SHUFFLE_WWWX,

    MM_SHUFFLE_XXXY, MM_SHUFFLE_YXXY, MM_SHUFFLE_ZXXY, MM_SHUFFLE_WXXY,
    MM_SHUFFLE_XYXY, MM_SHUFFLE_YYXY, MM_SHUFFLE_ZYXY, MM_SHUFFLE_WYXY,
    MM_SHUFFLE_XZXY, MM_SHUFFLE_YZXY, MM_SHUFFLE_ZZXY, MM_SHUFFLE_WZXY,
    MM_SHUFFLE_XWXY, MM_SHUFFLE_YWXY, MM_SHUFFLE_ZWXY, MM_SHUFFLE_WWXY,
    MM_SHUFFLE_XXYY, MM_SHUFFLE_YXYY, MM_SHUFFLE_ZXYY, MM_SHUFFLE_WXYY,
    MM_SHUFFLE_XYYY, MM_SHUFFLE_YYYY, MM_SHUFFLE_ZYYY, MM_SHUFFLE_WYYY,
    MM_SHUFFLE_XZYY, MM_SHUFFLE_YZYY, MM_SHUFFLE_ZZYY, MM_SHUFFLE_WZYY,
    MM_SHUFFLE_XWYY, MM_SHUFFLE_YWYY, MM_SHUFFLE_ZWYY, MM_SHUFFLE_WWYY,
    MM_SHUFFLE_XXZY, MM_SHUFFLE_YXZY, MM_SHUFFLE_ZXZY, MM_SHUFFLE_WXZY,
    MM_SHUFFLE_XYZY, MM_SHUFFLE_YYZY, MM_SHUFFLE_ZYZY, MM_SHUFFLE_WYZY,
    MM_SHUFFLE_XZZY, MM_SHUFFLE_YZZY, MM_SHUFFLE_ZZZY, MM_SHUFFLE_WZZY,
    MM_SHUFFLE_XWZY, MM_SHUFFLE_YWZY, MM_SHUFFLE_ZWZY, MM_SHUFFLE_WWZY,
    MM_SHUFFLE_XXWY, MM_SHUFFLE_YXWY, MM_SHUFFLE_ZXWY, MM_SHUFFLE_WXWY,
    MM_SHUFFLE_XYWY, MM_SHUFFLE_YYWY, MM_SHUFFLE_ZYWY, MM_SHUFFLE_WYWY,
    MM_SHUFFLE_XZWY, MM_SHUFFLE_YZWY, MM_SHUFFLE_ZZWY, MM_SHUFFLE_WZWY,
    MM_SHUFFLE_XWWY, MM_SHUFFLE_YWWY, MM_SHUFFLE_ZWWY, MM_SHUFFLE_WWWY,

    MM_SHUFFLE_XXXZ, MM_SHUFFLE_YXXZ, MM_SHUFFLE_ZXXZ, MM_SHUFFLE_WXXZ,
    MM_SHUFFLE_XYXZ, MM_SHUFFLE_YYXZ, MM_SHUFFLE_ZYXZ, MM_SHUFFLE_WYXZ,
    MM_SHUFFLE_XZXZ, MM_SHUFFLE_YZXZ, MM_SHUFFLE_ZZXZ, MM_SHUFFLE_WZXZ,
    MM_SHUFFLE_XWXZ, MM_SHUFFLE_YWXZ, MM_SHUFFLE_ZWXZ, MM_SHUFFLE_WWXZ,
    MM_SHUFFLE_XXYZ, MM_SHUFFLE_YXYZ, MM_SHUFFLE_ZXYZ, MM_SHUFFLE_WXYZ,
    MM_SHUFFLE_XYYZ, MM_SHUFFLE_YYYZ, MM_SHUFFLE_ZYYZ, MM_SHUFFLE_WYYZ,
    MM_SHUFFLE_XZYZ, MM_SHUFFLE_YZYZ, MM_SHUFFLE_ZZYZ, MM_SHUFFLE_WZYZ,
    MM_SHUFFLE_XWYZ, MM_SHUFFLE_YWYZ, MM_SHUFFLE_ZWYZ, MM_SHUFFLE_WWYZ,
    MM_SHUFFLE_XXZZ, MM_SHUFFLE_YXZZ, MM_SHUFFLE_ZXZZ, MM_SHUFFLE_WXZZ,
    MM_SHUFFLE_XYZZ, MM_SHUFFLE_YYZZ, MM_SHUFFLE_ZYZZ, MM_SHUFFLE_WYZZ,
    MM_SHUFFLE_XZZZ, MM_SHUFFLE_YZZZ, MM_SHUFFLE_ZZZZ, MM_SHUFFLE_WZZZ,
    MM_SHUFFLE_XWZZ, MM_SHUFFLE_YWZZ, MM_SHUFFLE_ZWZZ, MM_SHUFFLE_WWZZ,
    MM_SHUFFLE_XXWZ, MM_SHUFFLE_YXWZ, MM_SHUFFLE_ZXWZ, MM_SHUFFLE_WXWZ,
    MM_SHUFFLE_XYWZ, MM_SHUFFLE_YYWZ, MM_SHUFFLE_ZYWZ, MM_SHUFFLE_WYWZ,
    MM_SHUFFLE_XZWZ, MM_SHUFFLE_YZWZ, MM_SHUFFLE_ZZWZ, MM_SHUFFLE_WZWZ,
    MM_SHUFFLE_XWWZ, MM_SHUFFLE_YWWZ, MM_SHUFFLE_ZWWZ, MM_SHUFFLE_WWWZ,

    MM_SHUFFLE_XXXW, MM_SHUFFLE_YXXW, MM_SHUFFLE_ZXXW, MM_SHUFFLE_WXXW,
    MM_SHUFFLE_XYXW, MM_SHUFFLE_YYXW, MM_SHUFFLE_ZYXW, MM_SHUFFLE_WYXW,
    MM_SHUFFLE_XZXW, MM_SHUFFLE_YZXW, MM_SHUFFLE_ZZXW, MM_SHUFFLE_WZXW,
    MM_SHUFFLE_XWXW, MM_SHUFFLE_YWXW, MM_SHUFFLE_ZWXW, MM_SHUFFLE_WWXW,
    MM_SHUFFLE_XXYW, MM_SHUFFLE_YXYW, MM_SHUFFLE_ZXYW, MM_SHUFFLE_WXYW,
    MM_SHUFFLE_XYYW, MM_SHUFFLE_YYYW, MM_SHUFFLE_ZYYW, MM_SHUFFLE_WYYW,
    MM_SHUFFLE_XZYW, MM_SHUFFLE_YZYW, MM_SHUFFLE_ZZYW, MM_SHUFFLE_WZYW,
    MM_SHUFFLE_XWYW, MM_SHUFFLE_YWYW, MM_SHUFFLE_ZWYW, MM_SHUFFLE_WWYW,
    MM_SHUFFLE_XXZW, MM_SHUFFLE_YXZW, MM_SHUFFLE_ZXZW, MM_SHUFFLE_WXZW,
    MM_SHUFFLE_XYZW, MM_SHUFFLE_YYZW, MM_SHUFFLE_ZYZW, MM_SHUFFLE_WYZW,
    MM_SHUFFLE_XZZW, MM_SHUFFLE_YZZW, MM_SHUFFLE_ZZZW, MM_SHUFFLE_WZZW,
    MM_SHUFFLE_XWZW, MM_SHUFFLE_YWZW, MM_SHUFFLE_ZWZW, MM_SHUFFLE_WWZW,
    MM_SHUFFLE_XXWW, MM_SHUFFLE_YXWW, MM_SHUFFLE_ZXWW, MM_SHUFFLE_WXWW,
    MM_SHUFFLE_XYWW, MM_SHUFFLE_YYWW, MM_SHUFFLE_ZYWW, MM_SHUFFLE_WYWW,
    MM_SHUFFLE_XZWW, MM_SHUFFLE_YZWW, MM_SHUFFLE_ZZWW, MM_SHUFFLE_WZWW,
    MM_SHUFFLE_XWWW, MM_SHUFFLE_YWWW, MM_SHUFFLE_ZWWW, MM_SHUFFLE_WWWW
} MM_SHUFFLE;

#undef _MM_EXTRACT_FLOAT
#define mm_extract_ps(f32_dst, m128_src, imm_index)  \
{                                                    \
    union __intrin_type __align(4)                   \
    {                                                \
        f32 f;                                       \
        int i;                                       \
    } dst;                                           \
    dst.i     = _mm_extract_ps(m128_src, imm_index); \
    (f32_dst) = dst.f;                               \
}

#define mm256_extract_ps(f32_dst, m256_src, imm_index) \
{                                                      \
    union __intrin_type __align(32)                    \
    {                                                  \
        __m256  f;                                     \
        __m256i i;                                     \
    } mm;                                              \
    union __intrin_type __align(4)                     \
    {                                                  \
        f32 f;                                         \
        int i;                                         \
    } dst;                                             \
    mm.f      = (m256_src);                            \
    dst.i     = _mm256_extract_epi32(mm.i, imm_index); \
    (f32_dst) = dst.f;                                 \
}

#define mm_insert_f32(m128_dst, m128_src, f32_val, imm_index)      \
{                                                                  \
    union __intrin_type __align(16)                                \
    {                                                              \
        __m128  f;                                                 \
        __m128i i;                                                 \
    } mm_src, mm_dst;                                              \
    union __intrin_type __align(4)                                 \
    {                                                              \
        f32 f;                                                     \
        int i;                                                     \
    } val_src;                                                     \
    mm_src.f   = (m128_src);                                       \
    val_src.f  = (f32_val);                                        \
    mm_dst.i   = _mm_insert_epi32(mm_src.i, val_src.i, imm_index); \
    (m128_dst) = mm_dst.f;                                         \
}

#define mm256_insert_f32(m256_dst, m256_src, f32_val, imm_index)      \
{                                                                     \
    union __intrin_type __align(32)                                   \
    {                                                                 \
        __m256  f;                                                    \
        __m256i i;                                                    \
    } mm_src, mm_dst;                                                 \
    union __intrin_type __align(4)                                    \
    {                                                                 \
        f32 f;                                                        \
        int i;                                                        \
    } val_src;                                                        \
    mm_src.f   = (m256_src);                                          \
    val_src.f  = (f32_val);                                           \
    mm_dst.i   = _mm256_insert_epi32(mm_src.i, val_src.i, imm_index); \
    (m256_dst) = mm_dst.f;                                            \
}
