// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2025 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_UTIL_X_SRGB_GAMMA_HPP
#define ARCHON_X_UTIL_X_SRGB_GAMMA_HPP

/// \file


#include <cmath>
#include <type_traits>

#include <archon/core/features.hpp>


namespace archon::util {


/// \{
///
/// \brief sRGB-style "gamma" compression and expansion.
///
/// These functions perform sRGB-style "gamma" compression and expansion repsectively.
///
/// \tparam One of the standard floating-point types.
///
/// FIXME: Make these `constexpr` when switching to C++26.
///
template<class T> auto srgb_gamma_compress(T val) noexcept -> T;
template<class T> auto srgb_gamma_expand(T val) noexcept -> T;
/// \}








// Implementation


template<class T> inline auto srgb_gamma_compress(T val) noexcept -> T
{
    using type = T;
    static_assert(std::is_floating_point_v<type>);
    if (ARCHON_LIKELY(val > 0.0031308))
        return type(1.055 * std::pow(val, type(1) / 2.4) - 0.055);
    return type(12.92 * val);
}


template<class T> inline auto srgb_gamma_expand(T val) noexcept -> T
{
    using type = T;
    static_assert(std::is_floating_point_v<type>);
    if (ARCHON_LIKELY(val > 0.04045))
        return type(std::pow((val + 0.055) / 1.055, 2.4));
    return type(val / 12.92);
}


} // namespace archon::util

#endif // ARCHON_X_UTIL_X_SRGB_GAMMA_HPP
