/**
 * \file
 *
 * \author Kristian Spangsege
 */

#ifndef ARCHON_IMAGE_PIXEL_FORMAT_HPP
#define ARCHON_IMAGE_PIXEL_FORMAT_HPP

#include <limits>
#include <stdexcept>
#include <vector>
#include <string>

#include <archon/image/color_space.hpp>

namespace archon
{
  namespace image
  {
    /**
     * A description of a particular pixel format applicable when
     * storing an image in a memory buffer organized as one or more
     * sequences of pixels (frequently as a sequence of pixel rows.)
     *
     * This class describes how sequences of pixels are stored in
     * memory. It does not however concern itself with specifying how
     * such sequences make up an entire image. In particular it does
     * not describe how the order of pixels in memory relate to the
     * order of pixels in the image (eg. whether pixels are store
     * right-to-left or left-to-right or whether they are stored in
     * row-major or in column-major order.)
     *
     *
     * <h3>Storage model</h3>
     *
     * The general model is that a sequence of pixels is stored in a
     * sequence of words of some configurable size and type. Each
     * pixel requires a fixed number of bits and thus may require any
     * integer or non-integer number of words. In particular a pixel
     * may require less than one word, allowing several pixels to be
     * stored in a single word. Most practical pixel formats, however,
     * will require an integer number of words for reasons of
     * addressability.
     *
     *
     * <h3>Endianness</h3>
     *
     * The native endianess of the hardware platform determines how
     * bytes are assembled into words. On the other hand, the native
     * endianess has no impact on how words are assembled into
     * pixels. This aspect is completely described by this pixel
     * format. See also the more elaborate description of memory
     * layout below.
     *
     *
     * <h3>Word types and size</h3>
     *
     * Generally integer values are used when encoding the value of
     * the individual channels of a pixel. More exotic formats use
     * foating point values instead. This class supports both kinds
     * although in the case of floating point values severe
     * limitations apply. See the next section for details.
     *
     * The word type may be chosen freely among those supported by
     * your platform. In the case of integer based formats, one may
     * choose any of the following: char, short, int, long or
     * max_int. The first four types correspond with the types of the
     * same name from the C ISO standard. The last one 'max_int'
     * refers to the widest integer type available on your platform
     * (on GNU based platforms this corresponds to the uintmax_t
     * type.) In the case of floating point base formats one may
     * choose between: float: double and long double.
     *
     * There are two different ways of specifying the word type,
     * either by name or by bit width. In the latter case one must
     * also specify whether the format is integer or floating point
     * based.
     *
     *
     * <h3>Word alignment / addressability</h3>
     *
     * There are several alignment properties which may or may not be
     * posessed by a particular pixel format. These properties are
     * important since they express how difficulty it is to access the
     * individual channels or color components of a pixel.
     *
     * Alignment:
     *
     * 3 Every pixel consists of one or more full words.
     * 4 Every channel consists of one or more full words.
     *
     * Indivisibility:
     *
     * 1 Channels always fall completely within a single word.
     * 2 Pixels always fall completely within a single word.
     *
     * 2 implies 1.
     * 4 implies 3.
     *
     * A packed format is one that posesses property 2 and 3.
     * A direct format is one that posesses property 1 and 4.
     *
     *
     * <pre>
     *
     *    Alignment    Indivisibility | Accessability    Format type
     *   ----------------------------------------------------------------------
     *    none         none           |      0           Generic
     *    none         channel        |      1           Undivided channels
     *    none         pixel          |      2           Undivided pixels
     *    pixel        none           |      3           Addressible pixels
     *    pixel        channel        |      4           Multi word packed
     *    pixel        pixel          |      5           Packed
     *    channel      none           |      6           Multi word channels
     *    channel      channel        |      7           Direct
     *    channel      pixel          |      8           One channel direct
     *
     * </pre>
     *
     * Tight formats totally ignore word boundaries. Memory is simply
     * seen as a sequence of bits, and pixels are placed tightly
     * (without gaps) in this bit stream. For formats using relativly
     * few bits per pixel, this type of format will genrally be the
     * least memory hungy one. On the other hand, access to a
     * particular component of a particular pixel is cumbersome, in
     * that it requires a lot of calculations and bit-shifting.
     *
     * Packed formats are characterized by the fact that each pixel
     * begins on a word boundary. Thus one can point to a pixel simply
     * by using a memory address. This property is important since it
     * is an assumption made by many image handling libraries and
     * applications. Generally it also allows more efficient access to
     * the pixels and their components.
     *
     * Note that bit-shifting is required when accessing pixel
     * components of all pixel formats except those posessing the
     * highest level of addressability.
     *
     * Since bit-shifting is meaningless on words of floating point
     * type, pixel formats may be floating point based only when they
     * posess the highest level of addressability.
     *
     *
     * <h3>Pixel sequence layout</h3>
     *
     * The \c mostSignificantBitsFirst flag selects the bit order that
     * applies to your image data.
     *
     * Set this flag if you consider the most significant bit of a
     * word to be the first bit in that word. Clear it if you instead
     * consider the leat significant bit to be the first one.
     *
     * This flag affects pixel encoding/decoding in the following
     * ways:
     *
     * <UL>
     *
     *   <LI>Order of pixels within a word: If two pixels A and B
     *   fall within a single word and A is before B when decoded,
     *   then by default A uses bits of lesser significance
     *   than those used by B (least significant bits first.) When
     *   setting this flag, A uses bits of greater significance than
     *   those used by B (most significant bits first.)</LI>
     *
     *   <LI>Order of channels within a word: If a pixel format has
     *   multiple channels and two channels A and B of a single
     *   pixel fall within a single word and A is before B in the
     *   canonical order (decoded order) then by default A uses bits
     *   of lesser significance than those used by B (least
     *   significant bits first.) When setting this flag A uses bits
     *   of greater significance than those used by B (most
     *   significant bits first.)</LI>
     *
     *   <LI>Order of words within a channel: When a channel is
     *   distributed ocross multiple words then by default the word
     *   with the lowest address holds the least significant bits of
     *   the channel, and the word with the highest address holds
     *   the most significant bits (least significant bits first.)
     *   When setting this flag the word with the lowest address
     *   holds the most significant bits of the channel (most
     *   significant bits first.)</LI>
     *
     *   <LI>If a channel in a direct format has a bit-width W which
     *   is less than the bit-width of a word, then by default the W
     *   least significant bits of the word are used. When setting
     *   this flag it causes the W most significant bits to be used
     *   instead.</LI>
     *
     * </UL>
     *
     *  The following is an example of the default bit ordering
     *  (least significant bits first) using a tight format with
     *  channels red, green and blue each using three bits (r0 is
     *  the least significant bit of the red channel) and the
     *  canonical channel order being as listed (red comes before
     *  green when decoded) and a word size of 8 bits (w0 is the
     *  least significant bit of the word):
     *
     * <PRE>
     *
     *   |      pixel offset 0      |      pixel offset 1      |
     *   |                          |                          |
     *   |r0 r1 r2|g0 g1 g2|b0 b1 b2|r0 r1 r2|g0 g1 g2|b0 b1 b2|...
     *   |        |        |        |        |        |        |
     *   ----------------------------------------------------------
     *   |                       |                       |
     *   |w0 w1 w2 w3 w4 w5 w6 w7|w0 w1 w2 w3 w4 w5 w6 w7|.........
     *   |                       |                       |
     *   |     word offset 0     |     word offset 1     |
     *
     * </PRE>
     *
     * The next example is like the previous one except that the bit
     * ordering is now reversed (most significant bits first):
     *
     * <PRE>
     *
     *   |      pixel offset 0      |      pixel offset 1      |
     *   |                          |                          |
     *   |r2 r1 r0|g2 g1 g0|b2 b1 b0|r2 r1 r0|g2 g1 g0|b2 b1 b0|...
     *   |        |        |        |        |        |        |
     *   ----------------------------------------------------------
     *   |                       |                       |
     *   |w7 w6 w5 w4 w3 w2 w1 w0|w7 w6 w5 w4 w3 w2 w1 w0|.........
     *   |                       |                       |
     *   |     word offset 0     |     word offset 1     |
     *
     * </PRE>
     *
     * \note The bit order never affect decoding of individual
     * component values, That is, when a number of consecutive bits
     * inside a word make up a channel value, the most significant
     * bit in the decoded channel value will always correspond to
     * the most significant bit in the word.
     *
     *
     * <h3>Color spaces</h3>
     *
     * This class supports pixel formats with any number of channels
     * (color components.)
     *
     * WordType
     *
     * There are three interesting alignment characheristica:
     *
     * Every channel is aligned to a word
     * Every pixel is aligned to a word
     * No word alignment
     *
     * PARTIALLY OBSOLETE:
     *
     * <h3>Channel order</h3>
     *
     * The pixel format describes how to decode pixel data into a
     * sequence of one or more channels. If there are more than one
     * channel it must also describe how to decode them into the
     * connonical order applying to the relevant color space. For
     * custom color spaces the cannonical order is defined by
     * you. Otherwise they are as follows:
     *
     * <PRE>
     *
     *   l     Luminance (trivial)
     *   rgb   Red, Green, Blue
     *   hsv   Hue, Saturation, Value
     *
     * </PRE>
     *
     * If the pixel data contains an alpha channel, that channel
     * must always be last in decoded pixel.
     *
     * \todo Make a formal format for string representation of pixel
     * formats and a parser.
     */
    struct PixelFormat: virtual core::CntRefObjectBase, core::CntRefDefs<PixelFormat>
    {
      /**
       * Interpret image data as a sequence of words of this type. The
       * formation of words from bytes in the image buffer is affected
       * by the chosen byte order.
       *
       * Floating point word types can only be used with direct
       * formats.
       *
       * \sa ByteOrder
       */
      enum WordType
      {
	std_char,        ///< Characters
	std_short,       ///< Low precision integer numbers
	std_int,         ///< Normal precision integer numbers
	std_long,        ///< High precision integer numbers
	std_max_int,     ///< Ultra precision integer numbers
	std_float,       ///< Low precision floating point numbers
	std_double,      ///< Normal precision floating point numbers
	std_long_double  ///< High precision floating point number
      };


      /**
       * Describes a single channel
       */
      struct Channel
      {
	/**
	 * The channel offset in number of bits.
	 */
	int offset;

	/**
	 * The channel width in number of bits.
	 */
	int width;

	Channel(int offset, int width): offset(offset), width(width) {}

	bool operator==(Channel const &c) const
	{
	  return offset == c.offset && width == c.width;
	}
      };


      /**
       * Thrown when a format specification is found to be unsupported
       * by the current hardware platform, incompletely specified, or
       * inconsistent.
       */
      struct InvalidFormatException: std::runtime_error
      {
	InvalidFormatException(std::string m): std::runtime_error(m) {}
      };


      typedef uintmax_t MaxInt;

      static int const bitsPerChar = std::numeric_limits<unsigned char>::digits;

      /**
       * Get a new default format. The default format has a word type
       * of \c char which in this context is always the same as a
       * byte. It uses the RGBA color space with each component using
       * precisely one byte, and with the canonical channel
       * order. That is, each pixel uses 4 bytes which on almost all
       * platforms correspond to 32 bits.
       */
      static Ref newDefaultFormat();

      /**
       * Constructor for most luminance formats. If you format cannot
       * be described by the arguments of this constructor, please
       * resort to using a more general constructor.
       *
       * \param luminanceWidth The bit width of the luminance channel.
       *
       * \param alphaWidth The bit width of the alpha channel or zero
       * if the format has no alpha channel.
       *
       * \param bitsPerPixel The number of bits per pixel including
       * any unused bits following the channels. In other words, this
       * is the difference between the positions of the first bit of
       * two consecutive pixels. If there are no unused bits in your
       * format, you may pass zero.
       */
      static Ref newLuminanceFormat(int luminanceWidth=bitsPerChar,
                                    int alphaWidth=0,
                                    bool reverseChannelOrder=false,
                                    int bitsPerWord=0,
                                    bool floatingPointWords=false,
                                    bool mostSignificantBitsFirst=false,
                                    int bitsPerPixel=0);

      /**
       * Constructor for most RGB formats. If you format cannot
       * be described by the arguments of this constructor, please
       * resort to using a more general constructor.
       *
       * \param redWidth The bit width of the red channel.
       *
       * \param greenWidth The bit width of the green channel.
       *
       * \param blueWidth The bit width of the blue channel.
       *
       * \param alphaWidth The bit width of the alpha channel or zero
       * if the format has no alpha channel.
       *
       * \param bitsPerPixel The number of bits per pixel including
       * any unused bits following the channels. In other words, this
       * is the difference between the positions of the first bit of
       * two consecutive pixels. If there are no unused bits in your
       * format, you may pass zero.
       */
      static Ref newRgbFormat(int redWidth=bitsPerChar,
                              int greenWidth=bitsPerChar,
                              int blueWidth=bitsPerChar,
                              int alphaWidth=0,
                              bool reverseChannelOrder=false,
                              int bitsPerWord=0,
                              bool floatingPointWords=false,
                              bool mostSignificantBitsFirst=false,
                              int bitsPerPixel=0);

      /**
       * Constructor for most HSV formats. If you format cannot
       * be described by the arguments of this constructor, please
       * resort to using a more general constructor.
       *
       * \param hueWidth The bit width of the hue channel.
       *
       * \param saturationWidth The bit width of the saturation channel.
       *
       * \param valueWidth The bit width of the value channel.
       *
       * \param alphaWidth The bit width of the alpha channel or zero
       * if the format has no alpha channel.
       *
       * \param bitsPerPixel The number of bits per pixel including
       * any unused bits following the channels. In other words, this
       * is the difference between the positions of the first bit of
       * two consecutive pixels. If there are no unused bits in your
       * format, you may pass zero.
       */
      static Ref newHsvFormat(int hueWidth=bitsPerChar,
                              int saturationWidth=bitsPerChar,
                              int valueWidth=bitsPerChar,
                              int alphaWidth=0,
                              bool reverseChannelOrder=false,
                              int bitsPerWord=0,
                              bool floatingPointWords=false,
                              bool mostSignificantBitsFirst=false,
                              int bitsPerPixel=0);

      /**
       * General constructor for formats without gaps between channels
       * and with canonnical or reverse canonnical channel order.
       *
       * \param channelWidths Bit width of each channel in canonnical
       * order.
       */
      static Ref makeFormat(ColorSpace::ConstRefArg colorSpace,
                            int bitsPerChannel=bitsPerChar,
                            bool reverseChannelOrder=false,
                            int bitsPerWord=0,
                            bool floatingPointWords=false,
                            bool mostSignificantBitsFirst=false,
                            int bitsPerPixel=0);

      /**
       * General constructor for formats without gaps between channels
       * and with canonnical or reverse canonnical channel order.
       *
       * \param channelWidths Bit width of each channel in canonnical
       * order. So for the RGB space the first element is always the
       * width of the red channel.
       */
      static Ref makeFormat(std::vector<int> const &channelWidths,
                            ColorSpace::ConstRefArg colorSpace=core::CntRefNullTag(),
                            bool reverseChannelOrder=false,
                            int bitsPerWord=0,
                            bool floatingPointWords=false,
                            bool mostSignificantBitsFirst=false,
                            int bitsPerPixel=0);

      /**
       * Completely general format constructor allowing specification
       * of both channel widths, order and gaps.
       *
       * \param channelLayout A description of each channel in
       * canonical (decoded) order. For predefined color spaces the
       * canonical order is as specified in its name. For custom color
       * schems the canonical order is defined by the application. The
       * passed vector must describe at least one channel.
       *
       * \param colorSpace The color space (or color space) to base
       * this pixel format on. If unspecified, it will be
       * automatically determined based on the number of channels in
       * the layout. One channel yields the luminance only color
       * space, 2 channels adds an alpha channel, 3 channels yields
       * the RGB space, and 4 channels adds an alpha channel such
       * that we get RGBA. If both \c channelLayout and \c colorSpace
       * is specified, they must agree on the num,ber of channels.
       *
       * \param bitsPerWord The number of bits per word, where a word
       * is to be understood as the C/C++ data type from which the
       * image data buffer is comprised. This is typically a \c char,
       * but it may also be a larger multi-byte integer or even a
       * floating point type. If zero, the \c float type is used when
       * \c floatingPointWords is true, otherwise the \c{unsigned
       * char} type is used.
       *
       * \param floatingPointWords If true, the selected word type
       * will be a floting point type. Otherwise it will be an
       * unsigned integer type.
       *
       * \param mostSignificantBitsFirst This flag selects the bit
       * order that applies to your image data. Set this flag if you
       * consider the most significant bit of a word to be the first
       * bit in that word. Clear it if you instead consider the leat
       * significant bit to be the first one. See also "pixel sequence
       * layout" in the class documentation.
       *
       * \param bitsPerPixel The number of bits per pixel. If this is
       * not an integer multiple of the effective number of bits per
       * word, then some pixels will not start at a word boundary, and
       * this might have a negative impact on performance. See also
       * "pixel sequence layout" in the class documentation. If zero,
       * it will be set to the number of bits spanned by the channel
       * layout rounded up to the nearest word boundary.
       *
       * \throw InvalidFormatException If the specified format is not
       * supported by this hardware platform, inclompletely specified,
       * or inconsistent.
       */
      static Ref makeFormat(std::vector<Channel> const &channelLayout,
                            ColorSpace::ConstRefArg colorSpace=core::CntRefNullTag(),
                            int bitsPerWord=0,
                            bool floatingPointWords=false,
                            bool mostSignificantBitsFirst=false,
                            int bitsPerPixel=0);


      /**
       * Get a string representation of this pixel format.
       *
       *
       * Examples: rgb, float_rgb, int16_rgb, r3g2b3, b2g2r2_msb, b2_1g2_1r2, c_fido_int8_b8e2d2_1b2a1_msb
       *
       * <PRE>
       *
       *   foramt = simple_format                    (standard color space)
       *          | 'c_' color_space simple_format  (custom color space)
       *
       *   simple_format = word_type_opt channel_layout bit_order_opt
       *
       *   word_type_opt = word_type '_'
       *                 | empty          ('char' type is assumed)
       *
       *   bit_order_opt = '_' bit_order
       *                 | empty
       *
       *   word_type = 'char' | 'short' | 'int' | 'long' | 'max_int'
       *             | 'float' | 'double' | 'long_double'
       *             | 'int' word_size    (custom sized integer)
       *             | 'float' word_size  (custom sized float)
       *
       *   bit_order = 'lsb'   (least significant bits first - default)
       *             | 'msb'   (most significant bits first)
       *
       *
       *   channel_layout = field
       *                  | field channel_layout
       *
       *   field = '_' bit_width          (unused bits)
       *   field = channel bit_width 
       *         | channel                (word sized field with word alignment)
       *
       *   channel = ANY SINGLE ASCII LETTER
       *
       *   bit_width = ANY MULTI DIGIT POSITIVE INTEGER
       *
       * </PRE>
       *
       * The format string is not case sensitive.
       *
       * \return A string describing this pixel format.
       */
/*
      std::string toString() const;
*/

/*
      bool operator==(PixelFormat const &f) const
      {
	return
	  wordType                 == f.wordType                 &&
	  channelLayout            == f.channelLayout            &&
	  bitsPerPixel             == f.bitsPerPixel             &&
	  mostSignificantBitsFirst == f.mostSignificantBitsFirst &&
	  colorSpace               == f.colorSpace;
      }
*/

    private:

      std::vector<Channel> channelLayout;
      ColorSpace::ConstRef colorSpace;
      WordType wordType;
      bool mostSignificantBitsFirst;
      int bitsPerPixel;


      PixelFormat(std::vector<Channel> const &channelLayout,
                  ColorSpace::ConstRefArg colorSpace,
                  int bitsPerWord, bool floatingPointWords,
                  bool mostSignificantBitsFirst, int bitsPerPixel);
    };
  }
}

#endif // ARCHON_IMAGE_PIXEL_FORMAT_HPP
