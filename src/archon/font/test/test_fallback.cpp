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
#include <algorithm>
#include <memory>
#include <string_view>
#include <filesystem>

#include <archon/core/features.hpp>
#include <archon/core/pair.hpp>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/buffer.hpp>
#include <archon/check.hpp>
#include <archon/image.hpp>
#include <archon/font/size.hpp>
#include <archon/font/code_point.hpp>
#include <archon/font/face.hpp>
#include <archon/font/loader.hpp>
#include <archon/font/fallback_implementation.hpp>
#include <archon/font/freetype_implementation.hpp>


using namespace archon;


namespace {


constexpr std::string_view g_test_dir_path = "archon/font/test";


enum class font_spec { sans, mono };

ARCHON_TEST_VARIANTS(font_size_variants,
                     ARCHON_TEST_VALUE(core::Pair(font_spec::sans, font::size(14)),     Sans14),
                     ARCHON_TEST_VALUE(core::Pair(font_spec::sans, font::size(16)),     Sans16),
                     ARCHON_TEST_VALUE(core::Pair(font_spec::sans, font::size(15, 17)), Sans15x17),
                     ARCHON_TEST_VALUE(core::Pair(font_spec::sans, font::size(17, 15)), Sans17x15),
                     ARCHON_TEST_VALUE(core::Pair(font_spec::mono, font::size(12)),     Mono12),
                     ARCHON_TEST_VALUE(core::Pair(font_spec::mono, font::size(14)),     Mono14),
                     ARCHON_TEST_VALUE(core::Pair(font_spec::mono, font::size(13, 15)), Mono13x15),
                     ARCHON_TEST_VALUE(core::Pair(font_spec::mono, font::size(15, 13)), Mono15x13));


bool is_freetype_available() noexcept
{
    return font::get_freetype_implementation().is_available();
}


} // unnamed namespace


ARCHON_TEST(Font_Fallback_Basics)
{
    const font::implementation& impl = font::get_fallback_implementation();
    ARCHON_CHECK_EQUAL(impl.get_ident(), "fallback");
    ARCHON_CHECK(impl.is_available());
    namespace fs = std::filesystem;
    fs::path resource_path = test_context.get_data_path(g_test_dir_path, "..");
    font::loader::config config;
    config.logger = &test_context.logger;
    std::unique_ptr<font::loader> loader = impl.new_loader(resource_path, test_context.locale, config);
    ARCHON_CHECK_EQUAL(loader->get_num_faces(), 1);
    std::unique_ptr<font::face> face = loader->load_default_face();
    ARCHON_CHECK_NOT(face->is_scalable());
    ARCHON_CHECK_NOT(face->has_color());
    ARCHON_CHECK_EQUAL(face->get_num_fixed_sizes(), 1);
}


ARCHON_TEST_BATCH_IF(Font_Fallback_FreetypeRoundtrip, font_size_variants, ::is_freetype_available())
{
    ::font_spec font_spec = test_value.first;
    font::size font_size = test_value.second;

    std::string_view font_file = "../liberation-sans-regular.ttf";
    switch (font_spec) {
        case ::font_spec::sans:
            break;
        case ::font_spec::mono:
            font_file = "../liberation-mono-regular.ttf";
            break;
    }

    namespace fs = std::filesystem;
    fs::path font_file_2 = test_context.get_data_path(g_test_dir_path, font_file);

    // Use FreeType implementation to load test font
    font::loader::config loader_config;
    loader_config.logger = &test_context.logger;
    std::unique_ptr<font::loader> freetype_loader =
        font::new_freetype_loader_from_font_file(font_file_2, test_context.locale, loader_config);
    std::unique_ptr<font::face> freetype_face = freetype_loader->load_default_face();
    freetype_face->set_approx_size(font_size);

    // Generate fallback font
    font::code_point first, last;
    bool success = (first.try_from_int(0) && last.try_from_int(65534));
    ARCHON_ASSERT(success);
    font::code_point_range ranges[] {
        { first, last },
    };
    ARCHON_TEST_DIR(fallback_font_dir);
    std::string_view file_name_qual = ""; // Use genuine file names
    font::regen_fallback_font(*freetype_face, ranges, fallback_font_dir, file_name_qual, test_context.locale,
                              loader_config);
    freetype_face->reset_transform_translat();
    freetype_face->set_target_pos({ 0, 0 });

    // Load the generated fallback font
    std::unique_ptr<font::loader> fallback_loader =
        font::new_fallback_loader(fallback_font_dir, test_context.locale, loader_config);
    std::unique_ptr<font::face> fallback_face = fallback_loader->load_default_face();

    // Font selection parameters
    ARCHON_CHECK_EQUAL(fallback_face->get_family_name(), freetype_face->get_family_name());
    ARCHON_CHECK_EQUAL(fallback_face->is_bold(), freetype_face->is_bold());
    ARCHON_CHECK_EQUAL(fallback_face->is_italic(), freetype_face->is_italic());
    ARCHON_CHECK_EQUAL(fallback_face->is_monospace(), freetype_face->is_monospace());
    ARCHON_CHECK_NOT(fallback_face->is_scalable());
    if (ARCHON_LIKELY(ARCHON_CHECK_EQUAL(fallback_face->get_num_fixed_sizes(), 1)))
        ARCHON_CHECK_EQUAL(fallback_face->get_fixed_size(0), font_size);
    ARCHON_CHECK_EQUAL(fallback_face->get_size(), font_size);

    // Global metrics
    bool grid_fitting = true;
    for (bool vertical : { false, true }) {
        ARCHON_CHECK_EQUAL(fallback_face->get_ascender(grid_fitting, vertical),
                           freetype_face->get_ascender(grid_fitting, vertical));
        ARCHON_CHECK_EQUAL(fallback_face->get_descender(grid_fitting, vertical),
                           freetype_face->get_descender(grid_fitting, vertical));
        ARCHON_CHECK_EQUAL(fallback_face->get_baseline_spacing(grid_fitting, vertical),
                           freetype_face->get_baseline_spacing(grid_fitting, vertical));
    }

    // Individual glyphs
    core::Buffer<image::int8_type> buffer_1, buffer_2;
    auto compare = [&, &parent_test_context = test_context](font::code_point cp) {
        ARCHON_TEST_TRAIL(parent_test_context, cp);
        std::size_t index_1 = fallback_face->find_glyph(cp.to_char());
        std::size_t index_2 = freetype_face->find_glyph(cp.to_char());
        bool found_1 = (index_1 != 0);
        bool found_2 = (index_2 != 0);
        ARCHON_CHECK_EQUAL(found_1, found_2);
        if (ARCHON_LIKELY(!found_1 || !found_2))
            return;
        ARCHON_CHECK(fallback_face->try_load_glyph(index_1, grid_fitting));
        ARCHON_CHECK(freetype_face->try_load_glyph(index_2, grid_fitting));
        for (bool vertical : { false, true }) {
            ARCHON_CHECK_EQUAL(fallback_face->get_glyph_advance(vertical),
                               freetype_face->get_glyph_advance(vertical));
            ARCHON_CHECK_EQUAL(fallback_face->get_glyph_bearing(vertical),
                               freetype_face->get_glyph_bearing(vertical));
        }
        ARCHON_CHECK_EQUAL(fallback_face->get_glyph_pos(), freetype_face->get_glyph_pos());
        ARCHON_CHECK_EQUAL(fallback_face->get_glyph_size(), freetype_face->get_glyph_size());
        ARCHON_CHECK_NOT(fallback_face->glyph_may_be_colored());
        image::Box box = fallback_face->get_target_glyph_box();
        if (ARCHON_LIKELY(ARCHON_CHECK_EQUAL(box, freetype_face->get_target_glyph_box()))) {
            {
                using block_type = image::PixelBlock_Alpha_8;
                block_type block_1(box.size, buffer_1);
                block_type block_2(box.size, buffer_2);
                fallback_face->render_glyph_mask_a(box.pos, block_1.tray());
                freetype_face->render_glyph_mask_a(box.pos, block_2.tray());
                ARCHON_CHECK(block_1 == block_2);
            }
            {
                using block_type = image::PixelBlock_RGBA_8;
                block_type block_1(box.size, buffer_1);
                block_type block_2(box.size, buffer_2);
                fallback_face->render_glyph_rgba_a(box.pos, block_1.tray());
                freetype_face->render_glyph_rgba_a(box.pos, block_2.tray());
                ARCHON_CHECK(block_1 == block_2);
            }
        }
    };
    for (font::code_point_range range : ranges) {
        font::code_point::char_type ch = range.first().to_char();
        for (;;) {
            font::code_point cp;
            if (ARCHON_LIKELY(cp.try_from_char(ch)))
                compare(cp);
            if (ch == range.last().to_char())
                break;
            ++ch;
        }
    }
}
