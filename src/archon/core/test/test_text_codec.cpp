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


#include <type_traits>
#include <array>
#include <string_view>
#include <locale>

#include <archon/core/span.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/array_seeded_buffer.hpp>
#include <archon/core/char_mapper.hpp>
#include <archon/core/text_codec.hpp>
#include <archon/check.hpp>
#include <archon/core/test/stateful_char_codec.hpp>


using namespace archon;


static_assert(std::is_empty_v<core::TextCodec>);
static_assert(std::is_empty_v<core::PosixTextCodec>);
static_assert(std::is_empty_v<core::WindowsTextCodec>);

static_assert(!std::is_empty_v<core::WideTextCodec>);
static_assert(!std::is_empty_v<core::WidePosixTextCodec>);
static_assert(!std::is_empty_v<core::WideWindowsTextCodec>);

static_assert(core::PosixTextCodec::is_degen);
static_assert(!core::WindowsTextCodec::is_degen);
static_assert(!core::WidePosixTextCodec::is_degen);
static_assert(!core::WideWindowsTextCodec::is_degen);



namespace {

ARCHON_TEST_VARIANTS(impl_variants,
                     ARCHON_TEST_TYPE(core::PosixTextCodecImpl<char>,      Posix),
                     ARCHON_TEST_TYPE(core::WindowsTextCodecImpl<char>,    Windows),
                     ARCHON_TEST_TYPE(core::PosixTextCodecImpl<wchar_t>,   WidePosix),
                     ARCHON_TEST_TYPE(core::WindowsTextCodecImpl<wchar_t>, WideWindows));

} // unnamed namespace



ARCHON_TEST_BATCH(Core_TextCodec_Decode, impl_variants)
{
    using impl_type = test_type;
    using codec_type = core::GenericTextCodec<impl_type>;
    using char_type = typename codec_type::char_type;
    codec_type codec(test_context.locale);
    core::Buffer<char_type> buffer;
    std::basic_string_view<char_type> result = codec.decode(std::string_view("foo"), buffer);
    std::array<char_type, 64> seed_memory;
    core::BasicStringWidener<char_type> widener(test_context.locale, seed_memory);
    ARCHON_CHECK_EQUAL(result, widener.widen("foo"));
}


ARCHON_TEST_BATCH(Core_TextCodec_Encode, impl_variants)
{
    using impl_type = test_type;
    using codec_type = core::GenericTextCodec<impl_type>;
    using char_type = typename codec_type::char_type;
    codec_type codec(test_context.locale);
    core::Buffer<char> buffer;
    std::array<char_type, 64> seed_memory;
    core::BasicStringWidener<char_type> widener(test_context.locale, seed_memory);
    ARCHON_CHECK_EQUAL(codec.encode(widener.widen("foo"), buffer), "foo");
}


ARCHON_TEST_BATCH(Core_TextCodec_Decoder, impl_variants)
{
    using impl_type = test_type;
    using decoder_type = core::GenericTextDecoder<impl_type>;
    using char_type = typename decoder_type::char_type;
    std::array<char_type, 64> seed_memory_1;
    decoder_type decoder(test_context.locale, seed_memory_1);
    std::array<char_type, 64> seed_memory_2;
    core::BasicStringWidener widener(test_context.locale, seed_memory_2);
    std::string_view string = "foo";
    ARCHON_CHECK_EQUAL_SEQ(decoder.decode_sc(string), widener.widen(string));
}


ARCHON_TEST_BATCH(Core_TextCodec_Encoder, impl_variants)
{
    using impl_type = test_type;
    using encoder_type = core::GenericTextEncoder<impl_type>;
    using char_type = typename encoder_type::char_type;
    std::array<char, 64> seed_memory_1;
    encoder_type encoder(test_context.locale, seed_memory_1);
    std::array<char_type, 64> seed_memory_2;
    core::BasicStringWidener widener(test_context.locale, seed_memory_2);
    std::string_view string = "foo";
    ARCHON_CHECK_EQUAL_SEQ(encoder.encode_sc(widener.widen(string)), string);
}


ARCHON_TEST(Core_TextCodec_StatefulCharCodecDecode)
{
    using char_codec_type = core::test::StatefulCharCodec;
    using traits_type = char_codec_type::traits_type;
    using text_codec_type = core::BasicPosixTextCodec<char, traits_type, char_codec_type>;
    text_codec_type text_codec(test_context.locale);
    char data[] { 0x16, 0x01, 0x17, 0x03, 0x00, 0x16, 0x08, 0x01, 0x10 };
    core::ArraySeededBuffer<char, 16> buffer;
    ARCHON_CHECK_EQUAL(text_codec.decode(core::Span(data), buffer), "aspha");
}


ARCHON_TEST(Core_TextCodec_StatefulCharCodecEncode)
{
    using char_codec_type = core::test::StatefulCharCodec;
    using traits_type = char_codec_type::traits_type;
    using text_codec_type = core::BasicPosixTextCodec<char, traits_type, char_codec_type>;
    text_codec_type text_codec(test_context.locale);
    core::ArraySeededBuffer<char, 16> buffer;
    char expected[] { 0x16, 0x01, 0x17, 0x03, 0x00, 0x16, 0x08, 0x01, 0x10 };
    ARCHON_CHECK_EQUAL_SEQ(text_codec.encode(std::string_view("aspha"), buffer), expected);
}
