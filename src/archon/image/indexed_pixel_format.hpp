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

#ifndef ARCHON_X_IMAGE_X_INDEXED_PIXEL_FORMAT_HPP
#define ARCHON_X_IMAGE_X_INDEXED_PIXEL_FORMAT_HPP

/// \file


#include <cstddef>
#include <type_traits>
#include <utility>
#include <memory>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/endianness.hpp>
#include <archon/image/geom.hpp>
#include <archon/image/tray.hpp>
#include <archon/image/comp_types.hpp>
#include <archon/image/comp_repr.hpp>
#include <archon/image/buffer_format.hpp>
#include <archon/image/image.hpp>


namespace archon::image {


/// \brief Compile-time specification of indexed pixel format.
///
/// An instantiation of this template is a compile-time specification of a pixel format that
/// uses indexed color. Here each pixel is an index into a palette. Such a pixel format
/// implements \ref Concept_Archon_Image_PixelFormat and can therefore be used with \ref
/// image::BufferedImage.
///
/// For formats that use direct color, see \ref image::InterPixelFormat, \ref
/// image::PackedPixelFormat, \ref image::SubwordPixelFormat, and \ref
/// image::FloatingPixelFormat.                        
///
/// With this pixel format, the underlying sequence of words is aggregated into a sequence
/// of bit compounds using the specified number of words per compound (\p D) and taking the
/// specified number of bits from each word (\p B). Each constructed bit compound therefore
/// has \p D times \p B useful bits in it. The specified word order (\p E) determines how
/// words are assembled into bit compounds.
///
/// Those bits are then divided into smaller pieces such that each piece represents one one
/// pixel. Here, the number of pieces is the specified number of pixels per compound (\p N)
/// and the size of each piece in number of bits is the specified number of bits per pixel
/// (\p M). Naturally, the number of pixels per compound times the number of bits per pixel
/// must be less than, or equal to the number of useful bits in each compound (see previous
/// section). Thee specified bit order (\p A) determines how the bit compound is divided
/// into pieces.
///
/// Unused bits must be zero. This includes unused bits in words (at positions of
/// significance higher than \p B), unused bits in bit compounds (at positions of
/// significance higher than \p M times \p N), and bits in bit compounds associated with
/// unused pixel slots at end of pixel rows when the next row is aligned at a compound
/// boundary (\p H). Behavior is undefined if this pixel format is used with a pixel buffer
/// where these bits are not zero. Conversely, his pixel format guarantees that these bits
/// will remain zero.
///
/// Any pixel buffer used with this pixel format must contain a whole number of bit
/// compounds. This means that the number of words in the buffer must be divisible by the
/// number of words per bit compound (\p D). Behavior is undefined if this pixel format is
/// used with a pixel buffer whose size is not equal to `get_buffer_size(image_size)` where
/// `image_size` is the image size passed to \ref read(), \ref write(), or \ref fill(). See
/// \ref Concept_Archon_Image_PixelFormat for documentation of `get_buffer_size()`.
///
/// \tparam S The integer type to be used for assembling the bit compound. It must be a type
/// such that `image::bit_width<S>` (\ref image::bit_width) is greater than, or equal to the
/// number of bits in the bit compound, which is the numebr of bits per word (\p B) times
/// the number of words per compound (\p D).
///
/// \tparam M Number of bits per pixel (see \ref bits_per_pixel). This must be less than, or
/// equal to 8 (because an index must be representable in `image::int8_type` on all
/// platforms).
///
/// \tparam N Number of pixels per bit compound (see \ref pixels_per_compound). The number
/// of bits per pixel times the number of pixels per compound must be less than, or equal to
/// the number of bits per bit compound, which is the numebr of bits per word (\p B) times
/// the number of words per compound (\p D).
///
/// \tparam A Order of pixels within bit compound (see \ref bit_order).
///
/// \tparam W The type of words from which the bit compound is assembled. Memory will be
/// accessed in terms of words of this type.
///
/// \tparam B Number of bits per word (see \ref bits_per_word). It must be less than or
/// equal to `image::bit_width<W>` (\ref image::bit_width).
///
/// \tparam D Number of words per bit compound (see \ref words_per_compound).
///
/// \tparam E The order in which words are assembled into bit compounds (see \ref
/// word_order).
///
/// \tparam H Whether the start of each row of pixels is aligned on a bit compound boundary
/// (see \ref compound_aligned_rows).
///
template<class S, int M, int N = 1, core::Endianness A = core::Endianness::big, class W = S,
         int B = image::bit_width<W>, int D = 1, core::Endianness E = core::Endianness::big, bool H = true>
class IndexedPixelFormat {
public:
    using compound_type = S;
    using word_type     = W;

    /// \brief Specified number of bits per pixel.
    ///
    /// The specified number of bits per pixel. This determines the largest possible color
    /// index, which, in turn, sets a limit on the size of the palette (two to the power of
    /// `bits_per_pixel` colors).
    ///
    static constexpr int bits_per_pixel = M;

    /// \brief Specified number of pixels per bit compound.
    ///
    /// The specified number of pixels per bit compound. See \ref bit_order.
    ///
    static constexpr int pixels_per_compound = N;

    /// \brief Specified bit order.
    ///
    /// The specified bit order. The bit order controls the order of pixels within the bits
    /// of a bit compound. When the bit order is little-endian, the first, or left-most
    /// pixel in a compound occupies the M least significant bits in that compound, where M
    /// is \ref bits_per_pixel. When the bit order is big-endian, the first, or left-most
    /// pixel occupies the M most significant bits within the M times N least significant
    /// bits of the compound, where N is \ref pixels_per_compound. This means that if the
    /// compound has more bits than are used by pixels, then the unused bits will always be
    /// the most significant bits.
    ///
    static constexpr core::Endianness bit_order = A;

    /// \brief Specified number of bits per word.
    ///
    /// The specified number of bits per word. A bit compound is constructed from a set of
    /// words by taking this number of bits from each word and joining them according to the
    /// specified word order (see \ref word_order). When words have more than this number of
    /// bits in them, the used bits are the least significant ones. If the word type is
    /// signed, the sign bit is effectively available as an extra value bit provided that
    /// the corresponding unsigned type has more value bits that the signed type (usually
    /// the case). See also \ref image::pack_int() and \ref image::unpack_int().
    ///
    static constexpr int bits_per_word = B;

    /// \brief Specified number of words per bit compound.
    ///
    /// The specified number of words per bit compound. See \ref word_order for details on
    /// how words are assembled into bit compounds.
    ///
    static constexpr int words_per_compound = D;

    /// \brief Specified word order.
    ///
    /// The specified word order. The word order determines the order in which a sequence of
    /// words is combined into a bit compound. Within a pixel buffer (sequence of words that
    /// store a two dimension array of pixels), each run of M words is combined into a bit
    /// compound. Here, M is \ref words_per_compound. The N least significant bits are taken
    /// from each word and joined together forming a bit compound with M times N bits. Here,
    /// N is \ref bits_per_word. If the word order is little-endian, the first word
    /// (occurring first in the pixel buffer) will contribute the least significant bits in
    /// the bit compound, and words at subsequent positions in the pixel buffer will
    /// contribute bits of increasing significance. If the word order is big-endian, the
    /// first word will contribute the most significant bits in the bit compound, and words
    /// at subsequent positions will contribute bits of decreasing significance.
    ///
    static constexpr core::Endianness word_order = E;

    /// \brief Specified compound alignment of pixel rows.
    ///
    /// The specified compound alignment of pixel rows. If `true`, each row is aligned on a
    /// bit compound boundary. This means that if the bit compound that contains the last
    /// pixel in a row is not filled up with pixels from that row, the remaining pixel slot
    /// in that bit compound go unused rather than be filled with pixels from the next
    /// row. The first pixel of the next row then go into the next bit compound. On the
    /// other hand, if `compound_aligned_rows` is `false`, and the bit compound that
    /// contains the last pixel in a row is not filled with pixels from that row, then that
    /// bit compound will also contain the first pixel of the next row.
    ///
    static constexpr bool compound_aligned_rows = H;

    static constexpr int bits_per_compound = words_per_compound * bits_per_word;

    static_assert(std::is_integral_v<compound_type>);
    static_assert(bits_per_pixel > 0);
    static_assert(pixels_per_compound > 0);
    static_assert(pixels_per_compound <= bits_per_compound / bits_per_pixel);

    static_assert(std::is_integral_v<word_type>);
    static_assert(bits_per_word > 0);
    static_assert(bits_per_word <= image::bit_width<word_type>);
    static_assert(words_per_compound > 0);
    static_assert(words_per_compound <= image::bit_width<compound_type> / bits_per_word);

    /// \{
    ///
    /// \brief Construct indexed pixel format.
    ///
    /// These constructors construct an indexed pixel format whose color indexes refer to
    /// the specified palette.
    ///
    /// With the overload that takes a reference argument, the caller must ensure that the
    /// specified palette remains alive for as long as the indexed pixel format is in
    /// use. Destruction of the pixel format, however, is allowed to happen after the
    /// destruction of the palette.
    ///
    /// With the overload that takes a unique pointer argument, the ownership of the palette
    /// image is passed to the indexed pixel format.
    ///
    explicit IndexedPixelFormat(const image::Image& palette) noexcept;
    explicit IndexedPixelFormat(std::unique_ptr<const image::Image> palette) noexcept;
    /// \}

    /// \{
    ///
    /// \brief Required static pixel format members.
    ///
    /// See \ref Concept_Archon_Image_PixelFormat.
    ///
    static constexpr bool is_indexed_color = true;
    static constexpr image::CompRepr transf_repr = image::color_index_repr; // FIXME: Should be made varyable                 
    /// \}

    using transf_comp_type = image::comp_type<transf_repr>;

    /// \{
    ///
    /// \brief Required pixel format member functions.
    ///
    /// See \ref Concept_Archon_Image_PixelFormat.
    ///
    static auto get_buffer_size(image::Size) -> std::size_t;
    auto get_palette() const noexcept -> const image::Image&;
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
    /// If rows are aligned at compound boundaries (\p compound_aligned_rows), this function
    /// returns the number of words (elements of type \p word_type) that make up each row of
    /// an image of the specified width.
    ///
    static constexpr auto get_words_per_row(int image_width) -> std::size_t;

private:
    std::unique_ptr<const image::Image> m_palette_owner;
    const image::Image& m_palette;

    struct PixelPos {
        std::ptrdiff_t compound_index;
        int pixel_pos;
    };

    static auto get_pixel_pos(int image_width, image::Pos pos) -> PixelPos;

    // A type that is "prepromoted" and also avoids undefined behavior when bits are shifted
    // into and out of the highest relevant bit position.
    using value_type = image::unpacked_type<compound_type, bits_per_compound>;

    static auto read_compound(const word_type* source) noexcept -> value_type;
    static void write_compound(value_type value, word_type* target) noexcept;

    static void get_pixel(value_type compound, int pos, transf_comp_type* target) noexcept;
    static void set_pixel(const transf_comp_type* source, value_type& compound, int pos) noexcept;

    static constexpr int map_pixel_pos(int) noexcept;
    static constexpr int map_word_index(int) noexcept;
};


template<class S = char, int N = 8, core::Endianness A = core::Endianness::big, class W = S,
         int B = image::bit_width<W>, int D = 1, core::Endianness E = core::Endianness::big, bool H = true>
using IndexedPixelFormat_1 = image::IndexedPixelFormat<S, 1, N, A, W, B, D, E, H>;

template<class S = char, int N = 4, core::Endianness A = core::Endianness::big, class W = S,
         int B = image::bit_width<W>, int D = 1, core::Endianness E = core::Endianness::big, bool H = true>
using IndexedPixelFormat_2 = image::IndexedPixelFormat<S, 2, N, A, W, B, D, E, H>;

template<class S = char, int N = 2, core::Endianness A = core::Endianness::big, class W = S,
         int B = image::bit_width<W>, int D = 1, core::Endianness E = core::Endianness::big, bool H = true>
using IndexedPixelFormat_4 = image::IndexedPixelFormat<S, 4, N, A, W, B, D, E, H>;

template<class S = char, int N = 1, core::Endianness A = core::Endianness::big, class W = S,
         int B = image::bit_width<W>, int D = 1, core::Endianness E = core::Endianness::big, bool H = true>
using IndexedPixelFormat_8 = image::IndexedPixelFormat<S, 8, N, A, W, B, D, E, H>;








// Implementation


template<class S, int M, int N, core::Endianness A, class W, int B, int D, core::Endianness E, bool H>
inline IndexedPixelFormat<S, M, N, A, W, B, D, E, H>::IndexedPixelFormat(const image::Image& palette) noexcept
    : m_palette(palette)
{
}


template<class S, int M, int N, core::Endianness A, class W, int B, int D, core::Endianness E, bool H>
inline IndexedPixelFormat<S, M, N, A, W, B, D, E, H>::IndexedPixelFormat(std::unique_ptr<const image::Image> palette) noexcept
    : m_palette_owner(std::move(palette))
    , m_palette(*m_palette_owner)
{
}


template<class S, int M, int N, core::Endianness A, class W, int B, int D, core::Endianness E, bool H>
auto IndexedPixelFormat<S, M, N, A, W, B, D, E, H>::get_buffer_size(image::Size image_size) -> std::size_t
{
    if constexpr (compound_aligned_rows) {
        std::size_t size = get_words_per_row(image_size.width); // Throws
        core::int_mul(size, image_size.height); // Throws
        return size;
    }
    else {
        std::size_t n = 1;
        core::int_mul(n, image_size.width); // Throws
        core::int_mul(n, image_size.height); // Throws
        std::size_t m = core::int_div_round_up(n, pixels_per_compound);
        core::int_mul(m, words_per_compound); // Throws
        return m;
    }
}


template<class S, int M, int N, core::Endianness A, class W, int B, int D, core::Endianness E, bool H>
inline auto IndexedPixelFormat<S, M, N, A, W, B, D, E, H>::get_palette() const noexcept -> const image::Image&
{
    return m_palette;
}


template<class S, int M, int N, core::Endianness A, class W, int B, int D, core::Endianness E, bool H>
bool IndexedPixelFormat<S, M, N, A, W, B, D, E, H>::try_describe(image::BufferFormat& format) const
{
    image::BufferFormat::IntegerType word_type_2 = {};
    if (ARCHON_LIKELY(image::BufferFormat::try_map_integer_type<word_type>(word_type_2))) {
        format.set_indexed_format(word_type_2, bits_per_pixel, pixels_per_compound, bits_per_word, words_per_compound,
                                  bit_order, word_order, compound_aligned_rows); // Throws
        return true;
    }
    return false;
}


template<class S, int M, int N, core::Endianness A, class W, int B, int D, core::Endianness E, bool H>
auto IndexedPixelFormat<S, M, N, A, W, B, D, E, H>::get_transfer_info() const noexcept -> image::Image::TransferInfo
{
    return m_palette.get_transfer_info();
}


template<class S, int M, int N, core::Endianness A, class W, int B, int D, core::Endianness E, bool H>
void IndexedPixelFormat<S, M, N, A, W, B, D, E, H>::read(const word_type* buffer, image::Size image_size,
                                                         image::Pos pos,
                                                         const image::Tray<transf_comp_type>& tray) noexcept
{
    ARCHON_ASSERT(image::Box(pos, tray.size).contained_in(image_size));
    for (int y = 0; y < tray.size.height; ++y) {
        image::Pos pos_2 = pos + image::Size(0, y);
        image::Pos pos_3 = pos_2 + image::Size(tray.size.width, 0);
        PixelPos begin = get_pixel_pos(image_size.width, pos_2);
        PixelPos end   = get_pixel_pos(image_size.width, pos_3);
        const word_type* source = buffer + begin.compound_index * words_per_compound;
        value_type compound;
        int begin_pos = 0;
        int x = 0;
        if (ARCHON_LIKELY(end.compound_index > begin.compound_index)) {
            compound = read_compound(source);
            for (int p = begin.pixel_pos; p < pixels_per_compound; ++p) {
                transf_comp_type* target = tray(x, y);
                get_pixel(compound, p, target);
                ++x;
            }
            source += words_per_compound;
            int num_compounds_between = int(end.compound_index - begin.compound_index - 1);
            for (int i = 0; i < num_compounds_between; ++i) {
                compound = read_compound(source);
                for (int p = 0; p < pixels_per_compound; ++p) {
                    transf_comp_type* target = tray(x, y);
                    get_pixel(compound, p, target);
                    ++x;
                }
                source += words_per_compound;
            }
        }
        else {
            ARCHON_ASSERT(end.compound_index == begin.compound_index);
            begin_pos = begin.pixel_pos;
        }
        if (ARCHON_LIKELY(end.pixel_pos > begin_pos)) {
            compound = read_compound(source);
            for (int p = begin_pos; p < end.pixel_pos; ++p) {
                transf_comp_type* target = tray(x, y);
                get_pixel(compound, p, target);
                ++x;
            }
        }
    }
}


template<class S, int M, int N, core::Endianness A, class W, int B, int D, core::Endianness E, bool H>
void IndexedPixelFormat<S, M, N, A, W, B, D, E, H>::write(word_type* buffer, image::Size image_size, image::Pos pos,
                                                          const image::Tray<const transf_comp_type>& tray) noexcept
{
    ARCHON_ASSERT(image::Box(pos, tray.size).contained_in(image_size));
    for (int y = 0; y < tray.size.height; ++y) {
        image::Pos pos_2 = pos + image::Size(0, y);
        image::Pos pos_3 = pos_2 + image::Size(tray.size.width, 0);
        PixelPos begin = get_pixel_pos(image_size.width, pos_2);
        PixelPos end   = get_pixel_pos(image_size.width, pos_3);
        word_type* target = buffer + begin.compound_index * words_per_compound;
        value_type compound;
        int begin_pos = 0;
        int x = 0;
        if (ARCHON_LIKELY(end.compound_index > begin.compound_index)) {
            compound = read_compound(target);
            for (int p = begin.pixel_pos; p < pixels_per_compound; ++p) {
                const transf_comp_type* source = tray(x, y);
                set_pixel(source, compound, p);
                ++x;
            }
            write_compound(compound, target);
            target += words_per_compound;
            int num_compounds_between = int(end.compound_index - begin.compound_index - 1);
            for (int i = 0; i < num_compounds_between; ++i) {
                compound = 0;
                for (int p = 0; p < pixels_per_compound; ++p) {
                    const transf_comp_type* source = tray(x, y);
                    set_pixel(source, compound, p);
                    ++x;
                }
                write_compound(compound, target);
                target += words_per_compound;
            }
        }
        else {
            ARCHON_ASSERT(end.compound_index == begin.compound_index);
            begin_pos = begin.pixel_pos;
        }
        if (ARCHON_LIKELY(end.pixel_pos > begin_pos)) {
            compound = read_compound(target);
            for (int p = begin_pos; p < end.pixel_pos; ++p) {
                const transf_comp_type* source = tray(x, y);
                set_pixel(source, compound, p);
                ++x;
            }
            write_compound(compound, target);
        }
    }
}


template<class S, int M, int N, core::Endianness A, class W, int B, int D, core::Endianness E, bool H>
void IndexedPixelFormat<S, M, N, A, W, B, D, E, H>::fill(word_type* buffer, image::Size image_size,
                                                         const image::Box& area,
                                                         const transf_comp_type* color) noexcept
{
    ARCHON_ASSERT(area.contained_in(image_size));
    for (int y = area.pos.y; y < area.pos.y + area.size.height; ++y) {
        image::Pos pos_1 = { area.pos.x, y };
        image::Pos pos_2 = pos_1 + image::Size(area.size.width, 0);
        PixelPos begin = get_pixel_pos(image_size.width, pos_1);
        PixelPos end   = get_pixel_pos(image_size.width, pos_2);
        word_type* target = buffer + begin.compound_index * words_per_compound;
        value_type compound;
        int begin_pos = 0;
        if (ARCHON_LIKELY(end.compound_index > begin.compound_index)) {
            compound = read_compound(target);
            for (int p = begin.pixel_pos; p < pixels_per_compound; ++p)
                set_pixel(color, compound, p);
            write_compound(compound, target);
            target += words_per_compound;
            int num_compounds_between = int(end.compound_index - begin.compound_index - 1);
            for (int i = 0; i < num_compounds_between; ++i) {
                compound = 0;
                for (int p = 0; p < pixels_per_compound; ++p)
                    set_pixel(color, compound, p);
                write_compound(compound, target);
                target += words_per_compound;
            }
        }
        else {
            ARCHON_ASSERT(end.compound_index == begin.compound_index);
            begin_pos = begin.pixel_pos;
        }
        if (ARCHON_LIKELY(end.pixel_pos > begin_pos)) {
            compound = read_compound(target);
            for (int p = begin_pos; p < end.pixel_pos; ++p)
                set_pixel(color, compound, p);
            write_compound(compound, target);
        }
    }
}


template<class S, int M, int N, core::Endianness A, class W, int B, int D, core::Endianness E, bool H>
constexpr auto IndexedPixelFormat<S, M, N, A, W, B, D, E, H>::get_words_per_row(int image_width) -> std::size_t
{
    static_assert(compound_aligned_rows);
    std::size_t n = 1;
    core::int_mul(n, image_width); // Throws
    n = core::int_div_round_up(n, pixels_per_compound);
    core::int_mul(n, words_per_compound); // Throws
    return n;
}


template<class S, int M, int N, core::Endianness A, class W, int B, int D, core::Endianness E, bool H>
auto IndexedPixelFormat<S, M, N, A, W, B, D, E, H>::get_pixel_pos(int image_width, image::Pos pos) -> PixelPos
{
    std::ptrdiff_t compound_index;
    int pixel_pos;
    if constexpr (compound_aligned_rows) {
        int compound_index_in_row = pos.x / pixels_per_compound;
        pixel_pos = pos.x % pixels_per_compound;
        int compounds_per_row = core::int_div_round_up(image_width, pixels_per_compound);
        compound_index = (std::ptrdiff_t(pos.y * std::ptrdiff_t(compounds_per_row)) + compound_index_in_row);
    }
    else {
        auto pixel_index = pos.y * std::ptrdiff_t(image_width) + pos.x;
        compound_index = std::ptrdiff_t(pixel_index / pixels_per_compound);
        pixel_pos = int(pixel_index % pixels_per_compound);
    }
    return { compound_index, pixel_pos };
}


template<class S, int M, int N, core::Endianness A, class W, int B, int D, core::Endianness E, bool H>
auto IndexedPixelFormat<S, M, N, A, W, B, D, E, H>::read_compound(const word_type* source) noexcept -> value_type
{
    value_type value = 0;
    for (int i = 0; i < words_per_compound; ++i) {
        int shift = map_word_index(i) * bits_per_word;
        value |= value_type(image::unpack_int<bits_per_word>(source[i])) << shift;
    }
    return value;
}


template<class S, int M, int N, core::Endianness A, class W, int B, int D, core::Endianness E, bool H>
void IndexedPixelFormat<S, M, N, A, W, B, D, E, H>::write_compound(value_type value, word_type* target) noexcept
{
    for (int i = 0; i < words_per_compound; ++i) {
        int shift = map_word_index(i) * bits_per_word;
        value_type value_2 = (value >> shift) & core::int_mask<value_type>(bits_per_word);
        target[i] = image::pack_int<word_type, bits_per_word>(value_2);
    }
}


template<class S, int M, int N, core::Endianness A, class W, int B, int D, core::Endianness E, bool H>
void IndexedPixelFormat<S, M, N, A, W, B, D, E, H>::get_pixel(value_type compound, int pos,
                                                              transf_comp_type* target) noexcept
{
    value_type value = ((compound >> (map_pixel_pos(pos) * bits_per_pixel)) &
                        core::int_mask<value_type>(bits_per_pixel));
    *target = image::comp_repr_pack<transf_repr>(value);
}


template<class S, int M, int N, core::Endianness A, class W, int B, int D, core::Endianness E, bool H>
void IndexedPixelFormat<S, M, N, A, W, B, D, E, H>::set_pixel(const transf_comp_type* source, value_type& compound,
                                                              int pos) noexcept
{
    value_type value = value_type(image::comp_repr_unpack<transf_repr>(*source));
    int shift = map_pixel_pos(pos) * bits_per_pixel;
    compound &= (core::int_mask<value_type>(bits_per_compound) ^
                 (core::int_mask<value_type>(bits_per_pixel) << shift));
    compound |= (value << shift);
}


template<class S, int M, int N, core::Endianness A, class W, int B, int D, core::Endianness E, bool H>
constexpr int IndexedPixelFormat<S, M, N, A, W, B, D, E, H>::map_pixel_pos(int pos) noexcept
{
    int n = pixels_per_compound;
    ARCHON_ASSERT(pos >= 0 && pos < n);
    switch (bit_order) {
        case core::Endianness::big:
            break;
        case core::Endianness::little:
            return pos;
    }
    return (n - 1) - pos;
}


template<class S, int M, int N, core::Endianness A, class W, int B, int D, core::Endianness E, bool H>
constexpr int IndexedPixelFormat<S, M, N, A, W, B, D, E, H>::map_word_index(int i) noexcept
{
    // Map index from little-endian order to actual order.
    int n = words_per_compound;
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

#endif // ARCHON_X_IMAGE_X_INDEXED_PIXEL_FORMAT_HPP
