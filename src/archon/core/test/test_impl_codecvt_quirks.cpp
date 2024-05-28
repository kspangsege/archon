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
#include <archon/core/impl/codecvt_quirks.hpp>
#include <archon/core/string_codec.hpp>
#include <archon/core/format.hpp>
#include <archon/core/format_encoded.hpp>
#include <archon/core/quote.hpp>
#include <archon/core/string_formatter.hpp>
#include <archon/core/locale.hpp>
#include <archon/check.hpp>
#include <archon/core/test/locale_utils.hpp>


using namespace archon;


ARCHON_TEST(Core_Impl_CodecvtQuirks)
{
    bool quirk_1 = core::impl::codecvt_quirk_ok_result_on_zero_size_buffer;
    bool quirk_2 = core::impl::codecvt_quirk_partial_result_on_partial_char;
    bool quirk_3 = core::impl::codecvt_quirk_partial_result_on_invalid_byte_seq;
    bool quirk_4 = core::impl::codecvt_quirk_consume_partial_char;
    bool quirk_5 = core::impl::codecvt_quirk_consume_partial_char_but_not_good_bytes_on_error;
    bool quirk_6 = (quirk_4 && !quirk_5);

    std::codecvt_base::result ok      = std::codecvt_base::ok;
    std::codecvt_base::result partial = std::codecvt_base::partial;
    std::codecvt_base::result error   = std::codecvt_base::error;
    std::codecvt_base::result result_1 = (quirk_1 ? ok : partial);
    std::codecvt_base::result result_2 = (quirk_2 ? partial : ok);
    std::codecvt_base::result result_3 = (quirk_3 ? partial : error);

    std::array<wchar_t, 32> seed_memory;
    core::WideStringFormatter formatter(seed_memory, test_context.locale);

    core::ArraySeededBuffer<wchar_t, 10> decode_buffer;
    core::ArraySeededBuffer<char, 20> encode_buffer;

    auto subtest = [&, &parent_test_context = test_context](const std::locale& locale) {
        ARCHON_TEST_TRAIL(parent_test_context, core::quoted(std::string_view(locale.name())));
        bool is_utf8 = (core::assume_utf8_locale(locale) && (core::assume_unicode_locale(locale) || ARCHON_WINDOWS));
        using codecvt_type = std::codecvt<wchar_t, char, std::mbstate_t>;
        const codecvt_type& codecvt = std::use_facet<codecvt_type>(locale);

        auto decode = [&, &parent_test_context =
                       test_context](std::string_view data, std::size_t split_pos,
                                     std::size_t buffer_size,
                                     std::size_t expected_from_advance,
                                     std::size_t expected_to_advance,
                                     std::codecvt_base::result expected_result) {
            ARCHON_TEST_TRAIL(parent_test_context,
                              core::formatted("decode(%s, %s, %s)", core::quoted(data), split_pos, buffer_size));
            decode_buffer.reserve(buffer_size + 1);
            std::mbstate_t state = {};
            const char* from = data.data();
            const char* from_end = from + data.size();
            const char* from_next = nullptr;
            wchar_t* to = decode_buffer.data();
            wchar_t* to_end = to + buffer_size;
            wchar_t* to_next = nullptr;
            if (split_pos > 0) {
                ARCHON_ASSERT(split_pos < data.size());
                std::codecvt_base::result result =
                    codecvt.in(state, from, from + split_pos, from_next, to, to_end + 1, to_next);
                if (!ARCHON_CHECK(result == ok || result == partial))
                    return;
                if (!ARCHON_CHECK(from_next >= from && from_next <= from_end))
                    return;
                if (!ARCHON_CHECK(to_next >= to && to_next <= to_end))
                    return;
                from = from_next;
                to   = to_next;
                from_next = nullptr;
                to_next   = nullptr;
            }
            std::codecvt_base::result result = codecvt.in(state, from, from_end, from_next, to, to_end, to_next);
            ARCHON_CHECK_EQUAL(result, expected_result);
            ARCHON_CHECK_EQUAL(from_next - from, expected_from_advance);
            ARCHON_CHECK_EQUAL(to_next - to, expected_to_advance);
        };

        char decode_error_byte = 0;
        bool have_decode_error = core::test::find_decode_error<wchar_t>(locale, decode_error_byte);

        if (true) {
            decode("",               0,  0, 0,                 0, ok);
            decode("",               0, 10, 0,                 0, ok);

            decode("$",              0,  0, 0,                 0, result_1);
            decode("$",              0,  1, 1,                 1, ok);
            decode("$",              0, 10, 1,                 1, ok);

            decode("$$",             0,  0, 0,                 0, result_1);
            decode("$$",             0,  1, 1,                 1, partial);
            decode("$$",             0,  2, 2,                 2, ok);
            decode("$$",             0, 10, 2,                 2, ok);
        }

        if (have_decode_error) {
            char data[] { '$', decode_error_byte };

            decode({ data + 1, 1 },  0,  0, 0,                 0, result_1);
            decode({ data + 1, 1 },  0,  1, 0,                 0, result_3);
            decode({ data + 1, 1 },  0, 10, 0,                 0, result_3);

            decode({ data + 0, 2 },  0,  0, 0,                 0, result_1);
            decode({ data + 0, 2 },  0,  1, 1,                 1, partial);
            decode({ data + 0, 2 },  0,  2, 1,                 1, result_3);
            decode({ data + 0, 2 },  0, 10, 1,                 1, result_3);
        }

        if (is_utf8) {
            // 2-byte char (cent)
            decode("\xC2\xA2",       0,  0, 0,                 0, result_1);
            decode("\xC2\xA2",       0,  1, 2,                 1, ok);
            decode("\xC2\xA2",       0, 10, 2,                 1, ok);
            decode("\xC2\xA2",       1,  0, 0,                 0, result_1);
            decode("\xC2\xA2",       1,  1, (quirk_4 ? 1 : 2), 1, ok);
            decode("\xC2\xA2",       1, 10, (quirk_4 ? 1 : 2), 1, ok);

            // 3-byte char (euro)
            decode("\xE2\x82\xAC",   0,  0, 0,                 0, result_1);
            decode("\xE2\x82\xAC",   0,  1, 3,                 1, ok);
            decode("\xE2\x82\xAC",   0, 10, 3,                 1, ok);
            decode("\xE2\x82\xAC",   1,  0, 0,                 0, result_1);
            decode("\xE2\x82\xAC",   1,  1, (quirk_4 ? 2 : 3), 1, ok);
            decode("\xE2\x82\xAC",   1, 10, (quirk_4 ? 2 : 3), 1, ok);
            decode("\xE2\x82\xAC",   2,  0, 0,                 0, result_1);
            decode("\xE2\x82\xAC",   2,  1, (quirk_4 ? 1 : 3), 1, ok);
            decode("\xE2\x82\xAC",   2, 10, (quirk_4 ? 1 : 3), 1, ok);

            // Something followed by 2-byte char (cent)
            decode("$\xC2\xA2",      0,  0, 0,                 0, result_1);
            decode("$\xC2\xA2",      0,  1, 1,                 1, partial);
            decode("$\xC2\xA2",      0,  2, 3,                 2, ok);
            decode("$\xC2\xA2",      0, 10, 3,                 2, ok);

            // Only 1 byte of 2-byte char (cent)
            decode("\xC2",           0,  0, 0,                 0, result_1);
            decode("\xC2",           0,  1, (quirk_4 ? 1 : 0), 0, result_2);
            decode("\xC2",           0, 10, (quirk_4 ? 1 : 0), 0, result_2);

            // Only 2 bytes of 3-byte char (euro)
            decode("\xE2\x82",       0,  0, 0,                 0, result_1);
            decode("\xE2\x82",       0,  1, (quirk_4 ? 2 : 0), 0, result_2);
            decode("\xE2\x82",       0, 10, (quirk_4 ? 2 : 0), 0, result_2);
            decode("\xE2\x82",       1,  0, 0,                 0, result_1);
            decode("\xE2\x82",       1,  1, (quirk_4 ? 1 : 0), 0, result_2);
            decode("\xE2\x82",       1, 10, (quirk_4 ? 1 : 0), 0, result_2);

            // Something followed by partial char
            decode("$\xC2",          0,  0, 0,                 0, result_1);
            decode("$\xC2",          0,  1, 1,                 1, partial);
            decode("$\xC2",          0,  2, (quirk_4 ? 2 : 1), 1, result_2);
            decode("$\xC2",          0, 10, (quirk_4 ? 2 : 1), 1, result_2);

            // 1st byte of 1st char is bad
            decode("\xA2",           0,  0, 0,                 0, result_1);
            decode("\xA2",           0,  1, 0,                 0, result_3);
            decode("\xA2",           0, 10, 0,                 0, result_3);

            // 2nd byte of 1st char (cent) is bad
            decode("\xC2$",          0,  0, 0,                 0, result_1);
            decode("\xC2$",          0,  1, (quirk_6 ? 1 : 0), 0, result_3);
            decode("\xC2$",          0, 10, (quirk_6 ? 1 : 0), 0, result_3);
            decode("\xC2$",          1,  0, 0,                 0, result_1);
            decode("\xC2$",          1,  1, 0,                 0, result_3);
            decode("\xC2$",          1, 10, 0,                 0, result_3);

            // 3rd byte of 1st char (euro) is bad
            decode("\xE2\x82$",      0,  0, 0,                 0, result_1);
            decode("\xE2\x82$",      0,  1, (quirk_6 ? 2 : 0), 0, result_3);
            decode("\xE2\x82$",      0, 10, (quirk_6 ? 2 : 0), 0, result_3);
            decode("\xE2\x82$",      1,  0, 0,                 0, result_1);
            decode("\xE2\x82$",      1,  1, 0,                 0, result_3);
            decode("\xE2\x82$",      1, 10, 0,                 0, result_3);

            // 1st byte of 2nd char is bad
            decode("$\xA2",          0,  0, 0,                 0, result_1);
            decode("$\xA2",          0,  1, 1,                 1, partial);
            decode("$\xA2",          0,  2, 1,                 1, result_3);
            decode("$\xA2",          0, 10, 1,                 1, result_3);

            // 2nd byte of 2nd char (cent) is bad
            decode("$\xC2$",         0,  0, 0,                 0, result_1);
            decode("$\xC2$",         0,  1, 1,                 1, partial);
            decode("$\xC2$",         0,  2, (quirk_6 ? 2 : 1), 1, result_3);
            decode("$\xC2$",         0, 10, (quirk_6 ? 2 : 1), 1, result_3);
        }

        auto encode = [&, &parent_test_context = test_context](core::Span<const wchar_t> data, std::size_t buffer_size,
                                                               std::size_t expected_from_advance,
                                                               std::size_t expected_to_advance,
                                                               std::codecvt_base::result expected_result) {
            std::wstring_view data_2(data.data(), data.size());
            std::wstring_view segement = formatter.format("encode(%s, %s)", core::quoted(data_2), buffer_size);
            ARCHON_TEST_TRAIL(parent_test_context, core::encoded(segement));
            encode_buffer.reserve(buffer_size);
            std::mbstate_t state = {};
            const wchar_t* from = data.data();
            const wchar_t* from_end = from + data.size();
            const wchar_t* from_next = nullptr;
            char* to = encode_buffer.data();
            char* to_end = to + buffer_size;
            char* to_next = nullptr;
            std::codecvt_base::result result = codecvt.out(state, from, from_end, from_next, to, to_end, to_next);
            ARCHON_CHECK_EQUAL(result, expected_result);
            ARCHON_CHECK_EQUAL(from_next - from, expected_from_advance);
            ARCHON_CHECK_EQUAL(to_next - to, expected_to_advance);
        };

        using traits_type = std::char_traits<wchar_t>;
        wchar_t dollar = traits_type::to_char_type(0x24);

        wchar_t encode_error_char = 0;
        bool have_encode_error = core::test::find_encode_error(locale, encode_error_char);

        if (true) {
            encode(std::array<wchar_t, 0> {},      0, 0, 0, ok);
            encode(std::array<wchar_t, 0> {},     10, 0, 0, ok);

            encode(std::array { dollar },          0, 0, 0, result_1);
            encode(std::array { dollar },          1, 1, 1, ok);
            encode(std::array { dollar },         10, 1, 1, ok);

            encode(std::array { dollar, dollar },  0, 0, 0, result_1);
            encode(std::array { dollar, dollar },  1, 1, 1, partial);
            encode(std::array { dollar, dollar },  2, 2, 2, ok);
            encode(std::array { dollar, dollar }, 10, 2, 2, ok);
        }

        if (have_encode_error) {
            wchar_t err = encode_error_char;

            encode(std::array { err },             0, 0, 0, result_1);
            encode(std::array { err },             1, 0, 0, error);
            encode(std::array { err },            10, 0, 0, error);

            encode(std::array { dollar, err },     0, 0, 0, result_1);
            encode(std::array { dollar, err },     1, 1, 1, partial);
            encode(std::array { dollar, err },     2, 1, 1, error);
            encode(std::array { dollar, err },    10, 1, 1, error);
        }

        if (is_utf8) {
            wchar_t cent = traits_type::to_char_type(0x00A2);
            wchar_t euro = traits_type::to_char_type(0x20AC);

            encode(std::array { cent },            0, 0, 0, result_1);
            encode(std::array { cent },            1, 0, 0, partial);
            encode(std::array { cent },            2, 1, 2, ok);
            encode(std::array { cent },           10, 1, 2, ok);

            encode(std::array { dollar, cent },    0, 0, 0, result_1);
            encode(std::array { dollar, cent },    1, 1, 1, partial);
            encode(std::array { dollar, cent },    2, 1, 1, partial);
            encode(std::array { dollar, cent },    3, 2, 3, ok);
            encode(std::array { dollar, cent },   10, 2, 3, ok);

            encode(std::array { euro },            0, 0, 0, result_1);
            encode(std::array { euro },            1, 0, 0, partial);
            encode(std::array { euro },            2, 0, 0, partial);
            encode(std::array { euro },            3, 1, 3, ok);
            encode(std::array { euro },           10, 1, 3, ok);

            encode(std::array { dollar, euro },    0, 0, 0, result_1);
            encode(std::array { dollar, euro },    1, 1, 1, partial);
            encode(std::array { dollar, euro },    2, 1, 1, partial);
            encode(std::array { dollar, euro },    3, 1, 1, partial);
            encode(std::array { dollar, euro },    4, 2, 4, ok);
            encode(std::array { dollar, euro },   10, 2, 4, ok);
        }

        auto simul_decode = [&, &parent_test_context = test_context](std::string_view data, std::size_t split_pos,
                                                                     std::size_t buffer_size,
                                                                     std::size_t expected_from_advance) {
            ARCHON_TEST_TRAIL(parent_test_context,
                              core::formatted("simul_decode(%s, %s, %s)", core::quoted(data), split_pos, buffer_size));
            decode_buffer.reserve(buffer_size + 1);
            std::mbstate_t state = {};
            const char* from = data.data();
            const char* from_end = from + data.size();
            if (split_pos > 0) {
                ARCHON_ASSERT(split_pos < data.size());
                const char* from_next = nullptr;
                wchar_t* to = decode_buffer.data();
                wchar_t* to_end = to + buffer_size;
                wchar_t* to_next = nullptr;
                std::codecvt_base::result result =
                    codecvt.in(state, from, from + split_pos, from_next, to, to_end + 1, to_next);
                if (!ARCHON_CHECK(result == ok || result == partial))
                    return;
                if (!ARCHON_CHECK(from_next >= from && from_next <= from_end))
                    return;
                if (!ARCHON_CHECK(to_next >= to && to_next <= to_end))
                    return;
                from = from_next;
            }
            int from_advance = codecvt.length(state, from, from_end, buffer_size);
            ARCHON_CHECK_EQUAL(from_advance, expected_from_advance);
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
            simul_decode("\xC2\xA2\xC2\xA2$",         1, 1, (quirk_4 ? 1 : 2));
            simul_decode("\xC2\xA2\xC2\xA2$",         1, 2, (quirk_4 ? 3 : 4));

            // Two 3-byte chars (euro)
            simul_decode("\xE2\x82\xAC\xE2\x82\xAC$", 0, 0, 0);
            simul_decode("\xE2\x82\xAC\xE2\x82\xAC$", 0, 1, 3);
            simul_decode("\xE2\x82\xAC\xE2\x82\xAC$", 0, 2, 6);
            simul_decode("\xE2\x82\xAC\xE2\x82\xAC$", 1, 0, 0);
            simul_decode("\xE2\x82\xAC\xE2\x82\xAC$", 1, 1, (quirk_4 ? 2 : 3));
            simul_decode("\xE2\x82\xAC\xE2\x82\xAC$", 1, 2, (quirk_4 ? 5 : 6));
            simul_decode("\xE2\x82\xAC\xE2\x82\xAC$", 2, 0, 0);
            simul_decode("\xE2\x82\xAC\xE2\x82\xAC$", 2, 1, (quirk_4 ? 1 : 3));
            simul_decode("\xE2\x82\xAC\xE2\x82\xAC$", 2, 2, (quirk_4 ? 4 : 6));
        }
    };

    for (const std::locale& locale : core::test::get_candidate_locales())
        subtest(locale);
}
