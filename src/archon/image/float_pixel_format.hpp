// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2025 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_IMAGE_X_FLOAT_PIXEL_FORMAT_HPP
#define ARCHON_X_IMAGE_X_FLOAT_PIXEL_FORMAT_HPP

/// \file


#include <cstddef>
#include <type_traits>
#include <algorithm>
#include <utility>

#include <archon/core/features.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/image/geom.hpp>
#include <archon/image/tray.hpp>
#include <archon/image/comp_types.hpp>
#include <archon/image/comp_repr.hpp>
#include <archon/image/color_space.hpp>
#include <archon/image/standard_channel_spec.hpp>
#include <archon/image/transfer_info.hpp>
#include <archon/image/buffer_format.hpp>
#include <archon/image/image.hpp>


namespace archon::image {


/// \brief Specification of floating-point-based pixel format.
///
/// An instance of this class specifies a floating-point-based pixel format. Such a pixel
/// format conforms to \ref Concept_Archon_Image_PixelFormat and can therefore be used with
/// \ref image::BufferedImage.
///
/// Each channel component is stored using one word of the specified type (\p W). Channels
/// are stored consecutively in the specified order (\p F and \p G). No words are unused.
///
/// All color channels are stored in in linear form (not gamma compressed).
///
/// For integer-based pixel formats, see \ref image::IntegerPixelFormat, \ref
/// image::PackedPixelFormat, and \ref image::SubwordPixelFormat. For indirect color
/// formats, see \ref image::IndexedPixelFormat.
///
/// With this pixel format, the number of words in a pixel buffer must be divisible by the
/// number of words per pixel, which is the number of channels. Behavior is undefined if
/// this pixel format is used with a pixel buffer whose size is not equal to
/// `get_buffer_size(image_size)` where `image_size` is the image size passed to \ref
/// read(), \ref write(), or \ref fill(). See \ref Concept_Archon_Image_PixelFormat for
/// documentation of `get_buffer_size()`.
///
/// This class is an empty class if, and only if the given channel specification (\p C) is
/// an empty class.
///
/// \tparam C Channel specification. See \ref Concept_Archon_Image_ChannelSpec and \ref
/// image::StandardChannelSpec.
///
/// \tparam W Memory will be accessed in terms of words of this type. It must be one of the
/// standard floating-point types (`std::is_floating_point`).
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
/// \sa \ref image::BufferFormat::FloatFormat
///
template<class C, class W, bool F = false, bool G = false>
class FloatPixelFormat {
public:
    using channel_spec_type = C;
    using word_type         = W;

    static constexpr bool alpha_channel_first   = F;
    static constexpr bool reverse_channel_order = G;

    static constexpr bool has_alpha_channel = channel_spec_type::has_alpha_channel;
    static constexpr int num_channels = channel_spec_type::num_channels;

    static_assert(std::is_floating_point_v<word_type>);

    /// \brief Construct floating-point pixel format with given channel specification.
    ///
    /// This constructor constructs a floating-point pixel format with the given channel
    /// specification.
    ///
    FloatPixelFormat(channel_spec_type = {});

    /// \{
    ///
    /// \brief Required static pixel format members.
    ///
    /// See \ref Concept_Archon_Image_PixelFormat.
    ///
    static constexpr bool is_indexed_color = false;
    static constexpr image::CompRepr transf_repr = image::choose_float_transf_repr(image::bit_width<word_type>);
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
    static auto get_pixel_ptr(word_type* buffer, int image_width, image::Pos pos) -> word_type*;
    static auto get_pixel_ptr(const word_type* buffer, int image_width, image::Pos pos) -> const word_type*;

    static void read_pixel(const word_type* source, transf_comp_type* target) noexcept;
    static void write_pixel(const transf_comp_type* source, word_type* target) noexcept;

    static constexpr int map_channel_index(int) noexcept;

    ARCHON_NO_UNIQUE_ADDRESS channel_spec_type m_channel_spec;
};


template<class W> using FloatPixelFormat_Lum  = image::FloatPixelFormat<image::ChannelSpec_Lum, W>;
template<class W> using FloatPixelFormat_LumA = image::FloatPixelFormat<image::ChannelSpec_LumA, W>;
template<class W> using FloatPixelFormat_RGB  = image::FloatPixelFormat<image::ChannelSpec_RGB, W>;
template<class W> using FloatPixelFormat_RGBA = image::FloatPixelFormat<image::ChannelSpec_RGBA, W>;


using FloatPixelFormat_Lum_F  = image::FloatPixelFormat_Lum<image::float_type>;
using FloatPixelFormat_LumA_F = image::FloatPixelFormat_LumA<image::float_type>;
using FloatPixelFormat_RGB_F  = image::FloatPixelFormat_RGB<image::float_type>;
using FloatPixelFormat_RGBA_F = image::FloatPixelFormat_RGBA<image::float_type>;








// Implementation


template<class C, class W, bool F, bool G>
inline FloatPixelFormat<C, W, F, G>::FloatPixelFormat(channel_spec_type spec)
    : m_channel_spec(std::move(spec)) // Throws
{
}


template<class C, class W, bool F, bool G>
auto FloatPixelFormat<C, W, F, G>::get_buffer_size(image::Size image_size) -> std::size_t
{
    std::size_t size = get_words_per_row(image_size.width); // Throws
    core::int_mul(size, image_size.height); // Throws
    return size;
}


template<class C, class W, bool F, bool G>
bool FloatPixelFormat<C, W, F, G>::try_describe(image::BufferFormat& format) const
{
    const image::ColorSpace& color_space = m_channel_spec.get_color_space();
    image::BufferFormat::FloatType word_type_2 = {};
    if (ARCHON_LIKELY(image::BufferFormat::try_map_float_type<word_type>(word_type_2))) {
        format.set_float_format(word_type_2, color_space, has_alpha_channel,
                                alpha_channel_first, reverse_channel_order); // Throws
        return true;
    }
    return false;
}


template<class C, class W, bool F, bool G>
auto FloatPixelFormat<C, W, F, G>::get_transfer_info() const noexcept -> image::TransferInfo
{
    const image::ColorSpace& color_space = m_channel_spec.get_color_space();
    int bit_depth = image::bit_width<word_type>;
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


template<class C, class W, bool F, bool G>
void FloatPixelFormat<C, W, F, G>::read(const word_type* buffer, image::Size image_size, image::Pos pos,
                                        const image::Tray<transf_comp_type>& tray) noexcept
{
    ARCHON_ASSERT(image::Box(pos, tray.size).contained_in(image_size));
    for (int y = 0; y < tray.size.height; ++y) {
        image::Pos pos_2 = pos + image::Size(0, y);
        const word_type* source = get_pixel_ptr(buffer, image_size.width, pos_2);
        for (int x = 0; x < tray.size.width; ++x) {
            transf_comp_type* target = tray(x, y);
            read_pixel(source, target);
            source += num_channels;
        }
    }
}


template<class C, class W, bool F, bool G>
void FloatPixelFormat<C, W, F, G>::write(word_type* buffer, image::Size image_size, image::Pos pos,
                                         const image::Tray<const transf_comp_type>& tray) noexcept
{
    ARCHON_ASSERT(image::Box(pos, tray.size).contained_in(image_size));
    for (int y = 0; y < tray.size.height; ++y) {
        image::Pos pos_2 = pos + image::Size(0, y);
        word_type* target = get_pixel_ptr(buffer, image_size.width, pos_2);
        for (int x = 0; x < tray.size.width; ++x) {
            const transf_comp_type* source = tray(x, y);
            write_pixel(source, target);
            target += num_channels;
        }
    }
}


template<class C, class W, bool F, bool G>
void FloatPixelFormat<C, W, F, G>::fill(word_type* buffer, image::Size image_size, const image::Box& area,
                                        const transf_comp_type* color) noexcept
{
    ARCHON_ASSERT(area.contained_in(image_size));
    word_type color_2[num_channels];
    write_pixel(color, color_2);
    image::Pos begin = area.pos;
    image::Pos end = begin + area.size;
    for (int y = begin.y; y < end.y; ++y) {
        word_type* target = get_pixel_ptr(buffer, image_size.width, { begin.x, y });
        for (int x = begin.x; x < end.x; ++x) {
            std::copy(color_2, color_2 + num_channels, target);
            target += num_channels;
        }
    }
}


template<class C, class W, bool F, bool G>
constexpr auto FloatPixelFormat<C, W, F, G>::get_words_per_row(int image_width) -> std::size_t
{
    std::size_t n = 1;
    core::int_mul(n, num_channels); // Throws
    core::int_mul(n, image_width); // Throws
    return n;
}


template<class C, class W, bool F, bool G>
inline auto FloatPixelFormat<C, W, F, G>::get_pixel_ptr(word_type* buffer, int image_width,
                                                          image::Pos pos) -> word_type*
{
    auto pixel_index = pos.y * std::ptrdiff_t(image_width) + pos.x;
    return buffer + pixel_index * num_channels;
}


template<class C, class W, bool F, bool G>
inline auto FloatPixelFormat<C, W, F, G>::get_pixel_ptr(const word_type* buffer, int image_width,
                                                          image::Pos pos) -> const word_type*
{
    auto pixel_index = pos.y * std::ptrdiff_t(image_width) + pos.x;
    return buffer + pixel_index * num_channels;
}


template<class C, class W, bool F, bool G>
inline void FloatPixelFormat<C, W, F, G>::read_pixel(const word_type* source, transf_comp_type* target) noexcept
{
    for (int i = 0; i < num_channels; ++i) {
        int j = map_channel_index(i);
        target[i] = transf_comp_type(source[j]);
    }
}


template<class C, class W, bool F, bool G>
inline void FloatPixelFormat<C, W, F, G>::write_pixel(const transf_comp_type* source, word_type* target) noexcept
{
    for (int i = 0; i < num_channels; ++i) {
        int j = map_channel_index(i);
        target[j] = word_type(source[i]);
    }
}


template<class C, class W, bool F, bool G>
constexpr int FloatPixelFormat<C, W, F, G>::map_channel_index(int i) noexcept
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


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_FLOAT_PIXEL_FORMAT_HPP
