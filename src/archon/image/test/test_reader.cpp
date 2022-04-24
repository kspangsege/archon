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
#include <cmath>
#include <utility>
#include <algorithm>
#include <random>

#include <archon/core/features.h>
#include <archon/core/integer.hpp>
#include <archon/core/math.hpp>
#include <archon/core/random.hpp>
#include <archon/check.hpp>
#include <archon/util/colors.hpp>
#include <archon/image/size.hpp>
#include <archon/image/pos.hpp>
#include <archon/image/comp_types.hpp>
#include <archon/image/gamma.hpp>
#include <archon/image/comp_repr.hpp>
#include <archon/image/color_space.hpp>
#include <archon/image/pixel_repr.hpp>
#include <archon/image/pixel.hpp>
#include <archon/image/block.hpp>
#include <archon/image/standard_channel_spec.hpp>
#include <archon/image/tray_image.hpp>
#include <archon/image/integer_pixel_format.hpp>
#include <archon/image/indexed_pixel_format.hpp>
#include <archon/image/buffered_image.hpp>
#include <archon/image/reader.hpp>
#include <archon/image/test/box_utils.hpp>
#include <archon/image/test/comp_repr_utils.hpp>


using namespace archon;
namespace test = image::test;


namespace {


ARCHON_TEST_VARIANTS(color_space_variants,
                     ARCHON_TEST_VALUE(image::ColorSpace::Tag::lum, Lum),
                     ARCHON_TEST_VALUE(image::ColorSpace::Tag::rgb, RGB));


ARCHON_TEST_VARIANTS(channel_spec_variants,
                     ARCHON_TEST_TYPE(image::ChannelSpec_Lum,  Lum),
                     ARCHON_TEST_TYPE(image::ChannelSpec_LumA, LumA),
                     ARCHON_TEST_TYPE(image::ChannelSpec_RGB,  RGB),
                     ARCHON_TEST_TYPE(image::ChannelSpec_RGBA, RGBA));


ARCHON_TEST_VARIANTS(pixel_repr_variants,
                     ARCHON_TEST_TYPE(image::Lum_8,  Lum),
                     ARCHON_TEST_TYPE(image::LumA_8, LumA),
                     ARCHON_TEST_TYPE(image::RGB_8,  RGB),
                     ARCHON_TEST_TYPE(image::RGBA_8, RGBA));


} // unnamed namespace


ARCHON_TEST_BATCH(Image_Reader_GetBlock_ShortCircuitSameFormat, channel_spec_variants)
{
    using channel_spec_type = test_type;
    constexpr image::CompRepr comp_repr = image::CompRepr::int8;
    using comp_type = image::comp_type<comp_repr>;
    constexpr int bit_width = image::comp_repr_bit_width<comp_repr>();
    using format_type = image::IntegerPixelFormat<channel_spec_type, comp_type, bit_width>;
    image::Size image_size = { 4, 4 };
    image::BufferedImage<format_type> image(image_size);
    auto buffer = image.get_buffer();
    std::mt19937_64 random(test_context.seed_seq());
    for (std::size_t i = 0; i < buffer.size(); ++i)
        buffer[i] = test::rand_comp<comp_repr>(random);
    image::Reader reader(image);
    image::Pos pos = { 1, 1 };
    image::Size block_size = { 2, 2 };
    constexpr image::ColorSpace::Tag color_space_tag = channel_spec_type::color_space_tag;
    constexpr bool has_alpha = channel_spec_type::has_alpha_channel;
    using pixel_repr_type = image::PixelRepr<color_space_tag, has_alpha, comp_repr>;
    image::PixelBlock<pixel_repr_type> block(block_size);
    reader.get_block(pos, block);
    for (int y = 0; y < block_size.height; ++y) {
        for (int x = 0; x < block_size.width; ++x) {
            image::Pixel<pixel_repr_type> pixel_1 = block.get_pixel({ x, y });
            auto pixel_offset = pos.x + x + (pos.y + y) * std::size_t(image_size.width);
            int n = format_type::num_channels;
            const comp_type* pixel_2 = buffer.data() + pixel_offset * n;
            for (int i = 0; i < n; ++i)
                ARCHON_CHECK_EQUAL(pixel_1[i], pixel_2[i]);
        }
    }
}


ARCHON_TEST_BATCH(Image_Reader_GetBlock_ShortCircuirAddAlpha, color_space_variants)
{
    constexpr image::ColorSpace::Tag color_space_tag = test_value;
    constexpr bool has_alpha_1 = false;
    using channel_spec_type = image::StandardChannelSpec<color_space_tag, has_alpha_1>;
    constexpr image::CompRepr comp_repr = image::CompRepr::int8;
    using comp_type = image::comp_type<comp_repr>;
    constexpr int bit_width = image::comp_repr_bit_width<comp_repr>();
    using format_type = image::IntegerPixelFormat<channel_spec_type, comp_type, bit_width>;
    image::Size image_size = { 4, 4 };
    image::BufferedImage<format_type> image(image_size);
    auto buffer = image.get_buffer();
    std::mt19937_64 random(test_context.seed_seq());
    for (std::size_t i = 0; i < buffer.size(); ++i)
        buffer[i] = test::rand_comp<comp_repr>(random);
    image::Reader reader(image);
    image::Pos pos = { 1, 1 };
    image::Size block_size = { 2, 2 };
    constexpr bool has_alpha_2 = true;
    using pixel_repr_type = image::PixelRepr<color_space_tag, has_alpha_2, comp_repr>;
    image::PixelBlock<pixel_repr_type> block(block_size);
    reader.get_block(pos, block);
    for (int y = 0; y < block_size.height; ++y) {
        for (int x = 0; x < block_size.width; ++x) {
            image::Pixel<pixel_repr_type> pixel_1 = block.get_pixel({ x, y });
            auto pixel_offset = pos.x + x + (pos.y + y) * std::size_t(image_size.width);
            int n = format_type::num_channels;
            const comp_type* pixel_2 = buffer.data() + pixel_offset * n;
            for (int i = 0; i < n; ++i)
                ARCHON_CHECK_EQUAL(pixel_1[i], pixel_2[i]);
            ARCHON_CHECK_EQUAL(pixel_1[n], image::comp_repr_max<comp_repr>());
        }
    }
}


ARCHON_TEST_BATCH(Image_Reader_GetBlock_NotShortCircuitDirectColor, color_space_variants)
{
    constexpr image::ColorSpace::Tag color_space_tag = test_value;
    constexpr bool has_alpha_1 = true;
    using channel_spec_type = image::StandardChannelSpec<color_space_tag, has_alpha_1>;
    constexpr image::CompRepr comp_repr = image::CompRepr::int8;
    using comp_type = image::comp_type<comp_repr>;
    constexpr int bit_width = image::comp_repr_bit_width<comp_repr>();
    using format_type = image::IntegerPixelFormat<channel_spec_type, comp_type, bit_width>;
    image::Size image_size = { 4, 4 };
    image::BufferedImage<format_type> image(image_size);
    auto buffer = image.get_buffer();
    std::mt19937_64 random(test_context.seed_seq());
    for (std::size_t i = 0; i < buffer.size(); ++i)
        buffer[i] = test::rand_comp<comp_repr>(random);
    image::Reader reader(image);
    image::Pos pos = { 1, 1 };
    image::Size block_size = { 2, 2 };
    constexpr bool has_alpha_2 = false;
    using pixel_repr_type = image::PixelRepr<color_space_tag, has_alpha_2, comp_repr>;
    image::PixelBlock<pixel_repr_type> block(block_size);
    reader.get_block(pos, block);
    for (int y = 0; y < block_size.height; ++y) {
        for (int x = 0; x < block_size.width; ++x) {
            image::Pixel<pixel_repr_type> pixel_1 = block.get_pixel({ x, y });
            auto pixel_offset = pos.x + x + (pos.y + y) * std::size_t(image_size.width);
            int n = pixel_repr_type::num_channels;
            const comp_type* pixel_2 = buffer.data() + pixel_offset * (n + 1);
            image::float_type alpha = image::int_to_float<bit_width, image::float_type>(pixel_2[n]);
            for (int i = 0; i < n; ++i) {
                image::float_type value_1 =
                    alpha * image::compressed_int_to_float<bit_width>(pixel_2[i]);
                comp_type value_2 = image::float_to_compressed_int<comp_type, bit_width>(value_1);
                ARCHON_CHECK_DIST_LESS_EQUAL(image::unpack_int<bit_width>(pixel_1[i]),
                                             image::unpack_int<bit_width>(value_2), 1);
            }
        }
    }
}


ARCHON_TEST_BATCH(Image_Reader_GetBlock_IndirectColorSimilarFormats, color_space_variants)
{
    // Generate palette with randomized colors
    constexpr image::ColorSpace::Tag color_space_tag = test_value;
    constexpr bool palette_has_alpha = false;
    using channel_spec_type = image::StandardChannelSpec<color_space_tag, palette_has_alpha>;
    constexpr image::CompRepr palette_comp_repr = image::CompRepr::int8;
    using palette_comp_type = image::comp_type<palette_comp_repr>;
    constexpr int palette_bit_width = image::comp_repr_bit_width<palette_comp_repr>();
    using palette_format_type = image::IntegerPixelFormat<channel_spec_type, palette_comp_type,
                                                          palette_bit_width>;
    int palette_size = 8;
    image::BufferedImage<palette_format_type> palette({ palette_size, 1 });
    auto palette_buffer = palette.get_buffer();
    std::mt19937_64 random(test_context.seed_seq());
    for (std::size_t i = 0; i < palette_buffer.size(); ++i)
        palette_buffer[i] = test::rand_comp<palette_comp_repr>(random);

    // Generate image with randomized color indexes and a chance for an index to be out of range
    constexpr image::CompRepr image_comp_repr = image::CompRepr::int8;
    using image_comp_type = image::comp_type<image_comp_repr>;
    constexpr int image_bit_width = image::comp_repr_bit_width<image_comp_repr>();
    image::Size image_size = { 8, 8 };
    image::IndexedPixelFormat<image_comp_type, image_bit_width> format(palette);
    image::BufferedImage image(image_size, std::move(format));
    auto image_buffer = image.get_buffer();
    for (std::size_t i = 0; i < image_buffer.size(); ++i) {
        int max_index = palette_size; // max is then out of range by one
        int index = core::rand_int_max(random, max_index);
        image_buffer[i] = image::pack_int<image_comp_type, image_bit_width>(index);
    }

    // Read block from image using same color space as palette, but with an alpha channel
    // added
    image::Reader reader(image);
    reader.set_background_color(util::colors::transparent);
    image::Pos pos = { 1, 1 };
    image::Size block_size = { 6, 6 };
    constexpr bool read_has_alpha = true;
    using pixel_repr_type = image::PixelRepr<color_space_tag, read_has_alpha, palette_comp_repr>;
    image::PixelBlock<pixel_repr_type> block(block_size);
    reader.get_block(pos, block);
    for (int y = 0; y < block_size.height; ++y) {
        for (int x = 0; x < block_size.width; ++x) {
            image::Pixel<pixel_repr_type> pixel = block.get_pixel({ x, y });
            auto pixel_offset = pos.x + x + (pos.y + y) * std::size_t(image_size.width);
            auto index = image::unpack_int<image_bit_width>(image_buffer[pixel_offset]);
            if (ARCHON_LIKELY(core::int_less(index, palette_size))) {
                int n = palette_format_type::num_channels;
                const palette_comp_type* color = palette_buffer.data() + index * n;
                for (int i = 0; i < n; ++i)
                    ARCHON_CHECK_EQUAL(pixel[i], color[i]);
                ARCHON_CHECK_EQUAL(pixel[n], image::comp_repr_max<palette_comp_repr>());
            }
            else {
                int n = pixel_repr_type::num_channels;
                for (int i = 0; i < n; ++i)
                    ARCHON_CHECK_EQUAL(pixel[i], 0);
            }
        }
    }
}


ARCHON_TEST_BATCH(Image_Reader_GetBlock_IndirectColorDissimilarFormats, color_space_variants)
{
    // Generate palette with randomized colors
    constexpr image::ColorSpace::Tag color_space_tag = test_value;
    constexpr bool palette_has_alpha = true;
    using channel_spec_type = image::StandardChannelSpec<color_space_tag, palette_has_alpha>;
    constexpr image::CompRepr palette_comp_repr = image::CompRepr::int8;
    using palette_comp_type = image::comp_type<palette_comp_repr>;
    constexpr int palette_bit_width = image::comp_repr_bit_width<palette_comp_repr>();
    using palette_format_type = image::IntegerPixelFormat<channel_spec_type, palette_comp_type,
                                                          palette_bit_width>;
    int palette_size = 8;
    image::BufferedImage<palette_format_type> palette({ palette_size, 1 });
    auto palette_buffer = palette.get_buffer();
    std::mt19937_64 random(test_context.seed_seq());
    for (std::size_t i = 0; i < palette_buffer.size(); ++i)
        palette_buffer[i] = test::rand_comp<palette_comp_repr>(random);

    // Generate image with randomized color indexes and a chance for an index to be out of range
    constexpr image::CompRepr image_comp_repr = image::CompRepr::int8;
    using image_comp_type = image::comp_type<image_comp_repr>;
    constexpr int image_bit_width = image::comp_repr_bit_width<image_comp_repr>();
    image::Size image_size = { 8, 8 };
    image::IndexedPixelFormat<image_comp_type, image_bit_width> format(palette);
    image::BufferedImage image(image_size, std::move(format));
    auto image_buffer = image.get_buffer();
    for (std::size_t i = 0; i < image_buffer.size(); ++i) {
        int max_index = palette_size; // max is then out of range by one
        int index = core::rand_int_max(random, max_index);
        image_buffer[i] = image::pack_int<image_comp_type, image_bit_width>(index);
    }

    // Read block from image using same color space as palette, but without the alpha
    // channel (this makes it a dissimilar format)
    image::Reader reader(image);
    reader.set_background_color(util::colors::transparent);
    image::Pos pos = { 1, 1 };
    image::Size block_size = { 6, 6 };
    constexpr bool read_has_alpha = false;
    using pixel_repr_type = image::PixelRepr<color_space_tag, read_has_alpha, palette_comp_repr>;
    image::PixelBlock<pixel_repr_type> block(block_size);
    reader.get_block(pos, block);
    for (int y = 0; y < block_size.height; ++y) {
        for (int x = 0; x < block_size.width; ++x) {
            image::Pixel<pixel_repr_type> pixel = block.get_pixel({ x, y });
            auto pixel_offset = pos.x + x + (pos.y + y) * std::size_t(image_size.width);
            auto index = image::unpack_int<image_bit_width>(image_buffer[pixel_offset]);
            if (ARCHON_LIKELY(core::int_less(index, palette_size))) {
                int n = palette_format_type::num_channels;
                const palette_comp_type* color = palette_buffer.data() + index * n;
                image::float_type alpha =
                    image::int_to_float<palette_bit_width, image::float_type>(color[n - 1]);
                for (int i = 0; i < n - 1; ++i) {
                    image::float_type value_1 =
                        alpha * image::compressed_int_to_float<palette_bit_width>(color[i]);
                    palette_comp_type value_2 =
                        image::float_to_compressed_int<palette_comp_type,
                                                       palette_bit_width>(value_1);
                    ARCHON_CHECK_DIST_LESS_EQUAL(image::unpack_int<palette_bit_width>(pixel[i]),
                                                 image::unpack_int<palette_bit_width>(value_2), 1);
                }
            }
            else {
                int n = pixel_repr_type::num_channels;
                for (int i = 0; i < n; ++i)
                    ARCHON_CHECK_EQUAL(pixel[i], 0);
            }
        }
    }
}


ARCHON_TEST_BATCH(Image_Reader_GetBlock_Falloff, pixel_repr_variants)
{
    using pixel_repr_type = test_type;
    constexpr auto comp_repr = pixel_repr_type::comp_repr;
    using comp_type = image::comp_type<comp_repr>;
    constexpr int num_channels = pixel_repr_type::num_channels;
    using block_type = image::PixelBlock<pixel_repr_type>;
    using pixel_type = typename block_type::pixel_type;
    pixel_type background_color = pixel_type(util::colors::transparent);

    auto test = [&](check::TestContext& parent_test_context, image::Size image_size) {
        ARCHON_TEST_TRAIL(parent_test_context, image_size);
        block_type image_block(image_size);
        auto image_buffer = image_block.buffer();
        double frac = 0;
        for (std::size_t i = 0; i < image_buffer.size(); ++i) {
            constexpr int bit_width = image::comp_repr_bit_width<comp_repr>();
            image_buffer[i] = image::float_to_int<comp_type, bit_width>(frac);
            frac = std::fmod(frac + core::golden_fraction<double>(), 1.0);
        }

        auto get_expected_pixel = [&](image::Pos pos, image::Reader::FalloffMode horz_mode,
                                 image::Reader::FalloffMode vert_mode) {
            if (image_size.is_empty())
                return background_color;
            switch (horz_mode) {
                case image::Reader::FalloffMode::background:
                    if (pos.x < 0 || pos.x >= image_size.width)
                        return background_color;
                    break;
                case image::Reader::FalloffMode::edge:
                    pos.x = std::clamp(pos.x, 0, image_size.width - 1);
                    break;
                case image::Reader::FalloffMode::repeat:
                    pos.x = core::int_periodic_mod(pos.x, image_size.width);
                    break;
            }
            switch (vert_mode) {
                case image::Reader::FalloffMode::background:
                    if (pos.y < 0 || pos.y >= image_size.height)
                        return background_color;
                    break;
                case image::Reader::FalloffMode::edge:
                    pos.y = std::clamp(pos.y, 0, image_size.height - 1);
                    break;
                case image::Reader::FalloffMode::repeat:
                    pos.y = core::int_periodic_mod(pos.y, image_size.height);
                    break;
            }
            return image_block.get_pixel(pos);
        };

        image::TrayImage image(image_block);
        image::Reader reader(image);
        reader.set_background_color(background_color);

        image::Size max_falloff = max(2 * image_size, image::Size(1));
        image::Size block_size = image_size + 2 * max_falloff;
        block_type block(block_size);

        auto test_1 = [&](check::TestContext& parent_test_context, const image::Box& box,
                          image::Reader::FalloffMode horz_mode, image::Reader::FalloffMode vert_mode) {
            ARCHON_TEST_TRAIL(parent_test_context, box);
            auto tray = block.tray().subtray(box.size);
            reader.get_block<pixel_repr_type>(box.pos, tray);
            for (int y = 0; y < box.size.height; ++y) {
                for (int x = 0; x < box.size.width; ++x) {
                    const comp_type* pixel_1 = tray(x, y);
                    image::Pos pos = box.pos + image::Size(x, y);
                    pixel_type pixel_2 = get_expected_pixel(pos, horz_mode, vert_mode);
                    ARCHON_CHECK(std::equal(pixel_1, pixel_1 + num_channels, pixel_2.data()));
                }
            }
        };

        auto test_2 = [&](check::TestContext& parent_test_context, image::Reader::FalloffMode horz_mode,
                          image::Reader::FalloffMode vert_mode) {
            ARCHON_TEST_TRAIL(parent_test_context, core::formatted("%s, %s", horz_mode, vert_mode));
            reader.set_falloff_mode(horz_mode, vert_mode);
            test::for_each_box_in({ image::Pos() - max_falloff, block_size }, [&](const image::Box& box) {
                test_1(test_context, box, horz_mode, vert_mode);
            });
        };

        auto test_3 = [&](image::Reader::FalloffMode vert_mode) {
            test_2(test_context, image::Reader::FalloffMode::background, vert_mode);
            test_2(test_context, image::Reader::FalloffMode::edge, vert_mode);
            test_2(test_context, image::Reader::FalloffMode::repeat, vert_mode);
        };

        test_3(image::Reader::FalloffMode::background);
        test_3(image::Reader::FalloffMode::edge);
        test_3(image::Reader::FalloffMode::repeat);
    };

    test::for_each_pos_in(image::Size(3, 3), [&](const image::Pos& pos) {
        test(test_context, pos - image::Pos());
    });
}


ARCHON_TEST_BATCH(Image_Reader_SetColor_ShortCircuitSameAlpha, pixel_repr_variants)
{
    using pixel_repr_type = test_type;
    constexpr auto comp_repr = pixel_repr_type::comp_repr;
    using block_type = image::PixelBlock<pixel_repr_type>;
    using pixel_type = typename block_type::pixel_type;
    block_type block;
    image::TrayImage image(block);
    image::Reader reader(image);
    std::mt19937_64 random(test_context.seed_seq());
    long num_rounds = 256;
    for (long i = 0; i < num_rounds; ++i) {
        pixel_type color_1, color_2;
        for (std::size_t j = 0; j < color_1.size(); ++j)
            color_1[j] = test::rand_comp<comp_repr>(random);
        reader.set_color(image::Reader::ColorSlot::background, color_1);
        reader.get_color(image::Reader::ColorSlot::background, color_2);
        ARCHON_CHECK_EQUAL_SEQ(core::Span(color_2), core::Span(color_1));
    }
}
