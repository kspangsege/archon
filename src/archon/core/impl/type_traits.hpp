// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2023 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_CORE_X_IMPL_X_TYPE_TRAITS_HPP
#define ARCHON_X_CORE_X_IMPL_X_TYPE_TRAITS_HPP


#include <cstdint>
#include <type_traits>
#include <limits>
#include <utility>
#include <optional>
#include <tuple>
#include <ostream>

#include <archon/core/type_list.hpp>


namespace archon::core::impl {


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



template<class F> struct TupleOfFuncParams;

template<class R, class... P> struct TupleOfFuncParams<R(P...)> {
    using type = std::tuple<P...>;
};



template<class F> struct TupleOfDecayedFuncParams;

template<class R, class... P> struct TupleOfDecayedFuncParams<R(P...)> {
    using type = std::tuple<std::decay_t<P>...>;
};



template<class F> struct ReturnType;

template<class R, class... P> struct ReturnType<R(P...)> {
    using type = R;
};



template<class P, class F> struct LeastSignedIntTypeA {
private:
    using char_type = std::conditional_t<std::is_signed_v<char>, char, signed char>;
    using types = core::TypeList<char_type, short, int, long, long long, std::intmax_t>;
public:
    using type = core::FindType<types, P, F>;
};


template<class P, class F> struct LeastUnsignedIntTypeA {
private:
    using char_type = std::conditional_t<std::is_unsigned_v<char>, char, unsigned char>;
    using types = core::TypeList<char_type, unsigned short, unsigned, unsigned long, unsigned long long,
                                 std::uintmax_t>;
public:
    using type = core::FindType<types, P, F>;
};


template<class P, class F> struct FastSignedIntTypeA {
private:
    using types = core::TypeList<int, long, long long, std::intmax_t>;
public:
    using type = core::FindType<types, P, F>;
};


template<class P, class F> struct FastUnsignedIntTypeA {
private:
    using types = core::TypeList<unsigned, unsigned long, unsigned long long, std::uintmax_t>;
public:
    using type = core::FindType<types, P, F>;
};



template<int N> struct IntTypeMinWidthPred {
    template<class T> static constexpr int width = (std::numeric_limits<T>::digits +
                                                    int(std::numeric_limits<T>::is_signed));
    template<class T> static constexpr bool value = (width<T> >= N);
};


template<int N, class F> struct LeastSignedIntType {
    using type = typename impl::LeastSignedIntTypeA<impl::IntTypeMinWidthPred<N>, F>::type;
};

template<int N, class F> struct LeastUnsignedIntType {
    using type = typename impl::LeastUnsignedIntTypeA<impl::IntTypeMinWidthPred<N>, F>::type;
};

template<int N, class F> struct FastSignedIntType {
    using type = typename impl::FastSignedIntTypeA<impl::IntTypeMinWidthPred<N>, F>::type;
};

template<int N, class F> struct FastUnsignedIntType {
    using type = typename impl::FastUnsignedIntTypeA<impl::IntTypeMinWidthPred<N>, F>::type;
};



template<class T, class C, class U, class = decltype(std::declval<std::basic_ostream<C, U>>() << std::declval<T>())> constexpr bool has_stream_output_operator(int)
{
    return true;
}

template<class T, class C, class U> constexpr bool has_stream_output_operator(long)
{
    return false;
}


} // namespace archon::core::impl

#endif // ARCHON_X_CORE_X_IMPL_X_TYPE_TRAITS_HPP
