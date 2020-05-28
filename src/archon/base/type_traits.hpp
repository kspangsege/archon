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

#ifndef ARCHON_X_BASE_X_TYPE_TRAITS_HPP
#define ARCHON_X_BASE_X_TYPE_TRAITS_HPP

/// \file


#include <cstdint>
#include <type_traits>
#include <limits>
#include <iterator>
#include <optional>

#include <archon/base/type_list.hpp>


namespace archon::base {


namespace detail {
template<int n, class F> struct LeastSignedWithBits;
template<int n, class F> struct LeastUnsignedWithBits;
template<int n, class F> struct FastestSignedWithBits;
template<int n, class F> struct FastestUnsignedWithBits;
template<class T> struct RemoveOptionalHelper;
} // namespace detail


template<class T> struct Wrap {
    using type = T;
};


template<class A, class B> using CopyConst = std::conditional_t<std::is_const_v<A>, const B, B>;


template<class I> using NeedIter =
    std::enable_if_t<std::is_convertible_v<typename std::iterator_traits<I>::iterator_category,
                                           std::input_iterator_tag>>;


template<class T> using Strut = std::aligned_storage_t<sizeof (T), alignof (T)>;


/// \brief Least signed integer type with specified number of value bits.
///
/// The first type in the following list, that has at least \p n value bits, or
/// `F` (the fallback) if no type in this list has \p n value bits:
///
///   - `char` if `char` is signed, else `signed char`
///   - `short`
///   - `int`
///   - `long`
///   - `long long`
///   - `std::intmax_t`
///
template<int n, class F = void> using LeastSignedWithBits =
    typename detail::LeastSignedWithBits<n, F>::type;


/// \brief Least unsigned integer type with specified number of value bits.
///
/// The first type in the following list, that has at least \p n value bits, or
/// `F` (the fallback) if no type in this list has \p n value bits:
///
///   - `char` if `char` is unsigned, else `unsigned char`
///   - `unsigned short`
///   - `unsigned`
///   - `unsigned long`
///   - `unsigned long long`
///   - `std::uintmax_t`
///
template<int n, class F = void> using LeastUnsignedWithBits =
    typename detail::LeastUnsignedWithBits<n, F>::type;



/// \brief Fastest signed integer type with specified number of value bits.
///
/// The first type in the following list, that has at least \p n value bits, or
/// `F` (the fallback) if no type in this list has \p n value bits:
///
///   - `int`
///   - `long`
///   - `long long`
///   - `std::intmax_t`
///
template<int n, class F = void> using FastestSignedWithBits =
    typename detail::FastestSignedWithBits<n, F>::type;


/// \brief Fastest unsigned integer type with at least specified width.
///
/// The first type in the following list, that has at least \p n value bits, or
/// `F` (the fallback) if no type in this list has \p n value bits:
///
///   - `unsigned`
///   - `unsigned long`
///   - `unsigned long long`
///   - `std::uintmax_t`
///
template<int n, class F = void> using FastestUnsignedWithBits =
    typename detail::FastestUnsignedWithBits<n, F>::type;



/// \brief Remove one layer of optionality.
///
/// If \p T has the form `std::optional<U>` for some type `U`, then
/// `RemoveOptional<T>` is `U`. Otherwise `RemoveOptional<T>` is \p T.
///
template<class T> using RemoveOptional = typename detail::RemoveOptionalHelper<T>::type;








// Implementation


namespace detail {


template<int n, class F> struct LeastSignedWithBits {
private:
    using char_type = std::conditional_t<std::is_signed_v<char>, char, signed char>;
    using types = base::TypeList<char_type, short, int, long, long long, std::intmax_t>;
    template<class T> struct Pred {
        static constexpr bool value = (std::numeric_limits<T>::digits >= n);
    };
public:
    using type = base::FindType<types, Pred, F>;
};


template<int n, class F> struct LeastUnsignedWithBits {
private:
    using char_type = std::conditional_t<std::is_unsigned_v<char>, char, unsigned char>;
    using types = base::TypeList<char_type, unsigned short, unsigned, unsigned long,
                                 unsigned long long, std::uintmax_t>;
    template<class T> struct Pred {
        static constexpr bool value = (std::numeric_limits<T>::digits >= n);
    };
public:
    using type = base::FindType<types, Pred, F>;
};


template<int n, class F> struct FastestSignedWithBits {
private:
    using types = base::TypeList<int, long, long long, std::intmax_t>;
    template<class T> struct Pred {
        static constexpr bool value = (std::numeric_limits<T>::digits >= n);
    };
public:
    using type = base::FindType<types, Pred, F>;
};


template<int n, class F> struct FastestUnsignedWithBits {
private:
    using types = base::TypeList<unsigned, unsigned long, unsigned long long, std::uintmax_t>;
    template<class T> struct Pred {
        static constexpr bool value = (std::numeric_limits<T>::digits >= n);
    };
public:
    using type = base::FindType<types, Pred, F>;
};


template<class T> struct RemoveOptionalHelper {
    using type = T;
};

template<class T> struct RemoveOptionalHelper<std::optional<T>> {
    using type = T;
};


} // namespace detail
} // namespace archon::base

#endif // ARCHON_X_BASE_X_TYPE_TRAITS_HPP
