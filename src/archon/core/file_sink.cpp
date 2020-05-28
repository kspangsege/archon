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
#include <archon/core/assert.hpp>
#include <archon/core/file_sink.hpp>


using namespace archon;


bool core::FileSink::do_try_write(core::Span<const char> data, std::size_t& n, std::error_code& ec)
{
    return m_file.try_write(data, n, ec);
}


bool core::BufferedFileSink::do_try_write(core::Span<const char> data, std::size_t& n, std::error_code& ec)
{
    core::Span data_2 = data;
    for (;;) {
        std::size_t capacity = std::size_t(m_buffer.size() - m_end);
        std::size_t n_2 = std::min(data_2.size(), capacity);
        std::copy_n(data_2.data(), n_2, m_buffer.data() + m_end);
        m_end += n_2;
        if (ARCHON_LIKELY(data_2.size() <= capacity)) {
            n = data.size();
            return true; // Success
        }
        data_2 = data_2.subspan(n_2);
        if (ARCHON_LIKELY(try_flush(ec))) { // Throws
            ARCHON_ASSERT(m_end == 0);
            continue;
        }
        n = std::size_t(data.size() - data_2.size());
        return false; // Failure
    }
}


bool core::BufferedFileSink::try_flush(std::error_code& ec)
{
    ARCHON_ASSERT(m_begin <= m_end);
    std::size_t size = std::size_t(m_end - m_begin);
    core::Span data = core::Span(m_buffer).subspan(m_begin, size);
    std::size_t n = 0;
    if (ARCHON_LIKELY(m_file.try_write(data, n, ec))) {
        m_begin = 0;
        m_end   = 0;
        return true; // Success
    }
    m_begin += n;
    return false; // Failure
}
