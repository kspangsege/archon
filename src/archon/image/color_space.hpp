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

#ifndef ARCHON_IMAGE_COLOR_SPACE_HPP
#define ARCHON_IMAGE_COLOR_SPACE_HPP

#include <string>

#include <archon/core/refcnt.hpp>
#include <archon/image/word_type.hpp>


namespace Archon
{
  namespace Imaging
  {
    /**
     * This class represents a color space as used in image data. That
     * is, it encapsulates information about what kind of color
     * channels the data contains.
     *
     * It is the intention that there is exactly one instance of this
     * class per color space at any given point in time. Thus, to
     * determine if two color spaces are the same, it is enough to
     * compare pointers.
     *
     * An important feature of this class is to provide conversion
     * functions for converting a stretch of pixels from one color
     * space to another. All color spaces must provide functions for
     * conversion to and from RGB. This means that conversion between
     * arbitrary color spaces can be performed in at most two steps.
     *
     * 
     *
     * The main feature of this class is to provide of methods for converting to and from
     * the RGB color space(red, green, blue, alpha) format.
     *
     * Normally the concepts of color space and alpha channel are kept
     * apart. In this case however they are fused together to provide
     * for fast conversion of sequences of color compounds.
     *
     * \note Applications may assume that there is exactly one
     * instance of this class for each unique color space. This way
     * one may conclude that if \c a and \c b are color space instance
     * pointers then <tt>a == b</tt> if and only if \c a can be
     * converted to \c b by a simple copy operation (no conversion).
     *
     * \note When a color space has an alpha channel, that channel
     * must always come last in the canonical channel order of the
     * space.
     *
     * \note If the canonical channel order is not immediately
     * apparent from the space name/mnemonic, it must be clearly
     * documented what the canonical order is.
     *
     * \note A color space mnemonic must contain only alpha numeric
     * ASCII characters. This is necessary to circumvent ambiguities
     * in the string representation of pixel formats.
     *
     * \note A \c ColorSpace instance is to be considered immutable.
     *
     * \note It is the intention that color spaces are compared by
     * comparing object pointers, thus there should only ever be one
     * color space instance per color space. Or in other words, if two
     * objects exist for the same color space, one risks that color
     * space conversion will be performed even though it was not
     * strictly necessary. Further more, this may lead to loss of
     * information, for example due to conversion through a color
     * space of fewer channels. Special care must be taken when
     * working with custom color spaces, since \c new_custom returns a
     * new object every time. It is the responsibility of the
     * application to ensure that the same instance is used where
     * appropriate.
     *
     * \note No color space is allowed to have more channels than
     * <tt>max_number_of_channels</tt>.
     *
     * Thread safety: All methods shall be both individually and
     * mutually thread safe. This includes the \c Converter::convert
     * method.
     */
    struct ColorSpace: Core::CntRefObjectBase, Core::CntRefDefs<ColorSpace>
    {
      /**
       * Get the luminance color space.
       */
      static ConstRef get_Lum();

      /**
       * Get the sRGB color space.
       *
       * Canonican channel order is: red, green, blue.
       */
      static ConstRef get_RGB();

      /**
       * Get the CIE XYZ color space.
       *
       * Canonican channel order is: x, y, z.
       */
      static ConstRef get_XYZ();

      /**
       * Get the CIE 1976 (L*, a*, b*) color space.
       *
       * Canonican channel order is: lightness, a, b.
       */
      static ConstRef get_LAB();

      /**
       * Get the HSV color space.
       *
       * Canonican channel order is: hue, saturation, value.
       *
       * \sa Color::convertRgbToHsv
       */
      static ConstRef get_HSV();

      /**
       * Get the YCbCr color space. The YCbCr color space is closely
       * related to, but not the same as YUV.
       *
       * Canonican channel order is: luminance, blue chrominance, red
       * chrominance.
       *
       * \sa Color::convertRgbToYCbCr
       */
      static ConstRef get_YCbCr();

      /**
       * Get the CMYK color space. This conversion is based a very
       * simplistic implementation, and it cannot be considered
       * accurate. The main problem is that the CMYK color space is
       * device specific.
       *
       * Canonican channel order is: cyan, magenta, yellow, key.
       *
       * \sa Color::convertRgbToCmyk
       */
      static ConstRef get_CMYK();

      /**
       * Get a custom color space with an arbitrary number of channels
       * and an optional final alpha channel.
       *
       * Canonical channel order is defined by the application.
       *
       * this kind of color space is only usefull if you know
       * absolutely nothing about how to convert it to and from the
       * canonical RGBA color space. If you know something, you will
       * be better off defining you own cutom color space.
       *
       * When data is converted between this color space and the
       * canonical RGBA color space the first 3 color components are
       * mapped to red, green, and blue in that order. In the target
       * color space unmatched components will be set to zero. In the
       * source color space unmatched color components will be
       * ignored. The alpha channel, if present, is always preserved.
       *
       * \note Each call creates a new unique color space instance,
       * even if the arguments match those of a previously created
       * color space. What this means is that if you want to preserve
       * all your image data in a color space conversion sush as when
       * calling <tt>Image::put_image</tt>, you must specify the same
       * instance in both images, otherwise the image library will
       * assume that they are different color spaces and map the data
       * through the RGBA color space.
       */
      static ConstRef new_custom(int num_channels);

      /**
       * A color space descriptor that allows one to efficiently
       * distiguish between the built in color space types.
       */
      enum Type
      {
        type_Lum,    ///< Luminance
        type_RGB,    ///< sRGB color space (red, green, blue)
        type_XYZ,    ///< CIE XYZ color space
        type_LAB,    ///< CIE 1976 (L*, a*, b*) color space
        type_HSV,    ///< Hue, saturation, value
        type_YCbCr,  ///< Luminance, blue chrominance, red chrominance
        type_CMYK,   ///< Cyan, magenta, yellow, key

        /**
         * Any user defined color space, including any returned by
         * <tt>new_custom</tt>.
         **/
        type_custom
      };

      /**
       * Get the specified standard color space.
       */
      static ConstRef get(Type t);

      /**
       * Get the type of this color space. This type can be used to
       * distuguish between the built-in color spaces. A user defined
       * color space must return <tt>type_user</tt>.
       */
      virtual Type get_type() const { return type_custom; }

      /**
       * Get a short mnemonic that dscribes this color space. For
       * example "RGB" for the RGB color space. User defined color spaces
       * should attempt to provide a unique mnemonic, however since
       * the Archon image library does not use this information to
       * identify color spaces, it is not fatal if it is ambiguous.
       *
       * \param has_alpha If true, an "A" is added to the mnemonic.
       *
       * \return The mnemonic for this color space.
       */
      virtual std::string get_mnemonic(bool has_alpha = false) const = 0;


      /**
       * Get the ID of the specified channel. This is normally a
       * single capital letter that uniquely identifies the channel
       * within this color space. It may be longer that a single
       * letter, but it must consist entirely of digits and letters
       * and must always end with a letter. The ID "A" is reserved for
       * the alpha channel.
       *
       * \param channel_index The index of the channel whose ID you
       * want. If it is not in the range <tt>[0;n-1]</tt>, where \c n
       * is the number of channels, then "A" is returned.
       */
      virtual std::string get_channel_id(int channel_index) const = 0;


      /**
       * Get the name of the specified channel.
       *
       * \param channel_index The index of the channel whose name you
       * want. If it is not in the range <tt>[0;n-1]</tt>, where \c n
       * is the number of channels, then "alpha" is returned.
       */
      virtual std::string get_channel_name(int channel_index) const = 0;


      /**
       * Get the number of primary colors in this color space, which
       * is the same as the number of color channels.
       */
      virtual int get_num_primaries() const = 0;


      /**
       * Get the number of channels, which is the number of primaries
       * plus 1 if you pass true for <tt>has_alpha</tt>.
       */
      int get_num_channels(bool has_alpha) const ;


      /**
       * Just a quick way of detecting the RGB color space. The RGB
       * color space has special significance, since it is used as an
       * intermediate format when converting between color spaces, and
       * there is no direction conversion available.
       */
      virtual bool is_rgb() const { return false; }


      struct Converter
      {
        /**
         * Convert a number of memory consecutive pixels from one
         * color space to another. Each pixel (or color compound) is a
         * tuple of N memory consecutive words, where N is the number
         * of channels in the color space. The word type is implicit,
         * that is, determined at the time of acquisition of this
         * converter.
         *
         * \param n The number of color compounds (or pixels) to convert.
         *
         * \note The minimum size of the source and target buffers is
         * \c n times the number of channels in the color space, times
         * the number of bytes per word.
         */
        virtual void cvt(void const *source, void *target, size_t n) const throw() = 0;

        virtual ~Converter() {}
      };


      enum AlphaType
      {
        alpha_No,      ///< Neither source nor target has alpha.
        alpha_Keep,    ///< Both source and target has alpha.
        alpha_Add,     ///< Source has no alpha, target does, alpha is set to 1 everywhere.
        alpha_Discard, ///< Source has alpha, target does not, alpha channel is discarded.
        alpha_Merge    ///< Source has alpha, target does not, target is source blended with black.
      };


      /**
       * Retrieve a converter that converts pixels from this color
       * space to the standard RGB color space.
       *
       * \param w The type of each color component.
       *
       * \param a Specifies where an alpha channel is present, and if
       * it has to be removed, it specifies how to remove it. By
       * default it is assumed that neither the source nor the target
       * format has an alpha channel.
       *
       * \note The lifetime of the converter object ends when the
       * lifetime of this color space ends.
       */
      virtual Converter const &to_rgb(WordType w = word_type_UChar,
                                      AlphaType a = alpha_No) const = 0;

      Converter const &to_rgb(WordType w, bool source_has_alpha, bool target_has_alpha) const;

      /**
       * Retrieve a converter that converts pixels from the standard
       * RGBA color space to the color space that this class
       * represents.
       *
       * If this color space has no alpha channel, the RGBA input
       * shall be blended with black according to the standard
       * blending function <tt>a * c + (1-a) * b</tt> where \c a is
       * the alpha channel of the input, \c c is the red, green, or
       * blue channel of the input, and \c b is the corresponding
       * channel of 'black'. Since black has all channels equal to
       * zero, this reduces to <tt>a * c</tt>.
       *
       * \param t The type of each color component. Must be one of the
       * floating point types <tt>word_type_Float</tt>,
       * <tt>word_type_Double</tt>, or <tt>word_type_LngDbl</tt>.
       *
       * \note The lifetime of the converter object ends when the
       * lifetime of this color space ends.
       */
      virtual Converter const &from_rgb(WordType w = word_type_UChar,
                                        AlphaType a = alpha_No) const = 0;

      Converter const &from_rgb(WordType w, bool source_has_alpha, bool target_has_alpha) const;

      virtual Converter const &to_self(WordType w = word_type_UChar,
                                       AlphaType a = alpha_No) const = 0;

      Converter const &to_self(WordType w, bool source_has_alpha, bool target_has_alpha) const;

      virtual Converter const *to_any(ColorSpace const *, WordType, AlphaType) const = 0;

      /**
       * If available, retrieve a converter from this color space to
       * the specified color space. If the conversion is not
       * available, null is returned.
       *
       * Any conversion that is available using \c native_to_rgba
       * and \c rgba_to_native shall also be availble using this method,
       * and the trivial conversion from a color space to itself,
       * shall also always be available.
       *
       * Further more, it is recommended that a color space without
       * alpha channel, provides a conversion to the corresponding
       * color space with alpha channel, if such a color space
       * exists. This conversion shall set the alpha value to full
       * intensity. Similarly, if this color space has an alpha
       * channel, and a corresponmding color space exists without
       * alpha channel, it is recommended that a conversion is made
       * available. In this case the input must be blended with
       * 'black'. All standard color spaces must provide these
       * recommended conversions.
       *
       * A color space is free to provide any conversion not mentioned
       * above.
       *
       * \note The ownership of the returned object, is \b not
       * transferred to the caller. Thus, the caller must not attempt
       * to delete it.
       */
      Converter const *to_any(ColorSpace::ConstRefArg c, WordType w = word_type_UChar,
                              AlphaType a = alpha_No) const;

      Converter const *to_any(ColorSpace::ConstRefArg c, WordType w,
                              bool source_has_alpha, bool target_has_alpha) const;


      /**
       * This method will return alpha_Merge if the source format has
       * alpha and the target does not.
       */
      static AlphaType get_alpha_type(bool source_has_alpha, bool target_has_alpha);


      virtual ~ColorSpace() {}

      /**
       * The maximum allowable number of primaries for any color
       * space. This limit is needed because the image manipulation
       * routines need to know in advance how much memory to allocate
       * to be able to handle at least one pixel of the widest
       * possible kind.
       *
       * Magically we choose 4096 bytes as a reasonable buffer size
       * and since we need to be able to handle the case where each
       * channel is a <tt>long double</tt>, which is generally 16
       * bytes on a 64-bit platform, we arrive at a value of 4096 / 16
       * = 256. We choose one less to make room for an alpha channel.
       */
      static const int max_num_primaries = 255;


    private:
      struct TypeSpec { static Core::EnumAssoc map[]; };

    public:
      typedef Core::Enum<Type, TypeSpec> TypeEnum;
    };






    // Implementation:

    inline int ColorSpace::get_num_channels(bool has_alpha) const
    {
      return get_num_primaries() + (has_alpha?1:0);
    }

    inline ColorSpace::AlphaType ColorSpace::get_alpha_type(bool source_has_alpha, bool target_has_alpha)
    {
      return source_has_alpha ?
        target_has_alpha ? ColorSpace::alpha_Keep : ColorSpace::alpha_Merge :
        target_has_alpha ? ColorSpace::alpha_Add  : ColorSpace::alpha_No;
    }

    inline ColorSpace::Converter const &ColorSpace::to_rgb(WordType w, bool source_has_alpha,
                                                           bool target_has_alpha) const
    {
      return to_rgb(w, get_alpha_type(source_has_alpha, target_has_alpha));
    }

    inline ColorSpace::Converter const &ColorSpace::from_rgb(WordType w, bool source_has_alpha,
                                                             bool target_has_alpha) const
    {
      return from_rgb(w, get_alpha_type(source_has_alpha, target_has_alpha));
    }

    inline ColorSpace::Converter const &ColorSpace::to_self(WordType w, bool source_has_alpha,
                                                            bool target_has_alpha) const
    {
      return to_self(w, get_alpha_type(source_has_alpha, target_has_alpha));
    }

    inline ColorSpace::Converter const *ColorSpace::to_any(ColorSpace::ConstRefArg c,
                                                           WordType w, AlphaType a) const
    {
      return to_any(c.get(), w, a);
    }

    inline ColorSpace::Converter const *ColorSpace::to_any(ColorSpace::ConstRefArg c,
                                                           WordType w, bool source_has_alpha,
                                                           bool target_has_alpha) const
    {
      return to_any(c, w, get_alpha_type(source_has_alpha, target_has_alpha));
    }
  }
}

#endif // ARCHON_IMAGE_COLOR_SPACE_HPP
