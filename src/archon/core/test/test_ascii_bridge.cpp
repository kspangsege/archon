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
#include <iterator>
#include <array>
#include <string_view>
#include <string>
#include <locale>

#include <archon/core/features.h>
#include <archon/core/buffer.hpp>
#include <archon/core/locale.hpp>
#include <archon/core/ascii_bridge.hpp>
#include <archon/core/quote.hpp>
#include <archon/check.hpp>
#include <archon/core/test/locale_utils.hpp>


using namespace archon;


namespace {

auto fallback_level_to_string(core::native_mb_to_ascii_transcoder::fallback_level level) -> std::string_view
{
    using fallback_level = core::native_mb_to_ascii_transcoder::fallback_level;
    switch (level) {
        case fallback_level::normal:
            return "normal";
        case fallback_level::no_unicode_assumption:
            return "no_unicode_assumption";
        case fallback_level::no_utf8_assumption:
            return "no_utf8_assumption";
        case fallback_level::no_unicode_or_utf8_assumption:
            return "no_unicode_or_utf8_assumption";
    }
    return {};
}

auto fallback_level_to_string(core::ascii_to_native_mb_transcoder::fallback_level level) -> std::string_view
{
    using fallback_level = core::ascii_to_native_mb_transcoder::fallback_level;
    switch (level) {
        case fallback_level::normal:
            return "normal";
        case fallback_level::no_unicode_assumption:
            return "no_unicode_assumption";
        case fallback_level::no_utf8_assumption:
            return "no_utf8_assumption";
        case fallback_level::no_unicode_or_utf8_assumption:
            return "no_unicode_or_utf8_assumption";
    }
    return {};
}

} // unnamed namespace


ARCHON_TEST(Core_AsciiBridge_TranscodeNativeMbToAscii)
{
    auto test = [&, &parent_test_context = test_context](const std::locale& locale) {
        ARCHON_TEST_TRAIL(parent_test_context, core::quoted(std::string_view(locale.name())));
        using fallback_level = core::native_mb_to_ascii_transcoder::fallback_level;
        auto subtest = [&, &parent_test_context = test_context](fallback_level level) {
            ARCHON_TEST_TRAIL(parent_test_context, fallback_level_to_string(level));

            bool is_unicode = core::assume_unicode_locale(locale);
            bool is_utf8    = core::assume_utf8_locale(locale);
            bool allow_assume_unicode = true;
            bool allow_assume_utf8    = true;
            switch (level) {
                case fallback_level::normal:
                    break;
                case fallback_level::no_unicode_assumption:
                    allow_assume_unicode = false;
                    break;
                case fallback_level::no_utf8_assumption:
                    allow_assume_utf8 = false;
                    break;
                case fallback_level::no_unicode_or_utf8_assumption:
                    allow_assume_unicode = false;
                    allow_assume_utf8 = false;
                    break;
            }
            test_context.logger.detail("is_unicode: %s, is_utf8: %s, allow_assume_unicode: %s, allow_assume_utf8: %s",
                                       (is_unicode ? "yes" : "no"), (is_utf8 ? "yes" : "no"),
                                       (allow_assume_unicode ? "yes" : "no"), (allow_assume_utf8 ? "yes" : "no"));

            core::native_mb_to_ascii_transcoder transcoder(locale, level);
            std::array<char, 32> seed_memory;
            core::Buffer<char> buffer_1;
            core::Buffer<char> buffer_2(seed_memory);

            auto subsubtest = [&, &parent_test_context = test_context](bool empty) {
                ARCHON_TEST_TRAIL(parent_test_context, (empty ? "empty" : "nonempty"));
                core::Buffer<char>& buffer = (empty ? buffer_1 : buffer_2);
                std::string_view string = "xyz";
                std::size_t buffer_offset = 0;
                transcoder.transcode_l(string, buffer, buffer_offset);
                std::string_view string_2 = { buffer.data(), buffer_offset };
                char bytes[] = {
                    std::char_traits<char>::to_char_type(0x78), // `x`
                    std::char_traits<char>::to_char_type(0x79), // `y`
                    std::char_traits<char>::to_char_type(0x7A), // `z`
                };
                std::string_view string_3 = { bytes, std::size(bytes) };
                ARCHON_CHECK_EQUAL(string_2, string_3);
            };

            subsubtest(false); // Starting with empty buffer
            subsubtest(true);  // Starting with nonempty buffer

            // Input that is valid ASCII but outside the basic character set
            if (is_unicode && is_utf8 && allow_assume_unicode) {
                char bytes[] = {
                    std::char_traits<char>::to_char_type(0x7F), // DEL
                };
                std::string_view string = { bytes, std::size(bytes) };

                std::size_t buffer_offset = 0;
                transcoder.transcode_l(string, buffer_2, buffer_offset);
                std::string_view string_2 = { buffer_2.data(), buffer_offset };
                ARCHON_CHECK_EQUAL(string_2, string);
            }
            if (is_unicode && is_utf8 && allow_assume_unicode) {
                char bytes[] = {
                    '*',
                    std::char_traits<char>::to_char_type(0x7F), // DEL
                    '*',
                };
                std::string_view string = { bytes, std::size(bytes) };
                std::size_t buffer_offset = 0;
                transcoder.transcode_l(string, buffer_2, buffer_offset);
                std::string_view string_2 = { buffer_2.data(), buffer_offset };
                ARCHON_CHECK_EQUAL(string_2, string);
            }

            // Input that is invalid UTF-8
            if (is_utf8 && !allow_assume_utf8) {
                char bytes[] = {
                    '*',
                    std::char_traits<char>::to_char_type(0x90),
                    '*',
                };
                std::string_view string = { bytes, std::size(bytes) };
                std::size_t buffer_offset = 0;
                transcoder.transcode_l(string, buffer_2, buffer_offset);
                std::string_view string_2 = { buffer_2.data(), buffer_offset };
                ARCHON_CHECK_EQUAL(string_2, "*?*");
            }

            // Input that is valid UTF-8 but not representable in ASCII
            if (is_utf8 && !allow_assume_utf8) {
                // 4-byte UTF-8 sequence: Hwair (Gothic letter)
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
                transcoder.transcode_l(string, buffer_2, buffer_offset);
                std::string_view string_2 = { buffer_2.data(), buffer_offset };
                ARCHON_CHECK_EQUAL(string_2, "*?*");
            }
        };

        subtest(fallback_level::normal);
#if ARCHON_DEBUG
        subtest(fallback_level::no_unicode_assumption);
        subtest(fallback_level::no_utf8_assumption);
        subtest(fallback_level::no_unicode_or_utf8_assumption);
#endif
    };

    for (const std::locale& locale : core::test::get_candidate_locales())
        test(locale);
}


ARCHON_TEST(Core_AsciiBridge_TranscodeAsciiToNativeMb)
{
    auto test = [&, &parent_test_context = test_context](const std::locale& locale) {
        ARCHON_TEST_TRAIL(parent_test_context, core::quoted(std::string_view(locale.name())));
        using fallback_level = core::ascii_to_native_mb_transcoder::fallback_level;
        auto subtest = [&, &parent_test_context = test_context](fallback_level level) {
            ARCHON_TEST_TRAIL(parent_test_context, fallback_level_to_string(level));

            bool is_unicode = core::assume_unicode_locale(locale);
            bool is_utf8    = core::assume_utf8_locale(locale);
            bool allow_assume_unicode = true;
            bool allow_assume_utf8    = true;
            switch (level) {
                case fallback_level::normal:
                    break;
                case fallback_level::no_unicode_assumption:
                    allow_assume_unicode = false;
                    break;
                case fallback_level::no_utf8_assumption:
                    allow_assume_utf8 = false;
                    break;
                case fallback_level::no_unicode_or_utf8_assumption:
                    allow_assume_unicode = false;
                    allow_assume_utf8 = false;
                    break;
            }
            test_context.logger.detail("is_unicode: %s, is_utf8: %s, allow_assume_unicode: %s, allow_assume_utf8: %s",
                                       (is_unicode ? "yes" : "no"), (is_utf8 ? "yes" : "no"),
                                       (allow_assume_unicode ? "yes" : "no"), (allow_assume_utf8 ? "yes" : "no"));

            core::ascii_to_native_mb_transcoder transcoder(locale, level);
            std::array<char, 32> seed_memory;
            core::Buffer<char> buffer_1;
            core::Buffer<char> buffer_2(seed_memory);

            auto subsubtest = [&, &parent_test_context = test_context](bool empty) {
                ARCHON_TEST_TRAIL(parent_test_context, (empty ? "empty" : "nonempty"));
                core::Buffer<char>& buffer = (empty ? buffer_1 : buffer_2);
                char bytes[] = {
                    std::char_traits<char>::to_char_type(0x78), // `x`
                    std::char_traits<char>::to_char_type(0x79), // `y`
                    std::char_traits<char>::to_char_type(0x7A), // `z`
                };
                std::string_view string_1 = { bytes, std::size(bytes) };
                std::size_t buffer_offset = 0;
                transcoder.transcode_l(string_1, buffer, buffer_offset);
                std::string_view string_2 = { buffer.data(), buffer_offset };
                std::string_view string_3 = "xyz";
                ARCHON_CHECK_EQUAL(string_2, string_3);
            };

            subsubtest(false); // Starting with empty buffer
            subsubtest(true);  // Starting with nonempty buffer

            // Input that is valid ASCII but outside the basic character set
            if (is_unicode && is_utf8 && allow_assume_unicode) {
                char bytes[] = {
                    std::char_traits<char>::to_char_type(0x7F), // DEL
                };
                std::string_view string = { bytes, std::size(bytes) };
                std::size_t buffer_offset = 0;
                transcoder.transcode_l(string, buffer_2, buffer_offset);
                std::string_view string_2 = { buffer_2.data(), buffer_offset };
                ARCHON_CHECK_EQUAL(string_2, string);
            }
            if (is_unicode && is_utf8 && allow_assume_unicode) {
                char bytes[] = {
                    '*',
                    std::char_traits<char>::to_char_type(0x7F), // DEL
                    '*',
                };
                std::string_view string = { bytes, std::size(bytes) };
                std::size_t buffer_offset = 0;
                transcoder.transcode_l(string, buffer_2, buffer_offset);
                std::string_view string_2 = { buffer_2.data(), buffer_offset };
                ARCHON_CHECK_EQUAL(string_2, string);
            }

            // Input that is invalid ASCII
            if (is_utf8 && allow_assume_unicode && !allow_assume_utf8) {
                char bytes_1[] = {
                    '*',
                    std::char_traits<char>::to_char_type(0x80),
                    '*',
                };
                std::string_view string = { bytes_1, std::size(bytes_1) };
                std::size_t buffer_offset = 0;
                transcoder.transcode_l(string, buffer_2, buffer_offset);
                std::string_view string_2 = { buffer_2.data(), buffer_offset };
                // 3-byte UTF-8 sequence: Unicode replacement character
                char bytes_2[] = {
                    '*',
                    std::char_traits<char>::to_char_type(0xEF),
                    std::char_traits<char>::to_char_type(0xBF),
                    std::char_traits<char>::to_char_type(0xBD),
                    '*',
                };
                std::string_view string_3 = { bytes_2, std::size(bytes_2) };
                ARCHON_CHECK_EQUAL(string_2, string_3);
            }
            if (is_utf8 && !allow_assume_unicode && !allow_assume_utf8) {
                char bytes[] = {
                    '*',
                    std::char_traits<char>::to_char_type(0x80),
                    '*',
                };
                std::string_view string = { bytes, std::size(bytes) };
                std::size_t buffer_offset = 0;
                transcoder.transcode_l(string, buffer_2, buffer_offset);
                std::string_view string_2 = { buffer_2.data(), buffer_offset };
                ARCHON_CHECK_EQUAL(string_2, "*?*");
            }
        };

        subtest(fallback_level::normal);
#if ARCHON_DEBUG
        subtest(fallback_level::no_unicode_assumption);
        subtest(fallback_level::no_utf8_assumption);
        subtest(fallback_level::no_unicode_or_utf8_assumption);
#endif
    };

    for (const std::locale& locale : core::test::get_candidate_locales())
        test(locale);
}
