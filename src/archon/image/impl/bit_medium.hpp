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

#ifndef ARCHON_X_IMAGE_X_IMPL_X_BIT_MEDIUM_HPP
#define ARCHON_X_IMAGE_X_IMPL_X_BIT_MEDIUM_HPP


#include <type_traits>
#include <utility>

#include <archon/core/integer.hpp>


namespace archon::image::impl {


template<class T, int N> constexpr bool is_bit_medium_of_width() noexcept;


template<class T, int N> struct UnpackedType {
    static_assert(impl::is_bit_medium_of_width<T, N>());
    using type_1 = core::promoted_type<T>;
    using type_2 = core::unsigned_type<type_1>;
    using type = std::conditional_t<core::num_value_bits<type_1>() >= N, type_1, type_2>;
};








// Implementation


template<class T, int N> constexpr bool is_bit_medium_of_width() noexcept
{
    static_assert(N >= 0);
    if constexpr (core::is_integer<T>()) {
        if constexpr (core::int_inner_width<T>() >= N && core::int_inner_width<core::unsigned_type<T>>() >= N) {
            return true;
        }
    }
    return false;
}


} // namespace archon::image::impl

#endif // ARCHON_X_IMAGE_X_IMPL_X_BIT_MEDIUM_HPP
