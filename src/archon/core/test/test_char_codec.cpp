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


#include <cstddef>
#include <cwchar>
#include <array>
#include <string_view>
#include <locale>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/array_seeded_buffer.hpp>
#include <archon/core/char_codec.hpp>
#include <archon/core/string_codec.hpp>
#include <archon/core/format.hpp>
#include <archon/core/format_encoded.hpp>
#include <archon/core/quote.hpp>
#include <archon/core/string_formatter.hpp>
#include <archon/core/locale.hpp>
#include <archon/check.hpp>
#include <archon/core/test/locale_utils.hpp>


using namespace archon;


ARCHON_TEST(Core_CharCodec_DegenerateDecode)
{
    using codec_type = core::CharCodec;
    static_assert(codec_type::is_degen);
    codec_type codec(test_context.locale);
    std::mbstate_t state = {};
    std::string_view data = "foo";
    std::size_t data_offset = 0;
    bool end_of_data = true;
    std::array<char, 3> buffer;
    std::size_t buffer_offset = 0;
    bool error = false;
    bool complete = codec.decode(state, data, data_offset, end_of_data, buffer, buffer_offset, error);
    ARCHON_CHECK(complete) && ARCHON_CHECK_EQUAL(data_offset, 3) &&
        ARCHON_CHECK_EQUAL(buffer_offset, 3) && ARCHON_CHECK_NOT(error) &&
        ARCHON_CHECK_EQUAL_SEQ(buffer, data);
}


ARCHON_TEST(Core_CharCodec_DegenerateEncode)
{
    using codec_type = core::CharCodec;
    static_assert(codec_type::is_degen);
    codec_type codec(test_context.locale);
    std::mbstate_t state = {};
    std::string_view data = "foo";
    std::size_t data_offset = 0;
    std::array<char, 3> buffer;
    std::size_t buffer_offset = 0;
    bool error = false;
    bool complete = codec.encode(state, data, data_offset, buffer, buffer_offset, error);
    ARCHON_CHECK(complete) && ARCHON_CHECK_EQUAL(data_offset, 3) &&
        ARCHON_CHECK_EQUAL(buffer_offset, 3) && ARCHON_CHECK_NOT(error) &&
        ARCHON_CHECK_EQUAL_SEQ(buffer, data);
}


ARCHON_TEST(Core_CharCodec_Decode)
{
    core::ArraySeededBuffer<wchar_t, 10> buffer;

    auto subtest = [&, &parent_test_context = test_context](const std::locale& locale) {
        ARCHON_TEST_TRAIL(parent_test_context, core::quoted(std::string_view(locale.name())));
        bool is_utf8 = (core::assume_utf8_locale(locale) && (core::assume_unicode_locale(locale) || ARCHON_WINDOWS));
        core::WideSimpleCharCodec codec(locale);

        auto decode = [&, &parent_test_context =
                       test_context](std::string_view data, std::size_t split_pos,
                                     bool end_of_data, std::size_t buffer_size,
                                     std::size_t expected_data_advance,
                                     std::size_t expected_buffer_advance, bool expected_complete,
                                     bool expected_error) {
            ARCHON_TEST_TRAIL(parent_test_context,
                              core::formatted("decode(%s, %s, %s, %s)", core::quoted(data),
                                              split_pos, end_of_data, buffer_size));
            buffer.reserve(buffer_size + 1);
            std::mbstate_t state = {};
            std::size_t data_offset = 0;
            std::size_t buffer_offset = 0;
            if (split_pos > 0) {
                ARCHON_ASSERT(split_pos < data.size());
                bool end_of_data_2 = false;
                bool error = false;
                codec.decode(state, data.substr(0, split_pos), data_offset, end_of_data_2, buffer, buffer_offset,
                             error);
                if (ARCHON_UNLIKELY(!ARCHON_CHECK_NOT(error)))
                    return;
                if (ARCHON_UNLIKELY(!ARCHON_CHECK_LESS_EQUAL(buffer_offset, buffer_size)))
                    return;
            }
            std::size_t data_offset_2 = data_offset;
            std::size_t buffer_offset_2 = buffer_offset;
            bool error = false;
            core::Span buffer_2(buffer.data(), buffer_size);
            bool complete = codec.decode(state, data, data_offset_2, end_of_data, buffer_2, buffer_offset_2, error);
            ARCHON_CHECK_EQUAL(data_offset_2 - data_offset, expected_data_advance);
            ARCHON_CHECK_EQUAL(buffer_offset_2 - buffer_offset, expected_buffer_advance);
            ARCHON_CHECK_EQUAL(complete, expected_complete);
            ARCHON_CHECK_EQUAL(error, expected_error);
        };

        char decode_error_byte = 0;
        bool have_decode_error = core::test::find_decode_error<wchar_t>(locale, decode_error_byte);

        if (true) {
            decode("",               0, false,  0, 0, 0, true,  false);
            decode("",               0, false, 10, 0, 0, true,  false);

            decode("$",              0, false,  0, 0, 0, false, false);
            decode("$",              0, false,  1, 1, 1, true,  false);
            decode("$",              0, false, 10, 1, 1, true,  false);

            decode("$$",             0, false,  0, 0, 0, false, false);
            decode("$$",             0, false,  1, 1, 1, false, false);
            decode("$$",             0, false,  2, 2, 2, true,  false);
            decode("$$",             0, false, 10, 2, 2, true,  false);
        }

        if (have_decode_error) {
            char data[] { '$', decode_error_byte };

            decode({ data + 1, 1 },  0, false,  0, 0, 0, false, false);
            decode({ data + 1, 1 },  0, false,  1, 0, 0, false, true);
            decode({ data + 1, 1 },  0, false, 10, 0, 0, false, true);

            decode({ data + 0, 2 },  0, false,  0, 0, 0, false, false);
            decode({ data + 0, 2 },  0, false,  1, 1, 1, false, false);
            decode({ data + 0, 2 },  0, false,  2, 1, 1, false, true);
            decode({ data + 0, 2 },  0, false, 10, 1, 1, false, true);
        }

        if (is_utf8) {
            // 2-byte char (cent)
            decode("\xC2\xA2",       0, false,  0, 0, 0, false, false);
            decode("\xC2\xA2",       0, false,  1, 2, 1, true,  false);
            decode("\xC2\xA2",       0, false, 10, 2, 1, true,  false);
            decode("\xC2\xA2",       1, false,  0, 0, 0, false, false);
            decode("\xC2\xA2",       1, false,  1, 2, 1, true,  false);
            decode("\xC2\xA2",       1, false, 10, 2, 1, true,  false);

            // 3-byte char (euro)
            decode("\xE2\x82\xAC",   0, false,  0, 0, 0, false, false);
            decode("\xE2\x82\xAC",   0, false,  1, 3, 1, true,  false);
            decode("\xE2\x82\xAC",   0, false, 10, 3, 1, true,  false);
            decode("\xE2\x82\xAC",   1, false,  0, 0, 0, false, false);
            decode("\xE2\x82\xAC",   1, false,  1, 3, 1, true,  false);
            decode("\xE2\x82\xAC",   1, false, 10, 3, 1, true,  false);
            decode("\xE2\x82\xAC",   2, false,  0, 0, 0, false, false);
            decode("\xE2\x82\xAC",   2, false,  1, 3, 1, true,  false);
            decode("\xE2\x82\xAC",   2, false, 10, 3, 1, true,  false);

            // Something followed by 2-byte char (cent)
            decode("$\xC2\xA2",      0, false,  0, 0, 0, false, false);
            decode("$\xC2\xA2",      0, false,  1, 1, 1, false, false);
            decode("$\xC2\xA2",      0, false,  2, 3, 2, true,  false);
            decode("$\xC2\xA2",      0, false, 10, 3, 2, true,  false);

            // Only 1 byte of 2-byte char (cent)
            decode("\xC2",           0, false,  0, 0, 0, false, false);
            decode("\xC2",           0, false,  1, 0, 0, true,  false);
            decode("\xC2",           0, false, 10, 0, 0, true,  false);
            decode("\xC2",           0, true,   0, 0, 0, false, false);
            decode("\xC2",           0, true,   1, 0, 0, false, true);
            decode("\xC2",           0, true,  10, 0, 0, false, true);

            // Only 2 bytes of 3-byte char (euro)
            decode("\xE2\x82",       0, false,  0, 0, 0, false, false);
            decode("\xE2\x82",       0, false,  1, 0, 0, true,  false);
            decode("\xE2\x82",       0, false, 10, 0, 0, true,  false);
            decode("\xE2\x82",       1, false,  0, 0, 0, false, false);
            decode("\xE2\x82",       1, false,  1, 0, 0, true,  false);
            decode("\xE2\x82",       1, false, 10, 0, 0, true,  false);
            decode("\xE2\x82",       0, true,   0, 0, 0, false, false);
            decode("\xE2\x82",       0, true,   1, 0, 0, false, true);
            decode("\xE2\x82",       0, true,  10, 0, 0, false, true);
            decode("\xE2\x82",       1, true,   0, 0, 0, false, false);
            decode("\xE2\x82",       1, true,   1, 0, 0, false, true);
            decode("\xE2\x82",       1, true,  10, 0, 0, false, true);

            // Something followed by partial char
            decode("$\xC2",          0, false,  0, 0, 0, false, false);
            decode("$\xC2",          0, false,  1, 1, 1, false, false);
            decode("$\xC2",          0, false,  2, 1, 1, true,  false);
            decode("$\xC2",          0, false, 10, 1, 1, true,  false);
            decode("$\xC2",          0, true,   0, 0, 0, false, false);
            decode("$\xC2",          0, true,   1, 1, 1, false, false);
            decode("$\xC2",          0, true,   2, 1, 1, false, true);
            decode("$\xC2",          0, true,  10, 1, 1, false, true);

            // 1st byte of 1st char is bad
            decode("\xA2",           0, false,  0, 0, 0, false, false);
            decode("\xA2",           0, false,  1, 0, 0, false, true);
            decode("\xA2",           0, false, 10, 0, 0, false, true);

            // 2nd byte of 1st char (cent) is bad
            decode("\xC2$",          0, false,  0, 0, 0, false, false);
            decode("\xC2$",          0, false,  1, 0, 0, false, true);
            decode("\xC2$",          0, false, 10, 0, 0, false, true);
            decode("\xC2$",          1, false,  0, 0, 0, false, false);
            decode("\xC2$",          1, false,  1, 0, 0, false, true);
            decode("\xC2$",          1, false, 10, 0, 0, false, true);

            // 3rd byte of 1st char (euro) is bad
            decode("\xE2\x82$",      0, false,  0, 0, 0, false, false);
            decode("\xE2\x82$",      0, false,  1, 0, 0, false, true);
            decode("\xE2\x82$",      0, false, 10, 0, 0, false, true);
            decode("\xE2\x82$",      1, false,  0, 0, 0, false, false);
            decode("\xE2\x82$",      1, false,  1, 0, 0, false, true);
            decode("\xE2\x82$",      1, false, 10, 0, 0, false, true);

            // 1st byte of 2nd char is bad
            decode("$\xA2",          0, false,  0, 0, 0, false, false);
            decode("$\xA2",          0, false,  1, 1, 1, false, false);
            decode("$\xA2",          0, false,  2, 1, 1, false, true);
            decode("$\xA2",          0, false, 10, 1, 1, false, true);

            // 2nd byte of 2nd char (cent) is bad
            decode("$\xC2$",         0, false,  0, 0, 0, false, false);
            decode("$\xC2$",         0, false,  1, 1, 1, false, false);
            decode("$\xC2$",         0, false,  2, 1, 1, false, true);
            decode("$\xC2$",         0, false, 10, 1, 1, false, true);
        }
    };

    for (const std::locale& locale : core::test::get_candidate_locales())
        subtest(locale);
}


ARCHON_TEST(Core_CharCodec_Encode)
{
    std::array<wchar_t, 32> seed_memory;
    core::WideStringFormatter formatter(seed_memory, test_context.locale);
    core::ArraySeededBuffer<char, 20> buffer;

    auto subtest = [&, &parent_test_context = test_context](const std::locale& locale) {
        ARCHON_TEST_TRAIL(parent_test_context, core::quoted(std::string_view(locale.name())));
        bool is_utf8 = (core::assume_utf8_locale(locale) && (core::assume_unicode_locale(locale) || ARCHON_WINDOWS));
        core::WideSimpleCharCodec codec(locale);

        auto encode = [&, &parent_test_context =
                       test_context](core::Span<const wchar_t> data, std::size_t buffer_size,
                                     std::size_t expected_data_advance,
                                     std::size_t expected_buffer_advance, bool expected_complete,
                                     bool expected_error) {
            std::wstring_view data_2(data.data(), data.size());
            std::wstring_view segment = formatter.format("encode(%s, %s)", core::quoted(data_2), buffer_size);
            ARCHON_TEST_TRAIL(parent_test_context, core::encoded(segment));
            buffer.reserve(buffer_size);
            std::mbstate_t state = {};
            std::size_t data_offset = 0;
            core::Span buffer_2(buffer.data(), buffer_size);
            std::size_t buffer_offset = 0;
            bool error = false;
            bool complete = codec.encode(state, data, data_offset, buffer_2, buffer_offset, error);
            ARCHON_CHECK_EQUAL(data_offset, expected_data_advance);
            ARCHON_CHECK_EQUAL(buffer_offset, expected_buffer_advance);
            ARCHON_CHECK_EQUAL(complete, expected_complete);
            ARCHON_CHECK_EQUAL(error, expected_error);
        };

        using traits_type = std::char_traits<wchar_t>;
        wchar_t dollar = traits_type::to_char_type(0x24);

        wchar_t encode_error_char = 0;
        bool have_encode_error = core::test::find_encode_error(locale, encode_error_char);

        if (true) {
            encode(std::array<wchar_t, 0> {},      0, 0, 0, true,  false);
            encode(std::array<wchar_t, 0> {},     10, 0, 0, true,  false);

            encode(std::array { dollar },          0, 0, 0, false, false);
            encode(std::array { dollar },          1, 1, 1, true,  false);
            encode(std::array { dollar },         10, 1, 1, true,  false);

            encode(std::array { dollar, dollar },  0, 0, 0, false, false);
            encode(std::array { dollar, dollar },  1, 1, 1, false, false);
            encode(std::array { dollar, dollar },  2, 2, 2, true,  false);
            encode(std::array { dollar, dollar }, 10, 2, 2, true,  false);
        }

        if (have_encode_error) {
            wchar_t err = encode_error_char;

            encode(std::array { err },             0, 0, 0, false, false);
            encode(std::array { err },             1, 0, 0, false, true);
            encode(std::array { err },            10, 0, 0, false, true);

            encode(std::array { dollar, err },     0, 0, 0, false, false);
            encode(std::array { dollar, err },     1, 1, 1, false, false);
            encode(std::array { dollar, err },     2, 1, 1, false, true);
            encode(std::array { dollar, err },    10, 1, 1, false, true);
        }

        if (is_utf8) {
            wchar_t cent = traits_type::to_char_type(0x00A2);
            wchar_t euro = traits_type::to_char_type(0x20AC);

            encode(std::array { cent },            0, 0, 0, false, false);
            encode(std::array { cent },            1, 0, 0, false, false);
            encode(std::array { cent },            2, 1, 2, true,  false);
            encode(std::array { cent },           10, 1, 2, true,  false);

            encode(std::array { dollar, cent },    0, 0, 0, false, false);
            encode(std::array { dollar, cent },    1, 1, 1, false, false);
            encode(std::array { dollar, cent },    2, 1, 1, false, false);
            encode(std::array { dollar, cent },    3, 2, 3, true,  false);
            encode(std::array { dollar, cent },   10, 2, 3, true,  false);

            encode(std::array { euro },            0, 0, 0, false, false);
            encode(std::array { euro },            1, 0, 0, false, false);
            encode(std::array { euro },            2, 0, 0, false, false);
            encode(std::array { euro },            3, 1, 3, true,  false);
            encode(std::array { euro },           10, 1, 3, true,  false);

            encode(std::array { dollar, euro },    0, 0, 0, false, false);
            encode(std::array { dollar, euro },    1, 1, 1, false, false);
            encode(std::array { dollar, euro },    2, 1, 1, false, false);
            encode(std::array { dollar, euro },    3, 1, 1, false, false);
            encode(std::array { dollar, euro },    4, 2, 4, true,  false);
            encode(std::array { dollar, euro },   10, 2, 4, true,  false);
        }
    };

    for (const std::locale& locale : core::test::get_candidate_locales())
        subtest(locale);
}


ARCHON_TEST(Core_CharCodec_SimulDecode)
{
    core::ArraySeededBuffer<wchar_t, 10> buffer;

    auto subtest = [&, &parent_test_context = test_context](const std::locale& locale) {
        ARCHON_TEST_TRAIL(parent_test_context, core::quoted(std::string_view(locale.name())));
        bool is_utf8 = (core::assume_utf8_locale(locale) && (core::assume_unicode_locale(locale) || ARCHON_WINDOWS));
        core::WideSimpleCharCodec codec(locale);

        auto simul_decode = [&, &parent_test_context =
                       test_context](std::string_view data, std::size_t split_pos,
                                     std::size_t buffer_size, std::size_t expected_data_advance) {
            ARCHON_TEST_TRAIL(parent_test_context,
                              core::formatted("simul_decode(%s, %s, %s)", core::quoted(data),
                                              split_pos, buffer_size));
            buffer.reserve(buffer_size + 1);
            std::mbstate_t state = {};
            std::size_t data_offset = 0;
            std::size_t buffer_offset = 0;
            if (split_pos > 0) {
                ARCHON_ASSERT(split_pos < data.size());
                bool end_of_data = false;
                bool error = false;
                codec.decode(state, data.substr(0, split_pos), data_offset, end_of_data, buffer, buffer_offset, error);
                if (ARCHON_UNLIKELY(!ARCHON_CHECK_NOT(error)))
                    return;
                if (ARCHON_UNLIKELY(!ARCHON_CHECK_LESS_EQUAL(buffer_offset, buffer_size)))
                    return;
            }
            std::size_t data_offset_2 = data_offset;
            codec.simul_decode(state, data, data_offset_2, buffer_size);
            ARCHON_CHECK_EQUAL(data_offset_2 - data_offset, expected_data_advance);
        };

        if (true) {
            simul_decode("$$$",                       0, 0, 0);
            simul_decode("$$$",                       0, 1, 1);
            simul_decode("$$$",                       0, 2, 2);
        }

        if (is_utf8) {
            // Two 2-byte chars (cent)
            simul_decode("\xC2\xA2\xC2\xA2$",         0, 0, 0);
            simul_decode("\xC2\xA2\xC2\xA2$",         0, 1, 2);
            simul_decode("\xC2\xA2\xC2\xA2$",         0, 2, 4);
            simul_decode("\xC2\xA2\xC2\xA2$",         1, 0, 0);
            simul_decode("\xC2\xA2\xC2\xA2$",         1, 1, 2);
            simul_decode("\xC2\xA2\xC2\xA2$",         1, 2, 4);

            // Two 3-byte chars (euro)
            simul_decode("\xE2\x82\xAC\xE2\x82\xAC$", 0, 0, 0);
            simul_decode("\xE2\x82\xAC\xE2\x82\xAC$", 0, 1, 3);
            simul_decode("\xE2\x82\xAC\xE2\x82\xAC$", 0, 2, 6);
            simul_decode("\xE2\x82\xAC\xE2\x82\xAC$", 1, 0, 0);
            simul_decode("\xE2\x82\xAC\xE2\x82\xAC$", 1, 1, 3);
            simul_decode("\xE2\x82\xAC\xE2\x82\xAC$", 1, 2, 6);
            simul_decode("\xE2\x82\xAC\xE2\x82\xAC$", 2, 0, 0);
            simul_decode("\xE2\x82\xAC\xE2\x82\xAC$", 2, 1, 3);
            simul_decode("\xE2\x82\xAC\xE2\x82\xAC$", 2, 2, 6);
        }
    };

    for (const std::locale& locale : core::test::get_candidate_locales())
        subtest(locale);
}
