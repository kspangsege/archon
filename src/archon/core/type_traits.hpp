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

#ifndef ARCHON_X_CORE_X_TYPE_TRAITS_HPP
#define ARCHON_X_CORE_X_TYPE_TRAITS_HPP

/// \file


#include <type_traits>
#include <iterator>
#include <utility>
#include <string>

#include <archon/core/impl/type_traits.hpp>


namespace archon::core {


/// \brief Pick alternative type when main type is void.
///
/// This type alias is \p T if \p T is not `void`. Otherwise it is \p U.
///
template<class T, class U> using NotVoidOr = std::conditional_t<!std::is_same_v<T, void>, T, U>;



template<class I> using NeedIntegral = std::enable_if_t<std::is_integral_v<I>>;

template<class I> using NeedIter =
    std::enable_if_t<std::is_convertible_v<typename std::iterator_traits<I>::iterator_category,
                                           std::input_iterator_tag>>;



/// \brief Copy constness from one type to another.
///
/// If `T` is a `const` type, then `CopyConstness<T, U>` is `const U`. Otherwise it is `U`.
///
template<class T, class U> using CopyConstness = std::conditional_t<std::is_const_v<T>, const U, U>;



/// \brief Copy signedness from one integer type to another.
///
/// If `T` is a signed integer type according to `std::is_signed<>()`, then
/// `CopySignedness<T, U>` is `std::make_signed<U>`. Otherwise it is
/// `std::make_unsigned<U>`.
///
/// \p T must be a type such that `std::is_integral_v<T>` is `true`.
///
template<class T, class U> using CopySignedness = typename impl::CopySignedness<T, U>::type;



/// \brief Remove one layer of optionality.
///
/// If \p T has the form `std::optional<U>` for some type `U`, then `RemoveOptional<T>` is
/// `U`. Otherwise `RemoveOptional<T>` is \p T.
///
template<class T> using RemoveOptional = typename impl::RemoveOptional<T>::type;



/// \brief Whether type is in list.
///
/// This constant is `true` if, and only if the specified type (\p T) is in the specified
/// list of types (\p U...).
///
template<class T, class... U> constexpr bool type_in = impl::TypeIn<T, U...>::value;



/// \brief Reduce function type to its most basic form.
///
/// If `T` is a function type, a pointer to a function type, or a reference to a function
/// type, `FuncDecay<T>` is the function type (just like `std::decay_t<T>`). If T is a
/// pointer to a non-static member function, `FuncDecay<T>` is a function type with the same
/// return type, same number of parameters, and same parameter types. If `T` is a
/// function-like object, `FuncDecay<T>` is `FuncDecay<decltype(&T::operator())>`.
///
/// While lambdas generally count as function-like objects in this context, generic lambdas
/// do not (when `auto` is used as a parameter type), because generic lambdas do not
/// correspond to a definite function type until they are instantiated through invocation.
///
template<class T> using FuncDecay = typename impl::FuncDecay<T>::type;



/// \brief Type of tuple of function parameters.
///
/// Construct a tuple type with one component per parameter in the specified function type
/// (\p F), and such that the type of the n'th component is the type of the n'th function
/// parameter.
///
/// \tparam F The function type. It can be anything that is recognized as a function by \ref
/// core::FuncDecay.
///
/// \sa \ref core::TupleOfDecayedFuncParams
///
template<class F> using TupleOfFuncParams = typename impl::TupleOfFuncParams<core::FuncDecay<F>>::type;



/// \brief Type of tuple of decayed function parameters.
///
/// Construct a tuple type with one component per parameter in the specified function type
/// (\p F), and such that the type of the n'th component is `std::decay_t<T>` if `T` is the
/// type of the n'th function parameter.
///
/// \tparam F The function type. It can be anything that is recognized as a function by \ref
/// core::FuncDecay.
///
/// \sa \ref core::TupleOfFuncParams
///
template<class F> using TupleOfDecayedFuncParams = typename impl::TupleOfDecayedFuncParams<core::FuncDecay<F>>::type;



/// \brief Return type of function.
///
/// Get return type of function of specified type.
///
/// \tparam F The function type. It can be anything that is recognized as a function by \ref
/// core::FuncDecay.
///
template<class F> using ReturnType = typename impl::ReturnType<core::FuncDecay<F>>::type;



/// \{
///
/// \brief Least or fast signed or unsigned integer type with specified width.
///
/// `least_signed_int_type<N, F>` is an alias for `least_signed_int_type_a<P, F>` where `P`
/// is a predicate that requires the width of the type (\ref core::int_width()) to be
/// greater than, or equal to \p N. The width of the type is the number of value bits plus
/// one if the type is signed (see \ref core::int_width()).
///
/// \sa \ref core::least_signed_int_type
/// \sa \ref core::least_unsigned_int_type
/// \sa \ref core::fast_signed_int_type
/// \sa \ref core::fast_unsigned_int_type
/// \sa \ref core::least_signed_int_type_a
/// \sa \ref core::least_unsigned_int_type_a
/// \sa \ref core::fast_signed_int_type_a
/// \sa \ref core::fast_unsigned_int_type_a
///
template<int N, class F = void> using least_signed_int_type = typename impl::LeastSignedIntType<N, F>::type;
template<int N, class F = void> using least_unsigned_int_type = typename impl::LeastUnsignedIntType<N, F>::type;
template<int N, class F = void> using fast_signed_int_type = typename impl::FastSignedIntType<N, F>::type;
template<int N, class F = void> using fast_unsigned_int_type = typename impl::FastUnsignedIntType<N, F>::type;
/// \}



/// \{
///
/// \brief Least or fast signed or unsigned integer type satisfying specific criterion.
///
/// These types are aliases for the firsts type in the corresponding lists of integer types
/// below that satisfy the specified predicate (\p P), or, in each case, if none of those
/// types satisfy the predicate, it is an alias for the fallback type, which is \p F.
///
/// Candidate types for `least_signed_int_type_a`:
///
///   * `char` if `char` is signed, else `signed char`
///   * `short`
///   * `int`
///   * `long`
///   * `long long`
///   * `std::intmax_t`
///
/// Candidate types for `least_unsigned_int_type_a`:
///
///   * `char` if `char` is unsigned, else `unsigned char`
///   * `unsigned short`
///   * `unsigned`
///   * `unsigned long`
///   * `unsigned long long`
///   * `std::uintmax_t`
///
/// Candidate types for `fast_signed_int_type_a`:
///
///   * `int`
///   * `long`
///   * `long long`
///   * `std::intmax_t`
///
/// Candidate types for `fast_unsigned_int_type_a`:
///
///   * `unsigned`
///   * `unsigned long`
///   * `unsigned long long`
///   * `std::uintmax_t`
///
/// \sa \ref core::least_signed_int_type
/// \sa \ref core::least_unsigned_int_type
/// \sa \ref core::fast_signed_int_type
/// \sa \ref core::fast_unsigned_int_type
/// \sa \ref core::least_signed_int_type_a
/// \sa \ref core::least_unsigned_int_type_a
/// \sa \ref core::fast_signed_int_type_a
/// \sa \ref core::fast_unsigned_int_type_a
///
template<class P, class F = void> using least_signed_int_type_a = typename impl::LeastSignedIntTypeA<P, F>::type;
template<class P, class F = void> using least_unsigned_int_type_a = typename impl::LeastUnsignedIntTypeA<P, F>::type;
template<class P, class F = void> using fast_signed_int_type_a = typename impl::FastSignedIntTypeA<P, F>::type;
template<class P, class F = void> using fast_unsigned_int_type_a = typename impl::FastUnsignedIntTypeA<P, F>::type;
/// \}



/// \brief Whether equality comparison between two given types is non-throwing.
///
/// This constant is `true` when, and only when the equality comparison operation between an
/// object of type \p T and an object of type \p U is a declared non-throwing operation
/// (`noexcept`).
///
template<class T, class U> constexpr bool are_nothrow_equality_comparable =
    noexcept(std::declval<const T&>() == std::declval<const U&>());


/// \brief Whether less-than comparison between two given types is non-throwing.
///
/// This constant is `true` when, and only when the less-than comparison operation between
/// an object of type \p T and an object of type \p U is a declared non-throwing operation
/// (`noexcept`).
///
template<class T, class U> constexpr bool are_nothrow_less_comparable =
    noexcept(std::declval<const T&>() < std::declval<const U&>());


/// \brief Whether three-way comparison between two given types is non-throwing.
///
/// This constant is `true` when, and only when the three-way comparison operation between
/// an object of type \p T and an object of type \p U is a declared non-throwing operation
/// (`noexcept`).
///
template<class T, class U> constexpr bool are_nothrow_three_way_comparable =
    noexcept(std::declval<const T&>() <=> std::declval<const U&>());


/// \{
///
/// \brief Whether comparison is non-throwing for objects of specific type.
///
/// These constants are shorthands for \ref core::are_nothrow_equality_comparable, \ref
/// core::are_nothrow_less_comparable, and \ref core::are_nothrow_three_way_comparable with
/// both \p T and \p U set to \p T.
///
template<class T> constexpr bool is_nothrow_equality_comparable = core::are_nothrow_equality_comparable<T, T>;
template<class T> constexpr bool is_nothrow_less_comparable = core::are_nothrow_less_comparable<T, T>;
template<class T> constexpr bool is_nothrow_three_way_comparable = core::are_nothrow_three_way_comparable<T, T>;
/// \}


/// \brief Whether a stream output operator exists for given value and character type.
///
/// This constant is `true` when there is a stream output operator (`<<`) for an object of
/// type \p T and an output stream whose character type is \p C.
///
/// More precisely, if `out` is an object of type `std::basic_ostream<C>` and `val` is an
/// object of type `T`, then this constant is `true` when, and only when the expression `out
/// << val` is well formed.
///
template<class T, class C, class U = std::char_traits<C>> constexpr bool has_stream_output_operator =
    impl::has_stream_output_operator<T, C, U>(int());


} // namespace archon::core

#endif // ARCHON_X_CORE_X_TYPE_TRAITS_HPP
