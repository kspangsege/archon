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

#ifndef ARCHON_X_IMAGE_X_TEST_X_COMP_REPR_UTILS_HPP
#define ARCHON_X_IMAGE_X_TEST_X_COMP_REPR_UTILS_HPP

/// \file


#include <type_traits>
#include <string_view>

#include <archon/core/random.hpp>
#include <archon/image/comp_types.hpp>
#include <archon/image/comp_repr.hpp>


namespace archon::image::test {


template<image::CompRepr R> struct CompReprTag {
    static constexpr image::CompRepr comp_repr = R;
};


auto get_comp_repr_name(image::CompRepr repr) -> std::string_view;


template<class T, int N, class E> auto rand_int_comp(E& engine) -> T;

template<image::CompRepr R, class E> auto rand_comp(E& engine) -> image::comp_type<R>;








// Implementation


template<class T, int N, class E> inline auto rand_int_comp(E& engine) -> T
{
    using comp_type = T;
    constexpr int bit_width = N;
    using unpacked_type = image::unpacked_type<comp_type, bit_width>;
    unpacked_type value = core::rand_int_bits<unpacked_type>(engine, bit_width); // Throws
    return image::pack_int<comp_type, bit_width>(value);
}


template<image::CompRepr R, class E> inline auto rand_comp(E& engine) -> image::comp_type<R>
{
    constexpr image::CompRepr comp_repr = R;
    using comp_type = image::comp_type<comp_repr>;
    if constexpr (std::is_integral_v<comp_type>) {
        constexpr int bit_width = image::comp_repr_bit_width<comp_repr>();
        return test::rand_int_comp<comp_type, bit_width>(engine); // Throws
    }
    else {
        static_assert(std::is_floating_point_v<comp_type>);
        return core::rand_float<comp_type>(engine); // Throws
    }
}


} // namespace archon::image::test

#endif // ARCHON_X_IMAGE_X_TEST_X_COMP_REPR_UTILS_HPP
