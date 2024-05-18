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

#ifndef ARCHON_X_IMAGE_X_GAMMA_HPP
#define ARCHON_X_IMAGE_X_GAMMA_HPP

/// \file


#include <cmath>

#include <archon/core/features.h>
#include <archon/image/comp_types.hpp>


namespace archon::image {


/// \brief Recover component value from gamma-compressed integer representation.
///
/// This function recovers a component value from its gamma-compressed integer
/// representation form. Gamma decompression occurs as in sRGB. The specified integer value
/// is first converted to floating point form as if by \ref image::int_to_float(), and the
/// result is then gamma decompressed.
///
template<int N, class T> auto compressed_int_to_float(T) noexcept -> image::float_type;


/// \brief Convert component value to gamma-compressed integer representation.
///
/// This function converts the specified component value to a gamma-compressed integer
/// representation. Gamma compression occurs as in sRGB. After gamma compression, the
/// resulting value is converted to integer representation as if by \ref
/// image::float_to_int().
///
template<class T, int N> auto float_to_compressed_int(image::float_type) noexcept -> T;


/// \{
///
/// \brief sRGB-style "gamma" compression and expansion.
///
/// These functions perform sRGB-style "gamma" compression and expansion repsectively.
///
/// \tparam One of the standard floating-point types.
///
template<class T> auto gamma_compress(T val) noexcept -> T;
template<class T> auto gamma_expand(T val) noexcept -> T;
/// \}








// Implementation


namespace impl {


struct GammaDecompressTable8 {
    image::float_type table[256] = {};

    GammaDecompressTable8() noexcept
    {
        for (int i = 0; i < 256; ++i) {
            double val = image::int_to_float<8, double>(i);
            table[i] = image::float_type(image::gamma_expand(val));
        }
    }
};

inline GammaDecompressTable8 g_gamma_decompress_table_8 = {};


} // namespace impl


template<int N, class T> inline auto compressed_int_to_float(T val) noexcept -> image::float_type
{
    if constexpr (N == 1) {
        return image::int_to_float<1, image::float_type>(val);
    }
    else if constexpr (N <= 8) {
        int val_2 = image::int_to_int<N, int, 8>(val);
        return impl::g_gamma_decompress_table_8.table[val_2 & 255];
    }
    else {
        // FIXME: Must find more efficient way to do this. Maybe just lookup the two adjacent values from the 8-bit table, and then interpolate between them.                                                            
        double val_2 = image::int_to_float<N, double>(val);
        return image::float_type(image::gamma_expand(val_2));
    }
}


template<class T, int N> inline auto float_to_compressed_int(image::float_type val) noexcept -> T
{
    // FIXME: Must find more efficient way to do this.                                         
    return image::float_to_int<T, N>(image::gamma_compress(double(val)));
}


template<class T> inline auto gamma_compress(T val) noexcept -> T
{
    using type = T;
    static_assert(std::is_floating_point_v<type>);
    if (ARCHON_LIKELY(val > 0.0031308))
        return type(1.055 * std::pow(val, type(1) / 2.4) - 0.055);
    return type(12.92 * val);
}


template<class T> inline auto gamma_expand(T val) noexcept -> T
{
    using type = T;
    static_assert(std::is_floating_point_v<type>);
    if (ARCHON_LIKELY(val > 0.04045))
        return type(std::pow((val + 0.055) / 1.055, 2.4));
    return type(val / 12.92);
}


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_GAMMA_HPP
