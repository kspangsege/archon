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

#ifndef ARCHON_IMAGE_INTEGER_BUFFER_FORMAT_HPP
#define ARCHON_IMAGE_INTEGER_BUFFER_FORMAT_HPP

#include <algorithm>


#include <iostream>



#include <archon/image/word_based_buffer_format.hpp>


namespace archon
{
  namespace Imaging
  {
    /**
     * A highly flexible buffer format implementation where pixels are
     * encoded into a sequence of integer words of arbitrary width.
     *
     * This buffer format, like any other buffer format, does two
     * things; first it allows you to specify a particular memory
     * layout of pixels when constructing a new instance of
     * <tt>BufferedImage</tt>. The other thing it does, is to provide
     * a codec object (encoder and decoder functions) that the image
     * instance uses to read and write pixels.
     *
     * This buffer format is a specialization of \c
     * WordBasedBufferFormat in that it requires the word type to be
     * an integer. Any integer from the \c WordType enumeration will
     * be accepted.
     *
     * On the positive side, this buffer format provides extensive
     * flexibility in the desired layout of pixels and channels within
     * memory. Here is a quick list of features:
     *
     * - Any number of channels are supported.
     *
     * - Multiple channels may be packed into a single word.
     *
     * - Multiple pixels may be packed into a single word.
     *
     * - Any channel may span multiple words.
     *
     * - Neither pixel nor channel boundaries need to be aligned on
     *   word boundaries.
     *
     * - Any channel order can be achieved.
     *
     * - The format allows specification of unused bits before,
     *   between, and after channels.
     *
     * - Almost arbitrary endianness specification is allowed when
     *   each word consists of multiple bytes.
     *
     * - When pixels are not themselves word aligned, one may specify
     *   that each strip (or row) of pixels is.
     *
     *
     * It may be rather daunting for the newcomer to figure out
     * exactly how the different parameters of this format affect the
     * final layout of pixels and channels. Let us first have a look
     * at each parameter in turn, and then consider the precise
     * procedure that one must use to map those parameters to a
     * particular layout in memory:
     *
     * - <tt>word_type</tt> specifies the type of the smallest unit of
     *   memory that this format concerens itself with. It must be an
     *   unsigned integer type. Logically, the bits of each word are
     *   combined into one long sequence of bits, and pixels and
     *   channels are layed out inside this bit sequence without
     *   regard to word boundaries. In some contexts a word is
     *   understood specifically as a 16 bit entity. Note that this
     *   interpretation does not apply here.
     *
     * - <tt>bits_per_pixel</tt> is the number of bits consumed by
     *   each pixel. The number of bits, consumed by a sequence of N
     *   pixels within a row of the image, is exactly N times
     *   <tt>bits_per_pixel</tt>. Any value greater than or equal to 1
     *   is accepted, regardless of the word type.
     *
     * - <tt>channel_layout</tt> specifies, for each channel, a
     *   particular range of bits to be used by that channel. The
     *   range is specified as an offset and a width, where the offset
     *   is the position of the first bit, counting from the start of
     *   the pixel bit range, and the width is the number of bits used
     *   to encode the channel.
     *
     * - <tt>most_sig_bit_first</tt> is a flag that specifies whether
     *   the most, or the least significant bit should be considered
     *   as coming first in general. For example, when two
     *   horizontally adjacent pixels are packed into a single word,
     *   this flag controls whether the first (left-most) pixel
     *   occupies the least or the most significant bits. Likewise, if
     *   a single channel spans multiple words, this flag controls
     *   whether the first word in memory delivers the least or the
     *   most significant bits to the channel.
     *
     * - <tt>word_align_strips</tt> is a flag that specifies whether
     *   the first pixel of each row in the image is aligned on a word
     *   boundary. Of course, this makes a difference only when
     *   <tt>bits_per_pixel</tt> is not a multiple of the number of
     *   bits per word. The main reason that one would want such an
     *   alignment, is that it guarantees addressability of the
     *   individual row.
     *
     * - <tt>endianness</tt> is a specification of the byte order that
     *   applies when reading and writing words from/to memory. The
     *   two absolutely most common endianness variants are 'least
     *   significant byte first' and 'most significant byte first',
     *   however, there exist exotic variants that are a mixture of
     *   the two. This specification is general enough to cover almost
     *   any conceivable endianness. See
     *   <tt>core::compare_endianness</tt> for more details on the
     *   representation. In any case, it is important to undertand
     *   that if the specified endianness does not match the native
     *   endianness of the platform, a severe run-time penaly applies
     *   when pixels are encoded or decoded, simply because a regular
     *   memory access is far more efficient that when it has to be
     *   accompanied by an extra 'manual' byte swapping step.
     *
     *
     * With this, we should be ready for a complete and precise
     * description of the synthesis of the meory layout based on the
     * above parameters.
     *
     * The synthesis proceeds in two logical steps:
     *
     * <h3>Building a bit sequence from words</h3>
     *
     * The underlying memory buffer is viewed as a sequence of words
     * whose type is specified by <tt>word_type</tt>. All the words in
     * the buffer are logically combined into one long sequence of
     * bits. Let N be the number of bits per word, then the first N
     * bits of the combined bit sequence is delivered by the buffer
     * word with the lowest memory address. The next N bits are
     * delivered by the next word in memory (the one at the next
     * lowest address), and so on.
     *
     * If <tt>most_sig_bit_first</tt> is true, then the word bits are
     * added to the combined bit sequence in order of decreasing
     * significance, that is, the most significant bit of the word is
     * added first. If <tt>most_sig_bit_first</tt> is false, the bits
     * are instead added in order of increasing significance.
     *
     * When a word consists of multiple bytes, the specified
     * <tt>endianness</tt> determines how bytes are combined into
     * words, and therefore it determines, which byte delivers the
     * least significant bits of thw word, and which byte delivers the
     * most significant bits. Please see
     * <tt>core::compare_endianness</tt> for the complete story about
     * endianness and the description of it, used in this context.
     *
     *
     * <h3>Laying out pixels in the bit sequence</h3>
     *
     * The next thing that happens (logically), is that the bit
     * sequence, constructed above, is chopped into pixel sized
     * pieces. Let M be the specified number of bits per pixel. The
     * process begins with the lower left pixel of the image, which
     * uses the first M bits of the sequence. The next pixel is the
     * one immediately to the right of the first one (assuming that
     * the image width is at least two), and it uses the next M bits
     * of the sequence. This process continues until we reach the
     * leftmost pixel of the bottom-most row.
     *
     * At this point, one or more bits might be skipped, before
     * continuing with the leftmost pixel of the next pixel row (the
     * one immediately above the bottom-most one). Bits will be
     * skipped if, and only if <tt>word_align_strips</tt> is true and
     * the first bit of the remaining bit sequence comes from the same
     * word as the last consumed bit. In this case, all the remaining
     * bits of that word will be skipped, and the first bit consumed
     * by the next pixel is the first bit in the sequence belonging to
     * the next word. As can be seen, if P bits are skipped after the
     * first row, then P bits will be skipped after every row.
     *
     * The final part of the synthesis is the layout of channels
     * within pixels. Each channel mentioned in
     * <tt>channel_layout</tt> specifies a bit offset, Q, as well as a
     * bit width, W. Together they specify a sub-range of bits of the
     * pixel, that are used by the corresponding channel. The offset
     * is the position within the pixel of the first bit of the
     * sub-range. Altogether, the sub-range consists of the bits at
     * positions Q up to, but not including Q+W. Note that we still
     * view each pixel as a piece of the bit sequence that it was
     * extracted from, so if we let I be the overall index of the
     * pixel in the image, then the position of the first bit of the
     * sub-range, within the original bit sequence, is
     * <tt>I*bits_per_pixel+O</tt> (disregarding row alignment).
     *
     * Each channel is constructed from the corresponding sub-range of
     * pixel bits as follows: Starting with the first bit in the
     * sub-range, each one in turn, is associated with a bit position
     * in the corresponding channel. If <tt>most_sig_bit_first</tt> is
     * true, then the first sub-range bit will be associated with the
     * most significant bit of the channel, the the remaining channel
     * bit positions will be associated in order of decreasing
     * significance. If <tt>most_sig_bit_first</tt> is false, the
     * association will instead start with the least significant
     * channel bit position, and proceed in order of increasing
     * significance.
     *
     *
     * <h3>An example</h3>
     *
     * The following is an example of a layout where
     * <tt>most_sig_bit_first</tt> is false. Also, it has three
     * channels, red, green, and blue, each using 3 bits. The red
     * channel is at bit offset 0, green is at offset 3, and blue is
     * at offset 6. the number of bits per pixel is 9 and the number
     * of bits per word is 8. Note finally that r0 is used to indicate
     * the least significant bit of the red channel, while w0 is used
     * to indicate the least significant bit of a word:
     *
     * <pre>
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
     * </pre>
     *
     *
     * The next example is like the previous one except that the bit
     * ordering is now reversed, that is, <tt>most_sig_bit_first</tt>
     * is now true (red channel is still at bit offset 0):
     *
     * <pre>
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
     * </pre>
     *
     *
     * Note that I have adopted the convention that memory adresses
     * always increase from left to right. A consequence of this is
     * that we must also think of the offset into the bit sequence as
     * moving to the right when it is increased. Under this
     * convention, if \c most_sig_bit_first is true, the most
     * significant bit of a word is located to the left, otherwise it
     * it located to the right. Likewise for the most significant bit
     * of a channel.
     *
     * Notice that the bit order never affects the decoding of
     * individual component values, That is, when a number of
     * consecutive bits inside a word make up a channel value, the
     * most significant bit in the decoded channel value will always
     * correspond to the most significant bit in the covered region of
     * the word.
     *
     * The following is a list of some of the consequences of bit
     * order choice:
     *
     * - Order of pixels within a word: If two pixels A and B fall
     *   within a single word and A is before B with respect to the
     *   order of pixels in an image, then if \c most_sig_bit_first is
     *   false, A uses bits of lesser significance than those used by
     *   B. Otherwise A uses bits of greater significance than those
     *   used by B.
     *
     * - Order of channels within a word: If a buffer format has
     *   multiple channels and two channels A and B of a single pixel
     *   fall within a single word, and A is before B with respect to
     *   their positions in the constructed bit sequence, then if \c
     *   most_sig_bit_first is false, A uses bits of lesser
     *   significance than those used by B. Otherwise A uses bits of
     *   greater significance than those used by B.
     *
     * - Order of words within a channel: When a channel spans
     *   multiple words, then if \c most_sig_bit_first is false, the
     *   word with the lowest address holds the least significant bits
     *   of the channel, and the word with the highest address holds
     *   the most significant bits.  Otherwise the word with the
     *   lowest address holds the most significant bits of the
     *   channel.
     */
    struct IntegerBufferFormat: WordBasedBufferFormat
    {
      // We need to define these manually to avoid ambiguity with the
      // cousins in BufferFormat.
      typedef core::CntRef<IntegerBufferFormat>        Ref;
      typedef core::CntRef<IntegerBufferFormat const>  ConstRef;
      typedef Ref                               const &RefArg;
      typedef ConstRef                          const &ConstRefArg;


      /**
       * Get the number of bits per pixel. Think of the bits of each
       * word in the buffer as layed out in one long bit sequence,
       * then the number of bits per pixel is the distance from the
       * first bit of one pixel to the first bit of the next pixel.
       */
      int get_bits_per_pixel() const { return channel_layout.bits_per_pixel; }


      /**
       * \throw ImageSizeExcpetion If this buffer format does not
       * support an image of the specified width.
       */
      int get_bits_per_strip(int width) const;


      /**
       * NOTE: Each strip is only guaranteeds to take up an integer
       * number of bytes if this format has word aligned strips. In
       * any case, this method shall return the value of
       * <tt>floor(bits_per_strip/bits_per_byte)</tt>.
       *
       * \throw ImageSizeExcpetion If this buffer format does not
       * support an image of the specified width.
       */
      int get_bytes_per_strip(int width) const;


      /**
       * NOTE: Each strip is only guaranteeds to take up an integer
       * number of bytes if this format has word aligned strips. In
       * any case, this method shall return the value of
       * <tt>floor(bits_per_strip/bits_per_word)</tt>.
       *
       * \throw ImageSizeExcpetion If this buffer format does not
       * support an image of the specified width.
       */
      int get_words_per_strip(int width) const;


      /**
       * Get the offset of the first bit of the specified channel
       * within a pixel part of the composed bit sequence.
       */
      int get_channel_offset(int index) const;

      bool get_most_sig_bit_first() const { return most_sig_bit_first; }

      bool get_word_align_strips() const { return word_align_strips; }


      struct Channel
      {
        int bit_offset;
        int bit_width;
        Channel(int o, int w): bit_offset(o), bit_width(w) {}
        bool operator==(Channel const &c) const;
      };

      struct ChannelLayout
      {
        int bits_per_pixel; // get_bits_per_strip assumes this is an int
        typedef std::vector<Channel> Channels;
        Channels channels;

        /**
         * Construct an initially empty channel layout.
         */
        ChannelLayout(): bits_per_pixel(0) {}

        /**
         * Construct a channel layout with a single channel of the
         * specified width. There are no unused bits.
         *
         * \param w The width of the channel.
         */
        explicit ChannelLayout(int w): bits_per_pixel(0) { add(w); }

        /**
         * Construct a channel layout with two channels. The order of
         * the channels in the bit sequence is the same as the logical
         * order of the channels. There are no unused bits.
         *
         * \param w1, w2 The width of the first and second channel
         * respectively.
         */
        ChannelLayout(int w1, int w2): bits_per_pixel(0) { add(w1); add(w2); }

        /**
         * Construct a channel layout with three channels. The order of
         * the channels in the bit sequence is the same as the logical
         * order of the channels. There are no unused bits.
         *
         * \param w1, w2, w3 The width of the first, second, and third
         * channel respectively.
         */
        ChannelLayout(int w1, int w2, int w3): bits_per_pixel(0) { add(w1); add(w2); add(w3); }

        /**
         * Construct a channel layout with four channels. The order of
         * the channels in the bit sequence is the same as the logical
         * order of the channels. There are no unused bits.
         *
         * \param w1, w2, w3, w4 The width of the first, second,
         * third, and fourth channel respectively.
         */
        ChannelLayout(int w1, int w2, int w3, int w4);

        /**
         * Construct a one channel layout. The number of bits per
         * pixel will be set as low as possible.
         *
         * \param c The channel as it must appear in the layout.
         */
        explicit ChannelLayout(Channel const &c): bits_per_pixel(0) { add(c); }

        /**
         * Construct a two channel layout. The logical order of the
         * channels will correspond to the order they are
         * specified. The number of bits per pixel will be set as
         * low as possible.
         *
         * \param c1, c2 The first and second channel of the layout
         * (in that order).
         */
        ChannelLayout(Channel const &c1, Channel const &c2);

        /**
         * Construct a three channel layout. The logical order of the
         * channels will correspond to the order they are
         * specified. The number of bits per pixel will be set as
         * low as possible.
         *
         * \param c1, c2, c3 The first, second, and third channel of
         * the layout (in that order).
         */
        ChannelLayout(Channel const &c1, Channel const &c2, Channel const &c3);

        /**
         * Construct a four channel layout. The logical order of the
         * channels will correspond to the order they are
         * specified. The number of bits per pixel will be set as
         * low as possible.
         *
         * \param c1, c2, c3, c4 The first, second, third, and fourth
         * channel of the layout (in that order).
         */
        ChannelLayout(Channel const &c1, Channel const &c2, Channel const &c3, Channel const &c4);

        /**
         * Add a channel of the specified width to the current end of
         * the layout, expanding the number of bits per pixel by the
         * width of that channel.
         */
        void add(int w) { add(Channel(bits_per_pixel, w)); }

        /**
         * Add the specified channel to this layout increasing the
         * number of bits per pixel as necessary.
         */
        void add(Channel const &c);
      };



      /**
       * Get an instance of the integer buffer format that adheres to
       * the specified parameters.
       *
       * The returned buffer format may not use the exact same
       * parameters as the ones you specified, but it shall be
       * guaranteed that the returned format represents the exact same
       * layout of pixels in memory, as one that uses exactly the
       * specified parameters, and that this is true for any image
       * that the format can be used with.
       *
       * \param word_type Any integer word type. Interpret image data
       * as a sequence of words of this type. The formation of words
       * from bytes in the image buffer is affected by the byte order
       * / endianness in effect.
       *
       * \param bits_per_pixel Within the sequence of bits resulting
       * from word concattenation, each pixel consumes this number of
       * bits, or in other words, it is the difference in position,
       * within this bit sequence, of the first bit of two adjacent
       * pixels.
       *
       * \param channel_layout The number of entries in the specified
       * vector determinaes the number of channels in this
       * format. Each entry in the vector describes which range of
       * bits are used to encode that particular channel. The offset
       * is to be interpreted relative to the start of the pixel
       * within the sequence of bits resulting from word
       * concattenation. The bit ranges of the individual channels may
       * not overlap, and all ranges must be confined to the number of
       * bits per pixel. The buffer format itself, is not concerned
       * with the meaning of the individual channels, but when it is
       * used to create a buffered image (see
       * <tt>BufferedImage::new_image</tt>) the first entry in \c
       * channels becomes associated with the first primary color of
       * the color space of the new image, and so on.
       *
       * \param word_align_strips Set to true if each pixel strip must
       * start on a word boundary.
       */
      static Ref get_format(WordType word_type, ChannelLayout const &channel_layout,
                            bool most_sig_bit_first = false, bool word_align_strips = true,
                            std::vector<bool> const &endianness = std::vector<bool>());



      template<class T> struct Map
      {
        int add_channel_layout(ChannelLayout const &l);

        void add_format(WordType word_type, std::vector<bool> const &endianness,
                        bool most_sig_bit_first, int layout, T const &value);

        T const *find(ConstRefArg int_buf_fmt) const;

        void dump_info(std::ostream &out) const;

        Map(): buckets(strip_layout_hash_size) {}

      private:
        struct Fmt
        {
          unsigned bit_seq_comp;
          int layout;
          T value;
          Fmt(unsigned b, int l, T v): bit_seq_comp(b), layout(l), value(v) {}
        };

        typedef std::vector<Fmt> Bucket;

        T const *find(Bucket const &bucket, unsigned bit_seq_comp, ChannelLayout const &layout) const;

        std::vector<ChannelLayout> layouts;
        std::vector<Bucket> buckets; // Hash map
      };



      // Implementation of methods from BufferFormat
      int get_num_channels() const { return channel_layout.channels.size(); }
      int get_channel_width(int index) const;
      bool is_equiv_to(BufferFormat::ConstRefArg, int width, int height) const;
      size_t get_required_buffer_size(int width, int height) const;
      core::UniquePtr<BufferCodec> get_codec(void *buffer, int width, int height) const;

      // Implementation of methods from WordBasedBufferFormat
      WordType get_word_type() const           { return word_type; }
      int get_bytes_per_word() const           { return bytes_per_word; }
      std::vector<bool> get_endianness() const { return endianness; }
      std::string print(ColorSpace::ConstRefArg color_space, bool has_alpha) const;


    private:
      /**
       * Get the number of bits per strip when taking into account the
       * alignment of strips to a word boundary.
       *
       * \throw ImageSizeException If the width is too large for this
       * buffer format.
       */
      int get_gross_bits_per_strip(int width) const;


      /**
       * Get the number of bits per strip disregarding alignment of
       * strips to a word boundary.
       *
       * \throw ImageSizeException If the width is too large for this
       * buffer format.
       */
      int get_net_bits_per_strip(int width) const;


      BufferCodec *choose_codec(void *buffer, int width, int height) const;

      template<class Word, class WordAssemble>
      BufferCodec *choose_codec(void *buffer, int width, int height) const;

      template<class Word, class WordAssemble, class TrayWord, class ChannelAssemble>
      BufferCodec *choose_codec(void *buffer, int width, int height, WordType trayWordType) const;

      template<class Word, class WordAssemble, class TrayWord, class ChannelAssemble,
               bool nativeEndianness> struct FallbackCodec;


      IntegerBufferFormat(WordType word_type, int bytes_per_word, int bits_per_word,
                          ChannelLayout const &channel_layout, bool most_sig_bit_first,
                          bool word_align_strips, std::vector<bool> const &endianness,
                          std::vector<int> const &byte_perm);


      WordType const word_type;
      int const bytes_per_word, bits_per_word;
      ChannelLayout const channel_layout;
      bool const most_sig_bit_first;
      bool const word_align_strips;
      std::vector<bool> const endianness;

      /**
       * The composition of the bit sequence from words, which in turn
       * is composed from bytes, and is also dependent on the bit
       * order, is described uniquely using a single integer as
       * follows:
       *
       * <pre>
       *
       *   bit_seq_comp = most_sig_bit_first ?                             
       *     compact_endianness | ~0 << endianness_levels :
       *     compact_endianness
       *
       * </pre>
       *
       * Where \c endianness_levels is the number of significant
       * endianness levels given the selected word type, and \c
       * compact_endianness is an integer whose bits describe the
       * endianness in the same way as the \c std::vector<bool> with
       * the least significal bit of \c compact_endianness
       * corresponding to the first bit in the vector. Any remaining
       * bits in \c compact_endianness are always zero.
       *
       * This results in a negative value of \c bit_seq_comp when, and             
       * only when the bit order is 'most significant bit first'.
       *
       * This definition allows us to very quickly determine when two
       * formats use the same bit sequence composition, and also to
       * assess the level of discrepancy.
       */
      const unsigned bit_seq_comp;

      int const strip_layout_hash;

      /**
       * Only initialized if the endianness differs from native
       * platform endianness within the significant number of byte
       * combination levels.
       */
      std::vector<int> const byte_perm;


      static unsigned derive_bit_seq_comp(WordType word_type, std::vector<bool> const &endianness,
                                          bool most_sig_bit_first);

      /**
       * The canonical channel offset is the bit-level offset in
       * memory of the channel. In particular, it is the offset of the
       * least significant bit of the channel from the start of the
       * pixel, as it occurs in the first pixel of the buffer,
       * assuming the following bit ordering: Within each byte, bits
       * are ordered according to increasing significance, and bytes
       * are ordered according to acreasing memory address.
       */
      static int derive_canon_channel_offset(unsigned bit_seq_comp, Channel const &c);

      static int const strip_layout_hash_size = 61;
      static int derive_strip_layout_hash(unsigned bit_seq_comp, ChannelLayout const &l);

      static bool equiv_strip_layouts(unsigned bit_seq_comp1, ChannelLayout const &layout1,
                                      unsigned bit_seq_comp2, ChannelLayout const &layout2);

      static std::string print(WordType word_type, std::vector<bool> const &endianness,
                               bool most_sig_bit_first, ChannelLayout const &channel_layout,
                               ColorSpace::ConstRef color_space = core::CntRefNullTag(),
                               bool has_alpha = false);
    };






    // Implementation:

    inline int IntegerBufferFormat::get_bits_per_strip(int width) const
    {
      return get_gross_bits_per_strip(width);
    }


    inline int IntegerBufferFormat::get_bytes_per_strip(int width) const
    {
      return get_gross_bits_per_strip(width) / std::numeric_limits<unsigned char>::digits;
    }


    inline int IntegerBufferFormat::get_words_per_strip(int width) const
    {
      return get_gross_bits_per_strip(width) / bits_per_word;
    }


    inline int IntegerBufferFormat::get_channel_offset(int index) const
    {
      return channel_layout.channels.at(index).bit_offset;
    }


    inline int IntegerBufferFormat::get_channel_width(int index) const
    {
      return channel_layout.channels.at(index).bit_width;
    }


    inline bool IntegerBufferFormat::Channel::operator==(Channel const &c) const
    {
      return bit_offset == c.bit_offset && bit_width == c.bit_width;
    }


    inline IntegerBufferFormat::ChannelLayout::ChannelLayout(int w1, int w2, int w3, int w4):
      bits_per_pixel(0)
    {
      add(w1);
      add(w2);
      add(w3);
      add(w4);
    }

    inline IntegerBufferFormat::ChannelLayout::ChannelLayout(Channel const &c1,
                                                             Channel const &c2):
      bits_per_pixel(0)
    {
      add(c1);
      add(c2);
    }

    inline IntegerBufferFormat::ChannelLayout::ChannelLayout(Channel const &c1,
                                                             Channel const &c2,
                                                             Channel const &c3):
      bits_per_pixel(0)
    {
      add(c1);
      add(c2);
      add(c3);
    }

    inline IntegerBufferFormat::ChannelLayout::ChannelLayout(Channel const &c1,
                                                             Channel const &c2,
                                                             Channel const &c3,
                                                             Channel const &c4):
      bits_per_pixel(0)
    {
      add(c1);
      add(c2);
      add(c3);
      add(c4);
    }

    inline void IntegerBufferFormat::ChannelLayout::add(Channel const &c)
    {
      channels.push_back(c);
      int const n = c.bit_offset + c.bit_width;
      if(bits_per_pixel < n) bits_per_pixel = n;
    }


    inline core::UniquePtr<BufferCodec>
    IntegerBufferFormat::get_codec(void *buffer, int width, int height) const
    {
      core::UniquePtr<BufferCodec> c(choose_codec(buffer, width, height));
      return c;
    }


    inline std::string IntegerBufferFormat::print(ColorSpace::ConstRefArg color_space,
                                                  bool has_alpha) const
    {
      return print(word_type, endianness, most_sig_bit_first,
                   channel_layout, color_space, has_alpha);
    }


    inline int IntegerBufferFormat::get_gross_bits_per_strip(int width) const
    {
      unsigned bits_per_strip = get_net_bits_per_strip(width);
      if(!word_align_strips) return bits_per_strip;
      unsigned const rem = bits_per_strip % bits_per_word;
      if(rem) bits_per_strip += bits_per_word - rem;
      if(unsigned(std::numeric_limits<int>::max()) < bits_per_strip)
        throw ImageSizeException("Image is too wide for this buffer format");
      return bits_per_strip;
    }


    inline int IntegerBufferFormat::get_net_bits_per_strip(int width) const
    {
      if(width < 1) throw std::invalid_argument("Illegal width");
      // Be carefull about overflow and require that the number of
      // bits per strip can be represented in an 'int'.
      unsigned const max = std::numeric_limits<int>::max(), bpp = channel_layout.bits_per_pixel;
      unsigned const bits_per_strip = bpp * width;
      if(bits_per_strip/width != bpp || max < bits_per_strip)
        throw ImageSizeException("Image is too wide for this buffer format");
      return bits_per_strip;
    }


    inline IntegerBufferFormat::IntegerBufferFormat(WordType word_type, int bytes_per_word,
                                                    int bits_per_word,
                                                    ChannelLayout const &channel_layout,
                                                    bool most_sig_bit_first,
                                                    bool word_align_strips,
                                                    std::vector<bool> const &endianness,
                                                    std::vector<int> const &byte_perm):
      word_type(word_type), bytes_per_word(bytes_per_word), bits_per_word(bits_per_word),
      channel_layout(channel_layout), most_sig_bit_first(most_sig_bit_first),
      word_align_strips(word_align_strips), endianness(endianness),
      bit_seq_comp(derive_bit_seq_comp(word_type, endianness, most_sig_bit_first)),
      strip_layout_hash(derive_strip_layout_hash(bit_seq_comp, channel_layout)),
      byte_perm(byte_perm) {}


    template<class T>
    int IntegerBufferFormat::Map<T>::add_channel_layout(ChannelLayout const &l)
    {
      int i = layouts.size();
      layouts.push_back(l);
      return i;
    }

    template<class T>
    void IntegerBufferFormat::Map<T>::add_format(WordType word_type,
                                                 std::vector<bool> const &endianness,
                                                 bool most_sig_bit_first,
                                                 int layout, T const &value)
    {
      ChannelLayout const &channel_layout = layouts.at(layout);
      unsigned bit_seq_comp = derive_bit_seq_comp(word_type, endianness, most_sig_bit_first);
      int hash = derive_strip_layout_hash(bit_seq_comp, channel_layout);
      Bucket &bucket = buckets[hash];
      if(find(bucket, bit_seq_comp, channel_layout))
      {
        return;
      }
      bucket.push_back(Fmt(bit_seq_comp, layout, value));
    }

    template<class T>
    T const *IntegerBufferFormat::Map<T>::find(ConstRefArg int_buf_fmt) const
    {
      int const hash = int_buf_fmt->strip_layout_hash;
      return find(buckets[hash], int_buf_fmt->bit_seq_comp, int_buf_fmt->channel_layout);
    }

    template<class T>
    T const *IntegerBufferFormat::Map<T>::find(Bucket const &bucket, unsigned bit_seq_comp,
                                               ChannelLayout const &layout) const
    {
      typename Bucket::const_iterator const end = bucket.end();
      for(typename Bucket::const_iterator i=bucket.begin(); i!=end; ++i)
        if(equiv_strip_layouts(bit_seq_comp, layout, i->bit_seq_comp, layouts[i->layout]))
          return &i->value;
      return 0;
    }

    template<class T>
    void IntegerBufferFormat::Map<T>::dump_info(std::ostream &out) const
    {
      int const n = buckets.size();
      for(int i=0; i<n; ++i) out <<i<<": "<<buckets[i].size()<<"\n";
    }
  }
}

#endif // ARCHON_IMAGE_INTEGER_BUFFER_FORMAT_HPP
