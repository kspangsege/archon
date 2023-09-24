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

#ifndef ARCHON_X_UTIL_X_COLOR_SPACE_HPP
#define ARCHON_X_UTIL_X_COLOR_SPACE_HPP

/// \file


#include <cmath>
#include <algorithm>

#include <archon/core/features.h>
#include <archon/core/utility.hpp>
#include <archon/core/math.hpp>
#include <archon/math/vector.hpp>
#include <archon/math/matrix.hpp>


namespace archon::util {


/// \{
///
/// \brief Convert between sRGB and HSL color spaces.
///
/// These functions convert between the sRGB (gamma compressed RGB) and the HSL color spaces
/// (hue, saturation, lightness). The nominal ranges for all of the involved components are
/// 0 -> 1.
///
/// \tparam T Must be a floating-point type.
///
template<class T> auto cvt_sRGB_to_HSL(const math::Vector<3, T>& rgb) noexcept -> math::Vector<3, T>;
template<class T> auto cvt_HSL_to_sRGB(const math::Vector<3, T>& hsl) noexcept -> math::Vector<3, T>;
/// \}


/// \{
///
/// \brief Convert between sRGB and HSV color spaces.
///
/// These functions convert between the sRGB (gamma compressed RGB) and the HSV color spaces
/// (hue, saturation, value). The nominal ranges for all of the involved components are 0 ->
/// 1.
///
/// \tparam T Must be a floating-point type.
///
template<class T> auto cvt_sRGB_to_HSV(const math::Vector<3, T>& rgb) noexcept -> math::Vector<3, T>;
template<class T> auto cvt_HSV_to_sRGB(const math::Vector<3, T>& hsv) noexcept -> math::Vector<3, T>;
/// \}



/// \{
///
/// \brief Convert between sRGB (gamma compressed RGB) and CIE 1931 XYZ color spaces.
///
/// The XYZ space was defined by the International Commission on Illumination (ICE), and was
/// one of the first well defined (i.e. device independent) color spaces. The definition of
/// many other device independent color spaces are based on this one.
///
/// Although the component values will normally lie in the range [0,1], sometimes they do
/// not.
///
/// `cvt_sRGB_to_XYZ()` converts a color from the sRGB color space to the CIE 1931 XYZ color
/// space.
///
/// `cvt_XYZ_to_sRGB()` converts a color from the CIE 1931 XYZ color space to the sRGB color
/// space.
///
/// \tparam T Must be a floating-point type.
///
template<class T> auto cvt_sRGB_to_XYZ(const math::Vector<3, T>& rgb) noexcept -> math::Vector<3, T>;
template<class T> auto cvt_XYZ_to_sRGB(const math::Vector<3, T>& xyz) noexcept -> math::Vector<3, T>;
/// \}



/// \{
///
/// \brief Convert between lienar RGB and linear luminance color spaces.
///
/// These function convert between the lienar RGB and the linear luminance color spaces as
/// they are they are understood in CIE 1931.
///
/// An RGB color has three components, so \p rgb must point to an array of thee elements. a
/// luminance color has only one component, so \p lum must point to that single component.
///
/// The nominal range of each component is 0 -> 1. If the input is outside this range, the
/// output may also be outside.
///
/// `cvt_RGB_to_Lum()` converts a color from the RGB color space to the luminance color
/// space. Input components must be specified in their linear form (no gamma compression,
/// linear light). Output compoments will also be expressed in their linear form.
///
/// `cvt_Lum_to_RGB()` converts a color from the luminance color space to the RGB color
/// space. Input components must be specified in their linear form (no gamma compression,
/// linear light). Output compoments will also be expressed in their linear form.
///
/// \tparam T Must be a floating-point type.
///
template<class T> auto cvt_RGB_to_Lum(const math::Vector<3, T>& rgb) noexcept -> T;
template<class T> auto cvt_Lum_to_RGB(T lum) noexcept -> math::Vector<3, T>;
/// \}




/// \brief Specification of RGB color space.
///
/// An instance of this class specifies a particular RGB color space by describing its gamut
/// in the CIE XYZ reference color space.
///
/// \sa http://en.wikipedia.org/wiki/CIE_1931_color_space
///
class CIE_RGB_PrimSpec {
public:
    /// Coordinates of the primaries and the white point within the CIE 1931 xy chromaticity
    /// diagram.
    const math::Vector2F red, green, blue, white;

    constexpr CIE_RGB_PrimSpec(math::Vector2F red, math::Vector2F green, math::Vector2F blue,
                               math::Vector2F white) noexcept;
};








// Implementation


template<class T> auto cvt_sRGB_to_HSL(const math::Vector<3, T>& rgb) noexcept -> math::Vector<3, T>
{
    T r = rgb[0], g = rgb[1], b = rgb[2];
    T min = std::min({ r, g, b });
    T max = std::max({ r, g, b });
    T h = 0, s = 0, l = T((max + min) / 2);
    auto d = max - min;
    auto e = (l > 0.5 ? 2 - (max + min) : max + min);
    if (ARCHON_LIKELY(e != 0))
        s = T(d / e);
    if (ARCHON_LIKELY(d != 0)) {
        if (max == r) {
            h = T(((g - b) / d + (g < b ? 6 : 0)) / 6);
        }
        else if (max == g) {
            h = T(((b - r) / d + 2) / 6);
        }
        else {
            h = T(((r - g) / d + 4) / 6);
        }
    }
    return { h, s, l };
}


template<class T> auto cvt_HSL_to_sRGB(const math::Vector<3, T>& hsl) noexcept -> math::Vector<3, T>
{
    auto h = core::periodic_mod(hsl[0] * 360, T(360));
    auto s = hsl[1];
    auto l = hsl[2];

    auto f = [&](T n) noexcept {
        auto k = std::fmod(n + h / 30, 12);
        auto a = s * core::hetero_min(l, 1 - l);
        return T(l - a * core::hetero_max(-1, core::hetero_min(k - 3, 9 - k, 1)));
    };

    T r = f(0);
    T g = f(8);
    T b = f(4);
    return { r, g, b };
}


template<class T> auto cvt_sRGB_to_HSV(const math::Vector<3, T>& rgb) noexcept -> math::Vector<3, T>
{
    T r = rgb[0], g = rgb[1], b = rgb[2];
    T min = std::min({ r, g, b });
    T max = std::max({ r, g, b });
    T h = 0, s = 0, v = max;
    auto d = max - min;
    if (ARCHON_LIKELY(v != 0))
        s = T(d / v);
    if (ARCHON_LIKELY(d != 0)) {
        if (max == r) {
            h = T(((g - b) / d + (g < b ? 6 : 0)) / 6);
        }
        else if (max == g) {
            h = T(((b - r) / d + 2) / 6);
        }
        else {
            h = T(((r - g) / d + 4) / 6);
        }
    }
    return { h, s, v };
}


template<class T> auto cvt_HSV_to_sRGB(const math::Vector<3, T>& hsv) noexcept -> math::Vector<3, T>
{
    auto h = core::periodic_mod(hsv[0] * 360, T(360));
    auto s = hsv[1];
    auto v = hsv[2];

    auto f = [&](T n) {
        auto k = std::fmod(n + h / 60, 6);
        return T(v - v * s * core::hetero_max(0, core::hetero_min(k, 4 - k, 1)));
    };

    T r = f(5);
    T g = f(3);
    T b = f(1);
    return { r, g, b };
}


constexpr CIE_RGB_PrimSpec::CIE_RGB_PrimSpec(math::Vector2F r, math::Vector2F g, math::Vector2F b,
                                             math::Vector2F w) noexcept :
    red(r),
    green(g),
    blue(b),
    white(w)
{
}


namespace impl {


// See https://en.wikipedia.org/wiki/SRGB
constexpr util::CIE_RGB_PrimSpec g_srgb_prim_spec(math::Vector2F(0.6400F, 0.3300F),  // Red
                                                  math::Vector2F(0.3000F, 0.6000F),  // Green
                                                  math::Vector2F(0.1500F, 0.0600F),  // Blue
                                                  math::Vector2F(0.3127F, 0.3290F)); // White


// See https://en.wikipedia.org/wiki/SRGB
// See http://www.brucelindbloom.com/index.html?Eqn_DeltaE_CMC.html.
template<class T> class sRGB {
public:
    static constexpr T a = 0.055F;
    static constexpr T gamma = 2.4F;
    static constexpr T k_0 = 0.040448236276987F;

    static auto get_phi() noexcept -> T
    {
        // Approx 12.92
        return k_0 / std::pow((k_0 + a) / (1 + a), gamma);
    }

    static auto gamma_enc(T v) noexcept -> T
    {
        return (k_0 / get_phi() < v ? (1 + a) * std::pow(v, 1 / gamma) - a : get_phi() * v);
    }

    static auto gamma_dec(T v) noexcept -> T
    {
        return (k_0 < v ? std::pow((v + a) / (1 + a), gamma) : v / get_phi());
    }

    static constexpr auto get_white() noexcept -> math::Vector<3, T>
    {
        constexpr math::Vector2F w = impl::g_srgb_prim_spec.white;
        return math::Vector<3, T>(T(w[0]) / w[1], 1, (1 - (T(w[0]) + w[1])) / w[1]);
    }

    static constexpr auto get_to_xyz() noexcept -> math::Matrix<3, 3, T>
    {
        constexpr math::Vector2F r = impl::g_srgb_prim_spec.red;
        constexpr math::Vector2F g = impl::g_srgb_prim_spec.green;
        constexpr math::Vector2F b = impl::g_srgb_prim_spec.blue;

        math::Matrix<3, 3, T> mat;
        mat[0] = math::Vector<3, T>(r[0], g[0], b[0]);
        mat[1] = math::Vector<3, T>(r[1], g[1], b[1]);
        mat[2] = math::Vector<3, T>(1) - (mat[0] + mat[1]);

        math::Vector scales = math::inv(mat) * white;
        for (int i = 0; i< 3; ++i)
            mat.set_col(i, scales[i] * mat.get_col(i));
        return mat;
    }

    static constexpr math::Vector<3, T> white = get_white();
    static constexpr math::Matrix<3, 3, T> to_xyz = get_to_xyz();
    static constexpr math::Matrix<3, 3, T> fr_xyz = math::inv(to_xyz);
};


template<class T> inline auto cvt_sRGB_to_RGB(const math::Vector<3, T>& rgb) noexcept -> math::Vector<3, T>
{
    // FIXME: Should the standardized simplified sRGB gamma curve be used instead?                                                 
    math::Vector<3, T> lin;
    for (int i = 0; i < 3; ++i)
        lin[i] = impl::sRGB<T>::gamma_dec(rgb[i]);
    return lin;
}


template<class T> inline auto cvt_RGB_to_sRGB(const math::Vector<3, T>& lin) noexcept -> math::Vector<3, T>
{
    // FIXME: Should the standardized simplified sRGB gamma curve be used instead?                                                 
    math::Vector<3, T> rgb;
    for (int i = 0; i < 3; ++i)
        rgb[i] = impl::sRGB<T>::gamma_enc(lin[i]);
    return rgb;
}


template<class T> inline auto cvt_RGB_to_XYZ(const math::Vector<3, T>& lin) noexcept -> math::Vector<3, T>
{
    return impl::sRGB<T>::to_xyz * lin;
}


template<class T> inline auto cvt_XYZ_to_RGB(const math::Vector<3, T>& xyz) noexcept -> math::Vector<3, T>
{
    return impl::sRGB<T>::fr_xyz * xyz;
}


} // namespace impl


template<class T> inline auto cvt_sRGB_to_XYZ(const math::Vector<3, T>& rgb) noexcept -> math::Vector<3, T>
{
    return impl::cvt_RGB_to_XYZ(impl::cvt_sRGB_to_RGB(rgb));
}


template<class T> inline auto cvt_XYZ_to_sRGB(const math::Vector<3, T>& xyz) noexcept -> math::Vector<3, T>
{
    return impl::cvt_RGB_to_sRGB(impl::cvt_XYZ_to_RGB(xyz));
}


template<class T> inline auto cvt_RGB_to_Lum(const math::Vector<3, T>& rgb) noexcept -> T
{
    T r = rgb[0], g = rgb[1], b = rgb[2];
    return T(0.2126 * r + 0.7152 * g + 0.0722 * b);
}


template<class T> inline auto cvt_Lum_to_RGB(T lum) noexcept -> math::Vector<3, T>
{
    T r = lum;
    T g = lum;
    T b = lum;
    return { r, g, b };
}


} // namespace archon::util

#endif // ARCHON_X_UTIL_X_COLOR_SPACE_HPP
