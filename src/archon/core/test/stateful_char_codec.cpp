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


#include <archon/core/features.h>

#include <archon/core/test/stateful_char_codec.hpp>


using namespace archon;

using StatefulCharCodec = core::test::StatefulCharCodec;


bool StatefulCharCodec::decode(State& state, core::Span<const char> data, std::size_t& data_offset, bool,
                               core::Span<char> buffer, std::size_t& buffer_offset, bool& error) const noexcept
{
    char page = state.page;
    std::size_t i = data_offset;
    std::size_t j = buffer_offset;
    for (;;) {
        if (ARCHON_UNLIKELY(i == data.size())) {
            state.page = page;
            data_offset = i;
            buffer_offset = j;
            return true;
        }
        char ch = data[i];
        using uchar = unsigned char;
        if (ARCHON_LIKELY(uchar(ch) <= 0xF)) {
            if (ARCHON_UNLIKELY(j == buffer.size())) {
                state.page = page;
                data_offset = i;
                buffer_offset = j;
                error = false;
                return false;
            }
            buffer[j] = char(uchar(uchar(page) << 4 | ch));
            ++j;
        }
        else if (ARCHON_LIKELY(uchar(ch) <= 0x1F)) {
            page = char(ch - 0x10);
        }
        else {
            state.page = page;
            data_offset = i;
            buffer_offset = j;
            error = true;
            return false;
        }
        ++i;
    }
}


bool StatefulCharCodec::encode(State& state, core::Span<const char> data, std::size_t& data_offset,
                               core::Span<char> buffer, std::size_t& buffer_offset, bool& error) const noexcept
{
    char page = state.page;
    std::size_t i = data_offset;
    std::size_t j = buffer_offset;
    for (;;) {
        if (ARCHON_UNLIKELY(i == data.size())) {
            state.page = page;
            data_offset = i;
            buffer_offset = j;
            return true;
        }
        if (ARCHON_UNLIKELY(j == buffer.size())) {
            state.page = page;
            data_offset = i;
            buffer_offset = j;
            error = false;
            return false;
        }
        char ch = data[i];
        using uchar = unsigned char;
        char page_2 = char(uchar(ch) >> 4);
        if (ARCHON_LIKELY(page_2 == page)) {
            buffer[j] = char(uchar(ch) & 0xF);
            ++i;
        }
        else if (ARCHON_LIKELY(page_2 <= 0xF)) {
            page = page_2;
            buffer[j] = 0x10 + page;
        }
        else {
            state.page = page;
            data_offset = i;
            buffer_offset = j;
            error = true;
            return false;
        }
        ++j;
    }
}


bool StatefulCharCodec::unshift(State& state, core::Span<char> buffer, std::size_t& buffer_offset) const noexcept
{
    char page = state.page;
    if (ARCHON_LIKELY(page == 0))
        return true;
    if (ARCHON_LIKELY(buffer_offset < buffer.size())) {
        page = 0;
        buffer[buffer_offset] = 0x10 + page;
        state.page = page;
        ++buffer_offset;
        return true;
    }
    return false;
}


void StatefulCharCodec::simul_decode(State& state, core::Span<const char> data, std::size_t& data_offset,
                                     std::size_t buffer_size) const noexcept
{
    char page = state.page;
    std::size_t i = data_offset;
    std::size_t j = 0;
    for (;;) {
        if (i == data.size())
            break;
        char ch = data[i];
        using uchar = unsigned char;
        if (ARCHON_LIKELY(uchar(ch) <= 0xF)) {
            if (j == buffer_size)
                break;
            ++j;
        }
        else if (ARCHON_LIKELY(uchar(ch) <= 0x1F)) {
            page = char(ch - 0x10);
        }
        else {
            break;
        }
        ++i;
    }
    state.page = page;
    data_offset = i;
}
