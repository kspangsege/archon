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

#ifndef ARCHON_X_CORE_X_TYPE_HPP
#define ARCHON_X_CORE_X_TYPE_HPP

/// \file


#include <cstdint>
#include <type_traits>
#include <limits>
#include <iterator>
#include <optional>
#include <tuple>

#include <archon/core/type_list.hpp>
#include <archon/core/impl/type.hpp>


namespace archon::core {


namespace impl {
template<class, class> struct CopySignedness;
template<class> struct RemoveOptional;
template<class, class...> struct TypeIn;
template<class> struct FuncDecay;
template<class> struct TupleOfFuncParams;
template<class> struct TupleOfDecayedFuncParams;
template<class> struct ReturnType;
template<int, class> struct LeastSignedWithBits;
template<int, class> struct LeastUnsignedWithBits;
template<int, class> struct FastestSignedWithBits;
template<int, class> struct FastestUnsignedWithBits;
} // namespace impl



struct Empty {};



template<class T> struct Wrap {
    using type = T;
};



template<class T> using Type = typename core::Wrap<T>::type;



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



/// \brief Least signed integer type with specified number of value bits.
///
/// The first type in the following list, that has at least \p N value bits, or `F` (the
/// fallback) if no type in this list has \p N value bits:
///
///   - `char` if `char` is signed, else `signed char`
///   - `short`
///   - `int`
///   - `long`
///   - `long long`
///   - `std::intmax_t`
///
template<int N, class F = void> using LeastSignedWithBits = typename impl::LeastSignedWithBits<N, F>::type;


/// \brief Least unsigned integer type with specified number of value bits.
///
/// The first type in the following list, that has at least \p N value bits, or `F` (the
/// fallback) if no type in this list has \p N value bits:
///
///   - `char` if `char` is unsigned, else `unsigned char`
///   - `unsigned short`
///   - `unsigned`
///   - `unsigned long`
///   - `unsigned long long`
///   - `std::uintmax_t`
///
template<int N, class F = void> using LeastUnsignedWithBits = typename impl::LeastUnsignedWithBits<N, F>::type;



/// \brief Fastest signed integer type with specified number of value bits.
///
/// The first type in the following list, that has at least \p N value bits, or `F` (the
/// fallback) if no type in this list has \p N value bits:
///
///   - `int`
///   - `long`
///   - `long long`
///   - `std::intmax_t`
///
template<int N, class F = void> using FastestSignedWithBits = typename impl::FastestSignedWithBits<N, F>::type;


/// \brief Fastest unsigned integer type with at least specified width.
///
/// The first type in the following list, that has at least \p N value bits, or `F` (the
/// fallback) if no type in this list has \p N value bits:
///
///   - `unsigned`
///   - `unsigned long`
///   - `unsigned long long`
///   - `std::uintmax_t`
///
template<int N, class F = void> using FastestUnsignedWithBits = typename impl::FastestUnsignedWithBits<N, F>::type;


/// \brief Whether equality comparison between two given types is non-throwing.
///
/// This constant is `true` when, and only when the equality comparison between an object of
/// type \p T and an object of type \p U is a declared non-throwing operation (`noexcept`).
///
template<class T, class U> constexpr bool are_nothrow_equality_comparable =
    noexcept(std::declval<T>() == std::declval<U>());


/// \brief Whether three-way comparison between two given types is non-throwing.
///
/// This constant is `true` when, and only when the three-way comparison between an object
/// of type \p T and an object of type \p U is a declared non-throwing operation (`noexcept`).
///
template<class T, class U> constexpr bool are_nothrow_three_way_comparable =
    noexcept(std::declval<T>() <=> std::declval<U>());


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








// Implementation


namespace impl {


template<class T, class U> struct CopySignedness {
    static_assert(std::is_integral_v<T>);
    using type = std::conditional_t<std::is_signed_v<T>, std::make_signed_t<U>, std::make_unsigned_t<U>>;
};


template<class T> struct RemoveOptional {
    using type = T;
};

template<class T> struct RemoveOptional<std::optional<T>> {
    using type = T;
};


template<class T, class... U> struct TypeIn {
    static constexpr bool value = core::has_type<core::TypeList<U...>, T>;
};


template<class> struct FuncDecay1;

template<class R, class C, class... A> struct FuncDecay1<R(C::*)(A...) const> {
    using type = R(A...);
};

template<class R, class C, class... A> struct FuncDecay1<R(C::*)(A...)> {
    using type = R(A...);
};

template<class T> struct FuncDecay2 {
    using type = typename FuncDecay1<decltype(&T::operator())>::type;
};

template<class R, class... A> struct FuncDecay2<R(*)(A...)> {
    using type = R(A...);
};

template<class R, class C, class... A> struct FuncDecay2<R(C::*)(A...) const> {
    using type = R(A...);
};

template<class R, class C, class... A> struct FuncDecay2<R(C::*)(A...)> {
    using type = R(A...);
};

template<class F> struct FuncDecay {
    using type = typename FuncDecay2<std::decay_t<F>>::type;
};


template<class R, class... P> struct TupleOfFuncParams<R(P...)> {
    using type = std::tuple<P...>;
};


template<class R, class... P> struct TupleOfDecayedFuncParams<R(P...)> {
    using type = std::tuple<std::decay_t<P>...>;
};


template<class R, class... P> struct ReturnType<R(P...)> {
    using type = R;
};


template<int N, class F> struct LeastSignedWithBits {
private:
    using char_type = std::conditional_t<std::is_signed_v<char>, char, signed char>;
    using types = core::TypeList<char_type, short, int, long, long long, std::intmax_t>;
    struct Pred {
        template<class T> static constexpr bool value = (std::numeric_limits<T>::digits >= N);
    };
public:
    using type = core::FindType<types, Pred, F>;
};


template<int N, class F> struct LeastUnsignedWithBits {
private:
    using char_type = std::conditional_t<std::is_unsigned_v<char>, char, unsigned char>;
    using types = core::TypeList<char_type, unsigned short, unsigned, unsigned long, unsigned long long,
                                 std::uintmax_t>;
    struct Pred {
        template<class T> static constexpr bool value = (std::numeric_limits<T>::digits >= N);
    };
public:
    using type = core::FindType<types, Pred, F>;
};


template<int N, class F> struct FastestSignedWithBits {
private:
    using types = core::TypeList<int, long, long long, std::intmax_t>;
    struct Pred {
        template<class T> static constexpr bool value = (std::numeric_limits<T>::digits >= N);
    };
public:
    using type = core::FindType<types, Pred, F>;
};


template<int N, class F> struct FastestUnsignedWithBits {
private:
    using types = core::TypeList<unsigned, unsigned long, unsigned long long, std::uintmax_t>;
    struct Pred {
        template<class T> static constexpr bool value = (std::numeric_limits<T>::digits >= N);
    };
public:
    using type = core::FindType<types, Pred, F>;
};


} // namespace impl
} // namespace archon::core

#endif // ARCHON_X_CORE_X_TYPE_HPP
