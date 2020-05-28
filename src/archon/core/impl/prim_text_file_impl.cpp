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
#include <limits>
#include <algorithm>
#include <stdexcept>
#include <system_error>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/newline_codec.hpp>

#include <archon/core/impl/prim_text_file_impl.hpp>


using namespace archon;

using PrimPosixTextFileImpl   = core::impl::PrimPosixTextFileImpl;
using PrimWindowsTextFileImpl = core::impl::PrimWindowsTextFileImpl;



// ============================ PrimPosixTextFileImpl ============================


bool PrimPosixTextFileImpl::read_ahead(core::Span<char> buffer, bool, std::size_t& n, std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
    m_reading = true;
#endif

    std::size_t buffer_size = buffer.size();
    std::size_t max = std::size_t(std::numeric_limits<std::size_t>::max() - m_retain_size);
    if (ARCHON_UNLIKELY(buffer_size > max)) {
        if (ARCHON_UNLIKELY(max == 0))
            throw std::length_error("Retain size");
        buffer_size = max;
    }

    if (ARCHON_LIKELY(m_file.try_read_some({ buffer.data(), buffer_size }, n, ec))) {
        m_retain_size += n;
        return true;
    }
    return false;
}


bool PrimPosixTextFileImpl::discard(std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    pos_type n = 0;
    if (ARCHON_LIKELY(core::try_int_cast(m_retain_size, n))) {
        auto whence = core::File::Whence::cur;
        pos_type result; // Dummy
        if (ARCHON_LIKELY(n > 0)) {
            if (ARCHON_LIKELY(m_file.try_seek(-n, whence, result, ec))) {
                m_retain_size = 0;
                goto proceed;
            }
            return false; // Failure
        }
      proceed:
#if ARCHON_DEBUG
        m_reading = false;
#endif
        return true; // Success
    }
    throw std::length_error("Retain size");
}



// ============================ PrimWindowsTextFileImpl ============================


bool PrimWindowsTextFileImpl::read_ahead(core::Span<char> buffer, bool dynamic_eof, std::size_t& n,
                                         std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
    m_reading = true;
#endif

    bool end_of_file = false;
    for (;;) {
        core::Span data = core::Span(m_buffer).first(m_end);
        std::size_t buffer_offset = 0;
        std::size_t clear_offset = m_retain_size;
        core::newline_codec::decode(data, m_curr, end_of_file, buffer, buffer_offset, clear_offset, m_retain_clear);
        m_retain_size += buffer_offset;
        if (ARCHON_LIKELY(buffer_offset > 0 || buffer.size() == 0)) {
            n = buffer_offset;
            return true;
        }
        ARCHON_ASSERT(!end_of_file);
        // Move retained data to start of buffer
        ARCHON_ASSERT(m_begin <= m_curr);
        char* base = m_buffer.data();
        std::copy(base + m_begin, base + m_end, base);
        m_curr -= m_begin;
        m_end  -= m_begin;
        m_begin = 0;
        if (ARCHON_UNLIKELY(m_end == m_buffer.size()))
            expand_buffer(); // Throws
        std::size_t n_2 = 0;
        core::Span buffer_2 = core::Span(m_buffer).subspan(m_end);
        if (ARCHON_LIKELY(m_file.try_read_some(buffer_2, n_2, ec))) {
            if (ARCHON_LIKELY(n_2 > 0)) {
                m_end += n_2;
                continue;
            }
            if (ARCHON_LIKELY(m_end == m_curr || dynamic_eof)) {
                // Signal end of file
                n = 0;
                return true;
            }
            end_of_file = true;
            continue;
        }
        return false;
    }
}


bool PrimWindowsTextFileImpl::write(core::Span<const char> data, std::size_t& n, std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_reading);
    m_writing = true;
#endif

    std::size_t data_offset = 0;
    for (;;) {
        core::newline_codec::encode(data, data_offset, m_buffer, m_end);
        if (ARCHON_LIKELY(data_offset == data.size())) {
            n = data.size();
            return true;
        }
        if (ARCHON_LIKELY(m_end > 0)) {
            if (ARCHON_LIKELY(do_flush(ec)))
                continue;
            return false;
        }
        expand_buffer(); // Throws
    }
}


void PrimWindowsTextFileImpl::advance() noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    ARCHON_ASSERT(m_begin <= m_curr);
    m_begin = m_curr;
    m_retain_size  = 0;
    m_retain_clear = 0;
}


void PrimWindowsTextFileImpl::advance(std::size_t n) noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    ARCHON_ASSERT(n <= m_retain_size);
    ARCHON_ASSERT(m_begin <= m_curr);
    ARCHON_ASSERT(m_curr <= m_end);
    if (ARCHON_LIKELY(n >= m_retain_clear)) {
        m_begin = std::size_t(m_curr - (m_retain_size - n));
        m_retain_size -= n;
        m_retain_clear = 0;
        return;
    }
    core::Span data = core::Span(m_buffer).first(m_curr);
    bool success = core::newline_codec::simul_decode(data, m_begin, n);
    ARCHON_ASSERT(success);
    ARCHON_ASSERT(m_begin <= m_curr);
    m_retain_size  -= n;
    m_retain_clear -= n;
}


bool PrimWindowsTextFileImpl::discard(std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    ARCHON_ASSERT(m_begin <= m_end);
    pos_type n = 0;
    if (ARCHON_LIKELY(core::try_int_cast(m_end - m_begin, n))) {
        auto whence = core::File::Whence::cur;
        pos_type result; // Dummy
        if (ARCHON_LIKELY(n > 0)) {
            if (ARCHON_LIKELY(m_file.try_seek(-n, whence, result, ec)))
                goto proceed;
            return false; // Failure
        }
      proceed:
        m_begin = 0;
        m_end   = 0;
        m_curr  = 0;
        m_retain_size  = 0;
        m_retain_clear = 0;
#if ARCHON_DEBUG
        m_reading = false;
#endif
        return true; // Success
    }
    throw std::length_error("Retain size");
}


bool PrimWindowsTextFileImpl::tell_read(pos_type& pos, std::error_code& ec) noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    ARCHON_ASSERT(m_begin <= m_end);
    pos_type result;
    if (ARCHON_LIKELY(m_file.try_seek(0, core::File::Whence::cur, result, ec))) {
        if (ARCHON_LIKELY(core::try_int_sub(result, m_end - m_begin))) {
            pos = result;
            return true;
        }
        // We only get here if the position of the actual fikle pointer was manipulated
        // outside the control of this text file object.
        pos = 0;
        return true;
    }
    return false;
}


bool PrimWindowsTextFileImpl::tell_write(pos_type& pos, std::error_code& ec) noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_reading);
#endif

    pos_type result;
    if (ARCHON_LIKELY(m_file.try_seek(0, core::File::Whence::cur, result, ec))) {
        if (ARCHON_LIKELY(core::try_int_add(result, m_end - m_begin))) {
            pos = result;
            return true;
        }
        ec = make_error_code(std::errc::file_too_large);
    }
    return false;
}


bool PrimWindowsTextFileImpl::do_flush(std::error_code& ec) noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_reading);
#endif

    ARCHON_ASSERT(m_begin <= m_end);
    core::Span data = core::Span(m_buffer).first(m_end).subspan(m_begin);
    std::size_t n = 0;
    if (ARCHON_LIKELY(m_file.try_write(data, n, ec))) {
        m_begin = 0;
        m_end   = 0;
        return true;
    }
    m_begin += n;
    return false;
}
