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

#include <archon/core/type_traits.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/mul_prec_int.hpp>


namespace archon::core::impl {


template<int N, bool S> struct ExtIntTypePredWrapper {
    static constexpr int required_width = N;
    static constexpr bool is_signed = S;
    static constexpr int max_parts = core::int_div_round_up(required_width, core::int_width<std::uintmax_t>());
    struct Pred {
        template<class T> static constexpr bool value =
            (core::MulPrecInt<T, max_parts, is_signed>::width >= required_width);
    };
};


template<int N> struct LeastSignedExtIntType {
    using type = core::NotVoidOr<core::least_signed_int_type<N>,
                                 core::least_unsigned_int_type_a<typename impl::ExtIntTypePredWrapper<N, true>::Pred>>;
};

template<int N> struct LeastUnsignedExtIntType {
    using type = core::NotVoidOr<core::least_unsigned_int_type<N>,
                                 core::least_unsigned_int_type_a<typename impl::ExtIntTypePredWrapper<N, false>::Pred>>;
};

template<int N> struct FastSignedExtIntType {
    using type = core::NotVoidOr<core::fast_signed_int_type<N>,
                                 core::fast_unsigned_int_type_a<typename impl::ExtIntTypePredWrapper<N, true>::Pred>>;
};

template<int N> struct FastUnsignedExtIntType {
    using type = core::NotVoidOr<core::fast_unsigned_int_type<N>,
                                 core::fast_unsigned_int_type_a<typename impl::ExtIntTypePredWrapper<N, false>::Pred>>;
};


} // namespace archon::core::impl

#endif // ARCHON_X_CORE_X_IMPL_X_EXT_INT_TYPE_HPP
