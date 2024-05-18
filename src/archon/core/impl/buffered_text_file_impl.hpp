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

#ifndef ARCHON_X_CORE_X_IMPL_X_BUFFERED_TEXT_FILE_IMPL_HPP
#define ARCHON_X_CORE_X_IMPL_X_BUFFERED_TEXT_FILE_IMPL_HPP


#include <cstddef>
#include <utility>
#include <algorithm>
#include <system_error>
#include <locale>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/file.hpp>
#include <archon/core/text_file_config.hpp>


namespace archon::core::impl {


template<class S> class BufferedTextFileImpl {
public:
    using subimpl_type = S;

    using char_type   = typename subimpl_type::char_type;
    using traits_type = typename subimpl_type::traits_type;
    using codec_type  = typename subimpl_type::codec_type;
    using pos_type    = typename traits_type::pos_type;
    using state_type  = typename traits_type::state_type;

    using Config = core::BufferedTextFileImplConfig<subimpl_type>;

    static constexpr bool has_degen_unshift = S::has_degen_unshift;

    static constexpr bool is_buffered = true;
    static constexpr bool has_windows_newline_codec = subimpl_type::has_windows_newline_codec;

    explicit BufferedTextFileImpl(core::File&, const std::locale*, Config);

    void reset(state_type) noexcept;

    [[nodiscard]] bool read_ahead(core::Span<char_type> buffer, bool dynamic_eof, std::size_t& n, std::error_code&);

    [[nodiscard]] bool read_until(char_type delim, core::Buffer<char_type>&, bool dynamic_eof, std::size_t& offset,
                                  bool& found_delim, std::error_code&);

    [[nodiscard]] bool write(core::Span<const char_type> data, std::size_t& n, std::error_code&);

    [[nodiscard]] bool unshift(std::error_code&);

    void advance() noexcept;
    void advance(std::size_t n) noexcept;

    [[nodiscard]] bool discard(std::error_code&);

    [[nodiscard]] bool flush(std::error_code&);

    [[nodiscard]] bool tell_read(pos_type&, std::error_code&);
    [[nodiscard]] bool tell_write(pos_type&, std::error_code&);

    [[nodiscard]] bool seek(pos_type, std::error_code&);

    void imbue(const std::locale&, state_type);

private:
    subimpl_type m_subimpl;
    core::Buffer<char_type> m_buffer;

    // Beginning and end of the current contents of the buffer. In neutral mode, both are
    // zero. In reading mode, `m_begin` corresponds to the position of the logical file
    // pointer (as seen by applications of this implementation), and `m_end` corresponds to
    // the position of the subimplementation's read-ahead pointer (`m_subimpl`). In writing
    // mode, `m_end` corresponds to the position of the logical file pointer (as seen by
    // applications of this implementation), and `m_begin` corresponds to the position of
    // the subimplementation's logical file pointer.
    std::size_t m_begin;
    std::size_t m_end;

    // In neutral mode, and in reading mode, this is the position in the buffer that
    // corresponds to the subimplementation's logical file pointer (`m_subimpl`). In writing
    // mode, it has no meaning. It is always zero in neutral mode, and in writing mode.
    std::size_t m_offset;

    // In neutral mode, and in reading mode, this is the position in the buffer that
    // corresponds to the read-ahead pointer (as seen by applications of this
    // implementation). In writing mode, it has no meaning. It is always zero in neutral
    // mode, and in writing mode.
    std::size_t m_curr;

#if ARCHON_DEBUG
    // Mode      `m_reading`    `m_writing`
    // --------------------------------------
    // neutral   false          false
    // reading   true           false
    // writing   false          true
    //
    // INVARIANT: If the underlying layer (`m_subimpl`) is in reading mode, this layer is in
    // reading mode.
    //
    // INVARIANT: If the underlying layer (`m_subimpl`) is in writing mode, this layer is in
    // writing mode.
    //
    // INVARIANT: If this layer is in neutral mode, the underlying layer (`m_subimpl`) is in
    // neutral mode.
    //
    bool m_reading;
    bool m_writing;
#endif

    bool shallow_flush(std::error_code&);
    void expand_buffer();
};








// Implementation


template<class S>
inline BufferedTextFileImpl<S>::BufferedTextFileImpl(core::File& file, const std::locale* locale, Config config) :
    m_subimpl(file, locale, std::move(config.subimpl)), // Throws
    m_buffer(config.buffer_memory, config.buffer_size) // Throws
{
    // Buffer must not be empty
    m_buffer.reserve(1); // Throws
}


template<class S>
void BufferedTextFileImpl<S>::reset(state_type state) noexcept
{
    m_subimpl.reset(state);

    m_begin  = 0;
    m_end    = 0;
    m_offset = 0;
    m_curr   = 0;

#if ARCHON_DEBUG
    m_reading = false;
    m_writing = false;
#endif
}


template<class S>
bool BufferedTextFileImpl<S>::read_ahead(core::Span<char_type> buffer, bool dynamic_eof, std::size_t& n,
                                         std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
    m_reading = true;
#endif

    if (ARCHON_LIKELY(buffer.size() > 0)) {
        if (ARCHON_LIKELY(m_curr < m_end)) {
          copy:
            std::size_t n_2 = std::min(buffer.size(), std::size_t(m_end - m_curr));
            std::copy_n(m_buffer.data() + m_curr, n_2, buffer.data());
            m_curr += n_2;
            n = n_2;
            ARCHON_ASSERT(n > 0);
            return true;
        }
        {
            // Move any retained data to start of buffer
            ARCHON_ASSERT(m_offset <= m_begin);
            m_subimpl.advance(std::size_t(m_begin - m_offset)); // Throws
            char_type* base = m_buffer.data();
            std::copy(base + m_begin, base + m_end, base);
            m_end  -= m_begin;
            m_curr -= m_begin;
            m_begin  = 0;
            m_offset = 0;
            if (ARCHON_UNLIKELY(m_end == m_buffer.size()))
                expand_buffer(); // Throws
            std::size_t n_2 = 0;
            core::Span buffer_2 = core::Span<char_type>(m_buffer).subspan(m_end);
            if (ARCHON_LIKELY(m_subimpl.read_ahead(buffer_2, dynamic_eof, n_2, ec))) { // Throws
                if (ARCHON_LIKELY(n_2 > 0)) {
                    m_end += n_2;
                    goto copy;
                }
                // Signal end of file
            }
            else {
                return false;
            }
        }
    }
    n = 0;
    return true;
}


template<class S>
bool BufferedTextFileImpl<S>::read_until(char_type delim, core::Buffer<char_type>& buffer, bool dynamic_eof,
                                         std::size_t& offset, bool& found_delim, std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
    m_reading = true;
#endif

    ARCHON_ASSERT(m_begin <= m_curr);
    m_begin = m_curr;

  again:
    const char_type* base = m_buffer.data();
    const char_type* begin = base + m_curr;
    const char_type* end = base + m_end;
    const char_type* i = std::find(begin, end, delim);
    if (ARCHON_LIKELY(i != end)) {
        std::size_t n = std::size_t((i + 1) - begin);
        buffer.append({ begin, n }, offset); // Throws
        m_curr += n;
        found_delim = true;
        return true; // Success
    }
    std::size_t n = std::size_t(end - begin);
    buffer.append({ begin, n }, offset); // Throws
    m_curr += n;
    m_subimpl.advance(); // Throws
    m_begin  = 0;
    m_end    = 0;
    m_offset = 0;
    m_curr   = 0;
    std::size_t n_2 = 0;
    if (ARCHON_LIKELY(m_subimpl.read_ahead(m_buffer, dynamic_eof, n_2, ec))) { // Throws
        if (ARCHON_LIKELY(n_2 > 0)) {
            m_end = n_2;
            goto again;
        }
        found_delim = false; // Signal end of file
        return true; // Success
    }
    return false; // Failure
}


template<class S>
bool BufferedTextFileImpl<S>::write(core::Span<const char_type> data, std::size_t& n, std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_reading);
    m_writing = true;
#endif

    ARCHON_ASSERT(m_buffer.size() > 0);
    core::Span data_2 = data;
    for (;;) {
        std::size_t capacity = std::size_t(m_buffer.size() - m_end);
        std::size_t n_2 = std::min(data_2.size(), capacity);
        std::copy_n(data_2.data(), n_2, m_buffer.data() + m_end);
        m_end += n_2;
        if (ARCHON_LIKELY(data_2.size() <= capacity)) {
            n = data.size();
            return true;
        }
        data_2 = data_2.subspan(n_2);
        if (ARCHON_LIKELY(shallow_flush(ec))) { // Throws
            ARCHON_ASSERT(m_end == 0);
            continue;
        }
        n = std::size_t(data.size() - data_2.size());
        return false;
    }
}


template<class S>
bool BufferedTextFileImpl<S>::unshift(std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_reading);
    m_writing = true;
#endif

    if constexpr (subimpl_type::has_degen_unshift)
        return true;
    if (ARCHON_LIKELY(shallow_flush(ec))) // Throws
        return m_subimpl.unshift(ec); // Throws
    return false;
}


template<class S>
inline void BufferedTextFileImpl<S>::advance() noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    ARCHON_ASSERT(m_begin <= m_curr);
    m_begin = m_curr;
}


template<class S>
inline void BufferedTextFileImpl<S>::advance(std::size_t n) noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    ARCHON_ASSERT(m_begin <= m_curr);
    ARCHON_ASSERT(n <= std::size_t(m_curr - m_begin));
    m_begin += n;
}


template<class S>
bool BufferedTextFileImpl<S>::discard(std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    ARCHON_ASSERT(m_offset <= m_begin);
    std::size_t n = std::size_t(m_begin - m_offset);
    m_subimpl.advance(n); // Throws
    m_offset = m_begin;
    if (ARCHON_LIKELY(m_subimpl.discard(ec))) { // Throws
        m_begin   = 0;
        m_end     = 0;
        m_offset  = 0;
        m_curr    = 0;
#if ARCHON_DEBUG
        m_reading = false;
#endif
        return true;
    }
    return false;
}


template<class S>
inline bool BufferedTextFileImpl<S>::flush(std::error_code& ec)
{
    ARCHON_ASSERT(m_offset == 0);
    ARCHON_ASSERT(m_curr   == 0);

    if (ARCHON_LIKELY(shallow_flush(ec))) { // Throws
        if (ARCHON_LIKELY(m_subimpl.flush(ec))) { // Throws
#if ARCHON_DEBUG
            m_writing = false;
#endif
            return true;
        }
        return false;
    }

    // Even when everything in the local buffer could not be written, an attempt to
    // recursively flush the part, that could be writen, must still be made.
    std::error_code ec_2; // Dummy
    static_cast<void>(m_subimpl.flush(ec_2)); // Throws
    return false;
}


template<class S>
bool BufferedTextFileImpl<S>::tell_read(pos_type& pos, std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    ARCHON_ASSERT(m_offset <= m_begin);
    m_subimpl.advance(std::size_t(m_begin - m_offset)); // Throw
    m_offset = m_begin;
    return m_subimpl.tell_read(pos, ec); // Throws
}


template<class S>
bool BufferedTextFileImpl<S>::tell_write(pos_type& pos, std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_reading);
#endif

    // Take care to not invoke write() on `m_subimpl` unless there is actually somethig to
    // write. This is necesary to avoid ending up in a situation where `m_subimpl` is in
    // writing mode, but this file implementation object is in neutral mode.
    return ((m_begin == m_end || shallow_flush(ec)) && m_subimpl.tell_write(pos, ec)); // Throws
}


template<class S>
inline bool BufferedTextFileImpl<S>::seek(pos_type pos, std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    if (ARCHON_LIKELY(m_subimpl.seek(pos, ec))) { // Throws
        m_begin   = 0;
        m_end     = 0;
        m_offset  = 0;
        m_curr    = 0;
#if ARCHON_DEBUG
        m_reading = false;
#endif
        return true;
    }
    return false;
}


template<class S>
inline void BufferedTextFileImpl<S>::imbue(const std::locale& locale, state_type state)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_reading);
    ARCHON_ASSERT(!m_writing);
#endif

    m_subimpl.imbue(locale, state); // Throws
}


template<class S>
bool BufferedTextFileImpl<S>::shallow_flush(std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_reading);
#endif

    ARCHON_ASSERT(m_begin <= m_end);
    core::Span data = core::Span<char_type>(m_buffer).first(m_end).subspan(m_begin);
    std::size_t n = 0;
    if (ARCHON_LIKELY(m_subimpl.write(data, n, ec))) { // Throws
        m_begin = 0;
        m_end   = 0;
        return true;
    }
    m_begin += n;
    return false;
}


template<class S>
inline void BufferedTextFileImpl<S>::expand_buffer()
{
    m_buffer.expand(m_end); // Throws
}


} // namespace archon::core::impl

#endif // ARCHON_X_CORE_X_IMPL_X_BUFFERED_TEXT_FILE_IMPL_HPP
