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
#include <utility>
#include <array>
#include <string_view>
#include <string>
#include <random>
#include <locale>
#include <ios>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/quote.hpp>
#include <archon/core/locale.hpp>
#include <archon/core/file.hpp>
#include <archon/core/text_file_impl.hpp>
#include <archon/core/text_file.hpp>
#include <archon/core/text_file_stream.hpp>
#include <archon/check.hpp>
#include <archon/core/test/stateful_char_codec.hpp>
#include <archon/core/test/locale_utils.hpp>



using namespace archon;


namespace {


ARCHON_TEST_VARIANTS(variants,
                     ARCHON_TEST_TYPE(core::PosixTextFileStream,       Posix),
                     ARCHON_TEST_TYPE(core::WindowsTextFileStream,     Windows),
                     ARCHON_TEST_TYPE(core::WidePosixTextFileStream,   WidePosix),
                     ARCHON_TEST_TYPE(core::WideWindowsTextFileStream, WideWindows));


ARCHON_TEST_VARIANTS(wide_variants,
                     ARCHON_TEST_TYPE(core::WidePosixTextFileStream,   WidePosix),
                     ARCHON_TEST_TYPE(core::WideWindowsTextFileStream, WideWindows));


ARCHON_TEST_VARIANTS(buffered_impl_variants,
                     ARCHON_TEST_TYPE(core::BufferedPosixTextFileImpl<char>,      Posix),
                     ARCHON_TEST_TYPE(core::BufferedWindowsTextFileImpl<char>,    Windows),
                     ARCHON_TEST_TYPE(core::BufferedPosixTextFileImpl<wchar_t>,   WidePosix),
                     ARCHON_TEST_TYPE(core::BufferedWindowsTextFileImpl<wchar_t>, WideWindows));


} // unnamed namespace



ARCHON_TEST_BATCH(Core_TextFileStream_Read, variants)
{
    std::mt19937_64 random(test_context.seed_seq());
    std::uniform_int_distribution<std::size_t> buffer_size_distr(0, 8);
    std::locale locale(test_context.locale, std::locale::classic(), std::locale::numeric);

    ARCHON_TEST_FILE(path);
    {
        core::TextFile text_file(path, core::File::Mode::write);
        text_file.write(std::string_view("4689"));
        text_file.flush();
    }
    using stream_type = test_type;
    typename stream_type::Config config;
    config.buffer_size = buffer_size_distr(random);
    config.impl.char_codec_buffer_size = buffer_size_distr(random);
    config.impl.newline_codec_buffer_size = buffer_size_distr(random);
    stream_type stream(path, core::File::Mode::read, std::move(config));
    stream.imbue(locale);
    ARCHON_CHECK(stream);
    int value = 0;
    stream >> value;
    ARCHON_CHECK(stream);
    ARCHON_CHECK_EQUAL(value, 4689);
}


ARCHON_TEST_BATCH(Core_TextFileStream_WriteAndFlush, variants)
{
    std::mt19937_64 random(test_context.seed_seq());
    std::uniform_int_distribution<std::size_t> buffer_size_distr(0, 8);
    std::locale locale(test_context.locale, std::locale::classic(), std::locale::numeric);

    ARCHON_TEST_FILE(path);
    {
        using stream_type = test_type;
        typename stream_type::Config config;
        config.buffer_size = buffer_size_distr(random);
        config.impl.char_codec_buffer_size = buffer_size_distr(random);
        config.impl.newline_codec_buffer_size = buffer_size_distr(random);
        stream_type stream(path, core::File::Mode::write, std::move(config));
        stream.imbue(locale);
        ARCHON_CHECK(stream);
        stream << 4689;
        ARCHON_CHECK(stream);
        stream.flush();
        ARCHON_CHECK(stream);
    }
    core::TextFile text_file(path, core::File::Mode::read);
    std::array<char, 64> buffer;
    std::size_t n = text_file.read(buffer);
    std::string_view data(buffer.data(), n);
    ARCHON_CHECK_EQUAL(data, "4689");
}


ARCHON_TEST_BATCH(Core_TextFileStream_TellAndSeek, variants)
{
    std::mt19937_64 random(test_context.seed_seq());
    std::uniform_int_distribution<std::size_t> buffer_size_distr(0, 8);
    std::locale locale(test_context.locale, std::locale::classic(), std::locale::numeric);

    ARCHON_TEST_FILE(path);
    using stream_type = test_type;
    typename stream_type::Config config;
    config.buffer_size = buffer_size_distr(random);
    config.impl.char_codec_buffer_size = buffer_size_distr(random);
    config.impl.newline_codec_buffer_size = buffer_size_distr(random);
    stream_type stream(path, core::File::Mode::write, std::move(config));
    stream.imbue(locale);
    ARCHON_CHECK(stream);
    ARCHON_CHECK_EQUAL(stream.tellp(), 0);
    stream << 4689;
    ARCHON_CHECK(stream);
    ARCHON_CHECK_EQUAL(stream.tellp(), 4);
    stream.flush();
    ARCHON_CHECK(stream);
    stream.seekg(0);
    ARCHON_CHECK(stream);
    ARCHON_CHECK_EQUAL(stream.tellp(), 0);
    int value = 0;
    stream >> value;
    ARCHON_CHECK(stream);
    ARCHON_CHECK_EQUAL(value, 4689);
    ARCHON_CHECK_EQUAL(stream.tellp(), 4);
}


ARCHON_TEST_BATCH(Core_TextFileStream_PartialByteSequenceAtEndOfFile, wide_variants)
{
    std::mt19937_64 random(test_context.seed_seq());
    std::uniform_int_distribution<std::size_t> buffer_size_distr(0, 8);

    using stream_type = test_type;
    using char_type = typename stream_type::char_type;
    using traits_type = typename stream_type::traits_type;

    ARCHON_TEST_FILE(path);
    {
        core::File file(path, core::File::Mode::write);
        file.write(std::string_view("$\xE2\x82")); // First two bytes of euro sign
    }

    auto subtest = [&, &parent_test_context = test_context](const std::locale& locale) {
        ARCHON_TEST_TRAIL(parent_test_context, core::quoted(std::string_view(locale.name())));

        typename stream_type::Config config;
        config.buffer_size = buffer_size_distr(random);
        config.impl.char_codec_buffer_size = buffer_size_distr(random);
        config.impl.newline_codec_buffer_size = buffer_size_distr(random);
        stream_type stream(path, core::File::Mode::read, std::move(config));
        stream.imbue(locale);
        std::array<char_type, 64> buffer;
        stream.read(buffer.data(), buffer.size());
        if (ARCHON_UNLIKELY(!ARCHON_CHECK_NOT(stream)))
            return;
        if (ARCHON_UNLIKELY(!ARCHON_CHECK_EQUAL(stream.gcount(), 1)))
            return;
        if (ARCHON_UNLIKELY(!ARCHON_CHECK_EQUAL(buffer[0], traits_type::to_char_type(0x24))))
            return;
    };

    for (const std::locale& locale : core::test::get_candidate_locales()) {
        bool is_utf8 = (core::assume_utf8_locale(locale) && (core::assume_unicode_locale(locale) || ARCHON_WINDOWS));
        if (is_utf8)
            subtest(locale);
    }
}


ARCHON_TEST(Core_TextFileStream_StatefulCharCodec)
{
    std::mt19937_64 random(test_context.seed_seq());
    std::uniform_int_distribution<std::size_t> buffer_size_distr(0, 8);

    ARCHON_TEST_FILE(path);

    using codec_type = core::test::StatefulCharCodec;
    using traits_type = codec_type::traits_type;
    using stream_type = core::BasicPosixTextFileStream<char, traits_type, codec_type>;
    stream_type::Config config;
    config.disable_autounshift = true;
    config.buffer_size = buffer_size_distr(random);
    config.impl.char_codec_buffer_size = buffer_size_distr(random);
    config.impl.newline_codec_buffer_size = buffer_size_distr(random);
    stream_type stream(path, core::File::Mode::write, std::move(config));
    stream.imbue(test_context.locale);

    stream << "asph";

    auto pos = stream.tellp();
    ARCHON_CHECK_EQUAL(pos, 7);
    ARCHON_CHECK_EQUAL(pos.state().page, 6);

    stream.flush();
    {
        char data[] { 0x16, 0x01, 0x17, 0x03, 0x00, 0x16, 0x08 };
        std::string text = core::File::load(path);
        ARCHON_CHECK_EQUAL_SEQ(text, core::Span(data));
    }

    stream.seekg(0);
    {
        std::array<char, 2> buffer;
        stream.read(buffer.data(), std::streamsize(buffer.size()));
        std::size_t n = std::size_t(stream.gcount());
        ARCHON_CHECK_EQUAL(std::string_view(buffer.data(), n), "as");
    }

    auto pos_2 = stream.tellg();
    ARCHON_CHECK_EQUAL(pos_2, 4);
    ARCHON_CHECK_EQUAL(pos_2.state().page, 7);

    stream.seekp(pos);
    stream << "a";
    stream.unshift();

    auto pos_3 = stream.tellp();
    ARCHON_CHECK_EQUAL(pos_3, 9);
    ARCHON_CHECK_EQUAL(pos_3.state().page, 0);

    stream.flush();
    {
        char data[] { 0x16, 0x01, 0x17, 0x03, 0x00, 0x16, 0x08, 0x01, 0x10 };
        std::string text = core::File::load(path);
        ARCHON_CHECK_EQUAL_SEQ(text, core::Span(data));
    }

    stream.seekg(0);
    {
        std::array<char, 8> buffer;
        stream.read(buffer.data(), std::streamsize(buffer.size()));
        std::size_t n = std::size_t(stream.gcount());
        ARCHON_CHECK_EQUAL(std::string_view(buffer.data(), n), "aspha");
    }
}


ARCHON_TEST(Core_TextFileStream_Autounshift)
{
    std::mt19937_64 random(test_context.seed_seq());
    std::uniform_int_distribution<std::size_t> buffer_size_distr(0, 8);

    ARCHON_TEST_FILE(path);

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 2; ++j) {
            using codec_type = core::test::StatefulCharCodec;
            using traits_type = codec_type::traits_type;
            using stream_type = core::BasicPosixTextFileStream<char, traits_type, codec_type>;
            stream_type::Config config;
            config.disable_autounshift = (j == 1);
            config.buffer_size = buffer_size_distr(random);
            config.impl.char_codec_buffer_size = buffer_size_distr(random);
            config.impl.newline_codec_buffer_size = buffer_size_distr(random);
            stream_type stream(path, core::File::Mode::write, std::move(config));
            stream.imbue(test_context.locale);

            stream << "asph";
            switch (i) {
                case 0:
                    stream.flush();
                    break;
                case 1:
                    stream.seekp(0);
                    break;
                case 2:
                    stream.peek();
                    break;
            }

            std::string text = core::File::load(path);
            if (j == 0) {
                char data[] { 0x16, 0x01, 0x17, 0x03, 0x00, 0x16, 0x08, 0x10 };
                ARCHON_CHECK_EQUAL_SEQ(text, core::Span(data));
            }
            else {
                char data[] { 0x16, 0x01, 0x17, 0x03, 0x00, 0x16, 0x08 };
                ARCHON_CHECK_EQUAL_SEQ(text, core::Span(data));
            }
        }
    }
}


ARCHON_TEST_BATCH(Core_TextFileStream_BufferedTextFileImplementation, buffered_impl_variants)
{
    // While it does not make sense to use a buffered text file implementation with
    // `GenericTextFileStream` (because `GenericTextFileStream` provides a buffering
    // mechanism by itself), it is supposed to work, so it makes sense to check.

    std::mt19937_64 random(test_context.seed_seq());
    std::uniform_int_distribution<std::size_t> buffer_size_distr(0, 8);
    std::locale locale(test_context.locale, std::locale::classic(), std::locale::numeric);

    ARCHON_TEST_FILE(path);

    using impl_type = test_type;
    using stream_type = core::GenericTextFileStream<impl_type>;

    typename stream_type::Config config;
    config.buffer_size = buffer_size_distr(random);
    config.impl.buffer_size = buffer_size_distr(random);
    config.impl.subimpl.char_codec_buffer_size = buffer_size_distr(random);
    config.impl.subimpl.newline_codec_buffer_size = buffer_size_distr(random);
    stream_type stream(path, core::File::Mode::write, std::move(config));
    stream.imbue(locale);
    ARCHON_CHECK(stream);
    ARCHON_CHECK_EQUAL(stream.tellp(), 0);
    stream << 4689;
    ARCHON_CHECK(stream);
    ARCHON_CHECK_EQUAL(stream.tellp(), 4);
    stream.flush();
    ARCHON_CHECK(stream);
    stream.seekg(0);
    ARCHON_CHECK(stream);
    ARCHON_CHECK_EQUAL(stream.tellp(), 0);
    int value = 0;
    stream >> value;
    ARCHON_CHECK(stream);
    ARCHON_CHECK_EQUAL(value, 4689);
    ARCHON_CHECK_EQUAL(stream.tellp(), 4);
}
