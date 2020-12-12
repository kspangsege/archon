// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef ARCHON_X_BASE_X_TEXT_CODEC_HPP
#define ARCHON_X_BASE_X_TEXT_CODEC_HPP

/// \file


#include <string_view>

#include <archon/base/span.hpp>
#include <archon/base/seed_memory_buffer.hpp>
#include <archon/base/char_codec.hpp>
#include <archon/base/newline_codec.hpp>


namespace archon::base {


///
/// Combination of character codec (\ref BasicCharCodec) and newline codec (\ref BasicNewlineCodec).
///
///
template<class C, class T = std::char_traits<C>> class BasicTextCodec {
public:
    using char_type = C;
    using traits_type = T;
    using string_view_type = std::basic_string_view<C, T>;

    base::Span<char> encode(string_view_type string, base::SeedMemoryBuffer<char>&);

    bool inc_encode(std::mbstate_t&, const C*& data_begin, const C* data_end, bool end_of_input,
                    char*& buffer_begin, char*& buffer_end, F flush)
        noexcept(noexcept(flush()));

private:
    base::BasicCharCodec<C, T> m_char_codec;
};








// Implementation

base::Span<char> encode(string_view_type string, base::SeedMemoryBuffer<char>& buffer)
{
    if constexpr (s_is_degenerate) {
        return m_char_codec.encode(string, buffer); // Throws
    }
    else {
        std::array<char, 512> buffer;
        for (;;) {
            char_codec().inc_encode();
        }
    }
}


template<class C, class T> template<class F>
bool BasicTextCodec<C, T>::inc_encode(std::mbstate_t& state, const C*& data_begin,
                                      const C* data_end, bool end_of_input, char*& buffer_begin,
                                      char*& buffer_end, F& flush) noexcept(noexcept(flush()))
{
    if constexpr (s_is_degenerate) {
        return m_char_codec.inc_encode(state, data_begin, data_end, end_of_input, buffer_begin,
                                       buffer_end, flush); // Throws
    }
    else {
        std::array<char, 512> buffer;
        char* buffer_begin_2 = buffer.data();
        char* buffer_end_2   = buffer_begin_2 + buffer.size();
        auto flush_2 = []() noexcept(noexcept(flush())) {
            const char* data_begin_2 = buffer.data();
            const char* data_end_2   = buffer_begin_2;
            bool end_of_input_2 = false;
            bool success_2 = newline_codec_type::inc_encode(data_begin_2, data_end_2, end_of_input_2, ); // Throws
        };
        bool success = m_char_codec.inc_encode(state, data_begin, data_end, end_of_input,
                                               buffer_begin_2, buffer_end_2, flush_2); // Throws
        for (;;) {
            
        }
    }
    
}


} // namespace archon::base

#endif // ARCHON_X_BASE_X_TEXT_CODEC_HPP
