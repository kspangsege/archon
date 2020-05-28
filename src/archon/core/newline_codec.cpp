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
#include <archon/core/assert.hpp>

#include <archon/core/newline_codec.hpp>


using namespace archon;
namespace codec = core::newline_codec;


void codec::decode(core::Span<const char> data, std::size_t& data_offset, bool end_of_data, core::Span<char> buffer,
                   std::size_t& buffer_offset, std::size_t clear_offset, std::size_t& clear) noexcept
{
    ARCHON_ASSERT(data_offset <= data.size());
    ARCHON_ASSERT(buffer_offset <= buffer.size());

    // CR+LF -> NL
    std::size_t i = data_offset, i_2;
    std::size_t j = buffer_offset;
    std::size_t k = clear;
    while (ARCHON_LIKELY(i < data.size() && j < buffer.size())) {
        char ch = data[i];
        if (ARCHON_LIKELY(ch != '\r')) {
          cr:
            buffer[j++] = ch;
            ++i;
            continue;
        }
        i_2 = i + 1;
        if (ARCHON_LIKELY(i_2 < data.size())) {
            char ch_2 = data[i_2];
            if (ARCHON_LIKELY(ch_2 == '\n')) {
                buffer[j++] = '\n';
                k = clear_offset + j;
                i = i_2 + 1;
                continue;
            }
            goto cr;
        }
        if (!end_of_data)
            break;
        goto cr;
    }
    data_offset   = i;
    buffer_offset = j;
    clear         = k;
}


void codec::encode(core::Span<const char> data, std::size_t& data_offset,
                   core::Span<char> buffer, std::size_t& buffer_offset) noexcept
{
    ARCHON_ASSERT(data_offset <= data.size());
    ARCHON_ASSERT(buffer_offset <= buffer.size());

    // NL -> CR+LF
    std::size_t i = data_offset;
    std::size_t j = buffer_offset;
    while (ARCHON_LIKELY(i < data.size())) {
        char ch = data[i];
        if (ARCHON_LIKELY(ch != '\n')) {
            if (ARCHON_UNLIKELY(j == buffer.size()))
                break;
            buffer[j++] = ch;
        }
        else {
            if (ARCHON_UNLIKELY(buffer.size() - j < 2))
                break;
            buffer[j++] = '\r';
            buffer[j++] = '\n';
        }
        ++i;
    }
    data_offset   = i;
    buffer_offset = j;
}


bool codec::simul_decode(core::Span<const char> data, std::size_t& data_offset, std::size_t buffer_size) noexcept
{
    ARCHON_ASSERT(data_offset <= data.size());

    // CR+LF -> NL
    std::size_t i = data_offset;
    std::size_t j = 0;
    std::size_t i_2;
    while (ARCHON_LIKELY(j < buffer_size)) {
        if (ARCHON_LIKELY(i < data.size())) {
            char ch = data[i];
            if (ARCHON_LIKELY(ch != '\r')) {
              regular:
                ++j;
                ++i;
                continue;
            }
            i_2 = i + 1;
            if (ARCHON_LIKELY(i_2 < data.size())) {
                char ch_2 = data[i_2];
                if (ARCHON_LIKELY(ch_2 == '\n')) {
                    ++j;
                    i = i_2 + 1;
                    continue;
                }
            }
            goto regular;
        }
        return false;
    }
    data_offset = i;
    return true;
}
