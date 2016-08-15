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

#ifndef ARCHON_UTIL_COLOR_HPP
#define ARCHON_UTIL_COLOR_HPP

#include <cmath>
#include <algorithm>
#include <functional>
#include <locale>
#include <string>
#include <ios>

#include <archon/math/functions.hpp>
#include <archon/math/vector_adapt.hpp>
#include <archon/math/matrix.hpp>


namespace archon {
namespace util {

/// Defines a number of functions related to color space conversions.
///
/// All references to the RGB color space below are to be understood as
/// references to the device independent "sRGB" color space.
///
/// All color spaces mentioned below, are to be understood as device
/// independent. That is, each color space has a fixed/closed relationship with
/// the sRGB color space.
///
/// \sa http://www.rags-int-inc.com/PhotoTechStuff/DigitalFilm/ColorSpaces.html
/// \sa http://www.w3.org/Graphics/Color/sRGB
namespace Color {

/// This class defines a specific RGB color space by describing its gamut in the
/// CIE XYZ reference color space.
///
/// \sa http://en.wikipedia.org/wiki/CIE_1931_color_space
class CIE_RGB_PrimSpec {
public:
    /// Coordinates of the primaries and the white point within the CIE 1931 xy
    /// chromaticity diagram.
    const math::Vec2F red, green, blue, white;

    CIE_RGB_PrimSpec(math::Vec2F red, math::Vec2F green, math::Vec2F blue, math::Vec2F white):
        red(red),
        green(green),
        blue(blue),
        white(white)
    {
    }
};

/// <tt>R'G'B' -> Y'</tt>
///
/// Convert a gamma compressed RGB triplet to a "luma" value as defined by the
/// ITU-R Recommendation BT.709.
///
/// \sa http://en.wikipedia.org/wiki/Luma_(video)
///
/// http://www.itu.int/rec/R-REC-BT.709
template<class T> inline T cvt_RGB_to_Lum(const T* rgb)
{
    T kr = 0.2126;
    T kb = 0.0722;
    return kr*rgb[0] + (1-kr-kb)*rgb[1] + kb*rgb[2];
}

template<class T> inline void cvt_Lum_to_RGB(T lum, T* rgb)
{
    rgb[0] = rgb[1] = rgb[2] = lum;
}

template<class T> inline T cvt_RGB_To_Lum(const math::BasicVec<3,T>& rgb)
{
    return cvt_RGB_to_Lum(rgb.get());
}

template<class T> inline math::BasicVec<3,T> cvt_Lum_to_RGB(T lum)
{
    math::BasicVec<3,T> rgb;
    cvt_Lum_to_RGB(lum, rgb.get());
    return rgb;
}

/// Convert a color from the sRGB color space to the CIE 1931 XYZ color
/// space. The XYZ space was defined by the International Commission on
/// Illumination, and was one of the first well defined (i.e. device
/// independent) color spaces. The definition of many other device independent
/// color spaces are based on this one.
///
/// Although the component values will normally lie in the range [0,1],
/// sometimes they do not.
template<class T> void cvt_RGB_to_XYZ(const T* rgb, T* xyz);

template<class T> void cvt_XYZ_to_RGB(const T* xyz, T* rgb);

template<class T> void cvt_RGB_to_LAB(const T* rgb, T* lab);

template<class T> void cvt_LAB_to_RGB(const T* lab, T* rgb);

/// Convert an RGB color triplet to the alternative color space HSV (Hue,
/// Saturation, Value). Note that it is not required that the components of the
/// RGB triplet are less than or equal to 1, but they may not be negative.
///
/// \param rgb An RGB triplet with non-negative components.
///
/// \param hsv Hue in [0,1), saturation in [0, 1] and positive value. When the
/// saturation is 0 the hue is arbitrary/undefined (achromatici case). Value
/// will be less than or equal to 1 if each of the components of the RGB triplet
/// is less than or equal to one 1.
///
/// \sa http://en.wikipedia.org/wiki/HSV_color_space
/// \sa Computer Graphics - Foley, vanDam, Feiner, Hughes (1990)
template<class T> inline void cvt_RGB_to_HSV(const T* rgb, T* hsv)
{
    T v = std::max(std::max(rgb[0], rgb[1]), rgb[2]);
    T d = v - std::min(std::min(rgb[0], rgb[1]), rgb[2]);

    if (d == 0) {
        // Achromatic
        hsv[0] = 0;
        hsv[1] = 0;
        hsv[2] = v;
        return;
    }

    T h =
        (v == rgb[0] ? (rgb[1] - rgb[2]) / d :
         v == rgb[1] ? 2 + (rgb[2] - rgb[0]) / d :
         4 + (rgb[0] - rgb[1]) / d) / 6;
    if (h<0)
        h += 1;

    hsv[0] = h;
    hsv[1] = d/v;
    hsv[2] = v;
}

/// Convert a triplet from the HSV (Hue, Saturation, Value) color space to
/// RGB. Note that it is not required that the value of the HSV triplet is less
/// than or equal to 1, but it must not be negative.
///
/// \param hsv An HSV triplet with saturation in the interval [0,1] and
/// non-negative value. Any hue is accepted and shifted down to the canonical
/// interval [0,1).
///
/// \param rgb An RGB triplet with non-negative components. The components will
/// be less than or equal to 1 if the value of the HSV triplet is less than or
/// equal to one 1.
///
/// \sa http://en.wikipedia.org/wiki/HSV_color_space
/// \sa Computer Graphics - Foley, vanDam, Feiner, Hughes (1990)
template<class T> inline void cvt_HSV_to_RGB(const T* hsv, T* rgb)
{
    if (hsv[1] == 0) {
        // Achromatic
        rgb[0] = rgb[1] = rgb[2] = hsv[2];
        return;
    }

    T h = core::modulo<T>(hsv[0], 1) * 6;
    int i = static_cast<int>(h);
    T f = h - i;
    T c0 = hsv[2] * (1 - hsv[1]);
    T c1 = hsv[2] * (1 - (hsv[1] * f));
    T c2 = hsv[2] * (1 - (hsv[1] * (1 - f)));
    switch (i) {
	case 0:
            rgb[0] = hsv[2];
            rgb[1] = c2;
            rgb[2] = c0;
            break;
	case 1:
            rgb[0] = c1;
            rgb[1] = hsv[2];
            rgb[2] = c0;
            break;
	case 2:
            rgb[0] = c0;
            rgb[1] = hsv[2];
            rgb[2] = c2;
            break;
	case 3:
            rgb[0] = c0;
            rgb[1] = c1;
            rgb[2] = hsv[2];
            break;
	case 4:
            rgb[0] = c2;
            rgb[1] = c0;
            rgb[2] = hsv[2];
            break;
	default:
            rgb[0] = hsv[2];
            rgb[1] = c0;
            rgb[2] = c1;
            break;
    }
}

template<class T> inline math::BasicVec<3,T> cvt_RGB_to_HSV(const math::BasicVec<3,T>& rgb)
{
    math::BasicVec<3,T> hsv;
    cvt_RGB_to_HSV(rgb.get(), hsv.get());
    return hsv;
}

template<class T> inline math::BasicVec<3,T> cvt_HSV_to_RGB(const math::BasicVec<3,T>& hsv)
{
    math::BasicVec<3,T> rgb;
    cvt_HSV_to_RGB(hsv.get(), rgb.get());
    return rgb;
}

/// Convert an RGB color triplet to the alternative color space YCbCr
/// (Luminance, Blue chrominance, Red chrominance). The YCbCr color space is
/// closely related to, but not the same as YUV. YCbCr is a device independant
/// color space, and is the basis for the JPG image and MPG movie formats.
///
/// \param rgb An RGB triplet. Each component is expected to lie in interval \c
/// [0,1].
///
/// \param ycbcr The corresponding YCbCr triplet. If all RGB components are in
/// the range \c [0,1] then so are the YCbCr components.
///
/// The conversion formula is taken from Wikipedia and follows the Libjpeg style
/// where the full range [0;1] is used for YCbCr components.
template<class T> inline void cvt_RGB_to_YCbCr(const T* rgb, T* ycbcr)
{
    T kb = 0.114;
    T kr = 0.299;

    T r = rgb[0], g = rgb[1], b = rgb[2];
    T y = kr * r + (1 - kr - kb) * g + kb * b;

    ycbcr[0] = y;
    ycbcr[1] = 0.5 * (b - y) / (1 - kb) + 0.5;
    ycbcr[2] = 0.5 * (r - y) / (1 - kr) + 0.5;
}

/// Convert a triplet from the from the YCbCr (Luminance, Blue chrominance, Red
/// chrominance) color space to RGB. The YCbCr color space is closely related
/// to, but not the same as YUV.
///
/// \param ycbcr An YCbCr triplet. Each component is expected to lie in interval
/// \c [0,1].
///
/// \param rgb The corresponding RGB triplet. If the specified YCbCr triplet is
/// a valid combination of component values, all the resulting RGB components
/// will also lie in interval \c [0,1]. However since the YCbCr color space does
/// not allow all combinations of components within the unit cube, you may get
/// RGB triplets outside the cube, even if your YCbCr triplet is within the
/// cube.
///
/// The conversion formula is derived as the inverse of
/// <tt>convertRgbToYCbCr</tt>.
template<class T> inline void cvt_YCbCr_to_RGB(const T* ycbcr, T* rgb)
{
    T kb = 0.114;
    T kr = 0.299;

    T y = ycbcr[0], cb = ycbcr[1], cr = ycbcr[2];
    T b = 2 * (cb - 0.5) * (1 - kb) + y;
    T r = 2 * (cr - 0.5) * (1 - kr) + y;

    rgb[0] = r;
    rgb[1] = (y - kr * r - kb * b) / (1 - kr - kb);
    rgb[2] = b;
}

/// Convert an RGB color triplet to the alternative color space CMYK (Cyan,
/// Magenta, Yellow, Key). This is a very simplistic implementation, and it
/// cannot be considered accurate. The main problem is that the CMYK color space
/// is device specific.
///
/// \param rgb An RGB triplet. Each component is expected to lie in interval \c
/// [0,1].
///
/// \param cmyk The corresponding CMYK quadruple. If all RGB components are in
/// the range \c [0,1] then so are all the CMYK components.
///
/// The conversion formula is taken from
/// http://www.martinreddy.net/gfx/faqs/colorconv.faq.
template<class T> inline void cvt_RGB_to_CMYK(const T* rgb, T* cmyk)
{
    T c = 1-rgb[0], m = 1-rgb[1], y = 1-rgb[2];
    T b = cmyk[3] = core::min3(c,m,y);
    cmyk[0] = b == 1 && c == 1 ? 0 : (c-b) / (1-b);
    cmyk[1] = b == 1 && m == 1 ? 0 : (m-b) / (1-b);
    cmyk[2] = b == 1 && y == 1 ? 0 : (y-b) / (1-b);
}

/// Convert a quadruple from the from the CMYK (Cyan, Magenta, Yellow, Key)
/// color space to RGB. This is a very simplistic implementation, and it cannot
/// be considered accurate. The main problem is that the CMYK color space is
/// device specific.
///
/// \param cmyk A CMYK quadruple. Each component is expected to lie in interval
/// \c [0,1].
///
/// \param rgb The corresponding RGB triplet. If the specified CMYK triplet is a
/// valid combination of component values, all the resulting RGB components will
/// also lie in interval \c [0,1]. However since the CMYK color space does not
/// allow all combinations of components within the unit 4-cube, you may get RGB
/// triplets outside the unit cube, even if your CMYK quadruple is within the
/// 4-cube.
///
/// The conversion formula is taken from
/// http://www.martinreddy.net/gfx/faqs/colorconv.faq.
template<class T> inline void cvt_CMYK_to_RGB(const T* cmyk, T* rgb)
{
    T b = cmyk[3];
    rgb[0] = 1 - (cmyk[0] * (1-b) + b);
    rgb[1] = 1 - (cmyk[1] * (1-b) + b);
    rgb[2] = 1 - (cmyk[2] * (1-b) + b);
}

/// Linear interpolation of RGB colors in the HSV space.
///
/// Three matters will need further clairification:
///
/// Because the HSV space is cylindrical (or you could say, because the hue
/// component is an angle, and angles are cyclic) there are generally two paths
/// that takes us from the first point to the second. Clockwise and counter
/// clockwise around the value axis. Unless the two colors are complementary to
/// each other, one of the paths will be shortest, and this function chooses
/// that one. If the two colors are complementary to each other it is undefined
/// which one will be chosen.
///
/// If one of the colors (not both) have an abritrary/undefined hue component
/// (when the saturation component is zero / the achromatic case), then the
/// interpolation will use the hue component from the other color for every
/// interpolated color. This is sensible since any other hue will look
/// artificial.
///
/// If one of the colors (not both) have an abritrary/undefined saturation
/// component (when the value component is zero / black), then the interpolation
/// will use the saturation component from the other color for every
/// interpolated color. This is sensible since any other saturation will look
/// artificial.
///
/// \sa math::linInterp
math::Vec3 interp(double x, double x1, double x2, const math::Vec3& c1, const math::Vec3& c2);




// Implementation

namespace _Impl {

const CIE_RGB_PrimSpec srgb_prim_spec(math::Vec2F(0.6400, 0.3300),  // Red
                                      math::Vec2F(0.3000, 0.6000),  // Green
                                      math::Vec2F(0.1500, 0.0600),  // Blue
                                      math::Vec2F(0.3127, 0.3290)); // White

template<class T> class sRGB {
public:
    static const T a, gamma, phi, k0;
    static const math::BasicVec<3,T> white;
    static const math::BasicMat<3,3,T> to_xyz, fr_xyz;

    static T gamma_enc(T v)
    {
        return k0/phi < v ? (1+a)*std::pow(v, 1/gamma) - a : phi * v;
    }

    static T gamma_dec(T v)
    {
        return k0 < v ? std::pow((v+a)/(1+a), gamma) : v/phi;
    }

    static math::BasicVec<3,T> get_white()
    {
        const math::Vec2F& w = srgb_prim_spec.white;
        return math::BasicVec<3,T>(T(w[0])/w[1], 1, (1-(T(w[0])+w[1]))/w[1]);
    }

    static math::BasicMat<3,3,T> get_to_xyz()
    {
        const math::Vec2F& r = srgb_prim_spec.red;
        const math::Vec2F& g = srgb_prim_spec.green;
        const math::Vec2F& b = srgb_prim_spec.blue;

        math::BasicMat<3,3,T> m;
        m.row(0) = math::BasicVec<3,T>(r[0], g[0], b[0]);
        m.row(1) = math::BasicVec<3,T>(r[1], g[1], b[1]);
        m.row(2) = math::BasicVec<3,T>(1) - (m.row(0) + m.row(1));

        m.scale(math::inv(m) * white);
        return m;
    }
};

template<class T> const T sRGB<T>::a     = 0.055F;
template<class T> const T sRGB<T>::gamma = 2.4F;
template<class T> const T sRGB<T>::k0    = 0.040448236276987F;
template<class T> const T sRGB<T>::phi   = k0/std::pow((k0+a)/(1+a), gamma); // Approx 12.92

template<class T> const math::BasicVec<3,T> sRGB<T>::white(get_white());
template<class T> const math::BasicMat<3,3,T> sRGB<T>::to_xyz(get_to_xyz());
template<class T> const math::BasicMat<3,3,T> sRGB<T>::fr_xyz(math::inv(to_xyz));

template<class T> inline void cvt_RGB_to_Lin(const T* rgb, T* lin)
{
    std::transform(rgb, rgb+3, lin, std::ptr_fun(&sRGB<T>::gamma_dec));
}

template<class T> inline void cvt_Lin_to_RGB(const T* lin, T* rgb)
{
    std::transform(lin, lin+3, rgb, std::ptr_fun(&sRGB<T>::gamma_enc));
}

template<class T> inline void cvt_Lin_to_XYZ(const T* lin, T* xyz)
{
    math::vec3_adapt(xyz) = sRGB<T>::to_xyz * math::vec3_adapt(lin);
}

template<class T> inline void cvt_XYZ_to_Lin(const T* xyz, T* lin)
{
    math::vec3_adapt(lin) = sRGB<T>::fr_xyz * math::vec3_adapt(xyz);
}
}

template<class T> inline void cvt_RGB_to_XYZ(const T* rgb, T* xyz)
{
    T lin[3];
    _Impl::cvt_RGB_to_Lin(rgb, lin);
    _Impl::cvt_Lin_to_XYZ(lin, xyz);
}

template<class T> inline void cvt_XYZ_to_RGB(const T* xyz, T* rgb)
{
    T lin[3];
    _Impl::cvt_XYZ_to_Lin(xyz, lin);
    _Impl::cvt_Lin_to_RGB(lin, rgb);
}

/// \sa http://en.wikipedia.org/wiki/Lab_color_space
template<class T> class CIE_Lab {
public:
    static void from_xyz(const T* xyz, T* lab)
    {
        T f_y = f(xyz[1]);
        lab[0] = 116 * f_y - 16;
        lab[1] = 500 * (f(xyz[0] / _Impl::sRGB<T>::white[0]) - f_y);
        lab[2] = 200 * (f_y - f(xyz[2] / _Impl::sRGB<T>::white[2]));
    }

    static void to_xyz(const T* lab, T* xyz)
    {
        T f_y = (lab[0]+16) / 116;
        xyz[0] = t(f_y + lab[1]/500) * _Impl::sRGB<T>::white[0];
        xyz[1] = t(f_y);
        xyz[2] = t(f_y - lab[2]/200) * _Impl::sRGB<T>::white[2];
    }

private:
    static const T b, delta, t0, a;
    static T f(T t)
    {
        return t0 < t ? math::cbrt(t) : a*t + b;
    }
    static T t(T f)
    {
        return delta < f ? f*f*f : (f-b) * (1/a);
    }
};

template<class T> const T CIE_Lab<T>::b     = T(16)/116;
template<class T> const T CIE_Lab<T>::delta = 3*b/2; // = 6/29
template<class T> const T CIE_Lab<T>::t0    = delta*delta*delta;
template<class T> const T CIE_Lab<T>::a     = 1/(3*delta*delta);

} // namespace Color
} // namespace util
} // namespace archon

#endif // ARCHON_UTIL_COLOR_HPP
