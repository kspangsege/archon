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

#ifndef ARCHON_IMAGE_COLOR_SPACE_HELPER_HPP
#define ARCHON_IMAGE_COLOR_SPACE_HELPER_HPP

#include <limits>
#include <functional>
#include <algorithm>
#include <utility>

#include <archon/core/types.hpp>
#include <archon/util/unit_frac.hpp>
#include <archon/image/color_space.hpp>


namespace archon
{
  namespace image
  {
    /**
     * This template class is provided to make it easy to implement a
     * new color space. Implementing it from scratch is tedious due to
     * the many different converters that it must be able to
     * provide. All you need to do, is to define a \c Spec class, then
     * "plug" it into this template.
     *
     * A \c Spec class must have the following form:
     *
     * <pre>
     *
     *   struct MySpec
     *   {
     *     static bool const is_rgb = false;
     *     static int const num_channels;
     *     static string get_mnemonic();
     *     static string get_channel_id(int channel_index);
     *     static string get_channel_name(int channel_index);
     *     static CanBlendWithBlack const can_blend_with_black;
     *
     *     template<typename Float>
     *     static void to_rgb(Float const *native, Float *rgb);
     *
     *     template<typename Float>
     *     static void from_rgb(Float const *rgb, Float *native);
     *
     *     template<typename Float>
     *     static void blend_with_black(Float const *source, Float *target, Float alpha);
     *   };
     *
     * </pre>
     *
     * If your color space provides for a direct way to blend a color
     * with 'black' given some alpha value, you should set
     * <tt>can_blend_with_black</tt> to true, and define
     * <tt>BlendWithBlack</tt>. If, on the other hand, your color
     * space can only blend with black by first converting to RGB,
     * then blend, and then convert back to your color space, you must
     * set <tt>can_blend_with_black</tt> to false, and in this case you
     * can leave out the definition of <tt>BlendWithBlack</tt>.
     *
     * Each of the template classes <tt>ToRGB</tt>, <tt>FromRGB</tt>,
     * and <tt>BlendWithBlack</tt> must provide a method with the
     * following signature:
     *
     * <pre>
     *
     *   static void cvt(Float const *source, Float *target);
     *
     * </pre>
     *
     * <tt>FromRGB</tt> must convert a represenatation of a color in
     * the RGB color space to a represenatation of the closest
     * available color in your color space. <tt>source</tt> points to
     * the first component of an RGB triplet, with the order of
     * components always being 'red', 'green', and 'blue'. The
     * converted color, expressed in your color space, must be stored
     * into the tuple pointed to by <tt>target</tt>. The order in
     * which you store your channels into this tuple, defines the
     * <i>natural channel order</i> of your color space. The natural
     * channel order should be clearly stated as part of the
     * documentation of your color space.
     *
     * <tt>ToRGB</tt> must convert a represenatation of a color in
     * your color space to a represenatation of the closest available
     * color in the RGB color space. <tt>source</tt> points to a tuple
     * holding a color expressed in your color space. The order of
     * channels in this tuple will always follow the natural channel
     * order of your color space. The converted color, expressed in
     * RGB, must be stored into the triplet pointed to be
     * <tt>target</tt>.
     *
     * The function of <tt>BlendWithBlack</tt> is to eliminate an
     * alpha channel, that is, both source and target represenations
     * are expressed in your color space, but source comes with an
     * alpha value.
     */
    template<class Spec, ColorSpace::Type type = ColorSpace::type_custom>
    struct ColorSpaceHelper: ColorSpace
    {
      Type get_type() const { return type; }
      std::string get_mnemonic(bool a) const { return Spec::get_mnemonic() + (a?"A":""); }
      std::string get_channel_id(int i) const
      {
        return i < 0 || Spec::num_channels <= i ? "A" : Spec::get_channel_id(i);
      }
      std::string get_channel_name(int i) const
      {
        return i < 0 || Spec::num_channels <= i ? "alpha" : Spec::get_channel_name(i);
      }
      int get_num_primaries() const { return Spec::num_channels; }
      bool is_rgb() const { return Spec::is_rgb; }
      Converter const &to_rgb(WordType w,   AlphaType a) const { return get_cvt(way_ToRGB,   w, a); }
      Converter const &from_rgb(WordType w, AlphaType a) const { return get_cvt(way_FromRGB, w, a); }
      Converter const &to_self(WordType w,  AlphaType a) const { return get_cvt(way_ToSelf,  w, a); }
      Converter const *to_any(ColorSpace const *c, WordType w, AlphaType a) const
      {
        if(Spec::is_rgb) return &c->from_rgb(w,a);
        else if(c == this) return &to_self(w,a);
        else if(c->is_rgb()) return &to_rgb(w,a);
        else return 0;
      }
      ColorSpaceHelper(): cvt_set() {}

    private:
      enum Way
      {
        way_ToRGB,   // Conversion from native color space to RGB
        way_FromRGB, // Conversion from RGB to native color space
        way_ToSelf   // Aalpha channel manipulation only
      };

      Converter const &get_cvt(Way w, WordType t, AlphaType a) const
      {
        return cvt_set(t, std::make_pair(w,a));
      }

      template<typename, WordType> struct WaySet;
      WordTypeSwitch<WaySet, std::pair<Way, AlphaType>, Converter const &> const cvt_set;
    };


    enum CanBlendWithBlack
    {
      can_blend_with_black_No,   ///< Color space cannot blend with black directly.
      can_blend_with_black_Slow, ///< Blends with black slower than what can be done in RGB.
      can_blend_with_black_Fast  ///< Blends with black at least as fast as can be done in RGB.
    };






    // Implementation:

    namespace _impl
    {
      // This single-pixel operation wrapper allows an operation that
      // accepts only floating point word types, to be used in an
      // integer word type context. Conversion is performed on the
      // fly.
      template<template<typename> class O, class P> struct FloatWrap
      {
        template<typename UInt> struct Cvt
        {
          static void cvt(UInt const *source, UInt *target)
          {
            typedef typename core::FastestFloatCover<UInt>::type Float;
            Float s[P::source_color_channels], t[P::target_color_channels];
            util::frac_any_to_any(source, s, P::source_color_channels); // Int to float
            O<Float>::cvt(s,t);
            util::frac_any_to_any(t, target, P::target_color_channels); // Float to int
          }
        };
      };

      // This is the general color space converter implementation. It
      // just calls the specified single-pixel operation repeatedly,
      // and then allows for simple alpha channel manipulations (copy,
      // add, discard.)
      template<typename T, template<typename> class O, class P> struct Cvt3: ColorSpace::Converter
      {
        void cvt(void const *source, void *target, size_t n) const throw()
        {
          T const *s = static_cast<T const *>(source);
          T       *t = static_cast<T       *>(target);
          for(size_t i=0; i<n; ++i)
          {
            O<T>::cvt(s,t);
            if(P::target_has_alpha)
              t[P::target_color_channels] =
                P::source_has_alpha ? s[P::source_color_channels] : util::frac_any_to_any<double, T>(1);
            s += P::source_color_channels + (P::source_has_alpha ? 1 : 0);
            t += P::target_color_channels + (P::target_has_alpha ? 1 : 0);
          }
        }
      };

      // The sole purpose of this template is to detect when the word
      // type is integer, in which case the users operation must be
      // wrapped, since it is only required to accept floating point
      // data.
      template<typename T, bool is_int, template<typename> class O, class P> struct Cvt2 {};

      template<typename UInt, template<typename> class O, class P>
      struct Cvt2<UInt, true, O, P>: Cvt3<UInt, FloatWrap<O,P>::template Cvt, P> {};

      template<typename Float, template<typename> class O, class P>
      struct Cvt2<Float, false, O, P>: Cvt3<Float, O, P> {};

      // Just a wrapper around Cvt2 to make it easy to use.
      template<typename T, template<typename> class O, class P>
      struct Cvt: Cvt2<T, std::numeric_limits<T>::is_integer, O, P> {};

      // A degenerate color space converter used whan no conversion is
      // required.
      template<typename T, int N> struct Copy: ColorSpace::Converter
      {
        void cvt(void const *source, void *target, size_t n) const throw()
        {
          T const *s = static_cast<T const *>(source);
          T       *t = static_cast<T       *>(target);
          std::copy(s, s+n*N, t);
        }
      };

      // A degenerate color space converter used when no conversion is
      // required, except simple alpha channel manipulation (add,
      // discard, copy.)
      template<typename T, int N, bool discard> struct AddOrDiscardAlpha: ColorSpace::Converter
      {
        void cvt(void const *source, void *target, size_t n) const throw()
        {
          T const *s = static_cast<T const *>(source);
          T       *t = static_cast<T       *>(target);
          for(size_t i = 0; i<n; ++i)
          {
            T const *f = s;
            s += N;
            t = std::copy(f, s, t);
            if(discard) ++s;
            else *t++ = util::frac_any_to_any<double, T>(1);
          }
        }
      };



      // Wrap template classes around the to_rgb and from_rgb methods
      // from the color space specification such that they can be
      // passed as template arguments.
      template<class S> struct ToFromRGB
      {
        template<typename Float> struct ToRGB
        {
          static void cvt(Float const *nat, Float *rgb) { S::to_rgb(nat, rgb); }
        };
        template<typename Float> struct FromRGB
        {
          static void cvt(Float const *rgb, Float *nat) { S::from_rgb(rgb, nat); }
        };
      };

      // Blend the specified RGB color with black as in alpha*rgb +
      // (1-alpha)*black.
      template<typename Float> inline void blend_with_black_rgb(Float const *s, Float *t, Float alpha)
      {
        std::transform(s, s+3, t, std::bind1st(std::multiplies<Float>(), alpha));
      }

      // A degenerate case of merging with black for the RGB color
      // space.
      struct MergeRGB
      {
        template<typename Float> struct ToSelf
        {
          static void cvt(Float const *rgb_alpha, Float *rgb)
          {
            blend_with_black_rgb(rgb_alpha, rgb, rgb_alpha[3]);
          }
        };
      };

      // Merge a color with black in the absence of a custom, color
      // space specific blend function.
      template<class S> struct MergeViaRGB
      {
        template<typename Float> struct ToRGB
        {
          static void cvt(Float const *nat_alpha, Float *rgb)
          {
            Float rgb2[3];
            S::to_rgb(nat_alpha, rgb2); // Ignores alpha
            blend_with_black_rgb(rgb2, rgb, nat_alpha[S::num_channels]);
          }
        };
        template<typename Float> struct FromRGB
        {
          static void cvt(Float const *rgb_alpha, Float *nat)
          {
            Float rgb[3];
            blend_with_black_rgb(rgb_alpha, rgb, rgb_alpha[3]);
            S::from_rgb(rgb, nat);
          }
        };
        template<typename Float> struct ToSelf
        {
          static void cvt(Float const *nat_alpha, Float *nat)
          {
            Float rgb[3];
            ToRGB<Float>::cvt(nat_alpha, rgb);
            S::from_rgb(rgb, nat);
          }
        };
      };

      // Merge a color with black using the custom blend function
      // provided by the color space specification.
      template<class S> struct MergeNative
      {
        template<typename Float> struct ToRGB
        {
          static void cvt(Float const *nat_alpha, Float *rgb)
          {
            Float nat[S::num_channels];
            S::blend_with_black(nat_alpha, nat, nat_alpha[S::num_channels]);
            S::to_rgb(nat, rgb);
          }
        };
        template<typename Float> struct FromRGB
        {
          static void cvt(Float const *rgb_alpha, Float *nat)
          {
            Float nat2[S::num_channels];
            S::from_rgb(rgb_alpha, nat2); // Ignores alpha
            S::blend_with_black(nat2, nat, rgb_alpha[3]);
          }
        };
        template<typename Float> struct ToSelf
        {
          static void cvt(Float const *nat_alpha, Float *nat)
          {
            S::blend_with_black(nat_alpha, nat, nat_alpha[S::num_channels]);
          }
        };
      };



      // A set of converter instances, one for each type of alpha
      // channel handling.
      template<class No, class Keep, class Add, class Discard, class Merge>
      struct AlphaSet: No, Keep, Add, Discard, Merge
      {
        typename ColorSpace::Converter const &get_cvt(typename ColorSpace::AlphaType a) const
        {
          switch(a)
          {
          case ColorSpace::alpha_No:      return static_cast<No      const &>(*this);
          case ColorSpace::alpha_Keep:    return static_cast<Keep    const &>(*this);
          case ColorSpace::alpha_Add:     return static_cast<Add     const &>(*this);
          case ColorSpace::alpha_Discard: return static_cast<Discard const &>(*this);
          case ColorSpace::alpha_Merge:   return static_cast<Merge   const &>(*this);
          }
          throw std::runtime_error("Unexpected alpha manipulation type");
        }
      };



      template<int m, int n, bool a, bool b> struct Params
      {
        static int  const source_color_channels = m;
        static int  const target_color_channels = n;
        static bool const source_has_alpha      = a;
        static bool const target_has_alpha      = b;
      };

      template<typename T, class X, class M, int N>
      struct ToRGB: AlphaSet<Cvt<T, X::template ToRGB, Params<N,   3, false, false> >,
                             Cvt<T, X::template ToRGB, Params<N,   3, true,  true>  >,
                             Cvt<T, X::template ToRGB, Params<N,   3, false, true>  >,
                             Cvt<T, X::template ToRGB, Params<N,   3, true,  false> >,
                             Cvt<T, M::template ToRGB, Params<N+1, 3, false, false> > > {};

      template<typename T, class X, class M, int N>
      struct FromRGB: AlphaSet<Cvt<T, X::template FromRGB, Params<3, N, false, false> >,
                               Cvt<T, X::template FromRGB, Params<3, N, true,  true>  >,
                               Cvt<T, X::template FromRGB, Params<3, N, false, true>  >,
                               Cvt<T, X::template FromRGB, Params<3, N, true,  false> >,
                               Cvt<T, M::template FromRGB, Params<4, N, false, false> > > {};

      template<typename T, class M, int N>
      struct ToSelf: AlphaSet<Copy<T, N>,
                              Copy<T, N+1>,
                              AddOrDiscardAlpha<T, N, false>,
                              AddOrDiscardAlpha<T, N, true>,
                              Cvt<T, M::template ToSelf, Params<N+1, N, false, false> > > {};



      template<class S, typename T, CanBlendWithBlack> struct WaySet2 {};

      // Specialization for a color space that cannot blend with
      // black.
      template<class S, typename T> struct WaySet2<S, T, can_blend_with_black_No>
      {
        ToRGB   <T, ToFromRGB<S>, MergeViaRGB<S>, S::num_channels> to_rgb;
        FromRGB <T, ToFromRGB<S>, MergeViaRGB<S>, S::num_channels> from_rgb;
        ToSelf  <T,               MergeViaRGB<S>, S::num_channels> to_self;
      };

      // Specialization for a color space that can blend with black,
      // but blending in RGB is faster.
      template<class S, typename T> struct WaySet2<S, T, can_blend_with_black_Slow>
      {
        ToRGB   <T, ToFromRGB<S>, MergeViaRGB<S>, S::num_channels> to_rgb;
        FromRGB <T, ToFromRGB<S>, MergeViaRGB<S>, S::num_channels> from_rgb;
        ToSelf  <T,               MergeNative<S>, S::num_channels> to_self;
      };

      // Specialization for a color space that can blend with black at
      // least as fast as can be done in RGB.
      template<class S, typename T> struct WaySet2<S, T, can_blend_with_black_Fast>
      {
        ToRGB   <T, ToFromRGB<S>, MergeNative<S>, S::num_channels> to_rgb;
        FromRGB <T, ToFromRGB<S>, MergeNative<S>, S::num_channels> from_rgb;
        ToSelf  <T,               MergeNative<S>, S::num_channels> to_self;
      };



      template<class S, typename T, bool is_rgb> struct WaySet1 {};

      // Specialization for color spaces that are not RGB
      template<class S, typename T>
      struct WaySet1<S, T, false>: WaySet2<S, T, S::can_blend_with_black> {};

      // Specialization for the RGB color space
      template<class S, typename T> struct WaySet1<S, T, true>
      {
        ToSelf<T, MergeRGB, S::num_channels> to_rgb, from_rgb, to_self;
      };
    }



    // A set of AlphaSet's. One AlphaSet for each 'way' (to RGB, from RGB, to self).
    template<class S, ColorSpace::Type t> template<typename T, WordType>
    struct ColorSpaceHelper<S,t>::WaySet: _impl::WaySet1<S, T, S::is_rgb>
    {
      typename ColorSpace::Converter const &operator()(std::pair<Way, AlphaType> p) const
      {
        switch(p.first)
        {
        case way_ToRGB:   return   this->to_rgb.get_cvt(p.second);
        case way_FromRGB: return this->from_rgb.get_cvt(p.second);
        case way_ToSelf:  return  this->to_self.get_cvt(p.second);
        }
        throw std::runtime_error("Unexpected color space conversion way");
      }
    };
  }
}

#endif // ARCHON_IMAGE_COLOR_SPACE_HELPER_HPP
