// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


/// \file

#ifndef ARCHON__BASE__UTILITY_HPP
#define ARCHON__BASE__UTILITY_HPP

#include <cstddef>
#include <type_traits>
#include <utility>


namespace archon::base {


namespace detail {
template<std::size_t I, class A, class... B> struct GetArgTypeAtHelper;
} // namespace detail




template<std::size_t I, class... A> using GetArgTypeAt = typename detail::GetArgTypeAtHelper<I, A...>::type;

template<std::size_t I, class... A> constexpr auto&& get_arg_at(A&&... args) noexcept;

template<template<class> class P, class... A> constexpr bool has_arg() noexcept;

template<template<class> class P, class... A> constexpr std::size_t find_first_arg() noexcept;

template<template<class> class P, class... A> constexpr std::size_t find_unique_arg() noexcept;

template<template<class> class P, class... A> constexpr auto&& get_first_arg(A&&... args) noexcept;

template<template<class> class P, class... A> constexpr auto&& get_unique_arg(A&&... args) noexcept;

template<class T, class... A> constexpr T& get_first_arg_by_type(A&&... args) noexcept;

template<class T, class... A> constexpr T& get_unique_arg_by_type(A&&... args) noexcept;








// Implementation


namespace detail {


template<std::size_t I, class A, class... B> struct GetArgTypeAtHelper {
    using type = typename GetArgTypeAtHelper<I - 1, B...>::type;
};


template<class A, class... B> struct GetArgTypeAtHelper<0, A, B...> {
    using type = A;
};


template<std::size_t I, class A, class... B> constexpr auto&& get_arg_at_helper(A&& arg, B&&... args) noexcept
{
    if constexpr (I == 0) {
        return std::forward<A>(arg);
    }
    else {
        return get_arg_at_helper<I - 1>(std::forward<B>(args)...);
    }
}


template<template<class> class P> constexpr bool has_arg_helper() noexcept
{
    return false;
}


template<template<class> class P, class A, class... B> constexpr bool has_arg_helper() noexcept
{
    if constexpr (P<A>::value) {
        return true;
    }
    else {
        return has_arg_helper<P, B...>();
    }
}


template<template<class> class P, class A, class... B> constexpr std::size_t find_first_arg_helper() noexcept
{
    if constexpr (P<A>::value) {
        return 0;
    }
    else {
        return 1 + find_first_arg_helper<P, B...>();
    }
}


template<template<class> class P, class A, class... B> constexpr std::size_t find_unique_arg_helper() noexcept
{
    if constexpr (P<A>::value) {
        static_assert(!has_arg<P, B...>());
        return 0;
    }
    else {
        return 1 + find_unique_arg_helper<P, B...>();
    }
}


template<class T> struct GetArgByType {
    template<class A> struct Pred {
        static constexpr bool value =
            std::is_convertible_v<std::remove_reference_t<A>*, std::remove_reference_t<T>*>;
    };
};


} // namespace detail


template<std::size_t I, class... A> constexpr auto&& get_arg_at(A&&... args) noexcept
{
    return detail::get_arg_at_helper<I>(std::forward<A>(args)...);
}


template<template<class> class P, class... A> constexpr bool has_arg() noexcept
{
    return detail::has_arg_helper<P, A...>();
}


template<template<class> class P, class... A> constexpr std::size_t find_first_arg() noexcept
{
    return detail::find_first_arg_helper<P, A...>();
}


template<template<class> class P, class... A> constexpr std::size_t find_unique_arg() noexcept
{
    return detail::find_unique_arg_helper<P, A...>();
}


template<template<class> class P, class... A> constexpr auto&& get_first_arg(A&&... args) noexcept
{
    constexpr std::size_t I = find_first_arg<P, A...>();
    return get_arg_at<I>(std::forward<A>(args)...);
}


template<template<class> class P, class... A> constexpr auto&& get_unique_arg(A&&... args) noexcept
{
    constexpr std::size_t I = find_unique_arg<P, A...>();
    return get_arg_at<I>(std::forward<A>(args)...);
}


template<class T, class... A> constexpr T& get_first_arg_by_type(A&&... args) noexcept
{
    return get_first_arg<detail::GetArgByType<T>::template Pred>(std::forward<A>(args)...);
}


template<class T, class... A> constexpr T& get_unique_arg_by_type(A&&... args) noexcept
{
    return get_unique_arg<detail::GetArgByType<T>::template Pred>(std::forward<A>(args)...);
}


} // namespace archon::base

#endif // ARCHON__BASE__UTILITY_HPP
