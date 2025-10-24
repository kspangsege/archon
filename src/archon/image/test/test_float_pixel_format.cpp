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


#include <cstddef>
#include <memory>
#include <random>
#include <string_view>

#include <archon/core/features.hpp>
#include <archon/core/span.hpp>
#include <archon/core/format.hpp>
#include <archon/core/random.hpp>
#include <archon/check.hpp>
#include <archon/image/geom.hpp>
#include <archon/image/iter.hpp>
#include <archon/image/tray.hpp>
#include <archon/image/comp_types.hpp>
#include <archon/image/comp_repr.hpp>
#include <archon/image/color_space.hpp>
#include <archon/image/standard_channel_spec.hpp>
#include <archon/image/custom_channel_spec.hpp>
#include <archon/image/transfer_info.hpp>
#include <archon/image/float_pixel_format.hpp>


using namespace archon;


namespace {


using Format_Lum  = image::FloatPixelFormat_Lum_F;
using Format_LumA = image::FloatPixelFormat_LumA_F;
using Format_RGB  = image::FloatPixelFormat_RGB_F;
using Format_RGBA = image::FloatPixelFormat_RGBA_F;

using Format_AlphaFirst        = image::FloatPixelFormat<image::ChannelSpec_RGBA, float, true>;
using Format_Reverse           = image::FloatPixelFormat<image::ChannelSpec_RGBA, float, false, true>;
using Format_AlphaFirstReverse = image::FloatPixelFormat<image::ChannelSpec_RGBA, float, true, true>;

ARCHON_TEST_VARIANTS(variants,
                     ARCHON_TEST_TYPE(Format_Lum,               Lum),
                     ARCHON_TEST_TYPE(Format_LumA,              LumA),
                     ARCHON_TEST_TYPE(Format_RGB,               RGB),
                     ARCHON_TEST_TYPE(Format_RGBA,              RGBA),
                     ARCHON_TEST_TYPE(Format_AlphaFirst,        AlphaFirst),
                     ARCHON_TEST_TYPE(Format_Reverse,           Reverse),
                     ARCHON_TEST_TYPE(Format_AlphaFirstReverse, AlphaFirstReverse));


} // unnamed namespace


ARCHON_TEST(Image_FloatPixelFormat_GetTransferInfo)
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

    using channel_spec_type_1 = image::CustomChannelSpec<1, true>;
    using channel_spec_type_2 = image::CustomChannelSpec<3, false>;
    using custom_format_type_1 = image::FloatPixelFormat<channel_spec_type_1, float>;
    using custom_format_type_2 = image::FloatPixelFormat<channel_spec_type_2, float>;
    custom_format_type_1 custom_format_1 = custom_format_type_1(channel_spec_type_1(image::ColorSpace::get_lum()));
    custom_format_type_2 custom_format_2 = custom_format_type_2(channel_spec_type_2(image::ColorSpace::get_rgb()));

    int w = image::bit_width<float>;
    test(test_context, Format_Lum(),    "Lum",     image::CompRepr::float_, image::ColorSpace::get_lum(), false, w);
    test(test_context, Format_LumA(),   "LumA",    image::CompRepr::float_, image::ColorSpace::get_lum(), true,  w);
    test(test_context, Format_RGB(),    "RGB",     image::CompRepr::float_, image::ColorSpace::get_rgb(), false, w);
    test(test_context, Format_RGBA(),   "RGBA",    image::CompRepr::float_, image::ColorSpace::get_rgb(), true,  w);
    test(test_context, custom_format_1, "Custom1", image::CompRepr::float_, image::ColorSpace::get_lum(), true,  w);
    test(test_context, custom_format_2, "Custom2", image::CompRepr::float_, image::ColorSpace::get_rgb(), false, w);
}


ARCHON_TEST_BATCH(Image_FloatPixelFormat_Read, variants)
{
    std::mt19937_64 random(test_context.seed_seq());
    using format_type = test_type;
    using word_type = typename format_type::word_type;
    using transf_comp_type = typename format_type::transf_comp_type;

    auto test_1 = [&](check::TestContext& parent_test_context, image::Size image_size, const image::Box& block,
                      core::Span<word_type> image_buffer, auto tray, int repeat_index) {
        ARCHON_TEST_TRAIL(parent_test_context, core::formatted("%s", (repeat_index + 1)));
        constexpr int num_channels = format_type::num_channels;
        constexpr bool has_alpha_channel = format_type::has_alpha_channel;

        // Randomize image contents
        for (std::size_t i = 0; i < image_buffer.size(); ++i)
            image_buffer[i] = core::rand_float<word_type>(random);

        // Read
        format_type::read(image_buffer.data(), image_size, block.pos, tray);

        // Compare
        for (int y = 0; y < block.size.height; ++y) {
            for (int x = 0; x < block.size.width; ++x) {
                const transf_comp_type* pixel_1 = tray(x, y);
                word_type pixel_2[num_channels];
                {
                    int x_2 = block.pos.x + x;
                    int y_2 = block.pos.y + y;
                    int pixel_index = y_2 * image_size.width + x_2;
                    int word_index = pixel_index * num_channels;
                    for (int i = 0; i < num_channels; ++i) {
                        int comp_pos = i;
                        if constexpr (format_type::reverse_channel_order)
                            comp_pos = (num_channels - 1) - comp_pos;
                        if constexpr (has_alpha_channel && format_type::alpha_channel_first)
                            comp_pos = (comp_pos + (num_channels - 1)) % num_channels;
                        pixel_2[comp_pos] = image_buffer[word_index + i];
                    }
                }
                for (int i = 0; i < num_channels; ++i)
                    ARCHON_CHECK_EQUAL(pixel_1[i], transf_comp_type(pixel_2[i]));
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


ARCHON_TEST_BATCH(Image_FloatPixelFormat_Write, variants)
{
    std::mt19937_64 random(test_context.seed_seq());

    using format_type = test_type;
    using word_type = typename format_type::word_type;
    using transf_comp_type = typename format_type::transf_comp_type;

    auto test_1 = [&](check::TestContext& parent_test_context, image::Size image_size, const image::Box& block,
                      core::Span<word_type> image_buffer, auto tray_1, auto tray_2, int repeat_index) {
        ARCHON_TEST_TRAIL(parent_test_context, core::formatted("%s", (repeat_index + 1)));
        constexpr int num_channels = format_type::num_channels;

        // Fill image buffer with zeroes
        for (std::size_t i = 0; i < image_buffer.size(); ++i)
            image_buffer[i] = 0;

        // Generate tray with random contents
        for (int y = 0; y < block.size.height; ++y) {
            for (int x = 0; x < block.size.width; ++x) {
                transf_comp_type* pixel = tray_1(x, y);
                for (int i = 0; i < num_channels; ++i)
                    pixel[i] = core::rand_float<transf_comp_type>(random);
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
                    const transf_comp_type* pixel_1 = tray_1(x_2, y_2);
                    const transf_comp_type* pixel_2 = tray_2(x, y);
                    for (int i = 0; i < num_channels; ++i) {
                        bool success = ARCHON_CHECK_EQUAL(pixel_2[i], transf_comp_type(word_type(pixel_1[i])));
                        if (ARCHON_UNLIKELY(!success))
                            return;
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


ARCHON_TEST_BATCH(Image_FloatPixelFormat_Fill, variants)
{
    std::mt19937_64 random(test_context.seed_seq());

    using format_type = test_type;
    using word_type = typename format_type::word_type;
    using transf_comp_type = typename format_type::transf_comp_type;

    auto test_1 = [&](check::TestContext& parent_test_context, image::Size image_size, const image::Box& block,
                      core::Span<word_type> image_buffer, auto tray, int repeat_index) {
        ARCHON_TEST_TRAIL(parent_test_context, core::formatted("%s", (repeat_index + 1)));
        constexpr int num_channels = format_type::num_channels;

        // Fill image buffer with zeroes
        for (std::size_t i = 0; i < image_buffer.size(); ++i)
            image_buffer[i] = 0;

        // Generate random fill color
        transf_comp_type color_1[num_channels];
        for (int i = 0; i < num_channels; ++i)
            color_1[i] = core::rand_float<transf_comp_type>(random);

        // Fill
        format_type::fill(image_buffer.data(), image_size, block, color_1);

        // Read everything
        format_type::read(image_buffer.data(), image_size, { 0, 0 }, tray);

        // Compute expected color
        transf_comp_type color_2[num_channels];
        for (int i = 0; i < num_channels; ++i)
            color_2[i] = transf_comp_type(word_type(color_1[i]));

        // Check
        for (int y = 0; y < image_size.height; ++y) {
            for (int x = 0; x < image_size.width; ++x) {
                const transf_comp_type* pixel = tray(x, y);
                if (ARCHON_LIKELY(block.contains_pixel_at({ x, y }))) {
                    for (int i = 0; i < num_channels; ++i) {
                        bool success = ARCHON_CHECK_EQUAL(pixel[i], color_2[i]);
                        if (ARCHON_UNLIKELY(!success))
                            return;
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
