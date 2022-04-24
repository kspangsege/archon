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

                 
#include <cmath>
#include <array>

#include <archon/core/span.hpp>
#include <archon/core/demangle.hpp>
#include <archon/core/format.hpp>
#include <archon/check.hpp>
#include <archon/util/color.hpp>
#include <archon/util/colors.hpp>
#include <archon/util/as_css_color.hpp>
#include <archon/image/size.hpp>
#include <archon/image/comp_types.hpp>
#include <archon/image/gamma.hpp>
#include <archon/image/pixel.hpp>
#include <archon/image/block.hpp>
#include <archon/image/tray_image.hpp>
#include <archon/image/indexed_tray_image.hpp>
#include <archon/image/indexed_pixel_format.hpp>
#include <archon/image/buffered_image.hpp>
#include <archon/image/palettes.hpp>
#include <archon/image/writer.hpp>
#include <archon/image/test/box_utils.hpp>
#include <archon/image/test/comp_repr_utils.hpp>
#include <archon/image/test/pixel_utils.hpp>


using namespace archon;
namespace test = image::test;


namespace {


ARCHON_TEST_VARIANTS(pixel_repr_variants,
                     ARCHON_TEST_TYPE(image::Lum_8,  Lum_8),
                     ARCHON_TEST_TYPE(image::LumA_8, LumA_8),
                     ARCHON_TEST_TYPE(image::RGB_8,  RGB_8),
                     ARCHON_TEST_TYPE(image::RGBA_8, RGBA_8),
                     ARCHON_TEST_TYPE(image::Lum_16,  Lum_16),
                     ARCHON_TEST_TYPE(image::LumA_16, LumA_16),
                     ARCHON_TEST_TYPE(image::RGB_16,  RGB_16),
                     ARCHON_TEST_TYPE(image::RGBA_16, RGBA_16),
                     ARCHON_TEST_TYPE(image::Lum_F,  Lum_F),
                     ARCHON_TEST_TYPE(image::LumA_F, LumA_F),
                     ARCHON_TEST_TYPE(image::RGB_F,  RGB_F),
                     ARCHON_TEST_TYPE(image::RGBA_F, RGBA_F));


} // unnamed namespace


ARCHON_TEST_BATCH(Image_Writer_Fill_DirectColor, pixel_repr_variants)
{
    // FIXME: How to craft test to activate subdivision?                                           

    std::mt19937_64 random(test_context.seed_seq());

    image::Size image_size = { 5, 5 };
    image::Size margin = 1;
    image::Size max_fill_size = 2 * margin + image_size;
    using image_repr_type = test_type;
    using image_block_type = image::PixelBlock<image_repr_type>;
    using image_pixel_type = typename image_block_type::pixel_type;
    image_block_type image_block_1(image_size);
    image_block_type image_block_2(image_size);
    core::Span image_buffer_1 = image_block_1.buffer();
    core::Span image_buffer_2 = image_block_2.buffer();
    image::WritableTrayImage image(image_block_1);
    constexpr image::CompRepr image_comp_repr = image_repr_type::comp_repr;
    for (std::size_t i = 0; i < image_buffer_2.size(); ++i)
        image_buffer_2[i] = test::rand_comp<image_comp_repr>(random);

    auto test_1 = [&](check::TestContext& parent_test_context, const auto& fill_color,
                      image::float_type opacity, bool blending, const image::Box& subbox) {
        ARCHON_TEST_TRAIL(parent_test_context, subbox);

        std::copy_n(image_buffer_2.data(), image_buffer_2.size(), image_buffer_1.data());
        image::Writer writer(image);
        writer.set_opacity(opacity);
        writer.set_blending_enabled(blending);
        writer.set_foreground_color(fill_color);
        writer.fill(subbox);

        using promoted_image_pixel_type = typename image_pixel_type::promoted_pixel_type;
        promoted_image_pixel_type fill_color_2 =
            fill_color.template convert<typename image_repr_type::promoted_type>();

        for (int y = 0; y < image_size.height; ++y) {
            for (int x = 0; x < image_size.width; ++x) {
                image::Pos pos = { x, y };
                image_pixel_type pixel_1 = image_block_1.get_pixel(pos);
                image_pixel_type pixel_2 = image_block_2.get_pixel(pos);
                if (image::Box(pos, 1).contained_in(subbox)) {
                    promoted_image_pixel_type color = fill_color_2;
                    color = opacity * color;
                    if (blending)
                        color = color + pixel_2;
                    test::check_approx_equal_pixels(test_context, pixel_1, image_pixel_type(color));
                }
                else {
                    ARCHON_CHECK_EQUAL(pixel_1, pixel_2);
                }
            }
        }
    };

    auto test_2 = [&](check::TestContext& parent_test_context, const auto& fill_color,
                      image::float_type opacity, bool blending) {
        ARCHON_TEST_TRAIL(parent_test_context, core::formatted("%s, %s", opacity, blending));

        image::Box box = { image::Pos() - margin, max_fill_size };
        test::for_each_box_in(box, [&](const image::Box& subbox) {
            test_1(test_context, fill_color, opacity, blending, subbox);
        });
    };

    auto test_3 = [&](check::TestContext& parent_test_context, const auto& fill_color) {
        ARCHON_TEST_TRAIL(parent_test_context, fill_color);

        test_2(test_context, fill_color, 1.0, false);
        test_2(test_context, fill_color, 0.5, false);
        test_2(test_context, fill_color, 0.0, false);
        test_2(test_context, fill_color, 1.0, true);
        test_2(test_context, fill_color, 0.5, true);
        test_2(test_context, fill_color, 0.0, true);
    };

    auto test_4 = [&](check::TestContext& parent_test_context, auto fill_repr, const char* descr) {
        ARCHON_TEST_TRAIL(parent_test_context, descr);

        using fill_repr_type = decltype(fill_repr);
        using fill_color_type = image::Pixel<fill_repr_type>;
        constexpr image::CompRepr fill_comp_repr = fill_repr_type::comp_repr;
        for (int i = 0; i < 8; ++i) {
            fill_color_type fill_color;
            for (int i = 0; i < fill_color.num_channels; ++i)
                fill_color[i] = test::rand_comp<fill_comp_repr>(random);
            test_3(test_context, fill_color);
        }
    };

    test_4(test_context, image::Lum_8(), "Lum_8");
    test_4(test_context, image::Lum_F(), "Lum_F");
    test_4(test_context, image::LumA_8(), "LumA_8");
    test_4(test_context, image::LumA_F(), "LumA_F");
    test_4(test_context, image::RGB_8(), "RGB_8");
    test_4(test_context, image::RGB_F(), "RGB_F");
    test_4(test_context, image::RGBA_8(), "RGBA_8");
    test_4(test_context, image::RGBA_F(), "RGBA_F");
}


ARCHON_TEST_BATCH(Image_Writer_Fill_IndirectColor, pixel_repr_variants)
{
    // FIXME: How to craft test to activate subdivision?                                           
    // FIXME: With varying representations of index (requires support for varying index representation)       

    std::mt19937_64 random(test_context.seed_seq());

    using palette_repr_type = test_type;
    using palette_image_type = image::PaletteImage<palette_repr_type>;
    using palette_pixel_type = typename palette_image_type::pixel_type;
    constexpr image::CompRepr palette_comp_repr = palette_pixel_type::comp_repr;
    constexpr int palette_size = 16;
    palette_pixel_type palette_colors[palette_size];
    {
        double frac = core::rand_float<double>(random);
        for (int i = 0; i < palette_pixel_type::num_channels; ++i) {
            for (int j = 0; j < palette_size; ++j) {
                // We can use alpha-type representation for all channels here, because we
                // are pseudo-randomizing after all.
                palette_colors[j][i] = image::alpha_comp_from_float<palette_comp_repr>(image::float_type(frac));
                frac = std::fmod(frac + core::golden_fraction<double>(), 1.0);
            }
        }
    }
    palette_image_type palette(palette_colors);

    image::Size image_size = { 5, 5 };
    image::Size margin = 1;
    image::Size max_fill_size = 2 * margin + image_size;
    constexpr image::CompRepr index_repr = image::color_index_repr;
    using image_block_type = image::IndexBlock<index_repr>;
    image_block_type image_block_1(image_size);
    image_block_type image_block_2(image_size);
    core::Span image_buffer_1 = image_block_1.buffer();
    core::Span image_buffer_2 = image_block_2.buffer();
    image::WritableIndexedTrayImage image(image_block_1, palette);
    using unpacked_index_type = image::unpacked_comp_type<index_repr>;
    unpacked_index_type max_index = core::int_mask<unpacked_index_type>(image::comp_repr_bit_width<index_repr>());
    if (core::int_less(palette_size - 1, max_index))
        max_index = unpacked_index_type(palette_size - 1);
    for (std::size_t i = 0; i < image_buffer_2.size(); ++i) {
        unpacked_index_type index = core::rand_int_max(random, max_index);
        image_buffer_2[i] = image::comp_repr_pack<index_repr>(index);
    }

    auto test_1 = [&](check::TestContext& parent_test_context, const auto& fill_color,
                      image::float_type opacity, bool blending, const image::Box& subbox) {
        ARCHON_TEST_TRAIL(parent_test_context, subbox);

        std::copy_n(image_buffer_2.data(), image_buffer_2.size(), image_buffer_1.data());
        image::Writer writer(image);
        writer.set_opacity(opacity);
        writer.set_blending_enabled(blending);
        writer.set_foreground_color(fill_color);
        writer.fill(subbox);

        using promoted_palette_pixel_type = typename palette_pixel_type::promoted_pixel_type;
        promoted_palette_pixel_type fill_color_2 =
            fill_color.template convert<typename palette_repr_type::promoted_type>();

        for (int y = 0; y < image_size.height; ++y) {
            for (int x = 0; x < image_size.width; ++x) {
                image::Pos pos = { x, y };
                std::size_t index_1 = image_block_1.get_index(pos);
                std::size_t index_2 = image_block_2.get_index(pos);
                if (image::Box(pos, 1).contained_in(subbox)) {
                    promoted_palette_pixel_type color = fill_color_2;
                    color = opacity * color;
                    if (blending)
                        color = color + palette_colors[index_2];
                    test::check_color_index(test_context, writer, index_1, color);
                }
                else {
                    ARCHON_CHECK_EQUAL(index_1, index_2);
                }
            }
        }
    };

    auto test_2 = [&](check::TestContext& parent_test_context, const auto& fill_color,
                      image::float_type opacity, bool blending) {
        ARCHON_TEST_TRAIL(parent_test_context, core::formatted("%s, %s", opacity, blending));

        image::Box box = { image::Pos() - margin, max_fill_size };
        test::for_each_box_in(box, [&](const image::Box& subbox) {
            test_1(test_context, fill_color, opacity, blending, subbox);
        });
    };

    auto test_3 = [&](check::TestContext& parent_test_context, const auto& fill_color) {
        ARCHON_TEST_TRAIL(parent_test_context, fill_color);

        test_2(test_context, fill_color, 1.0, false);
        test_2(test_context, fill_color, 0.5, false);
        test_2(test_context, fill_color, 0.0, false);
        test_2(test_context, fill_color, 1.0, true);
        test_2(test_context, fill_color, 0.5, true);
        test_2(test_context, fill_color, 0.0, true);
    };

    auto test_4 = [&](check::TestContext& parent_test_context, auto fill_repr, const char* descr) {
        ARCHON_TEST_TRAIL(parent_test_context, descr);

        using fill_repr_type = decltype(fill_repr);
        using fill_color_type = image::Pixel<fill_repr_type>;
        constexpr image::CompRepr fill_comp_repr = fill_repr_type::comp_repr;
        for (int i = 0; i < 8; ++i) {
            fill_color_type fill_color;
            for (int i = 0; i < fill_color.num_channels; ++i)
                fill_color[i] = test::rand_comp<fill_comp_repr>(random);
            test_3(test_context, fill_color);
        }
    };

    test_4(test_context, image::Lum_8(), "Lum_8");
    test_4(test_context, image::Lum_F(), "Lum_F");
    test_4(test_context, image::LumA_8(), "LumA_8");
    test_4(test_context, image::LumA_F(), "LumA_F");
    test_4(test_context, image::RGB_8(), "RGB_8");
    test_4(test_context, image::RGB_F(), "RGB_F");
    test_4(test_context, image::RGBA_8(), "RGBA_8");
    test_4(test_context, image::RGBA_F(), "RGBA_F");
}


ARCHON_TEST_BATCH(Image_Writer_Fill_Lossless, pixel_repr_variants)
{
    // FIXME: How to craft test to activate subdivision?                                           

    std::mt19937_64 random(test_context.seed_seq());

    image::Size image_size = { 5, 5 };
    image::Size margin = 1;
    image::Size max_fill_size = 2 * margin + image_size;
    using image_repr_type = test_type;
    using image_block_type = image::PixelBlock<image_repr_type>;
    using image_pixel_type = typename image_block_type::pixel_type;
    image_block_type image_block_1(image_size);
    image_block_type image_block_2(image_size);
    core::Span image_buffer_1 = image_block_1.buffer();
    core::Span image_buffer_2 = image_block_2.buffer();
    image::WritableTrayImage image(image_block_1);
    constexpr image::CompRepr image_comp_repr = image_repr_type::comp_repr;
    for (std::size_t i = 0; i < image_buffer_2.size(); ++i)
        image_buffer_2[i] = test::rand_comp<image_comp_repr>(random);

    auto test_1 = [&](check::TestContext& parent_test_context, const auto& fill_color, const image::Box& subbox) {
        ARCHON_TEST_TRAIL(parent_test_context, subbox);

        std::copy_n(image_buffer_2.data(), image_buffer_2.size(), image_buffer_1.data());
        image::Writer writer(image);
        writer.set_foreground_color(fill_color);
        writer.fill(subbox);

        image_pixel_type fill_color_2 = image_pixel_type(fill_color);

        for (int y = 0; y < image_size.height; ++y) {
            for (int x = 0; x < image_size.width; ++x) {
                image::Pos pos = { x, y };
                image_pixel_type pixel = image_block_1.get_pixel(pos);
                if (image::Box(pos, 1).contained_in(subbox)) {
                    ARCHON_CHECK_EQUAL(pixel, fill_color_2);
                }
                else {
                    image_pixel_type pixel_2 = image_block_2.get_pixel(pos);
                    ARCHON_CHECK_EQUAL(pixel, pixel_2);
                }
            }
        }
    };

    auto test_2 = [&](check::TestContext& parent_test_context, const auto& fill_color) {
        ARCHON_TEST_TRAIL(parent_test_context, fill_color);

        image::Box box = { image::Pos() - margin, max_fill_size };
        test::for_each_box_in(box, [&](const image::Box& subbox) {
            test_1(test_context, fill_color, subbox);
        });
    };

    auto test_3 = [&](check::TestContext& parent_test_context, auto fill_repr, const char* descr) {
        ARCHON_TEST_TRAIL(parent_test_context, descr);

        using fill_repr_type = decltype(fill_repr);
        using fill_color_type = image::Pixel<fill_repr_type>;
        constexpr image::CompRepr fill_comp_repr = fill_repr_type::comp_repr;
        for (int i = 0; i < 8; ++i) {
            fill_color_type fill_color;
            for (int i = 0; i < fill_color.num_channels; ++i)
                fill_color[i] = test::rand_comp<fill_comp_repr>(random);
            bool remove_alpha = (fill_repr_type::has_alpha && !image_repr_type::has_alpha);
            if (remove_alpha)
                fill_color[fill_color.num_channels - 1] = image::comp_repr_max<fill_comp_repr>();
            test_2(test_context, fill_color);
        }
    };

    test_3(test_context, image_repr_type(), "Same alpha");
    if constexpr (image_repr_type::has_alpha) {
        using repr_type = image::PixelRepr<image_repr_type::color_space_tag, false, image_repr_type::comp_repr>;
        test_3(test_context, repr_type(), "Add alpha");
    }
    else {
        using repr_type = image::PixelRepr<image_repr_type::color_space_tag, true, image_repr_type::comp_repr>;
        test_3(test_context, repr_type(), "Remove alpha");
    }
}


ARCHON_TEST_BATCH(Image_Writer_PutBlock_DirectColor, pixel_repr_variants)
{
    // FIXME: How to craft test to activate subdivision?                                           

    std::mt19937_64 random(test_context.seed_seq());

    image::Size image_size = { 5, 5 };
    image::Size margin = 1;
    image::Size max_write_size = 2 * margin + image_size;
    using image_repr_type = test_type;
    using image_block_type = image::PixelBlock<image_repr_type>;
    using image_pixel_type = typename image_block_type::pixel_type;
    image_block_type image_block_1(image_size);
    image_block_type image_block_2(image_size);
    core::Span image_buffer_1 = image_block_1.buffer();
    core::Span image_buffer_2 = image_block_2.buffer();
    image::WritableTrayImage image(image_block_1);
    constexpr image::CompRepr image_comp_repr = image_repr_type::comp_repr;
    for (std::size_t i = 0; i < image_buffer_2.size(); ++i)
        image_buffer_2[i] = test::rand_comp<image_comp_repr>(random);

    auto test_1 = [&](check::TestContext& parent_test_context, const auto& write_block,
                      image::float_type opacity, bool blending, const image::Box& subbox) {
        ARCHON_TEST_TRAIL(parent_test_context, subbox);

        std::copy_n(image_buffer_2.data(), image_buffer_2.size(), image_buffer_1.data());
        image::Writer writer(image);
        writer.set_opacity(opacity);
        writer.set_blending_enabled(blending);
        image::Tray tray = write_block.tray().subtray(subbox.size);
        using write_block_type = std::decay_t<decltype(write_block)>;
        using write_repr_type = typename write_block_type::pixel_repr_type;
        constexpr image::CompRepr write_comp_repr = write_repr_type::comp_repr;
        const image::ColorSpace& write_color_space = write_repr_type::get_color_space();
        constexpr bool write_has_alpha = write_repr_type::has_alpha;
        writer.put_block_a<write_comp_repr>(subbox.pos, tray, write_color_space, write_has_alpha);

        for (int y = 0; y < image_size.height; ++y) {
            for (int x = 0; x < image_size.width; ++x) {
                image::Pos pos = { x, y };
                image_pixel_type pixel_1 = image_block_1.get_pixel(pos);
                image_pixel_type pixel_2 = image_block_2.get_pixel(pos);
                if (image::Box(pos, 1).contained_in(subbox)) {
                    using write_pixel_type = typename write_block_type::pixel_type;
                    write_pixel_type pixel_3 = write_block.get_pixel(image::Pos() + (pos - subbox.pos));
                    using promoted_image_pixel_type = typename image_pixel_type::promoted_pixel_type;
                    promoted_image_pixel_type pixel_4 =
                        pixel_3.template convert<typename image_repr_type::promoted_type>();
                    pixel_4 = opacity * pixel_4;
                    if (blending)
                        pixel_4 = pixel_4 + pixel_2;
                    test::check_approx_equal_pixels(test_context, pixel_1, image_pixel_type(pixel_4));
                }
                else {
                    ARCHON_CHECK_EQUAL(pixel_1, pixel_2);
                }
            }
        }
    };

    auto test_2 = [&](check::TestContext& parent_test_context, const auto& write_block,
                      image::float_type opacity, bool blending) {
        ARCHON_TEST_TRAIL(parent_test_context, core::formatted("%s, %s", opacity, blending));

        image::Box box = { image::Pos() - margin, max_write_size };
        test::for_each_box_in(box, [&](const image::Box& subbox) {
            test_1(test_context, write_block, opacity, blending, subbox);
        });
    };

    auto test_3 = [&](check::TestContext& parent_test_context, auto write_repr, const char* descr) {
        ARCHON_TEST_TRAIL(parent_test_context, descr);

        using write_repr_type = decltype(write_repr);
        using write_block_type = image::PixelBlock<write_repr_type>;
        write_block_type write_block(max_write_size);
        core::Span write_buffer = write_block.buffer();
        constexpr image::CompRepr write_comp_repr = write_repr_type::comp_repr;
        for (std::size_t i = 0; i < write_buffer.size(); ++i)
            write_buffer[i] = test::rand_comp<write_comp_repr>(random);

        test_2(test_context, write_block, 1.0, false);
        test_2(test_context, write_block, 0.5, false);
        test_2(test_context, write_block, 0.0, false);
        test_2(test_context, write_block, 1.0, true);
        test_2(test_context, write_block, 0.5, true);
        test_2(test_context, write_block, 0.0, true);
    };

    test_3(test_context, image::Lum_8(), "Lum_8");
    test_3(test_context, image::Lum_F(), "Lum_F");
    test_3(test_context, image::LumA_8(), "LumA_8");
    test_3(test_context, image::LumA_F(), "LumA_F");
    test_3(test_context, image::RGB_8(), "RGB_8");
    test_3(test_context, image::RGB_F(), "RGB_F");
    test_3(test_context, image::RGBA_8(), "RGBA_8");
    test_3(test_context, image::RGBA_F(), "RGBA_F");
}


ARCHON_TEST_BATCH(Image_Writer_PutBlock_IndirectColor, pixel_repr_variants)
{
    // FIXME: How to craft test to activate subdivision?                                           
    // FIXME: With varying representations of index (requires support for varying index representation)       

    std::mt19937_64 random(test_context.seed_seq());

    using palette_repr_type = test_type;
    using palette_image_type = image::PaletteImage<palette_repr_type>;
    using palette_pixel_type = typename palette_image_type::pixel_type;
    constexpr image::CompRepr palette_comp_repr = palette_pixel_type::comp_repr;
    constexpr int palette_size = 16;
    palette_pixel_type palette_colors[palette_size];
    {
        double frac = core::rand_float<double>(random);
        for (int i = 0; i < palette_pixel_type::num_channels; ++i) {
            for (int j = 0; j < palette_size; ++j) {
                // We can use alpha-type representation for all channels here, because we
                // are pseudo-randomizing after all.
                palette_colors[j][i] = image::alpha_comp_from_float<palette_comp_repr>(image::float_type(frac));
                frac = std::fmod(frac + core::golden_fraction<double>(), 1.0);
            }
        }
    }
    palette_image_type palette(palette_colors);

    image::Size image_size = { 5, 5 };
    image::Size margin = 1;
    image::Size max_write_size = 2 * margin + image_size;
    constexpr image::CompRepr index_repr = image::color_index_repr;
    using image_block_type = image::IndexBlock<index_repr>;
    image_block_type image_block_1(image_size);
    image_block_type image_block_2(image_size);
    core::Span image_buffer_1 = image_block_1.buffer();
    core::Span image_buffer_2 = image_block_2.buffer();
    image::WritableIndexedTrayImage image(image_block_1, palette);
    using unpacked_index_type = image::unpacked_comp_type<index_repr>;
    unpacked_index_type max_index = core::int_mask<unpacked_index_type>(image::comp_repr_bit_width<index_repr>());
    if (core::int_less(palette_size - 1, max_index))
        max_index = unpacked_index_type(palette_size - 1);
    for (std::size_t i = 0; i < image_buffer_2.size(); ++i) {
        unpacked_index_type index = core::rand_int_max(random, max_index);
        image_buffer_2[i] = image::comp_repr_pack<index_repr>(index);
    }

    auto test_1 = [&](check::TestContext& parent_test_context, const auto& write_block,
                      image::float_type opacity, bool blending, const image::Box& subbox) {
        ARCHON_TEST_TRAIL(parent_test_context, subbox);

        std::copy_n(image_buffer_2.data(), image_buffer_2.size(), image_buffer_1.data());
        image::Writer writer(image);
        writer.set_opacity(opacity);
        writer.set_blending_enabled(blending);
        image::Tray tray = write_block.tray().subtray(subbox.size);
        using write_block_type = std::decay_t<decltype(write_block)>;
        using write_repr_type = typename write_block_type::pixel_repr_type;
        constexpr image::CompRepr write_comp_repr = write_repr_type::comp_repr;
        const image::ColorSpace& write_color_space = write_repr_type::get_color_space();
        constexpr bool write_has_alpha = write_repr_type::has_alpha;
        writer.put_block_a<write_comp_repr>(subbox.pos, tray, write_color_space, write_has_alpha);

        for (int y = 0; y < image_size.height; ++y) {
            for (int x = 0; x < image_size.width; ++x) {
                image::Pos pos = { x, y };
                std::size_t index_1 = image_block_1.get_index(pos);
                std::size_t index_2 = image_block_2.get_index(pos);
                if (image::Box(pos, 1).contained_in(subbox)) {
                    using write_pixel_type = typename write_block_type::pixel_type;
                    write_pixel_type color_1 = write_block.get_pixel(image::Pos() + (pos - subbox.pos));
                    using promoted_palette_pixel_type = typename palette_pixel_type::promoted_pixel_type;
                    promoted_palette_pixel_type color_2 =
                        color_1.template convert<typename palette_repr_type::promoted_type>();
                    color_2 = opacity * color_2;
                    if (blending)
                        color_2 = color_2 + palette_colors[index_2];
                    test::check_color_index(test_context, writer, index_1, color_2);
                }
                else {
                    ARCHON_CHECK_EQUAL(index_1, index_2);
                }
            }
        }
    };

    auto test_2 = [&](check::TestContext& parent_test_context, const auto& write_block,
                      image::float_type opacity, bool blending) {
        ARCHON_TEST_TRAIL(parent_test_context, core::formatted("%s, %s", opacity, blending));

        image::Box box = { image::Pos() - margin, max_write_size };
        test::for_each_box_in(box, [&](const image::Box& subbox) {
            test_1(test_context, write_block, opacity, blending, subbox);
        });
    };

    auto test_3 = [&](check::TestContext& parent_test_context, auto write_repr, const char* descr) {
        ARCHON_TEST_TRAIL(parent_test_context, descr);

        using write_repr_type = decltype(write_repr);
        using write_block_type = image::PixelBlock<write_repr_type>;
        write_block_type write_block(max_write_size);
        core::Span write_buffer = write_block.buffer();
        constexpr image::CompRepr write_comp_repr = write_repr_type::comp_repr;
        for (std::size_t i = 0; i < write_buffer.size(); ++i)
            write_buffer[i] = test::rand_comp<write_comp_repr>(random);

        test_2(test_context, write_block, 1.0, false);
        test_2(test_context, write_block, 0.5, false);
        test_2(test_context, write_block, 0.0, false);
        test_2(test_context, write_block, 1.0, true);
        test_2(test_context, write_block, 0.5, true);
        test_2(test_context, write_block, 0.0, true);
    };

    test_3(test_context, image::Lum_8(), "Lum_8");
    test_3(test_context, image::Lum_F(), "Lum_F");
    test_3(test_context, image::LumA_8(), "LumA_8");
    test_3(test_context, image::LumA_F(), "LumA_F");
    test_3(test_context, image::RGB_8(), "RGB_8");
    test_3(test_context, image::RGB_F(), "RGB_F");
    test_3(test_context, image::RGBA_8(), "RGBA_8");
    test_3(test_context, image::RGBA_F(), "RGBA_F");
}


ARCHON_TEST_BATCH(Image_Writer_PutBlock_Lossless, pixel_repr_variants)
{
    // FIXME: How to craft test to activate subdivision?                                           

    std::mt19937_64 random(test_context.seed_seq());

    image::Size image_size = { 5, 5 };
    image::Size margin = 1;
    image::Size max_write_size = 2 * margin + image_size;
    using image_repr_type = test_type;
    using image_block_type = image::PixelBlock<image_repr_type>;
    using image_pixel_type = typename image_block_type::pixel_type;
    image_block_type image_block_1(image_size);
    image_block_type image_block_2(image_size);
    core::Span image_buffer_1 = image_block_1.buffer();
    core::Span image_buffer_2 = image_block_2.buffer();
    image::WritableTrayImage image(image_block_1);
    constexpr image::CompRepr image_comp_repr = image_repr_type::comp_repr;
    for (std::size_t i = 0; i < image_buffer_2.size(); ++i)
        image_buffer_2[i] = test::rand_comp<image_comp_repr>(random);

    auto test_1 = [&](check::TestContext& parent_test_context, const auto& write_block, const image::Box& subbox) {
        ARCHON_TEST_TRAIL(parent_test_context, subbox);

        std::copy_n(image_buffer_2.data(), image_buffer_2.size(), image_buffer_1.data());
        image::Writer writer(image);
        image::Tray tray = write_block.tray().subtray(subbox.size);
        using write_block_type = std::decay_t<decltype(write_block)>;
        using write_repr_type = typename write_block_type::pixel_repr_type;
        constexpr image::CompRepr write_comp_repr = write_repr_type::comp_repr;
        const image::ColorSpace& write_color_space = write_repr_type::get_color_space();
        constexpr bool write_has_alpha = write_repr_type::has_alpha;
        writer.put_block_a<write_comp_repr>(subbox.pos, tray, write_color_space, write_has_alpha);

        for (int y = 0; y < image_size.height; ++y) {
            for (int x = 0; x < image_size.width; ++x) {
                image::Pos pos = { x, y };
                image_pixel_type pixel_1 = image_block_1.get_pixel(pos);
                if (image::Box(pos, 1).contained_in(subbox)) {
                    using write_pixel_type = typename write_block_type::pixel_type;
                    write_pixel_type pixel_2 = write_block.get_pixel(image::Pos() + (pos - subbox.pos));
                    ARCHON_CHECK_EQUAL(pixel_1, image_pixel_type(pixel_2));
                }
                else {
                    image_pixel_type pixel_2 = image_block_2.get_pixel(pos);
                    ARCHON_CHECK_EQUAL(pixel_1, pixel_2);
                }
            }
        }
    };

    auto test_2 = [&](check::TestContext& parent_test_context, auto write_repr, const char* descr) {
        ARCHON_TEST_TRAIL(parent_test_context, descr);

        using write_repr_type = decltype(write_repr);
        using write_block_type = image::PixelBlock<write_repr_type>;
        write_block_type write_block(max_write_size);
        core::Span write_buffer = write_block.buffer();
        constexpr image::CompRepr write_comp_repr = write_repr_type::comp_repr;
        for (std::size_t i = 0; i < write_buffer.size(); ++i)
            write_buffer[i] = test::rand_comp<write_comp_repr>(random);

        image::Box box = { image::Pos() - margin, max_write_size };
        test::for_each_box_in(box, [&](const image::Box& subbox) {
            test_1(test_context, write_block, subbox);
        });
    };

    test_2(test_context, image_repr_type(), "Same alpha");
    if constexpr (image_repr_type::has_alpha) {
        using repr_type = image::PixelRepr<image_repr_type::color_space_tag, false, image_repr_type::comp_repr>;
        test_2(test_context, repr_type(), "Add alpha");
    }
}


ARCHON_TEST_BATCH(Image_Writer_PutBlockMask_DirectColor, pixel_repr_variants)
{
    // FIXME: How to craft test to activate subdivision?                                           
    // FIXME: With varying representations of mask (requires support for varying mask representation to be added to Writer)       

    std::mt19937_64 random(test_context.seed_seq());

    image::Size image_size = { 5, 5 };
    image::Size margin = 1;
    image::Size max_mask_size = 2 * margin + image_size;
    using image_repr_type = test_type;
    using image_block_type = image::PixelBlock<image_repr_type>;
    using image_pixel_type = typename image_block_type::pixel_type;
    image_block_type image_block_1(image_size);
    image_block_type image_block_2(image_size);
    core::Span image_buffer_1 = image_block_1.buffer();
    core::Span image_buffer_2 = image_block_2.buffer();
    image::WritableTrayImage image(image_block_1);
    constexpr image::CompRepr image_comp_repr = image_repr_type::comp_repr;
    for (std::size_t i = 0; i < image_buffer_2.size(); ++i)
        image_buffer_2[i] = test::rand_comp<image_comp_repr>(random);

    auto test_1 = [&](check::TestContext& parent_test_context, const auto& mask_block, util::Color bg, util::Color fg,
                      image::float_type opacity, bool blending, const image::Box& subbox) {
        ARCHON_TEST_TRAIL(parent_test_context, subbox);

        std::copy_n(image_buffer_2.data(), image_buffer_2.size(), image_buffer_1.data());
        using promoted_pixel_type = typename image_pixel_type::promoted_pixel_type;
        auto bg_2 = promoted_pixel_type(bg);
        auto fg_2 = promoted_pixel_type(fg);
        image::Writer writer(image);
        writer.set_background_color(bg_2);
        writer.set_foreground_color(fg_2);
        writer.set_opacity(opacity);
        writer.set_blending_enabled(blending);
        image::Tray tray = mask_block.tray().subtray(subbox.size);
        writer.put_block_mask(subbox.pos, tray);

        for (int y = 0; y < image_size.height; ++y) {
            for (int x = 0; x < image_size.width; ++x) {
                image::Pos pos = { x, y };
                image_pixel_type pixel_1 = image_block_1.get_pixel(pos);
                image_pixel_type pixel_2 = image_block_2.get_pixel(pos);
                if (image::Box(pos, 1).contained_in(subbox)) {
                    auto alpha = mask_block.get_pixel(image::Pos() + (pos - subbox.pos)).promote()[0];
                    promoted_pixel_type color = opacity * (alpha * fg_2 + bg_2);
                    if (blending)
                        color = color + pixel_2;
                    test::check_approx_equal_pixels(test_context, pixel_1, image_pixel_type(color));
                }
                else {
                    ARCHON_CHECK_EQUAL(pixel_1, pixel_2);
                }
            }
        }
    };

    auto test_2 = [&](check::TestContext& parent_test_context, const auto& mask_block, util::Color bg, util::Color fg,
                      image::float_type opacity, bool blending) {
        ARCHON_TEST_TRAIL(parent_test_context, core::formatted("%s, %s", opacity, blending));

        image::Box box = { image::Pos() - margin, max_mask_size };
        test::for_each_box_in(box, [&](const image::Box& subbox) {
            test_1(test_context, mask_block, bg, fg, opacity, blending, subbox);
        });
    };

    auto test_3 = [&](check::TestContext& parent_test_context, const auto& mask_block,
                      util::Color bg, util::Color fg) {
        ARCHON_TEST_TRAIL(parent_test_context, core::formatted("%s, %s", util::as_css_color(bg), as_css_color(fg)));

        test_2(test_context, mask_block, bg, fg, 1.0, false);
        test_2(test_context, mask_block, bg, fg, 0.5, false);
        test_2(test_context, mask_block, bg, fg, 0.0, false);
        test_2(test_context, mask_block, bg, fg, 1.0, true);
        test_2(test_context, mask_block, bg, fg, 0.5, true);
        test_2(test_context, mask_block, bg, fg, 0.0, true);
    };

    constexpr image::CompRepr mask_comp_repr = image::CompRepr::int8;
    using mask_repr_type = image::PixelRepr<image::ColorSpace::Tag::degen, true, mask_comp_repr>;
    using mask_block_type = image::PixelBlock<mask_repr_type>;
    mask_block_type mask_block(max_mask_size);
    core::Span mask_buffer = mask_block.buffer();
    for (std::size_t i = 0; i < mask_buffer.size(); ++i)
        mask_buffer[i] = test::rand_comp<mask_comp_repr>(random);

    test_3(test_context, mask_block, util::Color::from_rgba(0x00'00'00'00), util::Color::from_rgba(0xFF'FF'FF'FF));
    test_3(test_context, mask_block, util::Color::from_rgba(0x4B'07'82'FF), util::Color::from_rgba(0xE6'E6'FA'FF));
}


ARCHON_TEST_BATCH(Image_Writer_PutBlockMask_IndirectColor, pixel_repr_variants)
{
    // FIXME: How to craft test to activate subdivision?                                           
    // FIXME: With varying representations of index (requires support for varying index representation)       
    // FIXME: With varying representations of mask (requires support for varying mask representation to be added to Writer)       

    std::mt19937_64 random(test_context.seed_seq());

    using palette_repr_type = test_type;
    using palette_image_type = image::PaletteImage<palette_repr_type>;
    using palette_pixel_type = typename palette_image_type::pixel_type;
    constexpr image::CompRepr palette_comp_repr = palette_pixel_type::comp_repr;
    constexpr int palette_size = 16;
    palette_pixel_type palette_colors[palette_size];
    {
        double frac = core::rand_float<double>(random);
        for (int i = 0; i < palette_pixel_type::num_channels; ++i) {
            for (int j = 0; j < palette_size; ++j) {
                // We can use alpha-type representation for all channels here, because we
                // are pseudo-randomizing after all.
                palette_colors[j][i] = image::alpha_comp_from_float<palette_comp_repr>(image::float_type(frac));
                frac = std::fmod(frac + core::golden_fraction<double>(), 1.0);
            }
        }
    }
    palette_image_type palette(palette_colors);

    image::Size image_size = { 5, 5 };
    image::Size margin = 1;
    image::Size max_mask_size = 2 * margin + image_size;
    constexpr image::CompRepr index_repr = image::color_index_repr;
    using image_block_type = image::IndexBlock<index_repr>;
    image_block_type image_block_1(image_size);
    image_block_type image_block_2(image_size);
    core::Span image_buffer_1 = image_block_1.buffer();
    core::Span image_buffer_2 = image_block_2.buffer();
    image::WritableIndexedTrayImage image(image_block_1, palette);
    using unpacked_index_type = image::unpacked_comp_type<index_repr>;
    unpacked_index_type max_index = core::int_mask<unpacked_index_type>(image::comp_repr_bit_width<index_repr>());
    if (core::int_less(palette_size - 1, max_index))
        max_index = unpacked_index_type(palette_size - 1);
    for (std::size_t i = 0; i < image_buffer_2.size(); ++i) {
        unpacked_index_type index = core::rand_int_max(random, max_index);
        image_buffer_2[i] = image::comp_repr_pack<index_repr>(index);
    }

    auto test_1 = [&](check::TestContext& parent_test_context, const auto& mask_block, util::Color bg, util::Color fg,
                      image::float_type opacity, bool blending, const image::Box& subbox) {
        ARCHON_TEST_TRAIL(parent_test_context, subbox);

        std::copy_n(image_buffer_2.data(), image_buffer_2.size(), image_buffer_1.data());
        using promoted_pixel_type = typename palette_pixel_type::promoted_pixel_type;
        auto bg_2 = promoted_pixel_type(bg);
        auto fg_2 = promoted_pixel_type(fg);
        image::Writer writer(image);
        writer.set_background_color(bg_2);
        writer.set_foreground_color(fg_2);
        writer.set_opacity(opacity);
        writer.set_blending_enabled(blending);
        image::Tray tray = mask_block.tray().subtray(subbox.size);
        writer.put_block_mask(subbox.pos, tray);

        for (int y = 0; y < image_size.height; ++y) {
            for (int x = 0; x < image_size.width; ++x) {
                image::Pos pos = { x, y };
                std::size_t index_1 = image_block_1.get_index(pos);
                std::size_t index_2 = image_block_2.get_index(pos);
                if (image::Box(pos, 1).contained_in(subbox)) {
                    auto alpha = mask_block.get_pixel(image::Pos() + (pos - subbox.pos)).promote()[0];
                    promoted_pixel_type color = opacity * (alpha * fg_2 + bg_2);
                    if (blending)
                        color = color + palette_colors[index_2];
                    test::check_color_index(test_context, writer, index_1, color);
                }
                else {
                    ARCHON_CHECK_EQUAL(index_1, index_2);
                }
            }
        }
    };

    auto test_2 = [&](check::TestContext& parent_test_context, const auto& mask_block, util::Color bg, util::Color fg,
                      image::float_type opacity, bool blending) {
        ARCHON_TEST_TRAIL(parent_test_context, core::formatted("%s, %s", opacity, blending));

        image::Box box = { image::Pos() - margin, max_mask_size };
        test::for_each_box_in(box, [&](const image::Box& subbox) {
            test_1(test_context, mask_block, bg, fg, opacity, blending, subbox);
        });
    };

    auto test_3 = [&](check::TestContext& parent_test_context, const auto& mask_block,
                      util::Color bg, util::Color fg) {
        ARCHON_TEST_TRAIL(parent_test_context, core::formatted("%s, %s", util::as_css_color(bg), as_css_color(fg)));

        test_2(test_context, mask_block, bg, fg, 1.0, false);
        test_2(test_context, mask_block, bg, fg, 0.5, false);
        test_2(test_context, mask_block, bg, fg, 0.0, false);
        test_2(test_context, mask_block, bg, fg, 1.0, true);
        test_2(test_context, mask_block, bg, fg, 0.5, true);
        test_2(test_context, mask_block, bg, fg, 0.0, true);
    };

    constexpr image::CompRepr mask_comp_repr = image::CompRepr::int8;
    using mask_repr_type = image::PixelRepr<image::ColorSpace::Tag::degen, true, mask_comp_repr>;
    using mask_block_type = image::PixelBlock<mask_repr_type>;
    mask_block_type mask_block(max_mask_size);
    core::Span mask_buffer = mask_block.buffer();
    for (std::size_t i = 0; i < mask_buffer.size(); ++i)
        mask_buffer[i] = test::rand_comp<mask_comp_repr>(random);

    test_3(test_context, mask_block, util::Color::from_rgba(0x00'00'00'00), util::Color::from_rgba(0xFF'FF'FF'FF));
    test_3(test_context, mask_block, util::Color::from_rgba(0x4B'07'82'FF), util::Color::from_rgba(0xE6'E6'FA'FF));
}


ARCHON_TEST_BATCH(Image_Writer_PutImage_DirectColor, pixel_repr_variants)
{
    // FIXME: How to craft test to activate subdivision?                                           

    std::mt19937_64 random(test_context.seed_seq());

    image::Size destin_size = { 5, 5 };
    image::Size margin = 1;
    image::Size max_origin_size = 2 * margin + destin_size;
    using destin_repr_type = test_type;
    using destin_block_type = image::PixelBlock<destin_repr_type>;
    using destin_pixel_type = typename destin_block_type::pixel_type;
    destin_block_type destin_block_1(destin_size);
    destin_block_type destin_block_2(destin_size);
    core::Span destin_buffer_1 = destin_block_1.buffer();
    core::Span destin_buffer_2 = destin_block_2.buffer();
    image::WritableTrayImage destin(destin_block_1);
    constexpr image::CompRepr destin_comp_repr = destin_repr_type::comp_repr;
    for (std::size_t i = 0; i < destin_buffer_2.size(); ++i)
        destin_buffer_2[i] = test::rand_comp<destin_comp_repr>(random);

    auto test_1 = [&](check::TestContext& parent_test_context, const auto& origin_block,
                      image::float_type opacity, bool blending, const image::Box& subbox) {
        ARCHON_TEST_TRAIL(parent_test_context, subbox);

        std::copy_n(destin_buffer_2.data(), destin_buffer_2.size(), destin_buffer_1.data());
        image::Writer writer(destin);
        writer.set_opacity(opacity);
        writer.set_blending_enabled(blending);
        image::TrayImage origin(origin_block, subbox.size);
        writer.put_image(subbox.pos, origin);

        for (int y = 0; y < destin_size.height; ++y) {
            for (int x = 0; x < destin_size.width; ++x) {
                image::Pos pos = { x, y };
                destin_pixel_type pixel_1 = destin_block_1.get_pixel(pos);
                destin_pixel_type pixel_2 = destin_block_2.get_pixel(pos);
                if (image::Box(pos, 1).contained_in(subbox)) {
                    using origin_block_type = std::decay_t<decltype(origin_block)>;
                    using origin_pixel_type = typename origin_block_type::pixel_type;
                    origin_pixel_type pixel_3 = origin_block.get_pixel(image::Pos() + (pos - subbox.pos));
                    using promoted_destin_pixel_type = typename destin_pixel_type::promoted_pixel_type;
                    promoted_destin_pixel_type pixel_4 =
                        pixel_3.template convert<typename destin_repr_type::promoted_type>();
                    pixel_4 = opacity * pixel_4;
                    if (blending)
                        pixel_4 = pixel_4 + pixel_2;
                    test::check_approx_equal_pixels(test_context, pixel_1, destin_pixel_type(pixel_4));
                }
                else {
                    ARCHON_CHECK_EQUAL(pixel_1, pixel_2);
                }
            }
        }
    };

    auto test_2 = [&](check::TestContext& parent_test_context, const auto& origin_block,
                      image::float_type opacity, bool blending) {
        ARCHON_TEST_TRAIL(parent_test_context, core::formatted("%s, %s", opacity, blending));

        image::Box box = { image::Pos() - margin, max_origin_size };
        test::for_each_box_in(box, [&](const image::Box& subbox) {
            test_1(test_context, origin_block, opacity, blending, subbox);
        });
    };

    auto test_3 = [&](check::TestContext& parent_test_context, auto origin_repr, const char* descr) {
        ARCHON_TEST_TRAIL(parent_test_context, descr);

        using origin_repr_type = decltype(origin_repr);
        using origin_block_type = image::PixelBlock<origin_repr_type>;
        origin_block_type origin_block(max_origin_size);
        core::Span origin_buffer = origin_block.buffer();
        constexpr image::CompRepr origin_comp_repr = origin_repr_type::comp_repr;
        for (std::size_t i = 0; i < origin_buffer.size(); ++i)
            origin_buffer[i] = test::rand_comp<origin_comp_repr>(random);

        test_2(test_context, origin_block, 1.0, false);
        test_2(test_context, origin_block, 0.5, false);
        test_2(test_context, origin_block, 0.0, false);
        test_2(test_context, origin_block, 1.0, true);
        test_2(test_context, origin_block, 0.5, true);
        test_2(test_context, origin_block, 0.0, true);
    };

    test_3(test_context, image::Lum_8(), "Lum_8");
    test_3(test_context, image::Lum_F(), "Lum_F");
    test_3(test_context, image::LumA_8(), "LumA_8");
    test_3(test_context, image::LumA_F(), "LumA_F");
    test_3(test_context, image::RGB_8(), "RGB_8");
    test_3(test_context, image::RGB_F(), "RGB_F");
    test_3(test_context, image::RGBA_8(), "RGBA_8");
    test_3(test_context, image::RGBA_F(), "RGBA_F");
}


ARCHON_TEST_BATCH(Image_Writer_PutImage_IndirectColor, pixel_repr_variants)
{
    // FIXME: How to craft test to activate subdivision?                                           
    // FIXME: With varying representations of index (requires support for varying index representation)       

    std::mt19937_64 random(test_context.seed_seq());

    using palette_repr_type = test_type;
    using palette_image_type = image::PaletteImage<palette_repr_type>;
    using palette_pixel_type = typename palette_image_type::pixel_type;
    constexpr image::CompRepr palette_comp_repr = palette_pixel_type::comp_repr;
    constexpr int palette_size = 16;
    palette_pixel_type palette_colors[palette_size];
    {
        double frac = core::rand_float<double>(random);
        for (int i = 0; i < palette_pixel_type::num_channels; ++i) {
            for (int j = 0; j < palette_size; ++j) {
                // We can use alpha-type representation for all channels here, because we
                // are pseudo-randomizing after all.
                palette_colors[j][i] = image::alpha_comp_from_float<palette_comp_repr>(image::float_type(frac));
                frac = std::fmod(frac + core::golden_fraction<double>(), 1.0);
            }
        }
    }
    palette_image_type palette(palette_colors);

    image::Size destin_size = { 5, 5 };
    image::Size margin = 1;
    image::Size max_origin_size = 2 * margin + destin_size;
    constexpr image::CompRepr index_repr = image::color_index_repr;
    using destin_block_type = image::IndexBlock<index_repr>;
    destin_block_type destin_block_1(destin_size);
    destin_block_type destin_block_2(destin_size);
    core::Span destin_buffer_1 = destin_block_1.buffer();
    core::Span destin_buffer_2 = destin_block_2.buffer();
    image::WritableIndexedTrayImage destin(destin_block_1, palette);
    using unpacked_index_type = image::unpacked_comp_type<index_repr>;
    unpacked_index_type max_index = core::int_mask<unpacked_index_type>(image::comp_repr_bit_width<index_repr>());
    if (core::int_less(palette_size - 1, max_index))
        max_index = unpacked_index_type(palette_size - 1);
    for (std::size_t i = 0; i < destin_buffer_2.size(); ++i) {
        unpacked_index_type index = core::rand_int_max(random, max_index);
        destin_buffer_2[i] = image::comp_repr_pack<index_repr>(index);
    }

    auto test_1 = [&](check::TestContext& parent_test_context, const auto& origin_block,
                      image::float_type opacity, bool blending, const image::Box& subbox) {
        ARCHON_TEST_TRAIL(parent_test_context, subbox);

        std::copy_n(destin_buffer_2.data(), destin_buffer_2.size(), destin_buffer_1.data());
        image::Writer writer(destin);
        writer.set_opacity(opacity);
        writer.set_blending_enabled(blending);
        image::TrayImage origin(origin_block, subbox.size);
        writer.put_image(subbox.pos, origin);

        for (int y = 0; y < destin_size.height; ++y) {
            for (int x = 0; x < destin_size.width; ++x) {
                image::Pos pos = { x, y };
                std::size_t index_1 = destin_block_1.get_index(pos);
                std::size_t index_2 = destin_block_2.get_index(pos);
                if (image::Box(pos, 1).contained_in(subbox)) {
                    using origin_block_type = std::decay_t<decltype(origin_block)>;
                    using origin_pixel_type = typename origin_block_type::pixel_type;
                    origin_pixel_type color_1 = origin_block.get_pixel(image::Pos() + (pos - subbox.pos));
                    using promoted_palette_pixel_type = typename palette_pixel_type::promoted_pixel_type;
                    promoted_palette_pixel_type color_2 =
                        color_1.template convert<typename palette_repr_type::promoted_type>();
                    color_2 = opacity * color_2;
                    if (blending)
                        color_2 = color_2 + palette_colors[index_2];
                    test::check_color_index(test_context, writer, index_1, color_2);
                }
                else {
                    ARCHON_CHECK_EQUAL(index_1, index_2);
                }
            }
        }
    };

    auto test_2 = [&](check::TestContext& parent_test_context, const auto& origin_block,
                      image::float_type opacity, bool blending) {
        ARCHON_TEST_TRAIL(parent_test_context, core::formatted("%s, %s", opacity, blending));

        image::Box box = { image::Pos() - margin, max_origin_size };
        test::for_each_box_in(box, [&](const image::Box& subbox) {
            test_1(test_context, origin_block, opacity, blending, subbox);
        });
    };

    auto test_3 = [&](check::TestContext& parent_test_context, auto origin_repr, const char* descr) {
        ARCHON_TEST_TRAIL(parent_test_context, descr);

        using origin_repr_type = decltype(origin_repr);
        using origin_block_type = image::PixelBlock<origin_repr_type>;
        origin_block_type origin_block(max_origin_size);
        core::Span origin_buffer = origin_block.buffer();
        constexpr image::CompRepr origin_comp_repr = origin_repr_type::comp_repr;
        for (std::size_t i = 0; i < origin_buffer.size(); ++i)
            origin_buffer[i] = test::rand_comp<origin_comp_repr>(random);

        test_2(test_context, origin_block, 1.0, false);
        test_2(test_context, origin_block, 0.5, false);
        test_2(test_context, origin_block, 0.0, false);
        test_2(test_context, origin_block, 1.0, true);
        test_2(test_context, origin_block, 0.5, true);
        test_2(test_context, origin_block, 0.0, true);
    };

    test_3(test_context, image::Lum_8(), "Lum_8");
    test_3(test_context, image::Lum_F(), "Lum_F");
    test_3(test_context, image::LumA_8(), "LumA_8");
    test_3(test_context, image::LumA_F(), "LumA_F");
    test_3(test_context, image::RGB_8(), "RGB_8");
    test_3(test_context, image::RGB_F(), "RGB_F");
    test_3(test_context, image::RGBA_8(), "RGBA_8");
    test_3(test_context, image::RGBA_F(), "RGBA_F");
}


ARCHON_TEST_BATCH(Image_Writer_PutImage_Lossless, pixel_repr_variants)
{
    // FIXME: How to craft test to activate subdivision?                                           

    std::mt19937_64 random(test_context.seed_seq());

    image::Size destin_size = { 5, 5 };
    image::Size margin = 1;
    image::Size max_origin_size = 2 * margin + destin_size;
    using destin_repr_type = test_type;
    using destin_block_type = image::PixelBlock<destin_repr_type>;
    using destin_pixel_type = typename destin_block_type::pixel_type;
    destin_block_type destin_block_1(destin_size);
    destin_block_type destin_block_2(destin_size);
    core::Span destin_buffer_1 = destin_block_1.buffer();
    core::Span destin_buffer_2 = destin_block_2.buffer();
    image::WritableTrayImage destin(destin_block_1);
    constexpr image::CompRepr destin_comp_repr = destin_repr_type::comp_repr;
    for (std::size_t i = 0; i < destin_buffer_2.size(); ++i)
        destin_buffer_2[i] = test::rand_comp<destin_comp_repr>(random);

    auto test_1 = [&](check::TestContext& parent_test_context, const auto& origin_block, const image::Box& subbox) {
        ARCHON_TEST_TRAIL(parent_test_context, subbox);

        std::copy_n(destin_buffer_2.data(), destin_buffer_2.size(), destin_buffer_1.data());
        image::Writer writer(destin);
        image::TrayImage origin(origin_block, subbox.size);
        writer.put_image(subbox.pos, origin);

        for (int y = 0; y < destin_size.height; ++y) {
            for (int x = 0; x < destin_size.width; ++x) {
                image::Pos pos = { x, y };
                destin_pixel_type pixel_1 = destin_block_1.get_pixel(pos);
                if (image::Box(pos, 1).contained_in(subbox)) {
                    using origin_block_type = std::decay_t<decltype(origin_block)>;
                    using origin_pixel_type = typename origin_block_type::pixel_type;
                    origin_pixel_type pixel_2 = origin_block.get_pixel(image::Pos() + (pos - subbox.pos));
                    ARCHON_CHECK_EQUAL(pixel_1, destin_pixel_type(pixel_2));
                }
                else {
                    destin_pixel_type pixel_2 = destin_block_2.get_pixel(pos);
                    ARCHON_CHECK_EQUAL(pixel_1, pixel_2);
                }
            }
        }
    };

    auto test_2 = [&](check::TestContext& parent_test_context, auto origin_repr, const char* descr) {
        ARCHON_TEST_TRAIL(parent_test_context, descr);

        using origin_repr_type = decltype(origin_repr);
        using origin_block_type = image::PixelBlock<origin_repr_type>;
        origin_block_type origin_block(max_origin_size);
        core::Span origin_buffer = origin_block.buffer();
        constexpr image::CompRepr origin_comp_repr = origin_repr_type::comp_repr;
        for (std::size_t i = 0; i < origin_buffer.size(); ++i)
            origin_buffer[i] = test::rand_comp<origin_comp_repr>(random);

        image::Box box = { image::Pos() - margin, max_origin_size };
        test::for_each_box_in(box, [&](const image::Box& subbox) {
            test_1(test_context, origin_block, subbox);
        });
    };

    test_2(test_context, destin_repr_type(), "Same alpha");
    if constexpr (destin_repr_type::has_alpha) {
        using repr_type = image::PixelRepr<destin_repr_type::color_space_tag, false, destin_repr_type::comp_repr>;
        test_2(test_context, repr_type(), "Add alpha");
    }
}


ARCHON_TEST_BATCH(Image_Writer_PutImage_Falloff, pixel_repr_variants)
{
    using pixel_repr_type = test_type;
    using block_type = image::PixelBlock<pixel_repr_type>;
    using pixel_type = typename block_type::pixel_type;
    pixel_type background_color = pixel_type(util::colors::transparent);

    auto test = [&](check::TestContext& parent_test_context, image::Size image_size) {
        ARCHON_TEST_TRAIL(parent_test_context, image_size);
        block_type image_block(image_size);
        auto image_buffer = image_block.buffer();
        double frac = 0;
        for (std::size_t i = 0; i < image_buffer.size(); ++i) {
            constexpr auto comp_repr = pixel_repr_type::comp_repr;
            using comp_type = image::comp_type<comp_repr>;
            if constexpr (std::is_integral_v<comp_type>) {
                constexpr int bit_width = image::comp_repr_bit_width<comp_repr>();
                image_buffer[i] = image::float_to_int<comp_type, bit_width>(frac);
            }
            else {
                static_assert(std::is_floating_point_v<comp_type>);
                image_buffer[i] = comp_type(frac);
            }
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
            image::WritableTrayImage target(block, box.size);
            image::Writer writer(target);
            writer.put_image_a({ 0, 0 }, reader, box);
            for (int y = 0; y < box.size.height; ++y) {
                for (int x = 0; x < box.size.width; ++x) {
                    pixel_type pixel_1 = block.get_pixel({ x, y });
                    image::Pos pos = box.pos + image::Size(x, y);
                    pixel_type pixel_2 = get_expected_pixel(pos, horz_mode, vert_mode);
                    ARCHON_CHECK_EQUAL(pixel_1, pixel_2);
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
