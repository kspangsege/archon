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
#include <type_traits>
#include <limits>
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
#include <archon/image/gamma.hpp>
#include <archon/image/comp_repr.hpp>
#include <archon/image/standard_channel_spec.hpp>
#include <archon/image/custom_channel_spec.hpp>
#include <archon/image/integer_pixel_format.hpp>
#include <archon/image/test/comp_repr_utils.hpp>


using namespace archon;
namespace test = image::test;


namespace {


using Format_Lum2 = image::IntegerPixelFormat_Lum<char, 2, char, 2>;
using Format_Lum4 = image::IntegerPixelFormat_Lum<char, 4, char, 2>;
using Format_Lum  = image::IntegerPixelFormat_Lum<char, 8, short, 2>;
using Format_LumA = image::IntegerPixelFormat_LumA<char, 8, short, 2>;
using Format_RGB  = image::IntegerPixelFormat_RGB<char, 8, short, 2>;
using Format_RGBA = image::IntegerPixelFormat_RGBA<char, 8, short, 2>;

using Format_LittleEndian =
    image::IntegerPixelFormat_RGBA<char, 8, short, 2, core::Endianness::little>;

using Format_AlphaFirst =
    image::IntegerPixelFormat<image::ChannelSpec_RGBA, char, 8, short, 2, core::Endianness::big, true>;

using Format_Reverse =
    image::IntegerPixelFormat<image::ChannelSpec_RGBA, char, 8, short, 2, core::Endianness::big, false, true>;

using Format_AlphaFirstReverse =
    image::IntegerPixelFormat<image::ChannelSpec_RGBA, char, 8, short, 2, core::Endianness::big, true, true>;

ARCHON_TEST_VARIANTS(variants,
                     ARCHON_TEST_TYPE(Format_Lum2,              Lum2),
                     ARCHON_TEST_TYPE(Format_Lum4,              Lum4),
                     ARCHON_TEST_TYPE(Format_Lum,               Lum),
                     ARCHON_TEST_TYPE(Format_LumA,              LumA),
                     ARCHON_TEST_TYPE(Format_RGB,               RGB),
                     ARCHON_TEST_TYPE(Format_RGBA,              RGBA),
                     ARCHON_TEST_TYPE(Format_LittleEndian,      LittleEndian),
                     ARCHON_TEST_TYPE(Format_AlphaFirst,        AlphaFirst),
                     ARCHON_TEST_TYPE(Format_Reverse,           Reverse),
                     ARCHON_TEST_TYPE(Format_AlphaFirstReverse, AlphaFirstReverse));


} // unnamed namespace


ARCHON_TEST(Image_IntegerPixelFormat_GetTransferInfo)
{
    auto test = [](check::TestContext& parent_test_context, auto format, std::string_view label,
                   image::CompRepr comp_repr, const image::ColorSpace& color_space, bool has_alpha, int bit_depth) {
        ARCHON_TEST_TRAIL(parent_test_context, label);
        image::Image::TransferInfo info = format.get_transfer_info();
        ARCHON_CHECK_EQUAL(info.comp_repr, comp_repr);
        ARCHON_CHECK_EQUAL(info.color_space, &color_space);
        ARCHON_CHECK_EQUAL(info.has_alpha, has_alpha);
        ARCHON_CHECK_EQUAL(info.bit_depth, bit_depth);
    };

    using channel_spec_type_1 = image::CustomChannelSpec<1, true>;
    using channel_spec_type_2 = image::CustomChannelSpec<3, false>;
    using custom_format_type_1 = image::IntegerPixelFormat<channel_spec_type_1, char, 8>;
    using custom_format_type_2 = image::IntegerPixelFormat<channel_spec_type_2, char, 8>;
    custom_format_type_1 custom_format_1 = custom_format_type_1(channel_spec_type_1(image::ColorSpace::get_lum()));
    custom_format_type_2 custom_format_2 = custom_format_type_2(channel_spec_type_2(image::ColorSpace::get_rgb()));

    test(test_context, Format_Lum2(),   "Lum2",    image::CompRepr::int8,  image::ColorSpace::get_lum(), false,  4);
    test(test_context, Format_Lum4(),   "Lum4",    image::CompRepr::int8,  image::ColorSpace::get_lum(), false,  8);
    test(test_context, Format_Lum(),    "Lum",     image::CompRepr::int16, image::ColorSpace::get_lum(), false, 16);
    test(test_context, Format_LumA(),   "LumA",    image::CompRepr::int16, image::ColorSpace::get_lum(), true,  16);
    test(test_context, Format_RGB(),    "RGB",     image::CompRepr::int16, image::ColorSpace::get_rgb(), false, 16);
    test(test_context, Format_RGBA(),   "RGBA",    image::CompRepr::int16, image::ColorSpace::get_rgb(), true,  16);
    test(test_context, custom_format_1, "Custom1", image::CompRepr::int8,  image::ColorSpace::get_lum(), true,   8);
    test(test_context, custom_format_2, "Custom2", image::CompRepr::int8,  image::ColorSpace::get_rgb(), false,  8);
}


ARCHON_TEST_BATCH(Image_IntegerPixelFormat_Read, variants)
{
    std::mt19937_64 random(test_context.seed_seq());
    using format_type = test_type;
    using word_type = typename format_type::word_type;
    using comp_type = typename format_type::comp_type;
    using value_type = image::unpacked_type<comp_type, format_type::bit_depth>;
    using transf_comp_type = typename format_type::transf_comp_type;

    auto test_1 = [&](check::TestContext& parent_test_context, image::Size image_size, const image::Box& block,
                      core::Span<word_type> image_buffer, auto tray, int repeat_index) {
        ARCHON_TEST_TRAIL(parent_test_context, core::formatted("%s", (repeat_index + 1)));
        constexpr int depth = format_type::bit_depth;
        constexpr int num_channels = format_type::num_channels;
        constexpr bool has_alpha_channel = format_type::has_alpha_channel;
        constexpr int bits_per_word = format_type::bits_per_word;

        // Randomize image contents
        for (std::size_t i = 0; i < image_buffer.size(); ++i) {
            using word_value_type = image::unpacked_type<word_type, bits_per_word>;
            word_value_type value = core::rand_int_bits<word_value_type>(random, bits_per_word);
            image_buffer[i] = image::pack_int<word_type, bits_per_word>(value);
        }

        // Read
        format_type::read(image_buffer.data(), image_size, block.pos, tray);

        // Compare
        for (int y = 0; y < block.size.height; ++y) {
            for (int x = 0; x < block.size.width; ++x) {
                const transf_comp_type* pixel_1 = tray(x, y);
                comp_type pixel_2[num_channels];
                {
                    int x_2 = block.pos.x + x;
                    int y_2 = block.pos.y + y;
                    int pixel_index = y_2 * image_size.width + x_2;
                    int word_index = pixel_index * format_type::words_per_pixel;
                    for (int i = 0; i < num_channels; ++i) {
                        value_type comp = 0;
                        int words_per_channel = format_type::words_per_channel;
                        for (int j = 0; j < words_per_channel; ++j) {
                            word_type word = image_buffer[word_index];
                            value_type value = image::unpack_int<bits_per_word>(word);
                            word_index += 1;
                            int word_pos = j;
                            switch (format_type::word_order) {
                                case core::Endianness::big:
                                    word_pos = (words_per_channel - 1) - word_pos;
                                    break;
                                case core::Endianness::little:
                                    break;
                            }
                            comp |= value << (word_pos * bits_per_word);
                        }
                        int comp_pos = i;
                        if constexpr (format_type::reverse_channel_order)
                            comp_pos = (num_channels - 1) - comp_pos;
                        if constexpr (has_alpha_channel && format_type::alpha_channel_first)
                            comp_pos = (comp_pos + (num_channels - 1)) % num_channels;
                        pixel_2[comp_pos] = image::pack_int<comp_type, depth>(comp);
                    }
                }
                if constexpr (!std::is_floating_point_v<transf_comp_type>) {
                    // Integer
                    constexpr int bit_width = image::comp_repr_bit_width<format_type::transf_repr>();
                    for (int i = 0; i < num_channels; ++i) {
                        transf_comp_type comp = image::int_to_int<depth, transf_comp_type, bit_width>(pixel_2[i]);
                        bool success = ARCHON_CHECK_EQUAL(pixel_1[i], comp);
                        if (ARCHON_UNLIKELY(!success))
                            return;
                    }
                }
                else {
                    // Floating point
                    transf_comp_type eps = std::numeric_limits<transf_comp_type>::epsilon() * 10;
                    int n = num_channels - int(has_alpha_channel);
                    transf_comp_type alpha = 1;
                    if constexpr (has_alpha_channel)
                        alpha = image::int_to_float<depth, transf_comp_type>(pixel_2[n]);
                    for (int i = 0; i < n; ++i) {
                        transf_comp_type comp = alpha * image::compressed_int_to_float<depth>(pixel_2[i]);
                        bool success = ARCHON_CHECK_APPROXIMATELY_EQUAL(pixel_1[i], comp, eps);
                        if (ARCHON_UNLIKELY(!success))
                            return;
                    }
                    if constexpr (has_alpha_channel) {
                        bool success = ARCHON_CHECK_APPROXIMATELY_EQUAL(pixel_1[n], alpha, eps);
                        if (ARCHON_UNLIKELY(!success))
                            return;
                    }
                }
            }
        }
    };

    auto test_2 = [&](check::TestContext& parent_test_context, image::Size image_size, const image::Box& block) {
        ARCHON_TEST_TRAIL(parent_test_context, core::formatted("%s;%s", image_size, block));
        constexpr int num_channels = format_type::num_channels;
        std::size_t image_buffer_size = format_type::get_buffer_size(image_size);
        auto image_buffer = std::make_unique<word_type[]>(image_buffer_size);
        core::Span image_buffer_2 = { image_buffer.get(), image_buffer_size };
        int tray_buffer_size = block.size.height * block.size.width * num_channels;
        auto tray_buffer = std::make_unique<transf_comp_type[]>(tray_buffer_size);
        image::Iter iter = { tray_buffer.get(), num_channels, block.size.width * num_channels };
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


ARCHON_TEST_BATCH(Image_IntegerPixelFormat_Write, variants)
{
    std::mt19937_64 random(test_context.seed_seq());

    using format_type = test_type;
    using word_type = typename format_type::word_type;
    using comp_type = typename format_type::comp_type;
    using transf_comp_type = typename format_type::transf_comp_type;

    auto test_1 = [&](check::TestContext& parent_test_context, image::Size image_size, const image::Box& block,
                      core::Span<word_type> image_buffer, auto tray_1, auto tray_2, int repeat_index) {
        ARCHON_TEST_TRAIL(parent_test_context, core::formatted("%s", (repeat_index + 1)));
        constexpr int depth = format_type::bit_depth;
        constexpr int num_channels = format_type::num_channels;
        constexpr bool has_alpha_channel = format_type::has_alpha_channel;

        // Fill image buffer with zeroes
        for (std::size_t i = 0; i < image_buffer.size(); ++i)
            image_buffer[i] = 0;

        // Generate tray with random contents
        for (int y = 0; y < block.size.height; ++y) {
            for (int x = 0; x < block.size.width; ++x) {
                transf_comp_type* pixel = tray_1(x, y);
                if constexpr (!std::is_floating_point_v<transf_comp_type>) {
                    // Integer
                    constexpr int bit_width = image::comp_repr_bit_width<format_type::transf_repr>();
                    using value_type = image::unpacked_type<transf_comp_type, bit_width>;
                    for (int i = 0; i < num_channels; ++i) {
                        value_type value = core::rand_int_bits<value_type>(random, bit_width);
                        pixel[i] = image::pack_int<transf_comp_type, bit_width>(value);
                    }
                }
                else {
                    // Floating point
                    int n = num_channels - int(has_alpha_channel);
                    transf_comp_type alpha = 1;
                    if constexpr (has_alpha_channel)
                        alpha = core::rand_float<transf_comp_type>(random);
                    for (int i = 0; i < n; ++i)
                        pixel[i] = transf_comp_type(alpha * core::rand_float<transf_comp_type>(random));
                    if constexpr (has_alpha_channel)
                        pixel[n] = alpha;
                }
            }
        }

        // Write block
        format_type::write(image_buffer.data(), image_size, block.pos, tray_1);

        // Read everything
        format_type::read(image_buffer.data(), image_size, { 0, 0 }, tray_2);

        // Check
        for (int y = 0; y < image_size.height; ++y) {
            for (int x = 0; x < image_size.width; ++x) {
                if (ARCHON_LIKELY(image::Box({ x, y }, 1).contained_in(block))) {
                    int x_2 = x - block.pos.x;
                    int y_2 = y - block.pos.y;
                    const transf_comp_type* pixel_1 = tray_1(x_2, y_2);
                    const transf_comp_type* pixel_2 = tray_2(x, y);
                    if constexpr (!std::is_floating_point_v<transf_comp_type>) {
                        // Integer
                        for (int i = 0; i < num_channels; ++i) {
                            transf_comp_type comp_1 = pixel_1[i];
                            transf_comp_type comp_2 = pixel_2[i];
                            constexpr int bit_width = image::comp_repr_bit_width<format_type::transf_repr>();
                            comp_type value_1 = image::int_to_int<bit_width, comp_type, depth>(comp_1);
                            transf_comp_type value_2 = image::int_to_int<depth, transf_comp_type, bit_width>(value_1);
                            bool success = ARCHON_CHECK_EQUAL(comp_2, value_2);
                            if (ARCHON_UNLIKELY(!success))
                                return;
                        }
                    }
                    else {
                        // Floating point
                        transf_comp_type eps = std::numeric_limits<transf_comp_type>::epsilon() * 10;
                        int n = num_channels - int(has_alpha_channel);
                        transf_comp_type alpha = 1, inv_alpha = 1;
                        if constexpr (has_alpha_channel) {
                            transf_comp_type alpha_1 = pixel_1[n];
                            comp_type value = image::float_to_int<comp_type, depth>(alpha_1);
                            alpha = image::int_to_float<depth, transf_comp_type>(value);
                            inv_alpha = transf_comp_type(alpha_1 != 0 ? 1 / alpha_1 : 0);
                        }
                        for (int i = 0; i < n; ++i) {
                            transf_comp_type comp_1 = pixel_1[i];
                            transf_comp_type comp_2 = pixel_2[i];
                            comp_type value_1 = image::float_to_compressed_int<comp_type, depth>(inv_alpha * comp_1);
                            auto value_2 = transf_comp_type(alpha * image::compressed_int_to_float<depth>(value_1));
                            bool success = ARCHON_CHECK_APPROXIMATELY_EQUAL(comp_2, value_2, eps);
                            if (ARCHON_UNLIKELY(!success))
                                return;
                        }
                        if constexpr (has_alpha_channel) {
                            transf_comp_type alpha_2 = pixel_2[n];
                            bool success = ARCHON_CHECK_APPROXIMATELY_EQUAL(alpha_2, alpha, eps);
                            if (ARCHON_UNLIKELY(!success))
                                return;
                        }
                    }
                }
                else {
                    const transf_comp_type* pixel = tray_2(x, y);
                    for (int i = 0; i < num_channels; ++i) {
                        bool success = ARCHON_CHECK_EQUAL(pixel[i], 0);
                        if (ARCHON_UNLIKELY(!success))
                            return;
                    }
                }
            }
        }
    };

    auto test_2 = [&](check::TestContext& parent_test_context, image::Size image_size, const image::Box& block) {
        ARCHON_TEST_TRAIL(parent_test_context, core::formatted("%s;%s", image_size, block));
        constexpr int num_channels = format_type::num_channels;
        std::size_t image_buffer_size = format_type::get_buffer_size(image_size);
        auto image_buffer = std::make_unique<word_type[]>(image_buffer_size);
        core::Span image_buffer_2 = { image_buffer.get(), image_buffer_size };
        int tray_buffer_size_1 = block.size.height * block.size.width * num_channels;
        auto tray_buffer_1 = std::make_unique<transf_comp_type[]>(tray_buffer_size_1);
        image::Iter iter_1 = { tray_buffer_1.get(), num_channels, block.size.width * num_channels };
        image::Tray tray_1 = { iter_1, block.size };
        int tray_buffer_size_2 = image_size.height * image_size.width * num_channels;
        auto tray_buffer_2 = std::make_unique<transf_comp_type[]>(tray_buffer_size_2);
        image::Iter iter_2 = { tray_buffer_2.get(), num_channels, image_size.width * num_channels };
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


ARCHON_TEST_BATCH(Image_IntegerPixelFormat_Fill, variants)
{
    std::mt19937_64 random(test_context.seed_seq());

    using format_type = test_type;
    using word_type = typename format_type::word_type;
    using comp_type = typename format_type::comp_type;
    using transf_comp_type = typename format_type::transf_comp_type;

    auto test_1 = [&](check::TestContext& parent_test_context, image::Size image_size, const image::Box& block,
                      core::Span<word_type> image_buffer, auto tray, int repeat_index) {
        ARCHON_TEST_TRAIL(parent_test_context, core::formatted("%s", (repeat_index + 1)));
        constexpr int depth = format_type::bit_depth;
        constexpr int num_channels = format_type::num_channels;
        constexpr bool has_alpha_channel = format_type::has_alpha_channel;

        // Fill image buffer with zeroes
        for (std::size_t i = 0; i < image_buffer.size(); ++i)
            image_buffer[i] = 0;

        // Generate random fill color
        transf_comp_type color_1[num_channels];
        if constexpr (!std::is_floating_point_v<transf_comp_type>) {
            // Integer
            constexpr int bit_width = image::comp_repr_bit_width<format_type::transf_repr>();
            using value_type = image::unpacked_type<transf_comp_type, bit_width>;
            for (int i = 0; i < num_channels; ++i) {
                value_type value = core::rand_int_bits<value_type>(random, bit_width);
                color_1[i] = image::pack_int<transf_comp_type, bit_width>(value);
            }
        }
        else {
            // Floating point
            int n = num_channels - int(has_alpha_channel);
            transf_comp_type alpha = 1;
            if constexpr (has_alpha_channel)
                alpha = core::rand_float<transf_comp_type>(random);
            for (int i = 0; i < n; ++i)
                color_1[i] = transf_comp_type(alpha * core::rand_float<transf_comp_type>(random));
            if constexpr (has_alpha_channel)
                color_1[n] = alpha;
        }

        // Fill
        format_type::fill(image_buffer.data(), image_size, block, color_1);

        // Read everything
        format_type::read(image_buffer.data(), image_size, { 0, 0 }, tray);

        // Compute expected color
        transf_comp_type color_2[num_channels];
        if constexpr (!std::is_floating_point_v<transf_comp_type>) {
            // Integer
            for (int i = 0; i < num_channels; ++i) {
                transf_comp_type comp = color_1[i];
                constexpr int bit_width = image::comp_repr_bit_width<format_type::transf_repr>();
                comp_type value = image::int_to_int<bit_width, comp_type, depth>(comp);
                transf_comp_type comp_2 = image::int_to_int<depth, transf_comp_type, bit_width>(value);
                color_2[i] = comp_2;
            }
        }
        else {
            // Floating point
            int n = num_channels - int(has_alpha_channel);
            transf_comp_type alpha = 1, inv_alpha = 1;
            if constexpr (has_alpha_channel) {
                transf_comp_type comp = color_1[n];
                comp_type value = image::float_to_int<comp_type, depth>(comp);
                alpha = image::int_to_float<depth, transf_comp_type>(value);
                inv_alpha = transf_comp_type(comp != 0 ? 1 / comp : 0);
            }
            for (int i = 0; i < n; ++i) {
                transf_comp_type comp = color_1[i];
                comp_type value = image::float_to_compressed_int<comp_type, depth>(inv_alpha * comp);
                transf_comp_type comp_2 = transf_comp_type(alpha * image::compressed_int_to_float<depth>(value));
                color_2[i] = comp_2;
            }
            if constexpr (has_alpha_channel)
                color_2[n] = alpha;
        }

        // Check
        for (int y = 0; y < image_size.height; ++y) {
            for (int x = 0; x < image_size.width; ++x) {
                const transf_comp_type* pixel = tray(x, y);
                if (ARCHON_LIKELY(image::Box({ x, y }, 1).contained_in(block))) {
                    if constexpr (!std::is_floating_point_v<transf_comp_type>) {
                        // Integer
                        for (int i = 0; i < num_channels; ++i) {
                            bool success = ARCHON_CHECK_EQUAL(pixel[i], color_2[i]);
                            if (ARCHON_UNLIKELY(!success))
                                return;
                        }
                    }
                    else {
                        // Floating point
                        transf_comp_type eps = std::numeric_limits<transf_comp_type>::epsilon() * 10;
                        for (int i = 0; i < num_channels; ++i) {
                            bool success = ARCHON_CHECK_APPROXIMATELY_EQUAL(pixel[i], color_2[i], eps);
                            if (ARCHON_UNLIKELY(!success))
                                return;
                        }
                    }
                }
                else {
                    for (int i = 0; i < num_channels; ++i) {
                        bool success = ARCHON_CHECK_EQUAL(pixel[i], 0);
                        if (ARCHON_UNLIKELY(!success))
                            return;
                    }
                }
            }
        }
    };

    auto test_2 = [&](check::TestContext& parent_test_context, image::Size image_size, const image::Box& block) {
        ARCHON_TEST_TRAIL(parent_test_context, core::formatted("%s;%s", image_size, block));
        constexpr int num_channels = format_type::num_channels;
        std::size_t image_buffer_size = format_type::get_buffer_size(image_size);
        auto image_buffer = std::make_unique<word_type[]>(image_buffer_size);
        core::Span image_buffer_2 = { image_buffer.get(), image_buffer_size };
        int tray_buffer_size = image_size.height * image_size.width * num_channels;
        auto tray_buffer = std::make_unique<transf_comp_type[]>(tray_buffer_size);
        image::Iter iter = { tray_buffer.get(), num_channels, image_size.width * num_channels };
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
