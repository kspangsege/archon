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
#include <random>
#include <system_error>

#include <archon/core/file.hpp>
#include <archon/core/impl/prim_text_file_impl.hpp>
#include <archon/check.hpp>


using namespace archon;


namespace {

ARCHON_TEST_VARIANTS(variants,
                     ARCHON_TEST_TYPE(core::impl::PrimPosixTextFileImpl,   Posix),
                     ARCHON_TEST_TYPE(core::impl::PrimWindowsTextFileImpl, Windows));

} // unnamed namespace


ARCHON_TEST_BATCH(Core_Impl_PrimTextFileImpl, variants)
{
    ARCHON_TEST_FILE(path);
    core::File file(path, core::File::Mode::write);
    std::mt19937_64 random(test_context.seed_seq());
    using impl_type = test_type;
    typename impl_type::Config config;
    config.newline_codec_buffer_size = std::uniform_int_distribution<std::size_t>(0, 8)(random);
    impl_type text_file_impl(file, std::move(config));
    text_file_impl.reset();
    std::array<char, 64> buffer;
    std::size_t n = 0;
    bool success;
    std::error_code ec;

    auto read_ahead_all = [&](core::Span<char> buffer, std::size_t& n, std::error_code& ec) {
        bool dynamic_eof = false;
        auto buffer_2 = buffer;
        std::size_t n_2 = 0;
      again:
        if (ARCHON_LIKELY(text_file_impl.read_ahead(buffer_2, dynamic_eof, n_2, ec))) {
            buffer_2 = buffer_2.subspan(n_2);
            if (n_2 != 0 && buffer_2.size() > 0)
                goto again;
            n = std::size_t(buffer_2.data() - buffer.data());
            return true;
        }
        return false;
    };

    success = text_file_impl.write(std::string_view("foo\nbar\nbaz\n"), n, ec);
    if (!ARCHON_CHECK_NO_ERROR(success, ec))
        return;
    if (!ARCHON_CHECK_EQUAL(n, 12))
        return;

    success = text_file_impl.flush(ec);
    if (!ARCHON_CHECK_NO_ERROR(success, ec))
        return;

    std::size_t m = (impl_type::has_windows_newline_codec ? 5 : 4);
    success = text_file_impl.seek(m, ec);
    if (!ARCHON_CHECK_NO_ERROR(success, ec))
        return;

    n = 0;
    success = read_ahead_all(buffer, n, ec);
    if (!ARCHON_CHECK_NO_ERROR(success, ec))
        return;
    if (!ARCHON_CHECK_EQUAL(std::string_view(buffer.data(), n), "bar\nbaz\n"))
        return;

    text_file_impl.advance(5);
    success = text_file_impl.discard(ec);
    if (!ARCHON_CHECK_NO_ERROR(success, ec))
        return;

    n = 0;
    success = text_file_impl.write(std::string_view("o"), n, ec);
    if (!ARCHON_CHECK_NO_ERROR(success, ec))
        return;
    if (!ARCHON_CHECK_EQUAL(n, 1))
        return;

    success = text_file_impl.flush(ec);
    if (!ARCHON_CHECK_NO_ERROR(success, ec))
        return;

    success = text_file_impl.seek(0, ec);
    if (!ARCHON_CHECK_NO_ERROR(success, ec))
        return;

    n = 0;
    success = read_ahead_all(buffer, n, ec);
    if (!ARCHON_CHECK_NO_ERROR(success, ec))
        return;
    if (!ARCHON_CHECK_EQUAL(std::string_view(buffer.data(), n), "foo\nbar\nboz\n"))
        return;
}
