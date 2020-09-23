//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "platform.h"

namespace RTTI
{
    template<typename T, T val>
    struct base
    {
        using type       = base;
        using value_type = T;

        static constexpr T value = val;
    };

    template<bool val>
    using bool_constant  = base<bool, val>;
    using true_constant  = bool_constant<true>;
    using false_constant = bool_constant<false>;

    template<bool cond, typename T = void> struct enable_if          {                 };
    template<typename T>                   struct enable_if<true, T> { using type = T; };

    template<bool cond, typename T = void> using enable_if_t = typename enable_if<cond, T>::type;

    template<bool first_value, typename First, typename ...Rest> struct FirstTrue                              { using type = First;                                                };
    template<typename False, typename Next, typename ...Rest>    struct FirstTrue<false, False, Next, Rest...> { using type = typename FirstTrue<Next::value, Next, Rest...>::type; };

    template<typename ...Traits>               struct first_true                 : false_constant                                {};
    template<typename First, typename ...Rest> struct first_true<First, Rest...> : FirstTrue<First::value, First, Rest...>::type {};

    template<typename ...Traits> inline constexpr bool first_true_v = first_true<Traits...>::value;

    template<typename, typename> inline constexpr bool is_same_v       = false;
    template<typename T>         inline constexpr bool is_same_v<T, T> = true;

    template<typename T, typename U> using is_same = bool_constant<is_same_v<T, U>>;

    template<typename T, typename ...Types> inline constexpr bool is_any_of_v = first_true_v<is_same<T, Types>...>;

    template<typename T> struct remove_const          { using type = T; };
    template<typename T> struct remove_const<const T> { using type = T; };

    template<typename T> using remove_const_t = typename remove_const<T>::type;

    template<typename T> struct remove_volatile             { using type = T; };
    template<typename T> struct remove_volatile<volatile T> { using type = T; };

    template<typename T> using remove_volatile_t = typename remove_volatile<T>::type;

    template<typename T> struct remove_cv                   { using type = T; };
    template<typename T> struct remove_cv<const T>          { using type = T; };
    template<typename T> struct remove_cv<volatile T>       { using type = T; };
    template<typename T> struct remove_cv<const volatile T> { using type = T; };

    template<typename T> using remove_cv_t = typename remove_cv<T>::type;

    template<typename T> inline constexpr bool is_enum_v = __is_enum(T);

    template<typename T> using is_enum = bool_constant<__is_enum(T)>;

    template<typename T> inline constexpr bool is_bool_v = is_same_v<remove_cv_t<T>, bool>;

    template<typename T> using is_bool = bool_constant<is_bool_v<T>>;

    template<typename T>
    inline constexpr bool is_integral_v = is_any_of_v<remove_cv_t<T>, bool,
                                                                      char, unsigned char, signed char,
                                                                      wchar_t,
                                                                      #ifdef __cpp_char8_t
                                                                      char8_t,
                                                                      #endif
                                                                      char16_t, char32_t,
                                                                      short, unsigned short,
                                                                      int, unsigned int,
                                                                      long, unsigned long,
                                                                      long long, unsigned long long>
                                       || is_enum_v<T>;
    
    template<typename T> using is_integral = bool_constant<is_integral_v<T>>;

    template<typename T> inline constexpr bool is_floating_point_v = is_any_of_v<remove_cv_t<T>, float, double, long double>;

    template<typename T> using is_floating_point = bool_constant<is_floating_point_v<T>>;

    template<typename T> inline constexpr bool is_arithmetic_v = is_integral_v<T> || is_floating_point_v<T> && !is_bool_v<T>;

    template<typename T> using is_arithmetic = bool_constant<is_arithmetic_v<T>>;

    template<typename T> inline constexpr bool is_xmm_v = is_any_of_v<remove_cv_t<T>, __m128, __m128i, __m128d, xmm>;

#if ENGINE_ISA >= ENGINE_ISA_AVX
    template<typename T> inline constexpr bool is_ymm_v = is_any_of_v<remove_cv_t<T>, __m256, __m256i, __m256d, ymm>;
#else
    template<typename T> inline constexpr bool is_ymm_v = false;
#endif

#if ENGINE_ISA >= ENGINE_ISA_AVX512
    template<typename T> inline constexpr bool is_zmm_v = is_any_of_v<remove_cv_t<T>, __m512, __m512i, __m512d, zmm>;
#else
    template<typename T> inline constexpr bool is_zmm_v = false;
#endif

    template<typename T> using is_xmm = bool_constant<is_xmm_v<T>>;
    template<typename T> using is_ymm = bool_constant<is_ymm_v<T>>;
    template<typename T> using is_zmm = bool_constant<is_zmm_v<T>>;

    template<typename T> inline constexpr bool is_simd_v = is_xmm_v<T> || is_ymm_v<T> || is_zmm_v<T>;

    template<typename T> using is_simd = bool_constant<is_simd_v<T>>;

    template<bool cond, typename True, typename False> struct conditional                     { using type = True;  };
    template<           typename True, typename False> struct conditional<false, True, False> { using type = False; };

    template<bool cond, typename True, typename False> using conditional_t = typename conditional<cond, True, False>::type;

    template<typename T> constexpr bool greater(T a, T b) { return a > b; }
    template<typename T> constexpr bool less(T a, T b)    { return a < b; }

    template<typename T, typename U> using max_size_t = conditional_t<greater(sizeof(T), sizeof(U)), T, U>;
    template<typename T, typename U> using min_size_t = conditional_t<less(sizeof(T), sizeof(U)), T, U>;

    template<typename Base, typename Derived> inline constexpr bool is_base_of_v = __is_base_of(Base, Derived);

    template<typename Base, typename Derived> using is_base_of = bool_constant<__is_base_of(Base, Derived)>;

    template<typename From, typename To> inline constexpr bool is_convertible_to_v = __is_convertible_to(From, To);

    template<typename From, typename To> using is_convertible_to = bool_constant<__is_convertible_to(From, To)>;

    template<typename T> struct remove_ref      { using type = T; };
    template<typename T> struct remove_ref<T&>  { using type = T; };
    template<typename T> struct remove_ref<T&&> { using type = T; };

    template<typename T> using remove_ref_t = typename remove_ref<T>::type;

    template<typename>   inline constexpr bool is_lvalue_v     = false;
    template<typename T> inline constexpr bool is_lvalue_v<T&> = true;

    template<typename T> using is_lvalue = bool_constant<is_lvalue_v<T>>;

    template<typename>   inline constexpr bool is_rvalue_v      = false;
    template<typename T> inline constexpr bool is_rvalue_v<T&&> = true;

    template<typename T> using is_rvalue = bool_constant<is_rvalue_v<T>>;

    template<typename>   inline constexpr bool is_ref_v      = false;
    template<typename T> inline constexpr bool is_ref_v<T&>  = true;
    template<typename T> inline constexpr bool is_ref_v<T&&> = true;

    template<typename T> using is_ref = bool_constant<is_ref_v<T>>;

    template<typename T> constexpr remove_ref_t<T>&& move(T&& x)    { return static_cast<remove_ref_t<T>&&>(x); }
    template<typename T> constexpr T&& forward(remove_ref_t<T>& x)  { return static_cast<T&&>(x); }
    template<typename T> constexpr T&& forward(remove_ref_t<T>&& x) { static_assert(!is_lvalue_v<T>, "T can't be lvalue"); return static_cast<T&&>(x); }

    template<typename T, typename U = T>
    constexpr T exchange(T& val, U&& new_val)
    {
        T old_val = static_cast<T&&>(val);
        val       = static_cast<U&&>(new_val);
        return old_val;
    }

    template<typename>                             inline constexpr bool is_array_v           = false;
    template<typename T>                           inline constexpr bool is_array_v<T[]>      = true;
    template<typename T, unsigned long long count> inline constexpr bool is_array_v<T[count]> = true;

    template<typename T> using is_array = bool_constant<is_array_v<T>>;

#if 0
    template<typename T> constexpr remove_ref_t<T>&  to_lvalue(remove_ref_t<T>& x)  { return x; }
    template<typename T> constexpr remove_ref_t<T>&  to_lvalue(remove_ref_t<T>&& x) { return static_cast<T&>(x); }

    template<typename T> constexpr remove_ref_t<T>&& to_rvalue(remove_ref_t<T>&  x) { return static_cast<T&&>(x); }
    template<typename T> constexpr remove_ref_t<T>&& to_rvalue(remove_ref_t<T>&& x) { return x; }
#endif
}
