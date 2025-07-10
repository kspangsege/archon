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


#include <cstddef>
#include <utility>
#include <memory>
#include <random>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/format.hpp>
#include <archon/core/random.hpp>
#include <archon/core/endianness.hpp>
#include <archon/check.hpp>
#include <archon/image/geom.hpp>
#include <archon/image/iter.hpp>
#include <archon/image/tray.hpp>
#include <archon/image/comp_types.hpp>
#include <archon/image/comp_repr.hpp>
#include <archon/image/transfer_info.hpp>
#include <archon/image/indexed_pixel_format.hpp>
#include <archon/image/palettes.hpp>
#include <archon/image/test/comp_repr_utils.hpp>


using namespace archon;
namespace test = image::test;


namespace {


using Format_Short_1_16 = image::IndexedPixelFormat_1<short, 16>;
using Format_Short_2_8  = image::IndexedPixelFormat_2<short, 8>;
using Format_Short_4_4  = image::IndexedPixelFormat_4<short, 4>;
using Format_Short_8_2  = image::IndexedPixelFormat_8<short, 2>;

using Format_Short_4_4_LSB =
    image::IndexedPixelFormat<short, 4, 4, core::Endianness::little>;

using Format_Short_8_2_Char_8_2_BE =
    image::IndexedPixelFormat<short, 8, 2, core::Endianness::big, char, 8, 2>;

using Format_Short_8_2_Char_8_2_LE =
    image::IndexedPixelFormat<short, 8, 2, core::Endianness::big, char, 8, 2, core::Endianness::little>;

using Format_Long_5_6_Short_16_2 =
    image::IndexedPixelFormat<long, 5, 6, core::Endianness::big, short, 16, 2>;

using Format_Short_1_16_NCAR =
    image::IndexedPixelFormat<short, 1, 16, core::Endianness::big, short, 16, 1, core::Endianness::big, false>;

ARCHON_TEST_VARIANTS(variants,
                     ARCHON_TEST_TYPE(Format_Short_1_16,            Short_1_16),
                     ARCHON_TEST_TYPE(Format_Short_2_8,             Short_2_8),
                     ARCHON_TEST_TYPE(Format_Short_4_4,             Short_4_4),
                     ARCHON_TEST_TYPE(Format_Short_8_2,             Short_8_2),
                     ARCHON_TEST_TYPE(Format_Short_4_4_LSB,         Short_4_4_LSB),
                     ARCHON_TEST_TYPE(Format_Short_8_2_Char_8_2_BE, Short_8_2_Char_8_2_BE),
                     ARCHON_TEST_TYPE(Format_Short_8_2_Char_8_2_LE, Short_8_2_Char_8_2_LE),
                     ARCHON_TEST_TYPE(Format_Long_5_6_Short_16_2,   Long_5_6_Short_16_2),
                     ARCHON_TEST_TYPE(Format_Short_1_16_NCAR,       Short_1_16_NCAR));


} // unnamed namespace


ARCHON_TEST(Image_IndexedPixelFormat_GetTransferInfo)
{
    auto test = [](check::TestContext& parent_test_context, auto format, std::string_view label,
                   image::CompRepr comp_repr, const image::ColorSpace& color_space, bool has_alpha, int bit_depth) {
        ARCHON_TEST_TRAIL(parent_test_context, label);
        image::TransferInfo info = format.get_transfer_info();
        ARCHON_CHECK_EQUAL(info.comp_repr, comp_repr);
        ARCHON_CHECK_EQUAL(info.color_space, &color_space);
        ARCHON_CHECK_EQUAL(info.has_alpha, has_alpha);
        ARCHON_CHECK_EQUAL(info.bit_depth, bit_depth);
    };

    test(test_context, Format_Short_1_16(image::get_css16_palette()), "Format_Short_1_16", image::CompRepr::int8,
         image::ColorSpace::get_rgb(), true, 8);
}


ARCHON_TEST_BATCH(Image_IndexedPixelFormat_Read, variants)
{
    std::mt19937_64 random(test_context.seed_seq());

    using format_type = test_type;
    using word_type = typename format_type::word_type;
    using compound_type = typename format_type::compound_type;
    constexpr int bits_per_compound = format_type::bits_per_compound;
    using value_type = image::unpacked_type<compound_type, bits_per_compound>;
    constexpr int bits_per_pixel = format_type::bits_per_pixel;
    constexpr int pixels_per_compound = format_type::pixels_per_compound;
    constexpr int bits_per_word = format_type::bits_per_word;
    constexpr int words_per_compound = format_type::words_per_compound;
    constexpr value_type max_pixel = core::int_mask<value_type>(bits_per_pixel);
    constexpr image::CompRepr transf_repr = format_type::transf_repr;
    using transf_type = image::comp_type<transf_repr>;

    auto test_1 = [&](check::TestContext& parent_test_context, image::Size image_size, const image::Box& block,
                      core::Span<word_type> image_buffer, auto tray, int repeat_index) {
        ARCHON_TEST_TRAIL(parent_test_context, core::formatted("%s", (repeat_index + 1)));

        // Randomize image contents
        for (std::size_t i = 0; i < image_buffer.size(); ++i)
            image_buffer[i] = 0;
        auto fill = [&](word_type* words, int num_pixels) {
            for (int i = 0; i < num_pixels; ++i) {
                int compound_index = i / pixels_per_compound;
                int pixel_index = i % pixels_per_compound;
                word_type* compound = words + compound_index * words_per_compound;
                value_type value = 0;
                for (int j = 0; j < words_per_compound; ++j) {
                    int word_index = j;
                    switch (format_type::word_order) {
                        case core::Endianness::big:
                            word_index = (words_per_compound - 1) - word_index;
                            break;
                        case core::Endianness::little:
                            break;
                    }
                    word_type word = compound[j];
                    value_type value_2 = value_type(image::unpack_int<bits_per_word>(word));
                    value |= value_2 << (word_index * bits_per_word);
                }
                {
                    value_type value_2 = core::rand_int_max(random, max_pixel);
                    switch (format_type::bit_order) {
                        case core::Endianness::big:
                            pixel_index = (pixels_per_compound - 1) - pixel_index;
                            break;
                        case core::Endianness::little:
                            break;
                    }
                    value |= value_2 << (pixel_index * bits_per_pixel);
                }
                for (int j = 0; j < words_per_compound; ++j) {
                    int word_index = j;
                    switch (format_type::word_order) {
                        case core::Endianness::big:
                            word_index = (words_per_compound - 1) - word_index;
                            break;
                        case core::Endianness::little:
                            break;
                    }
                    value_type value_2 = value >> (word_index * j);
                    value_2 &= core::int_mask<value_type>(bits_per_word);
                    compound[j] = image::pack_int<word_type, bits_per_word>(value_2);
                }
            }
        };
        ARCHON_CHECK_EQUAL(image_buffer.size() % format_type::words_per_compound, 0);
        if constexpr (format_type::compound_aligned_rows) {
            int compounds_per_row = core::int_div_round_up(image_size.width, pixels_per_compound);
            std::size_t words_per_row = std::size_t(compounds_per_row * words_per_compound);
            for (int h = 0; h < image_size.height; ++h)
                fill(image_buffer.data() + h * words_per_row, image_size.width);
        }
        else {
            fill(image_buffer.data(), image_size.height * image_size.width);
        }

        // Read
        format_type::read(image_buffer.data(), image_size, block.pos, tray);

        // Compare
        for (int y = 0; y < block.size.height; ++y) {
            for (int x = 0; x < block.size.width; ++x) {
                transf_type pixel_1 = tray(x, y)[0];
                int compound_index; // Index of bit compound in image buffer
                int pixel_index; // Index of pixel in bit compound
                int x_2 = block.pos.x + x;
                int y_2 = block.pos.y + y;
                if constexpr (format_type::compound_aligned_rows) {
                    int compounds_per_row = core::int_div_round_up(image_size.width, pixels_per_compound);
                    compound_index = y_2 * compounds_per_row + x_2 / pixels_per_compound;
                    pixel_index = x_2 % pixels_per_compound;
                }
                else {
                    int pixel_index_2 = y_2 * image_size.width + x_2;
                    compound_index = pixel_index_2 / pixels_per_compound;
                    pixel_index = pixel_index_2 % pixels_per_compound;
                }
                const word_type* compound = image_buffer.data() + compound_index * words_per_compound;
                value_type value = 0;
                for (int i = 0; i < words_per_compound; ++i) {
                    int word_index = i;
                    switch (format_type::word_order) {
                        case core::Endianness::big:
                            word_index = (words_per_compound - 1) - i;
                            break;
                        case core::Endianness::little:
                            break;
                    }
                    word_type word = compound[word_index];
                    value_type value_2 = value_type(image::unpack_int<bits_per_word>(word));
                    value |= value_2 << (word_index * bits_per_word);
                }
                switch (format_type::bit_order) {
                    case core::Endianness::big:
                        pixel_index = (pixels_per_compound - 1) - pixel_index;
                        break;
                    case core::Endianness::little:
                        break;
                }
                value_type value_2 = value >> (pixel_index * bits_per_pixel);
                value_2 &= core::int_mask<value_type>(bits_per_pixel);
                transf_type pixel_2 = image::pack_int<transf_type, bits_per_pixel>(value_2);
                bool success = ARCHON_CHECK_EQUAL(pixel_1, pixel_2);
                if (ARCHON_UNLIKELY(!success))
                    return;
            }
        }
    };

    auto test_2 = [&](check::TestContext& parent_test_context, image::Size image_size, const image::Box& block) {
        ARCHON_TEST_TRAIL(parent_test_context, core::formatted("%s;%s", image_size, block));
        std::size_t image_buffer_size = format_type::get_buffer_size(image_size);
        // Buffere is cleared due to value initialization by std::make_unique()
        auto image_buffer = std::make_unique<word_type[]>(image_buffer_size);
        core::Span image_buffer_2 = { image_buffer.get(), image_buffer_size };
        int tray_buffer_size = block.size.height * block.size.width;
        auto tray_buffer = std::make_unique<transf_type[]>(tray_buffer_size);
        int horz_stride = 1;
        int vert_stride = block.size.width;
        image::Iter iter = { tray_buffer.get(), horz_stride, vert_stride };
        image::Tray tray = { iter, block.size };
        for (int i = 0; i < 10; ++i)
            test_1(test_context, image_size, block, image_buffer_2, tray, i);
    };

    test_2(test_context, { 1, 1 }, { { 0, 0 }, { 1, 1 } });
    test_2(test_context, { 3, 3 }, { { 0, 0 }, { 3, 3 } });

    test_2(test_context, { 3, 3 }, { { 0, 0 }, { 2, 2 } });
    test_2(test_context, { 3, 3 }, { { 1, 0 }, { 2, 2 } });
    test_2(test_context, { 3, 3 }, { { 0, 1 }, { 2, 2 } });
    test_2(test_context, { 3, 3 }, { { 1, 1 }, { 2, 2 } });

    test_2(test_context, { 4, 4 }, { { 0, 0 }, { 2, 2 } });
    test_2(test_context, { 4, 4 }, { { 0, 1 }, { 2, 2 } });
    test_2(test_context, { 4, 4 }, { { 0, 2 }, { 2, 2 } });
    test_2(test_context, { 4, 4 }, { { 1, 0 }, { 2, 2 } });
    test_2(test_context, { 4, 4 }, { { 1, 1 }, { 2, 2 } });
    test_2(test_context, { 4, 4 }, { { 1, 2 }, { 2, 2 } });
    test_2(test_context, { 4, 4 }, { { 2, 0 }, { 2, 2 } });
    test_2(test_context, { 4, 4 }, { { 2, 1 }, { 2, 2 } });
    test_2(test_context, { 4, 4 }, { { 2, 2 }, { 2, 2 } });

    test_2(test_context, { 5, 5 }, { { 0, 0 }, { 3, 3 } });
    test_2(test_context, { 5, 5 }, { { 0, 1 }, { 3, 3 } });
    test_2(test_context, { 5, 5 }, { { 0, 2 }, { 3, 3 } });
    test_2(test_context, { 5, 5 }, { { 1, 0 }, { 3, 3 } });
    test_2(test_context, { 5, 5 }, { { 1, 1 }, { 3, 3 } });
    test_2(test_context, { 5, 5 }, { { 1, 2 }, { 3, 3 } });
    test_2(test_context, { 5, 5 }, { { 2, 0 }, { 3, 3 } });
    test_2(test_context, { 5, 5 }, { { 2, 1 }, { 3, 3 } });
    test_2(test_context, { 5, 5 }, { { 2, 2 }, { 3, 3 } });

    test_2(test_context, { 9, 9 }, { { 0, 0 }, { 7, 7 } });
    test_2(test_context, { 9, 9 }, { { 0, 1 }, { 7, 7 } });
    test_2(test_context, { 9, 9 }, { { 0, 2 }, { 7, 7 } });
    test_2(test_context, { 9, 9 }, { { 1, 0 }, { 7, 7 } });
    test_2(test_context, { 9, 9 }, { { 1, 1 }, { 7, 7 } });
    test_2(test_context, { 9, 9 }, { { 1, 2 }, { 7, 7 } });
    test_2(test_context, { 9, 9 }, { { 2, 0 }, { 7, 7 } });
    test_2(test_context, { 9, 9 }, { { 2, 1 }, { 7, 7 } });
    test_2(test_context, { 9, 9 }, { { 2, 2 }, { 7, 7 } });
}


ARCHON_TEST_BATCH(Image_IndexedPixelFormat_Write, variants)
{
    std::mt19937_64 random(test_context.seed_seq());

    using format_type = test_type;
    using word_type = typename format_type::word_type;
    using compound_type = typename format_type::compound_type;
    constexpr int bits_per_compound = format_type::bits_per_compound;
    using value_type = image::unpacked_type<compound_type, bits_per_compound>;
    constexpr int bits_per_pixel = format_type::bits_per_pixel;
    constexpr value_type max_pixel = core::int_mask<value_type>(bits_per_pixel);
    constexpr image::CompRepr transf_repr = format_type::transf_repr;
    using transf_type = image::comp_type<transf_repr>;
    constexpr int transf_depth = image::comp_repr_bit_width<transf_repr>();

    auto test_1 = [&](check::TestContext& parent_test_context, image::Size image_size, const image::Box& block,
                      core::Span<word_type> image_buffer, auto tray_1, auto tray_2, int repeat_index) {
        ARCHON_TEST_TRAIL(parent_test_context, core::formatted("%s", (repeat_index + 1)));

        // Fill image buffer with zeroes
        for (std::size_t i = 0; i < image_buffer.size(); ++i)
            image_buffer[i] = 0;

        // Generate tray with random contents
        for (int y = 0; y < block.size.height; ++y) {
            for (int x = 0; x < block.size.width; ++x) {
                transf_type* pixel = tray_1(x, y);
                value_type value = core::rand_int_max(random, max_pixel);
                pixel[0] = image::pack_int<transf_type, transf_depth>(value);
            }
        }

        // Write block
        format_type::write(image_buffer.data(), image_size, block.pos, tray_1);

        // Read everything
        format_type::read(image_buffer.data(), image_size, { 0, 0 }, tray_2);

        // Check
        for (int y = 0; y < image_size.height; ++y) {
            for (int x = 0; x < image_size.width; ++x) {
                if (ARCHON_LIKELY(block.contains_pixel_at({ x, y }))) {
                    int x_2 = x - block.pos.x;
                    int y_2 = y - block.pos.y;
                    transf_type pixel_1 = tray_1(x_2, y_2)[0];
                    transf_type pixel_2 = tray_2(x, y)[0];
                    bool success = ARCHON_CHECK_EQUAL(pixel_1, pixel_2);
                    if (ARCHON_UNLIKELY(!success))
                        return;
                }
                else {
                    transf_type pixel = tray_2(x, y)[0];
                    bool success = ARCHON_CHECK_EQUAL(pixel, 0);
                    if (ARCHON_UNLIKELY(!success))
                        return;
                }
            }
        }
    };

    auto test_2 = [&](check::TestContext& parent_test_context, image::Size image_size, const image::Box& block) {
        ARCHON_TEST_TRAIL(parent_test_context, core::formatted("%s;%s", image_size, block));
        std::size_t image_buffer_size = format_type::get_buffer_size(image_size);
        auto image_buffer = std::make_unique<word_type[]>(image_buffer_size);
        core::Span image_buffer_2 = { image_buffer.get(), image_buffer_size };
        int tray_buffer_size_1 = block.size.height * block.size.width;
        auto tray_buffer_1 = std::make_unique<transf_type[]>(tray_buffer_size_1);
        int horz_stride_1 = 1;
        int vert_stride_1 = block.size.width;
        image::Iter iter_1 = { tray_buffer_1.get(), horz_stride_1, vert_stride_1 };
        image::Tray tray_1 = { iter_1, block.size };
        int tray_buffer_size_2 = image_size.height * image_size.width;
        auto tray_buffer_2 = std::make_unique<transf_type[]>(tray_buffer_size_2);
        int horz_stride_2 = 1;
        int vert_stride_2 = image_size.width;
        image::Iter iter_2 = { tray_buffer_2.get(), horz_stride_2, vert_stride_2 };
        image::Tray tray_2 = { iter_2, image_size };
        for (int i = 0; i < 10; ++i)
            test_1(test_context, image_size, block, image_buffer_2, tray_1, tray_2, i);
    };

    test_2(test_context, { 1, 1 }, { { 0, 0 }, { 1, 1 } });
    test_2(test_context, { 3, 3 }, { { 0, 0 }, { 3, 3 } });

    test_2(test_context, { 3, 3 }, { { 0, 0 }, { 2, 2 } });
    test_2(test_context, { 3, 3 }, { { 1, 0 }, { 2, 2 } });
    test_2(test_context, { 3, 3 }, { { 0, 1 }, { 2, 2 } });
    test_2(test_context, { 3, 3 }, { { 1, 1 }, { 2, 2 } });

    test_2(test_context, { 4, 4 }, { { 0, 0 }, { 2, 2 } });
    test_2(test_context, { 4, 4 }, { { 0, 1 }, { 2, 2 } });
    test_2(test_context, { 4, 4 }, { { 0, 2 }, { 2, 2 } });
    test_2(test_context, { 4, 4 }, { { 1, 0 }, { 2, 2 } });
    test_2(test_context, { 4, 4 }, { { 1, 1 }, { 2, 2 } });
    test_2(test_context, { 4, 4 }, { { 1, 2 }, { 2, 2 } });
    test_2(test_context, { 4, 4 }, { { 2, 0 }, { 2, 2 } });
    test_2(test_context, { 4, 4 }, { { 2, 1 }, { 2, 2 } });
    test_2(test_context, { 4, 4 }, { { 2, 2 }, { 2, 2 } });

    test_2(test_context, { 5, 5 }, { { 0, 0 }, { 3, 3 } });
    test_2(test_context, { 5, 5 }, { { 0, 1 }, { 3, 3 } });
    test_2(test_context, { 5, 5 }, { { 0, 2 }, { 3, 3 } });
    test_2(test_context, { 5, 5 }, { { 1, 0 }, { 3, 3 } });
    test_2(test_context, { 5, 5 }, { { 1, 1 }, { 3, 3 } });
    test_2(test_context, { 5, 5 }, { { 1, 2 }, { 3, 3 } });
    test_2(test_context, { 5, 5 }, { { 2, 0 }, { 3, 3 } });
    test_2(test_context, { 5, 5 }, { { 2, 1 }, { 3, 3 } });
    test_2(test_context, { 5, 5 }, { { 2, 2 }, { 3, 3 } });

    test_2(test_context, { 9, 9 }, { { 0, 0 }, { 7, 7 } });
    test_2(test_context, { 9, 9 }, { { 0, 1 }, { 7, 7 } });
    test_2(test_context, { 9, 9 }, { { 0, 2 }, { 7, 7 } });
    test_2(test_context, { 9, 9 }, { { 1, 0 }, { 7, 7 } });
    test_2(test_context, { 9, 9 }, { { 1, 1 }, { 7, 7 } });
    test_2(test_context, { 9, 9 }, { { 1, 2 }, { 7, 7 } });
    test_2(test_context, { 9, 9 }, { { 2, 0 }, { 7, 7 } });
    test_2(test_context, { 9, 9 }, { { 2, 1 }, { 7, 7 } });
    test_2(test_context, { 9, 9 }, { { 2, 2 }, { 7, 7 } });
}


ARCHON_TEST_BATCH(Image_IndexedPixelFormat_Fill, variants)
{
    std::mt19937_64 random(test_context.seed_seq());

    using format_type = test_type;
    using word_type = typename format_type::word_type;
    using compound_type = typename format_type::compound_type;
    constexpr int bits_per_compound = format_type::bits_per_compound;
    using value_type = image::unpacked_type<compound_type, bits_per_compound>;
    constexpr int bits_per_pixel = format_type::bits_per_pixel;
    constexpr value_type max_pixel = core::int_mask<value_type>(bits_per_pixel);
    constexpr image::CompRepr transf_repr = format_type::transf_repr;
    using transf_type = image::comp_type<transf_repr>;
    constexpr int transf_depth = image::comp_repr_bit_width<transf_repr>();

    auto test_1 = [&](check::TestContext& parent_test_context, image::Size image_size, const image::Box& block,
                      core::Span<word_type> image_buffer, auto tray, int repeat_index) {
        ARCHON_TEST_TRAIL(parent_test_context, core::formatted("%s", (repeat_index + 1)));

        // Fill image buffer with zeroes
        for (std::size_t i = 0; i < image_buffer.size(); ++i)
            image_buffer[i] = 0;

        // Generate random fill color
        value_type value = core::rand_int_max(random, max_pixel);
        transf_type color = image::pack_int<transf_type, transf_depth>(value);

        // Fill
        format_type::fill(image_buffer.data(), image_size, block, &color);

        // Read everything
        format_type::read(image_buffer.data(), image_size, { 0, 0 }, tray);

        // Check
        for (int y = 0; y < image_size.height; ++y) {
            for (int x = 0; x < image_size.width; ++x) {
                transf_type pixel = tray(x, y)[0];
                if (ARCHON_LIKELY(block.contains_pixel_at({ x, y }))) {
                    bool success = ARCHON_CHECK_EQUAL(pixel, color);
                    if (ARCHON_UNLIKELY(!success))
                        return;
                }
                else {
                    bool success = ARCHON_CHECK_EQUAL(pixel, 0);
                    if (ARCHON_UNLIKELY(!success))
                        return;
                }
            }
        }
    };

    auto test_2 = [&](check::TestContext& parent_test_context, image::Size image_size, const image::Box& block) {
        ARCHON_TEST_TRAIL(parent_test_context, core::formatted("%s;%s", image_size, block));
        std::size_t image_buffer_size = format_type::get_buffer_size(image_size);
        auto image_buffer = std::make_unique<word_type[]>(image_buffer_size);
        core::Span image_buffer_2 = { image_buffer.get(), image_buffer_size };
        int tray_buffer_size = image_size.height * image_size.width;
        auto tray_buffer = std::make_unique<transf_type[]>(tray_buffer_size);
        int horz_stride = 1;
        int vert_stride = image_size.width;
        image::Iter iter = { tray_buffer.get(), horz_stride, vert_stride };
        image::Tray tray = { iter, image_size };
        for (int i = 0; i < 10; ++i)
            test_1(test_context, image_size, block, image_buffer_2, tray, i);
    };

    test_2(test_context, { 1, 1 }, { { 0, 0 }, { 1, 1 } });
    test_2(test_context, { 3, 3 }, { { 0, 0 }, { 3, 3 } });

    test_2(test_context, { 3, 3 }, { { 0, 0 }, { 2, 2 } });
    test_2(test_context, { 3, 3 }, { { 1, 0 }, { 2, 2 } });
    test_2(test_context, { 3, 3 }, { { 0, 1 }, { 2, 2 } });
    test_2(test_context, { 3, 3 }, { { 1, 1 }, { 2, 2 } });

    test_2(test_context, { 4, 4 }, { { 0, 0 }, { 2, 2 } });
    test_2(test_context, { 4, 4 }, { { 0, 1 }, { 2, 2 } });
    test_2(test_context, { 4, 4 }, { { 0, 2 }, { 2, 2 } });
    test_2(test_context, { 4, 4 }, { { 1, 0 }, { 2, 2 } });
    test_2(test_context, { 4, 4 }, { { 1, 1 }, { 2, 2 } });
    test_2(test_context, { 4, 4 }, { { 1, 2 }, { 2, 2 } });
    test_2(test_context, { 4, 4 }, { { 2, 0 }, { 2, 2 } });
    test_2(test_context, { 4, 4 }, { { 2, 1 }, { 2, 2 } });
    test_2(test_context, { 4, 4 }, { { 2, 2 }, { 2, 2 } });

    test_2(test_context, { 5, 5 }, { { 0, 0 }, { 3, 3 } });
    test_2(test_context, { 5, 5 }, { { 0, 1 }, { 3, 3 } });
    test_2(test_context, { 5, 5 }, { { 0, 2 }, { 3, 3 } });
    test_2(test_context, { 5, 5 }, { { 1, 0 }, { 3, 3 } });
    test_2(test_context, { 5, 5 }, { { 1, 1 }, { 3, 3 } });
    test_2(test_context, { 5, 5 }, { { 1, 2 }, { 3, 3 } });
    test_2(test_context, { 5, 5 }, { { 2, 0 }, { 3, 3 } });
    test_2(test_context, { 5, 5 }, { { 2, 1 }, { 3, 3 } });
    test_2(test_context, { 5, 5 }, { { 2, 2 }, { 3, 3 } });

    test_2(test_context, { 9, 9 }, { { 0, 0 }, { 7, 7 } });
    test_2(test_context, { 9, 9 }, { { 0, 1 }, { 7, 7 } });
    test_2(test_context, { 9, 9 }, { { 0, 2 }, { 7, 7 } });
    test_2(test_context, { 9, 9 }, { { 1, 0 }, { 7, 7 } });
    test_2(test_context, { 9, 9 }, { { 1, 1 }, { 7, 7 } });
    test_2(test_context, { 9, 9 }, { { 1, 2 }, { 7, 7 } });
    test_2(test_context, { 9, 9 }, { { 2, 0 }, { 7, 7 } });
    test_2(test_context, { 9, 9 }, { { 2, 1 }, { 7, 7 } });
    test_2(test_context, { 9, 9 }, { { 2, 2 }, { 7, 7 } });
}
