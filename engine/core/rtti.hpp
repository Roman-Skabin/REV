//
// Copyright 2020 Roman Skabin
//

#pragma once

#include "core/platform.h"

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
    using bool_type  = base<bool, val>;
    using true_type  = bool_type<true>;
    using false_type = bool_type<false>;

    template<bool val> inline constexpr bool bool_type_v = bool_type<val>::value;

    template<typename> inline constexpr bool always_true  = true;
    template<typename> inline constexpr bool always_false = false;

    template<bool cond, typename T = void> struct enable_if          {                 };
    template<typename T>                   struct enable_if<true, T> { using type = T; };

    template<bool cond, typename T = void> using enable_if_t = typename enable_if<cond, T>::type;

    template<bool first_value, typename First, typename ...Rest> struct FirstTrue                              { using type = First;                                                };
    template<typename False, typename Next, typename ...Rest>    struct FirstTrue<false, False, Next, Rest...> { using type = typename FirstTrue<Next::value, Next, Rest...>::type; };

    template<typename ...Traits>               struct first_true                 : false_type                                {};
    template<typename First, typename ...Rest> struct first_true<First, Rest...> : FirstTrue<First::value, First, Rest...>::type {};

    template<typename ...Traits> inline constexpr bool first_true_v = first_true<Traits...>::value;

    template<typename, typename> inline constexpr bool is_same_v       = false;
    template<typename T>         inline constexpr bool is_same_v<T, T> = true;

    template<typename T, typename U> using is_same = bool_type<is_same_v<T, U>>;

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

    template<typename T> using is_enum = bool_type<__is_enum(T)>;

    template<typename T> inline constexpr bool is_bool_v = is_same_v<remove_cv_t<T>, bool>;

    template<typename T> using is_bool = bool_type<is_bool_v<T>>;

    template<typename T> inline constexpr bool is_char_v = is_same_v<remove_cv_t<T>, char>;

    template<typename T> using is_char = bool_type<is_char_v<T>>;

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
    
    template<typename T> using is_integral = bool_type<is_integral_v<T>>;

    template<typename T> inline constexpr bool is_floating_point_v = is_any_of_v<remove_cv_t<T>, float, double, long double>;

    template<typename T> using is_floating_point = bool_type<is_floating_point_v<T>>;

    template<typename T> inline constexpr bool is_arithmetic_v  = is_integral_v<T>
                                                               || is_floating_point_v<T>
                                                               && !is_bool_v<T>
                                                               && !is_char_v<T>;

    template<typename T> using is_arithmetic = bool_type<is_arithmetic_v<T>>;

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

    template<typename T> using is_xmm = bool_type<is_xmm_v<T>>;
    template<typename T> using is_ymm = bool_type<is_ymm_v<T>>;
    template<typename T> using is_zmm = bool_type<is_zmm_v<T>>;

    template<typename T> inline constexpr bool is_simd_v = is_xmm_v<T> || is_ymm_v<T> || is_zmm_v<T>;

    template<typename T> using is_simd = bool_type<is_simd_v<T>>;

    template<bool cond, typename True, typename False> struct conditional                     { using type = True;  };
    template<           typename True, typename False> struct conditional<false, True, False> { using type = False; };

    template<bool cond, typename True, typename False> using conditional_t = typename conditional<cond, True, False>::type;

    template<typename T, typename U> constexpr auto cmpeq(T a, U b) -> decltype(a == b) { return a == b; }
    template<typename T, typename U> constexpr auto cmpne(T a, U b) -> decltype(a != b) { return a != b; }
    template<typename T, typename U> constexpr auto cmplt(T a, U b) -> decltype(a <  b) { return a <  b; }
    template<typename T, typename U> constexpr auto cmpgt(T a, U b) -> decltype(a >  b) { return a >  b; }
    template<typename T, typename U> constexpr auto cmple(T a, U b) -> decltype(a <= b) { return a <= b; }
    template<typename T, typename U> constexpr auto cmpge(T a, U b) -> decltype(a >= b) { return a >= b; }

    template<typename T, typename U> using min_size_t = conditional_t<cmplt(sizeof(T), sizeof(U)), T, U>;
    template<typename T, typename U> using max_size_t = conditional_t<cmpgt(sizeof(T), sizeof(U)), T, U>;

    template<typename Base, typename Derived> inline constexpr bool is_base_of_v = __is_base_of(Base, Derived);

    template<typename Base, typename Derived> using is_base_of = bool_type<__is_base_of(Base, Derived)>;

    template<typename From, typename To> inline constexpr bool is_convertible_to_v = __is_convertible_to(From, To);

    template<typename From, typename To> using is_convertible_to = bool_type<__is_convertible_to(From, To)>;

    template<typename T> struct remove_ref      { using type = T; };
    template<typename T> struct remove_ref<T&>  { using type = T; };
    template<typename T> struct remove_ref<T&&> { using type = T; };

    template<typename T> using remove_ref_t = typename remove_ref<T>::type;

    template<typename>   inline constexpr bool is_lvalue_v     = false;
    template<typename T> inline constexpr bool is_lvalue_v<T&> = true;

    template<typename T> using is_lvalue = bool_type<is_lvalue_v<T>>;

    template<typename>   inline constexpr bool is_rvalue_v      = false;
    template<typename T> inline constexpr bool is_rvalue_v<T&&> = true;

    template<typename T> using is_rvalue = bool_type<is_rvalue_v<T>>;

    template<typename>   inline constexpr bool is_ref_v      = false;
    template<typename T> inline constexpr bool is_ref_v<T&>  = true;
    template<typename T> inline constexpr bool is_ref_v<T&&> = true;

    template<typename T> using is_ref = bool_type<is_ref_v<T>>;

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

    template<typename T> using is_array = bool_type<is_array_v<T>>;

    template<typename T> inline constexpr bool is_nullptr_t_v = is_same_v<remove_cv_t<T>, nullptr_t>;

    template<typename>   inline constexpr bool is_pointer_v                    = false;
    template<typename T> inline constexpr bool is_pointer_v<T *>               = true;
    template<typename T> inline constexpr bool is_pointer_v<T *const>          = true;
    template<typename T> inline constexpr bool is_pointer_v<T *volatile>       = true;
    template<typename T> inline constexpr bool is_pointer_v<T *const volatile> = true;

    template<typename T> using is_pointer = bool_type<is_pointer_v<T>>;

    template<typename>                 inline constexpr bool is_cstring_v                             = false;
    template<>                         inline constexpr bool is_cstring_v<const char *>               = true;
    template<>                         inline constexpr bool is_cstring_v<const char *const>          = true;
    template<>                         inline constexpr bool is_cstring_v<const char *volatile>       = true;
    template<>                         inline constexpr bool is_cstring_v<const char *const volatile> = true;
    template<unsigned long long count> inline constexpr bool is_cstring_v<const char[count]>          = true;
    template<>                         inline constexpr bool is_cstring_v<char *>                     = true;
    template<>                         inline constexpr bool is_cstring_v<char *const>                = true;
    template<>                         inline constexpr bool is_cstring_v<char *volatile>             = true;
    template<>                         inline constexpr bool is_cstring_v<char *const volatile>       = true;
    template<unsigned long long count> inline constexpr bool is_cstring_v<char[count]>                = true;

    template<typename T> using is_cstring = bool_type<is_cstring_v<T>>;

    template<typename ...Conditions>          struct all_true                : true_type                                                 {};
    template<typename Cond, typename ...Rest> struct all_true<Cond, Rest...> : conditional_t<Cond::value, all_true<Rest...>, false_type> {};

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

    template<typename T, typename ...ConstructorArgs> using is_constructible = bool_type<__is_constructible(T, ConstructorArgs...)>;

    template<typename T> inline constexpr bool is_default_constructible_v = __is_constructible(T);

    template<typename T> using is_default_constructible = bool_type<__is_constructible(T)>;

    template<typename T> inline constexpr bool is_copy_constructible_v = __is_constructible(T, const T&);

    template<typename T> using is_copy_constructible = bool_type<__is_constructible(T, const T&)>;

    template<typename T> inline constexpr bool is_move_constructible_v = __is_constructible(T, T);

    template<typename T> using is_move_constructible = bool_type<__is_constructible(T, T)>;

    template<typename To, typename From> inline constexpr bool is_assignable_v = __is_assignable(To, From);

    template<typename To, typename From> using is_assignable = bool_type<__is_assignable(To, From)>;

    template<typename T> inline constexpr bool is_copy_assignable_v = __is_assignable(T&, const T&);

    template<typename T> using is_copy_assignable = bool_type<__is_assignable(T&, const T&)>;

    template<typename T> inline constexpr bool is_move_assignable_v = __is_assignable(T&, T);

    template<typename T> using is_move_assignable = bool_type<__is_assignable(T&, T)>;

    template<typename T> inline constexpr bool is_destructible_v = __is_destructible(T);

    template<typename T> using is_destructible = bool_type<__is_destructible(T)>;

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

    template<typename ...T> inline constexpr unsigned long long sequence_count_v = sizeof...(T);

    template<typename ...T> using sequence_count = base<unsigned long long, sizeof...(T)>;

    template<unsigned long long index, typename T, typename ...U> struct get_sequence_type             { using type = typename get_sequence_type<index - 1, U...>::type; };
    template<                          typename T, typename ...U> struct get_sequence_type<0, T, U...> { using type = T; };

    template<unsigned long long index, typename ...T> using get_sequence_type_t = typename get_sequence_type<index, T...>::type;

    template<typename T> struct add_ref      { using type = T&;  };
    template<typename T> struct add_ref<T&>  { using type = T&&; };
    template<typename T> struct add_ref<T&&> { using type = T&&; };

    template<typename T> using add_ref_t = typename add_ref<T>::type;

    template<typename T> using void_t = void;

    template<typename T> constexpr remove_ref_t<T>&& declval() noexcept;

    template<typename Void, typename Callable, typename ...Args>
    struct _callable
    {
        static constexpr bool is_callable = false;
    };

    template<typename Callable, typename ...Args>
    struct _callable<void_t<decltype(declval<Callable>()(declval<Args>()...))>, Callable, Args...>
    {
        static constexpr bool is_callable = true;

        using ret_type  = decltype(declval<Callable>()(declval<Args>()...));
        using func_type = ret_type(Args...);
    };

    template<typename Callable, typename ...Args>
    using callable = _callable<void, Callable, Args...>;

    template<typename Callable, typename ...Args>
    inline constexpr bool is_callable_v = callable<Callable, Args...>::is_callable;

    template<typename T, typename U>
    struct comparable
    {
        using eq_t = decltype(declval<T>() == declval<U>());
        using ne_t = decltype(declval<T>() == declval<U>());
        using lt_t = decltype(declval<T>() <  declval<U>());
        using gt_t = decltype(declval<T>() >  declval<U>());
        using le_t = decltype(declval<T>() <= declval<U>());
        using ge_t = decltype(declval<T>() >= declval<U>());
    };

    template<typename T, typename U> using comparable_eq_t = typename comparable<T, U>::eq_t;
    template<typename T, typename U> using comparable_ne_t = typename comparable<T, U>::ne_t;
    template<typename T, typename U> using comparable_lt_t = typename comparable<T, U>::lt_t;
    template<typename T, typename U> using comparable_gt_t = typename comparable<T, U>::gt_t;
    template<typename T, typename U> using comparable_le_t = typename comparable<T, U>::le_t;
    template<typename T, typename U> using comparable_ge_t = typename comparable<T, U>::ge_t;

    template<typename T, typename U, typename Void> struct _is_comparable_eq                                      : false_type {};
    template<typename T, typename U               > struct _is_comparable_eq<T, U, void_t<comparable_eq_t<T, U>>> : true_type  {};
    template<typename T, typename U, typename Void> struct _is_comparable_ne                                      : false_type {};
    template<typename T, typename U               > struct _is_comparable_ne<T, U, void_t<comparable_ne_t<T, U>>> : true_type  {};
    template<typename T, typename U, typename Void> struct _is_comparable_lt                                      : false_type {};
    template<typename T, typename U               > struct _is_comparable_lt<T, U, void_t<comparable_lt_t<T, U>>> : true_type  {};
    template<typename T, typename U, typename Void> struct _is_comparable_gt                                      : false_type {};
    template<typename T, typename U               > struct _is_comparable_gt<T, U, void_t<comparable_gt_t<T, U>>> : true_type  {};
    template<typename T, typename U, typename Void> struct _is_comparable_le                                      : false_type {};
    template<typename T, typename U               > struct _is_comparable_le<T, U, void_t<comparable_le_t<T, U>>> : true_type  {};
    template<typename T, typename U, typename Void> struct _is_comparable_ge                                      : false_type {};
    template<typename T, typename U               > struct _is_comparable_ge<T, U, void_t<comparable_ge_t<T, U>>> : true_type  {};

    template<typename T, typename U> using is_comparable_eq = _is_comparable_eq<T, U, void>;
    template<typename T, typename U> using is_comparable_ne = _is_comparable_ne<T, U, void>;
    template<typename T, typename U> using is_comparable_lt = _is_comparable_lt<T, U, void>;
    template<typename T, typename U> using is_comparable_gt = _is_comparable_gt<T, U, void>;
    template<typename T, typename U> using is_comparable_le = _is_comparable_le<T, U, void>;
    template<typename T, typename U> using is_comparable_ge = _is_comparable_ge<T, U, void>;

    template<typename T, typename U> inline constexpr bool is_comparable_eq_v = is_comparable_eq<T, U>::value;
    template<typename T, typename U> inline constexpr bool is_comparable_ne_v = is_comparable_ne<T, U>::value;
    template<typename T, typename U> inline constexpr bool is_comparable_lt_v = is_comparable_lt<T, U>::value;
    template<typename T, typename U> inline constexpr bool is_comparable_gt_v = is_comparable_gt<T, U>::value;
    template<typename T, typename U> inline constexpr bool is_comparable_le_v = is_comparable_le<T, U>::value;
    template<typename T, typename U> inline constexpr bool is_comparable_ge_v = is_comparable_ge<T, U>::value;

    template<typename T, typename = enable_if_t<is_comparable_gt_v<T, T>>> constexpr T max(T left, T right) { return left > right ? left : right; }
    template<typename T, typename = enable_if_t<is_comparable_lt_v<T, T>>> constexpr T min(T left, T right) { return left < right ? left : right; }

    #pragma warning(suppress: 4146) // unsigned T warning
    template<typename T, typename = enable_if_t<is_comparable_lt_v<T, int>>> constexpr T abs(T val) { return val < 0 ? -val : val; }

    template<typename Void, typename T> struct _has_to_string                                               : false_type {};
    template<               typename T> struct _has_to_string<void_t<decltype(declval<T>().ToString())>, T> : true_type  {};

    template<typename T> using has_to_string = _has_to_string<void, T>;

    template<typename T> inline constexpr bool has_to_string_v = has_to_string<T>::value;

    template<typename T> inline constexpr bool is_scalar_v  = is_integral_v<T>
                                                           || is_floating_point_v<T>
                                                           || is_pointer_v<T>;

    template<typename T> using is_scalar = bool_type<is_scalar_v<T>>;
}
