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

/**
 * \file
 *
 * \author Kristian Spangsege
 */

#include <limits>
#include <stdexcept>
#include <algorithm>

#include <archon/core/meta.hpp>
#include <archon/core/types.hpp>
#include <archon/core/atomic.hpp>
#include <archon/core/text.hpp>
#include <archon/util/color.hpp>
#include <archon/image/color_space_helper.hpp>


using namespace std;
using namespace archon::core;
using namespace archon::util;
using namespace archon::Imaging;
namespace ImageImpl = archon::Imaging::_Impl;


namespace
{
  //////////////////////   Luminance   //////////////////////

  struct Lum
  {
    static bool const is_rgb = false;
    static int const num_channels = 1;
    static string get_mnemonic() { return "Lum"; }
    static string get_channel_id(int) { return "Lum"; }
    static string get_channel_name(int) { return "luminance"; }
    static CanBlendWithBlack const can_blend_with_black = can_blend_with_black_Fast;
    template<typename Float> static void to_rgb(Float const *lum, Float *rgb)
    {
      Color::cvt_Lum_to_RGB(lum[0], rgb);
    }
    template<typename Float> static void from_rgb(Float const *rgb, Float *lum)
    {
      lum[0] = Color::cvt_RGB_to_Lum(rgb);
    }
    template<typename Float>
    static void blend_with_black(Float const *lum1, Float *lum2, Float alpha)
    {
      // Blending with black is a matter of scaling the RGB components
      // with the alpha value, but since RGB -> Luminance is a lienar
      // combination, this is equivalent to scaling the luminance.
      lum2[0] = alpha * lum1[0];
    }
  };



  //////////////////////   RGB   //////////////////////

  char const *channel_ids_RGB[]   = { "R", "G", "B" };
  char const *channel_names_RGB[] = { "red", "green", "blue" };

  struct RGB
  {
    static bool const is_rgb = true;
    static int const num_channels = 3;
    static string get_mnemonic() { return "RGB"; }
    static string get_channel_id(int i) { return channel_ids_RGB[i]; };
    static string get_channel_name(int i) { return channel_names_RGB[i]; };
  };



  //////////////////////   XYZ   //////////////////////

  char const *channels_XYZ[] = { "X", "Y", "Z" };

  struct XYZ
  {
    static bool const is_rgb = false;
    static int const num_channels = 3;
    static string get_mnemonic() { return "XYZ"; }
    static string get_channel_id(int i) { return channels_XYZ[i]; };
    static string get_channel_name(int i) { return channels_XYZ[i]; };
    static CanBlendWithBlack const can_blend_with_black = can_blend_with_black_No;
    template<typename Float> static void to_rgb(Float const *xyz, Float *rgb)
    {
      Color::cvt_XYZ_to_RGB(xyz, rgb);
    }
    template<typename Float> static void from_rgb(Float const *rgb, Float *xyz)
    {
      Color::cvt_RGB_to_XYZ(rgb, xyz);
    }
  };



  //////////////////////   LAB   //////////////////////

  char const *channel_ids_LAB[]   = { "L", "a", "b" };
  char const *channel_names_LAB[] = { "lightness", "a", "b" };

  struct LAB
  {
    static bool const is_rgb = false;
    static int const num_channels = 3;
    static string get_mnemonic() { return "LAB"; }
    static string get_channel_id(int i) { return channel_ids_LAB[i]; };
    static string get_channel_name(int i) { return channel_names_LAB[i]; };
    static CanBlendWithBlack const can_blend_with_black = can_blend_with_black_No;
    template<typename Float> static void to_rgb(Float const *lab, Float *rgb)
    {
      Float xyz[3];
      Color::CIE_Lab<Float>::to_xyz(lab, xyz);
      Color::cvt_XYZ_to_RGB(xyz, rgb);
    }
    template<typename Float> static void from_rgb(Float const *rgb, Float *lab)
    {
      Float xyz[3];
      Color::cvt_RGB_to_XYZ(rgb, xyz);
      Color::CIE_Lab<Float>::from_xyz(xyz, lab);
    }
  };



  //////////////////////   HSV   //////////////////////

  char const *channel_ids_HSV[]   = { "H", "S", "V" };
  char const *channel_names_HSV[] = { "hue", "saturation", "value" };

  struct HSV
  {
    static bool const is_rgb = false;
    static int const num_channels = 3;
    static string get_mnemonic() { return "HSV"; }
    static string get_channel_id(int i) { return channel_ids_HSV[i]; };
    static string get_channel_name(int i) { return channel_names_HSV[i]; };
    static CanBlendWithBlack const can_blend_with_black = can_blend_with_black_Fast;
    template<typename Float> static void to_rgb(Float const *hsv, Float *rgb)
    {
      Color::cvt_HSV_to_RGB(hsv, rgb);
    }
    template<typename Float> static void from_rgb(Float const *rgb, Float *hsv)
    {
      Color::cvt_RGB_to_HSV(rgb, hsv);
    }
    template<typename Float>
    static void blend_with_black(Float const *hsv1, Float *hsv2, Float alpha)
    {
      // Blending with black is a matter of scaling the RGB
      // components with the alpha value, but due to the nature of
      // RGB -> HSV, this is equivalent to scaling the value
      // channel.
      hsv2[0] =         hsv1[0];
      hsv2[1] =         hsv1[1];
      hsv2[2] = alpha * hsv1[2];
    }
  };



  //////////////////////   YCbCr   //////////////////////

  char const *channel_ids_YCbCr[]   = { "Y", "Cb", "Cr" };
  char const *channel_names_YCbCr[] = { "luminance", "blue chrominance", "red chrominance" };

  struct YCbCr
  {
    static bool const is_rgb = false;
    static int const num_channels = 3;
    static string get_mnemonic() { return "YCbCr"; }
    static string get_channel_id(int i) { return channel_ids_YCbCr[i]; };
    static string get_channel_name(int i) { return channel_names_YCbCr[i]; };
    static CanBlendWithBlack const can_blend_with_black = can_blend_with_black_No;
    template<typename Float> static void to_rgb(Float const *ycbcr, Float *rgb)
    {
      Color::cvt_YCbCr_to_RGB(ycbcr, rgb);
    }
    template<typename Float> static void from_rgb(Float const *rgb, Float *ycbcr)
    {
      Color::cvt_RGB_to_YCbCr(rgb, ycbcr);
    }
  };



  //////////////////////   CMYK   //////////////////////

  char const *channel_ids_CMYK[]   = { "C", "M", "Y", "K" };
  char const *channel_names_CMYK[] = { "cyan", "magenta", "yellow", "key" };

  struct CMYK
  {
    static bool const is_rgb = false;
    static int const num_channels = 4;
    static string get_mnemonic() { return "CMYK"; }
    static string get_channel_id(int i) { return channel_ids_CMYK[i]; };
    static string get_channel_name(int i) { return channel_names_CMYK[i]; };
    static CanBlendWithBlack const can_blend_with_black = can_blend_with_black_No;
    template<typename Float> static void to_rgb(Float const *cmyk, Float *rgb)
    {
      Color::cvt_CMYK_to_RGB(cmyk, rgb);
    }
    template<typename Float> static void from_rgb(Float const *rgb, Float *cmyk)
    {
      Color::cvt_RGB_to_CMYK(rgb, cmyk);
    }
  };




  struct CustColorSpaceBase
  {
    int const primaries;
    CustColorSpaceBase(int primaries): primaries(primaries) {}
    CustColorSpaceBase(): primaries(0) {} // Should never be called
  };

  template<typename T, bool source_rgb, bool target_rgb, ColorSpace::AlphaType alpha>
  struct CustCvt: ColorSpace::Converter, virtual CustColorSpaceBase
  {
    void cvt(void const *source, void *target, size_t n) const throw()
    {
      int const preserve = min(primaries, 3);
      int const source_primaries = source_rgb ? 3 : primaries;
      int const target_primaries = target_rgb ? 3 : primaries;
      bool source_alpha = alpha != ColorSpace::alpha_No && alpha != ColorSpace::alpha_Add;
      bool target_alpha = alpha == ColorSpace::alpha_Keep || alpha == ColorSpace::alpha_Add;
      T const *s = static_cast<T const *>(source);
      T       *t = static_cast<T       *>(target);
      for(size_t i = 0; i<n; ++i)
      {
        if(source_rgb || target_rgb)
        {
          copy(s, s+preserve, t);
          fill(t+preserve, t+target_primaries, T());
        }
        else copy(s, s+primaries, t);
        if(target_alpha)
          t[target_primaries] = source_alpha ? s[source_primaries] : frac_any_to_any<double, T>(1);
        else if(alpha == ColorSpace::alpha_Merge)
        {
          typedef typename CondType<numeric_limits<T>::is_integer,
            typename FastestFloatCover<T>::type, T>::type F;
          F a = frac_any_to_any<T,F>(s[source_primaries]);
          for(T *p=t; p<t+preserve; ++p) *p = frac_any_to_any<F,T>(frac_any_to_any<T,F>(*p)*a);
        }
        s += source_primaries + (source_alpha?1:0);
        t += target_primaries + (target_alpha?1:0);
      }
    }
  };

  template<typename T>
  struct CustToRGB: ImageImpl::AlphaSet<CustCvt<T, false, true, ColorSpace::alpha_No>,
                                        CustCvt<T, false, true, ColorSpace::alpha_Keep>,
                                        CustCvt<T, false, true, ColorSpace::alpha_Add>,
                                        CustCvt<T, false, true, ColorSpace::alpha_Discard>,
                                        CustCvt<T, false, true, ColorSpace::alpha_Merge> > {};
  template<typename T>
  struct CustFromRGB: ImageImpl::AlphaSet<CustCvt<T, true, false, ColorSpace::alpha_No>,
                                          CustCvt<T, true, false, ColorSpace::alpha_Keep>,
                                          CustCvt<T, true, false, ColorSpace::alpha_Add>,
                                          CustCvt<T, true, false, ColorSpace::alpha_Discard>,
                                          CustCvt<T, true, false, ColorSpace::alpha_Merge> > {};
  template<typename T>
  struct CustToSelf: ImageImpl::AlphaSet<CustCvt<T, false, false, ColorSpace::alpha_No>,
                                         CustCvt<T, false, false, ColorSpace::alpha_Keep>,
                                         CustCvt<T, false, false, ColorSpace::alpha_Add>,
                                         CustCvt<T, false, false, ColorSpace::alpha_Discard>,
                                         CustCvt<T, false, false, ColorSpace::alpha_Merge> > {};

  enum CustWay
  {
    cway_ToRGB,   // Conversion from native color space to RGB
    cway_FromRGB, // Conversion from RGB to native color space
    cway_ToSelf   // Aalpha channel manipulation only
  };

  template<typename T, WordType> struct CustWaySet: CustToRGB<T>, CustFromRGB<T>, CustToSelf<T>
  {
    typename ColorSpace::Converter const &operator()(pair<CustWay, ColorSpace::AlphaType> p) const
    {
      switch(p.first)
      {
      case cway_ToRGB:   return static_cast<CustToRGB<T>   const *>(this)->get_cvt(p.second);
      case cway_FromRGB: return static_cast<CustFromRGB<T> const *>(this)->get_cvt(p.second);
      case cway_ToSelf:  return static_cast<CustToSelf<T>  const *>(this)->get_cvt(p.second);
      }
      throw std::runtime_error("Unexpected color space conversion way");
    }
  };

  struct CustColorSpace:
    WordTypeSwitch<CustWaySet, std::pair<CustWay, ColorSpace::AlphaType>,
                   ColorSpace::Converter const &>, ColorSpace
  {
    string get_mnemonic(bool a) const { return "Cust"+string(a?"A_":"_")+Text::print(unique_id); }
    string get_channel_id(int i) const
    {
      return i < 0 || primaries <= i ? "A" : Text::print(i+1)+"c";
    }
    string get_channel_name(int i) const
    {
      return i < 0 || primaries <= i ? "alpha" : "custom channel "+Text::print(i+1);
    }
    int get_num_primaries() const { return primaries; }
    ColorSpace::Converter const &to_rgb(WordType w,   AlphaType a) const
    {
      return (*this)(w, make_pair(cway_ToRGB, a));
    }
    ColorSpace::Converter const &from_rgb(WordType w, AlphaType a) const
    {
      return (*this)(w, make_pair(cway_FromRGB, a));
    }
    ColorSpace::Converter const &to_self(WordType w,  AlphaType a) const
    {
      return (*this)(w, make_pair(cway_ToSelf, a));
    }
    ColorSpace::Converter const *to_any(ColorSpace const *c, WordType w, AlphaType a) const
    {
      if(c == this) return &to_self(w,a);
      else if(c->is_rgb()) return &to_rgb(w,a);
      else return 0;
    }

    CustColorSpace(int primaries): CustColorSpaceBase(primaries), unique_id(get_next()+1) {}

    static int get_next()
    {
      static Atomic enumerator(0);
      return enumerator.fetch_and_add(1);
    }

    int const unique_id;
  };
}


namespace archon
{
  namespace Imaging
  {
    archon::core::EnumAssoc ColorSpace::TypeSpec::map[] =
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
      static ConstRef c(new ColorSpaceHelper<Lum, type_Lum>());
      return c;
    }

    ColorSpace::ConstRef ColorSpace::get_RGB()
    {
      static ConstRef c(new ColorSpaceHelper<RGB, type_RGB>());
      return c;
    }

    ColorSpace::ConstRef ColorSpace::get_XYZ()
    {
      static ConstRef c(new ColorSpaceHelper<XYZ, type_XYZ>());
      return c;
    }

    ColorSpace::ConstRef ColorSpace::get_LAB()
    {
      static ConstRef c(new ColorSpaceHelper<LAB, type_LAB>());
      return c;
    }

    ColorSpace::ConstRef ColorSpace::get_HSV()
    {
      static ConstRef c(new ColorSpaceHelper<HSV, type_HSV>());
      return c;
    }

    ColorSpace::ConstRef ColorSpace::get_YCbCr()
    {
      static ConstRef c(new ColorSpaceHelper<YCbCr, type_YCbCr>());
      return c;
    }

    ColorSpace::ConstRef ColorSpace::get_CMYK()
    {
      static ConstRef c(new ColorSpaceHelper<CMYK, type_CMYK>());
      return c;
    }

    ColorSpace::ConstRef ColorSpace::get(Type t)
    {
      switch(t)
      {
      case type_Lum:    return get_Lum();
      case type_RGB:    return get_RGB();
      case type_XYZ:    return get_XYZ();
      case type_LAB:    return get_LAB();
      case type_HSV:    return get_HSV();
      case type_YCbCr:  return get_YCbCr();
      case type_CMYK:   return get_CMYK();
      case type_custom: break;
      }
      throw invalid_argument("Bad color space");
    }

    ColorSpace::ConstRef ColorSpace::new_custom(int primaries)
    {
      // Make sure that we do not exceed the maximum allowable number
      // of primary colors.
      if(max_num_primaries < primaries)
        throw invalid_argument("Too many primary colors in custom color space");
      return ConstRef(new CustColorSpace(primaries));
    }
  }
}
