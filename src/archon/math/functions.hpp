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

/// \file
///
/// \author Kristian Spangsege
///
/// Defines a number of fundamental mathematical functions. Some of the
/// standard library functions are reproduced in a template form such
/// that the name is independant of argument type.

#ifndef ARCHON_MATH_FUNCTIONS_HPP
#define ARCHON_MATH_FUNCTIONS_HPP

#include <cmath>
#include <cstdlib>
#include <functional>
#include <algorithm>


namespace archon {
namespace math {

/// Square the argument.
template<class T> inline constexpr T square(T v)
{
    return v*v;
}

/// Raise the argument to the power of three.
template<class T> inline constexpr T cube(T v)
{
    return v*v*v;
}

/// Length of 2-D vector
template<class T> inline constexpr T pol_len(T x, T y)
{
    return std::sqrt(square(x) + square(y));
}

/// Angle of 2-D vector in range <tt>[-pi;pi]</tt>.
///
/// <pre>
///
///   pol_ang( 1,  0) = 0
///   pol_ang( 1,  1) = pi/4
///   pol_ang( 0,  1) = pi/2
///   pol_ang(-1,  0) = pi
///   pol_ang(-1, -0) = -pi
///   pol_ang(-1, -1) = 3/2 pi
///
/// </pre>
///
/// \sa std::atan2
template<class T> inline constexpr T pol_ang(T x, T y)
{
    return std::atan2(y,x);
}

/// Linear interpolation (and extrapolation).
///
/// Choose and return a value y such that the point (x,y) lies on
/// the line spanned by (x1,y1) and (x2,y2).
template<class T>
inline constexpr T lin_interp(double x, double x1, double x2, T y1, T y2)
{
    return T(y1 + (x-x1)/(x2-x1)*(y2-y1));
}


// Function objects

template<class T> struct Sq: std::unary_function<T,T> {
    T operator()(T v) const
    {
        return math::square(v);
    }
};

template<class T> struct Sqrt: std::unary_function<T,T> {
    T operator()(T v) const
    {
        return std::sqrt(v);
    }
};

template<class T> struct SqDiff: std::binary_function<T,T,T> {
    T operator()(T v, T w) const
    {
        return math::square(w - v);
    }
};

template<class T> struct AddAlpha: std::binary_function<T,T,T> {
    T operator()(T v, T w) const
    {
        return v + a*w;
    }
    AddAlpha(T a):
        a(a)
    {
    }
    T a;
};

template<class T> struct SubAlpha: std::binary_function<T,T,T> {
    T operator()(T v, T w) const
    {
        return v - a*w;
    }
    SubAlpha(T a):
        a(a)
    {
    }
    T a;
};

} // namespace math
} // namespace archon

#endif // ARCHON_MATH_FUNCTIONS_HPP
