// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
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


#include <array>
#include <string_view>
#include <string>
#include <vector>

#include <archon/core/buffer.hpp>
#include <archon/core/char_mapper.hpp>
#include <archon/core/text_file_impl.hpp>
#include <archon/core/buffered_text_file.hpp>
#include <archon/check.hpp>


using namespace archon;


namespace {


ARCHON_TEST_VARIANTS(impl_variants,
                     ARCHON_TEST_TYPE(core::BufferedPosixTextFileImpl<char>,      Posix),
                     ARCHON_TEST_TYPE(core::BufferedWindowsTextFileImpl<char>,    Windows),
                     ARCHON_TEST_TYPE(core::BufferedPosixTextFileImpl<wchar_t>,   WidePosix),
                     ARCHON_TEST_TYPE(core::BufferedWindowsTextFileImpl<wchar_t>, WideWindows));


} // unnamed namespace



ARCHON_TEST_BATCH(Core_BufferedTextFile_ReadLine, impl_variants)
{
    using impl_type = test_type;
    using text_file_type = core::GenericBufferedTextFile<impl_type>;
    using char_type = typename text_file_type::char_type;
    using string_type = std::basic_string<char_type>;
    using string_view_type = std::basic_string_view<char_type>;
    std::array<char_type, 16> seed_memory;
    core::BasicStringWidener<char_type> widener(test_context.locale, seed_memory);
    ARCHON_TEST_FILE(path);

    // Create text file with two lines, and verify that those two lines are read
    {
        text_file_type::save(path, widener.widen("foo\nbar\n"), test_context.locale);
        text_file_type file(path, test_context.locale);
        core::Buffer<char_type> buffer;
        string_view_type line;
        std::vector<string_type> lines;
        while (file.read_line(buffer, line))
            lines.push_back(string_type(line));
        string_type expected_lines[] = {
            string_type(widener.widen("foo")),
            string_type(widener.widen("bar"))
        };
        ARCHON_CHECK_EQUAL_SEQ(lines, expected_lines);
    }

    // Create text file with two lines, but with no newline termination on last line, and
    // verify that those two lines are read
    {
        text_file_type::save(path, widener.widen("foo\nbar"), test_context.locale);
        text_file_type file(path, test_context.locale);
        core::Buffer<char_type> buffer;
        string_view_type line;
        std::vector<string_type> lines;
        while (file.read_line(buffer, line))
            lines.push_back(string_type(line));
        string_type expected_lines[] = {
            string_type(widener.widen("foo")),
            string_type(widener.widen("bar"))
        };
        ARCHON_CHECK_EQUAL_SEQ(lines, expected_lines);
    }

    // Create text file with two lines, but where last line is empty, and verify that those
    // two lines are read
    {
        text_file_type::save(path, widener.widen("foo\n\n"), test_context.locale);
        text_file_type file(path, test_context.locale);
        core::Buffer<char_type> buffer;
        string_view_type line;
        std::vector<string_type> lines;
        while (file.read_line(buffer, line))
            lines.push_back(string_type(line));
        string_type expected_lines[] = {
            string_type(widener.widen("foo")),
            string_type(widener.widen(""))
        };
        ARCHON_CHECK_EQUAL_SEQ(lines, expected_lines);
    }
}


ARCHON_TEST_BATCH(Core_BufferedTextFile_WriteLine, impl_variants)
{
    using impl_type = test_type;
    using text_file_type = core::GenericBufferedTextFile<impl_type>;
    using char_type = typename text_file_type::char_type;
    using string_type = std::basic_string<char_type>;
    std::array<char_type, 16> seed_memory;
    core::BasicStringWidener<char_type> widener(test_context.locale, seed_memory);
    ARCHON_TEST_FILE(path);
    {
        text_file_type file(path, text_file_type::Mode::write, test_context.locale);
        file.write_line(widener.widen("foo"));
        file.write_line(widener.widen("bar"));
        file.flush();
    }
    string_type contents = text_file_type::load(path);
    ARCHON_CHECK_EQUAL(contents, widener.widen("foo\nbar\n"));
}
