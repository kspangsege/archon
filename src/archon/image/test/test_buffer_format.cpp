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
#include <type_traits>
#include <iterator>
#include <algorithm>
#include <functional>
#include <array>
#include <ostream>

#include <archon/core/features.h>
#include <archon/core/type_traits.hpp>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/flat_set.hpp>
#include <archon/core/format.hpp>
#include <archon/core/as_int.hpp>
#include <archon/core/as_list.hpp>
#include <archon/core/endianness.hpp>
#include <archon/check.hpp>
#include <archon/image/geom.hpp>
#include <archon/image/color_space.hpp>
#include <archon/image/bit_field.hpp>
#include <archon/image/buffer_format.hpp>
#include <archon/image/test/box_utils.hpp>


using namespace archon;


namespace {


using IntegerType = image::BufferFormat::IntegerType;

constexpr std::array g_other_integer_types = {
    IntegerType::short_,
};


template<class T> constexpr bool no_unused_bits = core::int_width<T>() == int(sizeof (T)) * core::int_width<char>();

struct is_double_int {
    template<class T> static constexpr bool value = (sizeof (T) == 2 && no_unused_bits<T>);
};

struct is_quadruple_int {
    template<class T> static constexpr bool value = (sizeof (T) == 4 && no_unused_bits<T>);
};

using double_int_type = core::pick_type<is_double_int, short, int, long, long long>;
using quadruple_int_type = core::pick_type<is_quadruple_int, short, int, long, long long>;


using IntegerFormat = image::BufferFormat::IntegerFormat;
using PackedFormat  = image::BufferFormat::PackedFormat;
using SubwordFormat = image::BufferFormat::SubwordFormat;


template<class F> void generate_integer_test_formats(IntegerType word_type, F&& func)
{
    auto gen = [&](int bits_per_word, int words_per_channel, core::Endianness word_order,
                   const image::ColorSpace& color_space, bool has_alpha_channel,
                   bool alpha_channel_first, bool reverse_channel_order) {
        image::BufferFormat::ChannelConf channel_conf = {
            &color_space,
            has_alpha_channel,
            alpha_channel_first,
            reverse_channel_order,
        };
        IntegerFormat format = {
            word_type,
            bits_per_word,
            words_per_channel,
            word_order,
            channel_conf,
        };
        func(format); // Throws
    };

    auto gen_2 = [&](int bits_per_word, int words_per_channel, core::Endianness word_order,
                     const image::ColorSpace& color_space, bool has_alpha_channel) {
        gen(bits_per_word, words_per_channel, word_order, color_space, has_alpha_channel, false, false); // Throws
        gen(bits_per_word, words_per_channel, word_order, color_space, has_alpha_channel, false, true);  // Throws
        gen(bits_per_word, words_per_channel, word_order, color_space, has_alpha_channel, true, false);  // Throws
        gen(bits_per_word, words_per_channel, word_order, color_space, has_alpha_channel, true, true);   // Throws
    };

    auto gen_3 = [&](int bits_per_word, int words_per_channel, core::Endianness word_order) {
        gen_2(bits_per_word, words_per_channel, word_order, image::ColorSpace::get_lum(), false); // Throws
        gen_2(bits_per_word, words_per_channel, word_order, image::ColorSpace::get_lum(), true);  // Throws
        gen_2(bits_per_word, words_per_channel, word_order, image::ColorSpace::get_rgb(), false); // Throws
        gen_2(bits_per_word, words_per_channel, word_order, image::ColorSpace::get_rgb(), true);  // Throws
    };

    auto gen_4 = [&](int bits_per_word, int words_per_channel) {
        gen_3(bits_per_word, words_per_channel, core::Endianness::big);    // Throws
        gen_3(bits_per_word, words_per_channel, core::Endianness::little); // Throws
    };

    auto gen_5 = [&](int bits_per_word) {
        gen_4(bits_per_word, 1); // Throws
        gen_4(bits_per_word, 2); // Throws
        gen_4(bits_per_word, 3); // Throws
        gen_4(bits_per_word, 4); // Throws
    };

    int bits_per_word = image::BufferFormat::get_bits_per_word(word_type);
    gen_5(bits_per_word / 1 - 0); // Throws
    gen_5(bits_per_word / 1 - 1); // Throws
    gen_5(bits_per_word / 2 - 0); // Throws
    gen_5(bits_per_word / 2 - 1); // Throws
}


template<class F> void generate_packed_test_formats(IntegerType word_type, bool thorough, F&& func)
{
    int field_width_denominators_1[] = { 2, 4, 6 };
    int field_width_denominators_2[] = { 1, 2, 3, 4, 5, 6 }; // Thorough mode
    int field_width_deductions[] = { 0, 1 };
    int field_gaps_1[] = { 0, 1 };
    int field_gaps_2[] = { 0, 1, 2 }; // Thorough mode
    core::Span<int> field_width_denominators = field_width_denominators_1;
    core::Span<int> field_gaps = field_gaps_1;
    if (thorough) {
        field_width_denominators = field_width_denominators_2;
        field_gaps = field_gaps_2;
    }
    constexpr int max_field_widths = std::size(field_width_denominators_2) * std::size(field_width_deductions);
    core::FlatSet<int, max_field_widths> field_widths;

    auto gen = [&](int bits_per_word, int words_per_pixel, core::Endianness word_order,
                   const image::ColorSpace& color_space, bool has_alpha_channel,
                   bool alpha_channel_first, bool reverse_channel_order) {
        image::BufferFormat::ChannelConf channel_conf = {
            &color_space,
            has_alpha_channel,
            alpha_channel_first,
            reverse_channel_order,
        };
        PackedFormat format = {
            word_type,
            bits_per_word,
            words_per_pixel,
            word_order,
            {}, // bit fields
            channel_conf,
        };
        int num_channels = channel_conf.get_num_channels();
        auto valid_fields_spec = [&]() noexcept {
            int w = 0;
            int n = num_channels;
            for (int i = 0; i < n; ++i) {
                const image::BitField& field = format.bit_fields[i];
                w = field.width + field.gap + w;
            }
            return (w <= format.words_per_pixel * format.bits_per_word);
        };
        std::function<void(int)> loop = [&](int field_index) {
            for (int i = 0; i < int(field_widths.size()); ++i) {
                format.bit_fields[field_index].width = field_widths.begin()[i];
                for (int j = 0; j < int(std::size(field_gaps)); ++j) {
                    format.bit_fields[field_index].gap = field_gaps[j];
                    if (ARCHON_LIKELY(field_index == num_channels - 1)) {
                        if (valid_fields_spec())
                            func(format); // Throws
                    }
                    else {
                        loop(field_index + 1); // Throws
                    }
                }
            }
        };
        loop(0); // Throws
    };

    auto gen_2 = [&](int bits_per_word, int words_per_pixel, core::Endianness word_order,
                     const image::ColorSpace& color_space, bool has_alpha_channel) {
        gen(bits_per_word, words_per_pixel, word_order, color_space, has_alpha_channel, false, false); // Throws
        gen(bits_per_word, words_per_pixel, word_order, color_space, has_alpha_channel, false, true);  // Throws
        gen(bits_per_word, words_per_pixel, word_order, color_space, has_alpha_channel, true, false);  // Throws
        gen(bits_per_word, words_per_pixel, word_order, color_space, has_alpha_channel, true, true);   // Throws
    };

    auto gen_3 = [&](int bits_per_word, int words_per_pixel, core::Endianness word_order) {
        gen_2(bits_per_word, words_per_pixel, word_order, image::ColorSpace::get_lum(), false); // Throws
        gen_2(bits_per_word, words_per_pixel, word_order, image::ColorSpace::get_lum(), true);  // Throws
        gen_2(bits_per_word, words_per_pixel, word_order, image::ColorSpace::get_rgb(), false); // Throws
        gen_2(bits_per_word, words_per_pixel, word_order, image::ColorSpace::get_rgb(), true);  // Throws
    };

    auto gen_4 = [&](int bits_per_word, int words_per_pixel) {
        gen_3(bits_per_word, words_per_pixel, core::Endianness::big);    // Throws
        gen_3(bits_per_word, words_per_pixel, core::Endianness::little); // Throws
    };

    auto gen_5 = [&](int bits_per_word) {
        field_widths.clear();
        for (int denom : field_width_denominators) {
            for (int deduc : field_width_deductions) {
                int cand = 2 * bits_per_word / denom - deduc;
                if (cand > 0)
                    field_widths.insert(cand); // Throws
            }
        }
        gen_4(bits_per_word, 1); // Throws
        gen_4(bits_per_word, 2); // Throws
        gen_4(bits_per_word, 3); // Throws
        gen_4(bits_per_word, 4); // Throws
    };

    int bits_per_word = image::BufferFormat::get_bits_per_word(word_type);
    gen_5(bits_per_word / 1 - 0); // Throws
    gen_5(bits_per_word / 1 - 1); // Throws
    gen_5(bits_per_word / 2 - 0); // Throws
    gen_5(bits_per_word / 2 - 1); // Throws
}


template<class F> void generate_subword_test_formats(IntegerType word_type, F&& func)
{
    auto gen = [&](int bits_per_channel, int pixels_per_word, core::Endianness bit_order, bool word_aligned_rows,
                   image::ColorSpace::Tag color_space_tag, bool has_alpha_channel, bool alpha_channel_first,
                   bool reverse_channel_order) {
        image::BufferFormat::ChannelConf channel_conf = {
            &image::get_color_space(color_space_tag),
            has_alpha_channel,
            alpha_channel_first,
            reverse_channel_order,
        };
        SubwordFormat format = {
            word_type,
            bits_per_channel,
            pixels_per_word,
            bit_order,
            word_aligned_rows,
            channel_conf,
        };
        func(format); // Throws
    };

    auto gen_2 = [&](int bits_per_channel, int pixels_per_word, core::Endianness bit_order, bool word_aligned_rows,
                     image::ColorSpace::Tag color_space_tag, bool has_alpha_channel) {
        gen(bits_per_channel, pixels_per_word, bit_order, word_aligned_rows,
            color_space_tag, has_alpha_channel, false, false); // Throws
        gen(bits_per_channel, pixels_per_word, bit_order, word_aligned_rows,
            color_space_tag, has_alpha_channel, false, true); // Throws
        gen(bits_per_channel, pixels_per_word, bit_order, word_aligned_rows,
            color_space_tag, has_alpha_channel, true, false); // Throws
        gen(bits_per_channel, pixels_per_word, bit_order, word_aligned_rows,
            color_space_tag, has_alpha_channel, true, true); // Throws
    };

    int bits_per_word = image::BufferFormat::get_bits_per_word(word_type);

    auto gen_3 = [&](int bits_per_channel, core::Endianness bit_order, bool word_aligned_rows,
                     image::ColorSpace::Tag color_space_tag, bool has_alpha_channel) {
        int num_channels = image::get_num_channels(color_space_tag) + int(has_alpha_channel);
        int bits_per_pixel = num_channels * bits_per_channel;
        int max_pixels_per_word = bits_per_word / bits_per_pixel;
        for (int deduct: { 0, 1, 2 }) {
            int pixels_per_word = max_pixels_per_word - deduct;
            if (pixels_per_word > 0) {
                gen_2(bits_per_channel, pixels_per_word, bit_order, word_aligned_rows,
                      color_space_tag, has_alpha_channel); // Throws
            }
        }
    };

    auto gen_4 = [&](int bits_per_channel, core::Endianness bit_order, bool word_aligned_rows) {
        gen_3(bits_per_channel, bit_order, word_aligned_rows, image::ColorSpace::Tag::lum, false); // Throws
        gen_3(bits_per_channel, bit_order, word_aligned_rows, image::ColorSpace::Tag::lum, true);  // Throws
        gen_3(bits_per_channel, bit_order, word_aligned_rows, image::ColorSpace::Tag::rgb, false); // Throws
        gen_3(bits_per_channel, bit_order, word_aligned_rows, image::ColorSpace::Tag::rgb, true);  // Throws
    };

    auto gen_5 = [&](int bits_per_channel, core::Endianness bit_order) {
        gen_4(bits_per_channel, bit_order, false); // Throws
        gen_4(bits_per_channel, bit_order, true);  // Throws
    };

    auto gen_6 = [&](int bits_per_channel) {
        gen_5(bits_per_channel, core::Endianness::big);    // Throws
        gen_5(bits_per_channel, core::Endianness::little); // Throws
    };

    for (int n: { 1, 2, 3, 4, 5, 7, 8, 9, 15, 16 })
        gen_6(n); // Throws
}


struct channel_conf_wrapper {
    const image::BufferFormat::ChannelConf& conf;

    bool operator==(const channel_conf_wrapper& other) const noexcept;
};


auto wrap(const image::BufferFormat::ChannelConf& conf) noexcept
{
    return channel_conf_wrapper { conf };
}


auto operator<<(std::ostream& out, channel_conf_wrapper wrapper) -> std::ostream&
{
    const image::BufferFormat::ChannelConf& conf = wrapper.conf;
    image::ColorSpace::Tag color_space_tag = {};
    bool success = conf.color_space->try_get_tag(color_space_tag);
    ARCHON_ASSERT(success);
    out << core::formatted("(%s, %s, %s, %s)", color_space_tag, conf.has_alpha, conf.alpha_first,
                           conf.reverse_order); // Throws
    return out;
}


inline bool channel_conf_wrapper::operator==(const channel_conf_wrapper& other) const noexcept
{
    using ChannelConf = image::BufferFormat::ChannelConf;
    const ChannelConf& a = conf;
    const ChannelConf& b = other.conf;
    return (a.color_space == b.color_space &&
            a.has_alpha == b.has_alpha     &&
            a.alpha_first == b.alpha_first &&
            a.reverse_order == b.reverse_order);
}


struct integer_format_wrapper {
    const IntegerFormat& format;

    bool operator==(const integer_format_wrapper& other) const noexcept;
};


auto wrap(const IntegerFormat& format) noexcept
{
    return integer_format_wrapper { format };
}


auto operator<<(std::ostream& out, integer_format_wrapper wrapper) -> std::ostream&
{
    const IntegerFormat& format = wrapper.format;
    out << core::formatted("(%s, %s, %s, %s, %s)", format.word_type, core::as_int(format.bits_per_word),
                           core::as_int(format.words_per_channel), format.word_order,
                           wrap(format.channel_conf)); // Throws
    return out;
}


inline bool integer_format_wrapper::operator==(const integer_format_wrapper& other) const noexcept
{
    const IntegerFormat& a = format;
    const IntegerFormat& b = other.format;
    return (a.word_type == b.word_type                 &&
            a.bits_per_word == b.bits_per_word         &&
            a.words_per_channel == b.words_per_channel &&
            a.word_order == b.word_order               &&
            wrap(a.channel_conf) == wrap(b.channel_conf));
}


struct packed_format_wrapper {
    const PackedFormat& format;

    bool operator==(const packed_format_wrapper& other) const noexcept;
};


auto wrap(const PackedFormat& format) noexcept
{
    return packed_format_wrapper { format };
}


auto operator<<(std::ostream& out, packed_format_wrapper wrapper) -> std::ostream&
{
    const PackedFormat& format = wrapper.format;
    int num_channels = format.channel_conf.get_num_channels();
    core::Span bit_fields = { format.bit_fields.data(), std::size_t(num_channels) };
    auto format_bit_field = [](const image::BitField& field) {
        return core::formatted("(%s, %s)", field.width, field.gap);
    };
    out << core::formatted("(%s, %s, %s, %s, %s, %s)", format.word_type, core::as_int(format.bits_per_word),
                           core::as_int(format.words_per_pixel), format.word_order,
                           core::as_sbr_list(bit_fields, format_bit_field),
                           wrap(format.channel_conf)); // Throws
    return out;
}


inline bool packed_format_wrapper::operator==(const packed_format_wrapper& other) const noexcept
{
    const PackedFormat& a = format;
    const PackedFormat& b = other.format;
    return (a.word_type == b.word_type             &&
            a.bits_per_word == b.bits_per_word     &&
            a.words_per_pixel == b.words_per_pixel &&
            a.word_order == b.word_order           &&
            std::equal(a.bit_fields.data(), a.bit_fields.data() + a.channel_conf.get_num_channels(),
                       b.bit_fields.data())        &&
            wrap(a.channel_conf) == wrap(b.channel_conf));
}


struct subword_format_wrapper {
    const SubwordFormat& format;

    bool operator==(const subword_format_wrapper& other) const noexcept;
};


auto wrap(const SubwordFormat& format) noexcept
{
    return subword_format_wrapper { format };
}


auto operator<<(std::ostream& out, subword_format_wrapper wrapper) -> std::ostream&
{
    const SubwordFormat& format = wrapper.format;
    out << core::formatted("(%s, %s, %s, %s, %s, %s)", format.word_type, core::as_int(format.bits_per_channel),
                           core::as_int(format.pixels_per_word), format.bit_order, format.word_aligned_rows,
                           wrap(format.channel_conf)); // Throws
    return out;
}


inline bool subword_format_wrapper::operator==(const subword_format_wrapper& other) const noexcept
{
    const SubwordFormat& a = format;
    const SubwordFormat& b = other.format;
    return (a.word_type == b.word_type                 &&
            a.bits_per_channel == b.bits_per_channel   &&
            a.pixels_per_word == b.pixels_per_word     &&
            a.bit_order == b.bit_order                 &&
            a.word_aligned_rows == b.word_aligned_rows &&
            wrap(a.channel_conf) == wrap(b.channel_conf));
}


// Map from canonical channel roder to channel storage order
int map_channel_index(const image::BufferFormat::ChannelConf& channel_conf, int i) noexcept
{
    int n = channel_conf.get_num_channels();
    ARCHON_ASSERT(i >= 0 && i < n);
    int j = i;
    if (channel_conf.has_alpha && channel_conf.alpha_first)
        j = (j + 1) % n;
    if (channel_conf.reverse_order)
        j = (n - 1) - j;
    return j;
}


int get_channel_width(const IntegerFormat& format, int) noexcept
{
    return format.words_per_channel * format.bits_per_word;
}


int get_channel_width(const PackedFormat& format, int channel_index) noexcept
{
    int channel_index_2 = map_channel_index(format.channel_conf, channel_index);
    ARCHON_ASSERT(channel_index_2 < format.channel_conf.get_num_channels());
    return format.bit_fields[channel_index_2].width;
}


int get_channel_width(const SubwordFormat& format, int) noexcept
{
    return format.bits_per_channel;
}


void map_bit_position(const IntegerFormat& format, const image::Size& image_size,
                      const image::Pos& pos, int channel_index, int bit_pos,
                      std::size_t& word_index, int& bit_pos_2) noexcept
{
    int num_channels = format.channel_conf.get_num_channels();
    std::size_t words_per_pixel = std::size_t(num_channels * format.words_per_channel);
    std::size_t pixel_offset = std::size_t((pos.x + pos.y * std::size_t(image_size.width)) * words_per_pixel);
    int channel_index_2 = map_channel_index(format.channel_conf, channel_index);
    std::size_t comp_offset = std::size_t(pixel_offset + channel_index_2 * format.words_per_channel);
    int word_index_2 = bit_pos / format.bits_per_word;
    int bit_pos_3 = bit_pos % format.bits_per_word;
    if (format.word_order == core::Endianness::big)
        word_index_2 = format.words_per_channel - 1 - word_index_2;
    word_index = std::size_t(comp_offset + word_index_2);
    bit_pos_2 = bit_pos_3;
}


void map_bit_position(const PackedFormat& format, const image::Size& image_size,
                      const image::Pos& pos, int channel_index, int bit_pos,
                      std::size_t& word_index, int& bit_pos_2) noexcept
{
    std::size_t pixel_offset = std::size_t((pos.x + pos.y * std::size_t(image_size.width)) * format.words_per_pixel);
    int num_channels = format.channel_conf.get_num_channels();
    int channel_index_2 = map_channel_index(format.channel_conf, channel_index);
    int bit_width = image::get_bit_field_width(format.bit_fields.data(), num_channels, channel_index_2);
    ARCHON_ASSERT(bit_pos < bit_width);
    int bit_shift = image::get_bit_field_shift(format.bit_fields.data(), num_channels, channel_index_2);
    int bit_pos_3 = bit_shift + bit_pos;
    int word_index_2 = bit_pos_3 / format.bits_per_word;
    int bit_pos_4 = bit_pos_3 % format.bits_per_word;
    if (format.word_order == core::Endianness::big)
        word_index_2 = format.words_per_pixel - 1 - word_index_2;
    word_index = std::size_t(pixel_offset + word_index_2);
    bit_pos_2 = bit_pos_4;
}


void map_bit_position(const SubwordFormat& format, const image::Size& image_size,
                      const image::Pos& pos, int channel_index, int bit_pos,
                      std::size_t& word_index, int& bit_pos_2) noexcept
{
    std::size_t word_offset;
    std::size_t pixel_index;
    if (format.word_aligned_rows) {
        int words_per_row = core::int_div_round_up(image_size.width, format.pixels_per_word);
        word_offset = std::size_t(pos.y * std::size_t(words_per_row));
        pixel_index = std::size_t(pos.x);
    }
    else {
        word_offset = 0;
        pixel_index = std::size_t((pos.x + pos.y * std::size_t(image_size.width)));
    }
    std::size_t word_index_2 = std::size_t(pixel_index / format.pixels_per_word);
    int pixel_index_2 = int(pixel_index % format.pixels_per_word);
    if (format.bit_order == core::Endianness::little)
        pixel_index_2 = format.pixels_per_word - 1 - pixel_index_2;
    int num_channels = format.channel_conf.get_num_channels();
    int channel_index_2 = map_channel_index(format.channel_conf, channel_index);
    int component_index = num_channels * pixel_index_2 + channel_index_2;
    int components_per_word = format.pixels_per_word * num_channels;
    int comp_pos = (components_per_word - 1 - component_index) * format.bits_per_channel;
    word_index = word_offset + word_index_2;
    bit_pos_2 = comp_pos + bit_pos;
}


inline void map_word_to_byte_position(core::Endianness byte_order, int bytes_per_word,
                                      std::size_t word_index, int bit_pos,
                                      std::size_t& byte_index, int& bit_pos_2) noexcept
{
    int bits_per_byte = core::int_width<char>();
    int byte_index_2 = bit_pos / bits_per_byte;
    int bit_pos_3 = bit_pos % bits_per_byte;
    if (byte_order == core::Endianness::big)
        byte_index_2 = bytes_per_word - 1 - byte_index_2;
    byte_index = std::size_t(word_index * bytes_per_word + byte_index_2);
    bit_pos_2 = bit_pos_3;
}


template<class F, class G> bool equivalent_formats(const F& origin_format, const G& target_format)
{
    int num_channels = origin_format.channel_conf.get_num_channels();
    ARCHON_ASSERT(num_channels == target_format.channel_conf.get_num_channels());
    for (int i = 0; i < num_channels; ++i) {
        int a = get_channel_width(origin_format, i);
        int b = get_channel_width(target_format, i);
        if (ARCHON_UNLIKELY(a != b))
            return false;
    }

    if (target_format.word_type == origin_format.word_type) {
        bool equivalent = true;
        auto check = [&](const image::Size& image_size) {
            image::test::for_each_pos_in(image_size, [&](const image::Pos& pos) {
                for (int i = 0; i < num_channels; ++i) {
                    int bit_width = get_channel_width(origin_format, i);
                    for (int j = 0; j < bit_width; ++j) {
                        std::size_t word_index_1 = {}, word_index_2 = {};
                        int bit_pos_1 = {}, bit_pos_2 = {};
                        map_bit_position(origin_format, image_size, pos, i, j, word_index_1, bit_pos_1);
                        map_bit_position(target_format, image_size, pos, i, j, word_index_2, bit_pos_2);
                        if (ARCHON_LIKELY(word_index_1 == word_index_2 && bit_pos_1 == bit_pos_2))
                            continue;
                        equivalent = false;
                    }
                }
            });
        };
        check({ 1, 2 });
        check({ 2, 2 });
        return equivalent;
    }

    core::Endianness origin_byte_order = {}, target_byte_order = {};
    bool success_1 = image::BufferFormat::try_get_byte_order(origin_format.word_type, origin_byte_order);
    bool success_2 = image::BufferFormat::try_get_byte_order(target_format.word_type, target_byte_order);
    ARCHON_ASSERT(success_1);
    ARCHON_ASSERT(success_2);

    int bytes_per_origin_word = image::BufferFormat::get_bytes_per_word(origin_format.word_type);
    int bytes_per_target_word = image::BufferFormat::get_bytes_per_word(target_format.word_type);

    bool equivalent = true;
    auto check = [&](const image::Size& image_size) {
        image::test::for_each_pos_in(image_size, [&](const image::Pos& pos) {
            for (int i = 0; i < num_channels; ++i) {
                int bit_width = get_channel_width(origin_format, i);
                for (int j = 0; j < bit_width; ++j) {
                    std::size_t word_index_1 = {}, word_index_2 = {};
                    std::size_t byte_index_1 = {}, byte_index_2 = {};
                    int bit_pos_1 = {}, bit_pos_2 = {}, bit_pos_3 = {}, bit_pos_4 = {};
                    map_bit_position(origin_format, image_size, pos, i, j, word_index_1, bit_pos_1);
                    map_word_to_byte_position(origin_byte_order, bytes_per_origin_word, word_index_1, bit_pos_1,
                                              byte_index_1, bit_pos_3);
                    map_bit_position(target_format, image_size, pos, i, j, word_index_2, bit_pos_2);
                    map_word_to_byte_position(target_byte_order, bytes_per_target_word, word_index_2, bit_pos_2,
                                              byte_index_2, bit_pos_4);
                    if (ARCHON_LIKELY(byte_index_1 == byte_index_2 && bit_pos_3 == bit_pos_4))
                        continue;
                    equivalent = false;
                }
            }
        });
    };
    check({ 1, 2 });
    check({ 2, 2 });
    return equivalent;
}


} // unnamed namespace



ARCHON_TEST_VARIANTS(abridged_integer_type_variants,
                     ARCHON_TEST_VALUE(IntegerType::byte,   Byte),
                     ARCHON_TEST_VALUE(IntegerType::schar,  SignedChar),
                     ARCHON_TEST_VALUE(IntegerType::fict_1, Fictional1),
                     ARCHON_TEST_VALUE(IntegerType::fict_2, Fictional2),
                     ARCHON_TEST_VALUE(IntegerType::fict_3, Fictional3));


ARCHON_TEST_BATCH(Image_BufferFormat_TryCastTo_IntegerToInteger, abridged_integer_type_variants)
{
    IntegerType word_type = test_value;

    auto test = [&, &parent_test_context = test_context](const IntegerFormat& format) {
        ARCHON_TEST_TRAIL(parent_test_context, wrap(format));

        image::BufferFormat format_2 = format;
        IntegerFormat format_3 = {};

        // Cast to same word type
        {
            bool success = format_2.try_cast_to(format_3, format.word_type);
            bool expect_success = true;
            ARCHON_CHECK_EQUAL(success, expect_success);
            if (success && expect_success)
                ARCHON_CHECK_EQUAL(wrap(format_3), wrap(format));
        }

        // Cast to bytes
        if (format.word_type != IntegerType::byte) {
            bool success = format_2.try_cast_to(format_3, IntegerType::byte);
            int bytes_per_word = image::BufferFormat::get_bytes_per_word(format.word_type);
            if (bytes_per_word == 1) {
                bool expect_success = true;
                ARCHON_CHECK_EQUAL(success, expect_success);
                if (success && expect_success) {
                    ARCHON_CHECK_EQUAL(format_3.word_type, IntegerType::byte);
                    ARCHON_CHECK_EQUAL(format_3.bits_per_word, format.bits_per_word);
                    ARCHON_CHECK_EQUAL(format_3.words_per_channel, format.words_per_channel);
                    ARCHON_CHECK_EQUAL(format_3.word_order, format.word_order);
                    ARCHON_CHECK_EQUAL(wrap(format_3.channel_conf), wrap(format.channel_conf));
                }
            }
            else {
                int bits_per_byte = core::int_width<char>();
                int bytes_per_word = image::BufferFormat::get_bytes_per_word(format.word_type);
                bool all_bits_used = (format.bits_per_word == bytes_per_word * bits_per_byte);
                core::Endianness byte_order = {};
                bool have_byte_order = image::BufferFormat::try_get_byte_order(format.word_type, byte_order);
                bool expect_success = (all_bits_used &&
                                       have_byte_order &&
                                       (format.words_per_channel == 1 || byte_order == format.word_order));
                ARCHON_CHECK_EQUAL(success, expect_success);
                if (success && expect_success) {
                    ARCHON_CHECK(format_3.is_valid());
                    ARCHON_CHECK_EQUAL(format_3.word_type, IntegerType::byte);
                    ARCHON_CHECK_EQUAL(format_3.bits_per_word, bits_per_byte);
                    ARCHON_CHECK_EQUAL(format_3.words_per_channel, format.words_per_channel * bytes_per_word);
                    ARCHON_ASSERT(have_byte_order);
                    ARCHON_CHECK_EQUAL(format_3.word_order, byte_order);
                    ARCHON_CHECK_EQUAL(wrap(format_3.channel_conf), wrap(format.channel_conf));
                    ARCHON_CHECK(equivalent_formats(format, format_3));
                }
            }
        }

        // Cast to other word types
        for (auto word_type : g_other_integer_types) {
            if (word_type == format.word_type || word_type == IntegerType::byte)
                continue;
            ARCHON_CHECK_NOT(format_2.try_cast_to(format_3, word_type));
        }
    };

    generate_integer_test_formats(word_type, [&](const IntegerFormat& format) {
        test(format);
    });
}


ARCHON_TEST_BATCH(Image_BufferFormat_TryCastTo_IntegerToPacked, abridged_integer_type_variants)
{
    IntegerType word_type = test_value;

    auto test = [&, &parent_test_context = test_context](const IntegerFormat& format) {
        ARCHON_TEST_TRAIL(parent_test_context, wrap(format));

        int num_channels = format.channel_conf.get_num_channels();
        int bits_per_channel = format.words_per_channel * format.bits_per_word;

        image::BufferFormat format_2 = format;
        PackedFormat format_3 = {};

        // Cast to same word type
        {
            bool success = format_2.try_cast_to(format_3, format.word_type);
            bool expect_success = (num_channels <= image::BufferFormat::max_bit_fields);
            ARCHON_CHECK_EQUAL(success, expect_success);
            if (success && expect_success) {
                ARCHON_CHECK(format_3.is_valid());
                ARCHON_CHECK_EQUAL(format_3.word_type, format.word_type);
                ARCHON_CHECK_EQUAL(format_3.bits_per_word, format.bits_per_word);
                ARCHON_CHECK_EQUAL(format_3.words_per_pixel, format.words_per_channel * num_channels);
                ARCHON_CHECK_EQUAL(format_3.word_order, format.word_order);
                for (int i = 0; i < image::BufferFormat::max_bit_fields; ++i) {
                    const image::BitField& bit_field = format_3.bit_fields[i];
                    ARCHON_CHECK_EQUAL(bit_field.width, (i < num_channels ? bits_per_channel : 0));
                    ARCHON_CHECK_EQUAL(bit_field.gap, 0);
                }
                ARCHON_CHECK_EQUAL(format_3.channel_conf.color_space, format.channel_conf.color_space);
                ARCHON_CHECK_EQUAL(format_3.channel_conf.has_alpha, format.channel_conf.has_alpha);
                ARCHON_CHECK_EQUAL(format_3.channel_conf.alpha_first, format.channel_conf.alpha_first);
                if (format_3.word_order == core::Endianness::big) {
                    ARCHON_CHECK_EQUAL(format_3.channel_conf.reverse_order, format.channel_conf.reverse_order);
                }
                else {
                    ARCHON_CHECK_EQUAL(format_3.channel_conf.reverse_order, !format.channel_conf.reverse_order);
                }
                ARCHON_CHECK(equivalent_formats(format, format_3));
            }
        }

        // Cast to bytes
        if (format.word_type != IntegerType::byte) {
            bool success = format_2.try_cast_to(format_3, IntegerType::byte);
            int bytes_per_word = image::BufferFormat::get_bytes_per_word(format.word_type);
            if (bytes_per_word == 1) {
                bool expect_success = (num_channels <= image::BufferFormat::max_bit_fields);
                ARCHON_CHECK_EQUAL(success, expect_success);
                if (success && expect_success) {
                    ARCHON_CHECK(format_3.is_valid());
                    ARCHON_CHECK_EQUAL(format_3.word_type, IntegerType::byte);
                    ARCHON_CHECK_EQUAL(format_3.bits_per_word, format.bits_per_word);
                    ARCHON_CHECK_EQUAL(format_3.words_per_pixel, format.words_per_channel * num_channels);
                    ARCHON_CHECK_EQUAL(format_3.word_order, format.word_order);
                    for (int i = 0; i < image::BufferFormat::max_bit_fields; ++i) {
                        const image::BitField& bit_field = format_3.bit_fields[i];
                        ARCHON_CHECK_EQUAL(bit_field.width, (i < num_channels ? bits_per_channel : 0));
                        ARCHON_CHECK_EQUAL(bit_field.gap, 0);
                    }
                    ARCHON_CHECK_EQUAL(format_3.channel_conf.color_space, format.channel_conf.color_space);
                    ARCHON_CHECK_EQUAL(format_3.channel_conf.has_alpha, format.channel_conf.has_alpha);
                    ARCHON_CHECK_EQUAL(format_3.channel_conf.alpha_first, format.channel_conf.alpha_first);
                    if (format_3.word_order == core::Endianness::big) {
                        ARCHON_CHECK_EQUAL(format_3.channel_conf.reverse_order, format.channel_conf.reverse_order);
                    }
                    else {
                        ARCHON_CHECK_EQUAL(format_3.channel_conf.reverse_order, !format.channel_conf.reverse_order);
                    }
                    ARCHON_CHECK(equivalent_formats(format, format_3));
                }
            }
            else {
                int bits_per_byte = core::int_width<char>();
                int bytes_per_word = image::BufferFormat::get_bytes_per_word(format.word_type);
                bool all_bits_used = (format.bits_per_word == bytes_per_word * bits_per_byte);
                core::Endianness byte_order = {};
                bool have_byte_order = image::BufferFormat::try_get_byte_order(format.word_type, byte_order);
                bool expect_success = (all_bits_used &&
                                       have_byte_order &&
                                       (format.words_per_channel == 1 || byte_order == format.word_order) &&
                                       num_channels <= image::BufferFormat::max_bit_fields);
                ARCHON_CHECK_EQUAL(success, expect_success);
                if (success && expect_success) {
                    ARCHON_CHECK(format_3.is_valid());
                    ARCHON_CHECK_EQUAL(format_3.word_type, IntegerType::byte);
                    ARCHON_CHECK_EQUAL(format_3.bits_per_word, bits_per_byte);
                    ARCHON_CHECK_EQUAL(format_3.words_per_pixel,
                                       format.words_per_channel * num_channels * bytes_per_word);
                    ARCHON_CHECK_EQUAL(format_3.word_order, byte_order);
                    for (int i = 0; i < image::BufferFormat::max_bit_fields; ++i) {
                        const image::BitField& bit_field = format_3.bit_fields[i];
                        ARCHON_CHECK_EQUAL(bit_field.width, (i < num_channels ? bits_per_channel : 0));
                        ARCHON_CHECK_EQUAL(bit_field.gap, 0);
                    }
                    ARCHON_CHECK_EQUAL(format_3.channel_conf.color_space, format.channel_conf.color_space);
                    ARCHON_CHECK_EQUAL(format_3.channel_conf.has_alpha, format.channel_conf.has_alpha);
                    ARCHON_CHECK_EQUAL(format_3.channel_conf.alpha_first, format.channel_conf.alpha_first);
                    if (format_3.word_order == core::Endianness::big) {
                        ARCHON_CHECK_EQUAL(format_3.channel_conf.reverse_order, format.channel_conf.reverse_order);
                    }
                    else {
                        ARCHON_CHECK_EQUAL(format_3.channel_conf.reverse_order, !format.channel_conf.reverse_order);
                    }
                    ARCHON_CHECK(equivalent_formats(format, format_3));
                }
            }
        }

        // Cast to other word types
        for (auto word_type : g_other_integer_types) {
            if (word_type == format.word_type || word_type == IntegerType::byte)
                continue;
            ARCHON_CHECK_NOT(format_2.try_cast_to(format_3, word_type));
        }
    };

    generate_integer_test_formats(word_type, [&](const IntegerFormat& format) {
        test(format);
    });
}


ARCHON_TEST_BATCH(Image_BufferFormat_TryCastTo_IntegerToSubword, abridged_integer_type_variants)
{
    IntegerType word_type = test_value;

    auto test = [&, &parent_test_context = test_context](const IntegerFormat& format) {
        ARCHON_TEST_TRAIL(parent_test_context, wrap(format));

        int num_channels = format.channel_conf.get_num_channels();

        image::BufferFormat format_2 = format;
        SubwordFormat format_3 = {};

        // Cast to same word type
        {
            bool success = format_2.try_cast_to(format_3, format.word_type);
            bool expect_success = (num_channels == 1 && format.words_per_channel == 1);
            ARCHON_CHECK_EQUAL(success, expect_success);
            if (success && expect_success) {
                ARCHON_CHECK(format_3.is_valid());
                ARCHON_CHECK_EQUAL(format_3.word_type, format.word_type);
                ARCHON_CHECK_EQUAL(format_3.bits_per_channel, format.bits_per_word);
                ARCHON_CHECK_EQUAL(format_3.pixels_per_word, 1);
                ARCHON_CHECK_EQUAL(format_3.bit_order, core::Endianness::big);
                ARCHON_CHECK_EQUAL(format_3.word_aligned_rows, false);
                ARCHON_CHECK_EQUAL(wrap(format_3.channel_conf), wrap(format.channel_conf));
                ARCHON_CHECK(equivalent_formats(format, format_3));
            }
        }

        // Cast to bytes
        if (format.word_type != IntegerType::byte) {
            int bytes_per_word = image::BufferFormat::get_bytes_per_word(format.word_type);
            bool success = format_2.try_cast_to(format_3, IntegerType::byte);
            bool expect_success = (num_channels == 1 && format.words_per_channel == 1 && bytes_per_word == 1);
            ARCHON_CHECK_EQUAL(success, expect_success);
            if (success && expect_success) {
                ARCHON_CHECK(format_3.is_valid());
                ARCHON_CHECK_EQUAL(format_3.word_type, IntegerType::byte);
                ARCHON_CHECK_EQUAL(format_3.bits_per_channel, format.bits_per_word);
                ARCHON_CHECK_EQUAL(format_3.pixels_per_word, 1);
                ARCHON_CHECK_EQUAL(format_3.bit_order, core::Endianness::big);
                ARCHON_CHECK_EQUAL(format_3.word_aligned_rows, false);
                ARCHON_CHECK_EQUAL(wrap(format_3.channel_conf), wrap(format.channel_conf));
                ARCHON_CHECK(equivalent_formats(format, format_3));
            }
        }

        // Cast to other word types
        for (auto word_type : g_other_integer_types) {
            if (word_type == format.word_type || word_type == IntegerType::byte)
                continue;
            ARCHON_CHECK_NOT(format_2.try_cast_to(format_3, word_type));
        }
    };

    generate_integer_test_formats(word_type, [&](const IntegerFormat& format) {
        test(format);
    });
}


ARCHON_TEST_BATCH(Image_BufferFormat_TryCastTo_PackedToInteger, abridged_integer_type_variants)
{
    IntegerType word_type = test_value;

    auto test = [&, &parent_test_context = test_context](const PackedFormat& format) {
        ARCHON_TEST_TRAIL(parent_test_context, wrap(format));

        int num_channels = format.channel_conf.get_num_channels();
        auto conforming_bit_fields = [&](int module, int depth) {
            int gap = module - depth;
            ARCHON_ASSERT(gap >= 0);
            for (int i = 0; i < num_channels; ++i) {
                const image::BitField& field = format.bit_fields[i];
                if (field.width != depth || field.gap != (i == num_channels - 1 ? 0 : gap))
                    return false;
            }
            return true;
        };

        image::BufferFormat format_2 = format;
        IntegerFormat format_3 = {};

        // Cast to same word type
        {
            bool success = format_2.try_cast_to(format_3, format.word_type);
            int words_per_channel = format.words_per_pixel / num_channels;
            int field_module = words_per_channel * format.bits_per_word;
            int depth = (words_per_channel == 1 ? format.bit_fields[0].width : field_module);
            bool expect_success = (format.words_per_pixel % num_channels == 0 &&
                                   conforming_bit_fields(field_module, depth));
            ARCHON_CHECK_EQUAL(success, expect_success);
            if (success && expect_success) {
                ARCHON_CHECK(format_3.is_valid());
                ARCHON_CHECK_EQUAL(format_3.word_type, format.word_type);
                if (words_per_channel == 1) {
                    ARCHON_CHECK_EQUAL(format_3.bits_per_word, depth);
                }
                else {
                    ARCHON_CHECK_EQUAL(format_3.bits_per_word, format.bits_per_word);
                }
                ARCHON_CHECK_EQUAL(format_3.words_per_channel, words_per_channel);
                ARCHON_CHECK_EQUAL(format_3.word_order, format.word_order);
                ARCHON_CHECK_EQUAL(format_3.channel_conf.color_space, format.channel_conf.color_space);
                ARCHON_CHECK_EQUAL(format_3.channel_conf.has_alpha, format.channel_conf.has_alpha);
                ARCHON_CHECK_EQUAL(format_3.channel_conf.alpha_first, format.channel_conf.alpha_first);
                if (format.word_order == core::Endianness::big) {
                    ARCHON_CHECK_EQUAL(format_3.channel_conf.reverse_order, format.channel_conf.reverse_order);
                }
                else {
                    ARCHON_CHECK_EQUAL(format_3.channel_conf.reverse_order, !format.channel_conf.reverse_order);
                }
                ARCHON_CHECK(equivalent_formats(format, format_3));
            }
        }

        // Cast to bytes
        if (format.word_type != IntegerType::byte) {
            bool success = format_2.try_cast_to(format_3, IntegerType::byte);
            int bytes_per_word = image::BufferFormat::get_bytes_per_word(format.word_type);
            int bytes_per_pixel = format.words_per_pixel * bytes_per_word;
            int bytes_per_channel = bytes_per_pixel / num_channels;
            if (bytes_per_word == 1) {
                int field_module = bytes_per_channel * format.bits_per_word;
                int depth = (bytes_per_channel == 1 ? format.bit_fields[0].width : field_module);
                bool expect_success = (bytes_per_pixel % num_channels == 0 &&
                                       conforming_bit_fields(field_module, depth));
                ARCHON_CHECK_EQUAL(success, expect_success);
                if (success && expect_success) {
                    ARCHON_CHECK(format_3.is_valid());
                    ARCHON_CHECK_EQUAL(format_3.word_type, IntegerType::byte);
                    if (bytes_per_channel == 1) {
                        ARCHON_CHECK_EQUAL(format_3.bits_per_word, depth);
                    }
                    else {
                        ARCHON_CHECK_EQUAL(format_3.bits_per_word, format.bits_per_word);
                    }
                    ARCHON_CHECK_EQUAL(format_3.words_per_channel, bytes_per_channel);
                    ARCHON_CHECK_EQUAL(format_3.word_order, format.word_order);
                    ARCHON_CHECK_EQUAL(format_3.channel_conf.color_space, format.channel_conf.color_space);
                    ARCHON_CHECK_EQUAL(format_3.channel_conf.has_alpha, format.channel_conf.has_alpha);
                    ARCHON_CHECK_EQUAL(format_3.channel_conf.alpha_first, format.channel_conf.alpha_first);
                    if (format.word_order == core::Endianness::big) {
                        ARCHON_CHECK_EQUAL(format_3.channel_conf.reverse_order, format.channel_conf.reverse_order);
                    }
                    else {
                        ARCHON_CHECK_EQUAL(format_3.channel_conf.reverse_order, !format.channel_conf.reverse_order);
                    }
                    ARCHON_CHECK(equivalent_formats(format, format_3));
                }
            }
            else {
                int bits_per_byte = core::int_width<char>();
                bool all_bits_used = (format.bits_per_word == bytes_per_word * bits_per_byte);
                core::Endianness byte_order = {};
                bool have_byte_order = image::BufferFormat::try_get_byte_order(format.word_type, byte_order);
                int field_module = bytes_per_channel * bits_per_byte;
                int depth = field_module;
                bool expect_success = (all_bits_used &&
                                       bytes_per_pixel % num_channels == 0 &&
                                       have_byte_order &&
                                       (format.words_per_pixel == 1 || format.words_per_pixel == num_channels ||
                                        format.word_order == byte_order) &&
                                       conforming_bit_fields(field_module, depth));
                ARCHON_CHECK_EQUAL(success, expect_success);
                if (success && expect_success) {
                    ARCHON_CHECK(format_3.is_valid());
                    ARCHON_CHECK_EQUAL(format_3.word_type, IntegerType::byte);
                    ARCHON_CHECK_EQUAL(format_3.bits_per_word, bits_per_byte);
                    ARCHON_CHECK_EQUAL(format_3.words_per_channel, bytes_per_channel);
                    ARCHON_CHECK_EQUAL(format_3.word_order, byte_order);
                    ARCHON_CHECK_EQUAL(format_3.channel_conf.color_space, format.channel_conf.color_space);
                    ARCHON_CHECK_EQUAL(format_3.channel_conf.has_alpha, format.channel_conf.has_alpha);
                    ARCHON_CHECK_EQUAL(format_3.channel_conf.alpha_first, format.channel_conf.alpha_first);
                    bool reverse_channel_ordeer = ((format.words_per_pixel == num_channels ? format.word_order :
                                                    byte_order) != core::Endianness::big);
                    if (reverse_channel_ordeer) {
                        ARCHON_CHECK_EQUAL(format_3.channel_conf.reverse_order, !format.channel_conf.reverse_order);
                    }
                    else {
                        ARCHON_CHECK_EQUAL(format_3.channel_conf.reverse_order, format.channel_conf.reverse_order);
                    }
                    ARCHON_CHECK(equivalent_formats(format, format_3));
                }
            }
        }

        // Cast to other word types
        for (auto word_type : g_other_integer_types) {
            if (word_type == format.word_type || word_type == IntegerType::byte)
                continue;
            ARCHON_CHECK_NOT(format_2.try_cast_to(format_3, word_type));
        }
    };

    bool thorough = false;
    generate_packed_test_formats(word_type, thorough, [&](const PackedFormat& format) {
        test(format);
    });
}


ARCHON_TEST(Image_BufferFormat_TryCastTo_PackedToInteger_Extra)
{
    // A suitably arranged packed format with N times M words per bit compound and with N
    // channels must be representable as an integer format using the same word type and
    // using M words per channel
    {
        // Case 1 of 4: One channel (N = 1) and one word per component in integer format (M = 1)
        using word_type = int;
        IntegerType word_type_2 = {};
        bool success = image::BufferFormat::try_map_integer_type<word_type>(word_type_2);
        ARCHON_ASSERT(success);
        int bits_per_word = 12;
        int words_per_pixel = 1;
        core::Endianness word_order = core::Endianness::big; // Immaterial
        int bits_per_field = 10;
        image::BitField bit_fields[] = {
            { bits_per_field, 0 }, // Luminance
        };
        const image::ColorSpace& color_space = image::ColorSpace::get_lum();
        bool has_alpha_channel = false;
        bool alpha_channel_first = false; // Immaterial
        bool reverse_channel_order = false; // Immaterial
        image::BufferFormat format = {};
        format.set_packed_format(word_type_2, bits_per_word, words_per_pixel, word_order, bit_fields, color_space,
                                 has_alpha_channel, alpha_channel_first, reverse_channel_order);
        IntegerFormat integer = {};
        if (ARCHON_LIKELY(ARCHON_CHECK(format.try_cast_to(integer, word_type_2)))) {
            ARCHON_CHECK_EQUAL(integer.word_type, word_type_2);
            ARCHON_CHECK_EQUAL(integer.bits_per_word, bits_per_field);
            ARCHON_CHECK_EQUAL(integer.words_per_channel, 1);
            ARCHON_CHECK_EQUAL(integer.channel_conf.color_space, &color_space);
            ARCHON_CHECK_EQUAL(integer.channel_conf.has_alpha, false);
        }
    }
    {
        // Case 2 of 4: Two channels (N = 2) and one word per component in integer format (M = 1)
        using word_type = int;
        IntegerType word_type_2 = {};
        bool success = image::BufferFormat::try_map_integer_type<word_type>(word_type_2);
        ARCHON_ASSERT(success);
        int bits_per_word = 12;
        int words_per_pixel = 2;
        core::Endianness word_order = core::Endianness::big;
        int bits_per_field = 10;
        image::BitField bit_fields[] = {
            { bits_per_field, 2 }, // Luminance
            { bits_per_field, 0 }, // Alpha
        };
        const image::ColorSpace& color_space = image::ColorSpace::get_lum();
        bool has_alpha_channel = true;
        bool alpha_channel_first = false;
        bool reverse_channel_order = false; // Immaterial
        image::BufferFormat format = {};
        format.set_packed_format(word_type_2, bits_per_word, words_per_pixel, word_order, bit_fields, color_space,
                                 has_alpha_channel, alpha_channel_first, reverse_channel_order);
        IntegerFormat integer = {};
        if (ARCHON_LIKELY(ARCHON_CHECK(format.try_cast_to(integer, word_type_2)))) {
            ARCHON_CHECK_EQUAL(integer.word_type, word_type_2);
            ARCHON_CHECK_EQUAL(integer.bits_per_word, bits_per_field);
            ARCHON_CHECK_EQUAL(integer.words_per_channel, 1);
            ARCHON_CHECK_EQUAL(integer.channel_conf.color_space, &color_space);
            ARCHON_CHECK_EQUAL(integer.channel_conf.has_alpha, true);
            ARCHON_CHECK_EQUAL(integer.channel_conf.alpha_first, false);
        }
    }
    for (core::Endianness word_order : { core::Endianness::big, core::Endianness::little }) {
        // Case 3 of 4: One channel (N = 1) and two words per component in integer format (M = 2)
        using word_type = int;
        IntegerType word_type_2 = {};
        bool success = image::BufferFormat::try_map_integer_type<word_type>(word_type_2);
        ARCHON_ASSERT(success);
        int bits_per_word = 12;
        int words_per_pixel = 2;
        int bits_per_field = 2 * bits_per_word;
        image::BitField bit_fields[] = {
            { bits_per_field, 0 }, // Luminance
        };
        const image::ColorSpace& color_space = image::ColorSpace::get_lum();
        bool has_alpha_channel = false;
        bool alpha_channel_first = false; // Immaterial
        bool reverse_channel_order = false; // Immaterial
        image::BufferFormat format = {};
        format.set_packed_format(word_type_2, bits_per_word, words_per_pixel, word_order, bit_fields, color_space,
                                 has_alpha_channel, alpha_channel_first, reverse_channel_order);
        IntegerFormat integer = {};
        if (ARCHON_LIKELY(ARCHON_CHECK(format.try_cast_to(integer, word_type_2)))) {
            ARCHON_CHECK_EQUAL(integer.word_type, word_type_2);
            ARCHON_CHECK_EQUAL(integer.bits_per_word, bits_per_word);
            ARCHON_CHECK_EQUAL(integer.words_per_channel, 2);
            ARCHON_CHECK_EQUAL(integer.word_order, word_order);
            ARCHON_CHECK_EQUAL(integer.channel_conf.color_space, &color_space);
            ARCHON_CHECK_EQUAL(integer.channel_conf.has_alpha, false);
        }
    }
    for (core::Endianness word_order : { core::Endianness::big, core::Endianness::little }) {
        // Case 4 of 4: Two channels (N = 2) and two words per component in integer format (M = 2)
        using word_type = int;
        IntegerType word_type_2 = {};
        bool success = image::BufferFormat::try_map_integer_type<word_type>(word_type_2);
        ARCHON_ASSERT(success);
        int bits_per_word = 12;
        int words_per_pixel = 4;
        int bits_per_field = 2 * bits_per_word;
        image::BitField bit_fields[] = {
            { bits_per_field, 0 }, // Luminance
            { bits_per_field, 0 }, // Alpha
        };
        const image::ColorSpace& color_space = image::ColorSpace::get_lum();
        bool has_alpha_channel = true;
        bool alpha_channel_first = false;
        bool reverse_channel_order = false; // Immaterial
        image::BufferFormat format = {};
        format.set_packed_format(word_type_2, bits_per_word, words_per_pixel, word_order, bit_fields, color_space,
                                 has_alpha_channel, alpha_channel_first, reverse_channel_order);
        IntegerFormat integer = {};
        if (ARCHON_LIKELY(ARCHON_CHECK(format.try_cast_to(integer, word_type_2)))) {
            ARCHON_CHECK_EQUAL(integer.word_type, word_type_2);
            ARCHON_CHECK_EQUAL(integer.bits_per_word, bits_per_word);
            ARCHON_CHECK_EQUAL(integer.words_per_channel, 2);
            ARCHON_CHECK_EQUAL(integer.word_order, word_order);
            ARCHON_CHECK_EQUAL(integer.channel_conf.color_space, &color_space);
            ARCHON_CHECK_EQUAL(integer.channel_conf.has_alpha, true);
            ARCHON_CHECK_EQUAL(integer.channel_conf.alpha_first, false);
        }
    }

    // A suitably arranged packed format with N channels, one word per bit compound, and a
    // word type that is made up of N bytes must be representable as an integer format using
    // "byte" as word type and one byte per channel
    if constexpr (!std::is_same_v<double_int_type, void>) {
        IntegerType word_type = {};
        bool success = image::BufferFormat::try_map_integer_type<double_int_type>(word_type);
        ARCHON_ASSERT(success);
        int bits_per_word = core::int_width<double_int_type>();
        int words_per_pixel = 1;
        core::Endianness word_order = core::Endianness::big; // Immaterial
        int bits_per_byte = core::int_width<char>();
        image::BitField bit_fields[] = {
            { bits_per_byte, 0 }, // Luminance
            { bits_per_byte, 0 }, // Alpha
        };
        const image::ColorSpace& color_space = image::ColorSpace::get_lum();
        bool has_alpha_channel = true;
        bool alpha_channel_first = false;
        bool reverse_channel_order = false; // Immaterial
        image::BufferFormat format = {};
        format.set_packed_format(word_type, bits_per_word, words_per_pixel, word_order, bit_fields, color_space,
                                 has_alpha_channel, alpha_channel_first, reverse_channel_order);
        IntegerFormat integer = {};
        if (ARCHON_LIKELY(ARCHON_CHECK(format.try_cast_to(integer, IntegerType::byte)))) {
            ARCHON_CHECK_EQUAL(integer.word_type, IntegerType::byte);
            ARCHON_CHECK_EQUAL(integer.bits_per_word, bits_per_byte);
            ARCHON_CHECK_EQUAL(integer.words_per_channel, 1);
            ARCHON_CHECK_EQUAL(integer.channel_conf.color_space, &color_space);
            ARCHON_CHECK_EQUAL(integer.channel_conf.has_alpha, true);
            ARCHON_CHECK_EQUAL(integer.channel_conf.alpha_first, false);
        }
    }
    if constexpr (!std::is_same_v<quadruple_int_type, void>) {
        IntegerType word_type = {};
        bool success = image::BufferFormat::try_map_integer_type<quadruple_int_type>(word_type);
        ARCHON_ASSERT(success);
        int bits_per_word = core::int_width<quadruple_int_type>();
        int words_per_pixel = 1;
        core::Endianness word_order = core::Endianness::big; // Immaterial
        int bits_per_byte = core::int_width<char>();
        image::BitField bit_fields[] = {
            { bits_per_byte, 0 }, // Red
            { bits_per_byte, 0 }, // Greeen
            { bits_per_byte, 0 }, // Blue
            { bits_per_byte, 0 }, // Alpha
        };
        const image::ColorSpace& color_space = image::ColorSpace::get_rgb();
        bool has_alpha_channel = true;
        bool alpha_channel_first = false;
        bool reverse_channel_order = false;
        image::BufferFormat format = {};
        format.set_packed_format(word_type, bits_per_word, words_per_pixel, word_order, bit_fields, color_space,
                                 has_alpha_channel, alpha_channel_first, reverse_channel_order);
        IntegerFormat integer = {};
        if (ARCHON_LIKELY(ARCHON_CHECK(format.try_cast_to(integer, IntegerType::byte)))) {
            ARCHON_CHECK_EQUAL(integer.word_type, IntegerType::byte);
            ARCHON_CHECK_EQUAL(integer.bits_per_word, bits_per_byte);
            ARCHON_CHECK_EQUAL(integer.words_per_channel, 1);
            ARCHON_CHECK_EQUAL(integer.channel_conf.color_space, &color_space);
            ARCHON_CHECK_EQUAL(integer.channel_conf.has_alpha, true);
            ARCHON_CHECK_EQUAL(integer.channel_conf.alpha_first, false);
        }
    }

    // A suitably arranged packed format with N channels, P words per bit compound, and Q
    // bytes per word must be representable as an integer format using "byte" as word type
    // and M bytes per channel so long as N times M is equal to P times Q and so long as the
    // word order in both formats is equal to the byte order of the word type in the packed
    // format

    // N = C, P = C, Q = X, M = X
    {
        using word_type = int;
        core::Endianness byte_order = {};
        if (ARCHON_LIKELY(core::try_get_byte_order<word_type>(byte_order))) {
            IntegerType word_type_2 = {};
            bool success = image::BufferFormat::try_map_integer_type<word_type>(word_type_2);
            ARCHON_ASSERT(success);
            int bits_per_word = core::int_width<word_type>();
            int words_per_pixel = 2;
            core::Endianness word_order = byte_order;
            int depth = bits_per_word;
            image::BitField bit_fields[] = {
                { depth, 0 }, // Luminance
                { depth, 0 }, // Alpha
            };
            const image::ColorSpace& color_space = image::ColorSpace::get_lum();
            bool has_alpha_channel = true;
            bool alpha_channel_first = false;
            bool reverse_channel_order = false; // Immaterial
            image::BufferFormat format = {};
            format.set_packed_format(word_type_2, bits_per_word, words_per_pixel, word_order, bit_fields, color_space,
                                     has_alpha_channel, alpha_channel_first, reverse_channel_order);
            IntegerFormat integer = {};
            if (ARCHON_LIKELY(ARCHON_CHECK(format.try_cast_to(integer, IntegerType::byte)))) {
                int bits_per_byte = core::int_width<char>();
                int bytes_per_word = sizeof (word_type);
                ARCHON_CHECK_EQUAL(integer.word_type, IntegerType::byte);
                ARCHON_CHECK_EQUAL(integer.bits_per_word, bits_per_byte);
                ARCHON_CHECK_EQUAL(integer.words_per_channel, bytes_per_word);
                ARCHON_CHECK_EQUAL(integer.word_order, byte_order);
                ARCHON_CHECK_EQUAL(integer.channel_conf.color_space, &color_space);
                ARCHON_CHECK_EQUAL(integer.channel_conf.has_alpha, true);
                ARCHON_CHECK_EQUAL(integer.channel_conf.alpha_first, false);
            }
        }
    }

    // Negative cases
    for (core::Endianness word_order : { core::Endianness::big, core::Endianness::little }) {
        // Case 1: Badly arranged fields in packed format: `bits_per_field` too low
        using word_type = int;
        IntegerType word_type_2 = {};
        bool success = image::BufferFormat::try_map_integer_type<word_type>(word_type_2);
        ARCHON_ASSERT(success);
        int bits_per_word = 12;
        int words_per_pixel = 2;
        int bits_per_field = 2 * bits_per_word - 1;
        image::BitField bit_fields[] = {
            { bits_per_field, 0 }, // Luminance
        };
        const image::ColorSpace& color_space = image::ColorSpace::get_lum();
        bool has_alpha_channel = false;
        bool alpha_channel_first = false; // Immaterial
        bool reverse_channel_order = false; // Immaterial
        image::BufferFormat format = {};
        format.set_packed_format(word_type_2, bits_per_word, words_per_pixel, word_order, bit_fields, color_space,
                                 has_alpha_channel, alpha_channel_first, reverse_channel_order);
        IntegerFormat integer = {};
        ARCHON_CHECK_NOT(format.try_cast_to(integer, word_type_2));
    }
    for (core::Endianness word_order : { core::Endianness::big, core::Endianness::little }) {
        // Case 2: Badly arranged fields in packed format: `bits_per_field` too low
        using word_type = int;
        IntegerType word_type_2 = {};
        bool success = image::BufferFormat::try_map_integer_type<word_type>(word_type_2);
        ARCHON_ASSERT(success);
        int bits_per_word = 12;
        int words_per_pixel = 4;
        int bits_per_field = 2 * bits_per_word - 1;
        image::BitField bit_fields[] = {
            { bits_per_field, 0 }, // Luminance
            { bits_per_field, 0 }, // Alpha
        };
        const image::ColorSpace& color_space = image::ColorSpace::get_lum();
        bool has_alpha_channel = true;
        bool alpha_channel_first = false;
        bool reverse_channel_order = false; // Immaterial
        image::BufferFormat format = {};
        format.set_packed_format(word_type_2, bits_per_word, words_per_pixel, word_order, bit_fields, color_space,
                                 has_alpha_channel, alpha_channel_first, reverse_channel_order);
        IntegerFormat integer = {};
        ARCHON_CHECK_NOT(format.try_cast_to(integer, word_type_2));
    }
}


ARCHON_TEST_BATCH(Image_BufferFormat_TryCastTo_PackedToPacked, abridged_integer_type_variants)
{
    IntegerType word_type = test_value;

    auto test = [&, &parent_test_context = test_context](const PackedFormat& format) {
        ARCHON_TEST_TRAIL(parent_test_context, wrap(format));

        image::BufferFormat format_2 = format;
        PackedFormat format_3 = {};

        // Cast to same word type
        {
            bool success = format_2.try_cast_to(format_3, format.word_type);
            bool expect_success = true;
            ARCHON_CHECK_EQUAL(success, expect_success);
            if (success && expect_success)
                ARCHON_CHECK_EQUAL(wrap(format_3), wrap(format));
        }

        // Cast to bytes
        if (format.word_type != IntegerType::byte) {
            bool success = format_2.try_cast_to(format_3, IntegerType::byte);
            int num_channels = format.channel_conf.get_num_channels();
            int bytes_per_word = image::BufferFormat::get_bytes_per_word(format.word_type);
            if (bytes_per_word == 1) {
                bool expect_success = true;
                ARCHON_CHECK_EQUAL(success, expect_success);
                if (success && expect_success) {
                    ARCHON_CHECK(format_3.is_valid());
                    ARCHON_CHECK_EQUAL(format_3.word_type, IntegerType::byte);
                    ARCHON_CHECK_EQUAL(format_3.bits_per_word, format.bits_per_word);
                    ARCHON_CHECK_EQUAL(format_3.words_per_pixel, format.words_per_pixel);
                    ARCHON_CHECK_EQUAL(format_3.word_order, format.word_order);
                    for (int i = 0; i < image::BufferFormat::max_bit_fields; ++i) {
                        const image::BitField& field = format_3.bit_fields[i];
                        if (i < num_channels) {
                            ARCHON_CHECK_EQUAL(field, format.bit_fields[i]);
                        }
                        else {
                            ARCHON_CHECK_EQUAL(field.width, 0);
                            ARCHON_CHECK_EQUAL(field.gap, 0);
                        }
                    }
                    ARCHON_CHECK_EQUAL(wrap(format_3.channel_conf), wrap(format.channel_conf));
                }
            }
            else {
                int bits_per_byte = core::int_width<char>();
                core::Endianness byte_order = {};
                bool have_byte_order = image::BufferFormat::try_get_byte_order(format.word_type, byte_order);
                bool reverse_channel_order = false;
                bool switch_alpha_channel_side = false;
                auto satisfies_a_bit_compound_condition = [&]() {
                    ARCHON_ASSERT(have_byte_order);
                    if (format.words_per_pixel == 1)
                        return true;
                    if (format.word_order == byte_order && format.bits_per_word == bytes_per_word * bits_per_byte)
                        return true;
                    int field_counts[image::BufferFormat::max_bit_fields] {};
                    int num_field_counts = 0;
                    int prev_word_index = -1;
                    int offset = 0;
                    for (int i = 0; i < num_channels; ++i) {
                        int j = num_channels - 1 - i;
                        const image::BitField& field = format.bit_fields[j];
                        int bit_pos = field.gap + offset;
                        int word_index = bit_pos / format.bits_per_word;
                        int bit_pos_2 = bit_pos % format.bits_per_word;
                        if (field.width > format.bits_per_word - bit_pos_2)
                            return false;
                        if (word_index != prev_word_index) {
                            num_field_counts += 1;
                            prev_word_index = word_index;
                        }
                        ARCHON_ASSERT(num_field_counts > 0);
                        field_counts[num_field_counts - 1] += 1;
                        offset = field.width + bit_pos;
                    }
                    if (format.word_order == byte_order)
                        return true;
                    if (num_field_counts == 1)
                        return true;
                    if (num_field_counts == num_channels) {
                        reverse_channel_order = true;
                        return true;
                    }
                    const image::BufferFormat::ChannelConf& channel_conf = format.channel_conf;
                    bool cond = (num_field_counts == 2 && channel_conf.has_alpha &&
                                 field_counts[channel_conf.alpha_first == channel_conf.reverse_order ? 0 : 1] == 1);
                    if (cond) {
                        switch_alpha_channel_side = true;
                        return true;
                    }
                    return false;
                };
                bool expect_success = (have_byte_order &&
                                       satisfies_a_bit_compound_condition());
                ARCHON_CHECK_EQUAL(success, expect_success);
                if (success && expect_success) {
                    ARCHON_CHECK(format_3.is_valid());
                    ARCHON_CHECK_EQUAL(format_3.word_type, IntegerType::byte);
                    ARCHON_CHECK_EQUAL(format_3.bits_per_word, bits_per_byte);
                    ARCHON_CHECK_EQUAL(format_3.words_per_pixel, format.words_per_pixel * bytes_per_word);
                    ARCHON_CHECK_EQUAL(format_3.word_order, byte_order);
                    struct Field {
                        int byte_index;
                        int bit_pos;
                        int width;
                    };
                    // In channel storage order for target format
                    Field fields[image::BufferFormat::max_bit_fields] {};
                    {
                        int offset = 0;
                        for (int i = 0; i < num_channels; ++i) {
                            int j = num_channels - 1 - i;
                            const image::BitField& field = format.bit_fields[j];
                            int bit_pos = field.gap + offset;
                            int word_index = bit_pos / format.bits_per_word;
                            int bit_pos_2  = bit_pos % format.bits_per_word;
                            if (format.word_order == core::Endianness::big)
                                word_index = format.words_per_pixel - 1 - word_index;
                            int byte_index = bit_pos_2 / bits_per_byte;
                            int bit_pos_3  = bit_pos_2 % bits_per_byte;
                            if (byte_order == core::Endianness::big)
                                byte_index = bytes_per_word - 1 - byte_index;
                            int byte_index_2 = word_index * bytes_per_word + byte_index;
                            int k = j;
                            if (switch_alpha_channel_side) {
                                ARCHON_ASSERT(format.channel_conf.has_alpha);
                                if (format.channel_conf.alpha_first != format.channel_conf.reverse_order) {
                                    k = (k + num_channels - 1) % num_channels; // front -> back
                                }
                                else {
                                    k = (k + 1) % num_channels; // back -> front
                                }
                            }
                            else if (reverse_channel_order) {
                                k = num_channels - 1 - k;
                            }
                            fields[k] = { byte_index_2, bit_pos_3, field.width };
                            offset = field.width + bit_pos;
                        }
                    }
                    {
                        int offset = 0;
                        for (int i = 0; i < image::BufferFormat::max_bit_fields; ++i) {
                            int j = image::BufferFormat::max_bit_fields - 1 - i;
                            const image::BitField& field = format_3.bit_fields[j];
                            if (j >= num_channels) {
                                ARCHON_CHECK_EQUAL(field.width, 0);
                                ARCHON_CHECK_EQUAL(field.gap, 0);
                                continue;
                            }
                            int bit_pos = field.gap + offset;
                            int byte_index = bit_pos / format_3.bits_per_word;
                            int bit_pos_2 = bit_pos % format_3.bits_per_word;
                            if (format_3.word_order == core::Endianness::big)
                                byte_index = format_3.words_per_pixel - 1 - byte_index;
                            const Field& field_2  = fields[j];
                            ARCHON_CHECK_EQUAL(byte_index, field_2.byte_index);
                            ARCHON_CHECK_EQUAL(bit_pos_2, field_2.bit_pos);
                            ARCHON_CHECK_EQUAL(field.width, field_2.width);
                            offset = field.width + bit_pos;
                        }
                    }
                    ARCHON_CHECK_EQUAL(format_3.channel_conf.color_space, format.channel_conf.color_space);
                    ARCHON_CHECK_EQUAL(format_3.channel_conf.has_alpha, format.channel_conf.has_alpha);
                    if (switch_alpha_channel_side) {
                        ARCHON_CHECK_EQUAL(format_3.channel_conf.alpha_first, !format.channel_conf.alpha_first);
                    }
                    else {
                        ARCHON_CHECK_EQUAL(format_3.channel_conf.alpha_first, format.channel_conf.alpha_first);
                    }
                    if (reverse_channel_order) {
                        ARCHON_CHECK_EQUAL(format_3.channel_conf.reverse_order, !format.channel_conf.reverse_order);
                    }
                    else {
                        ARCHON_CHECK_EQUAL(format_3.channel_conf.reverse_order, format.channel_conf.reverse_order);
                    }
                    ARCHON_CHECK(equivalent_formats(format, format_3));
                }
            }
        }

        // Cast to other word types
        for (auto word_type : g_other_integer_types) {
            if (word_type == format.word_type || word_type == IntegerType::byte)
                continue;
            ARCHON_CHECK_NOT(format_2.try_cast_to(format_3, word_type));
        }
    };

    bool thorough = false;
    generate_packed_test_formats(word_type, thorough, [&](const PackedFormat& format) {
        test(format);
    });
}


ARCHON_TEST_BATCH(Image_BufferFormat_TryCastTo_PackedToSubword, abridged_integer_type_variants)
{
    IntegerType word_type = test_value;

    auto test = [&, &parent_test_context = test_context](const PackedFormat& format) {
        ARCHON_TEST_TRAIL(parent_test_context, wrap(format));

        int num_channels = format.channel_conf.get_num_channels();
        int depth = {};
        auto conforming_bit_fields = [&] {
            int d = format.bit_fields[0].width;
            for (int i = 0; i < num_channels; ++i) {
                const image::BitField& field = format.bit_fields[i];
                if (field.width != d || field.gap > 0)
                    return false;
            }
            depth = d;
            return true;
        };

        image::BufferFormat format_2 = format;
        SubwordFormat format_3 = {};

        // Cast to same word type
        {
            bool success = format_2.try_cast_to(format_3, format.word_type);
            bool expect_success = (format.words_per_pixel == 1 && conforming_bit_fields());
            ARCHON_CHECK_EQUAL(success, expect_success);
            if (success && expect_success) {
                ARCHON_CHECK(format_3.is_valid());
                ARCHON_CHECK_EQUAL(format_3.word_type, format.word_type);
                ARCHON_CHECK_EQUAL(format_3.bits_per_channel, depth);
                ARCHON_CHECK_EQUAL(format_3.pixels_per_word, 1);
                ARCHON_CHECK_EQUAL(format_3.bit_order, core::Endianness::big);
                ARCHON_CHECK_EQUAL(format_3.word_aligned_rows, false);
                ARCHON_CHECK_EQUAL(wrap(format_3.channel_conf), wrap(format.channel_conf));
                ARCHON_CHECK(equivalent_formats(format, format_3));
            }
        }

        // Cast to bytes
        if (format.word_type != IntegerType::byte) {
            bool success = format_2.try_cast_to(format_3, IntegerType::byte);
            int bytes_per_word = image::BufferFormat::get_bytes_per_word(format.word_type);
            bool expect_success = (bytes_per_word == 1 && format.words_per_pixel == 1 && conforming_bit_fields());
            ARCHON_CHECK_EQUAL(success, expect_success);
            if (success && expect_success) {
                ARCHON_CHECK(format_3.is_valid());
                ARCHON_CHECK_EQUAL(format_3.word_type, IntegerType::byte);
                ARCHON_CHECK_EQUAL(format_3.bits_per_channel, depth);
                ARCHON_CHECK_EQUAL(format_3.pixels_per_word, 1);
                ARCHON_CHECK_EQUAL(format_3.bit_order, core::Endianness::big);
                ARCHON_CHECK_EQUAL(format_3.word_aligned_rows, false);
                ARCHON_CHECK_EQUAL(wrap(format_3.channel_conf), wrap(format.channel_conf));
                ARCHON_CHECK(equivalent_formats(format, format_3));
            }
        }

        // Cast to other word types
        for (auto word_type : g_other_integer_types) {
            if (word_type == format.word_type || word_type == IntegerType::byte)
                continue;
            ARCHON_CHECK_NOT(format_2.try_cast_to(format_3, word_type));
        }
    };

    bool thorough = false;
    generate_packed_test_formats(word_type, thorough, [&](const PackedFormat& format) {
        test(format);
    });
}


ARCHON_TEST_BATCH(Image_BufferFormat_TryCastTo_SubwordToInteger, abridged_integer_type_variants)
{
    IntegerType word_type = test_value;

    auto test = [&, &parent_test_context = test_context](const SubwordFormat& format) {
        ARCHON_TEST_TRAIL(parent_test_context, wrap(format));

        int num_channels = format.channel_conf.get_num_channels();

        image::BufferFormat format_2 = format;
        IntegerFormat format_3 = {};

        // Cast to same word type
        {
            bool success = format_2.try_cast_to(format_3, format.word_type);
            bool expect_success = (format.pixels_per_word == 1 && num_channels == 1);
            ARCHON_CHECK_EQUAL(success, expect_success);
            if (success && expect_success) {
                ARCHON_CHECK(format_3.is_valid());
                ARCHON_CHECK_EQUAL(format_3.word_type, format.word_type);
                ARCHON_CHECK_EQUAL(format_3.bits_per_word, format.bits_per_channel);
                ARCHON_CHECK_EQUAL(format_3.words_per_channel, 1);
                ARCHON_CHECK_EQUAL(format_3.word_order, core::Endianness::big);
                ARCHON_CHECK_EQUAL(wrap(format_3.channel_conf), wrap(format.channel_conf));
                ARCHON_CHECK(equivalent_formats(format, format_3));
            }
        }

        // Cast to bytes
        if (format.word_type != IntegerType::byte) {
            bool success = format_2.try_cast_to(format_3, IntegerType::byte);
            int bytes_per_word = image::BufferFormat::get_bytes_per_word(format.word_type);
            if (bytes_per_word == 1) {
                bool expect_success = (format.pixels_per_word == 1 && num_channels == 1);
                ARCHON_CHECK_EQUAL(success, expect_success);
                if (success && expect_success) {
                    ARCHON_CHECK(format_3.is_valid());
                    ARCHON_CHECK_EQUAL(format_3.word_type, IntegerType::byte);
                    ARCHON_CHECK_EQUAL(format_3.bits_per_word, format.bits_per_channel);
                    ARCHON_CHECK_EQUAL(format_3.words_per_channel, 1);
                    ARCHON_CHECK_EQUAL(format_3.word_order, core::Endianness::big);
                    ARCHON_CHECK_EQUAL(wrap(format_3.channel_conf), wrap(format.channel_conf));
                    ARCHON_CHECK(equivalent_formats(format, format_3));
                }
            }
            else {
                int bits_per_byte = core::int_width<char>();
                int used_bits_per_word = format.pixels_per_word * num_channels * format.bits_per_channel;
                bool all_bits_used = (used_bits_per_word == bytes_per_word * bits_per_byte);
                core::Endianness byte_order = {};
                bool have_byte_order = image::BufferFormat::try_get_byte_order(format.word_type, byte_order);
                bool expect_success = (format.bits_per_channel % bits_per_byte == 0 &&
                                       all_bits_used &&
                                       have_byte_order &&
                                       (format.pixels_per_word == 1 ||
                                        (format.bit_order == byte_order && !format.word_aligned_rows)));
                ARCHON_CHECK_EQUAL(success, expect_success);
                if (success && expect_success) {
                    ARCHON_CHECK(format_3.is_valid());
                    ARCHON_CHECK_EQUAL(format_3.word_type, IntegerType::byte);
                    ARCHON_CHECK_EQUAL(format_3.bits_per_word, bits_per_byte);
                    ARCHON_CHECK_EQUAL(format_3.words_per_channel, format.bits_per_channel / bits_per_byte);
                    ARCHON_CHECK_EQUAL(format_3.word_order, byte_order);
                    ARCHON_CHECK_EQUAL(format_3.channel_conf.color_space, format.channel_conf.color_space);
                    ARCHON_CHECK_EQUAL(format_3.channel_conf.has_alpha, format.channel_conf.has_alpha);
                    ARCHON_CHECK_EQUAL(format_3.channel_conf.alpha_first, format.channel_conf.alpha_first);
                    if (byte_order == core::Endianness::big) {
                        ARCHON_CHECK_EQUAL(format_3.channel_conf.reverse_order, format.channel_conf.reverse_order);
                    }
                    else {
                        ARCHON_CHECK_EQUAL(format_3.channel_conf.reverse_order, !format.channel_conf.reverse_order);
                    }
                    ARCHON_CHECK(equivalent_formats(format, format_3));
                }
            }
        }

        // Cast to other word types
        for (auto word_type : g_other_integer_types) {
            if (word_type == format.word_type || word_type == IntegerType::byte)
                continue;
            ARCHON_CHECK_NOT(format_2.try_cast_to(format_3, word_type));
        }
    };

    generate_subword_test_formats(word_type, [&](const SubwordFormat& format) {
        test(format);
    });
}


ARCHON_TEST_BATCH(Image_BufferFormat_TryCastTo_SubwordToPacked, abridged_integer_type_variants)
{
    IntegerType word_type = test_value;

    auto test = [&, &parent_test_context = test_context](const SubwordFormat& format) {
        ARCHON_TEST_TRAIL(parent_test_context, wrap(format));

        int num_channels = format.channel_conf.get_num_channels();
        int bits_per_pixel = num_channels * format.bits_per_channel;

        image::BufferFormat format_2 = format;
        PackedFormat format_3 = {};

        // Cast to same word type
        {
            bool success = format_2.try_cast_to(format_3, format.word_type);
            bool expect_success = (format.pixels_per_word == 1 &&
                                   num_channels <= image::BufferFormat::max_bit_fields);
            ARCHON_CHECK_EQUAL(success, expect_success);
            if (success && expect_success) {
                ARCHON_CHECK(format_3.is_valid());
                ARCHON_CHECK_EQUAL(format_3.word_type, format.word_type);
                ARCHON_CHECK_EQUAL(format_3.bits_per_word, bits_per_pixel);
                ARCHON_CHECK_EQUAL(format_3.words_per_pixel, 1);
                ARCHON_CHECK_EQUAL(format_3.word_order, core::Endianness::big);
                for (int i = 0; i < image::BufferFormat::max_bit_fields; ++i) {
                    const image::BitField& bit_field = format_3.bit_fields[i];
                    ARCHON_CHECK_EQUAL(bit_field.width, (i < num_channels ? format.bits_per_channel : 0));
                    ARCHON_CHECK_EQUAL(bit_field.gap, 0);
                }
                ARCHON_CHECK_EQUAL(wrap(format_3.channel_conf), wrap(format.channel_conf));
                ARCHON_CHECK(equivalent_formats(format, format_3));
            }
        }

        // Cast to bytes
        if (format.word_type != IntegerType::byte) {
            bool success = format_2.try_cast_to(format_3, IntegerType::byte);
            int bytes_per_word = image::BufferFormat::get_bytes_per_word(format.word_type);
            if (bytes_per_word == 1) {
                bool expect_success = (format.pixels_per_word == 1 &&
                                       num_channels <= image::BufferFormat::max_bit_fields);
                ARCHON_CHECK_EQUAL(success, expect_success);
                if (success && expect_success) {
                    ARCHON_CHECK(format_3.is_valid());
                    ARCHON_CHECK_EQUAL(format_3.word_type, IntegerType::byte);
                    ARCHON_CHECK_EQUAL(format_3.bits_per_word, bits_per_pixel);
                    ARCHON_CHECK_EQUAL(format_3.words_per_pixel, 1);
                    ARCHON_CHECK_EQUAL(format_3.word_order, core::Endianness::big);
                    for (int i = 0; i < image::BufferFormat::max_bit_fields; ++i) {
                        const image::BitField& bit_field = format_3.bit_fields[i];
                        ARCHON_CHECK_EQUAL(bit_field.width, (i < num_channels ? format.bits_per_channel : 0));
                        ARCHON_CHECK_EQUAL(bit_field.gap, 0);
                    }
                    ARCHON_CHECK_EQUAL(wrap(format_3.channel_conf), wrap(format.channel_conf));
                    ARCHON_CHECK(equivalent_formats(format, format_3));
                }
            }
            else {
                int bits_per_byte = core::int_width<char>();
                int used_bits_per_word = format.pixels_per_word * bits_per_pixel;
                bool all_bits_used = (used_bits_per_word == bytes_per_word * bits_per_byte);
                core::Endianness byte_order = {};
                bool have_byte_order = image::BufferFormat::try_get_byte_order(format.word_type, byte_order);
                bool expect_success = (have_byte_order &&
                                       (format.pixels_per_word == 1 ||
                                        (bits_per_pixel % bits_per_byte == 0 &&
                                         all_bits_used &&
                                         format.bit_order == byte_order &&
                                         !format.word_aligned_rows)) &&
                                       num_channels <= image::BufferFormat::max_bit_fields);
                ARCHON_CHECK_EQUAL(success, expect_success);
                if (success && expect_success) {
                    ARCHON_CHECK(format_3.is_valid());
                    ARCHON_CHECK_EQUAL(format_3.word_type, IntegerType::byte);
                    ARCHON_CHECK_EQUAL(format_3.bits_per_word, bits_per_byte);
                    ARCHON_CHECK_EQUAL(format_3.words_per_pixel, bytes_per_word / format.pixels_per_word);
                    ARCHON_CHECK_EQUAL(format_3.word_order, byte_order);
                    for (int i = 0; i < image::BufferFormat::max_bit_fields; ++i) {
                        const image::BitField& bit_field = format_3.bit_fields[i];
                        ARCHON_CHECK_EQUAL(bit_field.width, (i < num_channels ? format.bits_per_channel : 0));
                        ARCHON_CHECK_EQUAL(bit_field.gap, 0);
                    }
                    ARCHON_CHECK_EQUAL(wrap(format_3.channel_conf), wrap(format.channel_conf));
                    ARCHON_CHECK(equivalent_formats(format, format_3));
                }
            }
        }

        // Cast to other word types
        for (auto word_type : g_other_integer_types) {
            if (word_type == format.word_type || word_type == IntegerType::byte)
                continue;
            ARCHON_CHECK_NOT(format_2.try_cast_to(format_3, word_type));
        }
    };

    generate_subword_test_formats(word_type, [&](const SubwordFormat& format) {
        test(format);
    });
}


ARCHON_TEST_BATCH(Image_BufferFormat_TryCastTo_SubwordToSubword, abridged_integer_type_variants)
{
    IntegerType word_type = test_value;

    auto test = [&, &parent_test_context = test_context](const SubwordFormat& format) {
        ARCHON_TEST_TRAIL(parent_test_context, wrap(format));

        image::BufferFormat format_2 = format;
        SubwordFormat format_3 = {};

        // Cast to same word type
        {
            bool success = format_2.try_cast_to(format_3, format.word_type);
            bool expect_success = true;
            ARCHON_CHECK_EQUAL(success, expect_success);
            if (success && expect_success)
                ARCHON_CHECK_EQUAL(wrap(format_3), wrap(format));
        }

        // Cast to bytes
        if (format.word_type != IntegerType::byte) {
            bool success = format_2.try_cast_to(format_3, IntegerType::byte);

            int bits_per_byte = core::int_width<char>();
            int bytes_per_word = image::BufferFormat::get_bytes_per_word(format.word_type);
            int num_channels = format.channel_conf.get_num_channels();
            int used_bits_per_word = format.pixels_per_word * num_channels * format.bits_per_channel;
            bool all_bits_used = (used_bits_per_word == bytes_per_word * bits_per_byte);
            core::Endianness byte_order = {};
            bool have_byte_order = image::BufferFormat::try_get_byte_order(format.word_type, byte_order);
            bool expect_success = (bytes_per_word == 1 ||
                                   (!format.word_aligned_rows &&
                                    all_bits_used &&
                                    format.pixels_per_word % bytes_per_word == 0 &&
                                    have_byte_order && byte_order == format.bit_order));
            ARCHON_CHECK_EQUAL(success, expect_success);
            if (success && expect_success) {
                ARCHON_CHECK(format_3.is_valid());
                ARCHON_CHECK_EQUAL(format_3.word_type, IntegerType::byte);
                ARCHON_CHECK_EQUAL(format_3.bits_per_channel, format.bits_per_channel);
                ARCHON_CHECK_EQUAL(format_3.pixels_per_word, format.pixels_per_word / bytes_per_word);
                ARCHON_CHECK_EQUAL(format_3.bit_order, format.bit_order);
                ARCHON_CHECK_EQUAL(format_3.word_aligned_rows, format.word_aligned_rows);
                ARCHON_CHECK_EQUAL(wrap(format_3.channel_conf), wrap(format.channel_conf));
                ARCHON_CHECK(equivalent_formats(format, format_3));
            }
        }

        // Cast to other word types
        for (auto word_type : g_other_integer_types) {
            if (word_type == format.word_type || word_type == IntegerType::byte)
                continue;
            ARCHON_CHECK_NOT(format_2.try_cast_to(format_3, word_type));
        }
    };

    generate_subword_test_formats(word_type, [&](const SubwordFormat& format) {
        test(format);
    });
}
