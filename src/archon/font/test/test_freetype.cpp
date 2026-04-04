// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2026 Kristian Spangsege <kristian.spangsege@gmail.com>
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
#include <memory>
#include <stdexcept>
#include <string_view>
#include <filesystem>

#include <archon/core/features.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/math.hpp>
#include <archon/core/char_mapper.hpp>
#include <archon/check.hpp>
#include <archon/math/matrix.hpp>
#include <archon/util/colors.hpp>
#include <archon/image.hpp>
#include <archon/font/size.hpp>
#include <archon/font/face.hpp>
#include <archon/font/loader.hpp>
#include <archon/font/freetype_implementation.hpp>


using namespace archon;


namespace {


auto make_rotation(font::face::float_type angle) noexcept -> font::face::matrix_type
{
    return math::rot(font::face::float_type(core::deg_to_rad(angle)));
}


void rgba_to_premult_rgba(int& r, int& g, int& b, int a) noexcept
{
    // This emulates how FreeType does "destructive" alpha pre-multiplication
    // internally
    r = core::int_div_round_half_up(r * a, 255);
    g = core::int_div_round_half_up(g * a, 255);
    b = core::int_div_round_half_up(b * a, 255);
}


int premult_rgba_to_alpha(int r, int g, int b, int a) noexcept
{
    // Precise imitation of the integer-arithmetic-only conversion from BGRA to coverage
    // value as it happens inside FreeType 2.14.2 (in ft_gray_for_premultiplied_srgb_bgra()
    // in src/base/ftbitmap.c).

    if (ARCHON_LIKELY(a == 0))
        return 0;
    using uint32_type = std::uint_fast32_t;
    uint32_type r_2 = uint32_type(r) * r;
    uint32_type g_2 = uint32_type(g) * g;
    uint32_type b_2 = uint32_type(b) * b;
    uint32_type l = (13937 * r_2 + 46868 * g_2 + 4731 * b_2) / 65536;
    return int(a - l / a);
}


int rgba_to_premult_rgba_to_alpha(int r, int g, int b, int a) noexcept
{
    int r_2 = r, g_2 = g, b_2 = b;
    ::rgba_to_premult_rgba(r_2, g_2, b_2, a);
    return ::premult_rgba_to_alpha(r_2, g_2, b_2, a);
}


void rgba_to_premult_rgba_to_rgba(int& r, int& g, int& b, int a) noexcept
{
    int r_2 = r, g_2 = g, b_2 = b;
    ::rgba_to_premult_rgba(r_2, g_2, b_2, a);

    // This emulates how the FreeType-based implementation undoes alpha
    // pre-multiplication
    auto unpremultiply = [&](int val) noexcept -> int {
        if (ARCHON_LIKELY(a == 0))
            return 0;
        return int(core::int_div_round_half_up(unsigned(val) * 255, a));
    };
    r = unpremultiply(r_2);
    g = unpremultiply(g_2);
    b = unpremultiply(b_2);
}


void rgba_to_premult_rgba_to_alpha_to_rgba(int& r, int& g, int& b, int& a)
{
    int a_2 = ::rgba_to_premult_rgba_to_alpha(r, g, b, a);
    r = 0;
    g = 0;
    b = 0;
    a = a_2;
}


auto get_expected_mask(int r, int g, int b, std::string_view ink) -> image::PixelBlock_Alpha_8
{
    image::Size size;
    {
        std::size_t width = 0;
        std::size_t height = 0;
        std::size_t i = 0;
        for (;;) {
            height += 1;
            std::size_t j = ink.find('\n', i);
            if (j == ink.npos)
                j = ink.size();
            std::size_t w = std::size_t(j - i);
            if (height == 1) {
                width = w;
            }
            else if (w != width) {
                throw std::runtime_error("Bad ink");
            }
            if (j == ink.size())
                break;
            i = std::size_t(j + 1);
        }
        if (width > 0) {
            core::int_cast(width, size.width);
            core::int_cast(height, size.height);
        }
    }

    using block_type = image::PixelBlock_Alpha_8;
    block_type block(size); // Throws
    std::size_t offset = 0;
    int y = 0;
    for (;;) {
        ARCHON_ASSERT(std::size_t(ink.size() - offset) >= std::size_t(size.width));
        for (int x = 0; x < size.width; ++x) {
            char ch = ink[offset + x];
            if (ch == ' ')
                continue;
            if (ARCHON_LIKELY(ch >= '1' && ch <= '7')) {
                int a = image::int_to_int<3, int, 8>(ch - '0');
                int a_2 = ::rgba_to_premult_rgba_to_alpha(r, g, b, a);
                block_type::comp_type* pixel = block.get({ x, y });
                pixel[0] = image::comp_repr_pack<image::CompRepr::int8>(a_2);
                continue;
            }
            throw std::runtime_error("Bad ink");
        }
        offset += size.width;
        y += 1;
        if (offset == ink.size()) {
            ARCHON_ASSERT(y == size.height);
            break;
        }
        ARCHON_ASSERT(ink[offset] == '\n');
        offset += 1;
    }
    return block;
}


auto get_expected_rgba(int r, int g, int b, std::string_view ink, bool to_alpha) -> image::PixelBlock_RGBA_8
{
    image::Size size;
    {
        std::size_t width = 0;
        std::size_t height = 0;
        std::size_t i = 0;
        for (;;) {
            height += 1;
            std::size_t j = ink.find('\n', i);
            if (j == ink.npos)
                j = ink.size();
            std::size_t w = std::size_t(j - i);
            if (height == 1) {
                width = w;
            }
            else if (w != width) {
                throw std::runtime_error("Bad ink");
            }
            if (j == ink.size())
                break;
            i = std::size_t(j + 1);
        }
        if (width > 0) {
            core::int_cast(width, size.width);
            core::int_cast(height, size.height);
        }
    }

    using block_type = image::PixelBlock_RGBA_8;
    block_type block(size); // Throws
    std::size_t offset = 0;
    int y = 0;
    for (;;) {
        ARCHON_ASSERT(std::size_t(ink.size() - offset) >= std::size_t(size.width));
        for (int x = 0; x < size.width; ++x) {
            char ch = ink[offset + x];
            if (ch == ' ')
                continue;
            if (ARCHON_LIKELY(ch >= '1' && ch <= '7')) {
                int a = image::int_to_int<3, int, 8>(ch - '0');
                int r_2 = r, g_2 = g, b_2 = b, a_2 = a;
                if (!to_alpha) {
                    rgba_to_premult_rgba_to_rgba(r_2, g_2, b_2, a_2);
                }
                else {
                    rgba_to_premult_rgba_to_alpha_to_rgba(r_2, g_2, b_2, a_2);
                }
                block_type::comp_type* pixel = block.get({ x, y });
                pixel[0] = image::comp_repr_pack<image::CompRepr::int8>(r_2);
                pixel[1] = image::comp_repr_pack<image::CompRepr::int8>(g_2);
                pixel[2] = image::comp_repr_pack<image::CompRepr::int8>(b_2);
                pixel[3] = image::comp_repr_pack<image::CompRepr::int8>(a_2);
                continue;
            }
            throw std::runtime_error("Bad ink");
        }
        offset += size.width;
        y += 1;
        if (offset == ink.size()) {
            ARCHON_ASSERT(y == size.height);
            break;
        }
        ARCHON_ASSERT(ink[offset] == '\n');
        offset += 1;
    }
    return block;
}


template<class F> auto make_expected_rgba(image::Size size, F&& func, bool to_alpha) -> image::PixelBlock_RGBA_8
{
    image::PixelBlock_RGBA_8 block(size); // Throws
    {
        image::WritableTrayImage image(block);
        image::Writer writer(image); // Throws
        func(writer); // Throws
    }
    if (to_alpha) {
        for (int y = 0; y < size.height; ++y) {
            for (int x = 0; x < size.width; ++x) {
                image::Pixel_RGBA_8 pixel = block.get_pixel({ x, y });
                int r = int(pixel.get_comp_value(0));
                int g = int(pixel.get_comp_value(1));
                int b = int(pixel.get_comp_value(2));
                int a = int(pixel.get_comp_value(3));
                ::rgba_to_premult_rgba_to_alpha_to_rgba(r, g, b, a);
                using unpacked_comp_type = image::Pixel_RGBA_8::unpacked_comp_type;
                pixel.set_comp_value(0, unpacked_comp_type(r));
                pixel.set_comp_value(1, unpacked_comp_type(g));
                pixel.set_comp_value(2, unpacked_comp_type(b));
                pixel.set_comp_value(3, unpacked_comp_type(a));
                block.set_pixel({ x, y }, pixel);
            }
        }
    }
    return block;
}


constexpr std::string_view g_test_dir_path = "archon/font/test";


bool is_freetype_available() noexcept
{
    return font::get_freetype_implementation().is_available();
}


} // unnamed namespace


ARCHON_TEST_IF(Font_Freetype_ScalableUncolored, is_freetype_available())
{
    namespace fs = std::filesystem;
    fs::path file = test_context.get_data_path(g_test_dir_path, "test_font_scalable_uncolored.ttf");
    font::loader::config config;
    config.logger = &test_context.logger;
    std::unique_ptr<font::loader> loader =
        font::new_freetype_loader_from_font_file(file, test_context.locale, config); // Throws
    ARCHON_CHECK_EQUAL(loader->get_num_faces(), 1);
    std::unique_ptr<font::face> face = loader->load_default_face();
    ARCHON_CHECK_EQUAL(face->get_family_name(), "Test");
    ARCHON_CHECK_NOT(face->is_bold());
    ARCHON_CHECK_NOT(face->is_italic());
    ARCHON_CHECK_NOT(face->is_monospace());
    ARCHON_CHECK(face->is_scalable());
    ARCHON_CHECK_NOT(face->has_color());
    ARCHON_CHECK_EQUAL(face->get_num_fixed_sizes(), 0);
    ARCHON_CHECK_EQUAL(face->get_size(), font::size(16));
    face->set_scaled_size(1024);
    ARCHON_CHECK_EQUAL(face->get_size(), font::size(1024));
    ARCHON_CHECK_EQUAL(face->get_ascender(), 800);
    ARCHON_CHECK_EQUAL(face->get_descender(), -200);
    ARCHON_CHECK_EQUAL(face->get_baseline_spacing(), 1024);

    bool grid_fitting = false;
    core::WideCharMapper mapper(test_context.locale);
    using vector_type = font::face::vector_type;

    // Blank glyph
    {
        std::size_t i = face->find_glyph(mapper.widen(' '));
        ARCHON_CHECK_NOT_EQUAL(i, 0);
        ARCHON_CHECK(face->try_load_glyph(i, grid_fitting));
        ARCHON_CHECK_EQUAL(face->get_glyph_advance(), 256);
        ARCHON_CHECK_EQUAL(face->get_glyph_bearing(), vector_type(0, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_pos(), vector_type(0, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_size(), vector_type(0, 0));
        ARCHON_CHECK_NOT(face->glyph_may_be_colored());
        ARCHON_CHECK_EQUAL(face->get_target_glyph_box(), image::Box({ 0, 0 }, 0));
    }

    // Fallback glyph
    {
        std::size_t i = face->find_glyph(mapper.widen('@'));
        ARCHON_CHECK_EQUAL(i, 0);
        ARCHON_CHECK(face->try_load_glyph(i, grid_fitting));
        ARCHON_CHECK_EQUAL(face->get_glyph_advance(), 600);
        ARCHON_CHECK_EQUAL(face->get_glyph_bearing(), vector_type(0, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_pos(), vector_type(50, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_size(), vector_type(500, 700));
        ARCHON_CHECK_NOT(face->glyph_may_be_colored());
        ARCHON_CHECK_EQUAL(face->get_target_glyph_box(), image::Box({ 50, -700 }, { 500, 700 }));
    }

    // "A"
    {
        std::size_t i = face->find_glyph(mapper.widen('A'));
        ARCHON_CHECK_NOT_EQUAL(i, 0);
        ARCHON_CHECK(face->try_load_glyph(i, grid_fitting));
        ARCHON_CHECK_EQUAL(face->get_glyph_advance(), 948);
        ARCHON_CHECK_EQUAL(face->get_glyph_bearing(), vector_type(0, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_pos(), vector_type(-462, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_size(), vector_type(924, 800));
        ARCHON_CHECK_NOT(face->glyph_may_be_colored());
        ARCHON_CHECK_EQUAL(face->get_target_glyph_box(), image::Box({ -462, -800 }, { 924, 800 }));
    }

    // "A" translated
    {
        std::size_t i = face->find_glyph(mapper.widen('A'));
        ARCHON_CHECK(face->try_load_glyph(i, grid_fitting));
        face->set_translat({ -200, -100 });
        ARCHON_CHECK_EQUAL(face->get_glyph_advance(), 948);
        ARCHON_CHECK_EQUAL(face->get_glyph_bearing(), vector_type(0, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_pos(), vector_type(-462, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_size(), vector_type(924, 800));
        ARCHON_CHECK_NOT(face->glyph_may_be_colored());
        ARCHON_CHECK_EQUAL(face->get_target_glyph_box(), image::Box({ -662, -700 }, { 924, 800 }));
        face->set_target_pos({ 700, 500 });
        ARCHON_CHECK_EQUAL(face->get_target_glyph_box(), image::Box({ 38, -200 }, { 924, 800 }));
        face->set_translat({ 0, 0 });
        face->set_target_pos({ 0, 0 });
    }

    using block_type = image::PixelBlock_Alpha_8;
    block_type block;

    // "B" (rising coverage gradient)
    {
        std::size_t i = face->find_glyph(mapper.widen('B'));
        ARCHON_CHECK_NOT_EQUAL(i, 0);
        ARCHON_CHECK(face->try_load_glyph(i, grid_fitting));
        ARCHON_CHECK_EQUAL(face->get_glyph_advance(), 512);
        ARCHON_CHECK_EQUAL(face->get_glyph_bearing(), vector_type(0, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_pos(), vector_type(128, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_size(), vector_type(256, 1));
        ARCHON_CHECK_NOT(face->glyph_may_be_colored());
        ARCHON_CHECK_EQUAL(face->get_target_glyph_box(), image::Box({ 128, -1 }, { 256, 1 }));
        block = block_type(image::Size(256, 1));
        face->render_glyph_mask_a(image::Pos(128, -1), block.tray());
        for (int i = 0; i < 256; ++i) {
            int x = i;
            image::Pixel_Alpha_8 pixel = block.get_pixel({ x, 0 });
            auto value = pixel.get_comp_value(0);
            ARCHON_CHECK_DIST_LESS_EQUAL(value, i, 1);
        }
    }

    // "C" (rising coverage gradient)
    {
        std::size_t i = face->find_glyph(mapper.widen('C'));
        ARCHON_CHECK_NOT_EQUAL(i, 0);
        ARCHON_CHECK(face->try_load_glyph(i, grid_fitting));
        ARCHON_CHECK_EQUAL(face->get_glyph_advance(), 512);
        ARCHON_CHECK_EQUAL(face->get_glyph_bearing(), vector_type(0, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_pos(), vector_type(128, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_size(), vector_type(256, 1));
        ARCHON_CHECK_NOT(face->glyph_may_be_colored());
        ARCHON_CHECK_EQUAL(face->get_target_glyph_box(), image::Box({ 128, -1 }, { 256, 1 }));
        block = block_type(image::Size(256, 1));
        face->render_glyph_mask_a(image::Pos(128, -1), block.tray());
        for (int i = 0; i < 256; ++i) {
            int x = i;
            image::Pixel_Alpha_8 pixel = block.get_pixel({ x, 0 });
            auto value = pixel.get_comp_value(0);
            ARCHON_CHECK_DIST_LESS_EQUAL(value, 255 - i, 1);
        }
    }
}


ARCHON_TEST_IF(Font_Freetype_ScalableColored, is_freetype_available())
{
    namespace fs = std::filesystem;
    fs::path file = test_context.get_data_path(g_test_dir_path, "test_font_scalable_colored.ttf");
    font::loader::config config;
    config.logger = &test_context.logger;
    std::unique_ptr<font::loader> loader =
        font::new_freetype_loader_from_font_file(file, test_context.locale, config); // Throws
    ARCHON_CHECK_EQUAL(loader->get_num_faces(), 1);
    std::unique_ptr<font::face> face = loader->load_default_face();
    ARCHON_CHECK_EQUAL(face->get_family_name(), "Test");
    ARCHON_CHECK_NOT(face->is_bold());
    ARCHON_CHECK_NOT(face->is_italic());
    ARCHON_CHECK_NOT(face->is_monospace());
    ARCHON_CHECK(face->is_scalable());
    ARCHON_CHECK(face->has_color());
    ARCHON_CHECK_EQUAL(face->get_num_fixed_sizes(), 0);
    ARCHON_CHECK_EQUAL(face->get_size(), font::size(16));
    face->set_scaled_size(1024);
    ARCHON_CHECK_EQUAL(face->get_size(), font::size(1024));
    ARCHON_CHECK_EQUAL(face->get_ascender(), 800);
    ARCHON_CHECK_EQUAL(face->get_descender(), -200);
    ARCHON_CHECK_EQUAL(face->get_baseline_spacing(), 1024);
    face->set_color_loading_enabled(true);

    bool grid_fitting = false;
    core::WideCharMapper mapper(test_context.locale);
    using vector_type = font::face::vector_type;

    // Blank glyph
    {
        std::size_t i = face->find_glyph(mapper.widen(' '));
        ARCHON_CHECK_NOT_EQUAL(i, 0);
        ARCHON_CHECK(face->try_load_glyph(i, grid_fitting));
        ARCHON_CHECK_EQUAL(face->get_glyph_advance(), 256);
        ARCHON_CHECK_EQUAL(face->get_glyph_bearing(), vector_type(0, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_pos(), vector_type(0, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_size(), vector_type(0, 0));
        ARCHON_CHECK_NOT(face->glyph_may_be_colored());
        ARCHON_CHECK_EQUAL(face->get_target_glyph_box(), image::Box({ 0, 0 }, 0));
    }

    // Fallback glyph
    {
        std::size_t i = face->find_glyph(mapper.widen('@'));
        ARCHON_CHECK_EQUAL(i, 0);
        ARCHON_CHECK(face->try_load_glyph(i, grid_fitting));
        ARCHON_CHECK_EQUAL(face->get_glyph_advance(), 600);
        ARCHON_CHECK_EQUAL(face->get_glyph_bearing(), vector_type(0, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_pos(), vector_type(50, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_size(), vector_type(500, 700));
        ARCHON_CHECK_NOT(face->glyph_may_be_colored());
        ARCHON_CHECK_EQUAL(face->get_target_glyph_box(), image::Box({ 50, -700 }, { 500, 700 }));
    }

    // "A"
    {
        std::size_t i = face->find_glyph(mapper.widen('A'));
        ARCHON_CHECK_NOT_EQUAL(i, 0);
        ARCHON_CHECK(face->try_load_glyph(i, grid_fitting));
        ARCHON_CHECK_EQUAL(face->get_glyph_advance(), 512);
        ARCHON_CHECK_EQUAL(face->get_glyph_bearing(), vector_type(0, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_pos(), vector_type(6, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_size(), vector_type(500, 500));
        ARCHON_CHECK(face->glyph_may_be_colored());
        image::Box box = {{ 6, -500 }, { 500, 500 }};
        ARCHON_CHECK_EQUAL(face->get_target_glyph_box(), box);
        image::PixelBlock_RGBA_8 block(box.size);
        face->render_glyph_rgba_a(box.pos, block.tray());
        image::PixelBlock_RGBA_8 expected = ::make_expected_rgba(box.size, [](image::Writer& writer) {
            writer.set_foreground_color(util::colors::red);
            writer.fill();
            writer.set_foreground_color(util::colors::blue);
            writer.fill(image::Box({ 100, 100 }, { 300, 300 }));
        }, false);
        ARCHON_CHECK(block == expected);
    }

    // "A" translated
    {
        std::size_t i = face->find_glyph(mapper.widen('A'));
        ARCHON_CHECK(face->try_load_glyph(i, grid_fitting));
        face->set_translat({ -200, -100 });
        ARCHON_CHECK_EQUAL(face->get_glyph_advance(), 512);
        ARCHON_CHECK_EQUAL(face->get_glyph_bearing(), vector_type(0, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_pos(), vector_type(6, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_size(), vector_type(500, 500));
        ARCHON_CHECK(face->glyph_may_be_colored());
        ARCHON_CHECK_EQUAL(face->get_target_glyph_box(), image::Box({ -194, -400 }, { 500, 500 }));
        face->set_target_pos({ 700, 500 });
        ARCHON_CHECK_EQUAL(face->get_target_glyph_box(), image::Box({ 506, 100 }, { 500, 500 }));
        face->set_translat({ 0, 0 });
        face->set_target_pos({ 0, 0 });
    }

    // "B"
    {
        std::size_t i = face->find_glyph(mapper.widen('B'));
        ARCHON_CHECK_NOT_EQUAL(i, 0);
        ARCHON_CHECK(face->try_load_glyph(i, grid_fitting));
        ARCHON_CHECK_EQUAL(face->get_glyph_advance(), 512);
        ARCHON_CHECK_EQUAL(face->get_glyph_bearing(), vector_type(0, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_pos(), vector_type(6, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_size(), vector_type(500, 500));
        ARCHON_CHECK(face->glyph_may_be_colored());
        image::Box box = {{ 6, -500 }, { 500, 500 }};
        ARCHON_CHECK_EQUAL(face->get_target_glyph_box(), box);
        image::PixelBlock_RGBA_8 block(box.size);
        face->render_glyph_rgba_a(box.pos, block.tray());
        image::PixelBlock_RGBA_8 expected = ::make_expected_rgba(box.size, [](image::Writer& writer) {
            writer.set_foreground_color(util::colors::lime);
            writer.fill(image::Box({ 0, 0 }, { 250, 500 }));
            writer.set_foreground_color(util::colors::yellow);
            writer.fill(image::Box({ 250, 0 }, { 250, 500 }));
        }, false);
        ARCHON_CHECK(block == expected);
    }
}


ARCHON_TEST_IF(Font_Freetype_BitmapMono, is_freetype_available())
{
    // FIXME: Find a way to verify that test_font_bitmap_gray.ttf ideed produces bitmap
    // glyphs with pixel mode "mono"

    namespace fs = std::filesystem;
    fs::path file = test_context.get_data_path(g_test_dir_path, "test_font_bitmap_mono.bdf");
    font::loader::config config;
    config.logger = &test_context.logger;
    std::unique_ptr<font::loader> loader =
        font::new_freetype_loader_from_font_file(file, test_context.locale, config); // Throws
    ARCHON_CHECK_EQUAL(loader->get_num_faces(), 1);
    std::unique_ptr<font::face> face = loader->load_default_face();
    ARCHON_CHECK_EQUAL(face->get_family_name(), "Test");
    ARCHON_CHECK_NOT(face->is_bold());
    ARCHON_CHECK_NOT(face->is_italic());
    ARCHON_CHECK_NOT(face->is_monospace());
    ARCHON_CHECK_NOT(face->is_scalable());
    ARCHON_CHECK_NOT(face->has_color());
    if (ARCHON_LIKELY(ARCHON_CHECK_EQUAL(face->get_num_fixed_sizes(), 1)))
        ARCHON_CHECK_EQUAL(face->get_fixed_size(0), font::size(16));
    ARCHON_CHECK_EQUAL(face->get_size(), font::size(16));
    ARCHON_CHECK_THROW_ANY(face->set_scaled_size(1024));
    face->set_approx_size(1024); // Ignored
    ARCHON_CHECK_EQUAL(face->get_size(), font::size(16));
    ARCHON_CHECK_EQUAL(face->get_ascender(), 12);
    ARCHON_CHECK_EQUAL(face->get_descender(), -4);
    ARCHON_CHECK_EQUAL(face->get_baseline_spacing(), 16);

    core::WideCharMapper mapper(test_context.locale);
    using vector_type = font::face::vector_type;

    // Blank glyph
    {
        std::size_t i = face->find_glyph(mapper.widen(' '));
        ARCHON_CHECK_NOT_EQUAL(i, 0);
        ARCHON_CHECK(face->try_load_glyph(i));
        ARCHON_CHECK_EQUAL(face->get_glyph_advance(), 6);
        ARCHON_CHECK_EQUAL(face->get_glyph_bearing(), vector_type(0, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_pos(), vector_type(0, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_size(), vector_type(0, 0));
        ARCHON_CHECK_NOT(face->glyph_may_be_colored());
        ARCHON_CHECK_EQUAL(face->get_target_glyph_box(), image::Box({ 0, 0 }, 0));
    }

    // Fallback glyph
    {
        std::size_t i = face->find_glyph(mapper.widen('@'));
        ARCHON_CHECK_EQUAL(i, 0);
        ARCHON_CHECK(face->try_load_glyph(i));
        ARCHON_CHECK_EQUAL(face->get_glyph_advance(), 12);
        ARCHON_CHECK_EQUAL(face->get_glyph_bearing(), vector_type(0, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_pos(), vector_type(2, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_size(), vector_type(8, 10));
        ARCHON_CHECK_NOT(face->glyph_may_be_colored());
        std::string_view ink =
            "77777777\n"
            "7      7\n"
            "7      7\n"
            "7      7\n"
            "7      7\n"
            "7      7\n"
            "7      7\n"
            "7      7\n"
            "7      7\n"
            "77777777";
        image::PixelBlock_Alpha_8 expected_1 = ::get_expected_mask(0, 0, 0, ink);
        image::PixelBlock_RGBA_8  expected_2 = ::get_expected_rgba(0, 0, 0, ink, false);
        image::Box box = {{ 2, -10 }, { 8, 10 }};
        ARCHON_CHECK_EQUAL(face->get_target_glyph_box(), box);
        image::PixelBlock_Alpha_8 block_1(box.size);
        face->render_glyph_mask_a(box.pos, block_1.tray());
        ARCHON_CHECK(block_1 == expected_1);
        image::PixelBlock_RGBA_8 block_2(box.size);
        face->render_glyph_rgba_a(box.pos, block_2.tray());
        ARCHON_CHECK(block_2 == expected_2);
    }

    // "A"
    {
        std::size_t i = face->find_glyph(mapper.widen('A'));
        ARCHON_CHECK_NOT_EQUAL(i, 0);
        ARCHON_CHECK(face->try_load_glyph(i));
        ARCHON_CHECK_EQUAL(face->get_glyph_advance(), 14);
        ARCHON_CHECK_EQUAL(face->get_glyph_bearing(), vector_type(0, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_pos(), vector_type(2, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_size(), vector_type(10, 10));
        ARCHON_CHECK_NOT(face->glyph_may_be_colored());
        std::string_view ink =
            "    77    \n"
            "   7  7   \n"
            "  7    7  \n"
            " 7      7 \n"
            "7        7\n"
            "7777777777\n"
            "7        7\n"
            "7        7\n"
            "7        7\n"
            "7        7";
        image::PixelBlock_Alpha_8 expected_1 = ::get_expected_mask(0, 0, 0, ink);
        image::PixelBlock_RGBA_8  expected_2 = ::get_expected_rgba(0, 0, 0, ink, false);
        image::Box box = {{ 2, -10 }, { 10, 10 }};
        ARCHON_CHECK_EQUAL(face->get_target_glyph_box(), box);
        image::PixelBlock_Alpha_8 block_1(box.size);
        face->render_glyph_mask_a(box.pos, block_1.tray());
        ARCHON_CHECK(block_1 == expected_1);
        image::PixelBlock_RGBA_8 block_2(box.size);
        face->render_glyph_rgba_a(box.pos, block_2.tray());
        ARCHON_CHECK(block_2 == expected_2);
    }

    // "B"
    {
        std::size_t i = face->find_glyph(mapper.widen('B'));
        ARCHON_CHECK_NOT_EQUAL(i, 0);
        ARCHON_CHECK(face->try_load_glyph(i));
        ARCHON_CHECK_EQUAL(face->get_glyph_advance(), 12);
        ARCHON_CHECK_EQUAL(face->get_glyph_bearing(), vector_type(0, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_pos(), vector_type(2, -2));
        ARCHON_CHECK_EQUAL(face->get_glyph_size(), vector_type(8, 13));
        ARCHON_CHECK_NOT(face->glyph_may_be_colored());
        std::string_view ink =
            "77777   \n"
            "7    77 \n"
            "7      7\n"
            "7      7\n"
            "7      7\n"
            "7    77 \n"
            "77777   \n"
            "7    77 \n"
            "7      7\n"
            "7      7\n"
            "7      7\n"
            "7    77 \n"
            "77777   ";
        image::PixelBlock_Alpha_8 expected_1 = ::get_expected_mask(0, 0, 0, ink);
        image::PixelBlock_RGBA_8  expected_2 = ::get_expected_rgba(0, 0, 0, ink, false);
        image::Box box = {{ 2, -11 }, { 8, 13 }};
        ARCHON_CHECK_EQUAL(face->get_target_glyph_box(), box);
        image::PixelBlock_Alpha_8 block_1(box.size);
        face->render_glyph_mask_a(box.pos, block_1.tray());
        ARCHON_CHECK(block_1 == expected_1);
        image::PixelBlock_RGBA_8 block_2(box.size);
        face->render_glyph_rgba_a(box.pos, block_2.tray());
        ARCHON_CHECK(block_2 == expected_2);

        // With changed position of target point
        face->set_target_pos({ -20, 12 });
        image::Box box_2 = box + image::Size(-20, 12);
        ARCHON_CHECK_EQUAL(face->get_target_glyph_box(), box_2);
        face->render_glyph_mask_a(box_2.pos, block_1.tray());
        ARCHON_CHECK(block_1 == expected_1);
        face->render_glyph_rgba_a(box_2.pos, block_2.tray());
        ARCHON_CHECK(block_2 == expected_2);

        // With transformation and translation
        face->set_transform(make_rotation(22.5)); // Must be ignored
        face->set_translat({ 15.7, 2.3 }); // Must be rounded to nearest integer
        image::Box box_3 = box_2 + image::Size(16, -2);
        ARCHON_CHECK_EQUAL(face->get_target_glyph_box(), box_3);
        face->render_glyph_mask_a(box_3.pos, block_1.tray());
        ARCHON_CHECK(block_1 == expected_1);
        face->render_glyph_rgba_a(box_3.pos, block_2.tray());
        ARCHON_CHECK(block_2 == expected_2);
    }
}


ARCHON_TEST_IF(Font_Freetype_BitmapGray, is_freetype_available())
{
    // FIXME: Find a way to verify that test_font_bitmap_gray.ttf ideed produces bitmap
    // glyphs with pixel mode "gray"

    namespace fs = std::filesystem;
    fs::path file = test_context.get_data_path(g_test_dir_path, "test_font_bitmap_gray.ttf");
    font::loader::config config;
    config.logger = &test_context.logger;
    std::unique_ptr<font::loader> loader =
        font::new_freetype_loader_from_font_file(file, test_context.locale, config); // Throws
    ARCHON_CHECK_EQUAL(loader->get_num_faces(), 1);
    std::unique_ptr<font::face> face = loader->load_default_face();
    ARCHON_CHECK_EQUAL(face->get_family_name(), "Test");
    ARCHON_CHECK_NOT(face->is_bold());
    ARCHON_CHECK_NOT(face->is_italic());
    ARCHON_CHECK_NOT(face->is_monospace());
    ARCHON_CHECK_NOT(face->is_scalable());
    ARCHON_CHECK_NOT(face->has_color());
    if (ARCHON_LIKELY(ARCHON_CHECK_EQUAL(face->get_num_fixed_sizes(), 1)))
        ARCHON_CHECK_EQUAL(face->get_fixed_size(0), font::size(16));
    ARCHON_CHECK_EQUAL(face->get_size(), font::size(16));
    ARCHON_CHECK_THROW_ANY(face->set_scaled_size(1024));
    face->set_approx_size(1024); // Ignored
    ARCHON_CHECK_EQUAL(face->get_size(), font::size(16));
    ARCHON_CHECK_EQUAL(face->get_ascender(), 12);
    ARCHON_CHECK_EQUAL(face->get_descender(), -4);
    ARCHON_CHECK_EQUAL(face->get_baseline_spacing(), 16);

    core::WideCharMapper mapper(test_context.locale);
    using vector_type = font::face::vector_type;

    // Blank glyph
    {
        std::size_t i = face->find_glyph(mapper.widen(' '));
        ARCHON_CHECK_NOT_EQUAL(i, 0);
        ARCHON_CHECK(face->try_load_glyph(i));
        ARCHON_CHECK_EQUAL(face->get_glyph_advance(), 6);
        ARCHON_CHECK_EQUAL(face->get_glyph_bearing(), vector_type(0, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_pos(), vector_type(0, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_size(), vector_type(0, 0));
        ARCHON_CHECK_NOT(face->glyph_may_be_colored());
        ARCHON_CHECK_EQUAL(face->get_target_glyph_box(), image::Box({ 0, 0 }, 0));
    }

    // Fallback glyph
    {
        std::size_t i = face->find_glyph(mapper.widen('@'));
        ARCHON_CHECK_EQUAL(i, 0);
        ARCHON_CHECK(face->try_load_glyph(i));
        ARCHON_CHECK_EQUAL(face->get_glyph_advance(), 12);
        ARCHON_CHECK_EQUAL(face->get_glyph_bearing(), vector_type(0, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_pos(), vector_type(2, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_size(), vector_type(8, 10));
        ARCHON_CHECK_NOT(face->glyph_may_be_colored());
        std::string_view ink =
            "77777777\n"
            "7      7\n"
            "7      7\n"
            "7      7\n"
            "7      7\n"
            "7      7\n"
            "7      7\n"
            "7      7\n"
            "7      7\n"
            "77777777";
        image::PixelBlock_Alpha_8 expected_1 = ::get_expected_mask(0, 0, 0, ink);
        image::PixelBlock_RGBA_8  expected_2 = ::get_expected_rgba(0, 0, 0, ink, false);
        image::Box box = {{ 2, -10 }, { 8, 10 }};
        ARCHON_CHECK_EQUAL(face->get_target_glyph_box(), box);
        image::PixelBlock_Alpha_8 block_1(box.size);
        face->render_glyph_mask_a(box.pos, block_1.tray());
        ARCHON_CHECK(block_1 == expected_1);
        image::PixelBlock_RGBA_8 block_2(box.size);
        face->render_glyph_rgba_a(box.pos, block_2.tray());
        ARCHON_CHECK(block_2 == expected_2);
    }

    // "A"
    {
        std::size_t i = face->find_glyph(mapper.widen('A'));
        ARCHON_CHECK_NOT_EQUAL(i, 0);
        ARCHON_CHECK(face->try_load_glyph(i));
        ARCHON_CHECK_EQUAL(face->get_glyph_advance(), 14);
        ARCHON_CHECK_EQUAL(face->get_glyph_bearing(), vector_type(0, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_pos(), vector_type(2, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_size(), vector_type(10, 10));
        ARCHON_CHECK_NOT(face->glyph_may_be_colored());
        std::string_view ink =
            "    55    \n"
            "   7227   \n"
            "  7    7  \n"
            " 7      7 \n"
            "61      16\n"
            "7777777777\n"
            "7        7\n"
            "7        7\n"
            "7        7\n"
            "7        7";
        image::PixelBlock_Alpha_8 expected_1 = ::get_expected_mask(0, 0, 0, ink);
        image::PixelBlock_RGBA_8  expected_2 = ::get_expected_rgba(0, 0, 0, ink, false);
        image::Box box = {{ 2, -10 }, { 10, 10 }};
        ARCHON_CHECK_EQUAL(face->get_target_glyph_box(), box);
        image::PixelBlock_Alpha_8 block_1(box.size);
        face->render_glyph_mask_a(box.pos, block_1.tray());
        ARCHON_CHECK(block_1 == expected_1);
        image::PixelBlock_RGBA_8 block_2(box.size);
        face->render_glyph_rgba_a(box.pos, block_2.tray());
        ARCHON_CHECK(block_2 == expected_2);
    }

    // "B"
    {
        std::size_t i = face->find_glyph(mapper.widen('B'));
        ARCHON_CHECK_NOT_EQUAL(i, 0);
        ARCHON_CHECK(face->try_load_glyph(i));
        ARCHON_CHECK_EQUAL(face->get_glyph_advance(), 12);
        ARCHON_CHECK_EQUAL(face->get_glyph_bearing(), vector_type(0, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_pos(), vector_type(2, -2));
        ARCHON_CHECK_EQUAL(face->get_glyph_size(), vector_type(8, 13));
        ARCHON_CHECK_NOT(face->glyph_may_be_colored());
        std::string_view ink =
            "777764  \n"
            "7    47 \n"
            "7     27\n"
            "7      7\n"
            "7     27\n"
            "7    47 \n"
            "777764  \n"
            "7    47 \n"
            "7     27\n"
            "7      7\n"
            "7     27\n"
            "7    47 \n"
            "777764  ";
        image::PixelBlock_Alpha_8 expected_1 = ::get_expected_mask(0, 0, 0, ink);
        image::PixelBlock_RGBA_8  expected_2 = ::get_expected_rgba(0, 0, 0, ink, false);
        image::Box box = {{ 2, -11 }, { 8, 13 }};
        ARCHON_CHECK_EQUAL(face->get_target_glyph_box(), box);
        image::PixelBlock_Alpha_8 block_1(box.size);
        face->render_glyph_mask_a(box.pos, block_1.tray());
        ARCHON_CHECK(block_1 == expected_1);
        image::PixelBlock_RGBA_8 block_2(box.size);
        face->render_glyph_rgba_a(box.pos, block_2.tray());
        ARCHON_CHECK(block_2 == expected_2);

        // With changed position of target point
        face->set_target_pos({ -20, 12 });
        image::Box box_2 = box + image::Size(-20, 12);
        ARCHON_CHECK_EQUAL(face->get_target_glyph_box(), box_2);
        face->render_glyph_mask_a(box_2.pos, block_1.tray());
        ARCHON_CHECK(block_1 == expected_1);
        face->render_glyph_rgba_a(box_2.pos, block_2.tray());
        ARCHON_CHECK(block_2 == expected_2);

        // With transformation and translation
        face->set_transform(make_rotation(22.5)); // Must be ignored
        face->set_translat({ 15.7, 2.3 }); // Must be rounded to nearest integer
        image::Box box_3 = box_2 + image::Size(16, -2);
        ARCHON_CHECK_EQUAL(face->get_target_glyph_box(), box_3);
        face->render_glyph_mask_a(box_3.pos, block_1.tray());
        ARCHON_CHECK(block_1 == expected_1);
        face->render_glyph_rgba_a(box_3.pos, block_2.tray());
        ARCHON_CHECK(block_2 == expected_2);
    }
}


ARCHON_TEST_IF(Font_Freetype_BitmapColor, is_freetype_available())
{
    // FIXME: Find a way to verify that test_font_bitmap_color.ttf ideed produces bitmap
    // glyphs with pixel mode "bgra"

    namespace fs = std::filesystem;
    fs::path file = test_context.get_data_path(g_test_dir_path, "test_font_bitmap_color.ttf");
    font::loader::config config;
    config.logger = &test_context.logger;
    std::unique_ptr<font::loader> loader =
        font::new_freetype_loader_from_font_file(file, test_context.locale, config); // Throws
    ARCHON_CHECK_EQUAL(loader->get_num_faces(), 1);
    std::unique_ptr<font::face> face = loader->load_default_face();
    ARCHON_CHECK_EQUAL(face->get_family_name(), "Test");
    ARCHON_CHECK_NOT(face->is_bold());
    ARCHON_CHECK_NOT(face->is_italic());
    ARCHON_CHECK_NOT(face->is_monospace());
    ARCHON_CHECK_NOT(face->is_scalable());
    ARCHON_CHECK(face->has_color());
    if (ARCHON_LIKELY(ARCHON_CHECK_EQUAL(face->get_num_fixed_sizes(), 1)))
        ARCHON_CHECK_EQUAL(face->get_fixed_size(0), font::size(16));
    ARCHON_CHECK_EQUAL(face->get_size(), font::size(16));
    ARCHON_CHECK_THROW_ANY(face->set_scaled_size(1024));
    face->set_approx_size(1024); // Ignored
    ARCHON_CHECK_EQUAL(face->get_size(), font::size(16));
    ARCHON_CHECK_EQUAL(face->get_ascender(), 12);
    ARCHON_CHECK_EQUAL(face->get_descender(), -4);
    ARCHON_CHECK_EQUAL(face->get_baseline_spacing(), 16);
    face->set_color_loading_enabled(true);

    core::WideCharMapper mapper(test_context.locale);
    using vector_type = font::face::vector_type;

    // Blank glyph
    {
        std::size_t i = face->find_glyph(mapper.widen(' '));
        ARCHON_CHECK_NOT_EQUAL(i, 0);
        ARCHON_CHECK(face->try_load_glyph(i));
        ARCHON_CHECK_EQUAL(face->get_glyph_advance(), 6);
        ARCHON_CHECK_EQUAL(face->get_glyph_bearing(), vector_type(0, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_pos(), vector_type(0, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_size(), vector_type(0, 0));
        ARCHON_CHECK(face->glyph_may_be_colored());
        ARCHON_CHECK_EQUAL(face->get_target_glyph_box(), image::Box({ 0, 0 }, 0));
    }

    // Fallback glyph
    {
        std::size_t i = face->find_glyph(mapper.widen('@'));
        ARCHON_CHECK_EQUAL(i, 0);
        ARCHON_CHECK(face->try_load_glyph(i));
        ARCHON_CHECK_EQUAL(face->get_glyph_advance(), 12);
        ARCHON_CHECK_EQUAL(face->get_glyph_bearing(), vector_type(0, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_pos(), vector_type(2, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_size(), vector_type(8, 10));
        ARCHON_CHECK(face->glyph_may_be_colored());
        std::string_view ink =
            "77777777\n"
            "7      7\n"
            "7      7\n"
            "7      7\n"
            "7      7\n"
            "7      7\n"
            "7      7\n"
            "7      7\n"
            "7      7\n"
            "77777777";
        image::PixelBlock_Alpha_8 expected_1 = ::get_expected_mask(150, 150, 150, ink);
        image::PixelBlock_RGBA_8  expected_2 = ::get_expected_rgba(150, 150, 150, ink, false);
        image::Box box = {{ 2, -10 }, { 8, 10 }};
        ARCHON_CHECK_EQUAL(face->get_target_glyph_box(), box);
        image::PixelBlock_Alpha_8 block_1(box.size);
        face->render_glyph_mask_a(box.pos, block_1.tray());
        ARCHON_CHECK(block_1 == expected_1);
        image::PixelBlock_RGBA_8 block_2(box.size);
        face->render_glyph_rgba_a(box.pos, block_2.tray());
        ARCHON_CHECK(block_2 == expected_2);
    }

    // "A"
    {
        std::size_t i = face->find_glyph(mapper.widen('A'));
        ARCHON_CHECK_NOT_EQUAL(i, 0);
        ARCHON_CHECK(face->try_load_glyph(i));
        ARCHON_CHECK_EQUAL(face->get_glyph_advance(), 14);
        ARCHON_CHECK_EQUAL(face->get_glyph_bearing(), vector_type(0, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_pos(), vector_type(2, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_size(), vector_type(10, 10));
        ARCHON_CHECK(face->glyph_may_be_colored());
        std::string_view ink =
            "    55    \n"
            "   7227   \n"
            "  7    7  \n"
            " 7      7 \n"
            "61      16\n"
            "7777777777\n"
            "7        7\n"
            "7        7\n"
            "7        7\n"
            "7        7";
        image::PixelBlock_RGBA_8 expected = ::get_expected_rgba(0, 150, 255, ink, false);
        image::Box box = {{ 2, -10 }, { 10, 10 }};
        ARCHON_CHECK_EQUAL(face->get_target_glyph_box(), box);
        image::PixelBlock_RGBA_8 block(box.size);
        face->render_glyph_rgba_a(box.pos, block.tray());
        ARCHON_CHECK(block == expected);
    }

    // "B"
    {
        std::size_t i = face->find_glyph(mapper.widen('B'));
        ARCHON_CHECK_NOT_EQUAL(i, 0);
        ARCHON_CHECK(face->try_load_glyph(i));
        ARCHON_CHECK_EQUAL(face->get_glyph_advance(), 12);
        ARCHON_CHECK_EQUAL(face->get_glyph_bearing(), vector_type(0, 0));
        ARCHON_CHECK_EQUAL(face->get_glyph_pos(), vector_type(2, -2));
        ARCHON_CHECK_EQUAL(face->get_glyph_size(), vector_type(8, 13));
        ARCHON_CHECK(face->glyph_may_be_colored());
        std::string_view ink =
            "777764  \n"
            "7    47 \n"
            "7     27\n"
            "7      7\n"
            "7     27\n"
            "7    47 \n"
            "777764  \n"
            "7    47 \n"
            "7     27\n"
            "7      7\n"
            "7     27\n"
            "7    47 \n"
            "777764  ";
        image::PixelBlock_RGBA_8 expected = ::get_expected_rgba(255, 100, 0, ink, false);
        image::Box box = {{ 2, -11 }, { 8, 13 }};
        ARCHON_CHECK_EQUAL(face->get_target_glyph_box(), box);
        image::PixelBlock_RGBA_8 block(box.size);
        face->render_glyph_rgba_a(box.pos, block.tray());
        ARCHON_CHECK(block == expected);

        // With changed position of target point
        face->set_target_pos({ -20, 12 });
        image::Box box_2 = box + image::Size(-20, 12);
        ARCHON_CHECK_EQUAL(face->get_target_glyph_box(), box_2);
        face->render_glyph_rgba_a(box_2.pos, block.tray());
        ARCHON_CHECK(block == expected);

        // With transformation and translation
        face->set_transform(make_rotation(22.5)); // Must be ignored
        face->set_translat({ 15.7, 2.3 }); // Must be rounded to nearest integer
        image::Box box_3 = box_2 + image::Size(16, -2);
        ARCHON_CHECK_EQUAL(face->get_target_glyph_box(), box_3);
        face->render_glyph_rgba_a(box_3.pos, block.tray());
        ARCHON_CHECK(block == expected);
    }
}
