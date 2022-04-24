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


#include <filesystem>

#include <archon/check.hpp>
#include <archon/font/face.hpp>
#include <archon/font/loader.hpp>


using namespace archon;


namespace {

constexpr std::string_view g_test_dir_path = "archon/font/test";

} // unnamed namespace


ARCHON_TEST(Font_Loader_Basics)
{
    ARCHON_CHECK_GREATER_EQUAL(font::Loader::get_num_implementations(), 1);
    namespace fs = std::filesystem;
    fs::path resource_path = test_context.get_data_path(g_test_dir_path, "..");
    font::Loader::Config config;
    config.logger = &test_context.logger;
    auto test = [&](check::TestContext& parent_test_context, const font::Loader::Implementation& impl) {
        ARCHON_TEST_TRAIL(parent_test_context, impl.ident());
        std::unique_ptr<font::Loader> loader = impl.new_loader(resource_path, test_context.locale, config); // Throws
        std::unique_ptr<font::Face> face = loader->load_default_face();
        test_context.logger.info("%s", face->get_family_name());
    };
    int n = font::Loader::get_num_implementations();
    for (int i = 0; i < n; ++i)
        test(test_context, font::Loader::get_implementation(i));
}
