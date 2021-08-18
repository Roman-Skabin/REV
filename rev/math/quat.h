//
// Copyright 2020-2021 Roman Skabin
//

#pragma once

#include "math/mat.h"

#pragma pack(push, 1)
namespace REV::Math
{

// @TODO(Roman): #Bug: Remove _mm_sign_epi32 functions

union REV_INTRIN_TYPE quat final
{
    struct { f32 x, y, z, w; };
    struct { v3 v; f32 s; };
    f32   e[4];
    xmm_u mm_u;

    REV_INLINE quat() : mm_u(mm_insert_f32<3>(_mm_setzero_ps(), 1.0f)) {}
    REV_INLINE quat(v3 around, rad angle)
    {
        __m128 vec        = around.load();
        __m128 vec_normal = _mm_div_ps(vec, _mm_sqrt_ps(_mm_dp_ps(vec, vec, 0x7F)));

        __m128 cos_2;
        __m128 sin_2 = _mm_sincos_ps(&cos_2, _mm_set_ps1(angle / 2.0f));

        __m128 sincos_2 = _mm_blend_ps(sin_2, cos_2, MM_BLEND_AAAB);

        mm_u = _mm_mul_ps(vec_normal, sincos_2);
    }
    REV_INLINE quat(v4 around, rad angle)
    {
        __m128 vec        = mm_insert_f32<3>(around.mm, 0.0f);
        __m128 vec_normal = _mm_div_ps(vec, _mm_sqrt_ps(_mm_dp_ps(vec, vec, 0x7F)));

        __m128 cos_2;
        __m128 sin_2 = _mm_sincos_ps(&cos_2, _mm_set_ps1(angle / 2.0f));

        __m128 sincos_2 = _mm_blend_ps(sin_2, cos_2, MM_BLEND_AAAB);

        mm_u = _mm_mul_ps(vec_normal, sincos_2);
    }
    REV_INLINE quat(f32 x, f32 y, f32 z, f32 w) : mm_u(_mm_loadu_ps(&x))   {}
    REV_INLINE quat(f32 e[4])                   : mm_u(_mm_loadu_ps(e))    {}
    REV_INLINE quat(__m128 mm)                  : mm_u(mm)                 {}
    REV_INLINE quat(const quat& q)              : mm_u(q.mm_u)             {}
    REV_INLINE quat(quat&& q)                   : mm_u(RTTI::move(q.mm_u)) {}

    REV_INLINE quat& REV_VECTORCALL operator=(f32 e[4])      { mm_u = _mm_loadu_ps(e);    return *this; }
    REV_INLINE quat& REV_VECTORCALL operator=(const quat& q) { mm_u = q.mm_u;             return *this; }
    REV_INLINE quat& REV_VECTORCALL operator=(quat&& q)      { mm_u = RTTI::move(q.mm_u); return *this; }

    REV_INLINE quat REV_VECTORCALL conjugate() const
    {
        return quat(_mm_castsi128_ps(_mm_sign_epi32(_mm_castps_si128(mm_u), _mm_setr_epi32(MM_SIGN_NEG, MM_SIGN_NEG, MM_SIGN_NEG, MM_SIGN_POS))));
    }

    REV_INLINE quat REV_VECTORCALL inverse() const
    {
        // @NOTE(Roman): Since our quaternion is always a unit quaternion we can not to devide by its norm and just return its conjugate.
        return quat(_mm_castsi128_ps(_mm_sign_epi32(_mm_castps_si128(mm_u), _mm_setr_epi32(MM_SIGN_NEG, MM_SIGN_NEG, MM_SIGN_NEG, MM_SIGN_POS))));
    }

    REV_INLINE f32 REV_VECTORCALL norm_sq() const
    {
        __m128 mm = mm_u;
        return _mm_cvtss_f32(_mm_dp_ps(mm, mm, 0xF1));
    }

    REV_INLINE f32 REV_VECTORCALL dot(quat q) const
    {
        return _mm_cvtss_f32(_mm_dp_ps(mm_u, q.mm_u, 0xF1));
    }

    REV_INLINE v4 REV_VECTORCALL rotate(v4 v) const
    {
        __m128  mm       = mm_u;
        __m128i neg_mask = _mm_set1_epi32(MM_SIGN_NEG);

        //
        // q*v
        //

        __m128 qv;
        {
            // @NOTE(Roman): As soon as v.w == 0, we can apply all our multiplications with v.w as 0;

            // qv.x = q.w * v.x + (q.y * v.z - q.z * v.y);
            // qv.y = q.w * v.y + (q.z * v.x - q.x * v.z);
            // qv.z = q.w * v.z + (q.x * v.y - q.y * v.x);
            // qv.w =           - (q.x * v.x + q.y * v.y + q.z * v.z);

            __m128 v_mm = mm_insert_f32<3>(v.mm, 0.0f);

            __m128 cross = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(  mm,   mm, MM_SHUFFLE_YZXW),
                                                 _mm_shuffle_ps(v_mm, v_mm, MM_SHUFFLE_ZXYW)),
                                      _mm_mul_ps(_mm_shuffle_ps(  mm,   mm, MM_SHUFFLE_ZXYW),
                                                 _mm_shuffle_ps(v_mm, v_mm, MM_SHUFFLE_YZXW)));

            __m128 neg_dot = _mm_castsi128_ps(_mm_sign_epi32(_mm_castps_si128(_mm_dp_ps(mm, v_mm, 0x78)), neg_mask));

            __m128 cp_dp  = _mm_blend_ps(cross, neg_dot, MM_BLEND_AAAB);
            __m128 q_wwww = _mm_shuffle_ps(mm, mm, MM_SHUFFLE_WWWW);

            qv = _mm_add_ps(_mm_mul_ps(q_wwww, v_mm), cp_dp);
        }

        //
        // qv*q^-1
        //

        __m128 res;
        {
            __m128 conj  = _mm_castsi128_ps(_mm_sign_epi32(_mm_castps_si128(mm), _mm_setr_epi32(MM_SIGN_NEG, MM_SIGN_NEG, MM_SIGN_NEG, MM_SIGN_POS)));
            __m128 inv_q = _mm_div_ps(conj, _mm_dp_ps(mm, mm, 0xFF));

            __m128 cross = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(   qv,    qv, MM_SHUFFLE_YZXW),
                                                 _mm_shuffle_ps(inv_q, inv_q, MM_SHUFFLE_ZXYW)),
                                      _mm_mul_ps(_mm_shuffle_ps(   qv,    qv, MM_SHUFFLE_ZXYW),
                                                 _mm_shuffle_ps(inv_q, inv_q, MM_SHUFFLE_YZXW)));

            __m128 neg_dot = _mm_castsi128_ps(_mm_sign_epi32(_mm_castps_si128(_mm_dp_ps(qv, inv_q, 0x78)), neg_mask));

            __m128 qv_wwww   = _mm_shuffle_ps(   qv,    qv, MM_SHUFFLE_WWWW);
            __m128 invq_wwww = _mm_shuffle_ps(inv_q, inv_q, MM_SHUFFLE_WWWW);
            __m128 qv_0      = mm_insert_f32<3>(qv, 0.0f);
            __m128 cp_dp     = _mm_blend_ps(cross, neg_dot, MM_BLEND_AAAB);

            __m128 qvw_invq = _mm_mul_ps(qv_wwww,   inv_q);
            __m128 invqw_qv = _mm_mul_ps(invq_wwww, qv_0);

            res = _mm_add_ps(_mm_add_ps(qvw_invq, invqw_qv), cp_dp);
        }

        f32 w = mm_extract_f32<3>(res);
        if (w && w != 1.0f) return v4(_mm_div_ps(res, _mm_shuffle_ps(res, res, MM_SHUFFLE_WWWW)));
        else                return v4(res);
    }

    REV_INLINE m3 REV_VECTORCALL to_m3()
    {
        // [ 1 - 2yy - 2zz,     2xy - 2wz,     2xz + 2wy ]
        // [     2xy + 2wz, 1 - 2xx - 2zz,     2yz - 2wx ]
        // [     2xz - 2wy,     2yz + 2wx, 1 - 2xx - 2yy ]

        __m128 mm = mm_u;

        __m128 _1000 = _mm_setr_ps(1.0f, 0.0f, 0.0f, 0.0f);
        __m128 _2220 = _mm_setr_ps(2.0f, 2.0f, 2.0f, 0.0f);

        __m128 _0100 = _mm_shuffle_ps(_1000, _1000, MM_SHUFFLE_YXYY);
        __m128 _0010 = _mm_shuffle_ps(_1000, _1000, MM_SHUFFLE_YYXY);

        __m128 zzxw = _mm_shuffle_ps(mm, mm, MM_SHUFFLE_ZZXW);

        __m128 col0_1 = _mm_mul_ps(_mm_mul_ps(_2220, _mm_shuffle_ps(mm, mm, MM_SHUFFLE_YXXW)), _mm_shuffle_ps(mm, mm, MM_SHUFFLE_YYZW));
        __m128 col0_2 = _mm_mul_ps(_mm_mul_ps(_2220, _mm_shuffle_ps(mm, mm, MM_SHUFFLE_ZWWW)), _mm_shuffle_ps(mm, mm, MM_SHUFFLE_ZZYW));
        __m128 col1_1 = _mm_mul_ps(_mm_mul_ps(_2220, _mm_shuffle_ps(mm, mm, MM_SHUFFLE_XXYW)), _mm_shuffle_ps(mm, mm, MM_SHUFFLE_YXZW));
        __m128 col1_2 = _mm_mul_ps(_mm_mul_ps(_2220, _mm_shuffle_ps(mm, mm, MM_SHUFFLE_WZWW)), zzxw);
        __m128 col2_1 = _mm_mul_ps(_mm_mul_ps(_2220, _mm_shuffle_ps(mm, mm, MM_SHUFFLE_XYXW)), zzxw);
        __m128 col2_2 = _mm_mul_ps(_mm_mul_ps(_2220, _mm_shuffle_ps(mm, mm, MM_SHUFFLE_WWYW)), _mm_shuffle_ps(mm, mm, MM_SHUFFLE_YXYW));

        __m128i neg_mask = _mm_setr_epi32(MM_SIGN_NEG, MM_SIGN_POS, MM_SIGN_POS, MM_SIGN_ZERO);

        __m128 neg_col0_1 = _mm_castsi128_ps(_mm_sign_epi32(_mm_castps_si128(col0_1), neg_mask));
        __m128 neg_col0_2 = _mm_castsi128_ps(_mm_sign_epi32(_mm_castps_si128(col0_2), _mm_shuffle_epi32(neg_mask, MM_SHUFFLE_XYXW)));
        __m128 neg_col1_1 = _mm_castsi128_ps(_mm_sign_epi32(_mm_castps_si128(col1_1), _mm_shuffle_epi32(neg_mask, MM_SHUFFLE_YXYW)));
        __m128 neg_col1_2 = _mm_castsi128_ps(_mm_sign_epi32(_mm_castps_si128(col1_2), _mm_shuffle_epi32(neg_mask, MM_SHUFFLE_XXYW)));
        __m128 neg_col2_1 = _mm_castsi128_ps(_mm_sign_epi32(_mm_castps_si128(col2_1), _mm_shuffle_epi32(neg_mask, MM_SHUFFLE_YYXW)));
        __m128 neg_col2_2 = _mm_castsi128_ps(_mm_sign_epi32(_mm_castps_si128(col2_2), _mm_shuffle_epi32(neg_mask, MM_SHUFFLE_YXXW)));

        __m128 col0 = _mm_add_ps(_mm_add_ps(_1000, neg_col0_1), neg_col0_2);
        __m128 col1 = _mm_add_ps(_mm_add_ps(_0100, neg_col1_1), neg_col1_2);
        __m128 col2 = _mm_add_ps(_mm_add_ps(_0010, neg_col2_1), neg_col2_2);

        int m4_1_0 = _mm_extract_ps(col0, 1);
        int m4_2_0 = _mm_extract_ps(col0, 2);
        int m4_1_1 = _mm_extract_ps(col1, 1);
        int m4_2_1 = _mm_extract_ps(col1, 2);
        int m4_1_2 = _mm_extract_ps(col2, 1);
        int m4_2_2 = _mm_extract_ps(col2, 2);

        return m3(  _mm_cvtss_f32(col0),   _mm_cvtss_f32(col1),   _mm_cvtss_f32(col2),
                  *cast<f32 *>(&m4_1_0), *cast<f32 *>(&m4_1_1), *cast<f32 *>(&m4_1_2),
                  *cast<f32 *>(&m4_2_0), *cast<f32 *>(&m4_2_1), *cast<f32 *>(&m4_2_2));
    }

    REV_INLINE m4 REV_VECTORCALL to_m4()
    {
        // [ 1 - 2yy - 2zz,     2xy - 2wz,     2xz + 2wy, 0 ]
        // [     2xy + 2wz, 1 - 2xx - 2zz,     2yz - 2wx, 0 ]
        // [     2xz - 2wy,     2yz + 2wx, 1 - 2xx - 2yy, 0 ]
        // [             0,             0,             0, 1 ]

        __m128 mm = mm_u;

        __m128 _1000 = _mm_setr_ps(1.0f, 0.0f, 0.0f, 0.0f);
        __m128 _2220 = _mm_setr_ps(2.0f, 2.0f, 2.0f, 0.0f);

        __m128 _0100 = _mm_shuffle_ps(_1000, _1000, MM_SHUFFLE_YXYY);
        __m128 _0010 = _mm_shuffle_ps(_1000, _1000, MM_SHUFFLE_YYXY);

        __m128 zzxw = _mm_shuffle_ps(mm, mm, MM_SHUFFLE_ZZXW);

        __m128 col0_1 = _mm_mul_ps(_mm_mul_ps(_2220, _mm_shuffle_ps(mm, mm, MM_SHUFFLE_YXXW)), _mm_shuffle_ps(mm, mm, MM_SHUFFLE_YYZW));
        __m128 col0_2 = _mm_mul_ps(_mm_mul_ps(_2220, _mm_shuffle_ps(mm, mm, MM_SHUFFLE_ZWWW)), _mm_shuffle_ps(mm, mm, MM_SHUFFLE_ZZYW));
        __m128 col1_1 = _mm_mul_ps(_mm_mul_ps(_2220, _mm_shuffle_ps(mm, mm, MM_SHUFFLE_XXYW)), _mm_shuffle_ps(mm, mm, MM_SHUFFLE_YXZW));
        __m128 col1_2 = _mm_mul_ps(_mm_mul_ps(_2220, _mm_shuffle_ps(mm, mm, MM_SHUFFLE_WZWW)), zzxw);
        __m128 col2_1 = _mm_mul_ps(_mm_mul_ps(_2220, _mm_shuffle_ps(mm, mm, MM_SHUFFLE_XYXW)), zzxw);
        __m128 col2_2 = _mm_mul_ps(_mm_mul_ps(_2220, _mm_shuffle_ps(mm, mm, MM_SHUFFLE_WWYW)), _mm_shuffle_ps(mm, mm, MM_SHUFFLE_YXYW));

        __m128i neg_mask = _mm_setr_epi32(MM_SIGN_NEG, MM_SIGN_POS, MM_SIGN_POS, MM_SIGN_ZERO);

        __m128 neg_col0_1 = _mm_castsi128_ps(_mm_sign_epi32(_mm_castps_si128(col0_1), neg_mask));
        __m128 neg_col0_2 = _mm_castsi128_ps(_mm_sign_epi32(_mm_castps_si128(col0_2), _mm_shuffle_epi32(neg_mask, MM_SHUFFLE_XYXW)));
        __m128 neg_col1_1 = _mm_castsi128_ps(_mm_sign_epi32(_mm_castps_si128(col1_1), _mm_shuffle_epi32(neg_mask, MM_SHUFFLE_YXYW)));
        __m128 neg_col1_2 = _mm_castsi128_ps(_mm_sign_epi32(_mm_castps_si128(col1_2), _mm_shuffle_epi32(neg_mask, MM_SHUFFLE_XXYW)));
        __m128 neg_col2_1 = _mm_castsi128_ps(_mm_sign_epi32(_mm_castps_si128(col2_1), _mm_shuffle_epi32(neg_mask, MM_SHUFFLE_YYXW)));
        __m128 neg_col2_2 = _mm_castsi128_ps(_mm_sign_epi32(_mm_castps_si128(col2_2), _mm_shuffle_epi32(neg_mask, MM_SHUFFLE_YXXW)));

        __m128 col0 = _mm_add_ps(_mm_add_ps(_1000, neg_col0_1), neg_col0_2);
        __m128 col1 = _mm_add_ps(_mm_add_ps(_0100, neg_col1_1), neg_col1_2);
        __m128 col2 = _mm_add_ps(_mm_add_ps(_0010, neg_col2_1), neg_col2_2);

        int m4_1_0 = _mm_extract_ps(col0, 1);
        int m4_2_0 = _mm_extract_ps(col0, 2);
        int m4_1_1 = _mm_extract_ps(col1, 1);
        int m4_2_1 = _mm_extract_ps(col1, 2);
        int m4_1_2 = _mm_extract_ps(col2, 1);
        int m4_2_2 = _mm_extract_ps(col2, 2);

        return m4(  _mm_cvtss_f32(col0),   _mm_cvtss_f32(col1),   _mm_cvtss_f32(col2), 0.0f,
                  *cast<f32 *>(&m4_1_0), *cast<f32 *>(&m4_1_1), *cast<f32 *>(&m4_1_2), 0.0f,
                  *cast<f32 *>(&m4_2_0), *cast<f32 *>(&m4_2_1), *cast<f32 *>(&m4_2_2), 0.0f,
                                   0.0f,                  0.0f,                  0.0f, 1.0f);
    }

    static REV_INLINE quat REV_VECTORCALL lerp(quat start, quat end, f32 t)
    {
        //          (end-start)t + start
        // q(t) = ------------------------;
        //        ||(end-start)t + start||

        __m128 s_mm = start.mm_u;
        __m128 res  = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(end.mm_u, s_mm), _mm_set_ps1(t)), s_mm);
        return quat(_mm_div_ps(res, _mm_sqrt_ps(_mm_dp_ps(res, res, 0xFF))));
    }

    static REV_INLINE quat REV_VECTORCALL spherical_lerp(quat start, quat end, f32 t)
    {
        //        sin(O(1-t))        sin(Ot)
        // q(t) = -----------start + -------end;
        //           sin(O)          sin(O)

        __m128 s_mm = start.mm_u;
        __m128 e_mm = end.mm_u;

        f32    O    = 1.0f / cosf(_mm_cvtss_f32(_mm_dp_ps(s_mm, e_mm, 0xF1)));
        __m128 sins = _mm_sin_ps(_mm_setr_ps(O, O*t, O*(1.0f-t), 1.0f));

        __m128 sinO   = _mm_shuffle_ps(sins, sins, MM_SHUFFLE_XXXX);
        __m128 sinOt  = _mm_shuffle_ps(sins, sins, MM_SHUFFLE_YYYY);
        __m128 sinO1t = _mm_shuffle_ps(sins, sins, MM_SHUFFLE_ZZZZ);

        __m128 s_length = _mm_div_ps(sinO1t, sinO);
        __m128 e_length = _mm_div_ps(sinOt,  sinO);

        __m128 s_part = _mm_mul_ps(s_length, s_mm);
        __m128 e_part = _mm_mul_ps(e_length, e_mm);

        return quat(_mm_add_ps(s_part, e_part));
    }

    static REV_INLINE f32 REV_VECTORCALL invlerp(quat start, quat end, quat value) // ret = [0, 1]
    {
        //     value - start
        // t = -------------;
        //      end - start

        __m128 s_mm = start.mm_u;
        return _mm_cvtss_f32(_mm_div_ps(_mm_sub_ps(value.mm_u, s_mm), _mm_sub_ps(end.mm_u, s_mm)));
    }

    static REV_INLINE f32 REV_VECTORCALL invlerp_n(quat start, quat end, quat value) // ret = [-1, 1]
    {
        //      value - start
        // t = (------------- - 0.5) * 2;
        //       end - start

        __m128 s_mm = start.mm_u;
        return _mm_cvtss_f32(_mm_mul_ps(_mm_sub_ps(_mm_div_ps(_mm_sub_ps(value.mm_u, s_mm), _mm_sub_ps(end.mm_u, s_mm)), _mm_set_ps1(0.5f)), _mm_set_ps1(2.0f)));
    }

    // static REV_INLINE quat REV_VECTORCALL clamp(quat val, quat min, quat max)
    // {
    //     return quat(_mm_max_ps(min.mm_u, _mm_min_ps(max.mm_u, val.mm_u)));
    // }

    REV_INLINE quat& REV_VECTORCALL operator*=(f32 r)
    {
        __m128 res = _mm_mul_ps(mm_u, _mm_set_ps1(r));
        mm_u = _mm_div_ps(res, _mm_sqrt_ps(_mm_dp_ps(res, res, 0xFF)));
        return *this;
    }

    REV_INLINE quat& REV_VECTORCALL operator/=(f32 r)
    {
        __m128 res = _mm_div_ps(mm_u, _mm_set_ps1(r));
        mm_u = _mm_div_ps(res, _mm_sqrt_ps(_mm_dp_ps(res, res, 0xFF)));
        return *this;
    }

    REV_INLINE quat& REV_VECTORCALL operator+=(quat r)
    {
        __m128 res = _mm_add_ps(mm_u, r.mm_u);
        mm_u = _mm_div_ps(res, _mm_sqrt_ps(_mm_dp_ps(res, res, 0xFF)));
        return *this;
    }

    REV_INLINE quat& REV_VECTORCALL operator-=(quat r)
    {
        __m128 res = _mm_sub_ps(mm_u, r.mm_u);
        mm_u = _mm_div_ps(res, _mm_sqrt_ps(_mm_dp_ps(res, res, 0xFF)));
        return *this;
    }

    REV_INLINE quat& REV_VECTORCALL operator*=(quat r)
    {
        __m128 l_mm = mm_u;
        __m128 r_mm = r.mm_u;

        __m128 cross = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(l_mm, l_mm, MM_SHUFFLE_YZXW),
                                             _mm_shuffle_ps(r_mm, r_mm, MM_SHUFFLE_ZXYW)),
                                  _mm_mul_ps(_mm_shuffle_ps(l_mm, l_mm, MM_SHUFFLE_ZXYW),
                                             _mm_shuffle_ps(r_mm, r_mm, MM_SHUFFLE_YZXW)));

        __m128 neg_dot = _mm_castsi128_ps(_mm_sign_epi32(_mm_castps_si128(_mm_dp_ps(l_mm, r_mm, 0x78)), _mm_set1_epi32(MM_SIGN_NEG)));

        __m128 l_wwww = _mm_shuffle_ps(l_mm, l_mm, MM_SHUFFLE_WWWW);
        __m128 r_wwww = _mm_shuffle_ps(r_mm, r_mm, MM_SHUFFLE_WWWW);
        __m128 l_mm_0 = mm_insert_f32<3>(l_mm, 0.0f);
        __m128 cp_dp  = _mm_blend_ps(cross, neg_dot, MM_BLEND_AAAB);

        __m128 lw_r = _mm_mul_ps(l_wwww, r_mm);
        __m128 rw_l = _mm_mul_ps(r_wwww, l_mm_0);

        __m128 res = _mm_add_ps(_mm_add_ps(lw_r, rw_l), cp_dp);
        mm_u = _mm_div_ps(res, _mm_sqrt_ps(_mm_dp_ps(res, res, 0xFF)));
        return *this;
    }

    // @NOTE(Roman): *= r^-1
    REV_INLINE quat& REV_VECTORCALL operator/=(quat r)
    {
        __m128 l_mm  = mm_u;
        __m128 inv_r = _mm_castsi128_ps(_mm_sign_epi32(_mm_castps_si128(r.mm_u), _mm_setr_epi32(MM_SIGN_NEG, MM_SIGN_NEG, MM_SIGN_NEG, MM_SIGN_POS)));

        __m128 cross = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps( l_mm,  l_mm, MM_SHUFFLE_YZXW),
                                             _mm_shuffle_ps(inv_r, inv_r, MM_SHUFFLE_ZXYW)),
                                  _mm_mul_ps(_mm_shuffle_ps( l_mm,  l_mm, MM_SHUFFLE_ZXYW),
                                             _mm_shuffle_ps(inv_r, inv_r, MM_SHUFFLE_YZXW)));

        __m128 neg_dot = _mm_castsi128_ps(_mm_sign_epi32(_mm_castps_si128(_mm_dp_ps(l_mm, inv_r, 0x78)), _mm_set1_epi32(MM_SIGN_NEG)));

        __m128 l_wwww = _mm_shuffle_ps( l_mm,  l_mm, MM_SHUFFLE_WWWW);
        __m128 r_wwww = _mm_shuffle_ps(inv_r, inv_r, MM_SHUFFLE_WWWW);
        __m128 l_mm_0 = mm_insert_f32<3>(l_mm, 0.0f);
        __m128 cp_dp  = _mm_blend_ps(cross, neg_dot, MM_BLEND_AAAB);

        __m128 lw_r = _mm_mul_ps(l_wwww, inv_r);
        __m128 rw_l = _mm_mul_ps(r_wwww, l_mm_0);

        __m128 res = _mm_add_ps(_mm_add_ps(lw_r, rw_l), cp_dp);
        mm_u = _mm_div_ps(res, _mm_sqrt_ps(_mm_dp_ps(res, res, 0xFF)));
        return *this;
    }
};

REV_INLINE quat REV_VECTORCALL operator*(f32 l, quat r)
{
    __m128 res = _mm_mul_ps(_mm_set_ps1(l), r.mm_u);
    return quat(_mm_div_ps(res, _mm_sqrt_ps(_mm_dp_ps(res, res, 0xFF))));
}

REV_INLINE quat REV_VECTORCALL operator/(quat l, f32 r)
{
    __m128 res =_mm_div_ps(l.mm_u, _mm_set_ps1(r));
    return quat(_mm_div_ps(res, _mm_sqrt_ps(_mm_dp_ps(res, res, 0xFF))));
}

REV_INLINE quat REV_VECTORCALL operator+(quat l, quat r)
{
    __m128 res = _mm_add_ps(l.mm_u, r.mm_u);
    return quat(_mm_div_ps(res, _mm_sqrt_ps(_mm_dp_ps(res, res, 0xFF))));
}

REV_INLINE quat REV_VECTORCALL operator-(quat l, quat r)
{
    __m128 res = _mm_sub_ps(l.mm_u, r.mm_u);
    return quat(_mm_div_ps(res, _mm_sqrt_ps(_mm_dp_ps(res, res, 0xFF))));
}

REV_INLINE quat REV_VECTORCALL operator*(quat l, quat r)
{
    // [(l.s*r.v + r.s*l.v + cross(l.v, r.v)), (l.s*r.s - dot(l.v, r.v))]

    // res.x = l.w * r.x + r.w * l.x + (l.y*r.z - l.z*r.y);
    // res.y = l.w * r.y + r.w * l.y + (l.z*r.x - l.x*r.z);
    // res.z = l.w * r.z + r.w * l.z + (l.x*r.y - l.y*r.x);
    // res.w = l.w * r.w             - (l.x*r.x + l.y*r.y + l.z*r.z);

    __m128 l_mm = l.mm_u;
    __m128 r_mm = r.mm_u;

    __m128 cross = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(l_mm, l_mm, MM_SHUFFLE_YZXW),
                                         _mm_shuffle_ps(r_mm, r_mm, MM_SHUFFLE_ZXYW)),
                              _mm_mul_ps(_mm_shuffle_ps(l_mm, l_mm, MM_SHUFFLE_ZXYW),
                                         _mm_shuffle_ps(r_mm, r_mm, MM_SHUFFLE_YZXW)));

    __m128 neg_dot = _mm_castsi128_ps(_mm_sign_epi32(_mm_castps_si128(_mm_dp_ps(l_mm, r_mm, 0x78)), _mm_set1_epi32(MM_SIGN_NEG)));

    __m128 l_wwww = _mm_shuffle_ps(l_mm, l_mm, MM_SHUFFLE_WWWW);
    __m128 r_wwww = _mm_shuffle_ps(r_mm, r_mm, MM_SHUFFLE_WWWW);
    __m128 l_mm_0 = mm_insert_f32<3>(l_mm, 0.0f);
    __m128 cp_dp  = _mm_blend_ps(cross, neg_dot, MM_BLEND_AAAB);

    __m128 lw_r = _mm_mul_ps(l_wwww, r_mm);
    __m128 rw_l = _mm_mul_ps(r_wwww, l_mm_0);

    __m128 res = _mm_add_ps(_mm_add_ps(lw_r, rw_l), cp_dp);
    return quat(_mm_div_ps(res, _mm_sqrt_ps(_mm_dp_ps(res, res, 0xFF))));
}

// @NOTE(Roman): l * r^-1
REV_INLINE quat REV_VECTORCALL operator/(quat l, quat r)
{
    __m128 l_mm  = l.mm_u;
    __m128 inv_r = _mm_castsi128_ps(_mm_sign_epi32(_mm_castps_si128(r.mm_u), _mm_setr_epi32(MM_SIGN_NEG, MM_SIGN_NEG, MM_SIGN_NEG, MM_SIGN_POS)));

    __m128 cross = _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps( l_mm,  l_mm, MM_SHUFFLE_YZXW),
                                         _mm_shuffle_ps(inv_r, inv_r, MM_SHUFFLE_ZXYW)),
                              _mm_mul_ps(_mm_shuffle_ps( l_mm,  l_mm, MM_SHUFFLE_ZXYW),
                                         _mm_shuffle_ps(inv_r, inv_r, MM_SHUFFLE_YZXW)));

    __m128 neg_dot = _mm_castsi128_ps(_mm_sign_epi32(_mm_castps_si128(_mm_dp_ps(l_mm, inv_r, 0x78)), _mm_set1_epi32(MM_SIGN_NEG)));

    __m128 l_wwww = _mm_shuffle_ps( l_mm,  l_mm, MM_SHUFFLE_WWWW);
    __m128 r_wwww = _mm_shuffle_ps(inv_r, inv_r, MM_SHUFFLE_WWWW);
    __m128 l_mm_0 = mm_insert_f32<3>(l_mm, 0.0f);
    __m128 cp_dp  = _mm_blend_ps(cross, neg_dot, MM_BLEND_AAAB);

    __m128 lw_r = _mm_mul_ps(l_wwww, inv_r);
    __m128 rw_l = _mm_mul_ps(r_wwww, l_mm_0);

    __m128 res = _mm_add_ps(_mm_add_ps(lw_r, rw_l), cp_dp);
    return quat(_mm_div_ps(res, _mm_sqrt_ps(_mm_dp_ps(res, res, 0xFF))));
}

};
#pragma pack(pop)
