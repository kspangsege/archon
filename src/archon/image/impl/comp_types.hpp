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

#ifndef ARCHON_X_IMAGE_X_IMPL_X_COMP_TYPES_HPP
#define ARCHON_X_IMAGE_X_IMPL_X_COMP_TYPES_HPP


#include <type_traits>
#include <limits>
#include <algorithm>

#include <archon/core/integer.hpp>


namespace archon::image::impl {


template<class T> constexpr int get_bit_width() noexcept;








// Implementation


template<class T> constexpr int get_bit_width() noexcept
{
    if constexpr (core::is_integer<T>()) {
        return std::min(core::int_inner_width<T>(), core::int_inner_width<core::unsigned_type<T>>());
    }
    else {
        static_assert(std::is_floating_point_v<T>);
        using lim_type = std::numeric_limits<T>;
        static_assert(lim_type::is_specialized);
        return (lim_type::digits + core::int_find_msb_pos(unsigned(lim_type::max_exponent) -
                                                          unsigned(lim_type::min_exponent)) + 1);
    }
}


} // namespace archon::image::impl

#endif // ARCHON_X_IMAGE_X_IMPL_X_COMP_TYPES_HPP
