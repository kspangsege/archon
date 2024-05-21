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
#include <initializer_list>
#include <array>
#include <string>

#include <archon/core/span.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/unicode.hpp>
#include <archon/core/with_modified_locale.hpp>
#include <archon/core/format.hpp>
#include <archon/core/as_list.hpp>
#include <archon/core/enum.hpp>
#include <archon/check.hpp>


using namespace archon;


namespace {

enum class Result { in_exhausted, error, out_exhausted, in_exhausted_or_error };

struct ResultSpec {
    static constexpr archon::core::EnumAssoc map[] = {
        { int(Result::in_exhausted),          "in_exhausted"          },
        { int(Result::error),                 "error"                 },
        { int(Result::out_exhausted),         "out_exhausted"         },
        { int(Result::in_exhausted_or_error), "in_exhausted_or_error" },
    };
};

using ResultEnum = core::Enum<Result, ResultSpec>;

} // unnamed namespace


ARCHON_TEST(Core_Unicode_EncodeUtf8Incr)
{
    using char_type_1 = char32_t;
    using char_type_2 = char;

    using traits_type_1 = std::char_traits<char_type_1>;
    using traits_type_2 = std::char_traits<char_type_2>;

    using int_type_1 = traits_type_1::int_type;
    using int_type_2 = traits_type_2::int_type;

    std::array<char_type_1, 64> seed_memory_1;
    std::array<char_type_2, 64> seed_memory_2;

    core::Buffer buffer_1(seed_memory_1);
    core::Buffer buffer_2(seed_memory_2);

    auto test = [&, &parent_test_context = test_context](std::initializer_list<int_type_1> in, std::size_t out_size,
                                                         std::size_t expected_in_size,
                                                         std::initializer_list<int_type_2> expected_out,
                                                         Result expected_result) {
        ARCHON_TEST_TRAIL(parent_test_context,
                          core::formatted_wrn("%s, %s, %s, %s, %s", core::as_sbr_list(in), out_size, expected_in_size,
                                              core::as_sbr_list(expected_out), ResultEnum(expected_result)));
        std::size_t offset_1 = 0;
        for (int_type_1 val : in)
            buffer_1.append_a(traits_type_1::to_char_type(val), offset_1);
        std::size_t offset_2 = 0;
        for (int_type_2 val : expected_out)
            buffer_2.append_a(traits_type_2::to_char_type(val), offset_2);
        buffer_2.reserve_extra(out_size, offset_2);
        core::Span in_2 = { buffer_1.data(), offset_1 };
        core::Span expected_out_2 = { buffer_2.data(), offset_2 };
        core::Span out = { buffer_2.data() + offset_2, out_size };
        std::size_t in_offset = 0;
        std::size_t out_offset = 0;
        bool in_exhausted = {};
        bool error = {};
        core::encode_utf8_incr<char_type_1, char_type_2,
                               traits_type_1, traits_type_2>(in_2, out, in_offset, out_offset, in_exhausted, error);
        ARCHON_CHECK_EQUAL(in_offset, expected_in_size);
        ARCHON_CHECK_EQUAL_SEQ(out.subspan(0, out_offset), expected_out_2);
        switch (expected_result) {
            case Result::in_exhausted:
                ARCHON_CHECK(in_exhausted);
                break;
            case Result::error:
                if (ARCHON_LIKELY(ARCHON_CHECK_NOT(in_exhausted)))
                    ARCHON_CHECK(error);
                break;
            case Result::out_exhausted:
                if (ARCHON_LIKELY(ARCHON_CHECK_NOT(in_exhausted)))
                    ARCHON_CHECK_NOT(error);
                break;
            case Result::in_exhausted_or_error:
                ARCHON_CHECK(in_exhausted || error);
                break;
        }
    };

    // Empty
    test({}, 0, 0, {}, Result::in_exhausted);
    test({}, 1, 0, {}, Result::in_exhausted);

    // 1-byte form (dollar): U+0024 --> 0x24
    test({ 0x24 }, 0, 0, {}, Result::out_exhausted);
    test({ 0x24 }, 1, 1, { 0x24 }, Result::in_exhausted);
    test({ 0x24 }, 2, 1, { 0x24 }, Result::in_exhausted);
    test({ 0x2A, 0x24 }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0x24 }, 1, 1, { 0x2A }, Result::out_exhausted);
    test({ 0x2A, 0x24 }, 2, 2, { 0x2A, 0x24 }, Result::in_exhausted);
    test({ 0x2A, 0x24 }, 3, 2, { 0x2A, 0x24 }, Result::in_exhausted);
    test({ 0x24, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x24, 0x2B }, 1, 1, { 0x24 }, Result::out_exhausted);
    test({ 0x24, 0x2B }, 2, 2, { 0x24, 0x2B }, Result::in_exhausted);
    test({ 0x24, 0x2B }, 3, 2, { 0x24, 0x2B }, Result::in_exhausted);
    test({ 0x2A, 0x24, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0x24, 0x2B }, 1, 1, { 0x2A }, Result::out_exhausted);
    test({ 0x2A, 0x24, 0x2B }, 2, 2, { 0x2A, 0x24 }, Result::out_exhausted);
    test({ 0x2A, 0x24, 0x2B }, 3, 3, { 0x2A, 0x24, 0x2B }, Result::in_exhausted);
    test({ 0x2A, 0x24, 0x2B }, 4, 3, { 0x2A, 0x24, 0x2B }, Result::in_exhausted);

    // 2-byte form (cent): U+00A2 --> 0xC2, 0xA2
    test({ 0xA2 }, 0, 0, {}, Result::out_exhausted);
    test({ 0xA2 }, 1, 0, {}, Result::out_exhausted);
    test({ 0xA2 }, 2, 1, { 0xC2, 0xA2 }, Result::in_exhausted);
    test({ 0xA2 }, 3, 1, { 0xC2, 0xA2 }, Result::in_exhausted);
    test({ 0x2A, 0xA2 }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xA2 }, 1, 1, { 0x2A }, Result::out_exhausted);
    test({ 0x2A, 0xA2 }, 2, 1, { 0x2A }, Result::out_exhausted);
    test({ 0x2A, 0xA2 }, 3, 2, { 0x2A, 0xC2, 0xA2 }, Result::in_exhausted);
    test({ 0x2A, 0xA2 }, 4, 2, { 0x2A, 0xC2, 0xA2 }, Result::in_exhausted);
    test({ 0xA2, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0xA2, 0x2B }, 1, 0, {}, Result::out_exhausted);
    test({ 0xA2, 0x2B }, 2, 1, { 0xC2, 0xA2 }, Result::out_exhausted);
    test({ 0xA2, 0x2B }, 3, 2, { 0xC2, 0xA2, 0x2B }, Result::in_exhausted);
    test({ 0xA2, 0x2B }, 4, 2, { 0xC2, 0xA2, 0x2B }, Result::in_exhausted);
    test({ 0x2A, 0xA2, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xA2, 0x2B }, 1, 1, { 0x2A }, Result::out_exhausted);
    test({ 0x2A, 0xA2, 0x2B }, 2, 1, { 0x2A }, Result::out_exhausted);
    test({ 0x2A, 0xA2, 0x2B }, 3, 2, { 0x2A, 0xC2, 0xA2 }, Result::out_exhausted);
    test({ 0x2A, 0xA2, 0x2B }, 4, 3, { 0x2A, 0xC2, 0xA2, 0x2B }, Result::in_exhausted);
    test({ 0x2A, 0xA2, 0x2B }, 5, 3, { 0x2A, 0xC2, 0xA2, 0x2B }, Result::in_exhausted);

    // 3-byte form (euro): U+20AC --> 0xE2, 0x82, 0xAC
    test({ 0x20AC }, 0, 0, {}, Result::out_exhausted);
    test({ 0x20AC }, 1, 0, {}, Result::out_exhausted);
    test({ 0x20AC }, 2, 0, {}, Result::out_exhausted);
    test({ 0x20AC }, 3, 1, { 0xE2, 0x82, 0xAC }, Result::in_exhausted);
    test({ 0x20AC }, 4, 1, { 0xE2, 0x82, 0xAC }, Result::in_exhausted);
    test({ 0x2A, 0x20AC }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0x20AC }, 1, 1, { 0x2A }, Result::out_exhausted);
    test({ 0x2A, 0x20AC }, 2, 1, { 0x2A }, Result::out_exhausted);
    test({ 0x2A, 0x20AC }, 3, 1, { 0x2A }, Result::out_exhausted);
    test({ 0x2A, 0x20AC }, 4, 2, { 0x2A, 0xE2, 0x82, 0xAC }, Result::in_exhausted);
    test({ 0x2A, 0x20AC }, 5, 2, { 0x2A, 0xE2, 0x82, 0xAC }, Result::in_exhausted);
    test({ 0x20AC, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x20AC, 0x2B }, 1, 0, {}, Result::out_exhausted);
    test({ 0x20AC, 0x2B }, 2, 0, {}, Result::out_exhausted);
    test({ 0x20AC, 0x2B }, 3, 1, { 0xE2, 0x82, 0xAC }, Result::out_exhausted);
    test({ 0x20AC, 0x2B }, 4, 2, { 0xE2, 0x82, 0xAC, 0x2B }, Result::in_exhausted);
    test({ 0x20AC, 0x2B }, 5, 2, { 0xE2, 0x82, 0xAC, 0x2B }, Result::in_exhausted);
    test({ 0x2A, 0x20AC, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0x20AC, 0x2B }, 1, 1, { 0x2A }, Result::out_exhausted);
    test({ 0x2A, 0x20AC, 0x2B }, 2, 1, { 0x2A }, Result::out_exhausted);
    test({ 0x2A, 0x20AC, 0x2B }, 3, 1, { 0x2A }, Result::out_exhausted);
    test({ 0x2A, 0x20AC, 0x2B }, 4, 2, { 0x2A, 0xE2, 0x82, 0xAC }, Result::out_exhausted);
    test({ 0x2A, 0x20AC, 0x2B }, 5, 3, { 0x2A, 0xE2, 0x82, 0xAC, 0x2B }, Result::in_exhausted);
    test({ 0x2A, 0x20AC, 0x2B }, 6, 3, { 0x2A, 0xE2, 0x82, 0xAC, 0x2B }, Result::in_exhausted);

    // 4-byte form (hwair): U+10348 --> 0xF0, 0x90, 0x8D, 0x88
    test({ 0x10348 }, 0, 0, {}, Result::out_exhausted);
    test({ 0x10348 }, 1, 0, {}, Result::out_exhausted);
    test({ 0x10348 }, 2, 0, {}, Result::out_exhausted);
    test({ 0x10348 }, 3, 0, {}, Result::out_exhausted);
    test({ 0x10348 }, 4, 1, { 0xF0, 0x90, 0x8D, 0x88 }, Result::in_exhausted);
    test({ 0x10348 }, 5, 1, { 0xF0, 0x90, 0x8D, 0x88 }, Result::in_exhausted);
    test({ 0x2A, 0x10348 }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0x10348 }, 1, 1, { 0x2A }, Result::out_exhausted);
    test({ 0x2A, 0x10348 }, 2, 1, { 0x2A }, Result::out_exhausted);
    test({ 0x2A, 0x10348 }, 3, 1, { 0x2A }, Result::out_exhausted);
    test({ 0x2A, 0x10348 }, 4, 1, { 0x2A }, Result::out_exhausted);
    test({ 0x2A, 0x10348 }, 5, 2, { 0x2A, 0xF0, 0x90, 0x8D, 0x88 }, Result::in_exhausted);
    test({ 0x2A, 0x10348 }, 6, 2, { 0x2A, 0xF0, 0x90, 0x8D, 0x88 }, Result::in_exhausted);
    test({ 0x10348, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x10348, 0x2B }, 1, 0, {}, Result::out_exhausted);
    test({ 0x10348, 0x2B }, 2, 0, {}, Result::out_exhausted);
    test({ 0x10348, 0x2B }, 3, 0, {}, Result::out_exhausted);
    test({ 0x10348, 0x2B }, 4, 1, { 0xF0, 0x90, 0x8D, 0x88 }, Result::out_exhausted);
    test({ 0x10348, 0x2B }, 5, 2, { 0xF0, 0x90, 0x8D, 0x88, 0x2B }, Result::in_exhausted);
    test({ 0x10348, 0x2B }, 6, 2, { 0xF0, 0x90, 0x8D, 0x88, 0x2B }, Result::in_exhausted);
    test({ 0x2A, 0x10348, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0x10348, 0x2B }, 1, 1, { 0x2A }, Result::out_exhausted);
    test({ 0x2A, 0x10348, 0x2B }, 2, 1, { 0x2A }, Result::out_exhausted);
    test({ 0x2A, 0x10348, 0x2B }, 3, 1, { 0x2A }, Result::out_exhausted);
    test({ 0x2A, 0x10348, 0x2B }, 4, 1, { 0x2A }, Result::out_exhausted);
    test({ 0x2A, 0x10348, 0x2B }, 5, 2, { 0x2A, 0xF0, 0x90, 0x8D, 0x88 }, Result::out_exhausted);
    test({ 0x2A, 0x10348, 0x2B }, 6, 3, { 0x2A, 0xF0, 0x90, 0x8D, 0x88, 0x2B }, Result::in_exhausted);
    test({ 0x2A, 0x10348, 0x2B }, 7, 3, { 0x2A, 0xF0, 0x90, 0x8D, 0x88, 0x2B }, Result::in_exhausted);

    // Invalid: Code point in surrogate range: U+D821
    test({ 0xD821 }, 0, 0, {}, Result::error);
    test({ 0xD821 }, 1, 0, {}, Result::error);
    test({ 0xD821 }, 2, 0, {}, Result::error);
    test({ 0xD821 }, 3, 0, {}, Result::error);
    test({ 0xD821 }, 4, 0, {}, Result::error);
    test({ 0xD821 }, 5, 0, {}, Result::error);
    test({ 0x2A, 0xD821 }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xD821 }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xD821 }, 2, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xD821 }, 3, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xD821 }, 4, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xD821 }, 5, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xD821 }, 6, 1, { 0x2A }, Result::error);
    test({ 0xD821, 0x2B }, 0, 0, {}, Result::error);
    test({ 0xD821, 0x2B }, 1, 0, {}, Result::error);
    test({ 0xD821, 0x2B }, 2, 0, {}, Result::error);
    test({ 0xD821, 0x2B }, 3, 0, {}, Result::error);
    test({ 0xD821, 0x2B }, 4, 0, {}, Result::error);
    test({ 0xD821, 0x2B }, 5, 0, {}, Result::error);
    test({ 0x2A, 0xD821, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xD821, 0x2B }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xD821, 0x2B }, 2, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xD821, 0x2B }, 3, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xD821, 0x2B }, 4, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xD821, 0x2B }, 5, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xD821, 0x2B }, 6, 1, { 0x2A }, Result::error);

    // Invalid: Non-character code point: U+FFFE
    test({ 0xFFFE }, 0, 0, {}, Result::error);
    test({ 0xFFFE }, 1, 0, {}, Result::error);
    test({ 0xFFFE }, 2, 0, {}, Result::error);
    test({ 0xFFFE }, 3, 0, {}, Result::error);
    test({ 0xFFFE }, 4, 0, {}, Result::error);
    test({ 0xFFFE }, 5, 0, {}, Result::error);
    test({ 0x2A, 0xFFFE }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xFFFE }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xFFFE }, 2, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xFFFE }, 3, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xFFFE }, 4, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xFFFE }, 5, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xFFFE }, 6, 1, { 0x2A }, Result::error);
    test({ 0xFFFE, 0x2B }, 0, 0, {}, Result::error);
    test({ 0xFFFE, 0x2B }, 1, 0, {}, Result::error);
    test({ 0xFFFE, 0x2B }, 2, 0, {}, Result::error);
    test({ 0xFFFE, 0x2B }, 3, 0, {}, Result::error);
    test({ 0xFFFE, 0x2B }, 4, 0, {}, Result::error);
    test({ 0xFFFE, 0x2B }, 5, 0, {}, Result::error);
    test({ 0x2A, 0xFFFE, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xFFFE, 0x2B }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xFFFE, 0x2B }, 2, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xFFFE, 0x2B }, 3, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xFFFE, 0x2B }, 4, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xFFFE, 0x2B }, 5, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xFFFE, 0x2B }, 6, 1, { 0x2A }, Result::error);

    // Invalid: Overlarge code point: U+110021
    test({ 0x110021 }, 0, 0, {}, Result::error);
    test({ 0x110021 }, 1, 0, {}, Result::error);
    test({ 0x110021 }, 2, 0, {}, Result::error);
    test({ 0x110021 }, 3, 0, {}, Result::error);
    test({ 0x110021 }, 4, 0, {}, Result::error);
    test({ 0x110021 }, 5, 0, {}, Result::error);
    test({ 0x2A, 0x110021 }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0x110021 }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0x110021 }, 2, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0x110021 }, 3, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0x110021 }, 4, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0x110021 }, 5, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0x110021 }, 6, 1, { 0x2A }, Result::error);
    test({ 0x110021, 0x2B }, 0, 0, {}, Result::error);
    test({ 0x110021, 0x2B }, 1, 0, {}, Result::error);
    test({ 0x110021, 0x2B }, 2, 0, {}, Result::error);
    test({ 0x110021, 0x2B }, 3, 0, {}, Result::error);
    test({ 0x110021, 0x2B }, 4, 0, {}, Result::error);
    test({ 0x110021, 0x2B }, 5, 0, {}, Result::error);
    test({ 0x2A, 0x110021, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0x110021, 0x2B }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0x110021, 0x2B }, 2, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0x110021, 0x2B }, 3, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0x110021, 0x2B }, 4, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0x110021, 0x2B }, 5, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0x110021, 0x2B }, 6, 1, { 0x2A }, Result::error);
}


ARCHON_TEST(Core_Unicode_DecodeUtf8Incr)
{
    using char_type_1 = char;
    using char_type_2 = char32_t;

    using traits_type_1 = std::char_traits<char_type_1>;
    using traits_type_2 = std::char_traits<char_type_2>;

    using int_type_1 = traits_type_1::int_type;
    using int_type_2 = traits_type_2::int_type;

    std::array<char_type_1, 64> seed_memory_1;
    std::array<char_type_2, 64> seed_memory_2;

    core::Buffer buffer_1(seed_memory_1);
    core::Buffer buffer_2(seed_memory_2);

    auto test = [&, &parent_test_context = test_context](std::initializer_list<int_type_1> in, std::size_t out_size,
                                                         std::size_t expected_in_size,
                                                         std::initializer_list<int_type_2> expected_out,
                                                         Result expected_result) {
        ARCHON_TEST_TRAIL(parent_test_context,
                          core::formatted_wrn("%s, %s, %s, %s, %s", core::as_sbr_list(in), out_size, expected_in_size,
                                              core::as_sbr_list(expected_out), ResultEnum(expected_result)));
        std::size_t offset_1 = 0;
        for (int_type_1 val : in)
            buffer_1.append_a(traits_type_1::to_char_type(val), offset_1);
        std::size_t offset_2 = 0;
        for (int_type_2 val : expected_out)
            buffer_2.append_a(traits_type_2::to_char_type(val), offset_2);
        buffer_2.reserve_extra(out_size, offset_2);
        core::Span in_2 = { buffer_1.data(), offset_1 };
        core::Span expected_out_2 = { buffer_2.data(), offset_2 };
        core::Span out = { buffer_2.data() + offset_2, out_size };
        std::size_t in_offset = 0;
        std::size_t out_offset = 0;
        bool in_exhausted = {};
        bool error = {};
        core::decode_utf8_incr<char_type_1, char_type_2,
                               traits_type_1, traits_type_2>(in_2, out, in_offset, out_offset, in_exhausted, error);
        ARCHON_CHECK_EQUAL(in_offset, expected_in_size);
        ARCHON_CHECK_EQUAL_SEQ(out.subspan(0, out_offset), expected_out_2);
        switch (expected_result) {
            case Result::in_exhausted:
                ARCHON_CHECK(in_exhausted);
                break;
            case Result::error:
                if (ARCHON_LIKELY(ARCHON_CHECK_NOT(in_exhausted)))
                    ARCHON_CHECK(error);
                break;
            case Result::out_exhausted:
                if (ARCHON_LIKELY(ARCHON_CHECK_NOT(in_exhausted)))
                    ARCHON_CHECK_NOT(error);
                break;
            case Result::in_exhausted_or_error:
                ARCHON_CHECK(in_exhausted || error);
                break;
        }
    };

    // Empty
    test({}, 0, 0, {}, Result::in_exhausted);
    test({}, 1, 0, {}, Result::in_exhausted);

    // Valid 1-byte form (dollar): 0x24 --> U+0024
    test({ 0x24 }, 0, 0, {}, Result::out_exhausted);
    test({ 0x24 }, 1, 1, { 0x24 }, Result::in_exhausted);
    test({ 0x24 }, 2, 1, { 0x24 }, Result::in_exhausted);
    test({ 0x2A, 0x24 }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0x24 }, 1, 1, { 0x2A }, Result::out_exhausted);
    test({ 0x2A, 0x24 }, 2, 2, { 0x2A, 0x24 }, Result::in_exhausted);
    test({ 0x2A, 0x24 }, 3, 2, { 0x2A, 0x24 }, Result::in_exhausted);

    // Valid 2-byte form (cent): 0xC2, 0xA2 --> U+00A2
    test({ 0xC2 }, 0, 0, {}, Result::in_exhausted);
    test({ 0xC2 }, 1, 0, {}, Result::in_exhausted);
    test({ 0xC2 }, 2, 0, {}, Result::in_exhausted);
    test({ 0xC2, 0xA2 }, 0, 0, {}, Result::out_exhausted);
    test({ 0xC2, 0xA2 }, 1, 2, { 0xA2 }, Result::in_exhausted);
    test({ 0xC2, 0xA2 }, 2, 2, { 0xA2 }, Result::in_exhausted);
    test({ 0x2A, 0xC2, 0xA2 }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xC2, 0xA2 }, 1, 1, { 0x2A }, Result::out_exhausted);
    test({ 0x2A, 0xC2, 0xA2 }, 2, 3, { 0x2A, 0xA2 }, Result::in_exhausted);
    test({ 0x2A, 0xC2, 0xA2 }, 3, 3, { 0x2A, 0xA2 }, Result::in_exhausted);

    // Valid 3-byte form (euro): 0xE2, 0x82, 0xAC --> U+20AC
    test({ 0xE2 }, 0, 0, {}, Result::in_exhausted);
    test({ 0xE2 }, 1, 0, {}, Result::in_exhausted);
    test({ 0xE2 }, 2, 0, {}, Result::in_exhausted);
    test({ 0xE2, 0x82 }, 0, 0, {}, Result::in_exhausted);
    test({ 0xE2, 0x82 }, 1, 0, {}, Result::in_exhausted);
    test({ 0xE2, 0x82 }, 2, 0, {}, Result::in_exhausted);
    test({ 0xE2, 0x82, 0xAC }, 0, 0, {}, Result::out_exhausted);
    test({ 0xE2, 0x82, 0xAC }, 1, 3, { 0x20AC }, Result::in_exhausted);
    test({ 0xE2, 0x82, 0xAC }, 2, 3, { 0x20AC }, Result::in_exhausted);
    test({ 0x2A, 0xE2, 0x82, 0xAC }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xE2, 0x82, 0xAC }, 1, 1, { 0x2A }, Result::out_exhausted);
    test({ 0x2A, 0xE2, 0x82, 0xAC }, 2, 4, { 0x2A, 0x20AC }, Result::in_exhausted);
    test({ 0x2A, 0xE2, 0x82, 0xAC }, 3, 4, { 0x2A, 0x20AC }, Result::in_exhausted);

    // Valid 4-byte form (hwair): 0xF0, 0x90, 0x8D, 0x88 --> U+10348
    test({ 0xF0 }, 0, 0, {}, Result::in_exhausted);
    test({ 0xF0 }, 1, 0, {}, Result::in_exhausted);
    test({ 0xF0 }, 2, 0, {}, Result::in_exhausted);
    test({ 0xF0, 0x90 }, 0, 0, {}, Result::in_exhausted);
    test({ 0xF0, 0x90 }, 1, 0, {}, Result::in_exhausted);
    test({ 0xF0, 0x90 }, 2, 0, {}, Result::in_exhausted);
    test({ 0xF0, 0x90, 0x8D }, 0, 0, {}, Result::in_exhausted);
    test({ 0xF0, 0x90, 0x8D }, 1, 0, {}, Result::in_exhausted);
    test({ 0xF0, 0x90, 0x8D }, 2, 0, {}, Result::in_exhausted);
    test({ 0xF0, 0x90, 0x8D, 0x88 }, 0, 0, {}, Result::out_exhausted);
    test({ 0xF0, 0x90, 0x8D, 0x88 }, 1, 4, { 0x10348 }, Result::in_exhausted);
    test({ 0xF0, 0x90, 0x8D, 0x88 }, 2, 4, { 0x10348 }, Result::in_exhausted);
    test({ 0x2A, 0xF0, 0x90, 0x8D, 0x88 }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xF0, 0x90, 0x8D, 0x88 }, 1, 1, { 0x2A }, Result::out_exhausted);
    test({ 0x2A, 0xF0, 0x90, 0x8D, 0x88 }, 2, 5, { 0x2A, 0x10348 }, Result::in_exhausted);
    test({ 0x2A, 0xF0, 0x90, 0x8D, 0x88 }, 3, 5, { 0x2A, 0x10348 }, Result::in_exhausted);

    // Invalid: Stray continuation: 0xA2
    test({ 0xA2 }, 0, 0, {}, Result::error);
    test({ 0xA2 }, 1, 0, {}, Result::error);
    test({ 0x2A, 0xA2 }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xA2 }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xA2 }, 2, 1, { 0x2A }, Result::error);
    test({ 0xA2, 0x2B }, 0, 0, {}, Result::error);
    test({ 0xA2, 0x2B }, 1, 0, {}, Result::error);
    test({ 0x2A, 0xA2, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xA2, 0x2B }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xA2, 0x2B }, 2, 1, { 0x2A }, Result::error);

    // Invalid: Stray continuation: 0x82, 0xAC
    test({ 0x82, 0xAC }, 0, 0, {}, Result::error);
    test({ 0x82, 0xAC }, 1, 0, {}, Result::error);
    test({ 0x2A, 0x82, 0xAC }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0x82, 0xAC }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0x82, 0xAC }, 2, 1, { 0x2A }, Result::error);
    test({ 0x82, 0xAC, 0x2B }, 0, 0, {}, Result::error);
    test({ 0x82, 0xAC, 0x2B }, 1, 0, {}, Result::error);
    test({ 0x2A, 0x82, 0xAC, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0x82, 0xAC, 0x2B }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0x82, 0xAC, 0x2B }, 2, 1, { 0x2A }, Result::error);

    // Invalid: Stray continuation: 0x90, 0x8D, 0x88
    test({ 0x90, 0x8D, 0x88 }, 0, 0, {}, Result::error);
    test({ 0x90, 0x8D, 0x88 }, 1, 0, {}, Result::error);
    test({ 0x2A, 0x90, 0x8D, 0x88 }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0x90, 0x8D, 0x88 }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0x90, 0x8D, 0x88 }, 2, 1, { 0x2A }, Result::error);
    test({ 0x90, 0x8D, 0x88, 0x2B }, 0, 0, {}, Result::error);
    test({ 0x90, 0x8D, 0x88, 0x2B }, 1, 0, {}, Result::error);
    test({ 0x2A, 0x90, 0x8D, 0x88, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0x90, 0x8D, 0x88, 0x2B }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0x90, 0x8D, 0x88, 0x2B }, 2, 1, { 0x2A }, Result::error);

    // Invalid: Bad continuation, 2-byte form (cent): 0xC2, 0xA2
    test({ 0xC2, 0x25 }, 0, 0, {}, Result::error);
    test({ 0xC2, 0x25 }, 1, 0, {}, Result::error);
    test({ 0xC2, 0xD0, 0x98 }, 0, 0, {}, Result::error);
    test({ 0xC2, 0xD0, 0x98 }, 1, 0, {}, Result::error);
    test({ 0x2A, 0xC2, 0x25 }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xC2, 0x25 }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xC2, 0x25 }, 2, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xC2, 0xD0, 0x98 }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xC2, 0xD0, 0x98 }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xC2, 0xD0, 0x98 }, 2, 1, { 0x2A }, Result::error);
    test({ 0xC2, 0x25, 0x2B }, 0, 0, {}, Result::error);
    test({ 0xC2, 0x25, 0x2B }, 1, 0, {}, Result::error);
    test({ 0xC2, 0xD0, 0x98, 0x2B }, 0, 0, {}, Result::error);
    test({ 0xC2, 0xD0, 0x98, 0x2B }, 1, 0, {}, Result::error);
    test({ 0x2A, 0xC2, 0x25, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xC2, 0x25, 0x2B }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xC2, 0x25, 0x2B }, 2, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xC2, 0xD0, 0x98, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xC2, 0xD0, 0x98, 0x2B }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xC2, 0xD0, 0x98, 0x2B }, 2, 1, { 0x2A }, Result::error);

    // Invalid: Bad continuation, 3-byte form (euro): 0xE2, 0x82, 0xAC
    test({ 0xE2, 0x25 }, 0, 0, {}, Result::in_exhausted_or_error);
    test({ 0xE2, 0x25 }, 1, 0, {}, Result::in_exhausted_or_error);
    test({ 0xE2, 0xD0, 0x98 }, 0, 0, {}, Result::error);
    test({ 0xE2, 0xD0, 0x98 }, 1, 0, {}, Result::error);
    test({ 0x2A, 0xE2, 0x25 }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xE2, 0x25 }, 1, 1, { 0x2A }, Result::in_exhausted_or_error);
    test({ 0x2A, 0xE2, 0x25 }, 2, 1, { 0x2A }, Result::in_exhausted_or_error);
    test({ 0x2A, 0xE2, 0xD0, 0x98 }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xE2, 0xD0, 0x98 }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xE2, 0xD0, 0x98 }, 2, 1, { 0x2A }, Result::error);
    test({ 0xE2, 0x25, 0x2B }, 0, 0, {}, Result::error);
    test({ 0xE2, 0x25, 0x2B }, 1, 0, {}, Result::error);
    test({ 0xE2, 0xD0, 0x98, 0x2B }, 0, 0, {}, Result::error);
    test({ 0xE2, 0xD0, 0x98, 0x2B }, 1, 0, {}, Result::error);
    test({ 0x2A, 0xE2, 0x25, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xE2, 0x25, 0x2B }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xE2, 0x25, 0x2B }, 2, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xE2, 0xD0, 0x98, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xE2, 0xD0, 0x98, 0x2B }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xE2, 0xD0, 0x98, 0x2B }, 2, 1, { 0x2A }, Result::error);
    test({ 0xE2, 0x82, 0x25 }, 0, 0, {}, Result::error);
    test({ 0xE2, 0x82, 0x25 }, 1, 0, {}, Result::error);
    test({ 0xE2, 0x82, 0xD0, 0x98 }, 0, 0, {}, Result::error);
    test({ 0xE2, 0x82, 0xD0, 0x98 }, 1, 0, {}, Result::error);
    test({ 0x2A, 0xE2, 0x82, 0x25 }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xE2, 0x82, 0x25 }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xE2, 0x82, 0x25 }, 2, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xE2, 0x82, 0xD0, 0x98 }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xE2, 0x82, 0xD0, 0x98 }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xE2, 0x82, 0xD0, 0x98 }, 2, 1, { 0x2A }, Result::error);
    test({ 0xE2, 0x82, 0x25, 0x2B }, 0, 0, {}, Result::error);
    test({ 0xE2, 0x82, 0x25, 0x2B }, 1, 0, {}, Result::error);
    test({ 0xE2, 0x82, 0xD0, 0x98, 0x2B }, 0, 0, {}, Result::error);
    test({ 0xE2, 0x82, 0xD0, 0x98, 0x2B }, 1, 0, {}, Result::error);
    test({ 0x2A, 0xE2, 0x82, 0x25, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xE2, 0x82, 0x25, 0x2B }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xE2, 0x82, 0x25, 0x2B }, 2, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xE2, 0x82, 0xD0, 0x98, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xE2, 0x82, 0xD0, 0x98, 0x2B }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xE2, 0x82, 0xD0, 0x98, 0x2B }, 2, 1, { 0x2A }, Result::error);

    // Invalid: Bad continuation, 4-byte form (hwair): 0xF0, 0x90, 0x8D, 0x88
    test({ 0xF0, 0x25 }, 0, 0, {}, Result::in_exhausted_or_error);
    test({ 0xF0, 0x25 }, 1, 0, {}, Result::in_exhausted_or_error);
    test({ 0xF0, 0xD0, 0x98 }, 0, 0, {}, Result::in_exhausted_or_error);
    test({ 0xF0, 0xD0, 0x98 }, 1, 0, {}, Result::in_exhausted_or_error);
    test({ 0x2A, 0xF0, 0x25 }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xF0, 0x25 }, 1, 1, { 0x2A }, Result::in_exhausted_or_error);
    test({ 0x2A, 0xF0, 0x25 }, 2, 1, { 0x2A }, Result::in_exhausted_or_error);
    test({ 0x2A, 0xF0, 0xD0, 0x98 }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xF0, 0xD0, 0x98 }, 1, 1, { 0x2A }, Result::in_exhausted_or_error);
    test({ 0x2A, 0xF0, 0xD0, 0x98 }, 2, 1, { 0x2A }, Result::in_exhausted_or_error);
    test({ 0xF0, 0x25, 0x2B }, 0, 0, {}, Result::in_exhausted_or_error);
    test({ 0xF0, 0x25, 0x2B }, 1, 0, {}, Result::in_exhausted_or_error);
    test({ 0xF0, 0xD0, 0x98, 0x2B }, 0, 0, {}, Result::error);
    test({ 0xF0, 0xD0, 0x98, 0x2B }, 1, 0, {}, Result::error);
    test({ 0x2A, 0xF0, 0x25, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xF0, 0x25, 0x2B }, 1, 1, { 0x2A }, Result::in_exhausted_or_error);
    test({ 0x2A, 0xF0, 0x25, 0x2B }, 2, 1, { 0x2A }, Result::in_exhausted_or_error);
    test({ 0x2A, 0xF0, 0xD0, 0x98, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xF0, 0xD0, 0x98, 0x2B }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xF0, 0xD0, 0x98, 0x2B }, 2, 1, { 0x2A }, Result::error);
    test({ 0xF0, 0x90, 0x25 }, 0, 0, {}, Result::in_exhausted_or_error);
    test({ 0xF0, 0x90, 0x25 }, 1, 0, {}, Result::in_exhausted_or_error);
    test({ 0xF0, 0x90, 0xD0, 0x98 }, 0, 0, {}, Result::error);
    test({ 0xF0, 0x90, 0xD0, 0x98 }, 1, 0, {}, Result::error);
    test({ 0x2A, 0xF0, 0x90, 0x25 }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xF0, 0x90, 0x25 }, 1, 1, { 0x2A }, Result::in_exhausted_or_error);
    test({ 0x2A, 0xF0, 0x90, 0x25 }, 2, 1, { 0x2A }, Result::in_exhausted_or_error);
    test({ 0x2A, 0xF0, 0x90, 0xD0, 0x98 }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xF0, 0x90, 0xD0, 0x98 }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xF0, 0x90, 0xD0, 0x98 }, 2, 1, { 0x2A }, Result::error);
    test({ 0xF0, 0x90, 0x25, 0x2B }, 0, 0, {}, Result::error);
    test({ 0xF0, 0x90, 0x25, 0x2B }, 1, 0, {}, Result::error);
    test({ 0xF0, 0x90, 0xD0, 0x98, 0x2B }, 0, 0, {}, Result::error);
    test({ 0xF0, 0x90, 0xD0, 0x98, 0x2B }, 1, 0, {}, Result::error);
    test({ 0x2A, 0xF0, 0x90, 0x25, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xF0, 0x90, 0x25, 0x2B }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xF0, 0x90, 0x25, 0x2B }, 2, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xF0, 0x90, 0xD0, 0x98, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xF0, 0x90, 0xD0, 0x98, 0x2B }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xF0, 0x90, 0xD0, 0x98, 0x2B }, 2, 1, { 0x2A }, Result::error);
    test({ 0xF0, 0x90, 0x8D, 0x25 }, 0, 0, {}, Result::error);
    test({ 0xF0, 0x90, 0x8D, 0x25 }, 1, 0, {}, Result::error);
    test({ 0xF0, 0x90, 0x8D, 0xD0, 0x98 }, 0, 0, {}, Result::error);
    test({ 0xF0, 0x90, 0x8D, 0xD0, 0x98 }, 1, 0, {}, Result::error);
    test({ 0x2A, 0xF0, 0x90, 0x8D, 0x25 }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xF0, 0x90, 0x8D, 0x25 }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xF0, 0x90, 0x8D, 0x25 }, 2, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xF0, 0x90, 0x8D, 0xD0, 0x98 }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xF0, 0x90, 0x8D, 0xD0, 0x98 }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xF0, 0x90, 0x8D, 0xD0, 0x98 }, 2, 1, { 0x2A }, Result::error);
    test({ 0xF0, 0x90, 0x8D, 0x25, 0x2B }, 0, 0, {}, Result::error);
    test({ 0xF0, 0x90, 0x8D, 0x25, 0x2B }, 1, 0, {}, Result::error);
    test({ 0xF0, 0x90, 0x8D, 0xD0, 0x98, 0x2B }, 0, 0, {}, Result::error);
    test({ 0xF0, 0x90, 0x8D, 0xD0, 0x98, 0x2B }, 1, 0, {}, Result::error);
    test({ 0x2A, 0xF0, 0x90, 0x8D, 0x25, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xF0, 0x90, 0x8D, 0x25, 0x2B }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xF0, 0x90, 0x8D, 0x25, 0x2B }, 2, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xF0, 0x90, 0x8D, 0xD0, 0x98, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xF0, 0x90, 0x8D, 0xD0, 0x98, 0x2B }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xF0, 0x90, 0x8D, 0xD0, 0x98, 0x2B }, 2, 1, { 0x2A }, Result::error);

    // Invalid: Code point in surrogate range: 0xED, 0xA0, 0xA1 --> U+D821
    test({ 0xED, 0xA0, 0xA1 }, 0, 0, {}, Result::error);
    test({ 0xED, 0xA0, 0xA1 }, 1, 0, {}, Result::error);
    test({ 0x2A, 0xED, 0xA0, 0xA1 }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xED, 0xA0, 0xA1 }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xED, 0xA0, 0xA1 }, 2, 1, { 0x2A }, Result::error);
    test({ 0xED, 0xA0, 0xA1, 0x2B }, 0, 0, {}, Result::error);
    test({ 0xED, 0xA0, 0xA1, 0x2B }, 1, 0, {}, Result::error);
    test({ 0x2A, 0xED, 0xA0, 0xA1, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xED, 0xA0, 0xA1, 0x2B }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xED, 0xA0, 0xA1, 0x2B }, 2, 1, { 0x2A }, Result::error);

    // Invalid: Non-character code point: 0xEF, 0xBF, 0xBE --> U+FFFE
    test({ 0xEF, 0xBF, 0xBE }, 0, 0, {}, Result::error);
    test({ 0xEF, 0xBF, 0xBE }, 1, 0, {}, Result::error);
    test({ 0x2A, 0xEF, 0xBF, 0xBE }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xEF, 0xBF, 0xBE }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xEF, 0xBF, 0xBE }, 2, 1, { 0x2A }, Result::error);
    test({ 0xEF, 0xBF, 0xBE, 0x2B }, 0, 0, {}, Result::error);
    test({ 0xEF, 0xBF, 0xBE, 0x2B }, 1, 0, {}, Result::error);
    test({ 0x2A, 0xEF, 0xBF, 0xBE, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xEF, 0xBF, 0xBE, 0x2B }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xEF, 0xBF, 0xBE, 0x2B }, 2, 1, { 0x2A }, Result::error);

    // Invalid: Overlarge code point: 0xF4, 0x90, 0x80, 0xA1 --> U+110021
    test({ 0xF4, 0x90, 0x80, 0xA1 }, 0, 0, {}, Result::error);
    test({ 0xF4, 0x90, 0x80, 0xA1 }, 1, 0, {}, Result::error);
    test({ 0x2A, 0xF4, 0x90, 0x80, 0xA1 }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xF4, 0x90, 0x80, 0xA1 }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xF4, 0x90, 0x80, 0xA1 }, 2, 1, { 0x2A }, Result::error);
    test({ 0xF4, 0x90, 0x80, 0xA1, 0x2B }, 0, 0, {}, Result::error);
    test({ 0xF4, 0x90, 0x80, 0xA1, 0x2B }, 1, 0, {}, Result::error);
    test({ 0x2A, 0xF4, 0x90, 0x80, 0xA1, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xF4, 0x90, 0x80, 0xA1, 0x2B }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xF4, 0x90, 0x80, 0xA1, 0x2B }, 2, 1, { 0x2A }, Result::error);
}


ARCHON_TEST(Core_Unicode_EncodeUtf16Incr)
{
    using char_type_1 = char32_t;
    using char_type_2 = char16_t;

    using traits_type_1 = std::char_traits<char_type_1>;
    using traits_type_2 = std::char_traits<char_type_2>;

    using int_type_1 = traits_type_1::int_type;
    using int_type_2 = traits_type_2::int_type;

    std::array<char_type_1, 64> seed_memory_1;
    std::array<char_type_2, 64> seed_memory_2;

    core::Buffer buffer_1(seed_memory_1);
    core::Buffer buffer_2(seed_memory_2);

    auto test = [&, &parent_test_context = test_context](std::initializer_list<int_type_1> in, std::size_t out_size,
                                                         std::size_t expected_in_size,
                                                         std::initializer_list<int_type_2> expected_out,
                                                         Result expected_result) {
        ARCHON_TEST_TRAIL(parent_test_context,
                          core::formatted_wrn("%s, %s, %s, %s, %s", core::as_sbr_list(in), out_size, expected_in_size,
                                              core::as_sbr_list(expected_out), ResultEnum(expected_result)));
        std::size_t offset_1 = 0;
        for (int_type_1 val : in)
            buffer_1.append_a(traits_type_1::to_char_type(val), offset_1);
        std::size_t offset_2 = 0;
        for (int_type_2 val : expected_out)
            buffer_2.append_a(traits_type_2::to_char_type(val), offset_2);
        buffer_2.reserve_extra(out_size, offset_2);
        core::Span in_2 = { buffer_1.data(), offset_1 };
        core::Span expected_out_2 = { buffer_2.data(), offset_2 };
        core::Span out = { buffer_2.data() + offset_2, out_size };
        std::size_t in_offset = 0;
        std::size_t out_offset = 0;
        bool in_exhausted = {};
        bool error = {};
        core::encode_utf16_incr<char_type_1, char_type_2,
                                traits_type_1, traits_type_2>(in_2, out, in_offset, out_offset, in_exhausted, error);
        ARCHON_CHECK_EQUAL(in_offset, expected_in_size);
        ARCHON_CHECK_EQUAL_SEQ(out.subspan(0, out_offset), expected_out_2);
        switch (expected_result) {
            case Result::in_exhausted:
                ARCHON_CHECK(in_exhausted);
                break;
            case Result::error:
                if (ARCHON_LIKELY(ARCHON_CHECK_NOT(in_exhausted)))
                    ARCHON_CHECK(error);
                break;
            case Result::out_exhausted:
                if (ARCHON_LIKELY(ARCHON_CHECK_NOT(in_exhausted)))
                    ARCHON_CHECK_NOT(error);
                break;
            case Result::in_exhausted_or_error:
                ARCHON_CHECK(in_exhausted || error);
                break;
        }
    };

    // Empty
    test({}, 0, 0, {}, Result::in_exhausted);
    test({}, 1, 0, {}, Result::in_exhausted);

    // Single code unit form: U+20AC --> 0x20AC
    test({ 0x20AC }, 0, 0, {}, Result::out_exhausted);
    test({ 0x20AC }, 1, 1, { 0x20AC }, Result::in_exhausted);
    test({ 0x20AC }, 2, 1, { 0x20AC }, Result::in_exhausted);
    test({ 0x2A, 0x20AC }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0x20AC }, 1, 1, { 0x2A }, Result::out_exhausted);
    test({ 0x2A, 0x20AC }, 2, 2, { 0x2A, 0x20AC }, Result::in_exhausted);
    test({ 0x2A, 0x20AC }, 3, 2, { 0x2A, 0x20AC }, Result::in_exhausted);
    test({ 0x20AC, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x20AC, 0x2B }, 1, 1, { 0x20AC }, Result::out_exhausted);
    test({ 0x20AC, 0x2B }, 2, 2, { 0x20AC, 0x2B }, Result::in_exhausted);
    test({ 0x20AC, 0x2B }, 3, 2, { 0x20AC, 0x2B }, Result::in_exhausted);
    test({ 0x2A, 0x20AC, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0x20AC, 0x2B }, 1, 1, { 0x2A }, Result::out_exhausted);
    test({ 0x2A, 0x20AC, 0x2B }, 2, 2, { 0x2A, 0x20AC }, Result::out_exhausted);
    test({ 0x2A, 0x20AC, 0x2B }, 3, 3, { 0x2A, 0x20AC, 0x2B }, Result::in_exhausted);
    test({ 0x2A, 0x20AC, 0x2B }, 4, 3, { 0x2A, 0x20AC, 0x2B }, Result::in_exhausted);

    // Surrogate pair: U+24B62 --> 0xD852, 0xDF62
    test({ 0x24B62 }, 0, 0, {}, Result::out_exhausted);
    test({ 0x24B62 }, 1, 0, {}, Result::out_exhausted);
    test({ 0x24B62 }, 2, 1, { 0xD852, 0xDF62 }, Result::in_exhausted);
    test({ 0x24B62 }, 3, 1, { 0xD852, 0xDF62 }, Result::in_exhausted);
    test({ 0x2A, 0x24B62 }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0x24B62 }, 1, 1, { 0x2A }, Result::out_exhausted);
    test({ 0x2A, 0x24B62 }, 2, 1, { 0x2A }, Result::out_exhausted);
    test({ 0x2A, 0x24B62 }, 3, 2, { 0x2A, 0xD852, 0xDF62 }, Result::in_exhausted);
    test({ 0x2A, 0x24B62 }, 4, 2, { 0x2A, 0xD852, 0xDF62 }, Result::in_exhausted);
    test({ 0x24B62, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x24B62, 0x2B }, 1, 0, {}, Result::out_exhausted);
    test({ 0x24B62, 0x2B }, 2, 1, { 0xD852, 0xDF62 }, Result::out_exhausted);
    test({ 0x24B62, 0x2B }, 3, 2, { 0xD852, 0xDF62, 0x2B }, Result::in_exhausted);
    test({ 0x24B62, 0x2B }, 4, 2, { 0xD852, 0xDF62, 0x2B }, Result::in_exhausted);
    test({ 0x2A, 0x24B62, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0x24B62, 0x2B }, 1, 1, { 0x2A }, Result::out_exhausted);
    test({ 0x2A, 0x24B62, 0x2B }, 2, 1, { 0x2A }, Result::out_exhausted);
    test({ 0x2A, 0x24B62, 0x2B }, 3, 2, { 0x2A, 0xD852, 0xDF62 }, Result::out_exhausted);
    test({ 0x2A, 0x24B62, 0x2B }, 4, 3, { 0x2A, 0xD852, 0xDF62, 0x2B }, Result::in_exhausted);
    test({ 0x2A, 0x24B62, 0x2B }, 5, 3, { 0x2A, 0xD852, 0xDF62, 0x2B }, Result::in_exhausted);

    // Invalid: Code point in surrogate range: U+D821
    test({ 0xD821 }, 0, 0, {}, Result::error);
    test({ 0xD821 }, 1, 0, {}, Result::error);
    test({ 0xD821 }, 2, 0, {}, Result::error);
    test({ 0xD821 }, 3, 0, {}, Result::error);
    test({ 0x2A, 0xD821 }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xD821 }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xD821 }, 2, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xD821 }, 3, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xD821 }, 4, 1, { 0x2A }, Result::error);
    test({ 0xD821, 0x2B }, 0, 0, {}, Result::error);
    test({ 0xD821, 0x2B }, 1, 0, {}, Result::error);
    test({ 0xD821, 0x2B }, 2, 0, {}, Result::error);
    test({ 0xD821, 0x2B }, 3, 0, {}, Result::error);
    test({ 0x2A, 0xD821, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xD821, 0x2B }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xD821, 0x2B }, 2, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xD821, 0x2B }, 3, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xD821, 0x2B }, 4, 1, { 0x2A }, Result::error);

    // Invalid: Non-character code point: U+FFFE
    test({ 0xFFFE }, 0, 0, {}, Result::error);
    test({ 0xFFFE }, 1, 0, {}, Result::error);
    test({ 0xFFFE }, 2, 0, {}, Result::error);
    test({ 0xFFFE }, 3, 0, {}, Result::error);
    test({ 0x2A, 0xFFFE }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xFFFE }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xFFFE }, 2, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xFFFE }, 3, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xFFFE }, 4, 1, { 0x2A }, Result::error);
    test({ 0xFFFE, 0x2B }, 0, 0, {}, Result::error);
    test({ 0xFFFE, 0x2B }, 1, 0, {}, Result::error);
    test({ 0xFFFE, 0x2B }, 2, 0, {}, Result::error);
    test({ 0xFFFE, 0x2B }, 3, 0, {}, Result::error);
    test({ 0x2A, 0xFFFE, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xFFFE, 0x2B }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xFFFE, 0x2B }, 2, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xFFFE, 0x2B }, 3, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xFFFE, 0x2B }, 4, 1, { 0x2A }, Result::error);

    // Invalid: Overlarge code point: U+110021
    test({ 0x110021 }, 0, 0, {}, Result::error);
    test({ 0x110021 }, 1, 0, {}, Result::error);
    test({ 0x110021 }, 2, 0, {}, Result::error);
    test({ 0x110021 }, 3, 0, {}, Result::error);
    test({ 0x2A, 0x110021 }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0x110021 }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0x110021 }, 2, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0x110021 }, 3, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0x110021 }, 4, 1, { 0x2A }, Result::error);
    test({ 0x110021, 0x2B }, 0, 0, {}, Result::error);
    test({ 0x110021, 0x2B }, 1, 0, {}, Result::error);
    test({ 0x110021, 0x2B }, 2, 0, {}, Result::error);
    test({ 0x110021, 0x2B }, 3, 0, {}, Result::error);
    test({ 0x2A, 0x110021, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0x110021, 0x2B }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0x110021, 0x2B }, 2, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0x110021, 0x2B }, 3, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0x110021, 0x2B }, 4, 1, { 0x2A }, Result::error);
}


ARCHON_TEST(Core_Unicode_DecodeUtf16Incr)
{
    using char_type_1 = char16_t;
    using char_type_2 = char32_t;

    using traits_type_1 = std::char_traits<char_type_1>;
    using traits_type_2 = std::char_traits<char_type_2>;

    using int_type_1 = traits_type_1::int_type;
    using int_type_2 = traits_type_2::int_type;

    std::array<char_type_1, 64> seed_memory_1;
    std::array<char_type_2, 64> seed_memory_2;

    core::Buffer buffer_1(seed_memory_1);
    core::Buffer buffer_2(seed_memory_2);

    auto test = [&, &parent_test_context = test_context](std::initializer_list<int_type_1> in, std::size_t out_size,
                                                         std::size_t expected_in_size,
                                                         std::initializer_list<int_type_2> expected_out,
                                                         Result expected_result) {
        ARCHON_TEST_TRAIL(parent_test_context,
                          core::formatted_wrn("%s, %s, %s, %s, %s", core::as_sbr_list(in), out_size, expected_in_size,
                                              core::as_sbr_list(expected_out), ResultEnum(expected_result)));
        std::size_t offset_1 = 0;
        for (int_type_1 val : in)
            buffer_1.append_a(traits_type_1::to_char_type(val), offset_1);
        std::size_t offset_2 = 0;
        for (int_type_2 val : expected_out)
            buffer_2.append_a(traits_type_2::to_char_type(val), offset_2);
        buffer_2.reserve_extra(out_size, offset_2);
        core::Span in_2 = { buffer_1.data(), offset_1 };
        core::Span expected_out_2 = { buffer_2.data(), offset_2 };
        core::Span out = { buffer_2.data() + offset_2, out_size };
        std::size_t in_offset = 0;
        std::size_t out_offset = 0;
        bool in_exhausted = {};
        bool error = {};
        core::decode_utf16_incr<char_type_1, char_type_2,
                                traits_type_1, traits_type_2>(in_2, out, in_offset, out_offset, in_exhausted, error);
        ARCHON_CHECK_EQUAL(in_offset, expected_in_size);
        ARCHON_CHECK_EQUAL_SEQ(out.subspan(0, out_offset), expected_out_2);
        switch (expected_result) {
            case Result::in_exhausted:
                ARCHON_CHECK(in_exhausted);
                break;
            case Result::error:
                if (ARCHON_LIKELY(ARCHON_CHECK_NOT(in_exhausted)))
                    ARCHON_CHECK(error);
                break;
            case Result::out_exhausted:
                if (ARCHON_LIKELY(ARCHON_CHECK_NOT(in_exhausted)))
                    ARCHON_CHECK_NOT(error);
                break;
            case Result::in_exhausted_or_error:
                ARCHON_CHECK(in_exhausted || error);
                break;
        }
    };

    // Empty
    test({}, 0, 0, {}, Result::in_exhausted);
    test({}, 1, 0, {}, Result::in_exhausted);

    // Valid single code unit form: 0x20AC --> U+20AC
    test({ 0x20AC }, 0, 0, {}, Result::out_exhausted);
    test({ 0x20AC }, 1, 1, { 0x20AC }, Result::in_exhausted);
    test({ 0x20AC }, 2, 1, { 0x20AC }, Result::in_exhausted);
    test({ 0x2A, 0x20AC }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0x20AC }, 1, 1, { 0x2A }, Result::out_exhausted);
    test({ 0x2A, 0x20AC }, 2, 2, { 0x2A, 0x20AC }, Result::in_exhausted);
    test({ 0x2A, 0x20AC }, 3, 2, { 0x2A, 0x20AC }, Result::in_exhausted);

    // Valid surrogate pair: 0xD852, 0xDF62 --> U+24B62
    test({ 0xD852 }, 0, 0, {}, Result::in_exhausted);
    test({ 0xD852 }, 1, 0, {}, Result::in_exhausted);
    test({ 0xD852 }, 2, 0, {}, Result::in_exhausted);
    test({ 0xD852, 0xDF62 }, 0, 0, {}, Result::out_exhausted);
    test({ 0xD852, 0xDF62 }, 1, 2, { 0x24B62 }, Result::in_exhausted);
    test({ 0xD852, 0xDF62 }, 2, 2, { 0x24B62 }, Result::in_exhausted);
    test({ 0x2A, 0xD852, 0xDF62 }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xD852, 0xDF62 }, 1, 1, { 0x2A }, Result::out_exhausted);
    test({ 0x2A, 0xD852, 0xDF62 }, 2, 3, { 0x2A, 0x24B62 }, Result::in_exhausted);
    test({ 0x2A, 0xD852, 0xDF62 }, 3, 3, { 0x2A, 0x24B62 }, Result::in_exhausted);

    // Invalid: Stray continuation: 0xDF62
    test({ 0xDF62 }, 0, 0, {}, Result::error);
    test({ 0xDF62 }, 1, 0, {}, Result::error);
    test({ 0x2A, 0xDF62 }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xDF62 }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xDF62 }, 2, 1, { 0x2A }, Result::error);
    test({ 0xDF62, 0x2B }, 0, 0, {}, Result::error);
    test({ 0xDF62, 0x2B }, 1, 0, {}, Result::error);
    test({ 0x2A, 0xDF62, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xDF62, 0x2B }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xDF62, 0x2B }, 2, 1, { 0x2A }, Result::error);

    // Invalid: Bad continuation: 0xD852
    test({ 0xD852, 0x25 }, 0, 0, {}, Result::error);
    test({ 0xD852, 0x25 }, 1, 0, {}, Result::error);
    test({ 0xD852, 0xD801, 0xDC37 }, 0, 0, {}, Result::error);
    test({ 0xD852, 0xD801, 0xDC37 }, 1, 0, {}, Result::error);
    test({ 0x2A, 0xD852, 0x25 }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xD852, 0x25 }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xD852, 0x25 }, 2, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xD852, 0xD801, 0xDC37 }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xD852, 0xD801, 0xDC37 }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xD852, 0xD801, 0xDC37 }, 2, 1, { 0x2A }, Result::error);
    test({ 0xD852, 0x25, 0x2B }, 0, 0, {}, Result::error);
    test({ 0xD852, 0x25, 0x2B }, 1, 0, {}, Result::error);
    test({ 0xD852, 0xD801, 0xDC37, 0x2B }, 0, 0, {}, Result::error);
    test({ 0xD852, 0xD801, 0xDC37, 0x2B }, 1, 0, {}, Result::error);
    test({ 0x2A, 0xD852, 0x25, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xD852, 0x25, 0x2B }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xD852, 0x25, 0x2B }, 2, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xD852, 0xD801, 0xDC37, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xD852, 0xD801, 0xDC37, 0x2B }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xD852, 0xD801, 0xDC37, 0x2B }, 2, 1, { 0x2A }, Result::error);

    // Invalid: Non-character code point: U+FFFE
    test({ 0xFFFE }, 0, 0, {}, Result::error);
    test({ 0xFFFE }, 1, 0, {}, Result::error);
    test({ 0x2A, 0xFFFE }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xFFFE }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xFFFE }, 2, 1, { 0x2A }, Result::error);
    test({ 0xFFFE, 0x2B }, 0, 0, {}, Result::error);
    test({ 0xFFFE, 0x2B }, 1, 0, {}, Result::error);
    test({ 0x2A, 0xFFFE, 0x2B }, 0, 0, {}, Result::out_exhausted);
    test({ 0x2A, 0xFFFE, 0x2B }, 1, 1, { 0x2A }, Result::error);
    test({ 0x2A, 0xFFFE, 0x2B }, 2, 1, { 0x2A }, Result::error);
}
