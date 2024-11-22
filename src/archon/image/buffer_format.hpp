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

#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
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
    /// IntegerFormat, \ref SubwordFormat, and \ref IndexedFormat.
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
    enum class IntegerType { byte, schar, short_, ushort, int_, uint, long_, ulong, llong, ullong };

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
    enum class FloatType { float_, double_, ldouble };

    struct ChannelConf {
        const image::ColorSpace* color_space;
        bool has_alpha;
        bool alpha_channel_first;
        bool reverse_order;

        int get_num_channels() const noexcept;
        void reverse() noexcept;
    };

    /// \brief One or more words per channel.
    ///
    /// Pixels are stored in row-major order.
    ///
    /// Each word contributes bits to at most one channel.       
    ///
    /// Consistency requirements:
    ///
    ///   * `bits_per_word` and `words_per_channel` must be greater than, or equal to 1.
    ///
    ///   * `bits_per_word` must be less than, or equal to number of availble bits per word
    ///     (\ref get_bits_per_word()).
    ///
    ///   * The number of bits per channel (`words_per_channel` times `bits_per_word`) must
    ///     be representable in type `int`.
    ///
    struct IntegerFormat {
        IntegerType word_type;
        int bits_per_word;
        int words_per_channel;
        core::Endianness word_order;
        ChannelConf channel_conf;

        /// \brief Whether all consistency requirements are met.
        ///
        /// This function returns `true` if, and only if all the consistency requirements
        /// are met.
        ///
        bool is_valid() const noexcept;

        auto get_words_per_row(int image_width) const -> std::size_t;
    };

    /// \brief Maximum number of bit fields in packed format.
    ///
    /// This is the maximum number of bit fields, and therefore the maximum number of
    /// channels that can be descibed by \ref PackedFormat.
    ///
    static constexpr int max_bit_fields = 8;

    static_assert(max_bit_fields >= 4); // Documented promise

    /// \brief Channels packed into bit compound.
    ///
    /// A packed format groups words into groups of a fixed size. A bit compound is then
    /// constructed from each group of words in a customizable way. Each bit compound stores
    /// a single pixel. The channels are layed out as bit fields within the bit compound.
    ///
    /// Only the first N entries in `bit_fields` have meaningful values, where N is the
    /// number of channels (`channel_conf.get_num_channels()`). Pixel formats with more than
    /// \ref max_bit_fields channels cannot be described using this class.
    ///
    /// Consistency requirements:
    ///
    ///   * `bits_per_word` and `words_per_pixel` must be greater than, or equal to 1.
    ///
    ///   * `bits_per_word` must be less than, or equal to number of availble bits per word
    ///     (\ref get_bits_per_word()).
    ///
    ///   * The number of bits per compound (`words_per_pixel` times `bits_per_word`) must
    ///     be representable in type `int`.
    ///
    ///   * The number of channels (\ref ChannelConf::get_num_channels()) must be less than,
    ///     or equal to \ref max_bit_fields.
    ///
    ///   * If N is the number of channels (\ref ChannelConf::get_num_channels()), then the
    ///     first N bit fields must describe a valid sequence of bit fields that fit within
    ///     the specified number of bits per compound (`words_per_pixel` times
    ///     `bits_per_word`). See \ref image::valid_bit_fields() for an explanation of what
    ///     it means for a sequence of bit fields to be valid and fit within a certain
    ///     number of bits.
    ///
    struct PackedFormat {
        IntegerType word_type;
        int bits_per_word;
        int words_per_pixel;
        core::Endianness word_order;
        image::BitField bit_fields[max_bit_fields];
        ChannelConf channel_conf;

        /// \brief Whether all consistency requirements are met.
        ///
        /// This function returns `true` if, and only if all the consistency requirements
        /// are met.
        ///
        bool is_valid() const noexcept;
    };

    /// \brief One or more pixels per word.
    ///
    /// With this format, each word contains one or more pixels, and pixles cannot cross
    /// word boundaries. Pixels are stored in row-major order.
    ///
    /// Each pixel consists of N contiguously arranged channel slots of M bits each, where N
    /// is the number of channels and M is the number of bits per channel, i.e., \p
    /// bits_per_channel. The first channel always goes into the channel slot covering the
    /// most significant bits, regardless of the value of \p bit_order. The number of,
    /// meaning of, and order of channels is specified by \p channel_conf.
    ///
    /// The specified bit order (\p bit_order) controls the order of pixles within the bits
    /// of a word. It has no effect, however, on the order that the channels of a single
    /// pixel occur within the bits of a word. When the bit order is little-endian, the
    /// first, or left-most pixel in a word occupies the least significant N times M bits in
    /// that word. When the bit order is big-endian, the last, or right-most pixel in a word
    /// occupies the least significant N times M bits in that word. Therefore, if there are
    /// unused bits in the word, i.e., if N times M times the number of pixels per word (\p
    /// pixels_per_word) is less than the bit width of the word type (\p word_type), those
    /// unused bits are always the most significant bits of the word, regardless of the
    /// specified bit order.
    ///
    /// If rows are required to be word aligned (\p word_aligned_rows) and the last, or
    /// right-most pixel in a row is not the last pixel in the word, the reamining pixels in
    /// that word will be skipped, and the next row will start with the first pixel in the
    /// next word. If rows are not required to be word aligned, and the last, or right-most
    /// pixel in a row is not the last pixel in the word, then the next row starts with the
    /// next pixel in that word.
    ///
    /// Consistency requirements:
    ///
    ///   * `bits_per_channel` and `pixels_per_word` must be greater than, or equal to 1.
    ///
    ///   * The number of bits per word (`pixels_per_word` times number of channels times
    ///     `bits_per_channel`) must be less than, or equal to number of availble bits per
    ///     word (\ref get_bits_per_word()).
    ///
    struct SubwordFormat {
        IntegerType word_type;
        int bits_per_channel;
        int pixels_per_word;
        core::Endianness bit_order;
        bool word_aligned_rows;
        ChannelConf channel_conf;

        /// \brief Whether all consistency requirements are met.
        ///
        /// This function returns `true` if, and only if all the consistency requirements
        /// are met.
        ///
        bool is_valid() const noexcept;
    };

    /// \brief     
    ///
    ///    
    ///
    /// Pixels are stored in row-major order.
    ///
    struct FloatFormat {
        FloatType word_type;
        ChannelConf channel_conf;

        /// \brief Whether all consistency requirements are met.
        ///
        /// This function always retuns `true` because a floating-point format has no
        /// consistency requirements. It exists for the sake of alignment with the other
        /// format types.
        ///
        bool is_valid() const noexcept;
    };

    /// \brief Indexed color.
    ///
    /// With this format, each pixel in the pixel buffer is an index into a palette of
    /// colors.
    ///
    /// Pixels, i.e., indexes into the palette are stored in row-major order.
    ///
    ///                                      
    ///
    /// Consistency requirements:
    ///
    ///   * `bits_per_pixel` `pixels_per_compound`, `bits_per_word`, and
    ///     `words_per_compound` must all be greater than, or equal to 1.
    ///
    ///   * `bits_per_word` must be less than, or equal to number of availble bits per word
    ///     (\ref get_bits_per_word()).
    ///
    ///   * The number of bits per compound (`words_per_compound` times `bits_per_word`)
    ///     must be representable in type `int`.
    ///
    ///   * The number of bits per compound allocated for pixels (`pixels_per_compound`
    ///     times `bits_per_pixel`) must be less than, or equal to the total number of bits
    ///     per compound (`words_per_compound` times `bits_per_word`).
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

    /// \{
    ///
    /// \brief Initialize buffer format object.
    ///
    /// Except as noted below, these functions do not validate the consitency of the
    /// specified parameters. The caller can follow up with an invocation of \ref is_valid()
    /// in order to determine whether the constructed format is valid. A constructed format
    /// is valid when all of the documented requirements are met for the type of format
    /// constructed.
    ///
    /// For packed formats, the number of passed bit fields (\p bit_fields) must be equal to
    /// the number of channels (color channels and alpha channel when present). Also, the
    /// number of cannels cannot be greater than \ref max_bit_fields. If these requirements
    /// are not met, `set_packed_format()` will throw.
    ///
    void set_integer_format(IntegerType word_type, int bits_per_word, int words_per_channel,
                            core::Endianness word_order, const image::ColorSpace&, bool has_alpha,
                            bool alpha_channel_first, bool reverse_channel_order);
    void set_packed_format(IntegerType word_type, int bits_per_word, int words_per_pixel,
                           core::Endianness word_order,
                           core::Span<const image::BitField> bit_fields, const image::ColorSpace&,
                           bool has_alpha, bool alpha_channel_first,
                           bool reverse_channel_order);
    void set_subword_format(IntegerType word_type, int bits_per_channel, int pixels_per_word,
                            core::Endianness bit_order, bool word_aligned_rows,
                            const image::ColorSpace&, bool has_alpha, bool alpha_channel_first,
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
    /// the kind of format implied by the first argument with the additional requirement
    /// that the word type of the new format must be as specified.
    ///
    /// When it is possible to reexpress the original format as specified, these functions
    /// return `true` after setting \p format to the new format. Otherwise, these function
    /// return `false` and leave \p format unchanged.
    ///
    /// When these function return `true`, it means that pixels stored in memory according
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
    /// or \ref SubwordFormat), and that the specified word type (\p word_type) is either
    /// equal to the word type of the original buffer format, or is `IntegerType::byte`.
    ///
    /// These function require that the original format is valid (\ref is_valid()), and they
    /// ensure that the new format is valid.
    ///
    /// \sa \ref IntegerFormat, \ref PackedFormat, \ref SubwordFormat
    /// \sa \ref IntegerType
    ///
    bool try_cast_to(IntegerFormat& format, IntegerType word_type) const;
    bool try_cast_to(PackedFormat& format, IntegerType word_type) const;
    bool try_cast_to(SubwordFormat& format, IntegerType word_type) const;
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

    static bool try_get_byte_order(IntegerType, core::Endianness& byte_order) noexcept;

    template<class W> static constexpr bool try_map_integer_type(IntegerType&) noexcept;
    template<class W> static constexpr bool try_map_float_type(FloatType&) noexcept;
};








// Implementation


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
            image::valid_bit_fields(bit_fields, num_channels, words_per_pixel * bits_per_word));
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


inline void BufferFormat::set_integer_format(IntegerType word_type, int bits_per_word, int words_per_channel,
                                             core::Endianness word_order, const image::ColorSpace& color_space,
                                             bool has_alpha, bool alpha_channel_first, bool reverse_channel_order)
{
    ChannelConf channel_conf = {
        &color_space,
        has_alpha,
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
                                            const image::ColorSpace& color_space, bool has_alpha,
                                            bool alpha_channel_first, bool reverse_channel_order)
{
    ChannelConf channel_conf = {
        &color_space,
        has_alpha,
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
    std::copy_n(bit_fields.data(), bit_fields.size(), packed.bit_fields);
}


inline void BufferFormat::set_subword_format(IntegerType word_type, int bits_per_channel, int pixels_per_word,
                                             core::Endianness bit_order, bool word_aligned_rows,
                                             const image::ColorSpace& color_space, bool has_alpha,
                                             bool alpha_channel_first, bool reverse_channel_order)
{
    ChannelConf channel_conf = {
        &color_space,
        has_alpha,
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
