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

#include <archon/core/utility.hpp>
#include <archon/core/math.hpp>
#include <archon/math/vec.hpp>
#include <archon/math/mat_ops.hpp>
#include <archon/math/mat.hpp>


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
template<class T> void cvt_sRGB_to_HSL(const T* rgb, T* hsl);
template<class T> void cvt_HSL_to_sRGB(const T* hsl, T* rgb);
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
template<class T> void cvt_sRGB_to_HSV(const T* rgb, T* hsv);
template<class T> void cvt_HSV_to_sRGB(const T* hsv, T* rgb);
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
template<class T> void cvt_sRGB_to_XYZ(const T* rgb, T* xyz);
template<class T> void cvt_XYZ_to_sRGB(const T* xyz, T* rgb);
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
template<class T> void cvt_RGB_to_Lum(const T* rgb, T* lum);
template<class T> void cvt_Lum_to_RGB(const T* lum, T* rgb);
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
    const math::Vec2F red, green, blue, white;

    constexpr CIE_RGB_PrimSpec(math::Vec2F red, math::Vec2F green, math::Vec2F blue, math::Vec2F white) noexcept;
};








// Implementation


template<class T> void cvt_sRGB_to_HSL(const T* rgb, T* hsl)
{
    T r = rgb[0], g = rgb[1], b = rgb[2];
    T min = std::min({ r, g, b });
    T max = std::max({ r, g, b });
    T h = 0, s = 0, l = T((max + min) / 2); // Throws
    auto d = max - min; // Throws
    auto e = (l > 0.5 ? 2 - (max + min) : max + min); // Throws
    if (ARCHON_LIKELY(e != 0))
        s = T(d / e); // Throws
    if (ARCHON_LIKELY(d != 0)) {
        if (max == r) {
            h = T(((g - b) / d + (g < b ? 6 : 0)) / 6); // Throws
        }
        else if (max == g) {
            h = T(((b - r) / d + 2) / 6); // Throws
        }
        else {
            h = T(((r - g) / d + 4) / 6); // Throws
        }
    }
    hsl[0] = h;
    hsl[1] = s;
    hsl[2] = l;
}


template<class T> void cvt_HSL_to_sRGB(const T* hsl, T* rgb)
{
    auto hue   = core::periodic_mod(hsl[0] * 360, T(360)); // Throws
    auto sat   = hsl[1];
    auto light = hsl[2];

    auto f = [=](T n) {
        auto k = std::fmod(n + hue / 30, 12); // Throws
        auto a = sat * core::hetero_min(light, 1 - light); // Throws
        return T(light - a * core::hetero_max(-1, core::hetero_min(k - 3, 9 - k, 1))); // Throws
    };

    rgb[0] = f(0); // Throws
    rgb[1] = f(8); // Throws
    rgb[2] = f(4); // Throws
}


template<class T> void cvt_sRGB_to_HSV(const T* rgb, T* hsv)
{
    T r = rgb[0], g = rgb[1], b = rgb[2];
    T min = std::min({ r, g, b });
    T max = std::max({ r, g, b });
    T h = 0, s = 0, v = max;
    auto d = max - min; // Throws
    if (ARCHON_LIKELY(v != 0))
        s = T(d / v); // Throws
    if (ARCHON_LIKELY(d != 0)) {
        if (max == r) {
            h = T(((g - b) / d + (g < b ? 6 : 0)) / 6); // Throws
        }
        else if (max == g) {
            h = T(((b - r) / d + 2) / 6); // Throws
        }
        else {
            h = T(((r - g) / d + 4) / 6); // Throws
        }
    }
    hsv[0] = h;
    hsv[1] = s;
    hsv[2] = v;
}


template<class T> void cvt_HSV_to_sRGB(const T* hsv, T* rgb)
{
    auto hue = core::periodic_mod(hsv[0] * 360, T(360)); // Throws
    auto sat = hsv[1];
    auto val = hsv[2];

    auto f = [=](T n) {
        auto k = std::fmod(n + hue / 60, 6); // Throws
        return val - val * sat * core::hetero_max(0, core::hetero_min(k, 4 - k, 1)); // Throws
    };

    rgb[0] = f(5); // Throws
    rgb[1] = f(3); // Throws
    rgb[2] = f(1); // Throws
}


constexpr CIE_RGB_PrimSpec::CIE_RGB_PrimSpec(math::Vec2F r, math::Vec2F g, math::Vec2F b, math::Vec2F w) noexcept :
    red(r),
    green(g),
    blue(b),
    white(w)
{
}


namespace impl {


// See https://en.wikipedia.org/wiki/SRGB
constexpr util::CIE_RGB_PrimSpec g_srgb_prim_spec(math::Vec2F(0.6400F, 0.3300F),  // Red
                                                  math::Vec2F(0.3000F, 0.6000F),  // Green
                                                  math::Vec2F(0.1500F, 0.0600F),  // Blue
                                                  math::Vec2F(0.3127F, 0.3290F)); // White


// See https://en.wikipedia.org/wiki/SRGB
// See http://www.brucelindbloom.com/index.html?Eqn_DeltaE_CMC.html.
template<class T> class sRGB {
public:
    static constexpr T a = 0.055F;
    static constexpr T gamma = 2.4F;
    static constexpr T k_0 = 0.040448236276987F;

    static T get_phi()
    {
        // Approx 12.92
        return k_0 / std::pow((k_0 + a) / (1 + a), gamma); // Throws
    }

    static T gamma_enc(T v)
    {
        return (k_0 / get_phi() < v ? (1 + a) * std::pow(v, 1 / gamma) - a : get_phi() * v); // Throws
    }

    static T gamma_dec(T v)
    {
        return (k_0 < v ? std::pow((v + a) / (1 + a), gamma) : v / get_phi()); // Throws
    }

    static constexpr math::Vec<3, T> get_white()
    {
        constexpr math::Vec2F w = impl::g_srgb_prim_spec.white;
        return math::Vec<3, T>(T(w[0]) / w[1], 1, (1 - (T(w[0]) + w[1])) / w[1]); // Throws
    }

    static constexpr math::Mat<3, 3, T> get_to_xyz()
    {
        constexpr math::Vec2F r = impl::g_srgb_prim_spec.red;
        constexpr math::Vec2F g = impl::g_srgb_prim_spec.green;
        constexpr math::Vec2F b = impl::g_srgb_prim_spec.blue;

        math::Mat<3, 3, T> mat;
        mat.row(0) = math::Vec<3, T>(r[0], g[0], b[0]);
        mat.row(1) = math::Vec<3, T>(r[1], g[1], b[1]);
        mat.row(2) = math::Vec<3, T>(1) - (mat.row(0) + mat.row(1)); // Throws

        mat.scale_cols(math::inv(mat) * white); // Throws
        return mat;
    }

    static constexpr math::Vec<3, T> white = get_white(); // Throws
    static constexpr math::Mat<3, 3, T> to_xyz = get_to_xyz(); // Throws
    static constexpr math::Mat<3, 3, T> fr_xyz = math::inv(to_xyz); // Throws
};


template<class T> inline void cvt_sRGB_to_RGB(const T* rgb, T* lin)
{
    // FIXME: Should the standardized simplified sRGB gamma curve be used instead?                                                 
    std::transform(rgb, rgb + 3, lin, [](T v) {
        return impl::sRGB<T>::gamma_dec(v); // Throws
    }); // Throws
}


template<class T> inline void cvt_RGB_to_sRGB(const T* lin, T* rgb)
{
    // FIXME: Should the standardized simplified sRGB gamma curve be used instead?                                                 
    std::transform(lin, lin + 3, rgb, [](T v) {
        return impl::sRGB<T>::gamma_enc(v); // Throws
    }); // Throws
}


template<class T> inline void cvt_RGB_to_XYZ(const T* lin, T* xyz)
{
    math::vec3_adapt(xyz) = impl::sRGB<T>::to_xyz * math::vec3_adapt(lin); // Throws
}


template<class T> inline void cvt_XYZ_to_RGB(const T* xyz, T* lin)
{
    math::vec3_adapt(lin) = impl::sRGB<T>::fr_xyz * math::vec3_adapt(xyz); // Throws
}


} // namespace impl


template<class T> inline void cvt_sRGB_to_XYZ(const T* rgb, T* xyz)
{
    T lin[3];
    impl::cvt_sRGB_to_RGB(rgb, lin); // Throws
    impl::cvt_RGB_to_XYZ(lin, xyz); // Throws
}


template<class T> inline void cvt_XYZ_to_sRGB(const T* xyz, T* rgb)
{
    T lin[3];
    impl::cvt_XYZ_to_RGB(xyz, lin); // Throws
    impl::cvt_RGB_to_sRGB(lin, rgb); // Throws
}


template<class T> inline void cvt_RGB_to_Lum(const T* rgb, T* lum)
{
    T r = rgb[0], g = rgb[1], b = rgb[2];
    lum[0] = T(0.2126 * r + 0.7152 * g + 0.0722 * b); // Throws
}


template<class T> inline void cvt_Lum_to_RGB(const T* lum, T* rgb)
{
    rgb[0] = lum[0];
    rgb[1] = lum[0];
    rgb[2] = lum[0];
}


} // namespace archon::util

#endif // ARCHON_X_UTIL_X_COLOR_SPACE_HPP
