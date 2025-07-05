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

#ifndef ARCHON_X_IMAGE_X_SUBWORD_PIXEL_FORMAT_HPP
#define ARCHON_X_IMAGE_X_SUBWORD_PIXEL_FORMAT_HPP

/// \file


#include <cstddef>
#include <type_traits>
#include <utility>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/endianness.hpp>
#include <archon/image/geom.hpp>
#include <archon/image/tray.hpp>
#include <archon/image/comp_types.hpp>
#include <archon/image/gamma.hpp>
#include <archon/image/comp_repr.hpp>
#include <archon/image/color_space.hpp>
#include <archon/image/standard_channel_spec.hpp>
#include <archon/image/buffer_format.hpp>
#include <archon/image/image.hpp>


namespace archon::image {


/// \brief Specification of pixel format with multiple pixels per word.
///
/// An instantiation of this template is used to specify a pixel format that allows for
/// multiple pixels per integer word. Such a pixel format implements \ref
/// Concept_Archon_Image_PixelFormat and can therefore be used with \ref
/// image::BufferedImage.
///
/// Each pixel consists of N contiguously arranged channel slots of M bits each, where N is
/// the number of channels (determined by \p C) and M is the number of bits per channel (\p
/// B). The first channel always goes into the channel slot covering the most significant
/// bits, regardless of the specified bit order (\p E). The order of channels is determined
/// by \p F (alpha channel first) and \p G (reverse channel order).
///
/// The specified bit order (\p E) controls the order of pixels within the bits of a
/// word. It has no effect, however, on the order of occurance of channels within the bits
/// that are set aside for a single pixel. When the bit order is little-endian, the first,
/// or left-most pixel in a word occupies the least significant N times M bits in that
/// word. When the bit order is big-endian, the first, or left-most pixel in a word occupies
/// the most significant N times M bits within the least significant R bits of that word,
/// where R is M times N times number of pixels per word (\p D).
///
/// If rows are required to be word aligned (\p H is `true`) and the last, or right-most
/// pixel in a row is not the last pixel in the word, the remaining pixels in that word will
/// be skipped, and the next row will start with the first pixel in the next word. If rows
/// are not required to be word aligned (\p H is `false`), and the last, or right-most pixel
/// in a row is not the last pixel in the word, then the next row starts with the next pixel
/// in that word.
///
/// For formats that store one channel in each bit compound, see \ref
/// image::InterPixelFormat.
///
/// For formats that pack multiple channels into each bit compound, see \ref
/// image::PackedPixelFormat.
///
/// For floating-point based formats, see \ref image::FloatPixelFormat.         
///
/// For indirect color formats, see \ref image::IndexedPixelFormat.
///
/// Unused bits must be zero. This includes unused bits in words (at positions of
/// significance higher than \p B times \p D) and bits associated with unused pixel slots at
/// end of pixel rows when the next row is aligned at a word boundary (\p H). Behavior is
/// undefined if this pixel format is used with a pixel buffer where these bits are not
/// zero. Conversely, when these bits are zero, this pixel format guarantees that they will
/// remain zero.
///
/// Behavior is undefined if this pixel format is used with a pixel buffer whose size is not
/// equal to `get_buffer_size(image_size)` where `image_size` is the image size passed to
/// \ref read(), \ref write(), or \ref fill(). See \ref Concept_Archon_Image_PixelFormat for
/// documentation of `get_buffer_size()`.
///
/// This class is an empty class if, and only if the given channel specification (\p C) is
/// an empty class.
///
/// \tparam C Channel specification. See \ref Concept_Archon_Image_ChannelSpec and \ref
/// image::StandardChannelSpec.
///
/// \tparam W Memory will be accessed in terms of words of this type.
///
/// \tparam B Number of bits per channel. Two to the power of this number minus one
/// specifies the maximum value for channel components.
///
/// \tparam D Number of pixels per word.
///
/// \tparam E The order in which pixels occur within the bits of a word. See description
/// above for details.
///
/// \tparam F Controls whether the alpha channel comes first or last. The alpha channel
/// comes first (before the color channels) if \p F is set to `true` and \p G is set to
/// `false`, or if \p F is set to `false` and \p G is set to `true`. If \p F and \p G are
/// both set to `false` or both set to `true`, the alpha channel comes last.
///
/// \tparam G If set to `true`, the stored order of color channels is opposite of the
/// canonical order for the color space in effect. This also affects the position of the
/// alpha channel, see \p F.
///
/// \tparam H If set to `true`, each row of pixels is aligned on a word boundary.
///
/// \sa \ref image::BufferFormat::SubwordFormat
///
template<class C, class W, int B, int D, core::Endianness E = core::Endianness::big,
         bool F = false, bool G = false, bool H = true>
class SubwordPixelFormat {
public:
    using channel_spec_type = C;
    using word_type         = W;

    static constexpr int bits_per_channel       = B;
    static constexpr int pixels_per_word        = D;
    static constexpr core::Endianness bit_order = E;
    static constexpr bool alpha_channel_first   = F;
    static constexpr bool reverse_channel_order = G;
    static constexpr bool word_aligned_rows     = H;

    static constexpr bool has_alpha_channel = channel_spec_type::has_alpha_channel;
    static constexpr int num_channels = channel_spec_type::num_channels;
    static constexpr int bits_per_pixel = num_channels * bits_per_channel;
    static constexpr int bits_per_word = pixels_per_word * bits_per_pixel;

    static_assert(std::is_integral_v<word_type>);
    static_assert(bits_per_channel > 0);
    static_assert(pixels_per_word > 0);
    static_assert(num_channels <= core::int_max<int>() / bits_per_channel);
    static_assert(pixels_per_word <= core::int_max<int>() / bits_per_pixel);
    static_assert(bits_per_word <= image::bit_width<word_type>);

    /// \brief Construct subword pixel format with given channel specification.
    ///
    /// This constructor constructs a subword pixel format with the given channel
    /// specification.
    ///
    SubwordPixelFormat(channel_spec_type = {});

    /// \{
    ///
    /// \brief Required static pixel format members.
    ///
    /// See \ref Concept_Archon_Image_PixelFormat.
    ///
    static constexpr bool is_indexed_color = false;
    static constexpr image::CompRepr transf_repr = image::choose_transf_repr(bits_per_channel);
    /// \}

    using transf_comp_type = image::comp_type<transf_repr>;

    /// \{
    ///
    /// \brief Required pixel format member functions.
    ///
    /// See \ref Concept_Archon_Image_PixelFormat.
    ///
    static auto get_buffer_size(image::Size) -> std::size_t;
    bool try_describe(image::BufferFormat&) const;
    auto get_transfer_info() const noexcept -> image::Image::TransferInfo;
    static void read(const word_type* buffer, image::Size image_size, image::Pos,
                     const image::Tray<transf_comp_type>&) noexcept;
    static void write(word_type* buffer, image::Size image_size, image::Pos,
                      const image::Tray<const transf_comp_type>&) noexcept;
    static void fill(word_type* buffer, image::Size image_size, const image::Box& area,
                     const transf_comp_type* color) noexcept;
    /// \}

    /// \brief Number of words per row of pixels.
    ///
    /// If rows are aligned at word boundaries (\p word_aligned_rows), this function returns
    /// the number of words (elements of type \p word_type) that make up each row of an
    /// image of the specified width.
    ///
    static constexpr auto get_words_per_row(int image_width) -> std::size_t;

private:
    struct PixelPos {
        std::ptrdiff_t word_index;
        int pixel_pos;
    };

    static auto get_pixel_pos(int image_width, image::Pos pos) -> PixelPos;

    // A type that is "prepromoted" and also avoids undefined behaviour when bits are
    // shifted into and out of the highest relevant bit position.
    using value_type = image::unpacked_type<word_type, bits_per_word>;

    static auto read_word(const word_type* source) noexcept -> value_type;

    static void write_word(value_type word, word_type* target) noexcept;

    static void get_pixel(value_type word, int pos, transf_comp_type* target) noexcept;
    static void set_pixel(const transf_comp_type* source, value_type& word, int pos) noexcept;

    static auto encode_pixel(const transf_comp_type* source) noexcept -> value_type;

    static auto read_comp(value_type pixel, int channel) noexcept -> transf_comp_type;
    static void write_comp(transf_comp_type comp, value_type& pixel, int channel) noexcept;

    static void do_set_pixel(value_type pixel, value_type& word, int pos) noexcept;

    static constexpr int map_channel_pos(int) noexcept;
    static constexpr int map_pixel_pos(int) noexcept;

    ARCHON_NO_UNIQUE_ADDRESS channel_spec_type m_channel_spec;
};


template<class W, int B, int D, core::Endianness E = core::Endianness::big, bool H = true>
using SubwordPixelFormat_Lum = image::SubwordPixelFormat<image::ChannelSpec_Lum, W, B, D, E, false, false, H>;

template<class W, int B, int D, core::Endianness E = core::Endianness::big, bool H = true>
using SubwordPixelFormat_LumA = image::SubwordPixelFormat<image::ChannelSpec_LumA, W, B, D, E, false, false, H>;

template<class W, int B, int D, core::Endianness E = core::Endianness::big, bool H = true>
using SubwordPixelFormat_RGB = image::SubwordPixelFormat<image::ChannelSpec_RGB, W, B, D, E, false, false, H>;

template<class W, int B, int D, core::Endianness E = core::Endianness::big, bool H = true>
using SubwordPixelFormat_RGBA = image::SubwordPixelFormat<image::ChannelSpec_RGBA, W, B, D, E, false, false, H>;








// Implementation


template<class C, class W, int B, int D, core::Endianness E, bool F, bool G, bool H>
inline SubwordPixelFormat<C, W, B, D, E, F, G, H>::SubwordPixelFormat(channel_spec_type spec)
    : m_channel_spec(std::move(spec)) // Throws
{
}


template<class C, class W, int B, int D, core::Endianness E, bool F, bool G, bool H>
auto SubwordPixelFormat<C, W, B, D, E, F, G, H>::get_buffer_size(image::Size image_size) -> std::size_t
{
    if constexpr (word_aligned_rows) {
        std::size_t size = get_words_per_row(image_size.width); // Throws
        core::int_mul(size, image_size.height); // Throws
        return size;
    }
    else {
        std::size_t n = 1;
        core::int_mul(n, image_size.width); // Throws
        core::int_mul(n, image_size.height); // Throws
        return core::int_div_round_up(n, pixels_per_word);
    }
}


template<class C, class W, int B, int D, core::Endianness E, bool F, bool G, bool H>
bool SubwordPixelFormat<C, W, B, D, E, F, G, H>::try_describe(image::BufferFormat& format) const
{
    const image::ColorSpace& color_space = m_channel_spec.get_color_space();
    image::BufferFormat::IntegerType word_type_2 = {};
    if (ARCHON_LIKELY(image::BufferFormat::try_map_integer_type<word_type>(word_type_2))) {
        format.set_subword_format(word_type_2, bits_per_channel, pixels_per_word, bit_order, word_aligned_rows,
                                  color_space, has_alpha_channel, alpha_channel_first,
                                  reverse_channel_order); // Throws
        return true;
    }
    return false;
}


template<class C, class W, int B, int D, core::Endianness E, bool F, bool G, bool H>
auto SubwordPixelFormat<C, W, B, D, E, F, G, H>::get_transfer_info() const noexcept -> image::Image::TransferInfo
{
    const image::ColorSpace& color_space = m_channel_spec.get_color_space();
    int bit_depth = bits_per_channel;
    return { transf_repr, &color_space, has_alpha_channel, bit_depth };
}


template<class C, class W, int B, int D, core::Endianness E, bool F, bool G, bool H>
void SubwordPixelFormat<C, W, B, D, E, F, G, H>::read(const word_type* buffer, image::Size image_size, image::Pos pos,
                                                      const image::Tray<transf_comp_type>& tray) noexcept
{
    ARCHON_ASSERT(image::Box(pos, tray.size).contained_in(image_size));
    for (int y = 0; y < tray.size.height; ++y) {
        image::Pos pos_2 = pos + image::Size(0, y);
        image::Pos pos_3 = pos_2 + image::Size(tray.size.width, 0);
        PixelPos begin = get_pixel_pos(image_size.width, pos_2);
        PixelPos end   = get_pixel_pos(image_size.width, pos_3);
        const word_type* source = buffer + begin.word_index;
        value_type word;
        int begin_pos = 0;
        int x = 0;
        if (ARCHON_LIKELY(end.word_index > begin.word_index)) {
            word = read_word(source);
            for (int p = begin.pixel_pos; p < pixels_per_word; ++p) {
                transf_comp_type* target = tray(x, y);
                get_pixel(word, p, target);
                ++x;
            }
            ++source;
            int num_words_between = int(end.word_index - begin.word_index - 1);
            for (int i = 0; i < num_words_between; ++i) {
                word = read_word(source);
                for (int p = 0; p < pixels_per_word; ++p) {
                    transf_comp_type* target = tray(x, y);
                    get_pixel(word, p, target);
                    ++x;
                }
                ++source;
            }
        }
        else {
            ARCHON_ASSERT(end.word_index == begin.word_index);
            begin_pos = begin.pixel_pos;
        }
        if (ARCHON_LIKELY(end.pixel_pos > begin_pos)) {
            word = read_word(source);
            for (int p = begin_pos; p < end.pixel_pos; ++p) {
                transf_comp_type* target = tray(x, y);
                get_pixel(word, p, target);
                ++x;
            }
        }
    }
}


template<class C, class W, int B, int D, core::Endianness E, bool F, bool G, bool H>
void SubwordPixelFormat<C, W, B, D, E, F, G, H>::write(word_type* buffer, image::Size image_size, image::Pos pos,
                                                       const image::Tray<const transf_comp_type>& tray) noexcept
{
    ARCHON_ASSERT(image::Box(pos, tray.size).contained_in(image_size));
    for (int y = 0; y < tray.size.height; ++y) {
        image::Pos pos_2 = pos + image::Size(0, y);
        image::Pos pos_3 = pos_2 + image::Size(tray.size.width, 0);
        PixelPos begin = get_pixel_pos(image_size.width, pos_2);
        PixelPos end   = get_pixel_pos(image_size.width, pos_3);
        word_type* target = buffer + begin.word_index;
        value_type word;
        int begin_pos = 0;
        int x = 0;
        if (ARCHON_LIKELY(end.word_index > begin.word_index)) {
            word = read_word(target);
            for (int p = begin.pixel_pos; p < pixels_per_word; ++p) {
                const transf_comp_type* source = tray(x, y);
                set_pixel(source, word, p);
                ++x;
            }
            write_word(word, target);
            ++target;
            int num_words_between = int(end.word_index - begin.word_index - 1);
            for (int i = 0; i < num_words_between; ++i) {
                word = 0;
                for (int p = 0; p < pixels_per_word; ++p) {
                    const transf_comp_type* source = tray(x, y);
                    set_pixel(source, word, p);
                    ++x;
                }
                write_word(word, target);
                ++target;
            }
        }
        else {
            ARCHON_ASSERT(end.word_index == begin.word_index);
            begin_pos = begin.pixel_pos;
        }
        if (ARCHON_LIKELY(end.pixel_pos > begin_pos)) {
            word = read_word(target);
            for (int p = begin_pos; p < end.pixel_pos; ++p) {
                const transf_comp_type* source = tray(x, y);
                set_pixel(source, word, p);
                ++x;
            }
            write_word(word, target);
        }
    }
}


template<class C, class W, int B, int D, core::Endianness E, bool F, bool G, bool H>
void SubwordPixelFormat<C, W, B, D, E, F, G, H>::fill(word_type* buffer, image::Size image_size,
                                                      const image::Box& area, const transf_comp_type* color) noexcept
{
    ARCHON_ASSERT(area.contained_in(image_size));
    value_type pixel = encode_pixel(color);
    for (int y = area.pos.y; y < area.pos.y + area.size.height; ++y) {
        image::Pos pos_1 = { area.pos.x, y };
        image::Pos pos_2 = pos_1 + image::Size(area.size.width, 0);
        PixelPos begin = get_pixel_pos(image_size.width, pos_1);
        PixelPos end   = get_pixel_pos(image_size.width, pos_2);
        word_type* target = buffer + begin.word_index;
        value_type word;
        int begin_pos = 0;
        if (ARCHON_LIKELY(end.word_index > begin.word_index)) {
            word = read_word(target);
            for (int p = begin.pixel_pos; p < pixels_per_word; ++p)
                do_set_pixel(pixel, word, p);
            write_word(word, target);
            ++target;
            int num_words_between = int(end.word_index - begin.word_index - 1);
            for (int i = 0; i < num_words_between; ++i) {
                word = 0;
                for (int p = 0; p < pixels_per_word; ++p)
                    do_set_pixel(pixel, word, p);
                write_word(word, target);
                ++target;
            }
        }
        else {
            ARCHON_ASSERT(end.word_index == begin.word_index);
            begin_pos = begin.pixel_pos;
        }
        if (ARCHON_LIKELY(end.pixel_pos > begin_pos)) {
            word = read_word(target);
            for (int p = begin_pos; p < end.pixel_pos; ++p)
                do_set_pixel(pixel, word, p);
            write_word(word, target);
        }
    }
}


template<class C, class W, int B, int D, core::Endianness E, bool F, bool G, bool H>
constexpr auto SubwordPixelFormat<C, W, B, D, E, F, G, H>::get_words_per_row(int image_width) -> std::size_t
{
    static_assert(word_aligned_rows);
    std::size_t n = 1;
    core::int_mul(n, image_width); // Throws
    n = core::int_div_round_up(n, pixels_per_word);
    return n;
}


template<class C, class W, int B, int D, core::Endianness E, bool F, bool G, bool H>
inline auto SubwordPixelFormat<C, W, B, D, E, F, G, H>::get_pixel_pos(int image_width, image::Pos pos) -> PixelPos
{
    std::ptrdiff_t word_index;
    int pixel_pos;
    if constexpr (word_aligned_rows) {
        int word_index_in_row = pos.x / pixels_per_word;
        pixel_pos = pos.x % pixels_per_word;
        int words_per_row = core::int_div_round_up(image_width, pixels_per_word);
        word_index = std::ptrdiff_t(pos.y * std::ptrdiff_t(words_per_row)) + word_index_in_row;
    }
    else {
        auto pixel_index = pos.y * std::ptrdiff_t(image_width) + pos.x;
        word_index = std::ptrdiff_t(pixel_index / pixels_per_word);
        pixel_pos = int(pixel_index % pixels_per_word);
    }
    return { word_index, pixel_pos };
}


template<class C, class W, int B, int D, core::Endianness E, bool F, bool G, bool H>
inline auto SubwordPixelFormat<C, W, B, D, E, F, G, H>::read_word(const word_type* source) noexcept -> value_type
{
    return image::unpack_int<bits_per_word>(*source);
}


template<class C, class W, int B, int D, core::Endianness E, bool F, bool G, bool H>
inline void SubwordPixelFormat<C, W, B, D, E, F, G, H>::write_word(value_type word, word_type* target) noexcept
{
    *target = image::pack_int<word_type, bits_per_word>(word);
}


template<class C, class W, int B, int D, core::Endianness E, bool F, bool G, bool H>
void SubwordPixelFormat<C, W, B, D, E, F, G, H>::get_pixel(value_type word, int pos, transf_comp_type* target) noexcept
{
    value_type pixel = ((word >> (map_pixel_pos(pos) * bits_per_pixel)) & core::int_mask<value_type>(bits_per_pixel));
    if constexpr (!std::is_floating_point_v<transf_comp_type> || !has_alpha_channel) {
        for (int i = 0; i < num_channels; ++i)
            target[i] = read_comp(pixel, i);
    }
    else {
        // Do premultiplied alpha
        int last = num_channels - 1;
        transf_comp_type alpha = read_comp(pixel, last);
        for (int i = 0; i < last; ++i)
            target[i] = alpha * read_comp(pixel, i);
        target[last] = alpha;
    }
}


template<class C, class W, int B, int D, core::Endianness E, bool F, bool G, bool H>
inline void SubwordPixelFormat<C, W, B, D, E, F, G, H>::set_pixel(const transf_comp_type* source, value_type& word,
                                                                  int pos) noexcept
{
    value_type pixel = encode_pixel(source);
    do_set_pixel(pixel, word, pos);
}


template<class C, class W, int B, int D, core::Endianness E, bool F, bool G, bool H>
auto SubwordPixelFormat<C, W, B, D, E, F, G, H>::encode_pixel(const transf_comp_type* source) noexcept -> value_type
{
    value_type pixel = 0;
    if constexpr (!std::is_floating_point_v<transf_comp_type> || !has_alpha_channel) {
        for (int i = 0; i < num_channels; ++i)
            write_comp(source[i], pixel, i);
    }
    else {
        // Undo premultiplied alpha
        int last = num_channels - 1;
        transf_comp_type alpha = source[last];
        transf_comp_type inv_alpha = (alpha != 0 ? 1 / alpha : 0);
        for (int i = 0; i < last; ++i)
            write_comp(inv_alpha * source[i], pixel, i);
        write_comp(alpha, pixel, last);
    }
    return pixel;
}


template<class C, class W, int B, int D, core::Endianness E, bool F, bool G, bool H>
inline auto SubwordPixelFormat<C, W, B, D, E, F, G, H>::read_comp(value_type pixel,
                                                                  int channel) noexcept -> transf_comp_type
{
    value_type comp = ((pixel >> (map_channel_pos(channel) * bits_per_channel)) &
                       core::int_mask<value_type>(bits_per_channel));
    if constexpr (std::is_integral_v<transf_comp_type>) {
        constexpr int n = image::comp_repr_bit_width<transf_repr>();
        return image::int_to_int<bits_per_channel, transf_comp_type, n>(comp);
    }
    else {
        static_assert(std::is_same_v<transf_comp_type, image::float_type>);
        bool is_alpha = (has_alpha_channel && channel == num_channels - 1);
        if (ARCHON_LIKELY(!is_alpha))
            return image::compressed_int_to_float<bits_per_channel>(comp);
        return image::int_to_float<bits_per_channel, transf_comp_type>(comp);
    }
}


template<class C, class W, int B, int D, core::Endianness E, bool F, bool G, bool H>
inline void SubwordPixelFormat<C, W, B, D, E, F, G, H>::write_comp(transf_comp_type comp, value_type& pixel,
                                                                   int channel) noexcept
{
    value_type comp_2;
    if constexpr (std::is_integral_v<transf_comp_type>) {
            constexpr int n = image::comp_repr_bit_width<transf_repr>();
            comp_2 = image::int_to_int<n, value_type, bits_per_channel>(comp);
        }
    else {
        static_assert(std::is_same_v<transf_comp_type, image::float_type>);
        bool is_alpha = (has_alpha_channel && channel == num_channels - 1);
        if (ARCHON_LIKELY(!is_alpha)) {
            comp_2 = image::float_to_compressed_int<value_type, bits_per_channel>(comp);
        }
        else {
            comp_2 = image::float_to_int<value_type, bits_per_channel>(comp);
        }
    }

    pixel |= (comp_2 << (map_channel_pos(channel) * bits_per_channel));
}


template<class C, class W, int B, int D, core::Endianness E, bool F, bool G, bool H>
void SubwordPixelFormat<C, W, B, D, E, F, G, H>::do_set_pixel(value_type pixel, value_type& word, int pos) noexcept
{
    int shift = map_pixel_pos(pos) * bits_per_pixel;
    word &= (core::int_mask<value_type>(bits_per_word) ^ (core::int_mask<value_type>(bits_per_pixel) << shift));
    word |= (pixel << shift);
}


template<class C, class W, int B, int D, core::Endianness E, bool F, bool G, bool H>
constexpr int SubwordPixelFormat<C, W, B, D, E, F, G, H>::map_channel_pos(int pos) noexcept
{
    // Map position from canonical order to reverse of actual order.
    constexpr int n = num_channels;
    ARCHON_ASSERT(pos >= 0 && pos < n);
    int pos_2 = pos;
    if constexpr (has_alpha_channel && alpha_channel_first)
        pos_2 = (pos_2 + 1) % n;
    if constexpr (!reverse_channel_order)
        pos_2 = (n - 1) - pos_2;
    return pos_2;
}


template<class C, class W, int B, int D, core::Endianness E, bool F, bool G, bool H>
constexpr int SubwordPixelFormat<C, W, B, D, E, F, G, H>::map_pixel_pos(int pos) noexcept
{
    int n = pixels_per_word;
    ARCHON_ASSERT(pos >= 0 && pos < n);
    switch (bit_order) {
        case core::Endianness::big:
            break;
        case core::Endianness::little:
            return pos;
    }
    return (n - 1) - pos;
}


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_SUBWORD_PIXEL_FORMAT_HPP
