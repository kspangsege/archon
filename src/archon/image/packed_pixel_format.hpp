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

#ifndef ARCHON_X_IMAGE_X_PACKED_PIXEL_FORMAT_HPP
#define ARCHON_X_IMAGE_X_PACKED_PIXEL_FORMAT_HPP

/// \file


#include <cstddef>
#include <type_traits>
#include <iterator>
#include <utility>
#include <algorithm>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/utility.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/endianness.hpp>
#include <archon/image/geom.hpp>
#include <archon/image/tray.hpp>
#include <archon/image/comp_types.hpp>
#include <archon/image/gamma.hpp>
#include <archon/image/comp_repr.hpp>
#include <archon/image/color_space.hpp>
#include <archon/image/standard_channel_spec.hpp>
#include <archon/image/bit_field.hpp>
#include <archon/image/buffer_format.hpp>
#include <archon/image/image.hpp>


namespace archon::image {


/// \brief Compile-time specification of packed pixel format.
///
/// An instantiation of this template is a compile-time specification of pixel format that
/// divides memory into a sequence of bit compounds, each made from a fixed number of
/// consecutive words, and uses a particular layout of the channels of each pixel within
/// these bit compounds. such a pixel format stores exactly one pixel per bit compound. The
/// way in which the words are assembled into bit compounds is customizable (see \p B, \p D,
/// and \p E).
///
/// For formats that store one channel in each bit compound, see \ref
/// image::InterPixelFormat.
///
/// For formats that pack multiple pixels into each bit compound, see \ref
/// image::SubwordPixelFormat.
///
/// For floating-point based formats, see \ref image::FloatingPixelFormat.         
///
/// For indirect color formats, see \ref image::IndexedPixelFormat.
///
/// Unused bits must be zero. This includes unused bits in words (at positions of
/// significance higher than \p B) and unused bits in bit compounds (bits not covered by any
/// bit field). Behavior is undefined if this pixel format is used with a pixel buffer where
/// these bits are not zero. Conversely, his pixel format guarantees that these bits will
/// remain zero.
///
/// Any pixel buffer used with this pixel format must contain a whole number of bit
/// compounds. This means that the number of words in the buffer must be divisible by the
/// number of words per bit compound (\p D). Behavior is undefined if this pixel format is
/// used with a pixel buffer whose size is not equal to `get_buffer_size(image_size)` where
/// `image_size` is the image size passed to \ref read(), \ref write(), or \ref fill(). See
/// \ref Concept_Archon_Image_PixelFormat for documentation of `get_buffer_size()`.
///
/// This class is an empty class if, and only if the given channel specification (\p C) is
/// an empty class.
///
/// \tparam C Channel specification. See \ref Concept_Archon_Image_ChannelSpec and \ref
/// image::StandardChannelSpec.
///
/// \tparam S A type that is wide enough to be able to hold all the bits of a bit
/// compound. The number of available bits (see \ref image::bit_width) must be large enough
/// to cover all occupied bit positions in the packing specification (\p P). It must also be
/// greater than, or equal to the specified number of words per channel (\p D) times the
/// specified number of bits per word (\p B). The type that will actually be used, is the
/// one resulting from integral promotion of a value of the specified type, or possibly the
/// promotion of a value of the unsigned version of the specified type.
///
/// \tparam P Packing specification. See \ref Concept_Archon_Image_ChannelPacking, \ref
/// image::TriChannelPacking, and \ref image::QuadChannelPacking. The packing specification
/// must agree with the channel specification on number of channels.
///
/// \tparam W Memory will be accessed in terms of words of this type.
///
/// \tparam B Number of used bits per word. This must be less than, or equal to
/// `image::bit_width<W>` (\ref image::bit_width). When it is less than the number of
/// available bits in \p W, the used bits will be the \p B least significant ones.
///
/// \tparam D Number of words per bit compound. I.e., every chunk of this number of words
/// make up a single bit compound. The relation between the order of these words and the
/// significance of their bits in the assembled bit compound is determined by the specified
/// word order (\p E).
///
/// \tparam E The order in which words are assembled into bit compounds. If set to 'big
/// endian', words at lower memory address will make up bits of higher significance in the
/// bit compound. In any case, when the compound type (\p S) has more bits than needed, say
/// N unused bits, then it is always the N bits of highest significance that are unused.
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
template<class C, class S, class P, class W = S, int B = image::bit_width<W>, int D = 1,
         core::Endianness E = core::Endianness::big, bool F = false, bool G = false>
class PackedPixelFormat
    : private core::HiddenBase<C> {
public:
    using channel_spec_type    = C;
    using compound_type        = S;
    using channel_packing_type = P;
    using word_type            = W;

    static constexpr int bits_per_word           = B;
    static constexpr int words_per_pixel         = D;
    static constexpr core::Endianness word_order = E;
    static constexpr bool alpha_channel_first    = F;
    static constexpr bool reverse_channel_order  = G;

    static constexpr bool has_alpha_channel = channel_spec_type::has_alpha_channel;
    static constexpr int num_channels = channel_spec_type::num_channels;
    static constexpr int bits_per_pixel = words_per_pixel * bits_per_word;
    static constexpr int bit_depth = image::widest_bit_field(channel_packing_type::fields,
                                                             channel_packing_type::num_fields);

    static_assert(std::is_integral_v<word_type>);
    static_assert(std::is_integral_v<compound_type>);
    static_assert(bits_per_word > 0);
    static_assert(bits_per_word <= image::bit_width<word_type>);
    static_assert(words_per_pixel > 0);
    static_assert(words_per_pixel <= core::int_max<int>() / bits_per_word);
    static_assert(bits_per_pixel <= image::bit_width<compound_type>);
    static_assert(channel_packing_type::num_fields == num_channels);
    static_assert(std::size(channel_packing_type::fields) == num_channels);
    static_assert(image::valid_bit_fields(channel_packing_type::fields, num_channels, bits_per_pixel));

    /// \brief Construct packed pixel format with given channel specification.
    ///
    /// This constructor constructs a packed pixel format with the given channel
    /// specification.
    ///
    PackedPixelFormat(channel_spec_type = {});

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
    auto get_transfer_info() const noexcept -> image::Image::TransferInfo;
    static void read(const word_type* buffer, image::Size image_size, image::Pos,
                     const image::Tray<transf_comp_type>&);
    static void write(word_type* buffer, image::Size image_size, image::Pos,
                      const image::Tray<const transf_comp_type>&);
    static void fill(word_type* buffer, image::Size image_size, const image::Box& area,
                     const transf_comp_type* color);
    /// \}

    static constexpr int get_pack_width();
    static_assert(get_pack_width() <= bits_per_pixel);

    /// \brief Number of words per row of pixels.
    ///
    /// This function returns the number of words (elements of type \p word_type) that make
    /// up each row of an image of the specified width.
    ///
    static constexpr auto get_words_per_row(int image_width) -> std::size_t;

    /// \{
    ///
    /// \brief Width an position of bit field of specified channel.
    ///
    /// These functions return the bit width and shift of the specified channel. The channel
    /// is specified in terms of its index within the canonical channel order. The shift is
    /// the number of bit positions that the channel is shifted to the left as it resides in
    /// the bit compound.
    ///
    static constexpr int get_channel_width(int channel_index) noexcept;
    static constexpr int get_channel_shift(int channel_index) noexcept;
    /// \}

private:
    // Type that is "prepromoted" and also avoids undefined behaviour when bits are shifted
    // into and out of the highest relevant bit position.
    using value_type = image::unpacked_type<compound_type, bits_per_pixel>;

    static auto get_pixel_ptr(word_type* buffer, int image_width, image::Pos pos) -> word_type*;
    static auto get_pixel_ptr(const word_type* buffer, int image_width, image::Pos pos) -> const word_type*;

    static void read_pixel(const word_type* source, transf_comp_type* target);
    static void write_pixel(const transf_comp_type* source, word_type* target);

    template<int I> static auto get_component(value_type pixel) -> transf_comp_type;
    template<int I> static void set_component(value_type& pixel, transf_comp_type value);

    static constexpr int map_channel_index(int) noexcept;
    static constexpr int map_word_index(int) noexcept;

    auto channel_spec() const noexcept -> const channel_spec_type&;
};


template<class S, class P, class W = S, int B = image::bit_width<W>, int D = 1,
         core::Endianness E = core::Endianness::big>
using PackedPixelFormat_Lum = image::PackedPixelFormat<image::ChannelSpec_Lum, S, P, W, B, D, E>;

template<class S, class P, class W = S, int B = image::bit_width<W>, int D = 1,
         core::Endianness E = core::Endianness::big>
using PackedPixelFormat_LumA = image::PackedPixelFormat<image::ChannelSpec_LumA, S, P, W, B, D, E>;

template<class S, class P, class W = S, int B = image::bit_width<W>, int D = 1,
         core::Endianness E = core::Endianness::big>
using PackedPixelFormat_RGB = image::PackedPixelFormat<image::ChannelSpec_RGB, S, P, W, B, D, E>;

template<class S, class P, class W = S, int B = image::bit_width<W>, int D = 1,
         core::Endianness E = core::Endianness::big>
using PackedPixelFormat_RGBA = image::PackedPixelFormat<image::ChannelSpec_RGBA, S, P, W, B, D, E>;








// Implementation


template<class C, class S, class P, class W, int B, int D, core::Endianness E, bool F, bool G>
inline PackedPixelFormat<C, S, P, W, B, D, E, F, G>::PackedPixelFormat(channel_spec_type spec)
    : core::HiddenBase<C>(std::move(spec)) // Throws
{
}


template<class C, class S, class P, class W, int B, int D, core::Endianness E, bool F, bool G>
inline auto PackedPixelFormat<C, S, P, W, B, D, E, F, G>::get_buffer_size(image::Size image_size) -> std::size_t
{
    std::size_t size = get_words_per_row(image_size.width); // Throws
    core::int_mul(size, image_size.height); // Throws
    return size;
}


template<class C, class S, class P, class W, int B, int D, core::Endianness E, bool F, bool G>
bool PackedPixelFormat<C, S, P, W, B, D, E, F, G>::try_describe(image::BufferFormat& format) const
{
    if (ARCHON_LIKELY(num_channels <= image::BufferFormat::max_bit_fields)) {
        const image::ColorSpace& color_space = channel_spec().get_color_space();
        image::BufferFormat::IntegerType word_type_2 = {};
        if (ARCHON_LIKELY(image::BufferFormat::try_map_integer_type<word_type>(word_type_2))) {
            format.set_packed_format(word_type_2, bits_per_word, words_per_pixel, word_order,
                                     channel_packing_type::fields, color_space, has_alpha_channel,
                                     alpha_channel_first, reverse_channel_order); // Throws
            return true;
        }
    }
    return false;
}


template<class C, class S, class P, class W, int B, int D, core::Endianness E, bool F, bool G>
auto PackedPixelFormat<C, S, P, W, B, D, E, F, G>::get_transfer_info() const noexcept -> image::Image::TransferInfo
{
    const image::ColorSpace& color_space = channel_spec().get_color_space();
    return { transf_repr, &color_space, has_alpha_channel, bit_depth };
}


template<class C, class S, class P, class W, int B, int D, core::Endianness E, bool F, bool G>
void PackedPixelFormat<C, S, P, W, B, D, E, F, G>::read(const word_type* buffer, image::Size image_size,
                                                        image::Pos pos, const image::Tray<transf_comp_type>& tray)
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


template<class C, class S, class P, class W, int B, int D, core::Endianness E, bool F, bool G>
void PackedPixelFormat<C, S, P, W, B, D, E, F, G>::write(word_type* buffer, image::Size image_size, image::Pos pos,
                                                         const image::Tray<const transf_comp_type>& tray)
{
    ARCHON_ASSERT(image::Box(pos, tray.size).contained_in(image_size));
    for (int y = 0; y < tray.size.height; ++y) {
        image::Pos pos_2 = pos + image::Size(0, y);
        word_type* target = get_pixel_ptr(buffer, image_size.width, pos_2);
        for (int x = 0; x < tray.size.width; ++x) {
            const transf_comp_type* source = tray(x, y);
            write_pixel(source, target); // Throws
            target += words_per_pixel;
        }
    }
}


template<class C, class S, class P, class W, int B, int D, core::Endianness E, bool F, bool G>
void PackedPixelFormat<C, S, P, W, B, D, E, F, G>::fill(word_type* buffer, image::Size image_size,
                                                        const image::Box& area, const transf_comp_type* color)
{
    ARCHON_ASSERT(area.contained_in(image_size));
    word_type color_2[words_per_pixel];
    write_pixel(color, color_2); // Throws
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


template<class C, class S, class P, class W, int B, int D, core::Endianness E, bool F, bool G>
constexpr int PackedPixelFormat<C, S, P, W, B, D, E, F, G>::get_pack_width()
{
    int width = 0;
    for (image::BitField field : channel_packing_type::fields) {
        core::int_add(width, field.width); // Throws
        core::int_add(width, field.gap); // Throws
    }
    return width;
}


template<class C, class S, class P, class W, int B, int D, core::Endianness E, bool F, bool G>
constexpr auto PackedPixelFormat<C, S, P, W, B, D, E, F, G>::get_words_per_row(int image_width) -> std::size_t
{
    std::size_t n = 1;
    core::int_mul(n, words_per_pixel); // Throws
    core::int_mul(n, image_width); // Throws
    return n;
}


template<class C, class S, class P, class W, int B, int D, core::Endianness E, bool F, bool G>
constexpr int PackedPixelFormat<C, S, P, W, B, D, E, F, G>::get_channel_width(int channel_index) noexcept
{
    int field_index = map_channel_index(channel_index);
    return image::get_bit_field_width(channel_packing_type::fields, num_channels, field_index);
}


template<class C, class S, class P, class W, int B, int D, core::Endianness E, bool F, bool G>
constexpr int PackedPixelFormat<C, S, P, W, B, D, E, F, G>::get_channel_shift(int channel_index) noexcept
{
    int field_index = map_channel_index(channel_index);
    return image::get_bit_field_shift(channel_packing_type::fields, num_channels, field_index);
}


template<class C, class S, class P, class W, int B, int D, core::Endianness E, bool F, bool G>
inline auto PackedPixelFormat<C, S, P, W, B, D, E, F, G>::get_pixel_ptr(word_type* buffer, int image_width,
                                                                        image::Pos pos) -> word_type*
{
    auto pixel_index = pos.y * std::ptrdiff_t(image_width) + pos.x;
    return buffer + pixel_index * words_per_pixel;
}


template<class C, class S, class P, class W, int B, int D, core::Endianness E, bool F, bool G>
inline auto PackedPixelFormat<C, S, P, W, B, D, E, F, G>::get_pixel_ptr(const word_type* buffer, int image_width,
                                                                        image::Pos pos) -> const word_type*
{
    auto pixel_index = pos.y * std::ptrdiff_t(image_width) + pos.x;
    return buffer + pixel_index * words_per_pixel;
}


template<class C, class S, class P, class W, int B, int D, core::Endianness E, bool F, bool G>
void PackedPixelFormat<C, S, P, W, B, D, E, F, G>::read_pixel(const word_type* source, transf_comp_type* target)
{
    // Assemble bit compound from words
    value_type pixel = 0;
    for (int i = 0; i < words_per_pixel; ++i) {
        int shift = map_word_index(i) * bits_per_word;
        pixel |= value_type(image::unpack_int<bits_per_word>(source[i])) << shift;
    }

    // Unpack components
    if constexpr (!std::is_floating_point_v<transf_comp_type> || !has_alpha_channel) {
        core::for_each_int<int, num_channels>([&](auto obj) noexcept {
            constexpr int i = obj.value;
            target[i] = get_component<i>(pixel); // Throws
        });
    }
    else {
        // Do premultiplied alpha
        constexpr int last = num_channels - 1;
        transf_comp_type alpha = get_component<last>(pixel); // Throws
        core::for_each_int<int, last>([&](auto obj) {
            constexpr int i = obj.value;
            target[i] = alpha * get_component<i>(pixel); // Throws
        }); // Throws
        target[last] = alpha;
    }
}


template<class C, class S, class P, class W, int B, int D, core::Endianness E, bool F, bool G>
void PackedPixelFormat<C, S, P, W, B, D, E, F, G>::write_pixel(const transf_comp_type* source, word_type* target)
{
    // Pack components
    value_type pixel = 0;
    if constexpr (!std::is_floating_point_v<transf_comp_type> || !has_alpha_channel) {
        core::for_each_int<int, num_channels>([&](auto obj) noexcept {
            constexpr int i = obj.value;
            set_component<i>(pixel, source[i]); // Throw
        });
    }
    else {
        // Undo premultiplied alpha
        constexpr int last = num_channels - 1;
        transf_comp_type alpha = source[last];
        transf_comp_type inv_alpha = (alpha != 0 ? 1 / alpha : 0);
        core::for_each_int<int, last>([&](auto obj) noexcept {
            constexpr int i = obj.value;
            set_component<i>(pixel, inv_alpha * source[i]); // Throw
        });
        set_component<last>(pixel, alpha); // Throw
    }

    // Split bit compound into words
    for (int i = 0; i < words_per_pixel; ++i) {
        int shift = map_word_index(i) * bits_per_word;
        value_type value = (pixel >> shift) & core::int_mask<value_type>(bits_per_word);
        target[i] = image::pack_int<word_type, bits_per_word>(value);
    }
}


template<class C, class S, class P, class W, int B, int D, core::Endianness E, bool F, bool G>
template<int I> inline auto PackedPixelFormat<C, S, P, W, B, D, E, F, G>::get_component(value_type pixel) ->
    transf_comp_type
{
    constexpr int channel_index = I;

    constexpr int width = get_channel_width(channel_index);
    constexpr int shift = get_channel_shift(channel_index);
    value_type value = (pixel >> shift) & core::int_mask<value_type>(width);

    if constexpr (std::is_integral_v<transf_comp_type>) {
        constexpr int n = image::comp_repr_bit_width<transf_repr>();
        return image::int_to_int<width, transf_comp_type, n>(value);
    }
    else {
        static_assert(std::is_same_v<transf_comp_type, image::float_type>);
        constexpr bool is_alpha = (has_alpha_channel && channel_index == num_channels - 1);
        if constexpr (!is_alpha) {
            return image::compressed_int_to_float<width>(value); // Throws
        }
        return image::int_to_float<width, transf_comp_type>(value); // Throws
    }
}


template<class C, class S, class P, class W, int B, int D, core::Endianness E, bool F, bool G>
template<int I> inline void PackedPixelFormat<C, S, P, W, B, D, E, F, G>::set_component(value_type& pixel,
                                                                                        transf_comp_type value)
{
    constexpr int channel_index = I;

    constexpr int width = get_channel_width(channel_index);
    constexpr int shift = get_channel_shift(channel_index);
    value_type value_2 = 0;

    if constexpr (std::is_integral_v<transf_comp_type>) {
        constexpr int n = image::comp_repr_bit_width<transf_repr>();
        value_2 = image::int_to_int<n, value_type, width>(value);
    }
    else {
        static_assert(std::is_same_v<transf_comp_type, image::float_type>);
        constexpr bool is_alpha = (has_alpha_channel && channel_index == num_channels - 1);
        if constexpr (!is_alpha) {
            value_2 = image::float_to_compressed_int<value_type, width>(value); // Throws
        }
        else {
            value_2 = image::float_to_int<value_type, width>(value); // Throws
        }
    }

    pixel |= value_2 << shift;
}


template<class C, class S, class P, class W, int B, int D, core::Endianness E, bool F, bool G>
constexpr int PackedPixelFormat<C, S, P, W, B, D, E, F, G>::map_channel_index(int i) noexcept
{
    // Map specified canonical channel index to index of channel in
    // `channel_packing_type::fields`.
    int n = num_channels;
    ARCHON_ASSERT(i >= 0 && i < n);
    int j = i;
    if constexpr (has_alpha_channel && alpha_channel_first)
        j = (j + 1) % n;
    if constexpr (reverse_channel_order)
        j = (n - 1) - j;
    return j;
}


template<class C, class S, class P, class W, int B, int D, core::Endianness E, bool F, bool G>
constexpr int PackedPixelFormat<C, S, P, W, B, D, E, F, G>::map_word_index(int i) noexcept
{
    // Map index from little-endian order to actual order.
    int n = words_per_pixel;
    ARCHON_ASSERT(i >= 0 && i < n);
    switch (word_order) {
        case core::Endianness::big:
            break;
        case core::Endianness::little:
            return i;
    }
    return (n - 1) - i;
}


template<class C, class S, class P, class W, int B, int D, core::Endianness E, bool F, bool G>
auto PackedPixelFormat<C, S, P, W, B, D, E, F, G>::channel_spec() const noexcept -> const channel_spec_type&
{
    return this->hidden_base();
}


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_PACKED_PIXEL_FORMAT_HPP
