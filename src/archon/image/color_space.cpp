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

#include <atomic>
#include <limits>
#include <stdexcept>
#include <algorithm>

#include <archon/core/meta.hpp>
#include <archon/core/types.hpp>
#include <archon/core/string.hpp>
#include <archon/util/color.hpp>
#include <archon/image/color_space_helper.hpp>


using namespace archon::core;
using namespace archon::util;
using namespace archon::image;
namespace image_impl = archon::image::_impl;


namespace {

//////////////////////   Luminance   //////////////////////

class Lum {
public:
    static const bool is_rgb = false;
    static const int num_channels = 1;
    static std::string get_mnemonic() { return "Lum"; }
    static std::string get_channel_id(int) { return "Lum"; }
    static std::string get_channel_name(int) { return "luminance"; }
    static const CanBlendWithBlack can_blend_with_black = can_blend_with_black_Fast;
    template<class Float> static void to_rgb(const Float* lum, Float* rgb)
    {
        color::cvt_Lum_to_RGB(lum[0], rgb);
    }
    template<class Float> static void from_rgb(const Float* rgb, Float* lum)
    {
        lum[0] = color::cvt_RGB_to_Lum(rgb);
    }
    template<class Float>
    static void blend_with_black(const Float* lum_1, Float* lum_2, Float alpha)
    {
        // Blending with black is a matter of scaling the RGB components with
        // the alpha value, but since RGB -> Luminance is a lienar combination,
        // this is equivalent to scaling the luminance.
        lum_2[0] = alpha * lum_1[0];
    }
};



//////////////////////   RGB   //////////////////////

const char* channel_ids_RGB[]   = { "R", "G", "B" };
const char* channel_names_RGB[] = { "red", "green", "blue" };

class RGB {
public:
    static const bool is_rgb = true;
    static const int num_channels = 3;
    static std::string get_mnemonic() { return "RGB"; }
    static std::string get_channel_id(int i) { return channel_ids_RGB[i]; };
    static std::string get_channel_name(int i) { return channel_names_RGB[i]; };
};



//////////////////////   XYZ   //////////////////////

const char* channels_XYZ[] = { "X", "Y", "Z" };

class XYZ {
public:
    static const bool is_rgb = false;
    static const int num_channels = 3;
    static std::string get_mnemonic() { return "XYZ"; }
    static std::string get_channel_id(int i) { return channels_XYZ[i]; };
    static std::string get_channel_name(int i) { return channels_XYZ[i]; };
    static const CanBlendWithBlack can_blend_with_black = can_blend_with_black_No;
    template<class Float> static void to_rgb(const Float* xyz, Float* rgb)
    {
        color::cvt_XYZ_to_RGB(xyz, rgb);
    }
    template<class Float> static void from_rgb(const Float* rgb, Float* xyz)
    {
        color::cvt_RGB_to_XYZ(rgb, xyz);
    }
};



//////////////////////   LAB   //////////////////////

const char* channel_ids_LAB[]   = { "L", "a", "b" };
const char* channel_names_LAB[] = { "lightness", "a", "b" };

class LAB {
public:
    static const bool is_rgb = false;
    static const int num_channels = 3;
    static std::string get_mnemonic() { return "LAB"; }
    static std::string get_channel_id(int i) { return channel_ids_LAB[i]; };
    static std::string get_channel_name(int i) { return channel_names_LAB[i]; };
    static const CanBlendWithBlack can_blend_with_black = can_blend_with_black_No;
    template<class Float> static void to_rgb(const Float* lab, Float* rgb)
    {
        Float xyz[3];
        color::CIE_Lab<Float>::to_xyz(lab, xyz);
        color::cvt_XYZ_to_RGB(xyz, rgb);
    }
    template<class Float> static void from_rgb(const Float* rgb, Float* lab)
    {
        Float xyz[3];
        color::cvt_RGB_to_XYZ(rgb, xyz);
        color::CIE_Lab<Float>::from_xyz(xyz, lab);
    }
};



//////////////////////   HSV   //////////////////////

const char* channel_ids_HSV[]   = { "H", "S", "V" };
const char* channel_names_HSV[] = { "hue", "saturation", "value" };

class HSV {
public:
    static const bool is_rgb = false;
    static const int num_channels = 3;
    static std::string get_mnemonic() { return "HSV"; }
    static std::string get_channel_id(int i) { return channel_ids_HSV[i]; };
    static std::string get_channel_name(int i) { return channel_names_HSV[i]; };
    static const CanBlendWithBlack can_blend_with_black = can_blend_with_black_Fast;
    template<class Float> static void to_rgb(const Float* hsv, Float* rgb)
    {
        color::cvt_HSV_to_RGB(hsv, rgb);
    }
    template<class Float> static void from_rgb(const Float* rgb, Float* hsv)
    {
        color::cvt_RGB_to_HSV(rgb, hsv);
    }
    template<class Float>
    static void blend_with_black(const Float* hsv_1, Float* hsv_2, Float alpha)
    {
        // Blending with black is a matter of scaling the RGB components with
        // the alpha value, but due to the nature of RGB -> HSV, this is
        // equivalent to scaling the value channel.
        hsv_2[0] =         hsv_1[0];
        hsv_2[1] =         hsv_1[1];
        hsv_2[2] = alpha * hsv_1[2];
    }
};



//////////////////////   YCbCr   //////////////////////

const char* channel_ids_YCbCr[]   = { "Y", "Cb", "Cr" };
const char* channel_names_YCbCr[] = { "luminance", "blue chrominance", "red chrominance" };

class YCbCr {
public:
    static const bool is_rgb = false;
    static const int num_channels = 3;
    static std::string get_mnemonic() { return "YCbCr"; }
    static std::string get_channel_id(int i) { return channel_ids_YCbCr[i]; };
    static std::string get_channel_name(int i) { return channel_names_YCbCr[i]; };
    static const CanBlendWithBlack can_blend_with_black = can_blend_with_black_No;
    template<class Float> static void to_rgb(const Float* ycbcr, Float* rgb)
    {
        color::cvt_YCbCr_to_RGB(ycbcr, rgb);
    }
    template<class Float> static void from_rgb(const Float* rgb, Float* ycbcr)
    {
        color::cvt_RGB_to_YCbCr(rgb, ycbcr);
    }
};



//////////////////////   CMYK   //////////////////////

const char* channel_ids_CMYK[]   = { "C", "M", "Y", "K" };
const char* channel_names_CMYK[] = { "cyan", "magenta", "yellow", "key" };

class CMYK {
public:
    static const bool is_rgb = false;
    static const int num_channels = 4;
    static std::string get_mnemonic() { return "CMYK"; }
    static std::string get_channel_id(int i) { return channel_ids_CMYK[i]; };
    static std::string get_channel_name(int i) { return channel_names_CMYK[i]; };
    static const CanBlendWithBlack can_blend_with_black = can_blend_with_black_No;
    template<class Float> static void to_rgb(const Float* cmyk, Float* rgb)
    {
        color::cvt_CMYK_to_RGB(cmyk, rgb);
    }
    template<class Float> static void from_rgb(const Float* rgb, Float* cmyk)
    {
        color::cvt_RGB_to_CMYK(rgb, cmyk);
    }
};




class CustColorSpaceBase {
public:
    const int primaries = 0;
    CustColorSpaceBase(int primaries): primaries{primaries} {}
    CustColorSpaceBase() {} // Should never be called
};

template<class T, bool source_rgb, bool target_rgb, ColorSpace::AlphaType alpha>
class CustCvt: public ColorSpace::Converter, public virtual CustColorSpaceBase {
public:
    void cvt(const void* source, void* target, std::size_t n) const noexcept
    {
        int preserve = std::min(primaries, 3);
        int source_primaries = (source_rgb ? 3 : primaries);
        int target_primaries = (target_rgb ? 3 : primaries);
        bool source_alpha = (alpha != ColorSpace::alpha_No && alpha != ColorSpace::alpha_Add);
        bool target_alpha = (alpha == ColorSpace::alpha_Keep || alpha == ColorSpace::alpha_Add);
        const T* s = static_cast<const T*>(source);
        T* t = static_cast<T*>(target);
        for (std::size_t i = 0; i < n; ++i) {
            if (source_rgb || target_rgb) {
                std::copy(s, s+preserve, t);
                std::fill(t+preserve, t+target_primaries, T());
            }
            else {
                std::copy(s, s+primaries, t);
            }
            if (target_alpha) {
                t[target_primaries] = (source_alpha ? s[source_primaries] :
                                       frac_any_to_any<double, T>(1));
            }
            else if(alpha == ColorSpace::alpha_Merge) {
                using F = typename CondType<std::numeric_limits<T>::is_integer,
                                            typename FastestFloatCover<T>::type, T>::type;
                F a = frac_any_to_any<T,F>(s[source_primaries]);
                for (T* p = t; p < t+preserve; ++p)
                    *p = frac_any_to_any<F,T>(frac_any_to_any<T,F>(*p) * a);
            }
            s += source_primaries + (source_alpha ? 1 : 0);
            t += target_primaries + (target_alpha ? 1 : 0);
        }
    }
};

template<class T>
class CustToRGB: public image_impl::AlphaSet<CustCvt<T, false, true, ColorSpace::alpha_No>,
                                             CustCvt<T, false, true, ColorSpace::alpha_Keep>,
                                             CustCvt<T, false, true, ColorSpace::alpha_Add>,
                                             CustCvt<T, false, true, ColorSpace::alpha_Discard>,
                                             CustCvt<T, false, true, ColorSpace::alpha_Merge>>
{
};

template<class T>
class CustFromRGB: public image_impl::AlphaSet<CustCvt<T, true, false, ColorSpace::alpha_No>,
                                               CustCvt<T, true, false, ColorSpace::alpha_Keep>,
                                               CustCvt<T, true, false, ColorSpace::alpha_Add>,
                                               CustCvt<T, true, false, ColorSpace::alpha_Discard>,
                                               CustCvt<T, true, false, ColorSpace::alpha_Merge>>
{
};

template<class T>
class CustToSelf: public image_impl::AlphaSet<CustCvt<T, false, false, ColorSpace::alpha_No>,
                                              CustCvt<T, false, false, ColorSpace::alpha_Keep>,
                                              CustCvt<T, false, false, ColorSpace::alpha_Add>,
                                              CustCvt<T, false, false, ColorSpace::alpha_Discard>,
                                              CustCvt<T, false, false, ColorSpace::alpha_Merge>>
{
};

enum CustWay {
    cway_ToRGB,   // Conversion from native color space to RGB
    cway_FromRGB, // Conversion from RGB to native color space
    cway_ToSelf   // Aalpha channel manipulation only
};

template<class T, WordType>
class CustWaySet: public CustToRGB<T>, public CustFromRGB<T>, public CustToSelf<T> {
public:
    const typename ColorSpace::Converter& operator()(std::pair<CustWay, ColorSpace::AlphaType> p) const
    {
        switch (p.first) {
            case cway_ToRGB:
                return static_cast<const CustToRGB<T>*>(this)->get_cvt(p.second);
            case cway_FromRGB:
                return static_cast<const CustFromRGB<T>*>(this)->get_cvt(p.second);
            case cway_ToSelf:
                return static_cast<const CustToSelf<T>*>(this)->get_cvt(p.second);
        }
        throw std::runtime_error("Unexpected color space conversion way");
    }
};

class CustColorSpace:
        public WordTypeSwitch<CustWaySet, std::pair<CustWay, ColorSpace::AlphaType>,
                              ColorSpace::Converter const &>,
        public ColorSpace {
public:
    std::string get_mnemonic(bool a) const
    {
        return "Cust" + std::string(a?"A_":"_") + format_int(unique_id);
    }
    std::string get_channel_id(int i) const
    {
        return ((i < 0 || primaries <= i) ? "A" : format_int(i+1) + "c");
    }
    std::string get_channel_name(int i) const
    {
        return ((i < 0 || primaries <= i) ? "alpha" : "custom channel " + format_int(i+1));
    }
    int get_num_primaries() const
    {
        return primaries;
    }
    const ColorSpace::Converter& to_rgb(WordType w, AlphaType a) const
    {
        return (*this)(w, std::make_pair(cway_ToRGB, a));
    }
    const ColorSpace::Converter& from_rgb(WordType w, AlphaType a) const
    {
        return (*this)(w, std::make_pair(cway_FromRGB, a));
    }
    const ColorSpace::Converter& to_self(WordType w,  AlphaType a) const
    {
        return (*this)(w, std::make_pair(cway_ToSelf, a));
    }
    const ColorSpace::Converter* to_any(const ColorSpace* c, WordType w, AlphaType a) const
    {
        if (c == this)
            return &to_self(w,a);
        if (c->is_rgb())
            return &to_rgb(w,a);
        return nullptr;
    }

    CustColorSpace(int primaries):
        CustColorSpaceBase{primaries},
        unique_id{get_next()+1}
    {
    }

    static int get_next()
    {
        static std::atomic<int> enumerator{0};
        return enumerator++;
    }

    const int unique_id;
};

} // unnamed namespace


namespace archon {
namespace image {

core::EnumAssoc ColorSpace::TypeSpec::map[] =
{
    { ColorSpace::type_Lum,   "Lum"   },
    { ColorSpace::type_RGB,   "RGB"   },
    { ColorSpace::type_XYZ,   "XYZ"   },
    { ColorSpace::type_LAB,   "LAB"   },
    { ColorSpace::type_HSV,   "HSV"   },
    { ColorSpace::type_YCbCr, "YCbCr" },
    { ColorSpace::type_CMYK,  "CMYK"  },
    { 0, 0 }
};

ColorSpace::ConstRef ColorSpace::get_Lum()
{
    static ConstRef c{new ColorSpaceHelper<Lum, type_Lum>()};
    return c;
}

ColorSpace::ConstRef ColorSpace::get_RGB()
{
    static ConstRef c{new ColorSpaceHelper<RGB, type_RGB>()};
    return c;
}

ColorSpace::ConstRef ColorSpace::get_XYZ()
{
    static ConstRef c{new ColorSpaceHelper<XYZ, type_XYZ>()};
    return c;
}

ColorSpace::ConstRef ColorSpace::get_LAB()
{
    static ConstRef c{new ColorSpaceHelper<LAB, type_LAB>()};
    return c;
}

ColorSpace::ConstRef ColorSpace::get_HSV()
{
    static ConstRef c{new ColorSpaceHelper<HSV, type_HSV>()};
    return c;
}

ColorSpace::ConstRef ColorSpace::get_YCbCr()
{
    static ConstRef c{new ColorSpaceHelper<YCbCr, type_YCbCr>()};
    return c;
}

ColorSpace::ConstRef ColorSpace::get_CMYK()
{
    static ConstRef c{new ColorSpaceHelper<CMYK, type_CMYK>()};
    return c;
}

ColorSpace::ConstRef ColorSpace::get(Type t)
{
    switch (t) {
        case type_Lum:
            return get_Lum();
        case type_RGB:
            return get_RGB();
        case type_XYZ:
            return get_XYZ();
        case type_LAB:
            return get_LAB();
        case type_HSV:
            return get_HSV();
        case type_YCbCr:
            return get_YCbCr();
        case type_CMYK:
            return get_CMYK();
        case type_custom:
            break;
    }
    throw std::invalid_argument("Bad color space");
}

ColorSpace::ConstRef ColorSpace::new_custom(int primaries)
{
    // Make sure that we do not exceed the maximum allowable number of primary
    // colors.
    if (max_num_primaries < primaries)
        throw std::invalid_argument("Too many primary colors in custom color space");
    return ConstRef{new CustColorSpace(primaries)};
}

} // namespace image
} // namespace archon
