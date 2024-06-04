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
#include <initializer_list>
#include <string_view>
#include <string>
#include <locale>
#include <ostream>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/char_mapper.hpp>
#include <archon/core/char_codec.hpp>
#include <archon/core/format.hpp>
#include <archon/core/as_list.hpp>
#include <archon/core/format_as.hpp>
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
    std::array<wchar_t, 10> seed_memory;
    core::Buffer buffer(seed_memory);

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
        {
            auto format = [&](std::ostream& out) {
                if (have_decode_error) {
                    std::string_view str = { &decode_error_byte, 1 };
                    out << core::formatted("Yes (%s)", core::quoted_s(str));
                }
                else {
                    out << "No";
                }
            };
            test_context.logger.detail("Have decode error: %s", core::as_format_func(format));
        }

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
    std::array<wchar_t, 32> seed_memory_1;
    core::WideStringFormatter formatter(seed_memory_1, test_context.locale);

    std::array<char, 20> seed_memory_2;
    core::Buffer buffer(seed_memory_2);

    auto subtest = [&, &parent_test_context = test_context](const std::locale& locale) {
        ARCHON_TEST_TRAIL(parent_test_context, core::quoted(std::string_view(locale.name())));
        bool is_utf8 = (core::assume_utf8_locale(locale) && (core::assume_unicode_locale(locale) || ARCHON_WINDOWS));
        core::WideSimpleCharCodec codec(locale);

        auto encode = [&, &parent_test_context =
                       test_context](std::initializer_list<wchar_t> data, std::size_t buffer_size,
                                     std::size_t expected_data_advance,
                                     std::size_t expected_buffer_advance, bool expected_complete,
                                     bool expected_error) {
            std::wstring_view data_2(data.begin(), data.size());
            std::wstring_view segment = formatter.format("encode(%s, %s)", core::quoted(data_2), buffer_size);
            ARCHON_TEST_TRAIL(parent_test_context, core::encoded(segment));
            buffer.reserve(buffer_size);
            std::mbstate_t state = {};
            std::size_t data_offset = 0;
            core::Span buffer_2(buffer.data(), buffer_size);
            std::size_t buffer_offset = 0;
            bool error = false;
            bool complete = codec.encode(state, data_2, data_offset, buffer_2, buffer_offset, error);
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
            encode({},                  0, 0, 0, true,  false);
            encode({},                 10, 0, 0, true,  false);

            encode({ dollar },          0, 0, 0, false, false);
            encode({ dollar },          1, 1, 1, true,  false);
            encode({ dollar },         10, 1, 1, true,  false);

            encode({ dollar, dollar },  0, 0, 0, false, false);
            encode({ dollar, dollar },  1, 1, 1, false, false);
            encode({ dollar, dollar },  2, 2, 2, true,  false);
            encode({ dollar, dollar }, 10, 2, 2, true,  false);
        }

        if (have_encode_error) {
            wchar_t err = encode_error_char;

            encode({ err },             0, 0, 0, false, false);
            encode({ err },             1, 0, 0, false, true);
            encode({ err },            10, 0, 0, false, true);

            encode({ dollar, err },     0, 0, 0, false, false);
            encode({ dollar, err },     1, 1, 1, false, false);
            encode({ dollar, err },     2, 1, 1, false, true);
            encode({ dollar, err },    10, 1, 1, false, true);
        }

        if (is_utf8) {
            wchar_t cent = traits_type::to_char_type(0x00A2);
            wchar_t euro = traits_type::to_char_type(0x20AC);

            encode({ cent },            0, 0, 0, false, false);
            encode({ cent },            1, 0, 0, false, false);
            encode({ cent },            2, 1, 2, true,  false);
            encode({ cent },           10, 1, 2, true,  false);

            encode({ dollar, cent },    0, 0, 0, false, false);
            encode({ dollar, cent },    1, 1, 1, false, false);
            encode({ dollar, cent },    2, 1, 1, false, false);
            encode({ dollar, cent },    3, 2, 3, true,  false);
            encode({ dollar, cent },   10, 2, 3, true,  false);

            encode({ euro },            0, 0, 0, false, false);
            encode({ euro },            1, 0, 0, false, false);
            encode({ euro },            2, 0, 0, false, false);
            encode({ euro },            3, 1, 3, true,  false);
            encode({ euro },           10, 1, 3, true,  false);

            encode({ dollar, euro },    0, 0, 0, false, false);
            encode({ dollar, euro },    1, 1, 1, false, false);
            encode({ dollar, euro },    2, 1, 1, false, false);
            encode({ dollar, euro },    3, 1, 1, false, false);
            encode({ dollar, euro },    4, 2, 4, true,  false);
            encode({ dollar, euro },   10, 2, 4, true,  false);
        }
    };

    for (const std::locale& locale : core::test::get_candidate_locales())
        subtest(locale);
}


ARCHON_TEST(Core_CharCodec_SimulDecode)
{
    std::array<wchar_t, 10> seed_memory;
    core::Buffer buffer(seed_memory);

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


ARCHON_TEST(Core_CharCodec_LenientDecode)
{
    using wide_traits_type = std::char_traits<wchar_t>;
    using wide_int_type = wide_traits_type::int_type;

    std::array<wchar_t, 64> seed_memory_1;
    std::array<wchar_t, 64> seed_memory_2;

    core::Buffer buffer_1(seed_memory_1);
    core::Buffer buffer_2(seed_memory_2);

    auto subtest = [&, &parent_test_context = test_context](const std::locale& locale) {
        ARCHON_TEST_TRAIL(parent_test_context, core::quoted(std::string_view(locale.name())));
        bool is_utf8 = (core::assume_utf8_locale(locale) && (core::assume_unicode_locale(locale) || ARCHON_WINDOWS));

        core::WideCharCodec::Config config;
        config.lenient = true;
        config.use_fallback_replacement_char = true;
        core::WideCharCodec codec(locale, config);

        core::WideCharMapper char_mapper(locale);
        wide_int_type replacement = wide_traits_type::to_int_type(char_mapper.widen('?'));
        wide_int_type dollar      = wide_traits_type::to_int_type(char_mapper.widen('$'));
        wide_int_type star        = wide_traits_type::to_int_type(char_mapper.widen('*'));

        auto decode = [&, &parent_test_context =
                       test_context](std::string_view input, bool end_of_input, std::size_t output_size,
                                     std::size_t expected_input_advance,
                                     std::initializer_list<wide_int_type> expected_output, bool expected_complete) {
            ARCHON_TEST_TRAIL(parent_test_context,
                              core::formatted_wrn("%s, %s, %s, %s, %s, %s", core::quoted(input), end_of_input,
                                                  output_size, expected_input_advance,
                                                  core::as_sbr_list(expected_output), expected_complete));
            buffer_1.reserve(output_size);
            std::size_t buffer_offset = 0;
            for (wide_int_type val : expected_output)
                buffer_2.append_a(wide_traits_type::to_char_type(val), buffer_offset);
            std::mbstate_t state = {};
            std::size_t input_offset = 0;
            core::Span output = { buffer_1.data(), output_size };
            std::size_t output_offset = 0;
            bool error = false;
            bool complete = codec.decode(state, input, input_offset, end_of_input, output, output_offset, error);
            {
                // Collapse runs of '?'
                wchar_t replacement = char_mapper.widen('?');
                bool prev_is_replacement = false;
                std::size_t i = 0;
                for (std::size_t j = 0; j < output_offset; ++j) {
                    wchar_t ch = output[j];
                    bool is_replacement = (ch == replacement);
                    if (ARCHON_LIKELY(!is_replacement || !prev_is_replacement)) {
                        output[i] = output[j];
                        ++i;
                    }
                    prev_is_replacement = is_replacement;
                }
                output_offset = i;
            }
            ARCHON_CHECK_EQUAL(input_offset, expected_input_advance);
            std::wstring_view output_2 = { buffer_1.data(), output_offset };
            std::wstring_view expected_output_2 = { buffer_2.data(), buffer_offset };
            ARCHON_CHECK_EQUAL(output_2, expected_output_2);
            ARCHON_CHECK_EQUAL(complete, expected_complete);
            ARCHON_CHECK_NOT(error);
        };

        char decode_error_byte = 0;
        bool followed_by_star = true;
        bool have_decode_error = core::test::find_decode_error<wchar_t>(locale, decode_error_byte, followed_by_star);
        {
            auto format = [&](std::ostream& out) {
                if (have_decode_error) {
                    char str_1[] = { decode_error_byte, '*' };
                    std::string_view str_2 = { str_1, std::size(str_1) };
                    out << core::formatted("Yes (%s)", core::quoted(str_2));
                }
                else {
                    out << "No";
                }
            };
            test_context.logger.detail("Have decode error: %s", core::as_format_func(format));
        }

        if (true) {
            decode("",              false,  0, 0, {},                            true);
            decode("",              false, 10, 0, {},                            true);

            decode("$",             false,  0, 0, {},                            false);
            decode("$",             false,  1, 1, { dollar },                    true);
            decode("$",             false, 10, 1, { dollar },                    true);

            decode("$$",            false,  0, 0, {},                            false);
            decode("$$",            false,  1, 1, { dollar },                    false);
            decode("$$",            false,  2, 2, { dollar, dollar },            true);
            decode("$$",            false, 10, 2, { dollar, dollar },            true);
        }

        if (have_decode_error) {
            char data[] { '$', decode_error_byte, '*' };

            decode({ data + 1, 2 }, false,  0, 0, {},                            false);
            decode({ data + 1, 2 }, false,  1, 1, { replacement },               false);
            decode({ data + 1, 2 }, false,  2, 2, { replacement, star },         true);
            decode({ data + 1, 2 }, false, 10, 2, { replacement, star },         true);

            decode({ data + 0, 3 }, false,  0, 0, {},                            false);
            decode({ data + 0, 3 }, false,  1, 1, { dollar },                    false);
            decode({ data + 0, 3 }, false,  2, 2, { dollar, replacement },       false);
            decode({ data + 0, 3 }, false,  3, 3, { dollar, replacement, star }, true);
            decode({ data + 0, 3 }, false, 10, 3, { dollar, replacement, star }, true);
        }

        if (is_utf8) {
            // 2-byte char (cent)
            decode("\xC2\xA2",      false,  0, 0, {},                            false);
            decode("\xC2\xA2",      false,  1, 2, { 0xA2 },                      true);
            decode("\xC2\xA2",      false, 10, 2, { 0xA2 },                      true);

            // 3-byte char (euro)
            decode("\xE2\x82\xAC",  false,  0, 0, {},                            false);
            decode("\xE2\x82\xAC",  false,  1, 3, { 0x20AC },                    true);
            decode("\xE2\x82\xAC",  false, 10, 3, { 0x20AC },                    true);

            // Something followed by 2-byte char (cent)
            decode("$\xC2\xA2",     false,  0, 0, {},                            false);
            decode("$\xC2\xA2",     false,  1, 1, { 0x24 },                      false);
            decode("$\xC2\xA2",     false,  2, 3, { 0x24, 0xA2 },                true);
            decode("$\xC2\xA2",     false, 10, 3, { 0x24, 0xA2 },                true);

            // Only 1 byte of 2-byte char (cent)
            decode("\xC2",          false,  0, 0, {},                            false);
            decode("\xC2",          false,  1, 0, {},                            true);
            decode("\xC2",          false, 10, 0, {},                            true);
            decode("\xC2",          true,   0, 0, {},                            false);
            decode("\xC2",          true,   1, 1, { 0x3F },                      true);
            decode("\xC2",          true,  10, 1, { 0x3F },                      true);

            // Only 2 bytes of 3-byte char (euro)
            decode("\xE2\x82",      false,  0, 0, {},                            false);
            decode("\xE2\x82",      false,  1, 0, {},                            true);
            decode("\xE2\x82",      false, 10, 0, {},                            true);
            decode("\xE2\x82",      true,   0, 0, {},                            false);
            decode("\xE2\x82",      true,   1, 2, { 0x3F },                      true);
            decode("\xE2\x82",      true,  10, 2, { 0x3F },                      true);

            // Something followed by partial char
            decode("$\xC2",         false,  0, 0, {},                            false);
            decode("$\xC2",         false,  1, 1, { 0x24 },                      false);
            decode("$\xC2",         false,  2, 1, { 0x24 },                      true);
            decode("$\xC2",         false, 10, 1, { 0x24 },                      true);
            decode("$\xC2",         true,   0, 0, {},                            false);
            decode("$\xC2",         true,   1, 1, { 0x24 },                      false);
            decode("$\xC2",         true,   2, 2, { 0x24, 0x3F },                true);
            decode("$\xC2",         true,  10, 2, { 0x24, 0x3F },                true);

            // 1st byte of 1st char is bad
            decode("\xA2",          false,  0, 0, {},                            false);
            decode("\xA2",          false,  1, 0, {},                            true);
            decode("\xA2",          false, 10, 0, {},                            true);
            decode("\xA2",          true,   0, 0, {},                            false);
            decode("\xA2",          true,   1, 1, { 0x3F },                      true);
            decode("\xA2",          true,  10, 1, { 0x3F },                      true);

            // 2nd byte of 1st char (cent) is bad
            decode("\xC2$",         false,  0, 0, {},                            false);
            decode("\xC2$",         false,  1, 1, { 0x3F },                      false);
            decode("\xC2$",         false,  2, 2, { 0x3F, 0x24 },                true);
            decode("\xC2$",         false, 10, 2, { 0x3F, 0x24 },                true);

            // 3rd byte of 1st char (euro) is bad
            decode("\xE2\x82$",     false,  0, 0, {},                            false);
            decode("\xE2\x82$",     false,  1, 2, { 0x3F },                      false);
            decode("\xE2\x82$",     false,  2, 3, { 0x3F, 0x24 },                true);
            decode("\xE2\x82$",     false, 10, 3, { 0x3F, 0x24 },                true);

            // 1st byte of 2nd char is bad
            decode("$\xA2",         false,  0, 0, {},                            false);
            decode("$\xA2",         false,  1, 1, { 0x24 },                      false);
            decode("$\xA2",         false,  2, 1, { 0x24 },                      true);
            decode("$\xA2",         false, 10, 1, { 0x24 },                      true);
            decode("$\xA2",         true,   0, 0, {},                            false);
            decode("$\xA2",         true,   1, 1, { 0x24 },                      false);
            decode("$\xA2",         true,   2, 2, { 0x24, 0x3F },                true);
            decode("$\xA2",         true,  10, 2, { 0x24, 0x3F },                true);

            // 2nd byte of 2nd char (cent) is bad
            decode("$\xC2$",        false,  0, 0, {},                            false);
            decode("$\xC2$",        false,  1, 1, { 0x24 },                      false);
            decode("$\xC2$",        false,  2, 2, { 0x24, 0x3F },                false);
            decode("$\xC2$",        false,  3, 3, { 0x24, 0x3F, 0x24 },          true);
            decode("$\xC2$",        false, 10, 3, { 0x24, 0x3F, 0x24 },          true);
        }
    };

    for (const std::locale& locale : core::test::get_candidate_locales())
        subtest(locale);
}


ARCHON_TEST(Core_CharCodec_LenientEncode)
{
    using wide_traits_type = std::char_traits<wchar_t>;
    using wide_int_type = wide_traits_type::int_type;

    std::array<wchar_t, 64> seed_memory_1;
    std::array<char, 64> seed_memory_2;

    core::Buffer buffer_1(seed_memory_1);
    core::Buffer buffer_2(seed_memory_2);

    auto subtest = [&, &parent_test_context = test_context](const std::locale& locale) {
        ARCHON_TEST_TRAIL(parent_test_context, core::quoted(std::string_view(locale.name())));
        bool is_utf8 = (core::assume_utf8_locale(locale) && (core::assume_unicode_locale(locale) || ARCHON_WINDOWS));

        core::WideCharCodec::Config config;
        config.lenient = true;
        config.use_fallback_replacement_char = true;
        core::WideCharCodec codec(locale, config);

        auto encode = [&, &parent_test_context =
                       test_context](std::initializer_list<wide_int_type> input, std::size_t output_size,
                                     std::size_t expected_input_advance, std::string_view expected_output,
                                     bool expected_complete) {
            ARCHON_TEST_TRAIL(parent_test_context,
                              core::formatted_wrn("%s, %s, %s, %s, %s", core::as_sbr_list(input), output_size,
                                                  expected_input_advance, core::quoted(expected_output),
                                                  expected_complete));
            std::size_t buffer_offset = 0;
            for (wide_int_type val : input)
                buffer_1.append_a(wide_traits_type::to_char_type(val), buffer_offset);
            buffer_2.reserve(output_size);
            std::mbstate_t state = {};
            core::Span input_2 = { buffer_1.data(), input.size() };
            std::size_t input_offset = 0;
            core::Span output = { buffer_2.data(), output_size };
            std::size_t output_offset = 0;
            bool error = false;
            bool complete = codec.encode(state, input_2, input_offset, output, output_offset, error);
            ARCHON_CHECK_EQUAL(input_offset, expected_input_advance);
            std::string_view output_2 = { buffer_2.data(), output_offset };
            ARCHON_CHECK_EQUAL(output_2, expected_output);
            ARCHON_CHECK_EQUAL(complete, expected_complete);
            ARCHON_CHECK_NOT(error);
        };

        core::WideCharMapper char_mapper(locale);
        wide_int_type dollar = wide_traits_type::to_int_type(char_mapper.widen('$'));
        wide_int_type star   = wide_traits_type::to_int_type(char_mapper.widen('*'));

        wchar_t encode_error_char = {};
        bool have_encode_error = core::test::find_encode_error(locale, encode_error_char);
        {
            auto format = [&](std::ostream& out) {
                if (have_encode_error) {
                    wchar_t str_1[] = { encode_error_char };
                    std::wstring_view str_2 = { str_1, std::size(str_1) };
                    out << core::formatted("Yes (%s)", core::encoded_a<wchar_t>(core::quoted(str_2)));
                }
                else {
                    out << "No";
                }
            };
            test_context.logger.detail("Have encode error: %s", core::as_format_func(format));
        }

        if (true) {
            encode({},                      0, 0, "",               true);
            encode({},                     10, 0, "",               true);

            encode({ dollar },              0, 0, "",               false);
            encode({ dollar },              1, 1, "$",              true);
            encode({ dollar },             10, 1, "$",              true);

            encode({ dollar, star },        0, 0, "",               false);
            encode({ dollar, star },        1, 1, "$",              false);
            encode({ dollar, star },        2, 2, "$*",             true);
            encode({ dollar, star },       10, 2, "$*",             true);
        }

        if (have_encode_error) {
            wide_int_type bad = wide_traits_type::to_int_type(encode_error_char);

            encode({ bad },                 0, 0, "",               false);
            encode({ bad },                 1, 1, "?",              true);
            encode({ bad },                10, 1, "?",              true);

            encode({ bad, star },           0, 0, "",               false);
            encode({ bad, star },           1, 1, "?",              false);
            encode({ bad, star },           2, 2, "?*",             true);
            encode({ bad, star },          10, 2, "?*",             true);

            encode({ dollar, bad },         0, 0, "",               false);
            encode({ dollar, bad },         1, 1, "$",              false);
            encode({ dollar, bad },         2, 2, "$?",             true);
            encode({ dollar, bad },        10, 2, "$?",             true);

            encode({ dollar, bad, star },   0, 0, "",               false);
            encode({ dollar, bad, star },   1, 1, "$",              false);
            encode({ dollar, bad, star },   2, 2, "$?",             false);
            encode({ dollar, bad, star },   3, 3, "$?*",            true);
            encode({ dollar, bad, star },  10, 3, "$?*",            true);
        }

        if (is_utf8) {
            encode({ 0xA2 },                0, 0, "",               false);
            encode({ 0xA2 },                1, 0, "",               false);
            encode({ 0xA2 },                2, 1, "\xC2\xA2",       true);
            encode({ 0xA2 },               10, 1, "\xC2\xA2",       true);

            encode({ 0xA2, 0x2A },          0, 0, "",               false);
            encode({ 0xA2, 0x2A },          1, 0, "",               false);
            encode({ 0xA2, 0x2A },          2, 1, "\xC2\xA2",       false);
            encode({ 0xA2, 0x2A },          3, 2, "\xC2\xA2*",      true);
            encode({ 0xA2, 0x2A },         10, 2, "\xC2\xA2*",      true);

            encode({ 0x24, 0xA2 },          0, 0, "",               false);
            encode({ 0x24, 0xA2 },          1, 1, "$",              false);
            encode({ 0x24, 0xA2 },          2, 1, "$",              false);
            encode({ 0x24, 0xA2 },          3, 2, "$\xC2\xA2",      true);
            encode({ 0x24, 0xA2 },         10, 2, "$\xC2\xA2",      true);

            encode({ 0x24, 0xA2, 0x2A },    0, 0, "",               false);
            encode({ 0x24, 0xA2, 0x2A },    1, 1, "$",              false);
            encode({ 0x24, 0xA2, 0x2A },    2, 1, "$",              false);
            encode({ 0x24, 0xA2, 0x2A },    3, 2, "$\xC2\xA2",      false);
            encode({ 0x24, 0xA2, 0x2A },    4, 3, "$\xC2\xA2*",     true);
            encode({ 0x24, 0xA2, 0x2A },   10, 3, "$\xC2\xA2*",     true);

            encode({ 0x20AC },              0, 0, "",               false);
            encode({ 0x20AC },              1, 0, "",               false);
            encode({ 0x20AC },              2, 0, "",               false);
            encode({ 0x20AC },              3, 1, "\xE2\x82\xAC",   true);
            encode({ 0x20AC },             10, 1, "\xE2\x82\xAC",   true);

            encode({ 0x20AC, 0x2A },        0, 0, "",               false);
            encode({ 0x20AC, 0x2A },        1, 0, "",               false);
            encode({ 0x20AC, 0x2A },        2, 0, "",               false);
            encode({ 0x20AC, 0x2A },        3, 1, "\xE2\x82\xAC",   false);
            encode({ 0x20AC, 0x2A },        4, 2, "\xE2\x82\xAC*",  true);
            encode({ 0x20AC, 0x2A },       10, 2, "\xE2\x82\xAC*",  true);

            encode({ 0x24, 0x20AC },        0, 0, "",               false);
            encode({ 0x24, 0x20AC },        1, 1, "$",              false);
            encode({ 0x24, 0x20AC },        2, 1, "$",              false);
            encode({ 0x24, 0x20AC },        3, 1, "$",              false);
            encode({ 0x24, 0x20AC },        4, 2, "$\xE2\x82\xAC",  true);
            encode({ 0x24, 0x20AC },       10, 2, "$\xE2\x82\xAC",  true);

            encode({ 0x24, 0x20AC, 0x2A },  0, 0, "",               false);
            encode({ 0x24, 0x20AC, 0x2A },  1, 1, "$",              false);
            encode({ 0x24, 0x20AC, 0x2A },  2, 1, "$",              false);
            encode({ 0x24, 0x20AC, 0x2A },  3, 1, "$",              false);
            encode({ 0x24, 0x20AC, 0x2A },  4, 2, "$\xE2\x82\xAC",  false);
            encode({ 0x24, 0x20AC, 0x2A },  5, 3, "$\xE2\x82\xAC*", true);
            encode({ 0x24, 0x20AC, 0x2A }, 10, 3, "$\xE2\x82\xAC*", true);
        }
    };

    for (const std::locale& locale : core::test::get_candidate_locales())
        subtest(locale);
}
