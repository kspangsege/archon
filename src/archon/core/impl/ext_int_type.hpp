// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2024 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_CORE_X_IMPL_X_EXT_INT_TYPE_HPP
#define ARCHON_X_CORE_X_IMPL_X_EXT_INT_TYPE_HPP


#include <cstdint>
#include <type_traits>
#include <algorithm>

#include <archon/core/type_traits.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/mul_prec_int.hpp>


namespace archon::core::impl {


template<int N, bool S, bool F> struct ext_int_type_finder {
    static constexpr int required_width = N;
    static constexpr bool want_signed = S;
    static constexpr bool want_fast = F;
    static constexpr int max_part_width = core::int_width<std::uintmax_t>();
    static constexpr int num_parts = std::max(core::int_div_round_up(required_width, max_part_width), 1);
    static constexpr int min_part_width = core::int_div_round_up(required_width, num_parts);
    using part_type = std::conditional_t<want_fast, core::fast_unsigned_int_type<min_part_width>,
                                         core::least_unsigned_int_type<min_part_width>>;
    using type = core::MulPrecInt<part_type, num_parts, want_signed>;
    static_assert(core::int_width<type>() >= required_width);
};



template<int N> struct LeastSignedExtIntType {
    using type = core::NotVoidOr<core::least_signed_int_type<N>,
                                 typename impl::ext_int_type_finder<N, true, false>::type>;
};

template<int N> struct LeastUnsignedExtIntType {
    using type = core::NotVoidOr<core::least_unsigned_int_type<N>,
                                 typename impl::ext_int_type_finder<N, false, false>::type>;
};

template<int N> struct FastSignedExtIntType {
    using type = core::NotVoidOr<core::fast_signed_int_type<N>,
                                 typename impl::ext_int_type_finder<N, true, true>::type>;
};

template<int N> struct FastUnsignedExtIntType {
    using type = core::NotVoidOr<core::fast_unsigned_int_type<N>,
                                 typename impl::ext_int_type_finder<N, false, true>::type>;
};


} // namespace archon::core::impl

#endif // ARCHON_X_CORE_X_IMPL_X_EXT_INT_TYPE_HPP
