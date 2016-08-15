/*
 * This file is part of the Archon library framework.
 *
 * Copyright (C) 2012  Kristian Spangsege <kristian.spangsege@gmail.com>
 *
 * The Archon library framework is free software: You can redistribute
 * it and/or modify it under the terms of the GNU Lesser General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * The Archon library framework is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the Archon library framework.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef ARCHON_CORE_META_HPP
#define ARCHON_CORE_META_HPP

/// \file
///
/// \author Kristian Spangsege
///
/// Provides some tools for meta-programming, that is, programs that
/// run at compile time through template expansion. In
/// meta-programming templates are functions, and template arguments
/// are function arguments.

#include <climits>
#include <cwchar>
#include <limits>

#include <archon/features.h>

#ifdef ARCHON_HAVE_CXX11_TYPE_TRAITS
#  include <type_traits>
#endif

#ifdef ARCHON_HAVE_CXX11
#  define ARCHON_CORE_META_HAVE_LONG_LONG 1
#endif

// FIXME: In case ARCHON_HAVE_CXX11_TYPE_TRAITS is not defined,
// `IntegralPromote` and `ArithBinOpType` must both fail if the type
// is not a standard type, and if it is `long long` or `unsigned long
// long` and ARCHON_CORE_META_HAVE_LONG_LONG is not defined.

namespace archon {
namespace Core {


/// A ternary operator that selects the first type if the condition
/// evaluates to true, otherwise it selects the second type.
template<bool cond, class A, class B> struct CondType   { typedef A type; };
template<class A, class B> struct CondType<false, A, B> { typedef B type; };

template<class A, class B> struct SameType { static bool const value = false; };
template<class A> struct SameType<A,A>     { static bool const value = true;  };

template<class T, class A, class B> struct EitherTypeIs { static const bool value = false; };
template<class T, class A> struct EitherTypeIs<T,T,A> { static const bool value = true; };
template<class T, class A> struct EitherTypeIs<T,A,T> { static const bool value = true; };
template<class T> struct EitherTypeIs<T,T,T> { static const bool value = true; };

template<class T> struct IsConst          { static const bool value = false; };
template<class T> struct IsConst<const T> { static const bool value = true;  };

template<class From, class To> struct CopyConstness                 { typedef       To type; };
template<class From, class To> struct CopyConstness<const From, To> { typedef const To type; };

template<class T> struct DerefType {};
template<class T> struct DerefType<T*> { typedef T type; };



/// Determine whether a type is an integral type.
#ifdef ARCHON_HAVE_CXX11_TYPE_TRAITS
template<class T> struct IsIntegral { static const bool value = std::is_integral<T>::value; };
#else // !ARCHON_HAVE_CXX11_TYPE_TRAITS
template<class T> struct IsIntegral { static const bool value = false; };
template<> struct IsIntegral<bool>               { static const bool value = true; };
template<> struct IsIntegral<char>               { static const bool value = true; };
template<> struct IsIntegral<signed char>        { static const bool value = true; };
template<> struct IsIntegral<unsigned char>      { static const bool value = true; };
template<> struct IsIntegral<wchar_t>            { static const bool value = true; };
template<> struct IsIntegral<short>              { static const bool value = true; };
template<> struct IsIntegral<unsigned short>     { static const bool value = true; };
template<> struct IsIntegral<int>                { static const bool value = true; };
template<> struct IsIntegral<unsigned>           { static const bool value = true; };
template<> struct IsIntegral<long>               { static const bool value = true; };
template<> struct IsIntegral<unsigned long>      { static const bool value = true; };
#  ifdef ARCHON_CORE_META_HAVE_LONG_LONG
template<> struct IsIntegral<long long>          { static const bool value = true; };
template<> struct IsIntegral<unsigned long long> { static const bool value = true; };
#  endif
#endif // !ARCHON_HAVE_CXX11_TYPE_TRAITS



/// Determine the type resulting from integral promotion.
///
/// \note Enum types are supported only when the compiler supports the
/// C++11 'decltype' feature.
#ifdef ARCHON_HAVE_CXX11_DECLTYPE
template<class T> struct IntegralPromote { typedef decltype(+T()) type; };
#else // !ARCHON_HAVE_CXX11_DECLTYPE
template<class T> struct IntegralPromote;
template<> struct IntegralPromote<bool> { typedef int type; };
template<> struct IntegralPromote<char> {
    typedef CondType<INT_MIN <= CHAR_MIN && CHAR_MAX <= INT_MAX, int, unsigned>::type type;
};
template<> struct IntegralPromote<signed char> {
    typedef CondType<INT_MIN <= SCHAR_MIN && SCHAR_MAX <= INT_MAX, int, unsigned>::type type;
};
template<> struct IntegralPromote<unsigned char> {
    typedef CondType<UCHAR_MAX <= INT_MAX, int, unsigned>::type type;
};
template<> struct IntegralPromote<wchar_t> {
private:
#  ifdef ARCHON_CORE_META_HAVE_LONG_LONG
    typedef CondType<LLONG_MIN <= WCHAR_MIN && WCHAR_MAX <= LLONG_MAX, long long, unsigned long long>::type type_1;
    typedef CondType<0 <= WCHAR_MIN && WCHAR_MAX <= ULONG_MAX, unsigned long, type_1>::type type_2;
#  else
    typedef unsigned long type_2;
#  endif
    typedef CondType<LONG_MIN <= WCHAR_MIN && WCHAR_MAX <= LONG_MAX, long, type_2>::type type_3;
    typedef CondType<0 <= WCHAR_MIN && WCHAR_MAX <= UINT_MAX, unsigned, type_3>::type type_4;
public:
    typedef CondType<INT_MIN <= WCHAR_MIN && WCHAR_MAX <= INT_MAX, int, type_4>::type type;
};
template<> struct IntegralPromote<short> {
    typedef CondType<INT_MIN <= SHRT_MIN && SHRT_MAX <= INT_MAX, int, unsigned>::type type;
};
template<> struct IntegralPromote<unsigned short> {
    typedef CondType<USHRT_MAX <= INT_MAX, int, unsigned>::type type;
};
template<> struct IntegralPromote<int> { typedef int type; };
template<> struct IntegralPromote<unsigned> { typedef unsigned type; };
template<> struct IntegralPromote<long> { typedef long type; };
template<> struct IntegralPromote<unsigned long> { typedef unsigned long type; };
#  ifdef ARCHON_CORE_META_HAVE_LONG_LONG
template<> struct IntegralPromote<long long> { typedef long long type; };
template<> struct IntegralPromote<unsigned long long> { typedef unsigned long long type; };
#  endif
template<> struct IntegralPromote<float> { typedef float type; };
template<> struct IntegralPromote<double> { typedef double type; };
template<> struct IntegralPromote<long double> { typedef long double type; };
#endif // !ARCHON_HAVE_CXX11_DECLTYPE



/// Determine the type of the result of a binary arithmetic (or
/// bitwise) operation (+, -, *, /, %, |, &, ^). The type of the
/// result of a shift operation (<<, >>) can instead be found as the
/// type resulting from integral promotion of the left operand. The
/// type of the result of a unary arithmetic (or bitwise) operation
/// can be found as the type resulting from integral promotion of the
/// operand.
///
/// \note Enum types are supported only when the compiler supports the
/// C++11 'decltype' feature.
#ifdef ARCHON_HAVE_CXX11_DECLTYPE
template<class A, class B> struct ArithBinOpType { typedef decltype(A()+B()) type; };
#else // !ARCHON_HAVE_CXX11_DECLTYPE
template<class A, class B> struct ArithBinOpType {
private:
    typedef typename IntegralPromote<A>::type A2;
    typedef typename IntegralPromote<B>::type B2;

    typedef typename CondType<UINT_MAX <= LONG_MAX, long, unsigned long>::type type_l_u;
    typedef typename CondType<EitherTypeIs<unsigned, A2, B2>::value, type_l_u, long>::type type_l;

#  ifdef ARCHON_CORE_META_HAVE_LONG_LONG
    typedef typename CondType<UINT_MAX <= LLONG_MAX, long long, unsigned long long>::type type_ll_u;
    typedef typename CondType<ULONG_MAX <= LLONG_MAX, long long, unsigned long long>::type type_ll_ul;
    typedef typename CondType<EitherTypeIs<unsigned, A2, B2>::value, type_ll_u, long long>::type type_ll_1;
    typedef typename CondType<EitherTypeIs<unsigned long, A2, B2>::value, type_ll_ul, type_ll_1>::type type_ll;
#  endif

    typedef typename CondType<EitherTypeIs<unsigned, A2, B2>::value, unsigned, int>::type type_1;
    typedef typename CondType<EitherTypeIs<long, A2, B2>::value, type_l, type_1>::type type_2;
    typedef typename CondType<EitherTypeIs<unsigned long, A2, B2>::value, unsigned long, type_2>::type type_3;
#  ifdef ARCHON_CORE_META_HAVE_LONG_LONG
    typedef typename CondType<EitherTypeIs<long long, A2, B2>::value, type_ll, type_3>::type type_4;
    typedef typename CondType<EitherTypeIs<unsigned long long, A2, B2>::value, unsigned long long, type_4>::type type_5;
#  else
    typedef type_3 type_5;
#  endif
    typedef typename CondType<EitherTypeIs<float, A, B>::value, float, type_5>::type type_6;
    typedef typename CondType<EitherTypeIs<double, A, B>::value, double, type_6>::type type_7;

public:
    typedef typename CondType<EitherTypeIs<long double, A, B>::value, long double, type_7>::type type;
};
#endif // !ARCHON_HAVE_CXX11_DECLTYPE



template<class T> struct Wrap {
    Wrap(const T& v): m_value(v) {}
    operator T() const { return m_value; }
private:
    T m_value;
};


namespace _impl {

template<class T, bool is_signed> struct IsNegative {
    static bool __test(T value) { return value < 0; }
};
template<class T> struct IsNegative<T, false> {
    static bool __test(T) { return false; }
};

} // namespace _impl


/// This function allows you to test for a negative value in any
/// numeric type. Normally, if the type is unsigned, such a test will
/// produce a compiler warning.
template<class T> inline bool is_negative(T value)
{
    return _impl::IsNegative<T, std::numeric_limits<T>::is_signed>::__test(value);
}


} // namespace Core
} // namespace archon

#endif // ARCHON_CORE_META_HPP
