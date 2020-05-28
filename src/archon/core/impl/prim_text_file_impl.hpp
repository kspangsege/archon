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

#ifndef ARCHON_X_CORE_X_IMPL_X_PRIM_TEXT_FILE_IMPL_HPP
#define ARCHON_X_CORE_X_IMPL_X_PRIM_TEXT_FILE_IMPL_HPP

/// \file


#include <cstddef>
#include <system_error>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/file.hpp>
#include <archon/core/text_file_config.hpp>


namespace archon::core::impl {


class PrimPosixTextFileImpl;
class PrimWindowsTextFileImpl;


#if ARCHON_WINDOWS
using PrimTextFileImpl = PrimWindowsTextFileImpl;
#else
using PrimTextFileImpl = PrimPosixTextFileImpl;
#endif



class PrimPosixTextFileImpl {
public:
    using Config   = core::PrimTextFileImplConfig;
    using pos_type = core::File::offset_type;

    static constexpr bool has_windows_newline_codec = false;

    explicit PrimPosixTextFileImpl(core::File&, Config) noexcept;

    void reset() noexcept;

    [[nodiscard]] bool read_ahead(core::Span<char> buffer, bool dynamic_eof, std::size_t& n, std::error_code&);

    [[nodiscard]] bool write(core::Span<const char> data, std::size_t& n, std::error_code&) noexcept;

    void advance() noexcept;
    void advance(std::size_t n) noexcept;

    [[nodiscard]] bool discard(std::error_code&);

    [[nodiscard]] bool flush(std::error_code&) noexcept;

    [[nodiscard]] bool tell_read(pos_type&, std::error_code&) noexcept;
    [[nodiscard]] bool tell_write(pos_type&, std::error_code&) noexcept;

    [[nodiscard]] bool seek(pos_type, std::error_code&) noexcept;

private:
    core::File& m_file;
    std::size_t m_retain_size; // Zero in neutral and writing modes

#if ARCHON_DEBUG
    bool m_reading;
    bool m_writing;
#endif
};



class PrimWindowsTextFileImpl {
public:
    using Config   = core::PrimTextFileImplConfig;
    using pos_type = core::File::offset_type;

    static constexpr bool has_windows_newline_codec = true;

    explicit PrimWindowsTextFileImpl(core::File&, Config);

    void reset() noexcept;

    [[nodiscard]] bool read_ahead(core::Span<char> buffer, bool dynamic_eof, std::size_t& n, std::error_code&);

    [[nodiscard]] bool write(core::Span<const char> data, std::size_t& n, std::error_code&);

    void advance() noexcept;
    void advance(std::size_t n) noexcept;

    [[nodiscard]] bool discard(std::error_code&);

    [[nodiscard]] bool flush(std::error_code&) noexcept;

    [[nodiscard]] bool tell_read(pos_type&, std::error_code&) noexcept;
    [[nodiscard]] bool tell_write(pos_type&, std::error_code&) noexcept;

    [[nodiscard]] bool seek(pos_type, std::error_code&) noexcept;

private:
    core::File& m_file;
    core::Buffer<char> m_buffer;

    // Beginning and end of the current contents of the buffer. In neutral mode, both are
    // zero. In reading mode, `m_begin` corresponds to the position of the logical file
    // position, and `m_end` corresponds to the position of the actual file position. In
    // writing mode, it is the other way around.
    std::size_t m_begin;
    std::size_t m_end;

    // In neutral mode, and in reading mode, this is the position in the buffer that
    // corresponds to the read-ahead pointer. In writing mode, it has no meaning. It is
    // always zero in neutral mode, and in writing mode.
    std::size_t m_curr;

    // In reading mode, `m_retain_size` is the number of decoded characters between the
    // logical file pointer and the read-ahead pointer, and `m_retain_clear` is the number
    // of decoded characters that needs to be advanced by in order to clear all newline
    // conversions in the retained part. Both are zero in neutral mode, and in writing mode.
    std::size_t m_retain_size;
    std::size_t m_retain_clear;

#if ARCHON_DEBUG
    bool m_reading;
    bool m_writing;
#endif

    bool do_flush(std::error_code&) noexcept;
    void expand_buffer();
};








// Implementation


// ============================ PrimPosixTextFileImpl ============================


inline PrimPosixTextFileImpl::PrimPosixTextFileImpl(core::File& file, Config) noexcept
    : m_file(file)
{
}


inline void PrimPosixTextFileImpl::reset() noexcept
{
    m_retain_size = 0;

#if ARCHON_DEBUG
    m_reading = false;
    m_writing = false;
#endif
}


inline bool PrimPosixTextFileImpl::write(core::Span<const char> data, std::size_t& n, std::error_code& ec) noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_reading);
    m_writing = true;
#endif

    return m_file.try_write(data, n, ec);
}


inline void PrimPosixTextFileImpl::advance() noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    m_retain_size = 0;
}


inline void PrimPosixTextFileImpl::advance(std::size_t n) noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    ARCHON_ASSERT(n <= m_retain_size);
    m_retain_size -= n;
}


inline bool PrimPosixTextFileImpl::flush(std::error_code&) noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_reading);
    m_writing = false;
#endif
    return true;
}


inline bool PrimPosixTextFileImpl::tell_read(pos_type& pos, std::error_code& ec) noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    pos_type result;
    if (ARCHON_LIKELY(m_file.try_seek(0, core::File::Whence::cur, result, ec))) {
        if (ARCHON_LIKELY(core::try_int_sub(result, m_retain_size))) {
            pos = result;
            return true;
        }
        // We only get here if the position of the actual file pointer was manipulated
        // outside the control of this text file object.
        pos = 0;
        return true;
    }
    return false;
}


inline bool PrimPosixTextFileImpl::tell_write(pos_type& pos, std::error_code& ec) noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_reading);
#endif

    return m_file.try_seek(0, core::File::Whence::cur, pos, ec);
}


inline bool PrimPosixTextFileImpl::seek(pos_type pos, std::error_code& ec) noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    pos_type result; // Dummy
    if (ARCHON_LIKELY(m_file.try_seek(pos, core::File::Whence::set, result, ec))) {
        m_retain_size = 0;
#if ARCHON_DEBUG
        m_reading = false;
#endif
        return true;
    }
    return false;
}



// ============================ PrimWindowsTextFileImpl ============================


inline PrimWindowsTextFileImpl::PrimWindowsTextFileImpl(core::File& file, Config config)
    : m_file(file)
    , m_buffer(config.newline_codec_buffer_memory, config.newline_codec_buffer_size) // Throws
{
}


inline void PrimWindowsTextFileImpl::reset() noexcept
{
    m_begin = 0;
    m_end = 0;
    m_curr = 0;
    m_retain_size = 0;
    m_retain_clear = 0;

#if ARCHON_DEBUG
    m_reading = false;
    m_writing = false;
#endif
}


inline bool PrimWindowsTextFileImpl::flush(std::error_code& ec) noexcept
{
    ARCHON_ASSERT(m_curr == 0);
    ARCHON_ASSERT(m_retain_size  == 0);
    ARCHON_ASSERT(m_retain_clear == 0);
    if (ARCHON_LIKELY(do_flush(ec))) {
#if ARCHON_DEBUG
        m_writing = false;
#endif
        return true;
    }
    return false;
}


inline bool PrimWindowsTextFileImpl::seek(pos_type pos, std::error_code& ec) noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    pos_type result; // Dummy
    if (ARCHON_LIKELY(m_file.try_seek(pos, core::File::Whence::set, result, ec))) {
        m_begin = 0;
        m_end   = 0;
        m_curr  = 0;
        m_retain_size  = 0;
        m_retain_clear = 0;
#if ARCHON_DEBUG
        m_reading = false;
#endif
        return true;
    }
    return false;
}


inline void PrimWindowsTextFileImpl::expand_buffer()
{
    m_buffer.expand(1, m_end); // Throws
}


} // namespace archon::core::impl

#endif // ARCHON_X_CORE_X_IMPL_X_PRIM_TEXT_FILE_IMPL_HPP
