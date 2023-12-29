// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2024 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_DISPLAY_X_NOINST_X_MULT_PIXEL_FORMAT_HPP
#define ARCHON_X_DISPLAY_X_NOINST_X_MULT_PIXEL_FORMAT_HPP


#include <cstddef>
#include <type_traits>
#include <iterator>
#include <utility>
#include <algorithm>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/endianness.hpp>
#include <archon/util/unit_frac.hpp>
#include <archon/image/geom.hpp>
#include <archon/image/tray.hpp>
#include <archon/image/bit_medium.hpp>
#include <archon/image/comp_types.hpp>
#include <archon/image/gamma.hpp>
#include <archon/image/comp_repr.hpp>
#include <archon/image/color_space.hpp>
#include <archon/image/buffer_format.hpp>
#include <archon/image/image.hpp>


namespace archon::display::impl {


// This is a pixel format that can be used with image::BufferedImage. It satisfies all the
// requirements of the `Archon_Image_PixelFormat` concept.
//
// This pixel format uses a set number of consecutive memory words (\p W, \p D) for each
// pixel. A set number of bits from each of those words are joined together into a compound
// value (\p B). This process is further controlled by the specified word order (\p E).
//
// The compound value for each pixel is a packing of field values, one field value for each
// channel (color and alpha). The packing scheme is encapsulated by the field specification,
// which is an object of type \p F passed to the pixel format constructor. Let `f` be an
// object of type `F`, then the following requirements must be met by the field
// specification:
//
//  * `F::compound_type` must be the integer type used to hold compound values such as those
//    produced by `F::pack()`. It is also used to hold component values such as those passed
//    to `F::pack()`. All valid compound and component values are non-negative.
//
//  * `F::num_fields` must be a valid compile-time constant expression of type `int`. It
//    must specify the number of fields in the fields specification.
//
//  * `F::field_width_ceil` must be a valid compile-time constant expression of type
//    `int`. It must specify a ceiling for the number of bits needed to represent component
//    values in each of the fields of the fields specification. For a particular
//    specification, `f`, `F::field_width_ceil` must be greater than, or equal to
//    `core::int_num_bits(f.get_max(i))` for all values of `i` between zero and
//    `F::num_fields` minus one. In general, a lower value of `F::field_width_ceil` is
//    better as it leads to faster packing and unpacking operations (`F::pack()`,
//    `F::unpack()`).
//
//  * `F::transf_repr` must be a valid compile-time constant expression of type
//    `image::CompRepr`. It must specify the transfer representation scheme (see
//    `image::Image::TransferInfo::comp_repr`).
//
//  * `f.get_max(i)` must be a valid function invocation when `i` is an object of type
//    `int`. It must also be a `noexcept` expression. When `i` is between zero and
//    `F::num_fields` minus one, the function must return the maximum valid value of the
//    N'th field where N minus one is `i`. The return type must be
//    `F::compound_type`. `F::get_max()` can be a static or a non-static member function.
//
//  * `f.pack(components)` must be a valid function invocation when `components` is an array
//    of component values of type `const F::compount_type[F::num_fields]`. It must also be a
//    `noexcept` expression. If the component values are valid, the returned value must be
//    the compound value that is the result of the packing of the components. For the N'th
//    field, a particular value is valid if it is less than, or equal to `f.get_max(i)`
//    where `i` is N minus one. The result is unspecified if one or more component values
//    are invalid.
//
//  * `f.unpack(compound, components)` must be a valid function invocation when `compound`
//    is an object of type `compound_type` and `components` is an array of component values
//    of type `F::compount_type[F::num_fields]`. It must also be a `noexcept` expression. If
//    `compound` is a valid compound value, the effect of the function invocation must be
//    the unpacking operation that corresponds to the packing operation done by
//    `F::pack()`. The resulting component values must be stored in `components`. The
//    compound value is valid if it could have been produced by `F::pack()` and a set of
//    valid components.
//
// The channels of each pixel are specified through `C`, which must be a channel
// specification type that satisfies all the requirements of the `Archon_Image_ChannelSpec`
// concept. See `image::StandardChannelSpec`.
//
// The number of fields specified by `F`, i.e. `F::num_fields`, must equal the number of
// channels specified through `C`, i.e. `C::num_channels`. The first field in specified by
// `F` corresponds to the first channel specified by `S`, and so forth.
//
// FIXME: Mention additional requirements    
//
// FIXME: Document all template parameters    
//
template<class C, class F, class W = typename F::compound_type, int B = image::bit_width<W>, int D = 1,
         core::Endianness E = core::Endianness::big>
class MultPixelFormat {
public:
    using channel_spec_type = C;
    using field_spec_type   = F;
    using word_type         = W;

    static constexpr int bits_per_word           = B;
    static constexpr int words_per_pixel         = D;
    static constexpr core::Endianness word_order = E;

    using compound_type = typename field_spec_type::compound_type;

    static constexpr bool has_alpha_channel = channel_spec_type::has_alpha_channel;
    static constexpr int num_channels = channel_spec_type::num_channels;
    static constexpr int bits_per_pixel = words_per_pixel * bits_per_word;

    static_assert(std::is_integral_v<word_type>);
    static_assert(std::is_integral_v<compound_type>);
    static_assert(bits_per_word > 0);
    static_assert(bits_per_word <= image::bit_width<word_type>);
    static_assert(words_per_pixel > 0);
    static_assert(words_per_pixel <= core::int_max<int>() / bits_per_word);
    static_assert(bits_per_pixel <= image::bit_width<compound_type>);
    static_assert(field_spec_type::num_fields == num_channels);

    // The specified channel specification will be moved into place (`std::move()`).
    //
    MultPixelFormat(channel_spec_type = {}, field_spec_type = {}) noexcept(std::is_nothrow_move_constructible_v<C> &&
                                                                           std::is_nothrow_move_constructible_v<F>);

    // Required static pixel format members. See `Concept_Archon_Image_PixelFormat`.
    //
    static constexpr bool is_indexed_color = false;
    static constexpr image::CompRepr transf_repr = field_spec_type::transf_repr;

    using transf_comp_type = image::comp_type<transf_repr>;

    // Required pixel format member functions See `Concept_Archon_Image_PixelFormat`.
    //
    static auto get_buffer_size(image::Size) -> std::size_t;
    bool try_describe(image::BufferFormat&) const;
    auto get_transfer_info() const noexcept -> image::Image::TransferInfo;
    void read(const word_type* buffer, image::Size image_size, image::Pos,
              const image::Tray<transf_comp_type>&) const noexcept;
    void write(word_type* buffer, image::Size image_size, image::Pos,
               const image::Tray<const transf_comp_type>&) const noexcept;
    void fill(word_type* buffer, image::Size image_size, const image::Box& area,
              const transf_comp_type* color) const noexcept;

    // This function returns the number of words (elements of type `word_type`) that make up
    // each row of an image of the specified width.
    //
    static constexpr auto get_words_per_row(int image_width) -> std::size_t;

private:
    ARCHON_NO_UNIQUE_ADDRESS channel_spec_type m_channel_spec;
    ARCHON_NO_UNIQUE_ADDRESS field_spec_type m_field_spec;

    static auto get_pixel_ptr(word_type* buffer, int image_width, image::Pos pos) -> word_type*;
    static auto get_pixel_ptr(const word_type* buffer, int image_width, image::Pos pos) -> const word_type*;

    void read_pixel(const word_type* source, transf_comp_type* target) const noexcept;
    void write_pixel(const transf_comp_type* source, word_type* target) const noexcept;

    auto get_component(const compound_type* components, int i) const noexcept -> transf_comp_type;
    void set_component(compound_type* components, int i, transf_comp_type value) const noexcept;

    static constexpr int map_word_index(int) noexcept;
};



template<class T, int N, int W, image::CompRepr R> class MultFieldSpec {
public:
    using compound_type = T;

    static constexpr int num_fields = N;
    static constexpr int field_width_ceil = W;
    static constexpr image::CompRepr transf_repr = R;

    struct Field;

    MultFieldSpec(compound_type offset, const Field(& fields)[N]) noexcept;
    auto get_max(int i) const noexcept -> compound_type;
    auto pack(const compound_type* components) const noexcept -> compound_type;
    void unpack(compound_type compound, compound_type* components) const noexcept;

private:
    compound_type m_offset;
    compound_type m_mult[num_fields];
    compound_type m_max[num_fields];
    int m_order[num_fields];
};


template<class T, int N, int W, image::CompRepr R>
struct MultFieldSpec<T, N, W, R>::Field {
    // The multiplier for this channel. The contribution of a channel component V to the
    // compound value is V times the multiplier.
    //
    compound_type mult;

    // The maximum value for this pixel component.
    //
    compound_type max;
};








// Implementation


template<class C, class F, class W, int B, int D, core::Endianness E>
inline MultPixelFormat<C, F, W, B, D, E>::MultPixelFormat(C channel_spec, F field_spec)
    noexcept(std::is_nothrow_move_constructible_v<C> && std::is_nothrow_move_constructible_v<F>)
    : m_channel_spec(std::move(channel_spec)) // Throws
    , m_field_spec(std::move(field_spec)) // Throws
{
}


template<class C, class F, class W, int B, int D, core::Endianness E>
inline auto MultPixelFormat<C, F, W, B, D, E>::get_buffer_size(image::Size image_size) -> std::size_t
{
    std::size_t size = get_words_per_row(image_size.width); // Throws
    core::int_mul(size, image_size.height); // Throws
    return size;
}


template<class C, class F, class W, int B, int D, core::Endianness E>
bool MultPixelFormat<C, F, W, B, D, E>::try_describe(image::BufferFormat&) const
{
    return false;
}


template<class C, class F, class W, int B, int D, core::Endianness E>
auto MultPixelFormat<C, F, W, B, D, E>::get_transfer_info() const noexcept -> image::Image::TransferInfo
{
    int max_width = 0;
    for (int i = 0; i < field_spec_type::num_fields; ++i) {
        int width = core::int_find_msb_pos(m_field_spec.get_max(i)) + 1;
        max_width = std::max(width, max_width);
    }
    const image::ColorSpace& color_space = m_channel_spec.get_color_space();
    return { transf_repr, &color_space, has_alpha_channel, max_width };
}


template<class C, class F, class W, int B, int D, core::Endianness E>
void MultPixelFormat<C, F, W, B, D, E>::read(const word_type* buffer, image::Size image_size, image::Pos pos,
                                             const image::Tray<transf_comp_type>& tray) const noexcept
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


template<class C, class F, class W, int B, int D, core::Endianness E>
void MultPixelFormat<C, F, W, B, D, E>::write(word_type* buffer, image::Size image_size, image::Pos pos,
                                              const image::Tray<const transf_comp_type>& tray) const noexcept
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


template<class C, class F, class W, int B, int D, core::Endianness E>
void MultPixelFormat<C, F, W, B, D, E>::fill(word_type* buffer, image::Size image_size,
                                             const image::Box& area, const transf_comp_type* color) const noexcept
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


template<class C, class F, class W, int B, int D, core::Endianness E>
constexpr auto MultPixelFormat<C, F, W, B, D, E>::get_words_per_row(int image_width) -> std::size_t
{
    std::size_t n = 1;
    core::int_mul(n, words_per_pixel); // Throws
    core::int_mul(n, image_width); // Throws
    return n;
}


template<class C, class F, class W, int B, int D, core::Endianness E>
inline auto MultPixelFormat<C, F, W, B, D, E>::get_pixel_ptr(word_type* buffer, int image_width,
                                                             image::Pos pos) -> word_type*
{
    auto pixel_index = pos.y * std::ptrdiff_t(image_width) + pos.x;
    return buffer + pixel_index * words_per_pixel;
}


template<class C, class F, class W, int B, int D, core::Endianness E>
inline auto MultPixelFormat<C, F, W, B, D, E>::get_pixel_ptr(const word_type* buffer, int image_width,
                                                             image::Pos pos) -> const word_type*
{
    auto pixel_index = pos.y * std::ptrdiff_t(image_width) + pos.x;
    return buffer + pixel_index * words_per_pixel;
}


template<class C, class F, class W, int B, int D, core::Endianness E>
void MultPixelFormat<C, F, W, B, D, E>::read_pixel(const word_type* source, transf_comp_type* target) const noexcept
{
    // Assemble bit compound from words
    compound_type compound = 0;
    for (int i = 0; i < words_per_pixel; ++i) {
        int shift = map_word_index(i) * bits_per_word;
        compound |= compound_type(image::unpack_int<bits_per_word>(source[i])) << shift;
    }

    // Split bit compound into components
    compound_type components[num_channels];
    m_field_spec.unpack(compound, components);

    // Unpack components
    if constexpr (!std::is_floating_point_v<transf_comp_type> || !has_alpha_channel) {
        for (int i = 0; i < num_channels; ++i)
            target[i] = get_component(components, i);
    }
    else {
        // Do premultiplied alpha
        int last = num_channels - 1;
        transf_comp_type alpha = get_component(components, last);
        for (int i = 0; i < last; ++i)
            target[i] = alpha * get_component(components, i);
        target[last] = alpha;
    }
}


template<class C, class F, class W, int B, int D, core::Endianness E>
void MultPixelFormat<C, F, W, B, D, E>::write_pixel(const transf_comp_type* source, word_type* target) const noexcept
{
    // Pack components
    compound_type components[num_channels];
    if constexpr (!std::is_floating_point_v<transf_comp_type> || !has_alpha_channel) {
        for (int i = 0; i < num_channels; ++i)
            set_component(components, i, source[i]);
    }
    else {
        // Undo premultiplied alpha
        int last = num_channels - 1;
        transf_comp_type alpha = source[last];
        transf_comp_type inv_alpha = (alpha != 0 ? 1 / alpha : 0);
        for (int i = 0; i < last; ++i)
            set_component(components, i, inv_alpha * source[i]);
        set_component(components, last, alpha);
    }

    // Assemble bit compound from components
    const compound_type(& components_2)[num_channels] = components;
    compound_type compound = m_field_spec.pack(components_2);

    // Split bit compound into words
    for (int i = 0; i < words_per_pixel; ++i) {
        int shift = map_word_index(i) * bits_per_word;
        compound_type value = (compound >> shift) & core::int_mask<compound_type>(bits_per_word);
        target[i] = image::pack_int<word_type, bits_per_word>(value);
    }
}


template<class C, class F, class W, int B, int D, core::Endianness E>
inline auto MultPixelFormat<C, F, W, B, D, E>::get_component(const compound_type* components,
                                                             int i) const noexcept -> transf_comp_type
{
    compound_type val = components[i];
    compound_type max = m_field_spec.get_max(i);
    namespace uf = util::unit_frac;
    if constexpr (std::is_integral_v<transf_comp_type>) {
        constexpr int n = field_spec_type::field_width_ceil;
        constexpr int m = image::comp_repr_int_bit_width(transf_repr);
        using unpacked_transf_type = image::unpacked_comp_type<transf_repr>;
        unpacked_transf_type max_2 = image::comp_repr_unpacked_max<transf_repr>();
        unpacked_transf_type val_2 = uf::int_to_int_a<n, m>(val, max, max_2);
        return image::comp_repr_pack<transf_repr>(val_2);
    }
    else {
        static_assert(std::is_same_v<transf_comp_type, image::float_type>);
        bool is_alpha = (has_alpha_channel && i == num_channels - 1);
        if (!is_alpha) {
            using type = decltype(image::float_type() * double());
            type val_2 = uf::int_to_flt<type>(val, max);
            return image::float_type(image::gamma_expand(val_2));
        }
        else {
            return uf::int_to_flt<image::float_type>(val, max);
        }
    }
}


template<class C, class F, class W, int B, int D, core::Endianness E>
inline void MultPixelFormat<C, F, W, B, D, E>::set_component(compound_type* components, int i,
                                                             transf_comp_type value) const noexcept
{
    compound_type max = m_field_spec.get_max(i);
    namespace uf = util::unit_frac;
    if constexpr (std::is_integral_v<transf_comp_type>) {
        constexpr int n = image::comp_repr_int_bit_width(transf_repr);
        constexpr int m = field_spec_type::field_width_ceil;
        using unpacked_transf_type = image::unpacked_comp_type<transf_repr>;
        unpacked_transf_type val_2 = image::comp_repr_unpack<transf_repr>(value);
        unpacked_transf_type max_2 = image::comp_repr_unpacked_max<transf_repr>();
        components[i] = uf::int_to_int_a<n, m>(val_2, max_2, max);
    }
    else {
        static_assert(std::is_same_v<transf_comp_type, image::float_type>);
        bool is_alpha = (has_alpha_channel && i == num_channels - 1);
        if (!is_alpha) {
            using type = decltype(image::float_type() * double());
            type val_2 = image::gamma_compress(type(value));
            components[i] = uf::flt_to_int_a<compound_type>(val_2, max);
        }
        else {
            components[i] = uf::flt_to_int_a<compound_type>(value, max);
        }
    }
}


template<class C, class F, class W, int B, int D, core::Endianness E>
constexpr int MultPixelFormat<C, F, W, B, D, E>::map_word_index(int i) noexcept
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


template<class T, int N, int W, image::CompRepr R>
MultFieldSpec<T, N, W, R>::MultFieldSpec(compound_type offset, const Field(& fields)[N]) noexcept
{
    m_offset = offset;
    for (int i = 0; i < num_fields; ++i) {
        m_mult[i] = fields[i].mult;
        m_max[i] = fields[i].max;
        m_order[i] = i;
    }
    // Highest multiplier first
    std::sort(std::begin(m_order), std::end(m_order), [&](int a, int b) noexcept {
        return (m_mult[a] > m_mult[b]);
    });
}


template<class T, int N, int W, image::CompRepr R>
inline auto MultFieldSpec<T, N, W, R>::get_max(int i) const noexcept -> compound_type
{
    return m_max[i];
}


template<class T, int N, int W, image::CompRepr R>
auto MultFieldSpec<T, N, W, R>::pack(const compound_type* components) const noexcept -> compound_type
{
    compound_type val = m_offset;
    for (int i = 0; i < num_fields; ++i)
        val += components[i] * m_mult[i];
    return val;
}


template<class T, int N, int W, image::CompRepr R>
void MultFieldSpec<T, N, W, R>::unpack(compound_type compound, compound_type* components) const noexcept
{
    compound_type val_1 = compound - m_offset;
    bool good = (compound >= m_offset);
    for (int i = 0; i < num_fields; ++i) {
        int j = m_order[i];
        compound_type val_2 = val_1 / m_mult[j];
        val_1 %= m_mult[j];
        components[j] = val_2;
        good = (good && val_2 <= m_max[j]);
    }

    if (ARCHON_LIKELY(good))
        return;

    // Out of bounds
    for (int i = 0; i < num_fields; ++i)
        components[i] = 0;
}


} // namespace archon::display::impl

#endif // ARCHON_X_DISPLAY_X_NOINST_X_MULT_PIXEL_FORMAT_HPP
