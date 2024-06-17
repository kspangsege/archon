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


#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <type_traits>
#include <bit>
#include <utility>
#include <iterator>
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <array>
#include <optional>
#include <string_view>
#include <string>
#include <locale>
#include <system_error>

#include <archon/core/features.h>
#include <archon/core/type_traits.hpp>
#include <archon/core/pair.hpp>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/scope_exit.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/memory.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/flat_map.hpp>
#include <archon/core/locale.hpp>
#include <archon/core/string_codec.hpp>
#include <archon/core/unicode.hpp>
#include <archon/core/format.hpp>
#include <archon/core/as_int.hpp>
#include <archon/core/quote.hpp>
#include <archon/core/endianness.hpp>
#include <archon/log.hpp>
#include <archon/math/vector.hpp>
#include <archon/util/unit_frac.hpp>
#include <archon/util/color_space.hpp>
#include <archon/util/color.hpp>
#include <archon/image.hpp>
#include <archon/image/gamma.hpp>
#include <archon/image/standard_channel_spec.hpp>
#include <archon/image/bit_field.hpp>
#include <archon/image/channel_packing.hpp>
#include <archon/image/packed_pixel_format.hpp>
#include <archon/image/indexed_pixel_format.hpp>
#include <archon/display/geometry.hpp>
#include <archon/display/connection_config_x11.hpp>
#include <archon/display/noinst/mult_pixel_format.hpp>
#include <archon/display/noinst/palette_map.hpp>
#include <archon/display/noinst/x11/support.hpp>


using namespace archon;
namespace impl = display::impl;
namespace x11 = impl::x11;


#if HAVE_X11


namespace {


// These characters exist and have the same encoding in all locales that are supported
// by Xlib on a particular platform
constexpr char g_xlib_portable_chars[] = {
    '\t', '\n',
    ' ', '!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', '/',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?',
    '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
    'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', '\\', ']', '^', '_',
    '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
    'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~',
};


// FIXME: Use std::bitset when switching to C++23 (std::bitset made constexpr in C++23)
class XlibPortableCharSet {
public:
    constexpr XlibPortableCharSet() noexcept;

    bool has(char) const noexcept;

private:
    using word_type = std::uintmax_t;

    static constexpr int s_bits_per_word = core::int_width<word_type>();
    static constexpr int s_num_positions = 256;
    static constexpr int s_num_words = core::int_div_round_up(s_num_positions, s_bits_per_word);

    word_type m_words[s_num_words] = {};

    constexpr void add(char) noexcept;

    static constexpr void decompose(char ch, int& word_index, int& bit_index) noexcept;
};


constexpr XlibPortableCharSet::XlibPortableCharSet() noexcept
{
    for (char ch : g_xlib_portable_chars)
        add(ch);
}


inline bool XlibPortableCharSet::has(char ch) const noexcept
{
    int word_index = {};
    int bit_index = {};
    decompose(ch, word_index, bit_index);
    return ((m_words[word_index] & (word_type(1) << bit_index)) != 0);
}


constexpr void XlibPortableCharSet::add(char ch) noexcept
{
    int word_index = {};
    int bit_index = {};
    decompose(ch, word_index, bit_index);
    m_words[word_index] |= word_type(1) << bit_index;
}


constexpr void XlibPortableCharSet::decompose(char ch, int& word_index, int& bit_index) noexcept
{
    auto i = std::char_traits<char>::to_int_type(ch);
    ARCHON_ASSERT(i >= 0);
    ARCHON_ASSERT(i < s_num_positions);
    word_index = int(i) / s_bits_per_word;
    bit_index = int(i) % s_bits_per_word;
    ARCHON_ASSERT(word_index < s_num_words);
}


constinit const XlibPortableCharSet g_xlib_portable_char_set;



class XutilErrorCategory final
    : public std::error_category {
public:
    auto name() const noexcept -> const char* override;
    auto message(int) const -> std::string override;
};


auto XutilErrorCategory::name() const noexcept -> const char*
{
    return "xutil";
}


auto XutilErrorCategory::message(int err) const -> std::string
{
    switch (err) {
        case XNoMemory:
            return "No memory"; // Throws
        case XLocaleNotSupported:
            return "Locale not supported"; // Throws
        case XConverterNotFound:
            return "Converter not found"; // Throws
    }
    return {};
}


constinit const XutilErrorCategory g_xutil_error_category;



class VisualFinder {
public:
    VisualFinder(Display* dpy, int screen, core::Span<const x11::VisualSpec> visual_specs);
    bool find(const x11::FindVisualParams& params, std::size_t& index);
    auto find_all(const x11::FindVisualParams& params, core::Buffer<std::size_t>& indexes) -> std::size_t;

private:
    core::Span<const x11::VisualSpec> m_visual_specs;
    int m_default_depth;
    VisualID m_default_visual;

    bool filter(const x11::VisualSpec& spec, const x11::FindVisualParams& params) noexcept;
    bool less(const x11::VisualSpec& a, const x11::VisualSpec& b, const x11::FindVisualParams& params);

    static int get_class_value(int class_) noexcept;
};


inline VisualFinder::VisualFinder(Display* dpy, int screen, core::Span<const x11::VisualSpec> visual_specs)
    : m_visual_specs(visual_specs)
{
    m_default_depth = DefaultDepth(dpy, screen);
    m_default_visual = XVisualIDFromVisual(DefaultVisual(dpy, screen));
}


bool VisualFinder::find(const x11::FindVisualParams& params, std::size_t& index)
{
    bool have_best = false;
    std::size_t best_index = {};
    std::size_t n = m_visual_specs.size();
    for (std::size_t i = 0; i < n; ++i) {
        const x11::VisualSpec& spec = m_visual_specs[i];
        bool have_new_best =
            (filter(spec, params) && (!have_best || less(m_visual_specs[best_index], spec, params))); // Throws
        if (ARCHON_LIKELY(!have_new_best))
            continue;
        have_best = true;
        best_index = i;
    }

    if (ARCHON_LIKELY(have_best)) {
        index = best_index;
        return true;
    }

    return false;
}


auto VisualFinder::find_all(const x11::FindVisualParams& params, core::Buffer<std::size_t>& indexes) -> std::size_t
{
    std::size_t offset = 0;
    std::size_t n = m_visual_specs.size();
    for (std::size_t i = 0; i < n; ++i) {
        const x11::VisualSpec& spec = m_visual_specs[i];
        if (filter(spec, params))
            indexes.append_a(i, offset); // Throws
    }
    std::size_t* begin = indexes.data();
    std::size_t* end   = begin + offset;
    std::stable_sort(begin, end, [&](std::size_t a, std::size_t b) {
        // Reverse order
        return less(m_visual_specs[b], m_visual_specs[a], params); // throws
    }); // Throws
    return offset;
}


bool VisualFinder::filter(const x11::VisualSpec& spec, const x11::FindVisualParams& params) noexcept
{
    if (params.visual_depth.has_value() && spec.info.depth != params.visual_depth.value())
        return false;
    if (params.visual_class.has_value() && spec.info.c_class != params.visual_class.value())
        return false;
    if (params.visual_type.has_value() && spec.info.visualid != params.visual_type.value())
        return false;
    if (params.require_opengl && !spec.opengl_supported)
        return false;
    if (spec.opengl_level != 0)
        return false;
    if (spec.opengl_stereo)
        return false;
    if (params.require_opengl_depth_buffer && spec.opengl_depth_buffer_bits < params.min_opengl_depth_buffer_bits)
        return false;
    if (params.require_opengl_stencil_buffer && spec.opengl_stencil_buffer_bits < params.min_opengl_stencil_buffer_bits)
        return false;
    if (params.require_opengl_accum_buffer && spec.opengl_accum_buffer_bits < params.min_opengl_accum_buffer_bits)
        return false;
    return true;
}


bool VisualFinder::less(const x11::VisualSpec& a, const x11::VisualSpec& b, const x11::FindVisualParams& params)
{
    constexpr int max_criteria = 18;
    int values_1[max_criteria];
    int values_2[max_criteria];
    int num_criteria = 0;

    // Criterion 1: Prefer default visual
    if (params.prefer_default_visual_type) {
        int i = num_criteria++;
        values_1[i] = int(a.info.visualid == m_default_visual);
        values_2[i] = int(b.info.visualid == m_default_visual);
    }

    // Criteria 2 and 3: Prefer default depth
    if (params.prefer_default_visual_depth) {
        int i_1 = num_criteria++;
        int i_2 = num_criteria++;
        values_1[i_1] = 0;
        values_1[i_2] = 0;
        if (a.info.depth >= m_default_depth) {
            values_1[i_1] = 1;
            values_1[i_2] = -a.info.depth; // Non-positive
        }
        values_2[i_1] = 0;
        values_2[i_2] = 0;
        if (b.info.depth >= m_default_depth) {
            values_2[i_1] = 1;
            values_2[i_2] = -b.info.depth; // Non-positive
        }
    }

    // Criterion 4: Best class
    {
        int i = num_criteria++;
        values_1[i] = get_class_value(a.info.c_class);
        values_2[i] = get_class_value(b.info.c_class);
    }

    // Criterion 5: Prefer double buffered
    if (params.prefer_double_buffered) {
        int i = num_criteria++;
        values_1[i] = int(a.double_buffered);
        values_2[i] = int(b.double_buffered);
    }

    // Criterion 6: Prefer OpenGL double buffered
    if (params.require_opengl) {
        int i = num_criteria++;
        values_1[i] = int(a.opengl_double_buffered);
        values_2[i] = int(b.opengl_double_buffered);
    }

    // Criterion 7: Greatest depth
    {
        int i = num_criteria++;
        values_1[i] = a.info.depth;
        values_2[i] = b.info.depth;
    }

    // Criterion 8: Highest depth buffer bit width
    if (params.require_opengl_depth_buffer) {
        int i = num_criteria++;
        values_1[i] = a.opengl_depth_buffer_bits;
        values_2[i] = b.opengl_depth_buffer_bits;
    }

    // Criterion 9: Highest stencil buffer bit width
    if (params.require_opengl_stencil_buffer) {
        int i = num_criteria++;
        values_1[i] = a.opengl_stencil_buffer_bits;
        values_2[i] = b.opengl_stencil_buffer_bits;
    }

    // Criterion 10: Highest accumulation buffer bit width
    if (params.require_opengl_accum_buffer) {
        int i = num_criteria++;
        values_1[i] = a.opengl_accum_buffer_bits;
        values_2[i] = b.opengl_accum_buffer_bits;
    }

    // Criterion 11: Highest double buffer performance
    if (params.prefer_double_buffered) {
        int i = num_criteria++;
        values_1[i] = a.double_buffered_perflevel;
        values_2[i] = b.double_buffered_perflevel;
    }

    // Criterion 12: Prefer not double buffered
    if (!params.prefer_double_buffered) {
        int i = num_criteria++;
        values_1[i] = -int(a.double_buffered);
        values_2[i] = -int(b.double_buffered);
    }

    // Criterion 13: Prefer not OpenGL double buffered
    if (!params.require_opengl) {
        int i = num_criteria++;
        values_1[i] = -int(a.opengl_double_buffered);
        values_2[i] = -int(b.opengl_double_buffered);
    }

    // Criterion 14: Lowest depth buffer bit width
    if (!params.require_opengl_depth_buffer) {
        int i = num_criteria++;
        values_1[i] = -a.opengl_depth_buffer_bits;
        values_2[i] = -b.opengl_depth_buffer_bits;
    }

    // Criterion 15: Lowest stencil buffer bit width
    if (!params.require_opengl_stencil_buffer) {
        int i = num_criteria++;
        values_1[i] = -a.opengl_stencil_buffer_bits;
        values_2[i] = -b.opengl_stencil_buffer_bits;
    }

    // Criterion 16: Lowest accumulation buffer bit width
    if (!params.require_opengl_accum_buffer) {
        int i = num_criteria++;
        values_1[i] = -a.opengl_accum_buffer_bits;
        values_2[i] = -b.opengl_accum_buffer_bits;
    }

    // Criterion 17: Lowest number of OpenGL auxiliary buffers
    {
        int i = num_criteria++;
        values_1[i] = -a.opengl_num_aux_buffers;
        values_2[i] = -b.opengl_num_aux_buffers;
    }

    // Criterion 18: Prefer no OpenGL support
    if (!params.require_opengl) {
        int i = num_criteria++;
        values_1[i] = (a.opengl_supported ? 0 : 1);
        values_2[i] = (b.opengl_supported ? 0 : 1);
    }

    ARCHON_ASSERT(num_criteria <= max_criteria);
    return std::lexicographical_compare(values_1, values_1 + num_criteria,
                                        values_2, values_2 + num_criteria); // Throws
}


int VisualFinder::get_class_value(int class_) noexcept
{
    switch (class_) {
        case StaticGray:
            return 1;
        case GrayScale:
            return 0;
        case StaticColor:
            return 3;
        case PseudoColor:
            return 2;
        case TrueColor:
            return 5;
        case DirectColor:
            return 4;
    }
    ARCHON_ASSERT_UNREACHABLE();
    return 0;
}



bool try_record_bit_fields(const XVisualInfo& visual_info, x11::BitFields& fields) noexcept
{
    using ulong = unsigned long;
    auto record = [](ulong mask, int& shift, int& width) {
        if (ARCHON_LIKELY(mask > 0)) {
            int shift_2 = std::countr_zero(mask);
            int width_2 = std::countr_one(mask >> shift_2);
            if (ARCHON_LIKELY(core::int_mask<ulong>(width_2) << shift_2 == mask)) {
                shift = shift_2;
                width = width_2;
                return true;
            }
        }
        return false;
    };
    x11::BitFields fields_2 = {};
    if (ARCHON_LIKELY((visual_info.red_mask & visual_info.green_mask) == 0 &&
                      (visual_info.red_mask & visual_info.blue_mask) == 0 &&
                      (visual_info.green_mask & visual_info.blue_mask) == 0 &&
                      record(visual_info.red_mask, fields_2.red_shift, fields_2.red_width) &&
                      record(visual_info.green_mask, fields_2.green_shift, fields_2.green_width) &&
                      record(visual_info.blue_mask, fields_2.blue_shift, fields_2.blue_width))) {
        fields = fields_2;
        return true;
    }
    return false;
}



using color_index_type = std::uint_least16_t;



template<class T, int N, int W, image::CompRepr R> class PermMultFieldSpec
    : public impl::MultFieldSpec<T, N, W, R> {
public:
    using compound_type = typename impl::MultFieldSpec<T, N, W, R>::compound_type;

    using Field = typename impl::MultFieldSpec<T, N, W, R>::Field;

    using rev_perm_type = core::FlatMap<color_index_type, color_index_type>;

    PermMultFieldSpec(compound_type offset, const Field(& fields)[N], core::Span<const color_index_type> perm,
                      const rev_perm_type& rev_perm) noexcept;
    auto pack(const compound_type* components) const noexcept -> compound_type;
    void unpack(compound_type compound, compound_type* components) const noexcept;

private:
    core::Span<const color_index_type> m_perm;
    const rev_perm_type& m_rev_perm;
};


template<class T, int N, int W, image::CompRepr R>
inline PermMultFieldSpec<T, N, W, R>::PermMultFieldSpec(compound_type offset, const Field(& fields)[N],
                                                        core::Span<const color_index_type> perm,
                                                        const rev_perm_type& rev_perm) noexcept
    : impl::MultFieldSpec<T, N, W, R>(offset, fields)
    , m_perm(perm)
    , m_rev_perm(rev_perm)
{
}


template<class T, int N, int W, image::CompRepr R>
auto PermMultFieldSpec<T, N, W, R>::pack(const compound_type* components) const noexcept -> compound_type
{
    compound_type compound = impl::MultFieldSpec<T, N, W, R>::pack(components);
    ARCHON_ASSERT(!core::is_negative(compound));
    ARCHON_ASSERT(core::int_less(compound, m_perm.size()));
    return compound_type(m_perm[std::size_t(compound)]);
}


template<class T, int N, int W, image::CompRepr R>
void PermMultFieldSpec<T, N, W, R>::unpack(compound_type compound, compound_type* components) const noexcept
{
    color_index_type index = {};
    if (ARCHON_LIKELY(core::try_int_cast(compound, index))) {
        auto i = m_rev_perm.find(index);
        if (ARCHON_LIKELY(i != m_rev_perm.end())) {
            compound_type compound_2 = compound_type(i->second);
            impl::MultFieldSpec<T, N, W, R>::unpack(compound_2, components);
            return;
        }
    }

    for (int i = 0; i < N; ++i)
        components[i] = 0;
}



template<class R> void fetch_colormap(Display* dpy, Colormap colormap, core::Span<image::Pixel<R>> buffer)
{
    using pixel_repr_type = R;
    static_assert(pixel_repr_type::color_space_tag == image::ColorSpace::Tag::lum ||
                  pixel_repr_type::color_space_tag == image::ColorSpace::Tag::rgb);

    constexpr bool is_gray = (pixel_repr_type::color_space_tag == image::ColorSpace::Tag::lum);

    using pixel_type = image::Pixel<R>;

    constexpr int max_chunk_size = 64;
    XColor colors[max_chunk_size];

    std::size_t size = buffer.size();
    std::size_t offset = 0;
    while (offset < size) {
        int chunk_size = int(std::min(size - offset, std::size_t(max_chunk_size)));
        using ulong = unsigned long;
        for (int i = 0; i < chunk_size; ++i)
            colors[i].pixel = ulong(offset + i);
        XQueryColors(dpy, colormap, colors, chunk_size);
        for (int i = 0; i < chunk_size; ++i) {
            const XColor& color = colors[i];
            if constexpr (is_gray) {
                image::Pixel_Lum_16 pixel = std::array {
                    image::comp_repr_pack<image::CompRepr::int16>(color.red),
                };
                buffer[offset + i] = pixel_type(pixel);
            }
            else {
                image::Pixel_RGB_16 pixel = std::array {
                    image::comp_repr_pack<image::CompRepr::int16>(color.red),
                    image::comp_repr_pack<image::CompRepr::int16>(color.green),
                    image::comp_repr_pack<image::CompRepr::int16>(color.blue),
                };
                buffer[offset + i] = pixel_type(pixel);
            }
        }
        offset += chunk_size;
    }
}



template<class T> class ImageBridgeImpl
    : public x11::ImageBridge {
public:
    using format_type = T;

    ImageBridgeImpl(display::Size size, format_type&& format)
        : x11::ImageBridge(m_img)
        , m_img(size, std::move(format)) // Throws
    {
    }

    auto get_data() noexcept -> char*
    {
        return m_img.get_buffer().data();
    }

private:
    image::BufferedImage<format_type> m_img;
};



template<bool G, class T, int N> class MultFieldPixelFormat final
    : public x11::PixelFormat {
public:
    using compound_type = T;

    static constexpr bool is_gray = G;
    static constexpr int bytes_per_pixel = N;

    MultFieldPixelFormat(Display*, const XVisualInfo&, const XPixmapFormatValues&, const x11::MultFields&,
                         core::Slab<color_index_type> perm, x11::ColormapWrapper);

    auto intern_color(util::Color) const noexcept -> unsigned long override;
    auto create_image_bridge(display::Size) const -> std::unique_ptr<x11::ImageBridge> override;

private:
    using rev_perm_type = core::FlatMap<color_index_type, color_index_type>;

    Display* const m_dpy;
    const XVisualInfo& m_visual_info;
    const XPixmapFormatValues& m_pixmap_format;
    const x11::MultFields m_fields;
    const core::Slab<color_index_type> m_perm;
    const rev_perm_type m_rev_perm;

    static auto make_rev_perm(core::Span<const color_index_type> perm) -> rev_perm_type;
};


template<bool G, class T, int N>
inline MultFieldPixelFormat<G, T, N>::MultFieldPixelFormat(Display* dpy, const XVisualInfo& visual_info,
                                                           const XPixmapFormatValues& pixmap_format,
                                                           const x11::MultFields& fields,
                                                           core::Slab<color_index_type> perm,
                                                           x11::ColormapWrapper colormap)
: x11::PixelFormat(std::move(colormap))
    , m_dpy(dpy)
    , m_visual_info(visual_info)
    , m_pixmap_format(pixmap_format)
    , m_fields(fields)
    , m_perm(std::move(perm))
    , m_rev_perm(make_rev_perm(m_perm)) // Throws
{
}


template<bool G, class T, int N>
auto MultFieldPixelFormat<G, T, N>::intern_color(util::Color color) const noexcept -> unsigned long
{
    // FIXME: Is the proper scaling scheme used here?                   

    namespace uf = util::unit_frac;
    using ulong = unsigned long;

    auto scale = [](util::Color::comp_type val, ulong max) noexcept {
        constexpr int n = 8;
        constexpr int m = 16;
        return uf::int_to_int_a<n, m>(val, util::Color::comp_type(255), max);
    };

    auto expand = [](util::Color::comp_type val) noexcept -> double {
        return image::gamma_expand(uf::int_to_flt<double>(val, 255));
    };

    auto compress = [](double val, ulong max) noexcept -> ulong {
        return uf::flt_to_int_a<ulong>(image::gamma_compress(val), max);
    };

    ulong r = 0, g = 0, b = 0;
    if constexpr (is_gray) {
        if (color.is_opaque() && color.is_gray()) {
            r = scale(color.red(), m_fields.red_max);
        }
        else {
            double a = uf::int_to_flt<double>(color.alpha(), 255);
            double r_2 = a * expand(color.red());
            double g_2 = a * expand(color.green());
            double b_2 = a * expand(color.blue());
            double lum = util::cvt_RGB_to_Lum(math::Vector3(r_2, g_2, b_2));
            r = compress(lum, m_fields.red_max);
        }
    }
    else {
        if (ARCHON_LIKELY(color.is_opaque())) {
            r = scale(color.red(), m_fields.red_max);
            g = scale(color.green(), m_fields.green_max);
            b = scale(color.blue(), m_fields.blue_max);
        }
        else {
            double a = uf::int_to_flt<double>(color.alpha(), 255);
            r = compress(a * expand(color.red()), m_fields.red_max);
            g = compress(a * expand(color.green()), m_fields.green_max);
            b = compress(a * expand(color.blue()), m_fields.blue_max);
        }
    }

    ulong compound = m_fields.pack(r, g, b);
    if (m_perm.empty())
        return compound;
    ARCHON_ASSERT(!core::is_negative(compound));
    ARCHON_ASSERT(core::int_less(compound, m_perm.size()));
    return ulong(m_perm[std::size_t(compound)]);
}


template<bool G, class T, int N>
auto MultFieldPixelFormat<G, T, N>::create_image_bridge(display::Size size) const -> std::unique_ptr<x11::ImageBridge>
{
    // FIXME: Currently, little-endian byte-order is agreed upon by the two pixel format
    // descriptions, Archon and X11. The important thing is that they agree. Ideally,
    // however, for the sake of performance, the endianness should be chosen to match the
    // native platform endianness, which would then allow for a multiplicative format using
    // a word type equal to the compound type (so long as a byte has 8 bits and a compound
    // has `bytes_per_pixel` bytes).                        

    constexpr image::CompRepr transf_repr = image::CompRepr::float_;                                      

    constexpr int bits_per_word = 8;
    constexpr int words_per_pixel = bytes_per_pixel;
    constexpr int bits_per_pixel = words_per_pixel * bits_per_word;

    using channel_spec_type = std::conditional_t<is_gray, image::ChannelSpec_Lum, image::ChannelSpec_RGB>;
    constexpr int field_width_ceil = bits_per_pixel;
    using base_field_spec_type = impl::MultFieldSpec<compound_type, channel_spec_type::num_channels,
                                                     field_width_ceil, transf_repr>;

    using word_type = char;
    constexpr core::Endianness word_order = core::Endianness::little;

    typename base_field_spec_type::Field fields[channel_spec_type::num_channels];
    fields[0] = { compound_type(m_fields.red_mult), compound_type(m_fields.red_max) };
    if constexpr (!is_gray) {
        fields[1] = { compound_type(m_fields.green_mult), compound_type(m_fields.green_max) };
        fields[2] = { compound_type(m_fields.blue_mult),  compound_type(m_fields.blue_max)  };
    }

    std::unique_ptr<x11::ImageBridge> bridge;
    char* buffer;
    if (m_perm.empty()) {
        using field_spec_type = base_field_spec_type;
        using format_type = impl::MultPixelFormat<channel_spec_type, field_spec_type, word_type, bits_per_word,
                                                  words_per_pixel, word_order>;
        field_spec_type field_spec(compound_type(m_fields.offset), fields);
        format_type format(channel_spec_type(), field_spec);
        auto bridge_2 = std::make_unique<ImageBridgeImpl<format_type>>(size, std::move(format)); // Throws
        buffer = bridge_2->get_data();
        bridge = std::move(bridge_2);
    }
    else {
        using field_spec_type = PermMultFieldSpec<compound_type, channel_spec_type::num_channels, field_width_ceil,
                                                  transf_repr>;
        using format_type = impl::MultPixelFormat<channel_spec_type, field_spec_type, word_type, bits_per_word,
                                                  words_per_pixel, word_order>;
        field_spec_type field_spec(compound_type(m_fields.offset), fields, m_perm, m_rev_perm);
        format_type format(channel_spec_type(), field_spec);
        auto bridge_2 = std::make_unique<ImageBridgeImpl<format_type>>(size, std::move(format)); // Throws
        buffer = bridge_2->get_data();
        bridge = std::move(bridge_2);
    }

    int byte_order = LSBFirst;
    x11::init_ximage(m_dpy, bridge->img_2, m_visual_info, m_pixmap_format, byte_order, size, buffer); // Throws

    return bridge;
}


template<bool G, class T, int N>
auto MultFieldPixelFormat<G, T, N>::make_rev_perm(core::Span<const color_index_type> perm) -> rev_perm_type
{
    std::size_t n = perm.size();
    rev_perm_type rev_perm;
    rev_perm.reserve(n); // Throws
    for (std::size_t i = 0; i < n; ++i)
        rev_perm[perm[i]] = color_index_type(i);
    return rev_perm;
}



template<bool G, class T, class P, int N, bool R> class BitFieldPixelFormat final
    : public x11::PixelFormat {
public:
    using compound_type = T;
    using packing_type = P;

    static constexpr bool is_gray = G;
    static constexpr int bytes_per_pixel = N;
    static constexpr bool reverse_channel_order = R;

    BitFieldPixelFormat(Display*, const XVisualInfo&, const XPixmapFormatValues&, x11::ColormapWrapper) noexcept;

    auto intern_color(util::Color) const noexcept -> unsigned long override;
    auto create_image_bridge(display::Size) const -> std::unique_ptr<x11::ImageBridge> override;

private:
    Display* const m_dpy;
    const XVisualInfo& m_visual_info;
    const XPixmapFormatValues& m_pixmap_format;
};


template<bool G, class T, class P, int N, bool R>
inline BitFieldPixelFormat<G, T, P, N, R>::BitFieldPixelFormat(Display* dpy, const XVisualInfo& visual_info,
                                                               const XPixmapFormatValues& pixmap_format,
                                                               x11::ColormapWrapper colormap) noexcept
    : x11::PixelFormat(std::move(colormap))
    , m_dpy(dpy)
    , m_visual_info(visual_info)
    , m_pixmap_format(pixmap_format)
{
}


template<bool G, class T, class P, int N, bool R>
auto BitFieldPixelFormat<G, T, P, N, R>::intern_color(util::Color color) const noexcept -> unsigned long
{
    using ulong = unsigned long;
    if constexpr (is_gray) {
        static_assert(packing_type::num_fields == 1);

        constexpr int width = image::get_bit_field_width(packing_type::fields, 1, 0);
        constexpr int shift = image::get_bit_field_shift(packing_type::fields, 1, 0);

        ulong level;
        if (color.is_opaque() && color.is_gray()) {
            level = image::int_to_int<8, ulong, width>(color.red());
        }
        else {
            double a = image::int_to_float<8, double>(color.alpha());
            double r_2 = a * image::compressed_int_to_float<8>(color.red());
            double g_2 = a * image::compressed_int_to_float<8>(color.green());
            double b_2 = a * image::compressed_int_to_float<8>(color.blue());
            double lum = util::cvt_RGB_to_Lum(math::Vector3(r_2, g_2, b_2));
            level = image::float_to_compressed_int<ulong, width>(image::float_type(lum));
        }
        return (level << shift);
    }
    else {
        static_assert(packing_type::num_fields == 3);

        constexpr int red_width   = image::get_bit_field_width(packing_type::fields, 3, 0);
        constexpr int green_width = image::get_bit_field_width(packing_type::fields, 3, 1);
        constexpr int blue_width  = image::get_bit_field_width(packing_type::fields, 3, 2);

        constexpr int red_shift   = image::get_bit_field_shift(packing_type::fields, 3, 0);
        constexpr int green_shift = image::get_bit_field_shift(packing_type::fields, 3, 1);
        constexpr int blue_shift  = image::get_bit_field_shift(packing_type::fields, 3, 2);

        namespace uf = util::unit_frac;
        ulong r, g, b;
        if (ARCHON_LIKELY(color.is_opaque())) {
            r = image::int_to_int<8, ulong, red_width>(color.red());
            g = image::int_to_int<8, ulong, green_width>(color.green());
            b = image::int_to_int<8, ulong, blue_width>(color.blue());
        }
        else {
            image::float_type a = image::int_to_float<8, image::float_type>(color.alpha());
            r = image::float_to_compressed_int<ulong, red_width>(a * image::compressed_int_to_float<8>(color.red()));
            g = image::float_to_compressed_int<ulong, green_width>(a * image::compressed_int_to_float<8>(color.green()));
            b = image::float_to_compressed_int<ulong, blue_width>(a * image::compressed_int_to_float<8>(color.blue()));
        }
        return (r << red_shift) | (g << green_shift) | (b << blue_shift);
    }
}


template<bool G, class T, class P, int N, bool R>
auto BitFieldPixelFormat<G, T, P, N, R>::create_image_bridge(display::Size size) const ->
    std::unique_ptr<x11::ImageBridge>
{
    // FIXME: Currently, little-endian byte-order is agreed upon by the two pixel format
    // descriptions, Archon and X11. The important thing is that they agree. Ideally,
    // however, for the sake of performance, the endianness should be chosen to match the
    // native platform endianness, which would then allow for a packed format using a word
    // type equal to the compound type (so long as a byte has 8 bits and a compound has
    // `bytes_per_pixel` bytes).                        

    using channel_spec_type = std::conditional_t<is_gray, image::ChannelSpec_Lum, image::ChannelSpec_RGB>;
    using word_type = char;
    constexpr int bits_per_word = 8;
    constexpr core::Endianness word_order = core::Endianness::little;
    constexpr bool alpha_channel_first = false;

    using format_type = image::PackedPixelFormat<channel_spec_type, compound_type, packing_type, word_type,
                                                 bits_per_word, bytes_per_pixel, word_order, alpha_channel_first,
                                                 reverse_channel_order>;

    auto bridge = std::make_unique<ImageBridgeImpl<format_type>>(size, format_type()); // Throws

    int byte_order = LSBFirst;
    char* buffer = bridge->get_data();
    x11::init_ximage(m_dpy, bridge->img_2, m_visual_info, m_pixmap_format, byte_order, size, buffer); // Throws

    return bridge;
}



template<int N> class IndexedPixelFormat final
    : public x11::PixelFormat {
public:
    static constexpr int bytes_per_pixel = N;

    IndexedPixelFormat(Display*, const XVisualInfo&, const XPixmapFormatValues&, std::unique_ptr<image::Image> palette,
                       x11::ColormapWrapper);

    auto intern_color(util::Color) const -> unsigned long override;
    auto create_image_bridge(display::Size) const -> std::unique_ptr<x11::ImageBridge> override;

private:
    Display* const m_dpy;
    const XVisualInfo& m_visual_info;
    const XPixmapFormatValues& m_pixmap_format;
    const std::unique_ptr<image::Image> m_palette;
    const impl::PaletteMap m_palette_map;
};


template<int N>
inline IndexedPixelFormat<N>::IndexedPixelFormat(Display* dpy, const XVisualInfo& visual_info,
                                                 const XPixmapFormatValues& pixmap_format,
                                                 std::unique_ptr<image::Image> palette, x11::ColormapWrapper colormap)
    : x11::PixelFormat(std::move(colormap))
    , m_dpy(dpy)
    , m_visual_info(visual_info)
    , m_pixmap_format(pixmap_format)
    , m_palette(std::move(palette))
    , m_palette_map(*m_palette) // Throws
{
}


template<int N>
auto IndexedPixelFormat<N>::intern_color(util::Color color) const -> unsigned long
{
    using type = image::float_type;
    type a = image::int_to_float<8, type>(color.alpha());
    type components[3] {
        type(a * image::compressed_int_to_float<8>(color.red())),
        type(a * image::compressed_int_to_float<8>(color.green())),
        type(a * image::compressed_int_to_float<8>(color.blue())),
    };

    // FIXME: Should probably convert color to CIELAB colorspace (this assumes that palette
    // is also changed to be expressed in terms of CIELAB)        

    using ulong = unsigned long;
    int index = {};
    bool found = m_palette_map.reverse_lookup(components, index); // Throws
    ARCHON_ASSERT(found); // Palette reflects X11 colormap, so can never be empty
    return ulong(index);
}


template<int N>
auto IndexedPixelFormat<N>::create_image_bridge(display::Size size) const -> std::unique_ptr<x11::ImageBridge>
{
    // FIXME: Currently, little-endian byte-order is agreed upon by the two pixel format
    // descriptions, Archon and X11. The important thing is that they agree. Ideally,
    // however, for the sake of performance, the endianness should be chosen to match the
    // native platform endianness, which would then allow for a packed format using a word
    // type equal to the compound type (so long as a byte has 8 bits and a compound has
    // `bytes_per_pixel` bytes).                        

    // FIXME: It is weird that the compound type expected by image::IndexedPixelFormat is the packed, and not the unpacked type                  
    constexpr int bits_per_byte = 8;
    constexpr int bits_per_pixel = bytes_per_pixel * bits_per_byte;
    static_assert(bits_per_pixel <= 32);
    using compound_type = std::conditional_t<bits_per_pixel <= 8, image::int8_type,
                                             std::conditional_t<bits_per_pixel <= 16, image::int16_type,
                                                                image::int32_type>>;

    constexpr int pixels_per_compound = 1;
    constexpr core::Endianness bit_order = core::Endianness::big; // Immaterial
    using word_type = char;
    constexpr int bits_per_word = bits_per_byte;
    constexpr int words_per_compound = bytes_per_pixel;
    constexpr core::Endianness word_order = core::Endianness::little;
    constexpr bool compound_aligned_rows = true; // Immaterial
    using format_type = image::IndexedPixelFormat<compound_type, bits_per_pixel, pixels_per_compound, bit_order,
                                                  word_type, bits_per_word, words_per_compound, word_order,
                                                  compound_aligned_rows>;
    format_type format(*m_palette);

    using bridge_type = ImageBridgeImpl<format_type>;
    auto bridge = std::make_unique<bridge_type>(size, std::move(format)); // Throws

    int byte_order = LSBFirst;
    char* buffer = bridge->get_data();
    x11::init_ximage(m_dpy, bridge->img_2, m_visual_info, m_pixmap_format, byte_order, size, buffer); // Throws

    return bridge;
}



struct MultFieldsDigest {
    std::array<unsigned long, 3> max;
    std::array<unsigned long, 3> mult;
    int order[3] = { 0, 1, 2 };

    constexpr MultFieldsDigest(const x11::MultFields&) noexcept;

    constexpr bool is_valid_and_compact(unsigned long offset, int colormap_size, bool is_gray) const noexcept;

    // Precondition: Must be valid and compact (`is_valid_and_compact()`)
    constexpr bool is_confined_to_depth(int depth) const noexcept;
};


constexpr MultFieldsDigest::MultFieldsDigest(const x11::MultFields& fields) noexcept
{
    max  = { fields.red_max,  fields.green_max,  fields.blue_max  };
    mult = { fields.red_mult, fields.green_mult, fields.blue_mult };
    std::sort(std::begin(order), std::end(order), [&](int a, int b) noexcept {
        return (mult[a] < mult[b]);
    });
}


constexpr bool MultFieldsDigest::is_valid_and_compact(unsigned long offset, int colormap_size, bool is_gray) const noexcept
{
    ARCHON_ASSERT(colormap_size >= 0);
    using ulong = unsigned long;
    if (is_gray) {
        return (offset <= ulong(colormap_size) &&
                max[order[0]] == 0 &&
                max[order[1]] == 0 &&
                max[order[2]] < ulong(colormap_size) - offset &&
                mult[order[0]] == 0 &&
                mult[order[1]] == 0 &&
                mult[order[2]] == 1);
    }
    else {
        return (offset <= ulong(colormap_size) &&
                max[order[0]] < mult[order[1]] &&
                max[order[1]] < mult[order[2]] / mult[order[1]] &&
                max[order[2]] < (ulong(colormap_size) - offset) / mult[order[2]] &&
                mult[order[0]] == 1 &&
                mult[order[1]] == max[order[0]] + 1 &&
                mult[order[2]] == (max[order[1]] + 1) * mult[order[1]]);
    }
}


constexpr bool MultFieldsDigest::is_confined_to_depth(int depth) const noexcept
{
    ARCHON_ASSERT(depth >= 0);
    using ulong = unsigned long;
    return (depth >= core::num_value_bits<ulong>() || (max[order[2]] + 1) * mult[order[2]] <= ulong(1) << depth);
}



class PixelFormatCreator {
public:
    PixelFormatCreator(Display* dpy, ::Window root, const XVisualInfo& visual_info,
                       const XPixmapFormatValues& pixmap_format, const x11::ColormapFinder& colormap_finder,
                       const std::locale& locale, log::Logger& logger, bool prefer_default_nondecomposed_colormap,
                       bool weird) noexcept;

    bool create(std::unique_ptr<x11::PixelFormat>& format, std::string* error_message) const;

private:
    enum class Error;

    Display* m_dpy;
    ::Window m_root;
    const XVisualInfo& m_visual_info;
    const XPixmapFormatValues& m_pixmap_format;
    const x11::ColormapFinder& m_colormap_finder;
    const std::locale& m_locale;
    log::Logger& m_logger;
    const bool m_prefer_default_nondecomposed_colormap;
    const bool m_weird;

    bool create_any(std::unique_ptr<x11::PixelFormat>&, Error&) const;
    bool create_staticgray(std::unique_ptr<x11::PixelFormat>&, Error&) const;
    bool create_grayscale(std::unique_ptr<x11::PixelFormat>&, Error&) const;
    bool create_staticcolor(std::unique_ptr<x11::PixelFormat>&, Error&) const;
    bool create_pseudocolor(std::unique_ptr<x11::PixelFormat>&, Error&) const;
    bool create_truecolor(const x11::BitFields&, std::unique_ptr<x11::PixelFormat>&, Error&, bool fake = false) const;
    bool create_directcolor(const x11::BitFields&, std::unique_ptr<x11::PixelFormat>&, Error&) const;

    template<bool G>
    bool create_multfield_1(const x11::MultFields&, core::Slab<color_index_type> perm, x11::ColormapWrapper,
                            std::unique_ptr<x11::PixelFormat>&, Error&) const;
    template<bool G, int N>
    bool create_multfield_2(const x11::MultFields&, core::Slab<color_index_type> perm, x11::ColormapWrapper,
                            std::unique_ptr<x11::PixelFormat>&, Error&) const;

    template<bool G>
    bool create_bitfield_1(const x11::BitFields&, x11::ColormapWrapper, std::unique_ptr<x11::PixelFormat>&,
                           Error&) const;
    template<bool G, int N>
    bool create_bitfield_2(const x11::BitFields&, x11::ColormapWrapper, std::unique_ptr<x11::PixelFormat>&,
                           Error&) const;
    template<bool G, int N, class P, bool R>
    bool create_bitfield_3(x11::ColormapWrapper, std::unique_ptr<x11::PixelFormat>&, Error&) const;

    template<bool G>
    bool create_palette_1(Colormap, std::unique_ptr<image::Image>& palette, Error&) const;
    template<bool G, image::CompRepr R>
    bool create_palette_2(Colormap, std::unique_ptr<image::Image>& palette, Error&) const;

    bool create_indexed_1(std::unique_ptr<image::Image> palette, x11::ColormapWrapper,
                          std::unique_ptr<x11::PixelFormat>&, Error&) const;
    template<int N>
    bool create_indexed_2(std::unique_ptr<image::Image> palette, x11::ColormapWrapper,
                          std::unique_ptr<x11::PixelFormat>&, Error&) const;
    template<int N, image::CompRepr R>
    bool create_indexed_3(std::unique_ptr<image::Image> palette, x11::ColormapWrapper,
                          std::unique_ptr<x11::PixelFormat>&, Error&) const;

    bool find_nondecomposed_standard_colormap(XStandardColormap&, bool is_gray) const;
    bool find_decomposed_standard_colormap(const x11::BitFields&, XStandardColormap&) const;

    template<class P> static bool norm_mask_match(const x11::BitFields&) noexcept;
    template<class P> static bool rev_mask_match(const x11::BitFields&) noexcept;
    template<class P> static bool mask_match(const x11::BitFields&, bool reverse) noexcept;

    auto format_error_message(Error) const -> std::string;
};


inline PixelFormatCreator::PixelFormatCreator(Display* dpy, ::Window root, const XVisualInfo& visual_info,
                                              const XPixmapFormatValues& pixmap_format,
                                              const x11::ColormapFinder& colormap_finder, const std::locale& locale,
                                              log::Logger& logger, bool prefer_default_nondecomposed_colormap,
                                              bool weird) noexcept
    : m_dpy(dpy)
    , m_root(root)
    , m_visual_info(visual_info)
    , m_pixmap_format(pixmap_format)
    , m_colormap_finder(colormap_finder)
    , m_locale(locale)
    , m_logger(logger)
    , m_prefer_default_nondecomposed_colormap(prefer_default_nondecomposed_colormap)
    , m_weird(weird)
{
}


bool PixelFormatCreator::create(std::unique_ptr<x11::PixelFormat>& format, std::string* error_message) const
{
    Error error = {};
    if (ARCHON_LIKELY(create_any(format, error))) // Throws
        return true;
    if (error_message)
        *error_message = format_error_message(error); // Throws
    return false;
}


enum class PixelFormatCreator::Error {
    unsupported_bits_per_pixel,
    unsupported_depth,
    unsupported_channel_masks,
    unsupported_colormap_size,
};


bool PixelFormatCreator::create_any(std::unique_ptr<x11::PixelFormat>& format, Error& error) const
{
    using ulong = unsigned long;
    int class_ = m_visual_info.c_class;
    int depth = m_visual_info.depth;
    int colormap_depth = core::int_find_msb_pos(m_visual_info.colormap_size);
    x11::BitFields fields = {};
    bool valid_fields = try_record_bit_fields(m_visual_info, fields);
    int width = std::max({ fields.red_width, fields.green_width, fields.blue_width });
    int bits_per_pixel = m_pixmap_format.bits_per_pixel;
    ulong masks = m_visual_info.red_mask | m_visual_info.green_mask | m_visual_info.blue_mask;
    bool confined_masks = (core::int_find_msb_pos(masks) < depth);

    if (ARCHON_UNLIKELY(bits_per_pixel < depth))
        throw std::runtime_error("Bits per pixel of pixmap format is less than depth");

    switch (class_) {
        case StaticColor:
            // According to the X protocol specification, masks only have meaning for
            // decomposed visual classes (TrueColor and DirectColor). Never the less, some X
            // servers choose to expose the color structure of StaticColor visuals using
            // valid nonzero masks, notably Xephyr (e.g., using `Xephyr :1 -screen
            // 1024x1024x8`). If this information is reliable, it will be advantageous to
            // use it. The following assumes that the information is reliable when it
            // appears to be valid.
            if (bits_per_pixel == 8 && valid_fields && depth <= colormap_depth && confined_masks) {
                bool fake = true;
                return create_truecolor(fields, format, error, fake); // Throws
            }
            goto nondecomposed;
        case StaticGray:
        case GrayScale:
        case PseudoColor:
            goto nondecomposed;
        case TrueColor:
        case DirectColor:
            goto decomposed;
    }
    throw std::runtime_error("Unexpected visual class");

  nondecomposed:
    if (ARCHON_UNLIKELY(depth > colormap_depth))
        throw std::runtime_error("Depth too large for colormap");
    switch (class_) {
        case StaticGray:
            return create_staticgray(format, error); // Throws
        case GrayScale:
            return create_grayscale(format, error); // Throws
        case StaticColor:
            return create_staticcolor(format, error); // Throws
        case PseudoColor:
            return create_pseudocolor(format, error); // Throws
    }
    ARCHON_ASSERT_UNREACHABLE();
    return false;

  decomposed:
    if (ARCHON_UNLIKELY(!valid_fields))
        throw std::runtime_error("Channel masks are zero, overlapping, or non-contiguous");
    if (ARCHON_UNLIKELY(width > colormap_depth))
        throw std::runtime_error("Channels too wide for colormap");
    if (ARCHON_UNLIKELY(!confined_masks))
        throw std::runtime_error("Channel masks escape depth of visual");
    switch (class_) {
        case TrueColor:
            return create_truecolor(fields, format, error); // Throws
        case DirectColor:
            return create_directcolor(fields, format, error); // Throws
    }
    ARCHON_ASSERT_UNREACHABLE();
    return false;
}


bool PixelFormatCreator::create_staticgray(std::unique_ptr<x11::PixelFormat>& format, Error& error) const
{
    // Assumption: For a StaticGray visual and when given a color with equal red, green, and
    // blue components, XAllocColor finds the closest gray-level in the colormap. Allocation
    // never fails, but the returned gray-level can be very different from the requested
    // one.

    ARCHON_ASSERT(m_visual_info.c_class == StaticGray);

    VisualID visual = m_visual_info.visualid;
    x11::ColormapWrapper colormap_owner;

    // Look for a standard colormap
    XStandardColormap params = {};
    constexpr bool is_gray = true;
    if (find_nondecomposed_standard_colormap(params, is_gray)) { // Throws
        colormap_owner.set_unowned(params.colormap);
        m_logger.detail("Found suitable standard colormap (%s) for StaticGray visual (%s): max = %s, mult = %s, "
                        "base_pixel = %s", core::as_flex_int_h(params.colormap), core::as_flex_int_h(visual),
                        core::as_int(params.red_max), core::as_int(params.red_mult),
                        core::as_int(params.base_pixel)); // Throws
        x11::MultFields fields(params);
        core::Slab<color_index_type> perm; // No permutation
        return create_multfield_1<is_gray>(fields, std::move(perm), std::move(colormap_owner),
                                           format, error); // Throws
    }

    // Use default colormap if possible, else create new colormap
    Colormap colormap = {};
    if (m_colormap_finder.find_default_colormap(visual, colormap)) {
        colormap_owner.set_unowned(colormap);
        m_logger.detail("Using default colormap (%s) for StaticGray visual (%s)", core::as_flex_int_h(colormap),
                        core::as_flex_int_h(visual)); // Throws
    }
    else {
        colormap = XCreateColormap(m_dpy, m_root, m_visual_info.visual, AllocNone);
        colormap_owner.set_owned(m_dpy, colormap);
        m_logger.detail("New colormap (%s) created for StaticGray visual (%s)", core::as_flex_int_h(colormap),
                        core::as_flex_int_h(visual)); // Throws
    }

    std::unique_ptr<image::Image> palette;
    if (ARCHON_LIKELY(create_palette_1<is_gray>(colormap, palette, error))) // Throws
        return create_indexed_1(std::move(palette), std::move(colormap_owner), format, error); // Throws
    return false;
}


bool PixelFormatCreator::create_grayscale(std::unique_ptr<x11::PixelFormat>& format, Error& error) const
{
    ARCHON_ASSERT(m_visual_info.c_class == GrayScale);

    VisualID visual = m_visual_info.visualid;
    x11::ColormapWrapper colormap_owner;

    // Look for a standard colormap
    XStandardColormap params = {};
    constexpr bool is_gray = true;
    if (find_nondecomposed_standard_colormap(params, is_gray)) { // Throws
        colormap_owner.set_unowned(params.colormap);
        m_logger.detail("Found suitable standard colormap (%s) for GrayScale visual (%s): max = %s, mult = %s, "
                        "base_pixel = %s", core::as_flex_int_h(params.colormap), core::as_flex_int_h(visual),
                        core::as_int(params.red_max), core::as_int(params.red_mult),
                        core::as_int(params.base_pixel)); // Throws
        x11::MultFields fields(params);
        core::Slab<color_index_type> perm; // No permutation
        return create_multfield_1<is_gray>(fields, std::move(perm), std::move(colormap_owner),
                                           format, error); // Throws
    }

    // Look to use default colormap
    if (m_prefer_default_nondecomposed_colormap) {
        constexpr int max_colormap_size = 4096;
        static_assert(core::can_int_cast<color_index_type>(max_colormap_size - 1));
        constexpr int num_levels = 16;
        int colormap_size = m_visual_info.colormap_size;
        if (ARCHON_LIKELY(colormap_size <= max_colormap_size && colormap_size >= num_levels)) {
            Colormap colormap = {};
            if (m_colormap_finder.find_default_colormap(visual, colormap)) {
                // Assumption: For a GrayScale visual, XAllocColor() first computes the
                // closest representable gray-level that could be stored in a colormap
                // entry.  Then, if that gray-level is already in the colormap, that
                // gray-level is returned. Otherwise, if there is room in the colormap for
                // another entry, a new gray-level is allocated, initialized, and
                // returned. Otherwise allocation fails.
                core::Slab<color_index_type> perm(num_levels, {}); // Throws
                using ulong = unsigned long;
                constexpr int n = 32;
                constexpr int m = 16;
                static_assert(num_levels - 1 <= core::int_mask<ulong>(n));
                XColor color = {};
                namespace uf = util::unit_frac;
                int i = 0;
                while (i < num_levels) {
                    // FIXME: Is this the proper scaling scheme?                 
                    color.red = uf::int_to_int_a<n, m>(i, num_levels - 1, ulong(65535));
                    color.green = color.red;
                    color.blue  = color.red;
                    Status status = XAllocColor(m_dpy, colormap, &color);
                    if (ARCHON_LIKELY(status == 0))
                        goto fail;
                    ulong compound = ulong(i);
                    ARCHON_ASSERT(core::can_int_cast<color_index_type>(color.pixel));
                    perm[compound] = color_index_type(color.pixel);
                    ++i;
                }
                colormap_owner.set_unowned(colormap);
                m_logger.detail("Using default colormap (%s) for GrayScale visual (%s): num_levels = %s",
                                core::as_flex_int_h(colormap), core::as_flex_int_h(visual),
                                core::as_int(num_levels)); // Throws
                {
                    x11::MultFields fields(num_levels);
                    return create_multfield_1<is_gray>(fields, std::move(perm), std::move(colormap_owner),
                                                       format, error); // Throws
                }

              fail:
                for (int j = 0; j < i; ++j) {
                    ulong pixel = ulong((i - 1) - j);
                    ulong pixels[] = { pixel };
                    int npixels = 1;
                    ulong planes = 0;
                    XFreeColors(m_dpy, colormap, pixels, npixels, planes);
                }
            }
        }
    }

    // Create and set up a new colormap
    Colormap colormap = XCreateColormap(m_dpy, m_root, m_visual_info.visual, AllocAll);
    colormap_owner.set_owned(m_dpy, colormap);
    x11::setup_standard_grayscale_colormap(m_dpy, colormap, m_visual_info.depth, m_visual_info.colormap_size,
                                            m_weird); // Throws
    m_logger.detail("New colormap (%s) created for GrayScale visual (%s): depth = %s", core::as_flex_int_h(colormap),
                    core::as_flex_int_h(visual), core::as_int(m_visual_info.depth)); // Throws
    x11::BitFields fields = {};
    fields.red_width = m_visual_info.depth;
    return create_bitfield_1<is_gray>(fields, std::move(colormap_owner), format, error); // Throws
}


bool PixelFormatCreator::create_staticcolor(std::unique_ptr<x11::PixelFormat>& format, Error& error) const
{
    // Assumption: For a StaticColor visual, XAllocColor finds the closest color in the
    // colormap. Allocation never fails, but the returned color can be very different from
    // the requested one.

    ARCHON_ASSERT(m_visual_info.c_class == StaticColor);

    VisualID visual = m_visual_info.visualid;
    x11::ColormapWrapper colormap_owner;

    // Look for a standard colormap
    XStandardColormap params = {};
    constexpr bool is_gray = false;
    if (find_nondecomposed_standard_colormap(params, is_gray)) { // Throws
        colormap_owner.set_unowned(params.colormap);
        m_logger.detail("Found suitable standard colormap (%s) for StaticColor visual (%s): red_max = %s, "
                        "red_mult = %s, green_max = %s, green_mult = %s, blue_max = %s, blue_mult = %s, "
                        "base_pixel = %s", core::as_flex_int_h(params.colormap), core::as_flex_int_h(visual),
                        core::as_int(params.red_max), core::as_int(params.red_mult), core::as_int(params.green_max),
                        core::as_int(params.green_mult), core::as_int(params.blue_max), core::as_int(params.blue_mult),
                        core::as_int(params.base_pixel)); // Throws
        x11::MultFields fields(params);
        core::Slab<color_index_type> perm; // No permutation
        return create_multfield_1<is_gray>(fields, std::move(perm), std::move(colormap_owner),
                                           format, error); // Throws
    }

    // Use default colormap if possible, else create new colormap
    Colormap colormap = {};
    if (m_colormap_finder.find_default_colormap(visual, colormap)) {
        colormap_owner.set_unowned(colormap);
        m_logger.detail("Using default colormap (%s) for StaticColor visual (%s)", core::as_flex_int_h(colormap),
                        core::as_flex_int_h(visual)); // Throws
    }
    else {
        colormap = XCreateColormap(m_dpy, m_root, m_visual_info.visual, AllocNone);
        colormap_owner.set_owned(m_dpy, colormap);
        m_logger.detail("New colormap (%s) created for StaticColor visual (%s)", core::as_flex_int_h(colormap),
                        core::as_flex_int_h(visual)); // Throws
    }

    std::unique_ptr<image::Image> palette;
    if (ARCHON_LIKELY(create_palette_1<is_gray>(colormap, palette, error))) // Throws
        return create_indexed_1(std::move(palette), std::move(colormap_owner), format, error); // Throws
    return false;
}


bool PixelFormatCreator::create_pseudocolor(std::unique_ptr<x11::PixelFormat>& format, Error& error) const
{
    ARCHON_ASSERT(m_visual_info.c_class == PseudoColor);

    VisualID visual = m_visual_info.visualid;
    x11::ColormapWrapper colormap_owner;

    // Look for a standard colormap
    XStandardColormap params = {};
    constexpr bool is_gray = false;
    if (find_nondecomposed_standard_colormap(params, is_gray)) { // Throws
        colormap_owner.set_unowned(params.colormap);
        m_logger.detail("Found suitable standard colormap (%s) for PseudoColor visual (%s): red_max = %s, "
                        "red_mult = %s, green_max = %s, green_mult = %s, blue_max = %s, blue_mult = %s, "
                        "base_pixel = %s", core::as_flex_int_h(params.colormap), core::as_flex_int_h(visual),
                        core::as_int(params.red_max), core::as_int(params.red_mult), core::as_int(params.green_max),
                        core::as_int(params.green_mult), core::as_int(params.blue_max), core::as_int(params.blue_mult),
                        core::as_int(params.base_pixel)); // Throws
        x11::MultFields fields(params);
        core::Slab<color_index_type> perm; // No permutation
        return create_multfield_1<is_gray>(fields, std::move(perm), std::move(colormap_owner),
                                           format, error); // Throws
    }

    // Look to use default colormap
    if (m_prefer_default_nondecomposed_colormap) {
        constexpr int max_colormap_size = 4096;
        static_assert(core::can_int_cast<color_index_type>(max_colormap_size - 1));
        constexpr int num_red   = 4;
        constexpr int num_green = 4;
        constexpr int num_blue  = 4;
        constexpr int num_colors = num_red * num_green * num_blue;
        int colormap_size = m_visual_info.colormap_size;
        if (ARCHON_LIKELY(colormap_size <= max_colormap_size && colormap_size >= num_colors)) {
            Colormap colormap = {};
            if (m_colormap_finder.find_default_colormap(visual, colormap)) {
                // Assumption: For a PseudoColor visual, XAllocColor() first computes the
                // closest representable color that could be stored in a colormap entry.
                // Then, if that color is already in the colormap, that color is
                // returned. Otherwise, if there is room in the colormap for another entry,
                // a new color is allocated, initialized, and returned. Otherwise allocation
                // fails.
                x11::MultFields fields(num_red, num_green, num_blue);
                core::Slab<color_index_type> perm(num_colors, {}); // Throws
                using ulong = unsigned long;
                constexpr int n = 32;
                constexpr int m = 16;
                static_assert(num_red   - 1 <= core::int_mask<ulong>(n));
                static_assert(num_green - 1 <= core::int_mask<ulong>(n));
                static_assert(num_blue  - 1 <= core::int_mask<ulong>(n));
                XColor color = {};
                namespace uf = util::unit_frac;
                int i = 0;
                for (int r = 0; r < num_red; ++r) {
                    // FIXME: Is this the proper scaling scheme?                 
                    color.red = uf::int_to_int_a<n, m>(r, num_red - 1, ulong(65535));
                    for (int g = 0; g < num_green; ++g) {
                        color.green = uf::int_to_int_a<n, m>(g, num_green - 1, ulong(65535));
                        for (int b = 0; b < num_blue; ++b) {
                            color.blue = uf::int_to_int_a<n, m>(b, num_blue - 1, ulong(65535));
                            Status status = XAllocColor(m_dpy, colormap, &color);
                            if (ARCHON_LIKELY(status == 0))
                                goto fail;
                            ulong compound = fields.pack(ulong(r), ulong(g), ulong(b));
                            ARCHON_ASSERT(core::can_int_cast<color_index_type>(color.pixel));
                            perm[compound] = color_index_type(color.pixel);
                            ++i;
                        }
                    }
                }
                colormap_owner.set_unowned(colormap);
                m_logger.detail("Using default colormap (%s) for PseudoColor visual (%s): num_red = %s, "
                                "num_green = %s, num_blue = %s", core::as_flex_int_h(colormap),
                                core::as_flex_int_h(visual), core::as_int(num_red), core::as_int(num_green),
                                core::as_int(num_blue)); // Throws
                return create_multfield_1<is_gray>(fields, std::move(perm), std::move(colormap_owner),
                                                   format, error); // Throws

              fail:
                for (int j = 0; j < i; ++j) {
                    ulong pixel = ulong((i - 1) - j);
                    ulong pixels[] = { pixel };
                    int npixels = 1;
                    ulong planes = 0;
                    XFreeColors(m_dpy, colormap, pixels, npixels, planes);
                }
            }
        }
    }

    // Create and set up a new colormap
    Colormap colormap = XCreateColormap(m_dpy, m_root, m_visual_info.visual, AllocAll);
    colormap_owner.set_owned(m_dpy, colormap);
    x11::BitFields fields = {};
    x11::setup_standard_pseudocolor_colormap(m_dpy, colormap, m_visual_info.depth, m_visual_info.colormap_size,
                                              fields, m_weird); // Throws
    m_logger.detail("New colormap (%s) created for PseudoColor visual (%s): red_shift = %s, red_width = %s, "
                    "green_shift = %s, green_width = %s, blue_shift = %s, blue_width = %s",
                    core::as_flex_int_h(colormap), core::as_flex_int_h(visual), core::as_int(fields.red_shift),
                    core::as_int(fields.red_width), core::as_int(fields.green_shift), core::as_int(fields.green_width),
                    core::as_int(fields.blue_shift), core::as_int(fields.blue_width)); // Throws
    return create_bitfield_1<is_gray>(fields, std::move(colormap_owner), format, error); // Throws
}


bool PixelFormatCreator::create_truecolor(const x11::BitFields& fields, std::unique_ptr<x11::PixelFormat>& format,
                                          Error& error, bool fake) const
{
    // This function is used for TrueColor and fake TrueColor visuals. A fake TrueColor
    // visual is a StaticColor visual with channel masks (yes, it is weird).
    int class_ = m_visual_info.c_class;
    ARCHON_ASSERT((class_ == StaticColor && fake) || class_ == TrueColor);

    VisualID visual = m_visual_info.visualid;
    x11::ColormapWrapper colormap_owner;
    constexpr bool is_gray = false;

    // Look to use default colormap
    Colormap colormap = {};
    if (m_colormap_finder.find_default_colormap(visual, colormap)) {
        colormap_owner.set_unowned(colormap);
        m_logger.detail("Using default colormap (%s) for %s visual (%s)", core::as_flex_int_h(colormap),
                        x11::get_visual_class_name(class_), core::as_flex_int_h(visual)); // Throws
        return create_bitfield_1<is_gray>(fields, std::move(colormap_owner), format, error); // Throws
    }

    // Look for a standard colormap
    XStandardColormap params = {};
    if (find_decomposed_standard_colormap(fields, params)) { // Throws
        colormap_owner.set_unowned(params.colormap);
        m_logger.detail("Found suitable standard colormap (%s) for %s visual (%s): red_max = %s, red_mult = %s, "
                        "green_max = %s, green_mult = %s, blue_max = %s, blue_mult = %s, base_pixel = %s",
                        core::as_flex_int_h(params.colormap), x11::get_visual_class_name(class_),
                        core::as_flex_int_h(visual), core::as_int(params.red_max), core::as_int(params.red_mult),
                        core::as_int(params.green_max), core::as_int(params.green_mult), core::as_int(params.blue_max),
                        core::as_int(params.blue_mult), core::as_int(params.base_pixel)); // Throws
        return create_bitfield_1<is_gray>(fields, std::move(colormap_owner), format, error); // Throws
    }

    // Create and set up a new colormap
    colormap = XCreateColormap(m_dpy, m_root, m_visual_info.visual, AllocNone);
    colormap_owner.set_owned(m_dpy, colormap);
    m_logger.detail("New colormap (%s) created for %s visual (%s)", core::as_flex_int_h(colormap),
                    x11::get_visual_class_name(class_), core::as_flex_int_h(visual)); // Throws
    return create_bitfield_1<is_gray>(fields, std::move(colormap_owner), format, error); // Throws
}


bool PixelFormatCreator::create_directcolor(const x11::BitFields& fields, std::unique_ptr<x11::PixelFormat>& format,
                                            Error& error) const
{
    ARCHON_ASSERT(m_visual_info.c_class == DirectColor);

    VisualID visual = m_visual_info.visualid;
    x11::ColormapWrapper colormap_owner;
    constexpr bool is_gray = false;

    // FIXME: Should the default colormap be used if the selected visual is the default
    // visual? So far, no attempt is made to reuse the default colormap for a DirectColor
    // visual, because the assumption is that we do not know how it has been initialized. In
    // particular, we do not know how many entries have been allocated.                   

    // Look for a standard colormap
    XStandardColormap params = {};
    if (find_decomposed_standard_colormap(fields, params)) { // Throws
        colormap_owner.set_unowned(params.colormap);
        m_logger.detail("Found suitable standard colormap (%s) for DirectColor visual (%s): red_max = %s, "
                        "red_mult = %s, green_max = %s, green_mult = %s, blue_max = %s, blue_mult = %s, "
                        "base_pixel = %s", core::as_flex_int_h(params.colormap), core::as_flex_int_h(visual),
                        core::as_int(params.red_max), core::as_int(params.red_mult), core::as_int(params.green_max),
                        core::as_int(params.green_mult), core::as_int(params.blue_max), core::as_int(params.blue_mult),
                        core::as_int(params.base_pixel)); // Throws
        return create_bitfield_1<is_gray>(fields, std::move(colormap_owner), format, error); // Throws
    }

    // Create and set up a new colormap
    Colormap colormap = XCreateColormap(m_dpy, m_root, m_visual_info.visual, AllocAll);
    colormap_owner.set_owned(m_dpy, colormap);
    x11::init_directcolor_colormap(m_dpy, colormap, fields, m_visual_info.colormap_size, m_weird); // Throws
    m_logger.detail("New colormap (%s) created for DirectColor visual (%s): red_shift = %s, red_width = %s, "
                    "green_shift = %s, green_width = %s, blue_shift = %s, blue_width = %s",
                    core::as_flex_int_h(colormap), core::as_flex_int_h(visual), core::as_int(fields.red_shift),
                    core::as_int(fields.red_width), core::as_int(fields.green_shift), core::as_int(fields.green_width),
                    core::as_int(fields.blue_shift), core::as_int(fields.blue_width)); // Throws
    return create_bitfield_1<is_gray>(fields, std::move(colormap_owner), format, error); // Throws
}


template<bool G>
bool PixelFormatCreator::create_multfield_1(const x11::MultFields& fields, core::Slab<color_index_type> perm,
                                            x11::ColormapWrapper colormap, std::unique_ptr<x11::PixelFormat>& format,
                                            Error& error) const
{
    // FIXME: Check whether mult-fields can be expressed as bit-fields. If so, reroute to create_bitfield_1()                                      

    switch (m_pixmap_format.bits_per_pixel) {
        case 1:
            return create_multfield_2<G, 1>(fields, std::move(perm), std::move(colormap), format, error); // Throws
        case 4:
            return create_multfield_2<G, 4>(fields, std::move(perm), std::move(colormap), format, error); // Throws
        case 8:
            return create_multfield_2<G, 8>(fields, std::move(perm), std::move(colormap), format, error); // Throws
        case 16:
            return create_multfield_2<G, 16>(fields, std::move(perm), std::move(colormap), format, error); // Throws
        case 24:
            return create_multfield_2<G, 24>(fields, std::move(perm), std::move(colormap), format, error); // Throws
        case 32:
            return create_multfield_2<G, 32>(fields, std::move(perm), std::move(colormap), format, error); // Throws
    }
    throw std::runtime_error("Unexpected number of bits per pixel in pixmap format");
}


template<bool G, int N>
bool PixelFormatCreator::create_multfield_2(const x11::MultFields& fields, core::Slab<color_index_type> perm,
                                            x11::ColormapWrapper colormap, std::unique_ptr<x11::PixelFormat>& format,
                                            Error& error) const
{
    constexpr bool is_gray = G;
    constexpr int bits_per_pixel = N;

    // FIXME: Add support for 1 and 4 bits per pixel          

    if constexpr (bits_per_pixel >= 8) {
        using compound_type = core::fast_unsigned_int_type<bits_per_pixel>;
        static_assert(!std::is_same_v<compound_type, void>);

        static_assert(bits_per_pixel % 8 == 0);
        constexpr int bytes_per_pixel = bits_per_pixel / 8;
        using format_type = MultFieldPixelFormat<is_gray, compound_type, bytes_per_pixel>;

        format = std::make_unique<format_type>(m_dpy, m_visual_info, m_pixmap_format, fields, std::move(perm),
                                               std::move(colormap)); // Throws
        return true;
    }
    else {
        error = Error::unsupported_bits_per_pixel;
        return false;
    }
}


template<bool G>
bool PixelFormatCreator::create_bitfield_1(const x11::BitFields& fields, x11::ColormapWrapper colormap,
                                           std::unique_ptr<x11::PixelFormat>& format, Error& error) const
{
    switch (m_pixmap_format.bits_per_pixel) {
        case 1:
            return create_bitfield_2<G, 1>(fields, std::move(colormap), format, error); // Throws
        case 4:
            return create_bitfield_2<G, 4>(fields, std::move(colormap), format, error); // Throws
        case 8:
            return create_bitfield_2<G, 8>(fields, std::move(colormap), format, error); // Throws
        case 16:
            return create_bitfield_2<G, 16>(fields, std::move(colormap), format, error); // Throws
        case 24:
            return create_bitfield_2<G, 24>(fields, std::move(colormap), format, error); // Throws
        case 32:
            return create_bitfield_2<G, 32>(fields, std::move(colormap), format, error); // Throws
    }
    throw std::runtime_error("Unexpected number of bits per pixel in pixmap format");
}


template<bool G, int N>
bool PixelFormatCreator::create_bitfield_2(const x11::BitFields& fields, x11::ColormapWrapper colormap,
                                           std::unique_ptr<x11::PixelFormat>& format, Error& error) const
{
    constexpr bool is_gray = G;
    constexpr int bits_per_pixel = N;

    if constexpr (is_gray) {
        if constexpr (bits_per_pixel >= 8) {
            if (norm_mask_match<image::ChannelPacking_8>(fields)) {
                constexpr bool reverse_channel_order = false;
                return create_bitfield_3<G, N, image::ChannelPacking_8,
                                         reverse_channel_order>(std::move(colormap), format, error); // Throws
            }
        }

        // FIXME: Add support for any channel masks by using a more general form of packing specification     

        // FIXME: Add support for 1 bit per pixel?            

        // FIXME: Add support for 4 bits per pixel (which is two pixels per byte)?            

        error = Error::unsupported_depth;
        return false;
    }
    else {
        if constexpr (bits_per_pixel >= 24) {
            if (norm_mask_match<image::ChannelPacking_888>(fields)) {
                constexpr bool reverse_channel_order = false;
                return create_bitfield_3<G, N, image::ChannelPacking_888,
                                         reverse_channel_order>(std::move(colormap), format, error); // Throws
            }
            if (rev_mask_match<image::ChannelPacking_888>(fields)) {
                constexpr bool reverse_channel_order = true;
                return create_bitfield_3<G, N, image::ChannelPacking_888,
                                         reverse_channel_order>(std::move(colormap), format, error); // Throws
            }
        }

        if constexpr (bits_per_pixel >= 16) {
            if (norm_mask_match<image::ChannelPacking_565>(fields)) {
                constexpr bool reverse_channel_order = false;
                return create_bitfield_3<G, N, image::ChannelPacking_565,
                                         reverse_channel_order>(std::move(colormap), format, error); // Throws
            }
            if (rev_mask_match<image::ChannelPacking_565>(fields)) {
                constexpr bool reverse_channel_order = true;
                return create_bitfield_3<G, N, image::ChannelPacking_565,
                                         reverse_channel_order>(std::move(colormap), format, error); // Throws
            }
            if (norm_mask_match<image::ChannelPacking_555>(fields)) {
                constexpr bool reverse_channel_order = false;
                return create_bitfield_3<G, N, image::ChannelPacking_555,
                                         reverse_channel_order>(std::move(colormap), format, error); // Throws
            }
            if (rev_mask_match<image::ChannelPacking_555>(fields)) {
                constexpr bool reverse_channel_order = true;
                return create_bitfield_3<G, N, image::ChannelPacking_555,
                                         reverse_channel_order>(std::move(colormap), format, error); // Throws
            }
        }

        if constexpr (bits_per_pixel >= 8) {
            if (norm_mask_match<image::ChannelPacking_332>(fields)) {
                constexpr bool reverse_channel_order = false;
                return create_bitfield_3<G, N, image::ChannelPacking_332,
                                         reverse_channel_order>(std::move(colormap), format, error); // Throws
            }
            if (rev_mask_match<image::ChannelPacking_233>(fields)) {
                constexpr bool reverse_channel_order = true;
                return create_bitfield_3<G, N, image::ChannelPacking_233,
                                         reverse_channel_order>(std::move(colormap), format, error); // Throws
            }
        }

        // FIXME: Add support for any channel masks by using a more general form of packing specification     

        // FIXME: Add support for 4 bits per pixel (which is two pixels per byte)?            

        error = Error::unsupported_channel_masks;
        return false;
    }
}


template<bool G, int N, class P, bool R>
bool PixelFormatCreator::create_bitfield_3(x11::ColormapWrapper colormap, std::unique_ptr<x11::PixelFormat>& format,
                                           Error&) const
{
    constexpr bool is_gray = G;
    constexpr int bits_per_pixel = N;
    using packing_type = P;
    constexpr bool reverse_channel_order = R;

    // FIXME: It is weird that the compound type expected by image::PackedPixelFormat is the packed, and not the unpacked type                  
    static_assert(bits_per_pixel <= 32);
    using compound_type = std::conditional_t<bits_per_pixel <= 8, image::int8_type,
                                             std::conditional_t<bits_per_pixel <= 16, image::int16_type,
                                                                image::int32_type>>;

    static_assert(bits_per_pixel % 8 == 0);
    constexpr int bytes_per_pixel = bits_per_pixel / 8;
    using format_type = BitFieldPixelFormat<is_gray, compound_type, packing_type, bytes_per_pixel,
                                            reverse_channel_order>;

    format = std::make_unique<format_type>(m_dpy, m_visual_info, m_pixmap_format, std::move(colormap)); // Throws
    return true;
}


template<bool G>
bool PixelFormatCreator::create_palette_1(Colormap colormap, std::unique_ptr<image::Image>& palette,
                                          Error& error) const
{
    // X11 does not allow for colormap entries to be read with more than 16 bits per channel
    if (m_visual_info.bits_per_rgb <= 8)
        return create_palette_2<G, image::CompRepr::int8>(colormap, palette, error); // Throws
    return create_palette_2<G, image::CompRepr::int16>(colormap, palette, error); // Throws
}


template<bool G, image::CompRepr R>
bool PixelFormatCreator::create_palette_2(Colormap colormap, std::unique_ptr<image::Image>& palette, Error&) const
{
    constexpr bool is_gray = G;
    constexpr image::CompRepr comp_repr = R;

    std::size_t size = {};
    core::int_cast(m_visual_info.colormap_size, size); // Throws
    using pixel_repr_type = std::conditional_t<is_gray, image::Lum<comp_repr>, image::RGB<comp_repr>>;
    using pixel_type = image::Pixel<pixel_repr_type>;
    auto colors = std::make_unique<pixel_type[]>(size); // Throws
    fetch_colormap(m_dpy, colormap, core::Span(colors.get(), size)); // Throws
    using image_type = image::PaletteImage<pixel_repr_type>;
    palette = std::make_unique<image_type>(std::move(colors), size); // Throws
    return true; // Success
}


bool PixelFormatCreator::create_indexed_1(std::unique_ptr<image::Image> palette, x11::ColormapWrapper colormap,
                                          std::unique_ptr<x11::PixelFormat>& format, Error& error) const
{
    switch (m_pixmap_format.bits_per_pixel) {
        case 1:
            return create_indexed_2<1>(std::move(palette), std::move(colormap), format, error); // Throws
        case 4:
            return create_indexed_2<4>(std::move(palette), std::move(colormap), format, error); // Throws
        case 8:
            return create_indexed_2<8>(std::move(palette), std::move(colormap), format, error); // Throws
        case 16:
            return create_indexed_2<16>(std::move(palette), std::move(colormap), format, error); // Throws
        case 24:
            return create_indexed_2<24>(std::move(palette), std::move(colormap), format, error); // Throws
        case 32:
            return create_indexed_2<32>(std::move(palette), std::move(colormap), format, error); // Throws
    }
    throw std::runtime_error("Unexpected number of bits per pixel in pixmap format");
}


template<int N>
bool PixelFormatCreator::create_indexed_2(std::unique_ptr<image::Image> palette, x11::ColormapWrapper colormap,
                                          std::unique_ptr<x11::PixelFormat>& format, Error& error) const
{
    if (m_visual_info.colormap_size <= 256) {
        return create_indexed_3<N, image::CompRepr::int8>(std::move(palette), std::move(colormap),
                                                          format, error); // Throws
    }

    // FIXME: Expand with cases for larger colormaps when support for varying index representation scheme is added       
    error = Error::unsupported_colormap_size;
    return false;
}


template<int N, image::CompRepr R>
bool PixelFormatCreator::create_indexed_3(std::unique_ptr<image::Image> palette, x11::ColormapWrapper colormap,
                                          std::unique_ptr<x11::PixelFormat>& format, Error& error) const
{
    constexpr int bits_per_pixel = N;
    constexpr image::CompRepr index_repr = R;

    // FIXME: Support 1 and 4 bits per pixel          
    if constexpr (bits_per_pixel >= 8) {
        static_assert(bits_per_pixel % 8 == 0);
        constexpr int bytes_per_pixel = bits_per_pixel / 8;
        static_assert(index_repr == image::color_index_repr);
        using format_type = IndexedPixelFormat<bytes_per_pixel>;
        format = std::make_unique<format_type>(m_dpy, m_visual_info, m_pixmap_format, std::move(palette),
                                               std::move(colormap)); // Throws
        return true;
    }
    else {
        error = Error::unsupported_bits_per_pixel;
        return false;
    }
}


bool PixelFormatCreator::find_nondecomposed_standard_colormap(XStandardColormap& params, bool is_gray) const
{
    VisualID visual = m_visual_info.visualid;
    XStandardColormap params_2;
    if (m_colormap_finder.find_standard_colormap(visual, params_2)) { // Throws
        ARCHON_ASSERT(params_2.visualid == visual);
        x11::MultFields fields(params_2);
        MultFieldsDigest digest(fields);
        if (ARCHON_LIKELY(digest.is_valid_and_compact(fields.offset, m_visual_info.colormap_size, is_gray))) {
            if (ARCHON_LIKELY(digest.is_confined_to_depth(m_visual_info.depth))) {
                params = params_2;
                return true;
            }
        }
        m_logger.warn("Ignoring invalid (or unsupported) standard colormap (%s) for %s visual (%s)",
                      core::as_flex_int_h(params_2.colormap), x11::get_visual_class_name(m_visual_info.c_class),
                      core::as_flex_int_h(visual)); // Throws
    }
    return false;
}


bool PixelFormatCreator::find_decomposed_standard_colormap(const x11::BitFields& fields,
                                                           XStandardColormap& params) const
{
    VisualID visual = m_visual_info.visualid;
    XStandardColormap params_2;
    if (m_colormap_finder.find_standard_colormap(visual, params_2)) { // Throws
        ARCHON_ASSERT(params_2.visualid == visual);
        // FIXME: Strangely, when I use `xstdcmap -default` to set the standard colormaps,
        // and the server is Xorg with 24-bit visuals (8 bits for each channel), I get a
        // standard colormap for a DirectColor visual where `max` values are 127, and not
        // 255 as I expected. Is this a malfunction of `xstdcmap -default`, or is there some
        // kind of reason behind it?                
        bool is_gray = false;
        if (ARCHON_LIKELY(x11::MultFields(params_2) == x11::MultFields(fields, is_gray))) {
            params = params_2;
            return true;
        }
        m_logger.warn("Ignoring invalid (or unsupported) standard colormap (%s) for %s visual (%s)",
                      core::as_flex_int_h(params_2.colormap), x11::get_visual_class_name(m_visual_info.c_class),
                      core::as_flex_int_h(visual)); // Throws
    }
    return false;
}


template<class P> inline bool PixelFormatCreator::norm_mask_match(const x11::BitFields& fields) noexcept
{
    bool reverse = false;
    return mask_match<P>(fields, reverse);
}


template<class P> inline bool PixelFormatCreator::rev_mask_match(const x11::BitFields& fields) noexcept
{
    bool reverse = true;
    return mask_match<P>(fields, reverse);
}


template<class P> bool PixelFormatCreator::mask_match(const x11::BitFields& fields, bool reverse) noexcept
{
    using packing_type = P;

    constexpr int n = packing_type::num_fields;
    auto match = [](int shift, int width, int i) noexcept {
        return (shift == image::get_bit_field_shift(packing_type::fields, n, i) &&
                width == image::get_bit_field_width(packing_type::fields, n, i));
    };

    if constexpr (n == 1) {
        static_cast<void>(reverse);
        return (match(fields.red_shift, fields.red_width, 0) &&
                fields.green_shift == 0 && fields.green_width == 0 &&
                fields.blue_shift == 0 && fields.blue_width == 0);
    }
    else {
        static_assert(n == 3);
        int i_1 = 0, i_2 = 1, i_3 = 2;
        if (reverse)
            std::swap(i_1, i_3);
        return (match(fields.red_shift, fields.red_width, i_1) &&
                match(fields.green_shift, fields.green_width, i_2) &&
                match(fields.blue_shift, fields.blue_width, i_3));
    }
}


auto PixelFormatCreator::format_error_message(Error error) const -> std::string
{
    switch (error) {
        case Error::unsupported_bits_per_pixel:
            return core::format(m_locale, "Unsupported number of bits per pixel (%s) in pixmap format for %s visual "
                                "(%s) of depth %s", core::as_int(m_pixmap_format.bits_per_pixel),
                                x11::get_visual_class_name(m_visual_info.c_class),
                                core::as_flex_int_h(m_visual_info.visualid),
                                core::as_int(m_visual_info.depth)); // Throws
        case Error::unsupported_depth:
            return core::format(m_locale, "Unsupported depth (%s) of %s visual (%s)",
                                core::as_int(m_visual_info.depth),
                                x11::get_visual_class_name(m_visual_info.c_class),
                                core::as_flex_int_h(m_visual_info.visualid)); // Throws
        case Error::unsupported_channel_masks:
            return core::format(m_locale, "Unsupported channel masks (%s, %s, and %s) in %s visual (%s) of depth %s",
                                core::as_flex_int_h(m_visual_info.red_mask),
                                core::as_flex_int_h(m_visual_info.green_mask),
                                core::as_flex_int_h(m_visual_info.blue_mask),
                                x11::get_visual_class_name(m_visual_info.c_class),
                                core::as_flex_int_h(m_visual_info.visualid),
                                core::as_int(m_visual_info.depth)); // Throws
        case Error::unsupported_colormap_size:
            return core::format(m_locale, "Unsupported colormap size (%s) in %s visual (%s) of depth %s",
                                core::as_int(m_visual_info.colormap_size),
                                x11::get_visual_class_name(m_visual_info.c_class),
                                core::as_flex_int_h(m_visual_info.visualid),
                                core::as_int(m_visual_info.depth)); // Throws
    }
    ARCHON_ASSERT_UNREACHABLE();
    return {};
}


} // unnamed namespace



x11::TextPropertyWrapper::TextPropertyWrapper(Display* dpy, std::string_view str, const std::locale& loc)
{
    bool force_fallback = false;

#if HAVE_X11_UTF8

    auto set_utf8 = [&](char* data) {
        char* list[] = {
            data,
        };
        int count = std::size(list);
        XICCEncodingStyle style = XStdICCTextStyle;
        int ret = Xutf8TextListToTextProperty(dpy, list, count, style, &prop);
        if (ARCHON_LIKELY(ret >= 0))
            return;
        std::error_code ec = { ret, g_xutil_error_category };
        throw std::system_error(ec);
    };

    if (core::assume_utf8_locale(loc) && !force_fallback) { // Throws
        std::array<char, 256> seed_memory;
        core::Buffer buffer(seed_memory);
        std::size_t buffer_offset = 0;
        buffer.append(str, buffer_offset); // Throws
        buffer.append_a('\0', buffer_offset); // Throws
        set_utf8(buffer.data()); // Throws
        return;
    }

    if (core::assume_unicode_locale(loc) && !force_fallback) { // Throws
        std::array<wchar_t, 256> seed_memory_1;
        core::BasicStringDecoder<wchar_t> decoder(loc, seed_memory_1); // Throws
        std::basic_string_view str_2 = decoder.decode_sc(str); // Throws
        std::array<char, 256> seed_memory_2;
        core::Buffer buffer(seed_memory_2);
        std::size_t buffer_offset = 0;
        core::encode_utf8(core::StringSpan(str_2), buffer, buffer_offset); // Throws
        buffer.append_a('\0', buffer_offset); // Throws
        set_utf8(buffer.data()); // Throws
        return;
    }

#else // !HAVE_X11_UTF8

    static_cast<void>(dpy);

    auto set_latin_1 = [&](char* data) {
        char* list[] = {
            data,
        };
        int count = std::size(list);
        Status status = XStringListToTextProperty(list, count, &prop);
        if (ARCHON_LIKELY(status != 0))
            return;
        std::error_code ec = { XNoMemory, g_xutil_error_category };
        throw std::system_error(ec);
    };

    if (core::assume_utf8_locale(loc) && !force_fallback) { // Throws
        std::array<char32_t, 256> seed_memory_1;
        core::Buffer buffer_1(seed_memory_1);
        std::size_t buffer_offset = 0;
        core::decode_utf8(core::StringSpan(str), buffer_1, buffer_offset); // Throws
        std::basic_string_view str_2 = { buffer_1.data(), buffer_offset };
        std::array<char, 256> seed_memory_2;
        core::Buffer buffer_2(seed_memory_2);
        buffer_offset = 0;
        for (char32_t ch : str_2) {
            auto val = std::char_traits<char32_t>::to_int_type(ch);
            char ch_2 = (val < 0x100 ? std::char_traits<char>::to_char_type(int(val)) : '?');
            buffer_2.append_a(ch_2, buffer_offset); // Throws
        }
        buffer_2.append_a('\0', buffer_offset); // Throws
        set_latin_1(buffer_2.data()); // Throws
        return;
    }

    if (core::assume_unicode_locale(loc) && !force_fallback) { // Throws
        std::array<wchar_t, 256> seed_memory_1;
        core::Buffer buffer_1(seed_memory_1);
        core::BasicStringDecoder<wchar_t> decoder(loc, seed_memory_1); // Throws
        std::basic_string_view str_2 = decoder.decode_sc(str); // Throws
        std::array<char, 256> seed_memory_2;
        core::Buffer buffer_2(seed_memory_2);
        std::size_t buffer_offset = 0;
        for (wchar_t ch : str_2) {
            auto val = std::char_traits<wchar_t>::to_int_type(ch);
            char ch_2 = (val < 0x100 ? std::char_traits<char>::to_char_type(int(val)) : '?');
            buffer_2.append_a(ch_2, buffer_offset); // Throws
        }
        buffer_2.append_a('\0', buffer_offset); // Throws
        set_latin_1(buffer_2.data()); // Throws
        return;
    }

#endif // !HAVE_X11_UTF8

    auto set_mb = [&](char* data) {
        char* list[] = {
            data,
        };
        int count = std::size(list);
        XICCEncodingStyle style = XStdICCTextStyle;
        int ret = XmbTextListToTextProperty(dpy, list, count, style, &prop);
        if (ARCHON_LIKELY(ret >= 0))
            return;
        std::error_code ec = { ret, g_xutil_error_category };
        throw std::system_error(ec);
    };

    std::array<char, 256> seed_memory;
    core::Buffer buffer(seed_memory);
    std::size_t buffer_offset = 0;
    core::WideSimpleCharCodec codec(loc); // Throws
    std::size_t n = str.size();
    std::size_t i = 0;
    while (ARCHON_LIKELY(i < n)) {
        char ch = str[i];
        if (ARCHON_LIKELY(g_xlib_portable_char_set.has(ch))) {
            buffer.append_a(ch, buffer_offset); // Throws
            ++i;
            continue;
        }

        // Skip one logical character at a time
        std::mbstate_t state = {};
        bool error_continuation = false;
        for (;;) {
            bool end_of_input = true;
            wchar_t buffer_2[1];
            std::size_t buffer_offset_2 = 0;
            bool error = false;
            codec.decode(state, str, i, end_of_input, buffer_2, buffer_offset_2, error); // Throws
            if (ARCHON_LIKELY(!error)) {
                buffer.append_a('?', buffer_offset); // Throws
                if (ARCHON_LIKELY(std::mbsinit(&state)))
                    break;
                error_continuation = false;
                continue;
            }
            ARCHON_ASSERT(i < n);
            ++i;
            if (!error_continuation)
                buffer.append_a('?', buffer_offset); // Throws
            error_continuation = true;
        }
    }
    buffer.append_a('\0', buffer_offset); // Throws
    set_mb(buffer.data()); // Throws
}



auto x11::map_opt_visual_class(const std::optional<display::ConnectionConfigX11::VisualClass>& class_) noexcept ->
    std::optional<int>
{
    if (ARCHON_LIKELY(!class_.has_value()))
        return {};
    switch (class_.value()) {
        case display::ConnectionConfigX11::VisualClass::static_gray:
            return StaticGray;
        case display::ConnectionConfigX11::VisualClass::gray_scale:
            return GrayScale;
        case display::ConnectionConfigX11::VisualClass::static_color:
            return StaticColor;
        case display::ConnectionConfigX11::VisualClass::pseudo_color:
            return PseudoColor;
        case display::ConnectionConfigX11::VisualClass::true_color:
            return TrueColor;
        case display::ConnectionConfigX11::VisualClass::direct_color:
            return DirectColor;
    }
    ARCHON_ASSERT_UNREACHABLE();
    return {};
}


auto x11::get_visual_class_name(int class_) noexcept -> const char*
{
    switch (class_) {
        case StaticGray:
            return "StaticGray";
        case GrayScale:
            return "GrayScale";
        case StaticColor:
            return "StaticColor";
        case PseudoColor:
            return "PseudoColor";
        case TrueColor:
            return "TrueColor";
        case DirectColor:
            return "DirectColor";
    }
    ARCHON_ASSERT_UNREACHABLE();
    return nullptr;
}


auto x11::connect(std::optional<std::string_view> display, const std::locale& locale) -> x11::DisplayWrapper
{
    std::string_view display_2 = x11::get_display_string(display); // Throws
    x11::DisplayWrapper dpy_owner;
    if (ARCHON_LIKELY(x11::try_connect(display_2, dpy_owner))) // Throws
        return dpy_owner;
    std::string message = core::format(locale, "Failed to open X11 display connection to %s",
                                       core::quoted(display_2)); // Throws
    throw std::runtime_error(message);
}


auto x11::get_display_string(std::optional<std::string_view> display) -> std::string_view
{
    if (ARCHON_LIKELY(!display.has_value())) {
        char* val = std::getenv("DISPLAY");
        if (ARCHON_LIKELY(val))
            return std::string_view(val); // Throws
        return {};
    }
    return display.value();
}


bool x11::try_connect(std::string_view display, x11::DisplayWrapper& dpy_owner)
{
    std::string display_2 = std::string(display); // Throws
    Display* dpy = XOpenDisplay(display_2.data());
    if (ARCHON_LIKELY(dpy)) {
        dpy_owner.set(dpy);
        return true;
    }
    return false;
}


auto x11::init_extensions(Display* dpy) -> x11::ExtensionInfo
{
    x11::ExtensionInfo info = {};

#if HAVE_XDBE
    {
        int major = 0;
        int minor = 0;
        if (ARCHON_LIKELY(XdbeQueryExtension(dpy, &major, &minor))) {
            if (ARCHON_LIKELY(major >= 1)) {
                info.have_xdbe = true;
                info.xdbe_major = major;
                info.xdbe_minor = minor;
            }
        }
    }
#endif // HAVE_XDBE

    {
        int lib_major = XkbMajorVersion;
        int lib_minor = XkbMinorVersion;
        if (ARCHON_LIKELY(XkbLibraryVersion(&lib_major, &lib_minor))) {
            int opcode = 0;     // Unused
            int event_base = 0; // Unused
            int error_base = 0; // Unused
            int major = 0;
            int minor = 0;
            if (ARCHON_LIKELY(XkbQueryExtension(dpy, &opcode, &event_base, &error_base, &major, &minor))) {
                if (ARCHON_LIKELY(major >= 1)) {
                    info.have_xkb = true;
                    info.xkb_major = major;
                    info.xkb_minor = minor;
                }
            }
        }
    }

#if HAVE_XRANDR
    {
        int event_base = 0;
        int error_base = 0; // Unused
        if (ARCHON_LIKELY(XRRQueryExtension(dpy, &event_base, &error_base))) {
            int major = 0;
            int minor = 0;
            Status status = XRRQueryVersion(dpy, &major, &minor);
            if (ARCHON_UNLIKELY(status == 0))
                throw std::runtime_error("XRRQueryVersion() failed");
            if (ARCHON_LIKELY(major > 1 || (major == 1 && minor >= 5))) {
                info.have_xrandr = true;
                info.xrandr_event_base = event_base;
                info.xrandr_major = major;
                info.xrandr_minor = minor;
            }
        }
    }
#endif // HAVE_XRANDR

#if HAVE_XRENDER
    {
        int event_base = 0; // Unused
        int error_base = 0; // Unused
        if (ARCHON_LIKELY(XRenderQueryExtension(dpy, &event_base, &error_base))) {
            int major = 0;
            int minor = 0;
            Status status = XRenderQueryVersion(dpy, &major, &minor);
            if (ARCHON_UNLIKELY(status == 0))
                throw std::runtime_error("XRenderQueryVersion() failed");
            if (ARCHON_LIKELY(major > 0 || (major == 0 && minor >= 7))) {
                info.have_xrender = true;
                info.xrender_major = major;
                info.xrender_minor = minor;
            }
        }
    }
#endif // HAVE_XRENDER

#if HAVE_GLX
    {
        int error_base = 0; // Unused
        int event_base = 0; // Unused
        if (ARCHON_LIKELY(glXQueryExtension(dpy, &error_base, &event_base))) {
            int major = 0;
            int minor = 0;
            Bool success = glXQueryVersion(dpy, &major, &minor);
            if (ARCHON_UNLIKELY(!success))
                throw std::runtime_error("glXQueryVersion() failed");
            if (ARCHON_LIKELY(major > 1 || (major == 1 && minor >= 4))) {
                info.have_glx = true;
                info.glx_major = major;
                info.glx_minor = minor;
            }
        }
    }
#endif // HAVE_GLX

    return info;
}


bool x11::has_property(Display* dpy, ::Window win, Atom name)
{
    long offset = 0;
    long length = 0;
    Bool delete_ = False;
    Atom req_type = AnyPropertyType;
    Atom actual_type = {};
    int actual_format = {};
    unsigned long nitems = {};
    unsigned long bytes_after = {};
    unsigned char* prop = {};
    int ret = XGetWindowProperty(dpy, win, name, offset, length, delete_, req_type,
                                 &actual_type, &actual_format, &nitems, &bytes_after, &prop);
    if (ARCHON_LIKELY(ret == Success)) {
        ARCHON_SCOPE_EXIT {
            if (prop)
                XFree(prop);
        };
        bool found = (actual_type != None);
        return found;
    }
    throw std::runtime_error("XGetWindowProperty() failed");
}


auto x11::fetch_pixmap_formats(Display* dpy) -> core::FlatMap<int, XPixmapFormatValues>
{
    int n = 0;
    XPixmapFormatValues* entries = XListPixmapFormats(dpy, &n);
    if (!entries)
        throw std::runtime_error("XListPixmapFormats() failed");
    ARCHON_SCOPE_EXIT {
        XFree(entries);
    };
    core::FlatMap<int, XPixmapFormatValues> pixmap_formats;
    std::size_t n_2 = {};
    core::int_cast(n, n_2); // Throws
    pixmap_formats.reserve(n_2); // Throws
    pixmap_formats.clear();
    for (std::size_t i = 0; i < n_2; ++i) {
        XPixmapFormatValues& format = entries[i];
        pixmap_formats[format.depth] = format;
    }
    return pixmap_formats;
}


auto x11::fetch_standard_colormaps(Display* dpy, ::Window root) -> core::FlatMap<VisualID, XStandardColormap>
{
    // See command `xstdcmap` for a way to set up the standard colormaps
    core::FlatMap<VisualID, XStandardColormap> colormaps;
    XStandardColormap* std_colormap = {};
    int count = {};
    Status status = XGetRGBColormaps(dpy, root, &std_colormap, &count, XA_RGB_DEFAULT_MAP);
    if (status != 0) {
        ARCHON_SCOPE_EXIT {
            XFree(std_colormap);
        };
        std::size_t n = std::size_t(count);
        colormaps.reserve(n); // Throws
        for (std::size_t i = 0; i < n; ++i) {
            const XStandardColormap& entry = std_colormap[i];
            colormaps.emplace(entry.visualid, entry);
        }
    }
    return colormaps;
}


auto x11::load_visuals(Display* dpy, int screen, const x11::ExtensionInfo& extension_info) ->
    core::Slab<x11::VisualSpec>
{
    core::FlatMap<core::Pair<int, VisualID>, int> double_buffered_visuals;
#if HAVE_XDBE
    if (ARCHON_LIKELY(extension_info.have_xdbe)) {
        Window root = RootWindow(dpy, screen);
        Drawable screen_specifiers[] = {
            root,
        };
        int n = 1;
        XdbeScreenVisualInfo* entries = XdbeGetVisualInfo(dpy, screen_specifiers, &n);
        ARCHON_STEADY_ASSERT(entries);
        ARCHON_STEADY_ASSERT(n == 1);
        ARCHON_SCOPE_EXIT {
            XdbeFreeVisualInfo(entries);
        };
        const XdbeScreenVisualInfo& entry = entries[0];
        std::size_t n_2 = {};
        core::int_cast(entry.count, n_2); // Throws
        double_buffered_visuals.reserve(n_2); // Throws
        for (std::size_t i = 0; i < n_2; ++i) {
            const XdbeVisualInfo& subentry = entry.visinfo[i];
            auto p = double_buffered_visuals.emplace(core::Pair(subentry.depth, subentry.visual),
                                                     subentry.perflevel); // Throws
            bool was_inserted = p.second;
            ARCHON_ASSERT(was_inserted);
        }
    }
#else // !HAVE_XDBE
    static_cast<void>(extension_info);
#endif // !HAVE_XDBE

    core::Slab<x11::VisualSpec> visual_specs;
    int n = {};
    long vinfo_mask = VisualScreenMask;
    XVisualInfo vinfo_template = {};
    vinfo_template.screen = screen;
    XVisualInfo* entries = XGetVisualInfo(dpy, vinfo_mask, &vinfo_template, &n);
    if (ARCHON_LIKELY(entries)) {
        ARCHON_SCOPE_EXIT {
            XFree(entries);
        };
        std::size_t n_2 = {};
        core::int_cast(n , n_2); // Throws
        visual_specs.recreate(n_2); // Throws
        for (std::size_t i = 0; i < n_2; ++i) {
            XVisualInfo& info = entries[i];
            bool double_buffered = false;
            int double_buffered_perflevel = 0;
            {
                auto i = double_buffered_visuals.find(core::Pair(info.depth, info.visualid));
                if (i != double_buffered_visuals.end()) {
                    double_buffered = true;
                    double_buffered_perflevel = i->second;
                }
            }
            bool opengl_supported = false;
            int opengl_level = 0;
            bool opengl_double_buffered = false;
            bool opengl_stereo = false;
            int opengl_num_aux_buffers = 0;
            int opengl_depth_buffer_bits = 0;
            int opengl_stencil_buffer_bits = 0;
            int opengl_accum_buffer_bits = 0;
#if HAVE_GLX
            if (ARCHON_LIKELY(extension_info.have_glx)) {
                auto get = [&](int attrib) {
                    int value = {};
                    int ret = glXGetConfig(dpy, &info, attrib, &value);
                    if (ARCHON_UNLIKELY(ret != 0))
                        throw std::runtime_error("glXGetConfig() failed");
                    return value;
                };
                if (get(GLX_USE_GL) != 0) { // Throws
                    opengl_supported           = true;
                    opengl_level               = get(GLX_LEVEL); // Throws
                    opengl_double_buffered     = (get(GLX_DOUBLEBUFFER) != 0); // Throws
                    opengl_stereo              = (get(GLX_STEREO) != 0); // Throws
                    opengl_num_aux_buffers     = get(GLX_AUX_BUFFERS); // Throws
                    opengl_depth_buffer_bits   = get(GLX_DEPTH_SIZE); // Throws
                    opengl_stencil_buffer_bits = get(GLX_STENCIL_SIZE); // Throws
                    opengl_accum_buffer_bits   = (get(GLX_ACCUM_RED_SIZE) + get(GLX_ACCUM_GREEN_SIZE) +
                                                  get(GLX_ACCUM_BLUE_SIZE) + get(GLX_ACCUM_ALPHA_SIZE)); // Throws
                }
            }
#else // !HAVE_GLX
    static_cast<void>(extension_info);
#endif // !HAVE_GLX
            x11::VisualSpec spec = {
                info,
                double_buffered,
                opengl_supported,
                opengl_double_buffered,
                opengl_stereo,
                double_buffered_perflevel,
                opengl_level,
                opengl_num_aux_buffers,
                opengl_depth_buffer_bits,
                opengl_stencil_buffer_bits,
                opengl_accum_buffer_bits,
            };
            visual_specs.add(spec);
        }
    }

    return visual_specs;
}


bool x11::find_visual(Display* dpy, int screen, core::Span<const x11::VisualSpec> visual_specs,
                      const x11::FindVisualParams& params, std::size_t& index)
{
    VisualFinder finder(dpy, screen, visual_specs); // Throws
    return finder.find(params, index); // Throws
}


auto x11::find_visuals(Display* dpy, int screen, core::Span<const x11::VisualSpec> visual_specs,
                       const x11::FindVisualParams& params, core::Buffer<std::size_t>& indexes) -> std::size_t
{
    VisualFinder finder(dpy, screen, visual_specs); // Throws
    return finder.find_all(params, indexes); // Throws
}


auto x11::record_bit_fields(const XVisualInfo& visual_info) -> x11::BitFields
{
    x11::BitFields fields = {};
    if (ARCHON_LIKELY(try_record_bit_fields(visual_info, fields)))
        return fields;
    throw std::runtime_error("Bad channel mask in visual info");
}


void x11::init_ximage(Display* dpy, XImage& img, const XVisualInfo& visual_info,
                      const XPixmapFormatValues& pixmap_format, int byte_order,
                      const display::Size& size, char* buffer)
{
    // Xlib requires that the depth of the image (XImage) matches the depth of the window or
    // pixmap (target of XPutImage()). Only ZPixmap format is relevant. With ZPixmap, image
    // data is ordered by pixel rather than by bit-plane, and each scanline unit (word)
    // holds one or more pixels. The ZPixmap format supports the depths of any offered
    // visual. XPutImage() can handle byte swapping and changes in row alignment
    // (`scanline_pad` / `bitmap_pad`).
    int scanline_pad = pixmap_format.bits_per_pixel;
    img.width            = size.width;
    img.height           = size.height;
    img.xoffset          = 0;
    img.format           = ZPixmap;
    img.data             = buffer;
    img.byte_order       = byte_order;
    img.bitmap_unit      = BitmapUnit(dpy); // Immaterial
    img.bitmap_bit_order = BitmapBitOrder(dpy); // Immaterial
    img.bitmap_pad       = scanline_pad;
    img.depth            = visual_info.depth;
    img.bytes_per_line   = 0;
    img.bits_per_pixel   = pixmap_format.bits_per_pixel;
    img.red_mask         = visual_info.red_mask;
    img.green_mask       = visual_info.green_mask;
    img.blue_mask        = visual_info.blue_mask;
    Status status = XInitImage(&img);
    if (ARCHON_UNLIKELY(status == 0))
        throw std::runtime_error("XInitImage() failed");
}


void x11::init_grayscale_colormap(Display* dpy, Colormap colormap, const x11::MultFields& fields,
                                  int colormap_size, bool fill, bool weird)
{
    ARCHON_ASSERT(colormap_size >= 0);
    ARCHON_ASSERT(fields.red_max <= 65535);

    MultFieldsDigest digest(fields);
    bool is_gray = true;
    ARCHON_ASSERT(digest.is_valid_and_compact(fields.offset, colormap_size, is_gray));

    int begin_1 = int(fields.offset);
    int end_1   = int(fields.red_max + 1);

    int begin_2 = begin_1, end_2 = end_1;
    if (fill) {
        begin_2 = 0;
        end_2   = colormap_size;
    }

    using ulong = unsigned long;
    auto scale = [](int val, ulong max) noexcept -> ulong {
        ARCHON_ASSERT(max <= 65535u);
        ARCHON_ASSERT(val >= 0 && unsigned(val) <= max);
        return core::int_div_round_half_down(ulong(val) * 65535, max); // FIXME: Is this the proper scaling scheme?                 
    };

    constexpr int max_chunk_size = 256;
    XColor colors[max_chunk_size];

    int offset = begin_2;
    while (offset < end_2) {
        int n = std::min(end_2 - offset, max_chunk_size);
        for (int i = 0; i < n; ++i) {
            XColor& color = colors[i];
            color = {};
            color.pixel = ulong(i);
            color.flags = DoRed | DoGreen | DoBlue;
            int j = offset + i;
            if (ARCHON_LIKELY(j >= begin_1 && j < end_1)) {
                int val = j - begin_1;
                ulong val_2 = scale(val, fields.red_max);
                if (weird)
                    val_2 = 65535ul - val_2;
                color.red   = val_2;
                color.green = val_2;
                color.blue  = val_2;
            }
        }
        XStoreColors(dpy, colormap, colors, n);
        offset += n;
    }
}


void x11::init_pseudocolor_colormap(Display* dpy, Colormap colormap, const x11::MultFields& fields,
                                    int colormap_size, bool fill, bool weird)
{
    ARCHON_ASSERT(colormap_size >= 0);
    ARCHON_ASSERT(fields.red_max <= 65535);
    ARCHON_ASSERT(fields.green_max <= 65535);
    ARCHON_ASSERT(fields.blue_max <= 65535);

    MultFieldsDigest digest(fields);
    bool is_gray = false;
    ARCHON_ASSERT(digest.is_valid_and_compact(fields.offset, colormap_size, is_gray));

    int begin_1 = int(fields.offset);
    int end_1   = int((digest.max[digest.order[2]] + 1) * digest.mult[digest.order[2]]);

    int begin_2 = begin_1, end_2 = end_1;
    if (fill) {
        begin_2 = 0;
        end_2   = colormap_size;
    }

    using ulong = unsigned long;
    auto scale = [](int val, ulong max) noexcept -> ulong {
        ARCHON_ASSERT(max <= 65535u);
        ARCHON_ASSERT(val >= 0 && unsigned(val) <= max);
        return core::int_div_round_half_down(ulong(val) * 65535, max); // FIXME: Is this the proper scaling scheme?                 
    };

    constexpr int max_chunk_size = 256;
    XColor colors[max_chunk_size];

    int offset = begin_2;
    while (offset < end_2) {
        int n = std::min(end_2 - offset, max_chunk_size);
        for (int i = 0; i < n; ++i) {
            XColor& color = colors[i];
            color = {};
            color.pixel = ulong(i);
            color.flags = DoRed | DoGreen | DoBlue;
            int j = offset + i;
            if (ARCHON_LIKELY(j >= begin_1 && j < end_1)) {
                int val = j - begin_1;
                int comp[3] = {};

                comp[digest.order[2]] = val / digest.mult[digest.order[2]];
                val %= digest.mult[digest.order[2]];
                comp[digest.order[1]] = val / digest.mult[digest.order[1]];
                val %= digest.mult[digest.order[1]];
                comp[digest.order[0]] = val;

                color.red   = scale(comp[0], fields.red_max);
                color.green = scale(comp[1], fields.green_max);
                color.blue  = scale(comp[2], fields.blue_max);
                if (weird)
                    color.red = 65535ul - color.red;
            }
        }
        XStoreColors(dpy, colormap, colors, n);
        offset += n;
    }
}


void x11::init_directcolor_colormap(Display* dpy, Colormap colormap, const x11::BitFields& fields, int colormap_size,
                                    bool weird)
{
    ARCHON_ASSERT(fields.red_shift < 31);
    ARCHON_ASSERT(fields.green_shift < 31);
    ARCHON_ASSERT(fields.blue_shift < 31);
    ARCHON_ASSERT(fields.red_width <= 16);
    ARCHON_ASSERT(fields.green_width <= 16);
    ARCHON_ASSERT(fields.blue_width <= 16);

    constexpr int max_chunk_size = 256;
    XColor colors[max_chunk_size];

    long num_red   = long(1) << fields.red_width;
    long num_green = long(1) << fields.green_width;
    long num_blue  = long(1) << fields.blue_width;
    long num_entries = std::max({ num_red, num_green, num_blue });
    ARCHON_ASSERT(num_entries <= colormap_size);
    long offset = 0;
    while (offset < num_entries) {
        int n = int(std::min(num_entries - offset, long(max_chunk_size)));
        for (int i = 0; i < n; ++i) {
            namespace uf = util::unit_frac;
            using ushort = unsigned short;
            using ulong  = unsigned long;
            XColor& color = colors[i];
            color = {};
            long j = offset + i;
            if (j < num_red) {
                long val = uf::change_bit_width(j, fields.red_width, 16);
                if (weird)
                    val = 65535 - val;
                color.pixel |= ulong(j) << fields.red_shift;
                color.red = ushort(val);
                color.flags |= DoRed;
            }
            if (j < num_green) {
                long val = uf::change_bit_width(j, fields.green_width, 16);
                color.pixel |= ulong(j) << fields.green_shift;
                color.green = ushort(val);
                color.flags |= DoGreen;
            }
            if (j < num_blue) {
                long val = uf::change_bit_width(j, fields.blue_width, 16);
                color.pixel |= ulong(j) << fields.blue_shift;
                color.blue = ushort(val);
                color.flags |= DoBlue;
            }
        }
        XStoreColors(dpy, colormap, colors, n);
        offset += n;
    }
}


void x11::setup_standard_grayscale_colormap(Display* dpy, Colormap colormap, int depth, int colormap_size, bool weird)
{
    ARCHON_ASSERT(depth >= 0);

    x11::BitFields fields = {};
    fields.red_width = depth;

    bool is_gray = true;
    x11::MultFields fields_2(fields, is_gray);
    bool fill = true;
    x11::init_grayscale_colormap(dpy, colormap, fields_2, colormap_size, fill, weird); // Throws
}


void x11::setup_standard_pseudocolor_colormap(Display* dpy, Colormap colormap, int depth, int colormap_size,
                                              x11::BitFields& fields, bool weird)
{
    ARCHON_ASSERT(depth >= 0);

    // Eye sensitivity is highest for green and lowest for blue, so assign surplus bits to
    // the channels in that order.
    int remaining_width = depth;
    int blue_width = remaining_width / 3;
    remaining_width -= blue_width;
    int red_width = remaining_width / 2;
    remaining_width -= red_width;
    int green_width = remaining_width / 1;

    int red_shift   = 0;
    int green_shift = 0 + red_width;
    int blue_shift  = 0 + red_width + green_width;

    x11::BitFields fields_1 = {
        red_shift, red_width,
        green_shift, green_width,
        blue_shift, blue_width,
    };

    bool is_gray = false;
    x11::MultFields fields_2(fields_1, is_gray);
    bool fill = true;
    x11::init_pseudocolor_colormap(dpy, colormap, fields_2, colormap_size, fill, weird); // Throws

    fields = fields_1;
}


auto x11::create_pixel_format(Display* dpy, ::Window root, const XVisualInfo& visual_info,
                              const XPixmapFormatValues& pixmap_format, const x11::ColormapFinder& colormap_finder,
                              const std::locale& locale, log::Logger& logger,
                              bool prefer_default_nondecomposed_colormap,
                              bool weird) -> std::unique_ptr<x11::PixelFormat>
{
    std::unique_ptr<x11::PixelFormat> format;
    std::string error_message;
    PixelFormatCreator creator(dpy, root, visual_info, pixmap_format, colormap_finder, locale, logger,
                               prefer_default_nondecomposed_colormap, weird);
    if (ARCHON_LIKELY(creator.create(format, &error_message))) // Throws
        return format;
    throw std::runtime_error(error_message);
}


#endif // HAVE_X11
