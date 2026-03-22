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
#include <memory>
#include <string_view>
#include <filesystem>

#include <archon/core/char_mapper.hpp>
#include <archon/check.hpp>
#include <archon/font/size.hpp>
#include <archon/font/face.hpp>
#include <archon/font/loader.hpp>
#include <archon/font/freetype_implementation.hpp>


using namespace archon;


namespace {

constexpr std::string_view g_test_dir_path = "archon/font/test";

} // unnamed namespace


ARCHON_TEST_IF(Font_Freetype, font::get_freetype_implementation().is_available())
{
    namespace fs = std::filesystem;
    fs::path file = test_context.get_data_path(g_test_dir_path, "test_font.ttf");
    font::Loader::Config config;
    config.logger = &test_context.logger;
    std::unique_ptr<font::Loader> loader =
        font::new_freetype_loader_from_font_file(file, test_context.locale, config); // Throws
    ARCHON_CHECK_EQUAL(loader->get_num_faces(), 1);
    std::unique_ptr<font::Face> face = loader->load_default_face();
    ARCHON_CHECK_EQUAL(face->get_family_name(), "Test");
    ARCHON_CHECK_NOT(face->is_bold());
    ARCHON_CHECK_NOT(face->is_italic());
    ARCHON_CHECK_NOT(face->is_monospace());
    ARCHON_CHECK(face->is_scalable());
    ARCHON_CHECK_EQUAL(face->get_num_fixed_sizes(), 0);
    ARCHON_CHECK_EQUAL(face->get_size(), font::Size(12));
    face->set_scaled_size(1024);
    ARCHON_CHECK_EQUAL(face->get_size(), font::Size(1024));
    ARCHON_CHECK_EQUAL(face->get_baseline_spacing(), 1024);
    ARCHON_CHECK_EQUAL(face->get_baseline_offset(), 212);

    // Test glyph
    core::WideCharMapper mapper(test_context.locale);
    std::size_t i = face->find_glyph(mapper.widen('A'));
    ARCHON_CHECK_NOT_EQUAL(i, 0);
    face->load_glyph(i);
    ARCHON_CHECK_EQUAL(face->get_glyph_advance(), 948);
    using vector_type = font::Face::vector_type;
    ARCHON_CHECK_EQUAL(face->get_glyph_bearing(), vector_type(462, 0));
    ARCHON_CHECK_EQUAL(face->get_glyph_pa_size(), image::Size(924, 800));
    int left = {}, right = {}, bottom = {}, top = {};
    face->get_glyph_pa_box(left, right, bottom, top);
    ARCHON_CHECK_EQUAL(left,     0);
    ARCHON_CHECK_EQUAL(right,  924);
    ARCHON_CHECK_EQUAL(bottom,   0);
    ARCHON_CHECK_EQUAL(top,    800);

    // Blank glyph
    i = face->find_glyph(mapper.widen(' '));
    ARCHON_CHECK_NOT_EQUAL(i, 0);
    face->load_glyph(i);
    ARCHON_CHECK_EQUAL(face->get_glyph_advance(), 256);
    ARCHON_CHECK_EQUAL(face->get_glyph_bearing(), vector_type(0, 0));
    ARCHON_CHECK_EQUAL(face->get_glyph_pa_size(), image::Size(0));
    face->get_glyph_pa_box(left, right, bottom, top);
    ARCHON_CHECK_EQUAL(left,   0);
    ARCHON_CHECK_EQUAL(right,  0);
    ARCHON_CHECK_EQUAL(bottom, 0);
    ARCHON_CHECK_EQUAL(top,    0);

    // Fallback glyph
    i = face->find_glyph(mapper.widen('B'));
    ARCHON_CHECK_EQUAL(i, 0);
    face->load_glyph(i);
    ARCHON_CHECK_EQUAL(face->get_glyph_advance(), 600);
    ARCHON_CHECK_EQUAL(face->get_glyph_bearing(), vector_type(-50, 0));
    ARCHON_CHECK_EQUAL(face->get_glyph_pa_size(), image::Size(500, 700));
    face->get_glyph_pa_box(left, right, bottom, top);
    ARCHON_CHECK_EQUAL(left,     0);
    ARCHON_CHECK_EQUAL(right,  500);
    ARCHON_CHECK_EQUAL(bottom,   0);
    ARCHON_CHECK_EQUAL(top,    700);

    // Translated test glyph
    i = face->find_glyph(mapper.widen('A'));
    face->load_glyph(i);
    face->translate_glyph({ -200, -100 });
    ARCHON_CHECK_EQUAL(face->get_glyph_advance(), 948);
    ARCHON_CHECK_EQUAL(face->get_glyph_bearing(), vector_type(462, 0));
    ARCHON_CHECK_EQUAL(face->get_glyph_pa_size(), image::Size(924, 800));
    face->get_glyph_pa_box(left, right, bottom, top);
    ARCHON_CHECK_EQUAL(left,   -200);
    ARCHON_CHECK_EQUAL(right,   724);
    ARCHON_CHECK_EQUAL(bottom, -100);
    ARCHON_CHECK_EQUAL(top,     700);
}
