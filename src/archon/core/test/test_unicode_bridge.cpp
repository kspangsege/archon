// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2024 Kristian Spangsege <kristian.spangsege@gmail.com>
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
#include <array>
#include <string_view>
#include <string>

#include <archon/core/buffer.hpp>
#include <archon/core/unicode_bridge.hpp>
#include <archon/check.hpp>


using namespace archon;


ARCHON_TEST(Core_UnicodeBridge_TranscodeNativeMbToUtf8)
{
    core::native_mb_to_utf8_transcoder transcoder(test_context.locale);
    std::array<char, 32> seed_memory;
    core::Buffer buffer(seed_memory);

    {
        std::string_view string = "x";
        std::size_t buffer_offset = 0;
        transcoder.transcode_l(string, buffer, buffer_offset);
        std::string_view string_2 = { buffer.data(), buffer_offset };
        ARCHON_CHECK_EQUAL(string_2, string);
    }

    bool is_utf8 = core::assume_utf8_locale(test_context.locale);
    if (is_utf8)
        test_context.logger.detail("Is UTF-8 locale");

    if (is_utf8) {
        char bytes[] = {
            std::char_traits<char>::to_char_type(0xF0),
            std::char_traits<char>::to_char_type(0x90),
            std::char_traits<char>::to_char_type(0x8D),
            std::char_traits<char>::to_char_type(0x88),
        };
        std::string_view string = { bytes, std::size(bytes) };
        std::size_t buffer_offset = 0;
        transcoder.transcode_l(string, buffer, buffer_offset);
        std::string_view string_2 = { buffer.data(), buffer_offset };
        ARCHON_CHECK_EQUAL(string_2, string);
    }

    if (is_utf8) {
        char bytes[] = {
            '*',
            std::char_traits<char>::to_char_type(0xF0),
            std::char_traits<char>::to_char_type(0x90),
            std::char_traits<char>::to_char_type(0x8D),
            std::char_traits<char>::to_char_type(0x88),
            '*',
        };
        std::string_view string = { bytes, std::size(bytes) };
        std::size_t buffer_offset = 0;
        transcoder.transcode_l(string, buffer, buffer_offset);
        std::string_view string_2 = { buffer.data(), buffer_offset };
        ARCHON_CHECK_EQUAL(string_2, string);
    }
}
