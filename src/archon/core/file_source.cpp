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
#include <archon/core/file_source.hpp>


using namespace archon;


bool core::FileSource::do_try_read_some(core::Span<char> buffer, std::size_t& n, std::error_code& ec)
{
    return m_file.try_read_some(buffer, n, ec);
}


bool core::BufferedFileSource::do_try_read_some(core::Span<char> buffer, std::size_t& n, std::error_code& ec)
{
    for (;;) {
        std::size_t avail = std::size_t(m_end - m_begin);
        if (avail > 0) {
            std::size_t n_2 = std::min(avail, buffer.size());
            std::copy_n(m_buffer.data() + m_begin, n_2, buffer.data());
            m_begin += n_2;
            n = n_2;
            return true; // Success
        }
        std::size_t n_2 = 0;
        if (ARCHON_LIKELY(m_file.try_read_some(m_buffer, n_2, ec))) {
            if (ARCHON_LIKELY(n_2 > 0)) {
                m_begin = 0;
                m_end   = n_2;
                continue; // Try again
            }
            n = 0; // End of file
            return true; // Success
        }
        return false; // Failure
    }
}
