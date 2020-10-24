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

    template<bool val> inline constexpr bool bool_constant_v = bool_constant<val>::value;

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
/*
    template<bool cond, auto true_val, auto false_val>
    struct _conditional_v
    {
        using type = decltype(True);
        static constexpr type val = true_val;
    };

    template<auto true_val, auto false_val>
    struct _conditional_v<false, true_val, false_val>
    {
        using type = decltype(false_val);
        static constexpr type val = false_val;
    };

    template<bool cond, auto true_val, auto false_val>
    inline constexpr bool conditional_v = typename conditional<cond, true_val, false_val>::val;
*/
    template<typename T, typename U> constexpr auto cmpe(T  a, U b) -> decltype(a == b) { return a == b; }
    template<typename T, typename U> constexpr auto cmpne(T a, U b) -> decltype(a != b) { return a != b; }
    template<typename T, typename U> constexpr auto cmpl(T  a, U b) -> decltype(a <  b) { return a <  b; }
    template<typename T, typename U> constexpr auto cmpg(T  a, U b) -> decltype(a >  b) { return a >  b; }
    template<typename T, typename U> constexpr auto cmple(T a, U b) -> decltype(a <= b) { return a <= b; }
    template<typename T, typename U> constexpr auto cmpge(T a, U b) -> decltype(a >= b) { return a >= b; }

    template<typename T, typename U> using min_size_t = conditional_t<cmpl(sizeof(T), sizeof(U)), T, U>;
    template<typename T, typename U> using max_size_t = conditional_t<cmpg(sizeof(T), sizeof(U)), T, U>;

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

    template<typename>   inline constexpr bool is_pointer_v      = false;
    template<typename T> inline constexpr bool is_pointer_v<T *> = true;

    template<typename T> using is_pointer = bool_constant<is_pointer_v<T>>;

    template<typename ...Conditions>          struct all_true                : true_constant                                                 {};
    template<typename Cond, typename ...Rest> struct all_true<Cond, Rest...> : conditional_t<Cond::value, all_true<Rest...>, false_constant> {};

    template<typename ...Conditions> inline constexpr bool all_true_v = all_true<Conditions...>::value;

    template<typename T, typename ...Types> using are_same = all_true<is_same<T, Types>...>;

    template<typename T, typename ...Types> inline constexpr bool are_same_v = are_same<T, Types...>::value;

    template<typename Base, typename ...Deriveds> using are_base_of = all_true<is_base_of<Base, Deriveds>...>;

    template<typename Base, typename ...Deriveds> inline constexpr bool are_base_of_v = are_base_of<Base, Deriveds...>::value;

    template<typename T> struct remove_pointer                    { using type = T; };
    template<typename T> struct remove_pointer<T *>               { using type = T; };
    template<typename T> struct remove_pointer<T *const>          { using type = T; };
    template<typename T> struct remove_pointer<T *volatile>       { using type = T; };
    template<typename T> struct remove_pointer<T *const volatile> { using type = T; };

    template<typename T> using remove_pointer_t = typename remove_pointer<T>::type;

    template<typename T, bool = is_enum_v<T>> struct underlying_type           { using type = __underlying_type(T); };
    template<typename T>                      struct underlying_type<T, false> {};

    template<typename T> using underlying_type_t = typename underlying_type<T>::type;

    template<typename T, typename ...ConstructorArgs> inline constexpr bool is_constructible_v = __is_constructible(T, ConstructorArgs...);

    template<typename T, typename ...ConstructorArgs> using is_constructible = bool_constant<__is_constructible(T, ConstructorArgs...)>;

    template<typename T> inline constexpr bool is_default_constructible_v = __is_constructible(T);

    template<typename T> using is_default_constructible = bool_constant<__is_constructible(T)>;

    template<typename T> inline constexpr bool is_copy_constructible_v = __is_constructible(T, const T&);

    template<typename T> using is_copy_constructible = bool_constant<__is_constructible(T, const T&)>;

    template<typename T> inline constexpr bool is_move_constructible_v = __is_constructible(T, T);

    template<typename T> using is_move_constructible = bool_constant<__is_constructible(T, T)>;

    template<typename To, typename From> inline constexpr bool is_assignable_v = __is_assignable(To, From);

    template<typename To, typename From> using is_assignable = bool_constant<__is_assignable(To, From)>;

    template<typename T> inline constexpr bool is_copy_assignable_v = __is_assignable(T&, const T&);

    template<typename T> using is_copy_assignable = bool_constant<__is_assignable(T&, const T&)>;

    template<typename T> inline constexpr bool is_move_assignable_v = __is_assignable(T&, T);

    template<typename T> using is_move_assignable = bool_constant<__is_assignable(T&, T)>;

    template<typename T> inline constexpr bool is_destructible_v = __is_destructible(T);

    template<typename T> using is_destructible = bool_constant<__is_destructible(T)>;

    template<typename T, typename U> constexpr auto add(T a, U b) -> decltype(a + b) { return a + b; }
    template<typename T, typename U> constexpr auto sub(T a, U b) -> decltype(a - b) { return a - b; }
    template<typename T, typename U> constexpr auto nul(T a, U b) -> decltype(a * b) { return a * b; }
    template<typename T, typename U> constexpr auto div(T a, U b) -> decltype(a / b) { return a / b; }
    template<typename T, typename U> constexpr auto mod(T a, U b) -> decltype(a % b) { return a % b; }
    template<typename T, typename U> constexpr auto and(T a, U b) -> decltype(a & b) { return a & b; }
    template<typename T, typename U> constexpr auto  or(T a, U b) -> decltype(a | b) { return a | b; }
    template<typename T, typename U> constexpr auto xor(T a, U b) -> decltype(a ^ b) { return a ^ b; }

    template<typename T> constexpr auto pre_inc(T& a)  -> decltype(++a) { return ++a; }
    template<typename T> constexpr auto post_inc(T& a) -> decltype(a++) { return a++; }

    template<typename T> constexpr auto pre_dec(T& a)  -> decltype(--a) { return --a; }
    template<typename T> constexpr auto post_dec(T& a) -> decltype(a--) { return a--; }

    template<typename ...T> using are_default_constructible = all_true<is_default_constructible<T>...>;

    template<typename ...T> inline constexpr bool are_default_constructible_v = are_default_constructible<T...>::value;

    template<typename ...T> using are_copy_constructible = all_true<is_copy_constructible<T>...>;

    template<typename ...T> inline constexpr bool are_copy_constructible_v = are_copy_constructible<T...>::value;

    template<typename ...T> using are_move_constructible = all_true<is_move_constructible<T>...>;

    template<typename ...T> inline constexpr bool are_move_constructible_v = are_move_constructible<T...>::value;

    template<typename ...T> using are_copy_assignable = all_true<is_copy_assignable<T>...>;

    template<typename ...T> inline constexpr bool are_copy_assignable_v = are_copy_assignable<T...>::value;

    template<typename ...T> using are_move_assignable = all_true<is_move_assignable<T>...>;

    template<typename ...T> inline constexpr bool are_move_assignable_v = are_move_assignable<T...>::value;

    template<typename ...T> using are_destructible = all_true<is_destructible<T>...>;

    template<typename ...T> inline constexpr bool are_destructible_v = are_destructible<T...>::value;

#if 0
    template<typename T> constexpr remove_ref_t<T>&  to_lvalue(remove_ref_t<T>& x)  { return x; }
    template<typename T> constexpr remove_ref_t<T>&  to_lvalue(remove_ref_t<T>&& x) { return static_cast<T&>(x); }

    template<typename T> constexpr remove_ref_t<T>&& to_rvalue(remove_ref_t<T>&  x) { return static_cast<T&&>(x); }
    template<typename T> constexpr remove_ref_t<T>&& to_rvalue(remove_ref_t<T>&& x) { return x; }
#endif
}
