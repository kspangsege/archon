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

#ifndef ARCHON_X_CORE_X_IMPL_X_TEXT_FILE_IMPL_HPP
#define ARCHON_X_CORE_X_IMPL_X_TEXT_FILE_IMPL_HPP

/// \file


#include <cstddef>
#include <type_traits>
#include <utility>
#include <algorithm>
#include <locale>
#include <system_error>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/file.hpp>
#include <archon/core/text_file_config.hpp>
#include <archon/core/text_file_error.hpp>


namespace archon::core::impl {


// Variant: Degenerate character codec
template<class P, class D> class TextFileImpl1 {
public:
    using prim_type  = P;
    using codec_type = D;

    using char_type   = typename codec_type::char_type;
    using traits_type = typename codec_type::traits_type;
    using pos_type    = typename traits_type::pos_type;
    using state_type  = typename traits_type::state_type;

    using Config = core::TextFileImplConfig<codec_type>;

    static_assert(std::is_same_v<char_type, char>);

    static constexpr bool has_degen_unshift = true;

    static constexpr bool is_buffered = false;
    static constexpr bool has_windows_newline_codec = prim_type::has_windows_newline_codec;

    explicit TextFileImpl1(core::File&, const std::locale*, Config);

    void reset(state_type) noexcept;

    [[nodiscard]] bool read_ahead(core::Span<char> buffer, bool dynamic_eof, std::size_t& n, std::error_code&);

    [[nodiscard]] bool write(core::Span<const char> data, std::size_t& n, std::error_code&);

    [[nodiscard]] bool unshift(std::error_code&);

    void advance();
    void advance(std::size_t n);

    [[nodiscard]] bool discard(std::error_code&);

    [[nodiscard]] bool flush(std::error_code&);

    [[nodiscard]] bool tell_read(pos_type&, std::error_code&);
    [[nodiscard]] bool tell_write(pos_type&, std::error_code&);

    [[nodiscard]] bool seek(pos_type, std::error_code&);

    void imbue(const std::locale&, state_type);

private:
    prim_type m_prim_impl;
};



// Variant: Non-degenerate character codec
template<class P, class D> class TextFileImpl2 {
public:
    using prim_type  = P;
    using codec_type = D;

    using char_type   = typename codec_type::char_type;
    using traits_type = typename codec_type::traits_type;
    using pos_type    = typename traits_type::pos_type;
    using state_type  = typename traits_type::state_type;

    using Config = core::TextFileImplConfig<codec_type>;

    static constexpr bool has_degen_unshift = false;

    static constexpr bool is_buffered = false;
    static constexpr bool has_windows_newline_codec = prim_type::has_windows_newline_codec;

    explicit TextFileImpl2(core::File&, const std::locale*, Config);

    void reset(state_type) noexcept;

    [[nodiscard]] bool read_ahead(core::Span<char_type> buffer, bool dynamic_eof, std::size_t& n, std::error_code&);

    [[nodiscard]] bool write(core::Span<const char_type> data, std::size_t& n, std::error_code&);

    [[nodiscard]] bool unshift(std::error_code&);

    void advance() noexcept;
    void advance(std::size_t n);

    [[nodiscard]] bool discard(std::error_code&);

    [[nodiscard]] bool flush(std::error_code&);

    [[nodiscard]] bool tell_read(pos_type&, std::error_code&);
    [[nodiscard]] bool tell_write(pos_type&, std::error_code&);

    [[nodiscard]] bool seek(pos_type, std::error_code&);

    void imbue(const std::locale&, state_type);

private:
    prim_type m_prim_impl;
    codec_type m_codec;

    // In reading mode, this buffer contains data that has been read from the lower layer
    // (m_prim_impl), and has not yet been consumed by the application. In writing mode, it
    // contains data that has been written by the application, and has not yet been flushed
    // to the lower layer.
    //
    // In reading mode, `m_begin` is the offset within this buffer of the first byte of the
    // first unconsumed character, and `m_state` is the shift state associated with
    // `m_begin`.
    core::Buffer<char> m_buffer;

    // `m_state` is always the shift state at the position of the logical file pointer. In
    // neutral mode, `m_state_2` is equal to `m_state`. In reading mode, `m_state_2` is the
    // shift state at the position of the read-ahead pointer. In writing mode, the value of
    // `m_state_2` in undefined.
    state_type m_state = {};
    state_type m_state_2 = {};

    // Beginning and end of the current contents of the buffer. In neutral mode, both are
    // zero. In reading mode, `m_begin` corresponds to the position of the logical file
    // pointer, and `m_end` corresponds to the position of the primitive implementation's
    // read-ahead pointer (`m_prim_impl`). In writing mode, `m_end` corresponds to the
    // position of the logical file pointer, and `m_begin` corresponds to the position of
    // the primitive implementation's logical file pointer.
    std::size_t m_begin = 0;
    std::size_t m_end   = 0;

    // In neutral mode, and in reading mode, this is the position in the buffer that
    // corresponds to the primitive implementation's logical file pointer
    // (`m_prim_impl`). In writing mode, it has no meaning. It is always zero in neutral
    // mode, and in writing mode.
    std::size_t m_offset = 0;

    // In neutral mode, and in reading mode, this is the position in the buffer that
    // corresponds to the read-ahead pointer (as seen by applications of this
    // implementation). In writing mode, it has no meaning. It is always zero in neutral
    // mode, and in writing mode.
    std::size_t m_curr = 0;

    // In reading mode, this is the number of decoded characters between the logical file
    // pointer (m_begin) and the read-ahead pointer (m_curr). It is always zero in neutral
    // mode, and in writing mode.
    std::size_t m_retain_size  = 0;

#if ARCHON_DEBUG
    // Mode      `m_reading`    `m_writing`
    // --------------------------------------
    // neutral   false          false
    // reading   true           false
    // writing   false          true
    //
    // INVARIANT: If the underlying layer (`m_prim_impl`) is in reading mode, this layer is
    // in reading mode.
    //
    // INVARIANT: If the underlying layer (`m_prim_impl`) is in writing mode, this layer is
    // in writing mode.
    //
    // INVARIANT: If this layer is in neutral mode, the underlying layer (`m_prim_impl`) is
    // in neutral mode.
    //
    bool m_reading = false;
    bool m_writing = false;
#endif

    static auto make_buffer(Config&) -> core::Buffer<char>;
    static constexpr auto max_buffer_size() noexcept -> std::size_t;

    bool shallow_flush(std::error_code&);
    void expand_buffer();
    void do_advance() noexcept;
};



template<class P, class D, bool> class TextFileImpl3
    : public TextFileImpl1<P, D> {
public:
    using TextFileImpl1<P, D>::TextFileImpl1;
};

template<class P, class D> class TextFileImpl3<P, D, false>
    : public TextFileImpl2<P, D> {
public:
    using TextFileImpl2<P, D>::TextFileImpl2;
};



template<class P, class D> class TextFileImpl
    : public TextFileImpl3<P, D, D::is_degen> {
public:
    using TextFileImpl3<P, D, D::is_degen>::TextFileImpl3;
};








// Implementation


// ============================ TextFileImpl1<P, D> ============================


template<class P, class D>
inline TextFileImpl1<P, D>::TextFileImpl1(core::File& file, const std::locale*, Config config)
    : m_prim_impl(file, std::move(config)) // Throws
{
}


template<class P, class D>
inline void TextFileImpl1<P, D>::reset(state_type) noexcept
{
    m_prim_impl.reset();
}


template<class P, class D>
inline bool TextFileImpl1<P, D>::read_ahead(core::Span<char> buffer, bool dynamic_eof,
                                            std::size_t& n, std::error_code& ec)
{
    return m_prim_impl.read_ahead(buffer, dynamic_eof, n, ec); // Throws
}


template<class P, class D>
inline bool TextFileImpl1<P, D>::write(core::Span<const char> data, std::size_t& n, std::error_code& ec)
{
    return m_prim_impl.write(data, n, ec); // Throws
}


template<class P, class D>
inline bool TextFileImpl1<P, D>::unshift(std::error_code&)
{
    return true;
}


template<class P, class D>
inline void TextFileImpl1<P, D>::advance()
{
    m_prim_impl.advance(); // Throws
}


template<class P, class D>
inline void TextFileImpl1<P, D>::advance(std::size_t n)
{
    m_prim_impl.advance(n); // Throws
}


template<class P, class D>
inline bool TextFileImpl1<P, D>::discard(std::error_code& ec)
{
    return m_prim_impl.discard(ec); // Throws
}


template<class P, class D>
inline bool TextFileImpl1<P, D>::flush(std::error_code& ec)
{
    return m_prim_impl.flush(ec); // Throws
}


template<class P, class D>
inline bool TextFileImpl1<P, D>::tell_read(pos_type& pos, std::error_code& ec)
{
    typename prim_type::pos_type pos_2 = {};
    if (ARCHON_LIKELY(m_prim_impl.tell_read(pos_2, ec))) { // Throws
        pos = pos_type(pos_2);
        return true;
    }
    return false;
}


template<class P, class D>
inline bool TextFileImpl1<P, D>::tell_write(pos_type& pos, std::error_code& ec)
{
    typename prim_type::pos_type pos_2 = {};
    if (ARCHON_LIKELY(m_prim_impl.tell_write(pos_2, ec))) { // Throws
        pos = pos_type(pos_2);
        return true;
    }
    return false;
}


template<class P, class D>
inline bool TextFileImpl1<P, D>::seek(pos_type pos, std::error_code& ec)
{
    auto pos_2 = typename prim_type::pos_type(pos);
    return m_prim_impl.seek(pos_2, ec); // Throws
}


template<class P, class D>
inline void TextFileImpl1<P, D>::imbue(const std::locale&, state_type)
{
}



// ============================ TextFileImpl2<P, D> ============================


template<class P, class D>
inline TextFileImpl2<P, D>::TextFileImpl2(core::File& file, const std::locale* locale,
                                          Config config)
    : m_prim_impl(file, std::move(config)) // Throws
    , m_codec(locale, std::move(config.char_codec)) // Throws
    , m_buffer(make_buffer(config)) // Throws
{
}


template<class P, class D>
inline void TextFileImpl2<P, D>::reset(state_type state) noexcept
{
    m_prim_impl.reset();

    m_state = state;
    m_state_2 = state;
    m_begin = 0;
    m_end = 0;
    m_offset = 0;
    m_curr = 0;
    m_retain_size = 0;

#if ARCHON_DEBUG
    m_reading = false;
    m_writing = false;
#endif
}


template<class P, class D>
bool TextFileImpl2<P, D>::read_ahead(core::Span<char_type> buffer, bool dynamic_eof, std::size_t& n,
                                     std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
    m_reading = true;
#endif

    bool end_of_file = false;
    for (;;) {
        core::Span data = core::Span<char>(m_buffer).first(m_end);
        std::size_t buffer_offset = 0;
        bool error = false;
        bool complete = m_codec.decode(m_state_2, data, m_curr, end_of_file, buffer, buffer_offset, error); // Throws
        m_retain_size += buffer_offset;
        if (ARCHON_LIKELY(buffer_offset > 0 || buffer.size() == 0)) {
            n = buffer_offset;
            return true;
        }
        if (ARCHON_LIKELY(!error)) {
            if (ARCHON_LIKELY(!end_of_file)) {
                // Move any retained data to start of buffer
                ARCHON_ASSERT(m_offset <= m_begin);
                m_prim_impl.advance(std::size_t(m_begin - m_offset)); // Throws
                char* base = m_buffer.data();
                std::copy(base + m_begin, base + m_end, base);
                m_end  -= m_begin;
                m_curr -= m_begin;
                m_begin  = 0;
                m_offset = 0;
                if (ARCHON_UNLIKELY(m_end == m_buffer.size()))
                    expand_buffer(); // Throws
                std::size_t n_2 = 0;
                core::Span buffer_2 = core::Span<char>(m_buffer).subspan(m_end);
                if (ARCHON_LIKELY(m_prim_impl.read_ahead(buffer_2, dynamic_eof, n_2, ec))) { // Throws
                    if (ARCHON_LIKELY(n_2 > 0)) {
                        m_end += n_2;
                        continue;
                    }
                    if (ARCHON_LIKELY((end_of_file && complete) || dynamic_eof))
                        goto end_of_file;
                    end_of_file = true;
                    continue;
                }
                return false;
            }
          end_of_file:
            n = 0;
            return true;
        }
        ec = core::TextFileError::invalid_byte_seq;
        return false;
    }
}


template<class P, class D>
bool TextFileImpl2<P, D>::write(core::Span<const char_type> data, std::size_t& n, std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_reading);
    m_writing = true;
#endif

    std::size_t data_offset = 0;
    for (;;) {
        bool error = false;
        bool complete = m_codec.encode(m_state, data, data_offset, m_buffer, m_end, error); // Throws
        if (ARCHON_LIKELY(complete)) {
            ARCHON_ASSERT(data_offset == data.size());
            n = data.size();
            return true;
        }
        if (ARCHON_LIKELY(!error)) {
            if (ARCHON_LIKELY(m_end > 0)) {
                if (ARCHON_LIKELY(shallow_flush(ec))) // Throws
                    continue;
                return false;
            }
            expand_buffer(); // Throws
            continue;
        }
        n = data_offset;
        ec = core::TextFileError::invalid_char;
        return false;
    }
}


template<class P, class D>
bool TextFileImpl2<P, D>::unshift(std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_reading);
    m_writing = true;
#endif

    for (;;) {
        bool complete = m_codec.unshift(m_state, m_buffer, m_end); // Throws
        if (ARCHON_LIKELY(complete))
            return true;
        if (ARCHON_LIKELY(m_end > 0)) {
            if (ARCHON_LIKELY(shallow_flush(ec))) // Throws
                continue;
            return false;
        }
        expand_buffer(); // Throws
    }
}


template<class P, class D>
inline void TextFileImpl2<P, D>::advance() noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    ARCHON_ASSERT(m_begin <= m_curr);
    do_advance();
}


template<class P, class D>
inline void TextFileImpl2<P, D>::advance(std::size_t n)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    ARCHON_ASSERT(n <= m_retain_size);
    ARCHON_ASSERT(m_begin <= m_curr);
    if (ARCHON_LIKELY(n == m_retain_size)) {
        do_advance();
        return;
    }
    core::Span data = core::Span<char>(m_buffer).first(m_curr);
    // The difference between `m_begin` and `data.size()` cannot be greater than the size of
    // `m_buffer`, and the buffer is not allowed to grow larger than
    // `m_codec.max_simul_decode_size()`
    m_codec.simul_decode(m_state, data, m_begin, n); // Throws
    ARCHON_ASSERT(m_begin <= m_curr);
    m_retain_size -= n;
}


template<class P, class D>
bool TextFileImpl2<P, D>::discard(std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    ARCHON_ASSERT(m_offset <= m_begin);
    std::size_t n = std::size_t(m_begin - m_offset);
    m_prim_impl.advance(n); // Throws
    m_offset = m_begin;
    if (ARCHON_LIKELY(m_prim_impl.discard(ec))) { // Throws
        m_state_2     = m_state;
        m_begin       = 0;
        m_end         = 0;
        m_offset      = 0;
        m_curr        = 0;
        m_retain_size = 0;
#if ARCHON_DEBUG
        m_reading = false;
#endif
        return true;
    }
    return false;
}


template<class P, class D>
inline bool TextFileImpl2<P, D>::flush(std::error_code& ec)
{
    ARCHON_ASSERT(m_offset       == 0);
    ARCHON_ASSERT(m_curr         == 0);
    ARCHON_ASSERT(m_retain_size  == 0);
    if (ARCHON_LIKELY(shallow_flush(ec))) { // Throws
        if (ARCHON_LIKELY(m_prim_impl.flush(ec))) { // Throws
            m_state_2 = m_state; // Part of transitioning to neutral mode
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
    static_cast<void>(m_prim_impl.flush(ec_2)); // Throws
    return false;
}


template<class P, class D>
bool TextFileImpl2<P, D>::tell_read(pos_type& pos, std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    ARCHON_ASSERT(m_offset <= m_begin);
    m_prim_impl.advance(std::size_t(m_begin - m_offset)); // Throw
    m_offset = m_begin;
    typename prim_type::pos_type pos_2 = {};
    if (ARCHON_LIKELY(m_prim_impl.tell_read(pos_2, ec))) { // Throws
        pos = pos_type(pos_2);
        pos.state(m_state);
        return true;
    }
    return false;
}


template<class P, class D>
bool TextFileImpl2<P, D>::tell_write(pos_type& pos, std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_reading);
#endif

    typename prim_type::pos_type pos_2 = {};
    // Take care to not invoke write() on `m_prim_impl` unless there is actually somethig to
    // write. This is necesary to avoid ending up in a situation where `m_prim_impl` is in
    // writing mode, but this file implementation object is in neutral mode.
    bool success = ((m_begin == m_end || shallow_flush(ec)) && m_prim_impl.tell_write(pos_2, ec)); // Throws
    if (ARCHON_LIKELY(success)) {
        pos = pos_type(pos_2);
        pos.state(m_state);
        return true;
    }
    return false;
}


template<class P, class D>
inline bool TextFileImpl2<P, D>::seek(pos_type pos, std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    auto pos_2 = typename prim_type::pos_type(pos);
    if (ARCHON_LIKELY(m_prim_impl.seek(pos_2, ec))) { // Throws
        m_state       = pos.state();
        m_state_2     = m_state;
        m_begin       = 0;
        m_end         = 0;
        m_offset      = 0;
        m_curr        = 0;
        m_retain_size = 0;
#if ARCHON_DEBUG
        m_reading = false;
#endif
        return true;
    }
    return false;
}


template<class P, class D>
inline void TextFileImpl2<P, D>::imbue(const std::locale& locale, state_type state)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_reading);
    ARCHON_ASSERT(!m_writing);
#endif

    m_codec.imbue(locale); // Throws
    m_state   = state;
    m_state_2 = state;
}


template<class P, class D>
auto TextFileImpl2<P, D>::make_buffer(Config& config) -> core::Buffer<char>
{
    core::Span<char> seed_memory = config.char_codec_buffer_memory;
    if (ARCHON_UNLIKELY(seed_memory.size() > max_buffer_size()))
        seed_memory = seed_memory.subspan(0, max_buffer_size());
    std::size_t size = config.char_codec_buffer_size;
    if (ARCHON_UNLIKELY(size > max_buffer_size()))
        size = max_buffer_size();
    return core::Buffer<char>(seed_memory, size); // Throws
}


template<class P, class D>
constexpr auto TextFileImpl2<P, D>::max_buffer_size() noexcept -> std::size_t
{
    return decltype(m_codec)::max_simul_decode_size();
}


template<class P, class D>
bool TextFileImpl2<P, D>::shallow_flush(std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_reading);
#endif

    ARCHON_ASSERT(m_begin <= m_end);
    core::Span data = core::Span<char>(m_buffer).first(m_end).subspan(m_begin);
    std::size_t n = 0;
    if (ARCHON_LIKELY(m_prim_impl.write(data, n, ec))) { // Throws
        m_begin = 0;
        m_end   = 0;
        return true;
    }
    m_begin += n;
    return false;
}


template<class P, class D>
inline void TextFileImpl2<P, D>::expand_buffer()
{
    m_buffer.expand(1, m_end, max_buffer_size()); // Throws
}


template<class P, class D>
inline void TextFileImpl2<P, D>::do_advance() noexcept
{
    m_state = m_state_2;
    m_begin = m_curr;
    m_retain_size = 0;
}


} // namespace archon::core::impl

#endif // ARCHON_X_CORE_X_IMPL_X_TEXT_FILE_IMPL_HPP
