// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2022 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_MATH_X_TYPE_TRAITS_HPP
#define ARCHON_X_MATH_X_TYPE_TRAITS_HPP

/// \file


#include <type_traits>
#include <utility>


namespace archon::math {


namespace impl {
template<class T, class U> struct IsCompatScalarPair;
template<class T> struct TensorOrder;
} // namespace impl


template<class T, class U = T> using scalar_arith_type = decltype(T() + U());


template<class T, class U> inline constexpr bool is_valid_scalar_pair =
    (std::is_same_v<decltype(T() + U()), math::scalar_arith_type<T, U>> &&
     std::is_same_v<decltype(T() - U()), math::scalar_arith_type<T, U>> &&
     std::is_same_v<decltype(T() * U()), math::scalar_arith_type<T, U>> &&
     std::is_same_v<decltype(T() / U()), math::scalar_arith_type<T, U>>);


template<class T> inline constexpr bool is_valid_scalar = (math::is_valid_scalar_pair<T, T> &&
                                                           std::is_same_v<decltype(-T()), decltype(T() + T())>);


template<class T, class U> inline constexpr bool is_compat_scalar_pair = impl::IsCompatScalarPair<T, U>::value;


template<class T> constexpr std::size_t tensor_order = impl::TensorOrder<T>::value;








// Implementation


namespace impl {


template<class T, class U, class = decltype(T() + U())>
constexpr auto is_compat_scalar_pair_helper(int) noexcept -> std::size_t
{
    return true;
}

template<class T, class U> constexpr auto is_compat_scalar_pair_helper(long) noexcept -> std::size_t
{
    return false;
}


template<class T, class U> struct IsCompatScalarPair {
    static constexpr bool value = impl::is_compat_scalar_pair_helper<T, U>(int());
};


template<class T, std::size_t O = T::tensor_order>
constexpr auto tensor_order_helper(int) noexcept -> std::size_t
{
    return O;
}

template<class T>
constexpr auto tensor_order_helper(long) noexcept -> std::size_t
{
    return 0;
}

template<class T> struct TensorOrder {
    static constexpr bool value = impl::tensor_order_helper<T>(int());
};


} // namespace impl
} // namespace archon::math

#endif // ARCHON_X_MATH_X_TYPE_TRAITS_HPP
