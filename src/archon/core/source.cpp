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
#include <system_error>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/source.hpp>


using namespace archon;
using core::Source;


bool Source::try_read(core::Span<char> buffer, std::size_t& n, std::error_code& ec)
{
    core::Span<char> buffer_2 = buffer;
    std::size_t n_2 = 0;
  again:
    if (ARCHON_LIKELY(try_read_some(buffer_2, n_2, ec))) { // Throws
        ARCHON_ASSERT(n_2 <= buffer_2.size());
        if (ARCHON_LIKELY(n_2 != 0 && n_2 < buffer_2.size())) {
            buffer_2 = buffer_2.subspan(n_2);
            goto again;
        }
        n = std::size_t(buffer_2.data() + n_2 - buffer.data());
        return true; // Success
    }
    n = std::size_t(buffer_2.data() - buffer.data());
    return false; // Failure
}
