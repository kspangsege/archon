// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef ARCHON_X_CORE_X_IMPL_X_UTILITY_HPP
#define ARCHON_X_CORE_X_IMPL_X_UTILITY_HPP

/// \file


#include <cstddef>
#include <type_traits>
#include <utility>
#include <array>
#include <tuple>

#include <archon/core/features.h>


namespace archon::core::impl {


template<class T, bool E> class HiddenBase;

// Case: Non-empty
template<class T> class HiddenBase<T, false> {
public:
    template<class... A> constexpr HiddenBase(A&&... args) noexcept(noexcept(T(std::forward<A>(args)...)));

    constexpr auto hidden_base() noexcept       -> T&;
    constexpr auto hidden_base() const noexcept -> const T&;

private:
    T m_hidden_base;
};

// Case: Empty
template<class T> class HiddenBase<T, true> {
public:
    template<class... A> constexpr HiddenBase(A&&... args) noexcept(noexcept(T(std::forward<A>(args)...)));

    constexpr auto hidden_base() noexcept       -> T&;
    constexpr auto hidden_base() const noexcept -> const T&;

private:
    static constexpr T m_hidden_base = {};
};



template<class F, std::size_t... I> constexpr auto make_dispatch_array(std::index_sequence<I...>);



template<std::size_t, class, class...> struct GetArgTypeAt;

template<std::size_t I, class A, class... B> struct GetArgTypeAt {
    using type = typename GetArgTypeAt<I - 1, B...>::type;
};

template<class A, class... B> struct GetArgTypeAt<0, A, B...> {
    using type = A;
};



template<std::size_t I, class A, class... B>
constexpr auto get_arg_at(A&& arg, B&&... args) noexcept -> auto&&;



template<template<class> class P> constexpr bool has_arg() noexcept
{
    return false;
}

template<template<class> class P, class A, class... B> constexpr bool has_arg() noexcept
{
    if constexpr (P<A>::value) {
        return true;
    }
    else {
        return has_arg<P, B...>();
    }
}


template<template<class> class P> constexpr bool has_unique_arg() noexcept
{
    return false;
}

template<template<class> class P, class A, class... B> constexpr bool has_unique_arg() noexcept
{
    if constexpr (P<A>::value) {
        return !impl::has_arg<P, B...>();
    }
    else {
        return has_unique_arg<P, B...>();
    }
}



template<template<class> class P, class A, class... B> constexpr auto find_first_arg() noexcept -> std::size_t;
template<template<class> class P, class A, class... B> constexpr auto find_unique_arg() noexcept -> std::size_t;



template<class T, class F, std::size_t I> void for_each_tuple_elem(T& tuple, F& func);
template<class T, class F, std::size_t I> bool for_each_tuple_elem_a(T& tuple, F& func);



template<class T, class F> struct WithTupleElem {
    template<std::size_t I> static auto exec(T& tuple, F& func)
    {
        func(std::get<I>(std::forward<T>(tuple))); // Throws
    }
};



template<class T, T I, T N, class F> constexpr bool for_each_int_is_nothrow() noexcept;
template<class T, T I, T N, class F> void for_each_int(F& func) noexcept(impl::for_each_int_is_nothrow<T, I, N, F>());








// Implementation


template<class T>
template<class... A> constexpr HiddenBase<T, false>::HiddenBase(A&&... args)
    noexcept(noexcept(T(std::forward<A>(args)...)))
    : m_hidden_base(std::forward<A>(args)...) // Throws
{
}


template<class T>
constexpr auto HiddenBase<T, false>::hidden_base() noexcept -> T&
{
    return m_hidden_base;
}


template<class T>
constexpr auto HiddenBase<T, false>::hidden_base() const noexcept -> const T&
{
    return m_hidden_base;
}


template<class T>
template<class... A> constexpr HiddenBase<T, true>::HiddenBase(A&&... args)
    noexcept(noexcept(T(std::forward<A>(args)...)))
{
    T hidden_base(std::forward<A>(args)...); // Throws
    static_cast<void>(hidden_base);
}


template<class T>
constexpr auto HiddenBase<T, true>::hidden_base() noexcept -> T&
{
    return m_hidden_base;
}


template<class T>
constexpr auto HiddenBase<T, true>::hidden_base() const noexcept -> const T&
{
    return m_hidden_base;
}


template<class F, std::size_t... I> constexpr auto make_dispatch_array(std::index_sequence<I...>)
{
    using func_type = decltype(&F::template exec<0>);
    return std::array<func_type, sizeof... (I)>({ &F::template exec<I>... });
}


template<std::size_t I, class A, class... B> constexpr auto get_arg_at(A&& arg, B&&... args) noexcept -> auto&&
{
    if constexpr (I == 0) {
        return std::forward<A>(arg);
    }
    else {
        return get_arg_at<I - 1>(std::forward<B>(args)...);
    }
}


template<template<class> class P, class A, class... B> constexpr auto find_first_arg() noexcept -> std::size_t
{
    if constexpr (P<A>::value) {
        return 0;
    }
    else {
        return 1 + find_first_arg<P, B...>();
    }
}


template<template<class> class P, class A, class... B> constexpr auto find_unique_arg() noexcept -> std::size_t
{
    if constexpr (P<A>::value) {
        static_assert(!impl::has_arg<P, B...>());
        return 0;
    }
    else {
        return 1 + find_unique_arg<P, B...>();
    }
}


template<class T> struct GetArgByType {
    template<class A> struct Pred {
        static constexpr bool value = std::is_convertible_v<std::remove_reference_t<A>*, std::remove_reference_t<T>*>;
    };
};


template<class T, class F, std::size_t I> void for_each_tuple_elem(T& tuple, F& func)
{
    if constexpr (I < std::tuple_size_v<std::remove_reference_t<T>>) {
        func(std::get<I>(tuple)); // Throws
        for_each_tuple_elem<T, F, I + 1>(tuple, func); // Throws
    }
}


template<class T, class F, std::size_t I> bool for_each_tuple_elem_a(T& tuple, F& func)
{
    if constexpr (I < std::tuple_size_v<std::remove_reference_t<T>>) {
        if (ARCHON_LIKELY(func(std::get<I>(tuple)))) // Throws
            return for_each_tuple_elem_a<T, F, I + 1>(tuple, func); // Throws
        return false;
    }
    return true;
}


template<class T, T I, T N, class F> constexpr bool for_each_int_is_nothrow() noexcept
{
    if constexpr (I < N) {
        return (noexcept(std::declval<F&>()(std::integral_constant<T, I>())) &&
                for_each_int_is_nothrow<T, I + 1, N, F>());
    }
    else {
        return false;
    }
}


template<class T, T I, T N, class F> void for_each_int(F& func) noexcept(impl::for_each_int_is_nothrow<T, I, N, F>())
{
    if constexpr (I < N) {
        func(std::integral_constant<T, I>()); // Throws
        for_each_int<T, I + 1, N, F>(func); // Throws
    }
}


} // namespace archon::core::impl

#endif // ARCHON_X_CORE_X_IMPL_X_UTILITY_HPP
