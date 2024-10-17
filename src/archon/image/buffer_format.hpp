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

#ifndef ARCHON_X_IMAGE_X_BUFFER_FORMAT_HPP
#define ARCHON_X_IMAGE_X_BUFFER_FORMAT_HPP

/// \file


#include <cstddef>
#include <type_traits>
#include <algorithm>
#include <array>

#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/enum.hpp>
#include <archon/core/endianness.hpp>
#include <archon/image/color_space.hpp>
#include <archon/image/bit_field.hpp>


namespace archon::image {


/// \brief Description
///
/// An object of this type is a specification of the scheme by which pixels are stored in a
/// memory buffer.
///
/// A particular scheme may, or may not be describable using this class.
///
/// \sa \ref image::Image::try_get_buffer()
/// \sa \ref image::IntegerPixelFormat::try_describe()
///
struct BufferFormat {
    /// \brief Available integer types.
    ///
    /// These are the integer types supported by the integer based format descriptions; \ref
    /// IntegerFormat, \ref PackedFormat, \ref SubwordFormat, and \ref IndexedFormat.
    ///
    /// | Value     | Fundamental type
    /// |-----------|------------------------------------------
    /// | `byte`    | `std::byte`, `char`, or `unsigned char`
    /// | `schar`   | `signed char`
    /// | `short_`  | `short`
    /// | `ushort`  | `unsigned short`
    /// | `int_`    | `int`
    /// | `uint`    | `unsigned`
    /// | `long_`   | `long`
    /// | `ulong`   | `unsigned long`
    /// | `llong`   | `long long`
    /// | `ullong`  | `unsigned long long`
    ///
    /// The three fictitious integer types `fict_1`, `fict_2`, and `fict_3` are for testing
    /// purposes only. They should never be used in a non-testing context. The first two
    /// will be reported as having big and little-endian native byte order respectively
    /// (\ref BufferFormat::try_get_byte_order()). The last one (`fict_3`) will be reported
    /// as having undeterminable byte order. All three will be reported as having 2 bytes
    /// per word (\ref BufferFormat::get_bytes_per_word()).
    ///
    /// A specialization of \ref core::EnumTraits is provided, making stream input and
    /// output immediately available (trailing underscores are elided in the textual
    /// representations).
    ///
    enum class IntegerType {
        byte, schar,
        short_, ushort,
        int_, uint,
        long_, ulong,
        llong, ullong,
        fict_1, fict_2, fict_3,
    };

    /// \brief Available floating-point types.
    ///
    /// These are the floating-point types supported by the floating-point based format
    /// description, \ref FloatFormat.
    ///
    /// | Value     | Fundamental type
    /// |-----------|------------------
    /// | `float_`  | `float`
    /// | `double_` | `double`
    /// | `ldouble` | `long double`
    ///
    /// A specialization of \ref core::EnumTraits is provided, making stream input and
    /// output immediately available (trailing underscores are elided in the textual
    /// representations).
    ///
    enum class FloatType { float_, double_, ldouble };

    /// \brief Channel configuration.
    ///
    /// An instance of this type specifies the set of channels that are present in a pixel
    /// buffer (color space and whether alpha channel is present) and establishes the base
    /// order of those channels. The actual order in memory will generally also depend on
    /// other format parameters such as "word order".
    ///
    struct ChannelConf {
        /// \brief Color space in use.
        ///
        /// The color space in use in the described pixel buffer.
        ///
        const image::ColorSpace* color_space;

        /// \brief Whether an alpha channel is present.
        ///
        /// If `true`, an alpha channel is present. Otherwise there is no alpha channel.
        ///
        bool has_alpha;

        /// \{
        ///
        /// \brief Specification of channel order.
        ///
        /// Together, `alpha_first` and `reverse_order` specify how the *channel storage
        /// order* relates to the canonical channel order. The meaning of the storage order
        /// for channels depends on the sub-format in question (\ref IntegerFormat, \ref
        /// PackedFormat, \ref SubwordFormat, \ref FloatFormat, \ref IndexedFormat). The
        /// canonical channel order is the color channels in canonical order for the color
        /// space (\ref color_space) followed by the alpha channel when present (\ref
        /// has_alpha).
        ///
        /// If `alpha_first` and `reverse_order` are both `false`, the channel storage order
        /// is the canonical channel order. For example, RGB stays RGB and RGBA stays RGBA.
        ///
        /// If `alpha_first` is `true` and `reverse_order` is `false`, the alpha channel
        /// comes first in the storage order if it is present and is then followed by the
        /// color channels is their canonical order. For example, RGB stays RGB but RGBA
        /// becomes ARGB.
        ///
        /// If `reverse_order` is `true` and `alpha_first` is `false`, the channel storage
        /// order is reversed relative to the canonical order so the alpha channel now comes
        /// last in the storage order when it is present. For example, RGB becomes BGR and
        /// RGBA becomes ABGR.
        ///
        /// If `alpha_first` and `reverse_order` are both `true`, the channel storage order
        /// for color channels is reversed relative to the canonical order but the alpha
        /// channel still comes last in the storage order when it is present. For example,
        /// RGB becomes BGR and RGBA becomes BGRA.
        ///
        /// Note that when `reverse_order` is `true`, `alpha_first` should be understood as
        /// `alpha_last`.
        ///
        /// Note also that `alpha_first` has no meaning when there is no alpha channel (\ref
        /// has_alpha), and that `reverse_order` has no meaning when there is only one
        /// channel (\ref get_num_channels()).
        ///
        bool alpha_first;
        bool reverse_order;
        /// \}

        /// \brief Get number of specified channels.
        ///
        /// This function returns the number of specified channels. If the the number of
        /// color channels (\ref color_space) plus one if there is also an alpha channel
        /// (\ref has_alpha).
        ///
        int get_num_channels() const noexcept;

        /// \brief Reverse channel order.
        ///
        /// This function reverses the channel order in this channel configuration. This
        /// occurs by inverting \ref reverse_order.
        ///
        void reverse() noexcept;
    };

    struct IntegerFormat;
    struct PackedFormat;
    struct SubwordFormat;

    /// \brief Pixel buffer format with one or more words per channel.
    ///
    /// This pixel buffer format stores each pixel as a sequence of M times N memory words
    /// where M is \ref words_per_channel and N is the number of channels (\ref
    /// BufferFormat::ChannelConf::get_num_channels(), \ref channel_conf). The memory word
    /// type is specified by \ref word_type. Pixels are stored contiguously in memory
    /// according to a top-to-bottom, left-to-right, row-major order.
    ///
    /// Each pixel is stored as a sequence of channels with each channel taking up M
    /// words. The channels occur in channel storage order (see \ref
    /// ChannelConf::reverse_order). Within each channel, the word order is as specified by
    /// \ref word_order. The number of bits that are taken from each word is specified by
    /// \ref bits_per_word.
    ///
    /// The channel configuration (color space, presence of alpha channel, and channel
    /// order) is specified by \ref channel_conf.
    ///
    /// \sa \ref image::IntegerPixelFormat
    ///
    struct IntegerFormat {
        /// \brief Pixel buffer element type.
        ///
        /// This is the type of memory elements that the described pixel buffer is made up
        /// of. For example, if the word type is `IntegerType::int_` (see \ref
        /// BufferFormat::IntegerType), it means that the memory needs to be accessed as an
        /// array of elements of type `int`.
        ///
        IntegerType word_type;

        /// \brief Number of used bit positions per word
        ///
        /// This is the number of bits that are taken from each word to make up channel
        /// components. Specifically, if N is the value of `bits_per_word`, then the bits
        /// corresponding to the N least significant bit positions are taken from each word
        /// that make up a channel component. Those bits are then assembled into a channel
        /// component in the order specified by \ref word_order. Bits at unused bit
        /// positions must be zero.
        ///
        int bits_per_word;

        /// \brief Number of words per channel component.
        ///
        /// This is the number of words that make up each channel component.
        ///
        int words_per_channel;

        /// \brief Order in which words are assembled into channel component values.
        ///
        /// The word order specifies how words are assembled into channel component
        /// values. It does not specify the order of the channels in memory. The first
        /// channel always comes first in memory (lowest address).
        ///
        /// If the word order is set to 'big endian', words at lower memory address will
        /// make up bits of higher significance in component values.
        ///
        core::Endianness word_order;

        /// \brief Which channels and in which order.
        ///
        /// This is a specification of which channels are present in the pixel buffer and in
        /// which order these channels occur.
        ///
        ChannelConf channel_conf;

        /// \brief Whether all consistency requirements are met.
        ///
        /// This function returns `true` if, and only if all the consistency requirements
        /// are met.
        ///
        /// The consistency requirements are:
        ///
        ///   * \ref bits_per_word and \ref words_per_channel must be greater than, or equal
        ///     to 1.
        ///
        ///   * \ref bits_per_word must be less than, or equal to number of available bits
        ///     per word (\ref BufferFormat::get_bits_per_word(), \ref word_type).
        ///
        ///   * The number of bits per channel (\ref words_per_channel times \ref
        ///     bits_per_word) must be representable in type `int`.
        ///
        bool is_valid() const noexcept;

        /// \{
        ///
        /// \brief Try to cast integer format as format of different type.
        ///
        /// These functions are used by \ref BufferFormat::try_cast_to(), but can also be
        /// called directly by the application when the origin format is statically known to
        /// be an integer format (`IntegerFormat`).
        ///
        /// \sa \ref BufferFormat::try_cast_to()
        ///
        ///
        /// ### Integer to integer
        ///
        /// An integer format can be cast to another integer format if, and only if the cast
        /// takes one of the following two forms:
        ///
        /// <em>Form 1:</em> The origin word type (\ref word_type) is the same as the
        /// specified target word type (\p target_word_type), or there is only one byte per
        /// origin word (\ref BufferFormat::get_bytes_per_word()) and the specified target
        /// word type is `byte`.
        ///
        /// <em>Form 2:</em> There is more than one byte per origin word and the specified
        /// target word type is `byte`. Additionally, the following conditions must
        /// hold:
        ///
        ///   * There are no unused bits, which means that \ref bits_per_word is equal to
        ///     the number of bits per byte times the number of bytes in each origin word.
        ///
        ///   * The native byte order for the origin word type is determinable (\ref
        ///     BufferFormat::try_get_byte_order()).
        ///
        ///   * If there is more than one origin word per channel (\ref words_per_channel),
        ///     the origin word order (\ref word_order) matches the native byte order for
        ///     the origin word type.
        ///
        /// When the cast succeeds, the parameters of the target format are set as follows:
        ///
        ///   * The word type (\ref word_type) is set to the specified target word type.
        ///
        ///   * The number of bits per word (\ref bits_per_word) is set to the number of
        ///     bits per word in the origin format if the cast is on the first form (see
        ///     above). Otherwise, the number of bits per word is set to the number of bits
        ///     in a byte (usually 8).
        ///
        ///   * The number of words per channel (\ref words_per_channel) is set to the
        ///     number of words per channel in the origin format if the cast is on the first
        ///     form (see above). Otherwise, the number of words per channel is set to that
        ///     times the number of bytes per origin word.
        ///
        ///   * The word order (\ref word_order) is set to the word order in the origin
        ///     format if the cast is on the first form (see above). Otherwise, the word
        ///     order is set to the native byte order for the origin word type.
        ///
        ///   * The channel configuration (\ref channel_conf) is set to the channel
        ///     configuration in the origin format.
        ///
        ///
        /// ### Integer to packed
        ///
        /// An integer format can be cast to a packed format if, and only if the cast takes
        /// one of the following two forms and the number of channels (\ref
        /// ChannelConf::get_num_channels(), \ref channel_conf) is less than, or equal to
        /// the maximum number of bit fields in a packed format (\ref
        /// BufferFormat::max_bit_fields):
        ///
        /// <em>Form 1:</em> The origin word type (\ref word_type) is the same as the
        /// specified target word type (\p target_word_type), or there is only one byte per
        /// origin word (\ref BufferFormat::get_bytes_per_word()) and the specified target
        /// word type is `byte`. Additionally, the following conditions must hold:
        ///
        ///   * The number of words per pixel (\ref words_per_channel times number of
        ///     channels) is representable in an object of type `int`.
        ///
        /// <em>Form 2:</em> There is more than one byte per origin word and the specified
        /// target word type is `byte`. Additionally, the following conditions must hold:
        ///
        ///   * There are no unused bits, which means that \ref bits_per_word is equal to
        ///     the number of bits per byte times the number of bytes in each origin word.
        ///
        ///   * The native byte order for the origin word type is determinable (\ref
        ///     BufferFormat::try_get_byte_order()).
        ///
        ///   * If there is more than one origin word per channel (\ref words_per_channel),
        ///     the origin word order (\ref word_order) matches the native byte order for
        ///     the origin word type.
        ///
        ///   * The number of bytes per pixel (\ref words_per_channel times number of
        ///     channels times number of bytes per origin word) is representable in an
        ///     object of type `int`.
        ///
        /// When the cast succeeds, the parameters of the target format are set as follows:
        ///
        ///   * The word type (\ref PackedFormat::word_type) is set to the specified target
        ///     word type.
        ///
        ///   * The number of bits per word (\ref PackedFormat::bits_per_word) is set to the
        ///     number of bits per word in the origin format (\ref bits_per_word) if the
        ///     cast is on the first form (see above). Otherwise, the number of bits per
        ///     word is set to the number of bits in a byte (usually 8).
        ///
        ///   * The number of words per bit compound (\ref PackedFormat::words_per_pixel) is
        ///     set to the number of words per channel in the origin format (\ref
        ///     words_per_channel) times the number of channels if the cast is on the first
        ///     form (see above). Otherwise, the number of words per bit compound is set to
        ///     that times the number of bytes per origin word.
        ///
        ///   * The word order (\ref PackedFormat::word_order) is set to the word order in
        ///     the origin format (\ref word_order) if the cast is on the first form (see
        ///     above). Otherwise, the word order is set to the native byte order for the
        ///     origin word type.
        ///
        ///   * The list of bit fields (\ref PackedFormat::bit_fields) is initialized such
        ///     that each field has a width equal to the number of bits per channel in the
        ///     origin format (\ref bits_per_word times \ref words_per_channel) and all
        ///     field gaps are zero (see \ref image::BitField). Unused bit field slots will
        ///     be set as if zero initialized.
        ///
        ///   * The channel configuration (\ref PackedFormat::channel_conf) is set to the
        ///     channel configuration in the origin format (\ref channel_conf) except that,
        ///     if the target word order is 'little endian', the channel order will be
        ///     reversed (\ref ChannelConf::reverse()).
        ///
        /// \note There is generally a question of whether a sufficiently wide integer type
        /// exists for the purpose of assembling per pixel bit compounds in the target
        /// format (\ref PackedFormat). That question is not addressed by
        /// `try_cast_to()`. If the application needs to assemble per pixel bit compounds,
        /// it can follow up with a check that `format.bits_per_word *
        /// format.words_per_pixel` is small enough.
        ///
        ///
        /// ### Integer to subword
        ///
        /// An integer format can be cast to a subword format if, and only if both of the
        /// following conditions hold:
        ///
        ///   * The origin word type (\ref word_type) is the same as the specified target
        ///     word type (\p target_word_type), or there is only one byte per origin word
        ///     (\ref BufferFormat::get_bytes_per_word()) and the specified target word type
        ///     is `byte`.
        ///
        ///   * There is only one word per channel in the origin format (\ref
        ///     words_per_channel) and there is only one channel (\ref
        ///     ChannelConf::get_num_channels()).
        ///
        /// When the cast succeeds, the parameters of the target format are set as follows:
        ///
        ///   * The word type (\ref SubwordFormat::word_type) is set to the specified target
        ///     word type.
        ///
        ///   * The number of bits per channel (\ref SubwordFormat::bits_per_channel) is set
        ///     to the number of bits per word in the origin format (\ref bits_per_word).
        ///
        ///   * The number of pixels per word (\ref SubwordFormat::pixels_per_word) is set
        ///     to 1.
        ///
        ///   * The bit order (\ref SubwordFormat::bit_order) is set to 'big endian'.
        ///
        ///   * The word alignment requirement for pixel rows (\ref
        ///     SubwordFormat::word_aligned_rows) is set to `false`.
        ///
        ///   * The channel configuration (\ref SubwordFormat::channel_conf) is set to the
        ///     channel configuration in the origin format (\ref channel_conf).
        ///
        bool try_cast_to(IntegerFormat& format, IntegerType target_word_type) const;
        bool try_cast_to(PackedFormat& format, IntegerType target_word_type) const;
        bool try_cast_to(SubwordFormat& format, IntegerType target_word_type) const;
        /// \}

        auto get_words_per_row(int image_width) const -> std::size_t;
    };

    /// \brief Maximum number of bit fields in packed format.
    ///
    /// This is the maximum number of bit fields, and therefore the maximum number of
    /// channels that can be described by \ref PackedFormat.
    ///
    static constexpr int max_bit_fields = 8;

    static_assert(max_bit_fields >= 4); // Documented promise

    /// \brief Pixel buffer format with channels packed into bit compounds.
    ///
    /// This pixel buffer format stores each pixel as a sequence of N memory words where N
    /// is \ref words_per_pixel. The memory word type is specified by \ref word_type. Pixels
    /// are stored contiguously in memory according to a top-to-bottom, left-to-right,
    /// row-major order.
    ///
    /// Each group of N words makes up a bit compound, and the channels of the packed format
    /// are then laid out as bit fields inside each bit compound (\ref bit_fields). Bits at
    /// unused bit positions inside the bit compound must be zero.
    ///
    /// The number of bits that are taken from each word is specified by \ref bits_per_word
    /// and the order in which the words contribute their bits to the bit compound is
    /// specified by \ref word_order.
    ///
    /// The channel configuration (color space, presence of alpha channel, and channel
    /// order) is specified by \ref channel_conf.
    ///
    /// \sa \ref image::PackedPixelFormat
    ///
    struct PackedFormat {
        /// \brief Pixel buffer element type.
        ///
        /// This is the type of memory elements that the described pixel buffer is made up
        /// of. For example, if the word type is `IntegerType::int_` (see \ref
        /// BufferFormat::IntegerType), it means that the memory needs to be accessed as an
        /// array of elements of type `int`.
        ///
        IntegerType word_type;

        /// \brief Number of used bit positions per word
        ///
        /// This is the number of bits that are taken from each word to make up per pixel
        /// bit compounds. Specifically, if N is the value of `bits_per_word`, then the bits
        /// corresponding to the N least significant bit positions are taken from each word
        /// that make up a bit compound. Those bits are then assembled into a bit compound
        /// in the order specified by \ref word_order. Bits at unused bit positions must be
        /// zero.
        ///
        int bits_per_word;

        /// \brief Number of words per pixel.
        ///
        /// This is the number of words that make up each per pixel bit compound.
        ///
        int words_per_pixel;

        /// \brief Order in which words are assembled into bit compounds.
        ///
        /// The word order specifies how words are assembled into per pixel bit
        /// compounds. If the word order is set to 'big endian', words at lower memory
        /// address will make up bits of higher significance in the bit compound value.
        ///
        core::Endianness word_order;

        /// \brief Channel bit fields.
        ///
        /// If N is the number of channels (\ref ChannelConf::get_num_channels(), \ref
        /// channel_conf), the first N slots in `bit_fields` specify the bit fields of this
        /// packed format. Remaining slots are unused and their contents do not matter.
        ///
        /// Objects of this type cannot be used to describe packed formats with more than
        /// \ref max_bit_fields channels.
        ///
        /// Channels map to bit fields according to the channel storage order (see \ref
        /// ChannelConf::reverse_order). For example, the first bit field specifies the
        /// channel that comes first with respect to the channel storage order.
        ///
        std::array<image::BitField, max_bit_fields> bit_fields;

        /// \brief Which channels and in which order.
        ///
        /// This is a specification of which channels are present in the pixel buffer and in
        /// which order these channels occur.
        ///
        ChannelConf channel_conf;

        /// \brief Whether all consistency requirements are met.
        ///
        /// This function returns `true` if, and only if all the consistency requirements
        /// are met.
        ///
        /// The consistency requirements are:
        ///
        ///   * \ref bits_per_word and \ref words_per_pixel must be greater than, or equal
        ///     to 1.
        ///
        ///   * \ref bits_per_word must be less than, or equal to the number of available
        ///     bits per word (\ref BufferFormat::get_bits_per_word(), \ref word_type).
        ///
        ///   * The number of bits per bit compound (\ref words_per_pixel times \ref
        ///     bits_per_word) must be representable in type `int`.
        ///
        ///   * The number of channels (\ref ChannelConf::get_num_channels(), \ref
        ///     channel_conf) must be less than, or equal to \ref max_bit_fields.
        ///
        ///   * If N is the number of channels, the first N bit fields must describe a valid
        ///     sequence of bit fields that fit within a bit compound (\ref words_per_pixel
        ///     times \ref bits_per_word). See \ref image::valid_bit_fields() for an
        ///     explanation of what it means for a sequence of bit fields to be valid and
        ///     fit within a certain number of bits.
        ///
        bool is_valid() const noexcept;

        /// \{
        ///
        /// \brief Try to cast packed format as format of different type.
        ///
        /// These functions are used by \ref BufferFormat::try_cast_to(), but can also be
        /// called directly by the application when the origin format is statically known to
        /// be a packed format (`PackedFormat`).
        ///
        /// \sa \ref BufferFormat::try_cast_to()
        ///
        ///
        /// ### Packed to integer
        ///
        /// A packed format can be cast to an integer format if, and only if the cast takes
        /// one of the following two forms:
        ///
        /// <em>Form 1:</em> The origin word type (\ref word_type) is the same as the
        /// specified target word type (\p target_word_type), or there is only one byte per
        /// origin word (\ref BufferFormat::get_bytes_per_word()) and the specified target
        /// word type is `byte`. Additionally, the following conditions must hold:
        ///
        ///   * The number of origin words per bit compound (\ref words_per_pixel) is an
        ///     integer multiple of the number of channels.
        ///
        ///   * All bit fields (\ref bit_fields) have the same width.
        ///
        ///   * If there is more than one origin word per channel, the width of bit fields
        ///     is equal to the number of origin words per channel times the number of bits
        ///     per word (\ref bits_per_word).
        ///
        ///   * All bit field shifts (\ref image::get_bit_field_shift()) are integer
        ///     multiples of the number words per channel times the number of bits per word.
        ///
        /// <em>Form 2:</em> There is more than one byte per origin word and the specified
        /// target word type is `byte`. Additionally, the following conditions must hold:
        ///
        ///   * There are no unused bits, which means that the number of bits per origin
        ///     word (\ref bits_per_word) is equal to the number of bytes per origin word
        ///     (\ref BufferFormat::get_bytes_per_word()) times the number of bits in a
        ///     byte.
        ///
        ///   * The number of bytes per bit compound (\ref words_per_pixel times number of
        ///     bytes per origin word) is an integer multiple of the number of channels.
        ///
        ///   * The native byte order for the origin word type is determinable (\ref
        ///     BufferFormat::try_get_byte_order()).
        ///
        ///   * Unless there is exactly one origin word per channel or just one origin word
        ///     per bit compound, the word order in the origin format (\ref word_order)
        ///     matches the native byte order for the origin word type. There is exactly one
        ///     origin word per channel when \ref words_per_pixel equals the number of
        ///     channels.
        ///
        ///   * All bit fields have a width equal to the number of bytes per channel times
        ///     the number of bits in a byte.
        ///
        ///   * All bit fields have a gap equal to zero.
        ///
        /// When the cast succeeds, the parameters of the target format are set as follows:
        ///
        ///   * The word type (\ref IntegerFormat::word_type) is set to the specified target
        ///     word type.
        ///
        ///   * The number of bits per word (\ref IntegerFormat::bits_per_word) is set to
        ///     the number of bits per word in the origin format (\ref bits_per_word) if the
        ///     cast is on the first form (see above) and there is more than one origin word
        ///     per channel. Otherwise, if the cast is on the first form, the number of bits
        ///     per word is set to the width of bit fields. Otherwise, the number of bits
        ///     per word is set to the number of bits in a byte.
        ///
        ///   * The number of words per channel (\ref IntegerFormat::words_per_channel) is
        ///     set to the number of words per bit compound in the origin format (\ref
        ///     words_per_pixel) divided by the number of channels if the cast is on the
        ///     first form (see above). Otherwise, the number of words per bit compound is
        ///     set to the number of words per bit compound in the origin format times the
        ///     number of bytes per origin word divided by the number of channels.
        ///
        ///   * The word order (\ref IntegerFormat::word_order) is set to the word order in
        ///     the origin format (\ref word_order) if the cast is on the first form (see
        ///     above). Otherwise, the word order is set to the native byte order for the
        ///     origin word type.
        ///
        ///   * The channel configuration (\ref IntegerFormat::channel_conf) is set to the
        ///     channel configuration in the origin format except that, if there is exactly
        ///     one origin word per channel and the origin word order is 'little endian' or
        ///     there is not exactly one origin word per channel and the target word order
        ///     is 'little endian', the channel order will be reversed (\ref
        ///     ChannelConf::reverse()). There is exactly one origin word per channel when
        ///     \ref words_per_pixel equals the number of channels.
        ///
        ///
        /// ### Packed to packed
        ///
        /// A packed format can be cast to another packed format if, and only if the cast
        /// takes one of the following two forms:
        ///
        /// <em>Form 1:</em> The origin word type (\ref word_type) is the same as the
        /// specified target word type (\p target_word_type), or there is only one byte per
        /// origin word (\ref BufferFormat::get_bytes_per_word()) and the specified target
        /// word type is `byte`.
        ///
        /// <em>Form 2:</em> There is more than one byte per origin word and the specified
        /// target word type is `byte`. Additionally, the following conditions must hold:
        ///
        ///   * The native byte order for the origin word type is determinable (\ref
        ///     BufferFormat::try_get_byte_order()).
        ///
        ///   * At least one of the following *bit compound conditions* hold:
        ///
        ///      1. There is only one word per bit compound in the origin format.
        ///
        ///      2. There are no unused bits in the origin words (\ref bits_per_word is
        ///         equal to the number of bits in a byte times the number of bytes in an
        ///         origin word) and the word order in the origin format matches the native
        ///         byte order for the origin word type.
        ///
        ///      3. There is no bit field that crosses the boundary between two origin words
        ///         and the word order in the origin format matches the native byte order
        ///         for the origin word type.
        ///
        ///      4. There is no bit field that crosses the boundary between two origin words
        ///         and there is a single origin word that contains all the bit fields.
        ///
        ///      5. There is no bit field that crosses the boundary between two origin words
        ///         and no origin word contains more than one bit field.
        ///
        ///      6. There is no bit field that crosses the boundary between two origin words
        ///         and there is a single origin word that contains all the bit fields that
        ///         correspond to color channels.
        ///
        /// When the cast succeeds, the parameters of the target format are set as follows:
        ///
        ///   * The word type (\ref word_type) is set to the specified target word type.
        ///
        ///   * The number of bits per word (\ref bits_per_word) is set to the number of
        ///     bits per word in the origin format if the cast is on the first form (see
        ///     above). Otherwise, the number of bits per word is set to the number of bits
        ///     in a byte (usually 8).
        ///
        ///   * The number of words per bit compound (\ref words_per_pixel) is set to the
        ///     number of words per bit compound in the origin format if the cast is on the
        ///     first form (see above). Otherwise, the number of words per bit compound is
        ///     set to that times the number of bytes per origin word.
        ///
        ///   * The word order (\ref word_order) is set to the word order in the origin
        ///     format if the cast is on the first form (see above). Otherwise, the word
        ///     order is set to the native byte order for the origin word type.
        ///
        ///   * The list of bit fields (\ref bit_fields) is set to the list of bit fields in
        ///     the origin format if the cast is on the first form (see above), or if it is
        ///     on the second form and one of the first two bit compound conditions hold
        ///     (see above). Otherwise, the list of bit fields is set to a transformed
        ///     version of the list of bit fields in the origin format. The transformation
        ///     adjusts for the shifting and/or shuffling of bit fields caused by the
        ///     inversion of origin word order and potential insertion of bits between
        ///     origin words (when all bits are not used in the origin words). Such a
        ///     transformation is possible in these cases because no bit field crosses an
        ///     origin word boundary. Unused bit field slots will be set as if zero
        ///     initialized.
        ///
        ///   * The channel configuration (\ref channel_conf) is set to the channel
        ///     configuration in the origin format if the cast is on the first form (see
        ///     above), or if it is on the second form and one of the first four bit
        ///     compound conditions hold (see above). Otherwise, if the fifth bit compound
        ///     condition holds, the channel configuration is set to the origin channel
        ///     configuration with the channel order reversed (\ref
        ///     ChannelConf::reverse()). Otherwise, the origin format has an alpha channel
        ///     and the target channel configuration is set to the origin channel
        ///     configuration with the alpha channel side switched (\ref
        ///     ChannelConf::alpha_first).
        ///
        ///
        /// ### Packed to subword
        ///
        /// An packed format can be cast to a subword format if, and only if all of the
        /// following conditions hold:
        ///
        ///   * The origin word type (\ref word_type) is the same as the specified target
        ///     word type (\p target_word_type), or there is only one byte per origin word
        ///     (\ref BufferFormat::get_bytes_per_word()) and the specified target word type
        ///     is `byte`.
        ///
        ///   * There is only one word per bit compound in the origin format (\ref
        ///     words_per_pixel).
        ///
        ///   * All bit fields in the origin format (\ref bit_fields) have the same width
        ///     and all gaps are zero.
        ///
        /// When the cast succeeds, the parameters of the target format are set as follows:
        ///
        ///   * The word type (\ref SubwordFormat::word_type) is set to the specified target
        ///     word type.
        ///
        ///   * The number of bits per channel (\ref SubwordFormat::bits_per_channel) is set
        ///     to the width of the bit fields in the origin format.
        ///
        ///   * The number of pixels per word (\ref SubwordFormat::pixels_per_word) is set
        ///     to 1.
        ///
        ///   * The bit order (\ref SubwordFormat::bit_order) is set to 'big endian'.
        ///
        ///   * The word alignment requirement for pixel rows (\ref
        ///     SubwordFormat::word_aligned_rows) is set to `false`.
        ///
        ///   * The channel configuration (\ref SubwordFormat::channel_conf) is set to the
        ///     channel configuration in the origin format (\ref channel_conf).
        ///
        bool try_cast_to(IntegerFormat& format, IntegerType target_word_type) const;
        bool try_cast_to(PackedFormat& format, IntegerType target_word_type) const;
        bool try_cast_to(SubwordFormat& format, IntegerType target_word_type) const;
        /// \}
    };

    /// \brief Pixel buffer format with one or more pixels per memory word.
    ///
    /// This pixel buffer format stores multiple (one or more) pixels in each memory
    /// word. The memory word type is specified by \ref word_type. The number of pixel slots
    /// in each word is specified by \ref pixels_per_word. Pixels are assigned to words
    /// according to a top-to-bottom, left-to-right, row-major order. Within each word,
    /// pixel slots are filled in the order specified by \ref bit_order.
    ///
    /// The last word of the image, or the last word of each pixel row if \ref
    /// word_aligned_rows is `true`, can have unused pixel slots. When \ref
    /// word_aligned_rows is `true`, every pixel row starts on a word boundary. The values
    /// of bits inside unused pixel slots have no effect on the stored image, and they do
    /// not have to be zero.
    ///
    /// Each pixel slot is made up of M times N consecutive bit positions where M is \ref
    /// bits_per_channel and N is the number of channels (\ref
    /// ChannelConf::get_num_channels(), \ref channel_conf). Unused bit positions are always
    /// the bit positions of greatest significance within the word. Unused bit positions
    /// exist when M times N times \ref pixels_per_word is less than the number of available
    /// bits in each word. Bits at unused bit positions must be zero.
    ///
    /// The bits of a pixel slot are divided into N channel components, each one occupying M
    /// consecutive bit positions. From the point of view of the channel storage order (see
    /// \ref ChannelConf::reverse_order), channels occur within a pixel slot in order of
    /// decreasing bit position significance. For example, the channel that comes first
    /// according to the channel storage order occupies the bit positions of greatest
    /// significance within the pixel slot.
    ///
    /// \note While \ref bit_order affects the order of pixels within a word, it has no
    /// effect on the order of channels within a pixel slot.
    ///
    /// The channel configuration (color space, presence of alpha channel, and channel
    /// order) is specified by \ref channel_conf.
    ///
    /// \sa \ref image::SubwordPixelFormat
    ///
    struct SubwordFormat {
        /// \brief Pixel buffer element type.
        ///
        /// This is the type of memory elements that the described pixel buffer is made up
        /// of. For example, if the word type is `IntegerType::int_` (see \ref
        /// BufferFormat::IntegerType), it means that the memory needs to be accessed as an
        /// array of elements of type `int`.
        ///
        IntegerType word_type;

        /// \brief Number of bit used per channel component.
        ///
        /// This is the number of bits that are used per channel component.
        ///
        int bits_per_channel;

        /// \brief Number of pixels slots in each word.
        ///
        /// This is the number of pixel slots that will exist in each word. The order of
        /// pixels within the bits of the word is specified by \ref bit_order.
        ///
        int pixels_per_word;

        /// \brief Order of pixel to slot assignment in each memory word.
        ///
        /// The bit order specifies the order in which pixels are assigned to pixel slots of
        /// a particular memory word with respect to a top-to-bottom, left-to-right,
        /// row-major traversal of the image. If the bit order is set to 'big endian', pixel
        /// will be assigned to slots in order of decreasing bit position significance. If
        /// it is set to 'little endian', they will be assigned in order of increasing bit
        /// position significance.
        ///
        core::Endianness bit_order;

        /// \brief Whether pixel rows are aligned on word boundaries.
        ///
        /// If `true`, pixel rows are required to be aligned with word boundaries. This
        /// means that during a top-to-bottom, left-to-right, row-major traversal of the
        /// pixels, when the last pixel in a row is assigned to a word and there are
        /// additional unused pixel slots in that word, those unused pixel slots will be
        /// skipped such that the first pixel of the next row is assigned to the first slot
        /// in the next word.
        ///
        /// If `false`, pixel rows are required to be aligned with word boundaries, and no
        /// pixel slots are skipped. The last word, which is the one the last pixel is
        /// assigned to, may still have unused pixel slots.
        ///
        bool word_aligned_rows;

        /// \brief Which channels and in which order.
        ///
        /// This is a specification of which channels are present in the pixel buffer and in
        /// which order these channels occur.
        ///
        ChannelConf channel_conf;

        /// \brief Whether all consistency requirements are met.
        ///
        /// This function returns `true` if, and only if all the consistency requirements
        /// are met.
        ///
        /// The consistency requirements are:
        ///
        ///   * \ref bits_per_channel and \ref pixels_per_word must be greater than, or
        ///     equal to 1.
        ///
        ///   * The number of used bits per word must be less than, or equal to number of
        ///     available bits per word (\ref BufferFormat::get_bits_per_word(), \ref
        ///     word_type). The number of used bits per word is the number of pixels per
        ///     word (\ref pixels_per_word) times the number of channels (\ref
        ///     ChannelConf::get_num_channels(), \ref channel_conf) times the number of bits
        ///     per channel (\ref bits_per_channel).
        ///
        bool is_valid() const noexcept;

        /// \{
        ///
        /// \brief Try to cast subword format as format of different type.
        ///
        /// These functions are used by \ref BufferFormat::try_cast_to(), but can also be
        /// called directly by the application when the origin format is statically known to
        /// be a subword format (`SubwordFormat`).
        ///
        /// \sa \ref BufferFormat::try_cast_to()
        ///
        ///
        /// ### Subword to integer
        ///
        /// A subword format can be cast to an integer format if, and only if the cast takes
        /// one of the following two forms:
        ///
        /// <em>Form 1:</em> The origin word type (\ref word_type) is the same as the
        /// specified target word type (\p target_word_type), or there is only one byte per
        /// origin word (\ref BufferFormat::get_bytes_per_word()) and the specified target
        /// word type is `byte`. Additionally, the following conditions must hold:
        ///
        ///   * There is only one pixel per word in the subword format (\ref
        ///     pixels_per_word).
        ///
        ///   * There is only one channel (\ref ChannelConf::get_num_channels(), \ref
        ///     channel_conf).
        ///
        /// <em>Form 2:</em> There is more than one byte per origin word and the specified
        /// target word type is `byte`. Additionally, the following conditions must hold:
        ///
        ///   * The number of bits per channel component (\ref bits_per_channel) is an
        ///     integer multiple of the number of bits in a byte.
        ///
        ///   * There are no unused bits, which means that the number of pixels per origin
        ///     word (\ref pixels_per_word) times the number of channels (\ref
        ///     ChannelConf::get_num_channels(), \ref channel_conf) times the number of bits
        ///     per channel is equal to the number of bits per byte times the number of
        ///     bytes per origin word (\ref BufferFormat::get_bytes_per_word()).
        ///
        ///   * The native byte order for the origin word type is determinable (\ref
        ///     BufferFormat::try_get_byte_order()).
        ///
        ///   * If there is more than one pixel per origin word, the bit order in the
        ///     subword format (\ref bit_order) matches the native byte order for the origin
        ///     word type.
        ///
        ///   * If there is more than one pixel per origin word, pixel rows are not required
        ///     to be word aligned (\ref word_aligned_rows).
        ///
        /// When the cast succeeds, the parameters of the target format are set as follows:
        ///
        ///   * The word type (\ref IntegerFormat::word_type) is set to the specified target
        ///     word type.
        ///
        ///   * The number of bits per word (\ref IntegerFormat::bits_per_word) is set to
        ///     the number of bits per channel in the origin format (\ref bits_per_channel)
        ///     if the cast is on the first form (see above). Otherwise, the number of bits
        ///     per word is set to the number of bits in a byte (usually 8).
        ///
        ///   * The number of words per channel (\ref IntegerFormat::words_per_channel) is
        ///     set to one if the cast is on the first form (see above). Otherwise, the
        ///     number of words per channel is set to the number of bits per channel in the
        ///     origin format divided by the number of bits in a byte.
        ///
        ///   * The word order (\ref IntegerFormat::word_order) is set to 'big endian' if
        ///     the cast is on the first form (see above). Otherwise, the word order is set
        ///     to the native byte order for the origin word type.
        ///
        ///   * The channel configuration (\ref IntegerFormat::channel_conf) is set to the
        ///     channel configuration in the origin format except that, if the target word
        ///     order is 'little endian', the channel order will be reversed (\ref
        ///     ChannelConf::reverse()).
        ///
        ///
        /// ### Subword to packed
        ///
        /// A subword format can be cast to a packed format if, and only if the cast takes
        /// one of the following two forms and the number of channels (\ref
        /// ChannelConf::get_num_channels(), \ref channel_conf) is less than, or equal to
        /// the maximum number of bit fields in a packed format (\ref
        /// BufferFormat::max_bit_fields):
        ///
        /// <em>Form 1:</em> The origin word type (\ref word_type) is the same as the
        /// specified target word type (\p target_word_type), or there is only one byte per
        /// origin word (\ref BufferFormat::get_bytes_per_word()) and the specified target
        /// word type is `byte`. Additionally, the following conditions must hold:
        ///
        ///   * There is only one pixel per word in the subword format (\ref
        ///     pixels_per_word).
        ///
        /// <em>Form 2:</em> There is more than one byte per origin word and the specified
        /// target word type is `byte`. Additionally, the following conditions must hold:
        ///
        ///   * If there is more than one pixel per origin word, the number of bits per
        ///     pixel (number of channels times \ref bits_per_channel) is an integer
        ///     multiple of the number of bits in a byte.
        ///
        ///   * If there is more than one pixel per origin word, there are no unused bits,
        ///     which means that the number of pixels per origin word (\ref pixels_per_word)
        ///     times the number of channels times the number of bits per channel is equal
        ///     to the number of bits per byte times the number of bytes per origin word
        ///     (\ref BufferFormat::get_bytes_per_word()).
        ///
        ///   * The native byte order for the origin word type is determinable (\ref
        ///     BufferFormat::try_get_byte_order()).
        ///
        ///   * If there is more than one pixel per origin word, the bit order in the
        ///     subword format (\ref bit_order) matches the native byte order for the origin
        ///     word type.
        ///
        ///   * If there is more than one pixel per origin word, pixel rows are not required
        ///     to be word aligned (\ref word_aligned_rows).
        ///
        /// When the cast succeeds, the parameters of the target format are set as follows:
        ///
        ///   * The word type (\ref PackedFormat::word_type) is set to the specified target
        ///     word type.
        ///
        ///   * The number of bits per word (\ref PackedFormat::bits_per_word) is set to the
        ///     number of bits per pixel (number of channels times \ref bits_per_channel) if
        ///     the cast is on the first form (see above). Otherwise, the number of bits per
        ///     word is set to the number of bits in a byte (usually 8).
        ///
        ///   * The number of words per bit compound (\ref PackedFormat::words_per_pixel) is
        ///     set to one if the cast is on the first form (see above). Otherwise, the
        ///     number of words per bit compound is set to the number of bytes per origin
        ///     word divided by the number of pixels per origin word.
        ///
        ///   * The word order (\ref PackedFormat::word_order) is set to 'big endian' if the
        ///     cast is on the first form (see above). Otherwise, the word order is set to
        ///     the native byte order for the origin word type.
        ///
        ///   * The list of bit fields (\ref PackedFormat::bit_fields) is initialized such
        ///     that each field has a width equal to the number of bits per channel in the
        ///     origin format (\ref bits_per_channel) and all field gaps are zero (see \ref
        ///     image::BitField). Unused bit field slots will be set as if zero initialized.
        ///
        ///   * The channel configuration (\ref PackedFormat::channel_conf) is set to the
        ///     channel configuration in the origin format (\ref channel_conf).
        ///
        ///
        /// ### Subword to subword
        ///
        /// A subword format can be cast to another subword format if, and only if the cast
        /// takes one of the following two forms:
        ///
        /// <em>Form 1:</em> The origin word type (\ref word_type) is the same as the
        /// specified target word type (\p target_word_type), or there is only one byte per
        /// origin word (\ref BufferFormat::get_bytes_per_word()) and the specified target
        /// word type is `byte`.
        ///
        /// <em>Form 2:</em> There is more than one byte per origin word and the specified
        /// target word type is `byte`. Additionally, the following conditions must hold:
        ///
        ///   * There are no unused bits, which means that the number of pixels per word
        ///     (\ref pixels_per_word) times the number of channels (\ref
        ///     ChannelConf::get_num_channels(), \ref channel_conf) times the number of bits
        ///     per channel (\ref bits_per_channel) is equal to the number of bits per byte
        ///     times the number of bytes per origin word.
        ///
        ///   * There are a whole number of pixels per byte, which means that the number of
        ///     bytes per word evenly divides the number of pixels per word (\ref
        ///     pixels_per_word).
        ///
        ///   * The native byte order for the origin word type is determinable (\ref
        ///     BufferFormat::try_get_byte_order()).
        ///
        ///   * The bit order (\ref bit_order) matches the native byte order for the origin
        ///     word type.
        ///
        ///   * Pixel rows are not required to be word aligned (\ref word_aligned_rows).
        ///
        /// When the cast succeeds, the parameters of the target format are set as follows:
        ///
        ///   * The word type (\ref word_type) is set to the specified target word type.
        ///
        ///   * The number of bits per channel (\ref bits_per_channel) is set to the number
        ///     of bits per channel in the origin format.
        ///
        ///   * The number of pixels per word (\ref pixels_per_word) is set to the number of
        ///     pixels per word in the origin format if the cast is on the first form (see
        ///     above). Otherwise, the number of words per channel is set to that divided by
        ///     the number of bytes per origin word.
        ///
        ///   * The bit order (\ref bit_order) is set to the bit order in the origin format.
        ///
        ///   * Pixel rows will be required to be word aligned (\ref word_aligned_rows) if,
        ///     and only if pixel rows are required to be word aligned in the origin format.
        ///
        ///   * The channel configuration (\ref channel_conf) is set to the channel
        ///     configuration in the origin format.
        ///
        bool try_cast_to(IntegerFormat& format, IntegerType target_word_type) const;
        bool try_cast_to(PackedFormat& format, IntegerType target_word_type) const;
        bool try_cast_to(SubwordFormat& format, IntegerType target_word_type) const;
        /// \}
    };

    /// \brief Pixel buffer format with floating point channel components.
    ///
    /// This pixel buffer format stores each pixel as a sequence of channel components, one
    /// for each channel in the image (\ref BufferFormat::ChannelConf::get_num_channels(),
    /// \ref channel_conf). Each channel component is stored in a single memory word of
    /// floating-point type. The exact type is specified by \ref word_type.
    ///
    /// Pixels are stored contiguously in memory according to a top-to-bottom,
    /// left-to-right, row-major order.
    ///
    /// The channels occur in channel storage order (see \ref ChannelConf::reverse_order).
    ///
    /// The channel configuration (color space, presence of alpha channel, and channel
    /// order) is specified by \ref channel_conf.
    ///
    struct FloatFormat {
        /// \brief Pixel buffer element type.
        ///
        /// This is the type of memory elements that the described pixel buffer is made up
        /// of. For example, if the word type is `FloatType::float_` (see \ref
        /// BufferFormat::FloatType), it means that the memory needs to be accessed as an
        /// array of elements of type `float`.
        ///
        FloatType word_type;

        /// \brief Which channels and in which order.
        ///
        /// This is a specification of which channels are present in the pixel buffer and in
        /// which order these channels occur.
        ///
        ChannelConf channel_conf;

        /// \brief Whether all consistency requirements are met.
        ///
        /// This function always returns `true` because a floating-point format has no
        /// consistency requirements. It exists for the sake of alignment with the other
        /// format types.
        ///
        bool is_valid() const noexcept;
    };

    /// \brief Pixel buffer format using indexed color.
    ///
    /// With this format, each pixel in the pixel buffer is an index into a palette of
    /// colors.
    ///
    /// Pixels, i.e., indexes into the palette are stored in row-major order.
    ///
    ///                                      
    ///
    /// \sa \ref image::IndexedPixelFormat
    ///
    struct IndexedFormat {
        IntegerType word_type;
        int bits_per_pixel;
        int pixels_per_compound;
        int bits_per_word;
        int words_per_compound;
        core::Endianness bit_order;
        core::Endianness word_order;
        bool compound_aligned_rows;

        /// \brief Whether all consistency requirements are met.
        ///
        /// This function returns `true` if, and only if all the consistency requirements
        /// are met.
        ///
        /// The consistency requirements are:
        ///
        ///   * `bits_per_pixel` `pixels_per_compound`, `bits_per_word`, and
        ///     `words_per_compound` must all be greater than, or equal to 1.
        ///
        ///   * `bits_per_word` must be less than, or equal to number of available bits per
        ///     word (\ref BufferFormat::get_bits_per_word()).
        ///
        ///   * The number of bits per compound (`words_per_compound` times `bits_per_word`)
        ///     must be representable in type `int`.
        ///
        ///   * The number of bits per compound allocated for pixels (`pixels_per_compound`
        ///     times `bits_per_pixel`) must be less than, or equal to the total number of
        ///     bits per compound (`words_per_compound` times `bits_per_word`).
        ///
        bool is_valid() const noexcept;
    };

    enum class Type { integer,  packed, subword, float_, indexed };

    Type type;
    union {
        IntegerFormat integer;
        PackedFormat packed;
        SubwordFormat subword;
        FloatFormat float_;
        IndexedFormat indexed;
    };

    constexpr BufferFormat() noexcept = default;

    /// \{
    ///
    /// \brief Construct buffer format from specific sub-format.
    ///
    /// These constructors construct a buffer format from one of the available
    /// sub-types. They set both the type and the corresponding sub-format fields, e.g.,
    /// `type` and `integer`.
    ///
    constexpr BufferFormat(const IntegerFormat&) noexcept;
    constexpr BufferFormat(const PackedFormat&) noexcept;
    constexpr BufferFormat(const SubwordFormat&) noexcept;
    constexpr BufferFormat(const FloatFormat&) noexcept;
    constexpr BufferFormat(const IndexedFormat&) noexcept;
    /// \}

    /// \{
    ///
    /// \brief Initialize buffer format object.
    ///
    /// Except as noted below, these functions do not validate the consistency of the
    /// specified parameters. The caller can follow up with an invocation of \ref is_valid()
    /// in order to determine whether the constructed format is valid. A constructed format
    /// is valid when all of the documented requirements are met for the type of format
    /// constructed.
    ///
    /// For packed formats, the number of passed bit fields (\p bit_fields) must be equal to
    /// the number of channels (color channels and alpha channel when present). Also, the
    /// number of channels cannot be greater than \ref max_bit_fields. If these requirements
    /// are not met, `set_packed_format()` will throw.
    ///
    void set_integer_format(IntegerType word_type, int bits_per_word, int words_per_channel,
                            core::Endianness word_order, const image::ColorSpace&, bool has_alpha_channel,
                            bool alpha_channel_first, bool reverse_channel_order);
    void set_packed_format(IntegerType word_type, int bits_per_word, int words_per_pixel,
                           core::Endianness word_order,
                           core::Span<const image::BitField> bit_fields, const image::ColorSpace&,
                           bool has_alpha_channel, bool alpha_channel_first,
                           bool reverse_channel_order);
    void set_subword_format(IntegerType word_type, int bits_per_channel, int pixels_per_word,
                            core::Endianness bit_order, bool word_aligned_rows,
                            const image::ColorSpace&, bool has_alpha_channel, bool alpha_channel_first,
                            bool reverse_channel_order);
    void set_indexed_format(IntegerType word_type, int bits_per_pixel, int pixels_per_compound,
                            int bits_per_word, int words_per_compound, core::Endianness bit_order,
                            core::Endianness word_order, bool compound_aligned_rows);
    /// \}

    /// \brief Whether all consistency requirements are met.
    ///
    /// This function returns `true` if, and only if all the consistency requirements are
    /// met for the type of format described by this buffer format object.
    ///
    bool is_valid() const noexcept;

    /// \{
    ///
    /// \brief Try to downcast buffer format.
    ///
    /// These functions determine whether it is possible to reexpress this buffer format as
    /// the kind of format implied by the first argument, and with the additional
    /// requirement that the word type of the new format must be the specified word type (\p
    /// target_word_type).
    ///
    /// When it is possible to reexpress the original format as specified, these functions
    /// return `true` after setting \p format to the new format. Otherwise, these function
    /// return `false` and leave \p format unchanged.
    ///
    /// For details on each type of cast, see \ref IntegerFormat::try_cast_to(), \ref
    /// PackedFormat::try_cast_to(), \ref SubwordFormat::try_cast_to().
    ///
    /// When these functions return `true`, it means that pixels stored in memory according
    /// to the original format can be accessed as though they were stored according to the
    /// new format, i.e., the one specified by \p format upon return.
    ///
    /// CAUTION: The ability to cast format A to format B does *NOT* imply that pixels
    /// stored according to format B can be accessed as though they were stored according to
    /// format A. If it did, it would allow for memory allocated in terms of `char` to be
    /// accessed in terms of `int`, for example. This is not allowed in C++.
    ///
    /// A necessary, but insufficient condition for success is that the original buffer
    /// format is one of the integer based formats (\ref IntegerFormat, \ref PackedFormat,
    /// or \ref SubwordFormat), and that the specified word type (\p target_word_type) is
    /// either equal to the word type of the original buffer format, or is
    /// `IntegerType::byte`.
    ///
    /// A successful cast guarantees that the target format format is equivalent to the
    /// origin format for images of any size. Conversely, there could be a cast that fails
    /// even though an equivalent target format does exist for an image of a particular
    /// size.
    ///
    /// These function require that the original format is valid (\ref is_valid()), and they
    /// ensure that the new format is valid. If the original format is invalid, these
    /// functions throw.
    ///
    /// \sa \ref IntegerFormat, \ref PackedFormat, \ref SubwordFormat
    /// \sa \ref IntegerFormat::try_cast_to(), \ref PackedFormat::try_cast_to(), \ref SubwordFormat::try_cast_to()
    /// \sa \ref IntegerType
    ///
    bool try_cast_to(IntegerFormat& format, IntegerType target_word_type) const;
    bool try_cast_to(PackedFormat& format, IntegerType target_word_type) const;
    bool try_cast_to(SubwordFormat& format, IntegerType target_word_type) const;
    /// \}

    /// \brief Number of available bits for word type.
    ///
    /// This function returns the number of available bits in the specified word type. This
    /// is the value of \ref image::bit_width for the word type that \p word_type refers to.
    ///
    static constexpr int get_bits_per_word(IntegerType word_type) noexcept;

    /// \brief Number of bytes per word of specified type.
    ///
    /// This function returns the number of bytes that constitute a word of the specified
    /// type. If `W` is the type referred to by \p word_type, then the returned value is the
    /// value of `sizeof (W)`.
    ///
    static constexpr int get_bytes_per_word(IntegerType word_type) noexcept;

    /// \brief Determine native byte order for specific integer type.
    ///
    /// This function uses \ref core::try_get_byte_order() to determine the native byte
    /// order of the specified integer type.
    ///
    /// \sa \ref core::try_get_byte_order()
    ///
    static bool try_get_byte_order(IntegerType, core::Endianness& byte_order) noexcept;

    /// \brief Map type to corresponding integer type value.
    ///
    /// This function tries to map the specified type (\p W) to the corresponding value of
    /// the \ref IntegerType enumeration. If there is a corresponding enumeration value,
    /// this function sets \p type to that value and returns `true`. Otherwise it returns
    /// `false` and leaves \p type unchanged. See \ref IntegerType for the table of
    /// corresponding types and enumeration values.
    ///
    template<class W> static constexpr bool try_map_integer_type(IntegerType& type) noexcept;

    /// \brief Map type to corresponding floating-point type value.
    ///
    /// This function tries to map the specified type (\p W) to the corresponding value of
    /// the \ref FloatType enumeration. If there is a corresponding enumeration value, this
    /// function sets \p type to that value and returns `true`. Otherwise it returns `false`
    /// and leaves \p type unchanged. See \ref FloatType for the table of corresponding
    /// types and enumeration values.
    ///
    template<class W> static constexpr bool try_map_float_type(FloatType& type) noexcept;
};








// Implementation


} // namespace archon::image

namespace archon::core {


template<> struct EnumTraits<image::BufferFormat::IntegerType> {
    static constexpr bool is_specialized = true;
    struct Spec {
        static constexpr core::EnumAssoc map[] = {
            { int(image::BufferFormat::IntegerType::byte),   "byte"   },
            { int(image::BufferFormat::IntegerType::schar),  "schar"  },
            { int(image::BufferFormat::IntegerType::short_), "short"  },
            { int(image::BufferFormat::IntegerType::ushort), "ushort" },
            { int(image::BufferFormat::IntegerType::int_),   "int"    },
            { int(image::BufferFormat::IntegerType::uint),   "uint"   },
            { int(image::BufferFormat::IntegerType::long_),  "long"   },
            { int(image::BufferFormat::IntegerType::ulong),  "ulong"  },
            { int(image::BufferFormat::IntegerType::llong),  "llong"  },
            { int(image::BufferFormat::IntegerType::ullong), "ullong" },
            { int(image::BufferFormat::IntegerType::fict_1), "fict_1" },
            { int(image::BufferFormat::IntegerType::fict_2), "fict_2" },
            { int(image::BufferFormat::IntegerType::fict_3), "fict_3" },
        };
    };
    static constexpr bool ignore_case = false;
};


template<> struct EnumTraits<image::BufferFormat::FloatType> {
    static constexpr bool is_specialized = true;
    struct Spec {
        static constexpr core::EnumAssoc map[] = {
            { int(image::BufferFormat::FloatType::float_),  "float"   },
            { int(image::BufferFormat::FloatType::double_), "double"  },
            { int(image::BufferFormat::FloatType::ldouble), "ldouble" },
        };
    };
    static constexpr bool ignore_case = false;
};


} // namespace archon::core

namespace archon::image {


inline int BufferFormat::ChannelConf::get_num_channels() const noexcept
{
    return color_space->get_num_channels() + int(has_alpha);
}


inline void BufferFormat::ChannelConf::reverse() noexcept
{
    reverse_order = !reverse_order;
}


inline bool BufferFormat::IntegerFormat::is_valid() const noexcept
{
    return (bits_per_word > 0 && words_per_channel > 0 &&
            bits_per_word <= get_bits_per_word(word_type) &&
            words_per_channel <= core::int_max<int>() / bits_per_word);
}


inline auto BufferFormat::IntegerFormat::get_words_per_row(int image_width) const -> std::size_t
{
    std::size_t n = 1;
    core::int_mul(n, words_per_channel); // Throws
    core::int_mul(n, channel_conf.get_num_channels()); // Throws
    core::int_mul(n, image_width); // Throws
    return n;
}


inline bool BufferFormat::PackedFormat::is_valid() const noexcept
{
    int num_channels = channel_conf.get_num_channels();
    return (bits_per_word > 0 && words_per_pixel > 0 &&
            bits_per_word <= get_bits_per_word(word_type) &&
            words_per_pixel <= core::int_max<int>() / bits_per_word &&
            num_channels <= max_bit_fields &&
            image::valid_bit_fields(bit_fields.data(), num_channels, words_per_pixel * bits_per_word));
}


inline bool BufferFormat::SubwordFormat::is_valid() const noexcept
{
    int avail_bits_per_word = get_bits_per_word(word_type);
    int num_channels = channel_conf.get_num_channels();
    return (bits_per_channel > 0 && pixels_per_word > 0 &&
            num_channels <= core::int_max<int>() / bits_per_channel &&
            pixels_per_word <= avail_bits_per_word / (num_channels * bits_per_channel));
}


inline bool BufferFormat::FloatFormat::is_valid() const noexcept
{
    return true;
}


inline bool BufferFormat::IndexedFormat::is_valid() const noexcept
{
    return (bits_per_pixel > 0 && pixels_per_compound > 0 &&
            bits_per_word > 0 && words_per_compound > 0 &&
            bits_per_word <= get_bits_per_word(word_type) &&
            words_per_compound <= core::int_max<int>() / bits_per_word &&
            pixels_per_compound <= (words_per_compound * bits_per_word) / bits_per_pixel);
}


constexpr BufferFormat::BufferFormat(const IntegerFormat& format) noexcept
{
    type = Type::integer;
    integer = format;
}


constexpr BufferFormat::BufferFormat(const PackedFormat& format) noexcept
{
    type = Type::packed;
    packed = format;
}


constexpr BufferFormat::BufferFormat(const SubwordFormat& format) noexcept
{
    type = Type::subword;
    subword = format;
}


constexpr BufferFormat::BufferFormat(const FloatFormat& format) noexcept
{
    type = Type::float_;
    float_ = format;
}


constexpr BufferFormat::BufferFormat(const IndexedFormat& format) noexcept
{
    type = Type::indexed;
    indexed = format;
}


inline void BufferFormat::set_integer_format(IntegerType word_type, int bits_per_word, int words_per_channel,
                                             core::Endianness word_order, const image::ColorSpace& color_space,
                                             bool has_alpha_channel, bool alpha_channel_first,
                                             bool reverse_channel_order)
{
    ChannelConf channel_conf = {
        &color_space,
        has_alpha_channel,
        alpha_channel_first,
        reverse_channel_order,
    };
    type = Type::integer;
    integer = {
        word_type,
        bits_per_word,
        words_per_channel,
        word_order,
        channel_conf,
    };
}


inline void BufferFormat::set_packed_format(IntegerType word_type, int bits_per_word, int words_per_pixel,
                                            core::Endianness word_order, core::Span<const image::BitField> bit_fields,
                                            const image::ColorSpace& color_space, bool has_alpha_channel,
                                            bool alpha_channel_first, bool reverse_channel_order)
{
    ChannelConf channel_conf = {
        &color_space,
        has_alpha_channel,
        alpha_channel_first,
        reverse_channel_order,
    };
    int num_channels = channel_conf.get_num_channels();
    ARCHON_ASSERT(bit_fields.size() <= std::size_t(max_bit_fields));
    ARCHON_ASSERT(int(bit_fields.size()) == num_channels);
    if (ARCHON_UNLIKELY(core::int_not_equal(bit_fields.size(), num_channels) || num_channels > max_bit_fields))
        throw std::runtime_error("Bad number of bit fields");
    type = Type::packed;
    packed = {
        word_type,
        bits_per_word,
        words_per_pixel,
        word_order,
        {},
        channel_conf,
    };
    std::copy_n(bit_fields.data(), bit_fields.size(), packed.bit_fields.data());
}


inline void BufferFormat::set_subword_format(IntegerType word_type, int bits_per_channel, int pixels_per_word,
                                             core::Endianness bit_order, bool word_aligned_rows,
                                             const image::ColorSpace& color_space, bool has_alpha_channel,
                                             bool alpha_channel_first, bool reverse_channel_order)
{
    ChannelConf channel_conf = {
        &color_space,
        has_alpha_channel,
        alpha_channel_first,
        reverse_channel_order,
    };
    type = Type::subword;
    subword = {
        word_type,
        bits_per_channel,
        pixels_per_word,
        bit_order,
        word_aligned_rows,
        channel_conf,
    };
}


inline void BufferFormat::set_indexed_format(IntegerType word_type, int bits_per_pixel, int pixels_per_compound,
                                             int bits_per_word, int words_per_compound, core::Endianness bit_order,
                                             core::Endianness word_order, bool compound_aligned_rows)
{
    type = Type::indexed;
    indexed = {
        word_type,
        bits_per_pixel,
        pixels_per_compound,
        bits_per_word,
        words_per_compound,
        bit_order,
        word_order,
        compound_aligned_rows,
    };
}


constexpr int BufferFormat::get_bits_per_word(IntegerType word_type) noexcept
{
    switch (word_type) {
        case IntegerType::byte:
        case IntegerType::schar:
            return image::bit_width<char>;
        case IntegerType::short_:
        case IntegerType::ushort:
            return image::bit_width<short>;
        case IntegerType::int_:
        case IntegerType::uint:
            return image::bit_width<int>;
        case IntegerType::long_:
        case IntegerType::ulong:
            return image::bit_width<long>;
        case IntegerType::llong:
        case IntegerType::ullong:
            return image::bit_width<long long>;
        case IntegerType::fict_1:
        case IntegerType::fict_2:
        case IntegerType::fict_3:
            return get_bytes_per_word(word_type) * image::bit_width<char>;
    }
    ARCHON_ASSERT_UNREACHABLE();
    return 0;
}


constexpr int BufferFormat::get_bytes_per_word(IntegerType word_type) noexcept
{
    switch (word_type) {
        case IntegerType::byte:
        case IntegerType::schar:
            return 1;
        case IntegerType::short_:
        case IntegerType::ushort:
            return int(sizeof (short));
        case IntegerType::int_:
        case IntegerType::uint:
            return int(sizeof (int));
        case IntegerType::long_:
        case IntegerType::ulong:
            return int(sizeof (long));
        case IntegerType::llong:
        case IntegerType::ullong:
            return int(sizeof (long long));
        case IntegerType::fict_1:
        case IntegerType::fict_2:
        case IntegerType::fict_3:
            return 2;
    }
    ARCHON_ASSERT_UNREACHABLE();
    return 0;
}


template<class W> constexpr bool BufferFormat::try_map_integer_type(IntegerType& word_type) noexcept
{
    if constexpr (std::is_same_v<W, std::byte> || std::is_same_v<W, char> || std::is_same_v<W, unsigned char>) {
        word_type = IntegerType::byte;
        return true;
    }
    else if constexpr (std::is_same_v<W, signed char>) {
        word_type = IntegerType::schar;
        return true;
    }
    else if constexpr (std::is_same_v<W, short>) {
        word_type = IntegerType::short_;
        return true;
    }
    else if constexpr (std::is_same_v<W, unsigned short>) {
        word_type = IntegerType::ushort;
        return true;
    }
    else if constexpr (std::is_same_v<W, int>) {
        word_type = IntegerType::int_;
        return true;
    }
    else if constexpr (std::is_same_v<W, unsigned>) {
        word_type = IntegerType::uint;
        return true;
    }
    else if constexpr (std::is_same_v<W, long>) {
        word_type = IntegerType::long_;
        return true;
    }
    else if constexpr (std::is_same_v<W, unsigned long>) {
        word_type = IntegerType::ulong;
        return true;
    }
    else if constexpr (std::is_same_v<W, long long>) {
        word_type = IntegerType::llong;
        return true;
    }
    else if constexpr (std::is_same_v<W, unsigned long long>) {
        word_type = IntegerType::ullong;
        return true;
    }
    else {
        return true;
    }
}


template<class W> constexpr bool BufferFormat::try_map_float_type(FloatType& word_type) noexcept
{
    if constexpr (std::is_same_v<W, float>) {
        word_type = FloatType::float_;
        return true;
    }
    else if constexpr (std::is_same_v<W, double>) {
        word_type = FloatType::double_;
        return true;
    }
    else if constexpr (std::is_same_v<W, long double>) {
        word_type = FloatType::ldouble;
        return true;
    }
    else {
        return true;
    }
}


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_BUFFER_FORMAT_HPP
