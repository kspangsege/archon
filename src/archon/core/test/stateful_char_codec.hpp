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

#ifndef ARCHON_X_CORE_X_TEST_X_STATEFUL_CHAR_CODEC_HPP
#define ARCHON_X_CORE_X_TEST_X_STATEFUL_CHAR_CODEC_HPP


#include <cstddef>
#include <string_view>
#include <locale>
#include <ios>

#include <archon/core/type.hpp>
#include <archon/core/span.hpp>


namespace archon::core::test {


// Stateful character codec invented for testing purposes.
//
// This class provides a stateful character codec that satisfies all the requirements of the
// CharCodec concept (see `archon/core/char_codec.hpp`). It is useful for testing components
// that are parameterized by a character codec type such as `core::BasicTextFile` (see
// `archon/core/text_file.hpp`).
//
// The character codec is restricted to 8-bit character values, and divides the 256 possible
// values into 16 code pages (the upper 4 bits). The shift state then specifies current code
// page, which is zero initially. An encoded character is then a byte value in the range [0,
// 15], and the code page is changed by a byte value in the range [16, 31].
//
class StatefulCharCodec {
public:
    struct State {
        char page;
    };

    struct CharTraits : std::char_traits<char> {
        using pos_type   = std::fpos<State>;
        using state_type = State;
    };

    using char_type   = char;
    using traits_type = CharTraits;

    using Config = core::Empty;

    static constexpr bool is_degen = false;

    explicit StatefulCharCodec(const std::locale&, Config = {}) noexcept;
    explicit StatefulCharCodec(const std::locale*, Config) noexcept;
    void imbue(const std::locale&) noexcept;

    bool decode(State&, core::Span<const char> data, std::size_t& data_offset, bool,
                core::Span<char> buffer, std::size_t& buffer_offset, bool& error) const noexcept;
    bool encode(State&, core::Span<const char> data, std::size_t& data_offset,
                core::Span<char> buffer, std::size_t& buffer_offset, bool& error) const noexcept;
    bool unshift(State&, core::Span<char> buffer, std::size_t& buffer_offset) const noexcept;
    void simul_decode(State&, core::Span<const char> data, std::size_t& data_offset,
                      std::size_t buffer_size) const noexcept;

    static constexpr auto max_simul_decode_size() noexcept -> std::size_t;
};








// Implementation


inline StatefulCharCodec::StatefulCharCodec(const std::locale&, Config) noexcept
{
}


inline StatefulCharCodec::StatefulCharCodec(const std::locale*, Config) noexcept
{
}


inline void StatefulCharCodec::imbue(const std::locale&) noexcept
{
}


constexpr auto StatefulCharCodec::max_simul_decode_size() noexcept -> std::size_t
{
    return std::size_t(-1);
}


} // namespace archon::core::test

#endif // ARCHON_X_CORE_X_TEST_X_STATEFUL_CHAR_CODEC_HPP
