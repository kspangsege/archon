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

#ifndef ARCHON_X_CORE_X_MATH_HPP
#define ARCHON_X_CORE_X_MATH_HPP

/// \file


#include <cmath>
#include <type_traits>
#include <numbers>

#include <archon/core/integer.hpp>


namespace archon::core {


/// \brief Absolute value.
///
/// This function is a `constexpr` enabled alternative implementation of `std::abs()`. This
/// implementation does not change `-0` to `0`, and does not necessarily change `-nan` to
/// `nan`.
///
/// FIXME: In C++23, `std::abs()` becomes `constexpr`, so the need for this function
/// vanishes.
///
template<class T> constexpr auto abs(T) noexcept -> T;



/// \brief Perform periodic modulo operation.
///
/// This function implements a modulo operation where the returned remainder always has the
/// same sign as the divisor (\p b), or is zero. In particular, if the divisor is positive,
/// which it ordinarily will be, the returned remainder is always non-negative.
///
/// Note also that this function is periodic with respect to the dividend, but neither
/// "even" nor "odd" with respect to the dividend. The standard library function
/// 'std::fmod()', on the other hand, is an "odd" function with respect to the dividend, and
/// not periodic with respect to the dividend.
///
/// This function works for both integer and floating point types. Either both types have to
/// conform to the integer concept (\ref Concept_Archon_Core_Integer), or both types must be
/// either standard integer or standard floating-point types (`std::is_integral` or
/// `std::is_floating_point`). When both types conform to the integer concept (\ref
/// core::is_integer), this function has the same effect as `core::int_periodic_mod(a, b)`
/// (see \ref core::int_periodic_mod()).
///
/// FIXME: Add test case for this function (only the floating-point part needs testing here).                    
///
template<class T, class U> auto periodic_mod(T a, U b) noexcept -> U;



/// \brief Multiply value by itself.
///
/// This function returns the result of multiplying the specified value by itself. This
/// function works for all numeric types with a suitable multiplication operator.
///
template<class T> constexpr auto square(T val) noexcept;



/// \brief Interpolate linearly between values.
///
/// This function is a shorthand for `(1 - t) * a + t * b`.
///
template<class T, class U> auto lerp(T a, T b, U t) noexcept;



/// \brief Convert angle from degrees to radians.
///
/// This function converts an angle from the specified number of degrees to the
/// corresponding number of radians. The return type will be \p T if \p T is one of the
/// standard floating point types (`std::is_floating_point`). Otherwise it will be
/// `decltype(T() + double())`. For example, `deg_to_rad(90)` produces `core::pi<double> /
/// 2`.
///
template<class T> auto deg_to_rad(T) noexcept;



/// \brief Ratio of circle's circumference to its diameter.
///
/// This is the mathematical constant that is the ratio of a circle's circumference to its
/// diameter.
///
template<class T> constexpr T pi = std::numbers::pi_v<T>;



/// \{
///
/// \brief The golden ratio and its friends.
///
/// These are the golden ratio and its friends.
///
template<class T> constexpr T golden_ratio = std::numbers::phi_v<T>;
template<class T> constexpr T golden_fraction = T(1 - 1 / core::golden_ratio<T>);
template<class T> constexpr T golden_angle = T(2 * core::pi<T> * core::golden_fraction<T>);
/// \}








// Implementation


template<class T> constexpr auto abs(T x) noexcept -> T
{
    return (x < 0 ? -x : x);
}


namespace impl {


template<class T> inline auto periodic_mod(T a, T b) noexcept -> T
{
    static_assert(std::is_floating_point_v<T>);
    T c = std::fmod(a, b);
    if (ARCHON_LIKELY(b >= 0)) {
        if (ARCHON_LIKELY(c >= 0))
            return c;
        c += b;
        // Round-off error could make `c` equal to `b`
        if (ARCHON_LIKELY(c < b))
            return c;
    }
    else {
        if (ARCHON_LIKELY(c <= 0))
            return c;
        c += b;
        // Round-off error could make `c` equal to `b`
        if (ARCHON_LIKELY(c > b))
            return c;
    }
    return 0;
}


} // namespace impl


template<class T, class U> inline auto periodic_mod(T a, U b) noexcept -> U
{
    if constexpr (core::is_integer<T>() && core::is_integer<U>()) {
        return core::int_periodic_mod(a, b);
    }
    else {
        static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>);
        static_assert(std::is_integral_v<U> || std::is_floating_point_v<U>);
        using type = std::common_type_t<T, U>;
        return U(impl::periodic_mod(type(a), type(b)));
    }
}


template<class T> constexpr auto square(T val) noexcept
{
    static_assert(noexcept(val * val));
    return val * val;
}


template<class T, class U> auto lerp(T a, T b, U t) noexcept
{
    static_assert(noexcept((1 - t) * a + t * b));
    return (1 - t) * a + t * b;
}


template<class T> auto deg_to_rad(T angle) noexcept
{
    using type = std::conditional_t<std::is_floating_point_v<T>, T, double>;
    return core::pi<type> / 180 * angle;
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_MATH_HPP
