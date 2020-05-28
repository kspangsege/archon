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
#include <system_error>
#include <locale>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/char_mapper.hpp>
#include <archon/core/quote.hpp>
#include <archon/core/string_formatter.hpp>
#include <archon/core/locale.hpp>
#include <archon/core/file.hpp>
#include <archon/core/text_file_error.hpp>
#include <archon/core/text_file.hpp>
#include <archon/check.hpp>
#include <archon/core/test/stateful_char_codec.hpp>
#include <archon/core/test/locale_utils.hpp>


using namespace archon;


namespace {


ARCHON_TEST_VARIANTS(impl_variants,
                     ARCHON_TEST_TYPE(core::PosixTextFileImpl<char>,              Posix),
                     ARCHON_TEST_TYPE(core::WindowsTextFileImpl<char>,            Windows),
                     ARCHON_TEST_TYPE(core::PosixTextFileImpl<wchar_t>,           WidePosix),
                     ARCHON_TEST_TYPE(core::WindowsTextFileImpl<wchar_t>,         WideWindows),
                     ARCHON_TEST_TYPE(core::BufferedPosixTextFileImpl<char>,      BufferedPosix),
                     ARCHON_TEST_TYPE(core::BufferedWindowsTextFileImpl<char>,    BufferedWindows),
                     ARCHON_TEST_TYPE(core::BufferedPosixTextFileImpl<wchar_t>,   WideBufferedPosix),
                     ARCHON_TEST_TYPE(core::BufferedWindowsTextFileImpl<wchar_t>, WideBufferedWindows));


ARCHON_TEST_VARIANTS(wide_impl_variants,
                     ARCHON_TEST_TYPE(core::PosixTextFileImpl<wchar_t>,           Posix),
                     ARCHON_TEST_TYPE(core::WindowsTextFileImpl<wchar_t>,         Windows),
                     ARCHON_TEST_TYPE(core::BufferedPosixTextFileImpl<wchar_t>,   BufferedPosix),
                     ARCHON_TEST_TYPE(core::BufferedWindowsTextFileImpl<wchar_t>, BufferedWindows));


} // unnamed namespace



ARCHON_TEST_BATCH(Core_TextFile_OpenClose, impl_variants)
{
    using impl_type = test_type;
    using text_file_type = core::GenericTextFile<impl_type>;
    ARCHON_TEST_FILE(path_1);
    ARCHON_TEST_FILE(path_2);
    text_file_type text_file(test_context.locale);
    ARCHON_CHECK_NOT(text_file.is_open());
    text_file.open(path_1, core::File::Mode::write);
    ARCHON_CHECK(text_file.is_open());
    text_file.open(path_2, core::File::Mode::write);
    ARCHON_CHECK(text_file.is_open());
    text_file.close();
    ARCHON_CHECK_NOT(text_file.is_open());
    text_file.open(path_1);
    ARCHON_CHECK(text_file.is_open());
}


ARCHON_TEST_BATCH(Core_TextFile_Read, impl_variants)
{
    using impl_type = test_type;
    using text_file_type = core::GenericTextFile<impl_type>;
    using char_type = typename text_file_type::char_type;
    using string_view_type = std::basic_string_view<char_type>;
    ARCHON_TEST_FILE(path);
    {
        core::File file(path, core::File::Mode::write);
        file.write(std::string_view("foo\r\nbar\r\nbaz\r\n"));
    }
    std::mt19937_64 random(test_context.seed_seq());
    std::uniform_int_distribution<std::size_t> buffer_size_distr(0, 8);
    typename text_file_type::Config config;
    if constexpr (!text_file_type::impl_type::is_buffered) {
        config.impl.char_codec_buffer_size = buffer_size_distr(random);
        config.impl.newline_codec_buffer_size = buffer_size_distr(random);
    }
    else {
        config.impl.subimpl.char_codec_buffer_size = buffer_size_distr(random);
        config.impl.subimpl.newline_codec_buffer_size = buffer_size_distr(random);
        config.impl.buffer_size = buffer_size_distr(random);
    }
    text_file_type text_file(test_context.locale, std::move(config));
    text_file.open(path);
    std::array<char_type, 64> buffer;
    std::size_t n = text_file.read(buffer);
    string_view_type data(buffer.data(), n);
    std::array<char_type, 64> seed_memory;
    core::BasicStringWidener<char_type> widener(test_context.locale, seed_memory);
    if constexpr (text_file_type::impl_type::has_windows_newline_codec) {
        ARCHON_CHECK_EQUAL(data, widener.widen("foo\nbar\nbaz\n"));
    }
    else {
        ARCHON_CHECK_EQUAL(data, widener.widen("foo\r\nbar\r\nbaz\r\n"));
    }
}


ARCHON_TEST_BATCH(Core_TextFile_WriteUnshiftAndFlush, impl_variants)
{
    using impl_type = test_type;
    using text_file_type = core::GenericTextFile<impl_type>;
    using char_type = typename text_file_type::char_type;
    ARCHON_TEST_FILE(path);
    {
        std::mt19937_64 random(test_context.seed_seq());
        std::uniform_int_distribution<std::size_t> buffer_size_distr(0, 8);
        typename text_file_type::Config config;
        if constexpr (!text_file_type::impl_type::is_buffered) {
            config.impl.char_codec_buffer_size = buffer_size_distr(random);
            config.impl.newline_codec_buffer_size = buffer_size_distr(random);
        }
        else {
            config.impl.subimpl.char_codec_buffer_size = buffer_size_distr(random);
            config.impl.subimpl.newline_codec_buffer_size = buffer_size_distr(random);
            config.impl.buffer_size = buffer_size_distr(random);
        }
        text_file_type text_file(test_context.locale, std::move(config));
        text_file.open(path, core::File::Mode::write);
        std::array<char_type, 64> seed_memory;
        core::BasicStringWidener<char_type> widener(test_context.locale, seed_memory);
        text_file.write(widener.widen("foo\nbar\nbaz\n"));
        text_file.unshift();
        text_file.flush();
    }
    core::File file(path);
    std::array<char, 64> buffer;
    std::size_t n = file.read(buffer);
    std::string_view data(buffer.data(), n);
    if constexpr (text_file_type::impl_type::has_windows_newline_codec) {
        ARCHON_CHECK_EQUAL(data, "foo\r\nbar\r\nbaz\r\n");
    }
    else {
        ARCHON_CHECK_EQUAL(data, "foo\nbar\nbaz\n");
    }
}


ARCHON_TEST_BATCH(Core_TextFile_TellAndSeek, impl_variants)
{
    using impl_type = test_type;
    using text_file_type = core::GenericTextFile<impl_type>;
    using char_type = typename text_file_type::char_type;
    using string_view_type = std::basic_string_view<char_type>;
    ARCHON_TEST_FILE(path);
    std::mt19937_64 random(test_context.seed_seq());
    std::uniform_int_distribution<std::size_t> buffer_size_distr(0, 8);
    typename text_file_type::Config config;
    if constexpr (!text_file_type::impl_type::is_buffered) {
        config.impl.char_codec_buffer_size = buffer_size_distr(random);
        config.impl.newline_codec_buffer_size = buffer_size_distr(random);
    }
    else {
        config.impl.subimpl.char_codec_buffer_size = buffer_size_distr(random);
        config.impl.subimpl.newline_codec_buffer_size = buffer_size_distr(random);
        config.impl.buffer_size = buffer_size_distr(random);
    }
    text_file_type text_file(test_context.locale, std::move(config));
    text_file.open(path, core::File::Mode::write);
    std::array<char_type, 64> seed_memory;
    core::BasicStringWidener<char_type> widener(test_context.locale, seed_memory);
    using pos_type = typename text_file_type::pos_type;
    ARCHON_CHECK_EQUAL(text_file.tell(), pos_type(0));
    text_file.write(string_view_type(widener.widen("foo\nbar")));
    pos_type pos = text_file.tell();
    ARCHON_CHECK_EQUAL(text_file.tell(), pos);
    if constexpr (text_file_type::impl_type::has_windows_newline_codec) {
        ARCHON_CHECK_EQUAL(pos, pos_type(8));
    }
    else {
        ARCHON_CHECK_EQUAL(pos, pos_type(7));
    }
    text_file.write(widener.widen("\nbaz\n"));
    if constexpr (text_file_type::impl_type::has_windows_newline_codec) {
        ARCHON_CHECK_EQUAL(text_file.tell(), pos_type(15));
    }
    else {
        ARCHON_CHECK_EQUAL(text_file.tell(), pos_type(12));
    }
    text_file.seek(pos);
    ARCHON_CHECK_EQUAL(text_file.tell(), pos);
    text_file.seek(0);
    ARCHON_CHECK_EQUAL(text_file.tell(), pos_type(0));
    text_file.seek(pos);
    ARCHON_CHECK_EQUAL(text_file.tell(), pos);
    text_file.seek(0);
    text_file.seek(pos);
    std::array<char_type, 64> buffer;
    std::size_t n = text_file.read(buffer);
    ARCHON_CHECK_EQUAL(string_view_type(buffer.data(), n), widener.widen("\nbaz\n"));
}


ARCHON_TEST_BATCH(Core_TextFile_Load, impl_variants)
{
    using impl_type = test_type;
    using text_file_type = core::GenericTextFile<impl_type>;
    using char_type = typename text_file_type::char_type;
    using traits_type = typename text_file_type::traits_type;

    std::array<char_type, 64> seed_memory;
    core::BasicStringWidener<char_type, traits_type> widener(test_context.locale, seed_memory);

    ARCHON_TEST_FILE(path);
    {
        core::File file(path, core::File::Mode::write);
        if constexpr (text_file_type::impl_type::has_windows_newline_codec) {
            file.write(std::string_view("foo\r\nbar\r\n"));
        }
        else {
            file.write(std::string_view("foo\nbar\n"));
        }
    }

    auto string_2 = text_file_type::load(path, test_context.locale);
    ARCHON_CHECK_EQUAL(string_2, widener.widen(std::string_view("foo\nbar\n")));
}


ARCHON_TEST_BATCH(Core_TextFile_Save, impl_variants)
{
    using impl_type = test_type;
    using text_file_type = core::GenericTextFile<impl_type>;
    using char_type = typename text_file_type::char_type;
    using traits_type = typename text_file_type::traits_type;

    std::array<char_type, 64> seed_memory;
    core::BasicStringWidener<char_type, traits_type> widener(test_context.locale, seed_memory);

    ARCHON_TEST_FILE(path);
    text_file_type::save(path, widener.widen("foo\nbar\n"), test_context.locale);

    std::string string = core::File::load(path);
    if constexpr (text_file_type::impl_type::has_windows_newline_codec) {
        ARCHON_CHECK_EQUAL(string, "foo\r\nbar\r\n");
    }
    else {
        ARCHON_CHECK_EQUAL(string, "foo\nbar\n");
    }
}


ARCHON_TEST_BATCH(Core_TextFile_LoadAndChomp, impl_variants)
{
    using impl_type = test_type;
    using text_file_type = core::GenericTextFile<impl_type>;
    using char_type = typename text_file_type::char_type;
    using traits_type = typename text_file_type::traits_type;

    std::array<char_type, 64> seed_memory;
    core::BasicStringWidener<char_type, traits_type> widener(test_context.locale, seed_memory);

    ARCHON_TEST_FILE(path);
    {
        core::File file(path, core::File::Mode::write);
        if constexpr (text_file_type::impl_type::has_windows_newline_codec) {
            file.write(std::string_view("foo\r\nbar\r\n"));
        }
        else {
            file.write(std::string_view("foo\nbar\n"));
        }
    }

    auto string_2 = text_file_type::load_and_chomp(path, test_context.locale);
    ARCHON_CHECK_EQUAL(string_2, widener.widen(std::string_view("foo\nbar")));
}


ARCHON_TEST_BATCH(Core_TextFile_ReopenWhileDirty, impl_variants)
{
    using impl_type = test_type;
    using text_file_type = core::GenericTextFile<impl_type>;
    using char_type = typename text_file_type::char_type;
    using traits_type = typename text_file_type::traits_type;

    std::mt19937_64 random(test_context.seed_seq());
    std::uniform_int_distribution<std::size_t> buffer_size_distr(0, 8);
    std::array<char_type, 64> seed_memory;
    core::BasicStringWidener<char_type, traits_type> widener(test_context.locale, seed_memory);

    ARCHON_TEST_FILE(path);
    text_file_type::save(path, widener.widen(std::string_view("foo\nbar\n")), test_context.locale);

    typename text_file_type::Config config;
    if constexpr (!text_file_type::impl_type::is_buffered) {
        config.impl.char_codec_buffer_size = buffer_size_distr(random);
        config.impl.newline_codec_buffer_size = buffer_size_distr(random);
    }
    else {
        config.impl.subimpl.char_codec_buffer_size = buffer_size_distr(random);
        config.impl.subimpl.newline_codec_buffer_size = buffer_size_distr(random);
        config.impl.buffer_size = buffer_size_distr(random);
    }
    text_file_type text_file(test_context.locale, std::move(config));
    text_file.open(path);
    std::array<char_type, 2> buffer;
    std::size_t n = text_file.read(buffer);
    ARCHON_CHECK_EQUAL(std::basic_string_view(buffer.data(), n), widener.widen(std::string_view("fo")));

    text_file.open(path, core::File::Mode::update);
    n = text_file.read(buffer);
    ARCHON_CHECK_EQUAL(std::basic_string_view(buffer.data(), n), widener.widen(std::string_view("fo")));
    text_file.write(widener.widen(std::string_view("o\nm")));
    text_file.flush();

    text_file.open(path, core::File::Mode::update);
    text_file.write(widener.widen(std::string_view("r")));
    text_file.flush();

    auto string = text_file_type::load(path, test_context.locale);
    ARCHON_CHECK_EQUAL(string, widener.widen(std::string_view("roo\nmar\n")));
}


ARCHON_TEST_BATCH(Core_TextFile_StrictModeDecodeError, impl_variants)
{
    using impl_type = test_type;
    using text_file_type = core::GenericTextFile<impl_type>;
    using char_type = typename text_file_type::char_type;
    using string_view_type = std::basic_string_view<char_type>;

    ARCHON_TEST_FILE(path);

    std::mt19937_64 random(test_context.seed_seq());
    std::uniform_int_distribution<std::size_t> buffer_size_distr(0, 8);

    auto subtest = [&, &parent_test_context = test_context](const std::locale& locale, char ch) {
        ARCHON_TEST_TRAIL(parent_test_context, core::quoted(std::string_view(locale.name())));

        std::string string = "foo\r\nbar\r\nbaz\r\n";
        string[11] = ch; // Invalid byte at offset 11
        {
            core::File file(path, core::File::Mode::write);
            file.write(string);
        }

        typename text_file_type::Config config;
        if constexpr (!text_file_type::impl_type::is_buffered) {
            config.impl.char_codec_buffer_size = buffer_size_distr(random);
            config.impl.newline_codec_buffer_size = buffer_size_distr(random);
        }
        else {
            config.impl.subimpl.char_codec_buffer_size = buffer_size_distr(random);
            config.impl.subimpl.newline_codec_buffer_size = buffer_size_distr(random);
            config.impl.buffer_size = buffer_size_distr(random);
        }
        text_file_type text_file(locale, std::move(config));
        text_file.open(path);
        std::array<char_type, 64> buffer;
        std::size_t n = 0;
        std::error_code ec;
        if (ARCHON_CHECK_NOT(text_file.try_read(buffer, n, ec))) {
            std::array<char_type, 64> seed_memory;
            core::BasicStringWidener<char_type> widener(locale, seed_memory);
            if constexpr (text_file_type::impl_type::has_windows_newline_codec) {
                if (ARCHON_CHECK_EQUAL(n, 9)) {
                    string_view_type data(buffer.data(), n);
                    ARCHON_CHECK_EQUAL(data, widener.widen("foo\nbar\nb"));
                }
            }
            else {
                if (ARCHON_CHECK_EQUAL(n, 11)) {
                    string_view_type data(buffer.data(), n);
                    ARCHON_CHECK_EQUAL(data, widener.widen("foo\r\nbar\r\nb"));
                }
            }
            ARCHON_CHECK_EQUAL(ec, core::TextFileError::invalid_byte_seq);
            ARCHON_CHECK_EQUAL(text_file.tell(), 11);
        }
    };

    for (const std::locale& locale : core::test::candidate_locales) {
        char ch;
        if (core::test::find_decode_error<char_type>(locale, ch))
            subtest(locale, ch);
    }
}


ARCHON_TEST_BATCH(Core_TextFile_StrictModeEncodeError, impl_variants)
{
    using impl_type = test_type;
    using text_file_type = core::GenericTextFile<impl_type>;
    using char_type = typename text_file_type::char_type;
    using string_type = std::basic_string<char_type>;

    ARCHON_TEST_FILE(path);

    std::mt19937_64 random(test_context.seed_seq());
    std::uniform_int_distribution<std::size_t> buffer_size_distr(0, 8);

    auto subtest = [&, &parent_test_context = test_context](const std::locale& locale, char_type ch) {
        ARCHON_TEST_TRAIL(parent_test_context, core::quoted(std::string_view(locale.name())));

        std::array<char_type, 64> seed_memory_2;
        core::BasicStringWidener<char_type> widener(locale, seed_memory_2);

        string_type string(widener.widen("foo\nbar\nbaz\n"));
        string[9] = ch; // Invalid character at offset 9

        {
            typename text_file_type::Config config;
            if constexpr (!text_file_type::impl_type::is_buffered) {
                config.impl.char_codec_buffer_size = buffer_size_distr(random);
                config.impl.newline_codec_buffer_size = buffer_size_distr(random);
            }
            else {
                config.impl.subimpl.char_codec_buffer_size = buffer_size_distr(random);
                config.impl.subimpl.newline_codec_buffer_size = buffer_size_distr(random);
                config.impl.buffer_size = buffer_size_distr(random);
            }
            text_file_type text_file(locale, std::move(config));
            text_file.open(path, core::File::Mode::write);
            std::size_t n = 0;
            std::error_code ec, ec_2;
            if (text_file.try_write(string, n, ec)) {
                // All was written, but not flushed
                bool good = (ARCHON_CHECK_EQUAL(n, 12) &&
                             ARCHON_CHECK_NOT(ec) &&
                             ARCHON_CHECK_NOT(text_file.try_flush(ec)) &&
                             ARCHON_CHECK_EQUAL(ec, core::TextFileError::invalid_char));
                if (!good)
                    return;
            }
            else if (n > 9) {
                // All was not written, but the bad character was
                bool good = (ARCHON_CHECK_GREATER_EQUAL(n, 10) &&
                             ARCHON_CHECK_LESS_EQUAL(n, 11) &&
                             ARCHON_CHECK_EQUAL(ec, core::TextFileError::invalid_char) &&
                             ARCHON_CHECK_NOT(text_file.try_flush(ec_2)) &&
                             ARCHON_CHECK_EQUAL(ec_2, core::TextFileError::invalid_char));
                if (!good)
                    return;
            }
            else {
                // The bad character was not written, but everything preceding it was
                bool good = (ARCHON_CHECK_EQUAL(n, 9) &&
                             ARCHON_CHECK_EQUAL(ec, core::TextFileError::invalid_char) &&
                             ARCHON_CHECK_NO_ERROR(text_file.try_flush(ec_2), ec_2) &&
                             ARCHON_CHECK_NOT(ec_2));
                if (!good)
                    return;
            }
        }
        core::File file(path);
        std::array<char, 64> buffer;
        std::size_t n = file.read(buffer);
        std::string_view data(buffer.data(), n);
        if constexpr (text_file_type::impl_type::has_windows_newline_codec) {
            ARCHON_CHECK_EQUAL(data, "foo\r\nbar\r\nb");
        }
        else {
            ARCHON_CHECK_EQUAL(data, "foo\nbar\nb");
        }
    };

    for (const std::locale& locale : core::test::candidate_locales) {
        char_type ch;
        if (core::test::find_encode_error<char_type>(locale, ch))
            subtest(locale, ch);
    }
}


ARCHON_TEST_BATCH(Core_TextFile_LenientModeDecodeError, impl_variants)
{
    using impl_type = test_type;
    using text_file_type = core::GenericTextFile<impl_type>;
    using char_type = typename text_file_type::char_type;
    using string_view_type = std::basic_string_view<char_type>;

    ARCHON_TEST_FILE(path);

    std::mt19937_64 random(test_context.seed_seq());
    std::uniform_int_distribution<std::size_t> buffer_size_distr(0, 8);

    auto subtest = [&, &parent_test_context = test_context](const std::locale& locale, char ch) {
        ARCHON_TEST_TRAIL(parent_test_context, core::quoted(std::string_view(locale.name())));

        std::string string = "foo\r\nbar\r\nbaz\r\n";
        string[11] = ch; // Invalid byte at offset 11
        {
            core::File file(path, core::File::Mode::write);
            file.write(string);
        }

        typename text_file_type::Config config;
        if constexpr (!text_file_type::impl_type::is_buffered) {
            config.impl.char_codec.lenient = true;
            config.impl.char_codec.use_fallback_replacement_char = true;
            config.impl.char_codec_buffer_size = buffer_size_distr(random);
            config.impl.newline_codec_buffer_size = buffer_size_distr(random);
        }
        else {
            config.impl.subimpl.char_codec.lenient = true;
            config.impl.subimpl.char_codec.use_fallback_replacement_char = true;
            config.impl.subimpl.char_codec_buffer_size = buffer_size_distr(random);
            config.impl.subimpl.newline_codec_buffer_size = buffer_size_distr(random);
            config.impl.buffer_size = buffer_size_distr(random);
        }
        text_file_type text_file(locale, std::move(config));
        text_file.open(path);
        std::array<char_type, 64> buffer;
        std::size_t n = 0;
        std::error_code ec;
        if (ARCHON_CHECK_NO_ERROR(text_file.try_read(buffer, n, ec), ec)) {
            std::array<char_type, 64> seed_memory;
            core::BasicStringWidener<char_type> widener(locale, seed_memory);
            string_view_type expected;
            if constexpr (text_file_type::impl_type::has_windows_newline_codec) {
                expected = widener.widen("foo\nbar\nb?z\n");
            }
            else {
                expected = widener.widen("foo\r\nbar\r\nb?z\r\n");
            }
            string_view_type data(buffer.data(), n);
            ARCHON_CHECK_EQUAL(data, expected);
        }
    };

    for (const std::locale& locale : core::test::candidate_locales) {
        char ch;
        if (core::test::find_decode_error<char_type>(locale, ch))
            subtest(locale, ch);
    }
}


ARCHON_TEST_BATCH(Core_TextFile_LenientModeEncodeError, impl_variants)
{
    using impl_type = test_type;
    using text_file_type = core::GenericTextFile<impl_type>;
    using char_type = typename text_file_type::char_type;
    using string_type = std::basic_string<char_type>;

    ARCHON_TEST_FILE(path);

    std::mt19937_64 random(test_context.seed_seq());
    std::uniform_int_distribution<std::size_t> buffer_size_distr(0, 8);

    auto subtest = [&, &parent_test_context = test_context](const std::locale& locale, char_type ch) {
        ARCHON_TEST_TRAIL(parent_test_context, core::quoted(std::string_view(locale.name())));

        std::array<char_type, 64> seed_memory_2;
        core::BasicStringWidener<char_type> widener(locale, seed_memory_2);

        string_type string(widener.widen("foo\nbar\nbaz\n"));
        string[9] = ch; // Invalid byte at offset 9

        {
            typename text_file_type::Config config;
            if constexpr (!text_file_type::impl_type::is_buffered) {
                config.impl.char_codec.lenient = true;
                config.impl.char_codec.use_fallback_replacement_char = true;
                config.impl.char_codec_buffer_size = buffer_size_distr(random);
                config.impl.newline_codec_buffer_size = buffer_size_distr(random);
            }
            else {
                config.impl.subimpl.char_codec.lenient = true;
                config.impl.subimpl.char_codec.use_fallback_replacement_char = true;
                config.impl.subimpl.char_codec_buffer_size = buffer_size_distr(random);
                config.impl.subimpl.newline_codec_buffer_size = buffer_size_distr(random);
                config.impl.buffer_size = buffer_size_distr(random);
            }
            text_file_type text_file(locale, std::move(config));
            text_file.open(path, core::File::Mode::write);
            std::size_t n = 0;
            std::error_code ec;
            if (!ARCHON_CHECK_NO_ERROR(text_file.try_write(string, n, ec), ec))
                return;
            if (!ARCHON_CHECK_EQUAL(n, string.size()))
                return;
            if (!ARCHON_CHECK_NO_ERROR(text_file.try_flush(ec), ec))
                return;
        }
        core::File file(path);
        std::array<char, 64> buffer;
        std::size_t n = file.read(buffer);
        std::string_view data(buffer.data(), n);
        if constexpr (text_file_type::impl_type::has_windows_newline_codec) {
            ARCHON_CHECK_EQUAL(data, "foo\r\nbar\r\nb?z\r\n");
        }
        else {
            ARCHON_CHECK_EQUAL(data, "foo\nbar\nb?z\n");
        }
    };

    for (const std::locale& locale : core::test::candidate_locales) {
        char_type ch;
        if (core::test::find_encode_error<char_type>(locale, ch))
            subtest(locale, ch);
    }
}


ARCHON_TEST_BATCH(Core_TextFile_WithoutDynamicEndOfFile, wide_impl_variants)
{
    using impl_type = test_type;
    using text_file_type = core::GenericTextFile<impl_type>;
    using char_type = typename text_file_type::char_type;
    using traits_type = typename text_file_type::traits_type;

    std::mt19937_64 random(test_context.seed_seq());
    std::uniform_int_distribution<std::size_t> buffer_size_distr(0, 8);

    ARCHON_TEST_FILE(path);
    {
        core::File file(path, core::File::Mode::write);
        file.write(std::string_view("$\xE2\x82")); // First two bytes of euro sign
    }

    auto subtest = [&, &parent_test_context = test_context](const std::locale& locale) {
        ARCHON_TEST_TRAIL(parent_test_context, core::quoted(std::string_view(locale.name())));

        typename text_file_type::Config config;
        if constexpr (!text_file_type::impl_type::is_buffered) {
            config.impl.char_codec_buffer_size = buffer_size_distr(random);
            config.impl.newline_codec_buffer_size = buffer_size_distr(random);
        }
        else {
            config.impl.subimpl.char_codec_buffer_size = buffer_size_distr(random);
            config.impl.subimpl.newline_codec_buffer_size = buffer_size_distr(random);
            config.impl.buffer_size = buffer_size_distr(random);
        }
        text_file_type text_file(locale, std::move(config));
        text_file.open(path);
        std::array<char_type, 64> buffer;
        std::size_t n = 0;
        std::error_code ec;
        bool success = text_file.try_read(buffer, n, ec);
        if (ARCHON_UNLIKELY(!ARCHON_CHECK_NOT(success)))
            return;
        if (ARCHON_UNLIKELY(!ARCHON_CHECK_EQUAL(ec, core::TextFileError::invalid_byte_seq)))
            return;
        if (ARCHON_UNLIKELY(!ARCHON_CHECK_EQUAL(n, 1)))
            return;
        if (ARCHON_UNLIKELY(!ARCHON_CHECK_EQUAL(buffer[0], traits_type::to_char_type(0x24))))
            return;
        if (ARCHON_UNLIKELY(!ARCHON_CHECK_EQUAL(text_file.tell(), 1)))
            return;
    };

    for (const std::locale& locale : core::test::candidate_locales) {
        bool is_utf8 = (core::assume_utf8_locale(locale) && (core::assume_unicode_locale(locale) || ARCHON_WINDOWS));
        if (is_utf8)
            subtest(locale);
    }
}


ARCHON_TEST_BATCH(Core_TextFile_WithDynamicEndOfFile, wide_impl_variants)
{
    using impl_type = test_type;
    using text_file_type = core::GenericTextFile<impl_type>;
    using char_type = typename text_file_type::char_type;
    using traits_type = typename text_file_type::traits_type;

    std::mt19937_64 random(test_context.seed_seq());
    std::uniform_int_distribution<std::size_t> buffer_size_distr(0, 8);

    ARCHON_TEST_FILE(path);

    auto subtest = [&, &parent_test_context = test_context](const std::locale& locale) {
        ARCHON_TEST_TRAIL(parent_test_context, core::quoted(std::string_view(locale.name())));

        core::File file(path, core::File::Mode::write);
        file.write(std::string_view("$\xE2\x82")); // First two bytes of euro sign

        typename text_file_type::Config config;
        config.dynamic_eof = true;
        if constexpr (!text_file_type::impl_type::is_buffered) {
            config.impl.char_codec_buffer_size = buffer_size_distr(random);
            config.impl.newline_codec_buffer_size = buffer_size_distr(random);
        }
        else {
            config.impl.subimpl.char_codec_buffer_size = buffer_size_distr(random);
            config.impl.subimpl.newline_codec_buffer_size = buffer_size_distr(random);
            config.impl.buffer_size = buffer_size_distr(random);
        }
        text_file_type text_file(locale, std::move(config));
        text_file.open(path);
        std::array<char_type, 64> buffer;
        std::size_t n = 0;
        std::error_code ec;
        bool success = text_file.try_read(buffer, n, ec);
        if (ARCHON_UNLIKELY(!ARCHON_CHECK_NO_ERROR(success, ec)))
            return;
        if (ARCHON_UNLIKELY(!ARCHON_CHECK_EQUAL(n, 1)))
            return;
        if (ARCHON_UNLIKELY(!ARCHON_CHECK_EQUAL(buffer[0], traits_type::to_char_type(0x24))))
            return;
        if (ARCHON_UNLIKELY(!ARCHON_CHECK_EQUAL(text_file.tell(), 1)))
            return;

        text_file.seek(0);
        success = text_file.try_read(buffer, n, ec);
        if (ARCHON_UNLIKELY(!ARCHON_CHECK_NO_ERROR(success, ec)))
            return;
        if (ARCHON_UNLIKELY(!ARCHON_CHECK_EQUAL(n, 1)))
            return;
        if (ARCHON_UNLIKELY(!ARCHON_CHECK_EQUAL(buffer[0], traits_type::to_char_type(0x24))))
            return;

        file.write(std::string_view("\xAC$"));
        success = text_file.try_read(buffer, n, ec);
        if (ARCHON_UNLIKELY(!ARCHON_CHECK_NO_ERROR(success, ec)))
            return;
        if (ARCHON_UNLIKELY(!ARCHON_CHECK_EQUAL(n, 2)))
            return;
        if (ARCHON_UNLIKELY(!ARCHON_CHECK_EQUAL(buffer[0], traits_type::to_char_type(0x20AC))))
            return;
        if (ARCHON_UNLIKELY(!ARCHON_CHECK_EQUAL(buffer[1], traits_type::to_char_type(0x24))))
            return;
    };

    for (const std::locale& locale : core::test::candidate_locales) {
        bool is_utf8 = (core::assume_utf8_locale(locale) && (core::assume_unicode_locale(locale) || ARCHON_WINDOWS));
        if (is_utf8)
            subtest(locale);
    }
}


ARCHON_TEST(Core_TextFile_StatefulCharCodec)
{
    ARCHON_TEST_FILE(path);

    std::mt19937_64 random(test_context.seed_seq());
    std::uniform_int_distribution<std::size_t> buffer_size_distr(0, 8);
    using codec_type = core::test::StatefulCharCodec;
    using traits_type = codec_type::traits_type;
    using file_type = core::BasicPosixTextFile<char, traits_type, codec_type>;
    file_type::Config config;
    config.disable_autounshift = true;
    config.impl.char_codec_buffer_size = buffer_size_distr(random);
    config.impl.newline_codec_buffer_size = buffer_size_distr(random);
    file_type file(test_context.locale, std::move(config));
    file.open(path, core::File::Mode::write);

    file.write(std::string_view("asph"));

    auto pos = file.tell();
    ARCHON_CHECK_EQUAL(pos, 7);
    ARCHON_CHECK_EQUAL(pos.state().page, 6);

    file.flush();
    {
        char data[] { 0x16, 0x01, 0x17, 0x03, 0x00, 0x16, 0x08 };
        std::string text = core::File::load(path);
        ARCHON_CHECK_EQUAL_SEQ(text, core::Span(data));
    }

    file.seek(0);
    {
        std::array<char, 2> buffer;
        std::size_t n = file.read(buffer);
        ARCHON_CHECK_EQUAL(std::string_view(buffer.data(), n), "as");
    }

    auto pos_2 = file.tell();
    ARCHON_CHECK_EQUAL(pos_2, 4);
    ARCHON_CHECK_EQUAL(pos_2.state().page, 7);

    file.seek(pos);
    file.write(std::string_view("a"));
    file.unshift();

    auto pos_3 = file.tell();
    ARCHON_CHECK_EQUAL(pos_3, 9);
    ARCHON_CHECK_EQUAL(pos_3.state().page, 0);

    file.flush();
    {
        char data[] { 0x16, 0x01, 0x17, 0x03, 0x00, 0x16, 0x08, 0x01, 0x10 };
        std::string text = core::File::load(path);
        ARCHON_CHECK_EQUAL_SEQ(text, core::Span(data));
    }

    file.seek(0);
    {
        std::array<char, 8> buffer;
        std::size_t n = file.read(buffer);
        ARCHON_CHECK_EQUAL(std::string_view(buffer.data(), n), "aspha");
    }
}


ARCHON_TEST(Core_TextFile_Autounshift)
{
    ARCHON_TEST_FILE(path);

    using codec_type = core::test::StatefulCharCodec;
    using traits_type = codec_type::traits_type;
    using file_type = core::BasicPosixTextFile<char, traits_type, codec_type>;
    std::mt19937_64 random(test_context.seed_seq());
    std::uniform_int_distribution<std::size_t> buffer_size_distr(0, 8);
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 2; ++j) {
            file_type::Config config;
            config.disable_autounshift = (j == 1);
            config.impl.char_codec_buffer_size = buffer_size_distr(random);
            config.impl.newline_codec_buffer_size = buffer_size_distr(random);
            file_type file(test_context.locale, std::move(config));
            file.open(path, core::File::Mode::write);

            file.write(std::string_view("asph"));
            std::array<char, 1> buffer;
            switch (i) {
                case 0:
                    file.flush();
                    break;
                case 1:
                    file.seek(0);
                    break;
                case 2:
                    file.read_some(buffer);
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


ARCHON_TEST(Core_TextFile_DoubleBuffered)
{
    // While it probably does not make sense to use multiple layers of buffering, it is
    // supposed to work, so it makes sense to check.

    using impl_type_1 = core::BufferedTextFileImpl<char>;
    using impl_type_2 = core::GenericBufferedTextFileImpl<impl_type_1>;
    using file_type = core::GenericTextFile<impl_type_2>;

    ARCHON_TEST_FILE(path);
    std::mt19937_64 random(test_context.seed_seq());
    std::uniform_int_distribution<std::size_t> buffer_size_distr(0, 8);
    typename file_type::Config config;
    config.impl.buffer_size = buffer_size_distr(random);
    config.impl.subimpl.buffer_size = buffer_size_distr(random);
    config.impl.subimpl.subimpl.newline_codec_buffer_size = buffer_size_distr(random);
    file_type file(test_context.locale, std::move(config));
    file.open(path, core::File::Mode::write);
    std::string_view string = "foo\nbar\nbaz\n";
    file.write(string);
    file.unshift();
    file.flush();
    file.seek(0);
    std::array<char, 64> buffer;
    std::size_t n = file.read(buffer);
    ARCHON_CHECK_EQUAL(std::string_view(buffer.data(), n), string);
}
