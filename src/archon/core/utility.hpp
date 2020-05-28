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

#ifndef ARCHON_X_CORE_X_UTILITY_HPP
#define ARCHON_X_CORE_X_UTILITY_HPP

/// \file


#include <cstddef>
#include <type_traits>
#include <utility>
#include <algorithm>
#include <tuple>

#include <archon/core/assert.hpp>
#include <archon/core/impl/utility.hpp>


namespace archon::core {


/// \{
///
/// \brief Maximum and minimum over values of heterogeneous type.
///
/// These functions are shorthands for calling `std::min()` and `std::max()` respectively
/// with arguments cast to the common type as determined by `std::common_type_t<T...>`.
///
template<class... T> constexpr auto hetero_min(T&&...);
template<class... T> constexpr auto hetero_max(T&&...);
/// \}



/// \brief Properly hide members of base class.
///
/// A class `A` can list `HiddenBase<B>` as a base class in order to retain the effect of
/// empty base class optimization while avoiding making the members of `B` visible in the
/// scope of `A`. This is relevant in some cases, and particularly when `B` may have virtual
/// functions.
///
/// A reference to the state corresponding to the hidden base class is returned by
/// `hidden_base()` when called from within the scope of `A`. In a 'const' context, a
/// `const` reference is returned.
///
/// `HiddenBase<B>` is constructible from any set of arguments that `B` is constructible
/// from. Even when `B` is an empty class, an object of type `B` is constructed, but then
/// thrown away.
///
/// When \p B is an empty class, it must be default constructible at compile time.
///
template<class B> using HiddenBase = impl::HiddenBase<B, std::is_empty_v<B>>;



/// \brief Invoke one of multiple instantiations of template function.
///
/// Invoke the specified instantiation (\p i) of the specified function template (\p F) with
/// the specified arguments (\p args).
///
/// The returned value is whatever is returned by the invoked instantiation.
///
/// \param i The value corresponding to the function template instantiation to be
/// invoked. Behavior is undefined if the specified value is greater than, or equal to \p N.
///
/// \param args These arguments will be forwarded perfectly to the specified template
/// function instantiation.
///
/// \tparam F A class type such that `F::template exec<I>` is a static function that accepts
/// the specified arguments (\p args) for all values of `I` less than \p N. The return type
/// must be invariant for all values of `I` less than \p N.
///
/// \tparam N The number of different instantiations of the function template (\p F) that
/// must be made available.
///
template<class F, std::size_t N, class... A> auto dispatch(std::size_t i, A&&... args);



/// \brief Pick type from template parameter pack by index.
///
/// If `A...` is a template parameter pack of types and `I` is the index of one of the types
/// in that parameter pack, then `GetArgTypeAt<I, A...>` is that type.
///
template<std::size_t I, class... A> using GetArgTypeAt = typename impl::GetArgTypeAt<I, A...>::type;


/// \brief Pick argument from function parameter pack by index.
///
/// If `args...` is a function parameter pack and `I` is the index of one of the arguments
/// in that parameter pack, then `get_arg_at<I>(args...)` returns a reference to that
/// argument.
///
template<std::size_t I, class... A> constexpr auto get_arg_at(A&&... args) noexcept -> auto&&;


/// \brief Check template parameter pack for type satisfying predicate.
///
/// If `A...` is a template parameter pack of types and `P` is a predicate, then `has_arg<P,
/// A...>() GetArgTypeAt<I, A...>` returns `true` if, and only if the predicate is `true`
/// for at least one of the elements in the parameter pack. The predicate must be a class
/// template with one parameter such that `P<T>::value` is a compile-time constant that is
/// convertible to `bool` for any of the types in `A...`. The predicate evaluates to `true`
/// for `T` if, and only if `P<T>::value` evaluates to `true`.
///
/// \sa \ref core::has_unique_arg().
/// \sa \ref core::find_first_arg().
///
template<template<class> class P, class... A> constexpr bool has_arg() noexcept;


/// \brief Whether template parameter pack contains exactly one type satisfying predicate.
///
/// If `A...` is a template parameter pack of types and `P` is a predicate, then `has_arg<P,
/// A...>() GetArgTypeAt<I, A...>` returns `true` if, and only if the predicate is `true`
/// for exactly one of the elements in the parameter pack. If the number of types for which
/// the predicate is `true` is zero, or more than one, this function returns `false`. The
/// predicate must be a class template with one parameter such that `P<T>::value` is a
/// compile-time constant that is convertible to `bool` for any of the types in `A...`. The
/// predicate evaluates to `true` for `T` if, and only if `P<T>::value` evaluates to `true`.
///
/// \sa \ref core::has_arg().
/// \sa \ref core::find_unique_arg().
///
template<template<class> class P, class... A> constexpr bool has_unique_arg() noexcept;


/// \brief Find index of first type in parameter pack satisfying predicate.
///
/// If `has_arg<P, A...>()` returns `true` for a particular predicate, `P`; and a particular
/// parameter pack, `A...`; then `find_first_arg<P, A>()` returns the index of the first
/// type in the parameter pack that satisfies the predicate.
///
/// If `has_arg<P, A...>()` returns `false`, then `find_first_arg<P, A>()` will fail to
/// compile.
///
/// \sa \ref core::has_arg().
/// \sa \ref core::get_first_arg().
/// \sa \ref core::find_unique_arg().
///
template<template<class> class P, class... A> constexpr auto find_first_arg() noexcept -> std::size_t;


/// \brief Find index of unique type in parameter pack satisfying predicate.
///
/// If `has_unique_arg<P, A...>()` returns `true` for a particular predicate, `P`; and a
/// particular parameter pack, `A...`; then `find_unique_arg<P, A>()` returns the index of
/// the unique type in the parameter pack that satisfies the predicate.
///
/// If `has_unique_arg<P, A...>()` returns `false`, then `find_unique_arg<P, A>()` will fail
/// to compile.
///
/// \sa \ref core::has_unique_arg().
/// \sa \ref core::get_unique_arg().
/// \sa \ref core::find_first_arg().
///
template<template<class> class P, class... A> constexpr auto find_unique_arg() noexcept -> std::size_t;


/// \brief Get first argument whose type satisfies predicate.
///
/// This function returns a reference to the first argument whose type satisfies the
/// specified predicate (\p P). The index of the argument is found as if by
/// `find_first_arg<P, A...>()`.
///
/// \sa \ref find_first_arg().
/// \sa \ref get_unique_arg().
///
template<template<class> class P, class... A> constexpr auto get_first_arg(A&&... args) noexcept -> auto&&;


/// \brief Get unique argument whose type satisfies predicate.
///
/// This function returns a reference to the unique argument whose type satisfies the
/// specified predicate (\p P). The index of the argument is found as if by
/// `find_unique_arg<P, A...>()`.
///
/// \sa \ref find_unique_arg().
/// \sa \ref get_first_arg().
///
template<template<class> class P, class... A> constexpr auto get_unique_arg(A&&... args) noexcept -> auto&&;


/// \brief Get first argument with matching type.
///
/// This function returns a reference to the first argument whose type matches the specified
/// type (\p T). More precisely, it is a shorthand for calling \ref core::get_first_arg()
/// with a predicate that evaluates to `true` when, and only when
/// `std::is_convertible_v<std::remove_reference_t<A>*, std::remove_reference_t<T>*>` is
/// `true`. Here, `A` stands for one of the argument types.
///
template<class T, class... A> constexpr auto get_first_arg_by_type(A&&... args) noexcept -> T&;


/// \brief Get unique argument with matching type.
///
/// This function returns a reference to the unique argument whose type matches the
/// specified type (\p T). More precisely, it is a shorthand for calling \ref
/// core::get_unique_arg() with a predicate that evaluates to `true` when, and only when
/// `std::is_convertible_v<std::remove_reference_t<A>*, std::remove_reference_t<T>*>` is
/// `true`. Here, `A` stands for one of the argument types.
///
template<class T, class... A> constexpr auto get_unique_arg_by_type(A&&... args) noexcept -> T&;



/// \brief Execute function for each element in tuple.
///
/// Execute the specified generic function (e.g., generic lambda) for each element in the
/// specified tuple.
///
/// The specified function (\p func) must be callable with a single argument, which will be
/// a reference to one of the tuple elements. The elements will be called for in the order
/// that they occur in the tuple.
///
/// The type of the tuple-like object can be any of `std::tuple`, `std::pair`, or
/// `std::array`.
///
/// If a non-`const` tuple-like object is passed, the arguments passed to the specified
/// function will also be references to non-`const` tuple elements.
///
/// \sa \ref core::for_each_tuple_elem_a()
///
template<class T, class F> void for_each_tuple_elem(T&& tuple, F&& func);


/// \brief Execute function for each element in tuple until failure.
///
/// Execute the specified generic function (e.g., generic lambda) for each element in the
/// specified tuple in turn until an execution returns `false`. If execution returns `false`
/// for a particular element, no execution of the specified generic function will take place
/// for subsequent elements in the tuple.
///
/// If execution returns `true` for every element in the tuple, this function returns
/// `true`, otherwise it returns `false`.
///
/// The specified function (\p func) must be callable with a single argument, which will be
/// a reference to one of the tuple elements. The elements will be called for in the order
/// that they occur in the tuple.
///
/// The type of the tuple-like object can be any of `std::tuple`, `std::pair`, or
/// `std::array`.
///
/// If a non-`const` tuple-like object is passed, the arguments passed to the specified
/// function will also be references to non-`const` tuple elements.
///
/// \sa \ref core::for_each_tuple_elem()
///
template<class T, class F> bool for_each_tuple_elem_a(T&& tuple, F&& func);


/// \brief Execute function for specific element in tuple.
///
/// Execute the specified generic function (e.g., generic lambda) for the specified element
/// (\p i) in the specified tuple.
///
/// Behavior is undefined if the specified element index (\p i) is greater than, or equal to
/// the size of the tuple.
///
/// The type of the tuple-like object can be any of `std::tuple`, `std::pair`, or
/// `std::array`.
///
/// The returned value will be whatever is returned by \p func.
///
template<class T, class F> auto with_tuple_elem(T&& tuple, std::size_t i, F&& func);


/// \brief Determined whether compile-time iteration over integers can throw.
///
/// This function determines whether \ref core::for_each_int() is guaranteed to not throw
/// exceptions. Formally, it returns true if, and only if
/// `noexcept(std::declval<F&>(std::integral_constant<T, I>()))` is `true` for all
/// non-negative values of `I` of type \p T strictly less than \p N.
///
template<class T, T N, class F> constexpr bool for_each_int_is_nothrow() noexcept;


/// \brief Iterate over compile-time given integer values.
///
/// This function will execute the specified generic function (e.g., generic lambda) once
/// for each non-negative integer value strictly less than \p N. A particular value, `I`, is
/// passed in the form of a default constructed object of type `std::integral_constant<T,
/// I>`. The executions happen in order of integer value.
///
/// This function is intended to be used in place of `constexpr for`, which does not
/// currently exist in C++. Here is an example:
///
/// \code{.cpp}
///
///     archon::core::for_each_int<int, num_channels>([&](auto obj) {
///         constexpr int i = obj.value;
///         constexpr int depth = channel_depths[i];
///         target[i] = decode<depth>(source[i]);
///     });
///
/// \endcode
///
template<class T, T N, class F> auto for_each_int(F&& func) noexcept(core::for_each_int_is_nothrow<T, N, F>());








// Implementation


template<class... T> constexpr auto hetero_min(T&&... values)
{
    using type = std::common_type_t<T...>;
    return std::min({ type(std::forward<T>(values))... }); // Throws
}


template<class... T> constexpr auto hetero_max(T&&... values)
{
    using type = std::common_type_t<T...>;
    return std::max({ type(std::forward<T>(values))... }); // Throws
}


template<class F, std::size_t N, class... A> auto dispatch(std::size_t i, A&&... args)
{
    ARCHON_ASSERT(i < N);
    constexpr auto array = impl::make_dispatch_array<F>(std::make_index_sequence<N>());
    return (*array[i])(std::forward<A>(args)...);
}


template<std::size_t I, class... A> constexpr auto get_arg_at(A&&... args) noexcept -> auto&&
{
    return impl::get_arg_at<I>(std::forward<A>(args)...);
}


template<template<class> class P, class... A> constexpr bool has_arg() noexcept
{
    return impl::has_arg<P, A...>();
}


template<template<class> class P, class... A> constexpr bool has_unique_arg() noexcept
{
    return impl::has_unique_arg<P, A...>();
}


template<template<class> class P, class... A> constexpr auto find_first_arg() noexcept -> std::size_t
{
    return impl::find_first_arg<P, A...>();
}


template<template<class> class P, class... A> constexpr auto find_unique_arg() noexcept -> std::size_t
{
    return impl::find_unique_arg<P, A...>();
}


template<template<class> class P, class... A> constexpr auto get_first_arg(A&&... args) noexcept -> auto&&
{
    constexpr std::size_t I = find_first_arg<P, A...>();
    return get_arg_at<I>(std::forward<A>(args)...);
}


template<template<class> class P, class... A> constexpr auto get_unique_arg(A&&... args) noexcept -> auto&&
{
    constexpr std::size_t I = find_unique_arg<P, A...>();
    return get_arg_at<I>(std::forward<A>(args)...);
}


template<class T, class... A> constexpr auto get_first_arg_by_type(A&&... args) noexcept -> T&
{
    return get_first_arg<impl::GetArgByType<T>::template Pred>(std::forward<A>(args)...);
}


template<class T, class... A> constexpr auto get_unique_arg_by_type(A&&... args) noexcept -> T&
{
    return get_unique_arg<impl::GetArgByType<T>::template Pred>(std::forward<A>(args)...);
}


template<class T, class F> void for_each_tuple_elem(T&& tuple, F&& func)
{
    impl::for_each_tuple_elem<T, F, 0>(tuple, func); // Throws
}


template<class T, class F> bool for_each_tuple_elem_a(T&& tuple, F&& func)
{
    return impl::for_each_tuple_elem_a<T, F, 0>(tuple, func); // Throws
}


template<class T, class F> auto with_tuple_elem(T&& tuple, std::size_t i, F&& func)
{
    constexpr std::size_t n = std::tuple_size_v<std::remove_reference_t<T>>;
    core::dispatch<impl::WithTupleElem<T, F>, n>(i, tuple, func); // Throws
}


template<class T, T N, class F> constexpr bool for_each_int_is_nothrow() noexcept
{
    return impl::for_each_int_is_nothrow<T, 0, N, F>();
}


template<class T, T N, class F> auto for_each_int(F&& func) noexcept(core::for_each_int_is_nothrow<T, N, F>())
{
    impl::for_each_int<T, 0, N, F>(func); // Throws
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_UTILITY_HPP
