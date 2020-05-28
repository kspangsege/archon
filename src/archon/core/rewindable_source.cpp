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
#include <algorithm>
#include <system_error>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/rewindable_source.hpp>


using namespace archon;


bool core::RewindableSource::do_try_read_some(core::Span<char> buffer, std::size_t& n, std::error_code& ec)
{
    if (ARCHON_LIKELY(m_offset == m_size)) {
        if (ARCHON_LIKELY(m_released))
            return m_subsource.try_read_some(buffer, n, ec); // Throws
        m_buffer.reserve_extra(1, m_size); // Throws
        core::Span<char> buffer_2 = m_buffer.span().subspan(m_size);
        std::size_t n_2 = 0;
        if (ARCHON_LIKELY(m_subsource.try_read_some(buffer_2, n_2, ec))) { // Throws
            if (ARCHON_LIKELY(n_2 > 0)) {
                m_size += n_2;
                goto read_from_buffer;
            }
            n = 0;
            return true; // Success (end of input)
        }
        return false; // Faulure
    }
  read_from_buffer:
    std::size_t avail = std::size_t(m_size - m_offset);
    std::size_t n_2 = std::min(avail, buffer.size());
    std::copy_n(m_buffer.data() + m_offset, n_2, buffer.data());
    m_offset += n_2;
    n = n_2;
    return true;
}
