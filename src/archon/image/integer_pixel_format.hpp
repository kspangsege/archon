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

#ifndef ARCHON_X_IMAGE_X_INTEGER_PIXEL_FORMAT_HPP
#define ARCHON_X_IMAGE_X_INTEGER_PIXEL_FORMAT_HPP

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
#include <archon/image/transfer_info.hpp>
#include <archon/image/buffer_format.hpp>


namespace archon::image {


/// \brief Specification of simple integer-based pixel format.
///
/// An instantiation of this template is used to specify an integer-based pixel format. Such
/// a pixel format implements \ref Concept_Archon_Image_PixelFormat and can therefore be
/// used with \ref image::BufferedImage.
///
/// Each channel component is stored using \p D consecutive words. Channels are stored
/// consecutively in the specified order (\p F and \p G). No words are unused.
///
/// All color channels are stored in in gamma compressed form according to the gamma
/// compression scheme of sRGB. The alpha channel, on the other hand, is stored linearly.
///
/// For formats that pack multiple channels into each bit compound, see \ref
/// image::PackedPixelFormat.
///
/// For formats that pack multiple pixels into each bit compound, see \ref
/// image::SubwordPixelFormat.
///
/// For floating-point based formats, see \ref image::FloatPixelFormat.         
///
/// For indirect color formats, see \ref image::IndexedPixelFormat.
///
/// If the word type (\p W) has more bits than are used (\p B), the unused bits must be
/// zero. Behavior is undefined if this pixel format is used with a pixel buffer where these
/// bits are not zero. Conversely, his pixel format guarantees that these bits will remain
/// zero.
///
/// With this pixel format, the number of words in a pixel buffer must be divisible by the
/// number of words per pixel (number of channels times number of words per
/// channel). Behavior is undefined if this pixel format is used with a pixel buffer whose
/// size is not equal to `get_buffer_size(image_size)` where `image_size` is the image size
/// passed to \ref read(), \ref write(), or \ref fill(). See \ref
/// Concept_Archon_Image_PixelFormat for documentation of `get_buffer_size()`.
///
/// This class is an empty class if, and only if the given channel specification (\p C) is
/// an empty class.
///
/// \tparam C Channel specification. See \ref Concept_Archon_Image_ChannelSpec and \ref
/// image::StandardChannelSpec.
///
/// \tparam W Memory will be accessed in terms of words of this type.
///
/// \tparam B Number of used bits per word. When this is less than the number of bits in \p
/// W, the used bits will be the \p B least significant ones.
///
/// \tparam S A type that is wide enough to be able to hold all the bits of a single channel
/// component. It's width (number of value bits, plus one if signed) must be greater than,
/// or equal to the specified number of words per channel (\p D) times the specified number
/// of bits per word (\p B). The type that will actually be used, is the one resulting from
/// integral promotion of a value of the specified type, or possibly the promotion of a
/// value of the unsigned version of the specified type.
///
/// \tparam D Number of words per channel component. I.e., every chunk of this number of
/// words make up a single channel component. The relation between the order of these words
/// and the significance of their bits in the assembled component is determined by the
/// specified word order (\p E).
///
/// \tparam E The order in which words are assembled into component values. If set to 'big
/// endian', words at lower memory address will make up bits of higher significance in the
/// component value. In any case, when the component type (\p S) has more bits than needed,
/// say N unused bits, then it is always the N bits of highest significance that are unused.
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
/// \sa \ref image::BufferFormat::IntegerFormat
///
template<class C, class W, int B, class S = W, int D = 1,
         core::Endianness E = core::Endianness::big, bool F = false, bool G = false>
class IntegerPixelFormat {
public:
    using channel_spec_type = C;
    using word_type         = W;
    using comp_type         = S;

    static constexpr int bits_per_word           = B;
    static constexpr int words_per_channel       = D;
    static constexpr core::Endianness word_order = E;
    static constexpr bool alpha_channel_first    = F;
    static constexpr bool reverse_channel_order  = G;

    static constexpr bool has_alpha_channel = channel_spec_type::has_alpha_channel;
    static constexpr int num_channels = channel_spec_type::num_channels;
    static constexpr int bit_depth = words_per_channel * bits_per_word;
    static constexpr int words_per_pixel = num_channels * words_per_channel;

    static_assert(std::is_integral_v<word_type>);
    static_assert(std::is_integral_v<comp_type>);
    static_assert(bits_per_word > 0);
    static_assert(bits_per_word <= image::bit_width<word_type>);
    static_assert(words_per_channel > 0);
    static_assert(words_per_channel <= core::int_max<int>() / bits_per_word);
    static_assert(bit_depth <= image::bit_width<comp_type>);
    static_assert(words_per_channel <= core::int_max<int>() / num_channels);

    /// \brief Construct integer pixel format with given channel specification.
    ///
    /// This constructor constructs an integer pixel format with the given channel
    /// specification.
    ///
    IntegerPixelFormat(channel_spec_type = {});

    /// \{
    ///
    /// \brief Required static pixel format members.
    ///
    /// See \ref Concept_Archon_Image_PixelFormat.
    ///
    static constexpr bool is_indexed_color = false;
    static constexpr image::CompRepr transf_repr = image::choose_transf_repr(bit_depth);
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
    auto get_transfer_info() const noexcept -> image::TransferInfo;
    static void read(const word_type* buffer, image::Size image_size, image::Pos,
                     const image::Tray<transf_comp_type>&) noexcept;
    static void write(word_type* buffer, image::Size image_size, image::Pos,
                      const image::Tray<const transf_comp_type>&) noexcept;
    static void fill(word_type* buffer, image::Size image_size, const image::Box& area,
                     const transf_comp_type* color) noexcept;
    /// \}

    /// \brief Number of words per row of pixels.
    ///
    /// This function returns the number of words (elements of type \p word_type) that make
    /// up each row of an image of the specified width.
    ///
    static constexpr auto get_words_per_row(int image_width) -> std::size_t;

private:
    using unpacked_comp_type = image::unpacked_type<comp_type, bit_depth>;

    static auto get_pixel_ptr(word_type* buffer, int image_width, image::Pos pos) -> word_type*;
    static auto get_pixel_ptr(const word_type* buffer, int image_width, image::Pos pos) -> const word_type*;

    static void read_pixel(const word_type* source, transf_comp_type* target) noexcept;
    static void write_pixel(const transf_comp_type* source, word_type* target) noexcept;

    static auto read_comp(const word_type* pixel_ptr, int channel) noexcept -> transf_comp_type;
    static void write_comp(transf_comp_type, word_type* pixel_ptr, int channel) noexcept;

    static constexpr bool is_transf_repr_match() noexcept;

    static constexpr int map_channel_index(int) noexcept;
    static constexpr int map_word_index(int) noexcept;

    ARCHON_NO_UNIQUE_ADDRESS channel_spec_type m_channel_spec;
};


template<class W = image::int8_type, int B = image::bit_width<W>, class S = W, int D = 1,
         core::Endianness E = core::Endianness::big>
using IntegerPixelFormat_Lum = image::IntegerPixelFormat<image::ChannelSpec_Lum, W, B, S, D, E>;

template<class W = image::int8_type, int B = image::bit_width<W>, class S = W, int D = 1,
         core::Endianness E = core::Endianness::big>
using IntegerPixelFormat_LumA = image::IntegerPixelFormat<image::ChannelSpec_LumA, W, B, S, D, E>;

template<class W = image::int8_type, int B = image::bit_width<W>, class S = W, int D = 1,
         core::Endianness E = core::Endianness::big>
using IntegerPixelFormat_RGB = image::IntegerPixelFormat<image::ChannelSpec_RGB, W, B, S, D, E>;

template<class W = image::int8_type, int B = image::bit_width<W>, class S = W, int D = 1,
         core::Endianness E = core::Endianness::big>
using IntegerPixelFormat_RGBA = image::IntegerPixelFormat<image::ChannelSpec_RGBA, W, B, S, D, E>;


using IntegerPixelFormat_Lum_8  = image::IntegerPixelFormat_Lum<image::int8_type, 8>;
using IntegerPixelFormat_LumA_8 = image::IntegerPixelFormat_LumA<image::int8_type, 8>;
using IntegerPixelFormat_RGB_8  = image::IntegerPixelFormat_RGB<image::int8_type, 8>;
using IntegerPixelFormat_RGBA_8 = image::IntegerPixelFormat_RGBA<image::int8_type, 8>;

using IntegerPixelFormat_Lum_16  = image::IntegerPixelFormat_Lum<image::int16_type, 16>;
using IntegerPixelFormat_LumA_16 = image::IntegerPixelFormat_LumA<image::int16_type, 16>;
using IntegerPixelFormat_RGB_16  = image::IntegerPixelFormat_RGB<image::int16_type, 16>;
using IntegerPixelFormat_RGBA_16 = image::IntegerPixelFormat_RGBA<image::int16_type, 16>;

using IntegerPixelFormat_Lum_32  = image::IntegerPixelFormat_Lum<image::int32_type, 32>;
using IntegerPixelFormat_LumA_32 = image::IntegerPixelFormat_LumA<image::int32_type, 32>;
using IntegerPixelFormat_RGB_32  = image::IntegerPixelFormat_RGB<image::int32_type, 32>;
using IntegerPixelFormat_RGBA_32 = image::IntegerPixelFormat_RGBA<image::int32_type, 32>;








// Implementation


template<class C, class W, int B, class S, int D, core::Endianness E, bool F, bool G>
inline IntegerPixelFormat<C, W, B, S, D, E, F, G>::IntegerPixelFormat(channel_spec_type spec)
    : m_channel_spec(std::move(spec)) // Throws
{
}


template<class C, class W, int B, class S, int D, core::Endianness E, bool F, bool G>
auto IntegerPixelFormat<C, W, B, S, D, E, F, G>::get_buffer_size(image::Size image_size) -> std::size_t
{
    std::size_t size = get_words_per_row(image_size.width); // Throws
    core::int_mul(size, image_size.height); // Throws
    return size;
}


template<class C, class W, int B, class S, int D, core::Endianness E, bool F, bool G>
bool IntegerPixelFormat<C, W, B, S, D, E, F, G>::try_describe(image::BufferFormat& format) const
{
    const image::ColorSpace& color_space = m_channel_spec.get_color_space();
    image::BufferFormat::IntegerType word_type_2 = {};
    if (ARCHON_LIKELY(image::BufferFormat::try_map_integer_type<word_type>(word_type_2))) {
        format.set_integer_format(word_type_2, bits_per_word, words_per_channel, word_order, color_space,
                                  has_alpha_channel, alpha_channel_first, reverse_channel_order); // Throws
        return true;
    }
    return false;
}


template<class C, class W, int B, class S, int D, core::Endianness E, bool F, bool G>
auto IntegerPixelFormat<C, W, B, S, D, E, F, G>::get_transfer_info() const noexcept -> image::TransferInfo
{
    const image::ColorSpace& color_space = m_channel_spec.get_color_space();
    const image::Image* palette = nullptr;
    int index_depth = 0;
    return {
        &color_space,
        has_alpha_channel,
        transf_repr,
        bit_depth,
        palette,
        index_depth,
    };
}


template<class C, class W, int B, class S, int D, core::Endianness E, bool F, bool G>
void IntegerPixelFormat<C, W, B, S, D, E, F, G>::read(const word_type* buffer, image::Size image_size, image::Pos pos,
                                                      const image::Tray<transf_comp_type>& tray) noexcept
{
    ARCHON_ASSERT(image::Box(pos, tray.size).contained_in(image_size));
    for (int y = 0; y < tray.size.height; ++y) {
        image::Pos pos_2 = pos + image::Size(0, y);
        const word_type* source = get_pixel_ptr(buffer, image_size.width, pos_2);
        for (int x = 0; x < tray.size.width; ++x) {
            transf_comp_type* target = tray(x, y);
            read_pixel(source, target);
            source += words_per_pixel;
        }
    }
}


template<class C, class W, int B, class S, int D, core::Endianness E, bool F, bool G>
void IntegerPixelFormat<C, W, B, S, D, E, F, G>::write(word_type* buffer, image::Size image_size, image::Pos pos,
                                                       const image::Tray<const transf_comp_type>& tray) noexcept
{
    ARCHON_ASSERT(image::Box(pos, tray.size).contained_in(image_size));
    for (int y = 0; y < tray.size.height; ++y) {
        image::Pos pos_2 = pos + image::Size(0, y);
        word_type* target = get_pixel_ptr(buffer, image_size.width, pos_2);
        for (int x = 0; x < tray.size.width; ++x) {
            const transf_comp_type* source = tray(x, y);
            write_pixel(source, target);
            target += words_per_pixel;
        }
    }
}


template<class C, class W, int B, class S, int D, core::Endianness E, bool F, bool G>
void IntegerPixelFormat<C, W, B, S, D, E, F, G>::fill(word_type* buffer, image::Size image_size,
                                                      const image::Box& area, const transf_comp_type* color) noexcept
{
    ARCHON_ASSERT(area.contained_in(image_size));
    word_type color_2[words_per_pixel];
    write_pixel(color, color_2);
    image::Pos begin = area.pos;
    image::Pos end = begin + area.size;
    for (int y = begin.y; y < end.y; ++y) {
        word_type* target = get_pixel_ptr(buffer, image_size.width, { begin.x, y });
        for (int x = begin.x; x < end.x; ++x) {
            std::copy_n(color_2, words_per_pixel, target);
            target += words_per_pixel;
        }
    }
}


template<class C, class W, int B, class S, int D, core::Endianness E, bool F, bool G>
constexpr auto IntegerPixelFormat<C, W, B, S, D, E, F, G>::get_words_per_row(int image_width) -> std::size_t
{
    std::size_t n = 1;
    core::int_mul(n, words_per_pixel); // Throws
    core::int_mul(n, image_width); // Throws
    return n;
}


template<class C, class W, int B, class S, int D, core::Endianness E, bool F, bool G>
inline auto IntegerPixelFormat<C, W, B, S, D, E, F, G>::get_pixel_ptr(word_type* buffer, int image_width,
                                                                      image::Pos pos) -> word_type*
{
    auto pixel_index = pos.y * std::ptrdiff_t(image_width) + pos.x;
    return buffer + pixel_index * words_per_pixel;
}


template<class C, class W, int B, class S, int D, core::Endianness E, bool F, bool G>
inline auto IntegerPixelFormat<C, W, B, S, D, E, F, G>::get_pixel_ptr(const word_type* buffer, int image_width,
                                                                      image::Pos pos) -> const word_type*
{
    auto pixel_index = pos.y * std::ptrdiff_t(image_width) + pos.x;
    return buffer + pixel_index * words_per_pixel;
}


template<class C, class W, int B, class S, int D, core::Endianness E, bool F, bool G>
void IntegerPixelFormat<C, W, B, S, D, E, F, G>::read_pixel(const word_type* source, transf_comp_type* target) noexcept
{
    if constexpr (!std::is_floating_point_v<transf_comp_type> || !has_alpha_channel) {
        for (int i = 0; i < num_channels; ++i)
            target[i] = read_comp(source, i);
    }
    else {
        // Do premultiplied alpha
        int last = num_channels - 1;
        transf_comp_type alpha = read_comp(source, last);
        for (int i = 0; i < last; ++i)
            target[i] = alpha * read_comp(source, i);
        target[last] = alpha;
    }
}


template<class C, class W, int B, class S, int D, core::Endianness E, bool F, bool G>
void IntegerPixelFormat<C, W, B, S, D, E, F, G>::write_pixel(const transf_comp_type* source,
                                                             word_type* target) noexcept
{
    if constexpr (!std::is_floating_point_v<transf_comp_type> || !has_alpha_channel) {
        for (int i = 0; i < num_channels; ++i)
            write_comp(source[i], target, i);
    }
    else {
        // Undo premultiplied alpha
        int last = num_channels - 1;
        transf_comp_type alpha = source[last];
        transf_comp_type inv_alpha = (alpha != 0 ? 1 / alpha : 0);
        for (int i = 0; i < last; ++i)
            write_comp(inv_alpha * source[i], target, i);
        write_comp(alpha, target, last);
    }
}


template<class C, class W, int B, class S, int D, core::Endianness E, bool F, bool G>
auto IntegerPixelFormat<C, W, B, S, D, E, F, G>::read_comp(const word_type* pixel_ptr,
                                                           int channel) noexcept -> transf_comp_type
{
    const word_type* channel_ptr = pixel_ptr + map_channel_index(channel) * words_per_channel;
    if constexpr (is_transf_repr_match()) {
        return *channel_ptr;
    }
    else {
        unpacked_comp_type comp = 0;
        for (int i = 0; i < words_per_channel; ++i) {
            int shift = map_word_index(i) * bits_per_word;
            comp |= unpacked_comp_type(image::unpack_int<bits_per_word>(channel_ptr[i])) << shift;
        }
        if constexpr (std::is_integral_v<transf_comp_type>) {
            constexpr int n = image::comp_repr_bit_width<transf_repr>();
            return image::int_to_int<bit_depth, transf_comp_type, n>(comp);
        }
        else {
            static_assert(std::is_same_v<transf_comp_type, image::float_type>);
            bool is_alpha = (has_alpha_channel && channel == num_channels - 1);
            if (ARCHON_LIKELY(!is_alpha))
                return image::compressed_int_to_float<bit_depth>(comp);
            return image::int_to_float<bit_depth, transf_comp_type>(comp);
        }
    }
}


template<class C, class W, int B, class S, int D, core::Endianness E, bool F, bool G>
void IntegerPixelFormat<C, W, B, S, D, E, F, G>::write_comp(transf_comp_type val, word_type* pixel_ptr,
                                                            int channel) noexcept
{
    word_type* channel_ptr = pixel_ptr + map_channel_index(channel) * words_per_channel;
    if constexpr (is_transf_repr_match()) {
        *channel_ptr = val;
    }
    else {
        unpacked_comp_type comp;
        if constexpr (std::is_integral_v<transf_comp_type>) {
            constexpr int n = image::comp_repr_bit_width<transf_repr>();
            comp = image::int_to_int<n, unpacked_comp_type, bit_depth>(val);
        }
        else {
            static_assert(std::is_same_v<transf_comp_type, image::float_type>);
            bool is_alpha = (has_alpha_channel && channel == num_channels - 1);
            if (ARCHON_LIKELY(!is_alpha)) {
                comp = image::float_to_compressed_int<unpacked_comp_type, bit_depth>(val);
            }
            else {
                comp = image::float_to_int<unpacked_comp_type, bit_depth>(val);
            }
        }
        for (int i = 0; i < words_per_channel; ++i) {
            int shift = map_word_index(i) * bits_per_word;
            unpacked_comp_type value = (comp >> shift) & core::int_mask<unpacked_comp_type>(bits_per_word);
            channel_ptr[i] = image::pack_int<word_type, bits_per_word>(value);
        }
    }
}


template<class C, class W, int B, class S, int D, core::Endianness E, bool F, bool G>
constexpr bool IntegerPixelFormat<C, W, B, S, D, E, F, G>::is_transf_repr_match() noexcept
{
    return (words_per_channel == 1 && std::is_same_v<transf_comp_type, word_type> &&
            image::comp_repr_int_bit_width(transf_repr) == bit_depth);
}


template<class C, class W, int B, class S, int D, core::Endianness E, bool F, bool G>
constexpr int IntegerPixelFormat<C, W, B, S, D, E, F, G>::map_channel_index(int i) noexcept
{
    // Map index from canonical order to actual order.
    int n = num_channels;
    ARCHON_ASSERT(i >= 0 && i < n);
    int j = i;
    if constexpr (has_alpha_channel && alpha_channel_first)
        j = (j + 1) % n;
    if constexpr (reverse_channel_order)
        j = (n - 1) - j;
    return j;
}


template<class C, class W, int B, class S, int D, core::Endianness E, bool F, bool G>
constexpr int IntegerPixelFormat<C, W, B, S, D, E, F, G>::map_word_index(int i) noexcept
{
    // Map index from little-endian order to actual order.
    int n = words_per_channel;
    ARCHON_ASSERT(i >= 0 && i < n);
    switch (word_order) {
        case core::Endianness::big:
            break;
        case core::Endianness::little:
            return i;
    }
    return (n - 1) - i;
}


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_INTEGER_PIXEL_FORMAT_HPP
