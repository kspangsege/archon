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

#ifndef ARCHON_X_CORE_X_TEXT_FILE_STREAM_HPP
#define ARCHON_X_CORE_X_TEXT_FILE_STREAM_HPP

/// \file


#include <cstddef>
#include <utility>
#include <string_view>
#include <locale>
#include <system_error>
#include <ios>
#include <streambuf>
#include <istream>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/char_codec.hpp>
#include <archon/core/filesystem.hpp>
#include <archon/core/file.hpp>
#include <archon/core/text_file_impl.hpp>


namespace archon::core {


template<class I> class GenericTextFileStreambuf;



/// \brief Text file stream.
///
/// This class is just a combination of a `std::iostream` and a \ref
/// GenericTextFileStreambuf.
///
/// \tparam I The text file implementation type to be used. This is simply passed on to \ref
/// GenericTextFileStreambuf.
///
template<class I> class GenericTextFileStream
    : public std::basic_iostream<typename I::char_type, typename I::traits_type> {
public:
    using impl_type = I;

    using codec_type = typename impl_type::codec_type;

    using Mode   = core::File::Mode;
    using Config = typename GenericTextFileStreambuf<I>::Config;

    explicit GenericTextFileStream(core::FilesystemPathRef, Mode = Mode::read, Config = {});
    explicit GenericTextFileStream(core::File, Config = {});
    explicit GenericTextFileStream(core::File*, Config = {});

    /// \brief Generate bytes to revert to initial shift state.
    ///
    /// If this file stream uses a stateful character codec, and if the shift state at the
    /// current position of the file pointer is not the initial shift state, this function
    /// produces a byte sequence that brings the shift state back to the initial shift
    /// state.
    ///
    /// By default, that is, when \ref Config::disable_autounshift is false, the unshifting
    /// operation is done automatically as part of the flushing operation (`flush()`).
    ///
    /// After an invocation of `unshift()`, it is necessary to follow upo with a flushing
    /// operation (`flush()`) to ensure that the produced bytes are written to the
    /// underlying medium.
    ///
    void unshift();

private:
    core::File m_file;
    GenericTextFileStreambuf<I> m_streambuf;
};



template<class C, class T = std::char_traits<C>, class D = core::BasicCharCodec<C, T>> using BasicTextFileStream =
    GenericTextFileStream<core::TextFileImpl<C, T, D>>;
template<class C, class T = std::char_traits<C>, class D = core::BasicCharCodec<C, T>> using BasicPosixTextFileStream =
    GenericTextFileStream<core::PosixTextFileImpl<C, T, D>>;
template<class C, class T = std::char_traits<C>, class D = core::BasicCharCodec<C, T>> using BasicWindowsTextFileStream =
    GenericTextFileStream<core::WindowsTextFileImpl<C, T, D>>;


using TextFileStream        = BasicTextFileStream<char>;
using PosixTextFileStream   = BasicPosixTextFileStream<char>;
using WindowsTextFileStream = BasicWindowsTextFileStream<char>;

using WideTextFileStream        = BasicTextFileStream<wchar_t>;
using WidePosixTextFileStream   = BasicPosixTextFileStream<wchar_t>;
using WideWindowsTextFileStream = BasicWindowsTextFileStream<wchar_t>;




/// \brief Stream buffer for text file streams.
///
/// This class is a stream buffer implementation based on the text file implementation
/// concept as defined by the Archon Core library (see \ref
/// Concept_Archon_Core_TextFileImpl). It is effectively an afternative to
/// `std::basic_filebuf` offering some extra features, and increased control over the
/// character encoding and decoding process.
///
/// \tparam I The text file implementation type to be used. This must be a type that
/// satisfies \ref Concept_Archon_Core_TextFileImpl. While it would be possible to use a
/// buffered implementation type here, it is a bad idea, as the text file stream class
/// already introduces then necessary buffering.
///
/// This implementation does not perform a flushing operation (`sync()`) as part of the
/// stream buffer destruction process (a design choice). For this reason, applications
/// should make an explicit attempt to flush the stream buffer before the stream buffer is
/// destroyed.
///
/// With this implementation, `seekoff()` fails unless the spcified offset is zero and the
/// specified direction is `std::ios_base::cur`. This means that relative seeking is
/// unsupported, and that `seekoff()` can only be used for the purpose of telling the
/// current position of the file pointer (file offset).
///
/// With this implementation, `setbuf()` has no effect. To use a custom buffer, specify it
/// through \ref Config::buffer_memory.
///
/// With this implementation, `showmanyc()` always returns 0, which means that `in_avail()`,
/// and, in turn, `std::basic_istream<C, T>::readsome()` will generally not work in a useful
/// way, and it is not clear that there is any way to remedy the situation. See \ref
/// GenericTextFile<I>::read_some() for a working alternative.
///
/// With this implementation, `pbackfail()` always fails (returns `T::eof()`), so
/// `sungetc()` and `sputbackc()` (`unget()` and `putback()` in `std::basic_istream`) can
/// only be relied on immediately after advancing the read position, such as through
/// `sbumpc()`.
///
/// This implementation assumes that `setg()`, `setp()`, `gbump()`, and `pbump()` in
/// `std::basic_streambuf` never throw. While this is not currently guaranteed by the
/// standard (C++17), it is deemed very likely to be the case in practice. This assumption
/// is necessary in order to uphold critical invariants.
///
template<class I> class GenericTextFileStreambuf
    : public std::basic_streambuf<typename I::char_type, typename I::traits_type> {
public:
    using impl_type = I;

    using char_type   = typename impl_type::char_type;
    using traits_type = typename impl_type::traits_type;
    using codec_type  = typename impl_type::codec_type;
    using int_type    = typename traits_type::int_type;
    using off_type    = typename traits_type::off_type;
    using pos_type    = typename traits_type::pos_type;

    struct Config;

    explicit GenericTextFileStreambuf(core::File&);
    explicit GenericTextFileStreambuf(core::File&, const std::locale&);
    explicit GenericTextFileStreambuf(core::File&, const std::locale&, Config);

    /// \brief Generate bytes to revert to initial shift state.
    ///
    /// Done automatically as part of `sync()` (called by `pubsync()`) unless \ref
    /// Config::disable_autounshift is set to `true`.
    ///
    /// Returns `true` on success and `false` on failure.
    ///
    /// See \ref GenericTextFileStream<I>::unshift() for further details.
    ///
    bool unshift();

protected:
    void imbue(const std::locale&) override;
    auto xsgetn(char_type*, std::streamsize) -> std::streamsize override;
    auto xsputn(const char_type*, std::streamsize) -> std::streamsize override;
    auto underflow() -> int_type override;
    auto uflow() -> int_type override;
    auto overflow(int_type) -> int_type override;
    int sync() override;
    auto seekoff(off_type, std::ios_base::seekdir, std::ios_base::openmode) -> pos_type override;
    auto seekpos(pos_type, std::ios_base::openmode) -> pos_type override;

private:
    // In all modes, `eback()` (beginning of get area) and `pbase()` (beginning of put area)
    // both point to the beginning of the buffer (`m_buffer`).
    //
    // In all modes, `m_base` marks the position in `m_buffer` of the logical file pointer
    // of the text file implementation (`m_text_file_impl`).
    //
    // In reading mode, `egptr()` (end of get area) marks the position in `m_buffer` of the
    // read-ahead pointer of the text file implementation (`m_text_file_impl`).
    //
    // In writing mode, `epptr()` (end of put area) point to the end of the buffer
    // (`m_buffer`).
    //
    // In reading, and in neutral mode, `epptr()` (end of put area) point to the beginning
    // of the buffer (`m_buffer`), so the put area is empty.
    //
    // In writing, and in neutral mode, `egptr()` (end of get area) point to the beginning
    // of the buffer (`m_buffer`), so the get area is empty.
    //
    // In reading, and in neutral mode, `eback() <= m_base <= gptr()`.
    //
    // In writing, and in neutral mode, `pbase() <= m_base <= pptr()`.
    //
    I m_text_file_impl;
    core::Buffer<char_type> m_buffer;
    char_type* m_base;

    bool m_disable_autounshift;

    // Mode      `m_reading`    `m_writing`
    // --------------------------------------
    // neutral   false          false
    // reading   true           false
    // writing   false          true
    //
    // INVARIANT: If the implementation (`m_text_file_impl`) is in reading mode, this stream
    // buffer is in reading mode.
    //
    // INVARIANT: If the implementation (`m_text_file_impl`) is in writing mode, this stream
    // buffer is in writing mode.
    //
    // INVARIANT: If this stream buffer is in neutral mode, the implementation
    // (`m_text_file_impl`) is in neutral mode.
    //
    bool m_reading = false;
    bool m_writing = false;

    bool do_sync(std::error_code&);
    bool discard(std::error_code&);
    void advance();
    bool ensure_reading(std::error_code&);
    bool ensure_writing(std::error_code&);
    bool read(core::Span<char_type> buffer, std::size_t& n, std::error_code&);
    bool write(core::Span<const char_type> data, std::size_t& n, std::error_code&);
    void bump_get_ptr(std::size_t) noexcept;
    void bump_put_ptr(std::size_t) noexcept;
    bool flush(std::error_code&);
    bool pull(std::error_code&);
    bool push(std::error_code&);
};



template<class C, class T = std::char_traits<C>, class D = core::BasicCharCodec<C, T>> using BasicTextFileStreambuf =
    GenericTextFileStreambuf<core::TextFileImpl<C, T, D>>;
template<class C, class T = std::char_traits<C>, class D = core::BasicCharCodec<C, T>> using BasicPosixTextFileStreambuf =
    GenericTextFileStreambuf<core::PosixTextFileImpl<C, T, D>>;
template<class C, class T = std::char_traits<C>, class D = core::BasicCharCodec<C, T>> using BasicWindowsTextFileStreambuf =
    GenericTextFileStreambuf<core::WindowsTextFileImpl<C, T, D>>;


using TextFileStreambuf        = BasicTextFileStreambuf<char>;
using PosixTextFileStreambuf   = BasicPosixTextFileStreambuf<char>;
using WindowsTextFileStreambuf = BasicWindowsTextFileStreambuf<char>;

using WideTextFileStreambuf        = BasicTextFileStreambuf<wchar_t>;
using WidePosixTextFileStreambuf   = BasicPosixTextFileStreambuf<wchar_t>;
using WideWindowsTextFileStreambuf = BasicWindowsTextFileStreambuf<wchar_t>;



/// \brief Configuration parameters for text file streams.
///
/// Objects of this type are used to pass configuration parameters to file streams, and to
/// file stream buffers.
///
template<class I> struct GenericTextFileStreambuf<I>::Config {
    /// \brief Disable automatic unshift mode.
    ///
    /// When set to `true`, automatic unshift mode is disabled.
    ///
    /// When automatic unshift mode is enabled (it is enabled by default), an unshift
    /// operation is automatically carried out as part of the flush operation. The flush
    /// operation occurs whenever `sync()` is invoked on the stream buffer, which it is
    /// whenever `flush()` is invoked on the stream (\ref GenericTextFileStream); whenever
    /// `seekpos()` is invoked on the stream buffer, which is whenever `seekg()` or
    /// `seekp()` is invoked on the stream with an absolute position as argument; and
    /// whenever there is a switch from writing mode to reading mode.
    ///
    /// When automatic unshift mode is disabled, unshifting only happens when explicitely
    /// called upon through an invocation of \ref unshift(), which happens when \ref
    /// GenericTextFileStream::unshift() is invoked.
    ///
    bool disable_autounshift = false;

    /// \{
    ///
    /// \brief Buffer size and memory.
    ///
    /// `buffer_size` specifies the size of the memory buffer to be used by the stream
    /// buffer. `buffer_memory` specifies a chunk of memory that could be used.
    ///
    /// If the specified size is greater than the amount of specified memory, a buffer of
    /// the specified size will be dynamically allocated.
    ///
    /// Also, if the specified buffer size, or the amount of specified memory is too small
    /// for the stream buffer to operate, a larger buffer will be allocated, although the
    /// size will be kept as small as possible.
    ///
    static constexpr std::size_t default_buffer_size = 4096;
    std::size_t buffer_size = default_buffer_size;
    core::Span<char_type> buffer_memory;
    /// \}

    /// \brief Text file implementation configuration.
    ///
    /// Configuration parameters specific to the selected text file implementation.
    ///
    typename impl_type::Config impl;
};








// Implementation


// ============================ GenericTextFileStream ============================


template<class I>
inline GenericTextFileStream<I>::GenericTextFileStream(core::FilesystemPathRef path, Mode mode, Config config)
    : GenericTextFileStream(core::File(path, mode), std::move(config)) // Throws
{
}


template<class I>
inline GenericTextFileStream<I>::GenericTextFileStream(core::File file, Config config)
    : std::basic_iostream<typename I::char_type, typename I::traits_type>(nullptr) // Throws
    , m_file(std::move(file)) // Throws
    , m_streambuf(m_file, this->getloc(), std::move(config)) // Throws
{
    this->rdbuf(&m_streambuf); // Throws
}


template<class I>
inline GenericTextFileStream<I>::GenericTextFileStream(core::File* file, Config config)
    : std::basic_iostream<typename I::char_type, typename I::traits_type>(nullptr) // Throws
    , m_streambuf(*file, this->getloc(), std::move(config)) // Throws
{
    this->rdbuf(&m_streambuf); // Throws
}


template<class I>
inline void GenericTextFileStream<I>::unshift()
{
    using char_type   = typename I::char_type;
    using traits_type = typename I::traits_type;
    typename std::basic_ostream<char_type, traits_type>::sentry sentry(*this); // Throws
    if (ARCHON_LIKELY(sentry)) {
        try {
            if (ARCHON_LIKELY(m_streambuf.unshift())) // Throws
                return;
        }
        catch (...) {}
        this->setstate(std::ios_base::badbit); // Throws
    }
}



// ============================ GenericTextFileStreambuf ============================


template<class I>
inline GenericTextFileStreambuf<I>::GenericTextFileStreambuf(core::File& file)
    : GenericTextFileStreambuf(file, std::locale()) // Throws
{
}


template<class I>
inline GenericTextFileStreambuf<I>::GenericTextFileStreambuf(core::File& file, const std::locale& locale)
    : GenericTextFileStreambuf(file, locale, {}) // Throws
{
}


template<class I>
inline GenericTextFileStreambuf<I>::GenericTextFileStreambuf(core::File& file, const std::locale& locale,
                                                             Config config)
    : m_text_file_impl(file, &locale, std::move(config.impl)) // Throws
    , m_buffer(config.buffer_memory, config.buffer_size) // Throws
    , m_disable_autounshift(config.disable_autounshift)
{
    typename impl_type::state_type state = {};
    m_text_file_impl.reset(state);

    // Buffer must not be empty
    m_buffer.reserve(1); // Throws

    // Start out in neutral mode
    m_base = m_buffer.data();
    this->setg(m_base, m_base, m_base);
    this->setp(m_base, m_base);
}


template<class I>
inline bool GenericTextFileStreambuf<I>::unshift()
{
    if constexpr (impl_type::has_degen_unshift)
        return true;
    std::error_code ec; // Dummy
    if (ARCHON_LIKELY(ensure_writing(ec))) { // Throws
        if (ARCHON_LIKELY(push(ec))) // Throws
            return m_text_file_impl.unshift(ec); // Throws
    }
    return false;
}


template<class I>
void GenericTextFileStreambuf<I>::imbue(const std::locale& loc)
{
    // Ideally, this function should not have had any failure modes, because it is called
    // from `std::basic_ios<C, T>::imbue()`, and after that function has made changes that
    // are not guaranteed to be reversible. But, eliminating all failure modes from this
    // function is essentially impossible. Ignoring practical limitations, one resolution
    // would be to modify the C++ standard by introducing a two-phase commit protocol for
    // changing the locale in a stream.

    std::error_code ec;
    if (ARCHON_LIKELY(do_sync(ec))) {
        ARCHON_ASSERT(!m_reading);
        ARCHON_ASSERT(!m_writing);
        typename traits_type::state_type state = {}; // Best guess
        m_text_file_impl.imbue(loc, state); // Throws
        return;
    }
    throw std::system_error(ec, "Failed to synchronize");
}


template<class I>
auto GenericTextFileStreambuf<I>::xsgetn(char_type* s, std::streamsize count) -> std::streamsize
{
    if (ARCHON_LIKELY(count > 0)) {
        std::error_code ec; // Dummy
        if (ARCHON_LIKELY(ensure_reading(ec))) { // Throws
            std::size_t size = 0;
            if (ARCHON_LIKELY(core::try_int_cast(count, size))) {
                std::size_t n = 0;
                read({ s, size }, n, ec); // Throws
                return std::streamsize(n);
            }
            throw std::length_error("xsgetn count");
        }
    }
    return 0;
}


template<class I>
auto GenericTextFileStreambuf<I>::xsputn(const char_type* s, std::streamsize count) -> std::streamsize
{
    if (ARCHON_LIKELY(count > 0)) {
        std::error_code ec; // Dummy
        if (ARCHON_LIKELY(ensure_writing(ec))) { // Throws
            std::size_t size = 0;
            if (ARCHON_LIKELY(core::try_int_cast(count, size))) {
                std::size_t n = 0;
                write({ s, size }, n, ec); // Throws
                return std::streamsize(n);
            }
            throw std::length_error("xsputn count");
        }
    }
    return 0;
}


template<class I>
auto GenericTextFileStreambuf<I>::underflow() -> int_type
{
    ARCHON_ASSERT(this->gptr() == this->egptr());

    std::error_code ec; // Dummy
    if (ARCHON_LIKELY(ensure_reading(ec))) { // Throws
        if (ARCHON_LIKELY(pull(ec))) { // Throws
            bool end_of_file = (this->egptr() == this->gptr());
            if (ARCHON_LIKELY(!end_of_file)) {
                char_type ch = *this->gptr(); // Throws
                return traits_type::to_int_type(ch);
            }
            return traits_type::eof(); // End of file
        }
    }
    return traits_type::eof(); // Failure
}


template<class I>
auto GenericTextFileStreambuf<I>::uflow() -> int_type
{
    ARCHON_ASSERT(this->gptr() == this->egptr());

    std::error_code ec; // Dummy
    if (ARCHON_LIKELY(ensure_reading(ec))) { // Throws
        if (ARCHON_LIKELY(pull(ec))) { // Throws
            bool end_of_file = (this->egptr() == this->gptr());
            if (ARCHON_LIKELY(!end_of_file)) {
                char_type ch = *this->gptr(); // Throws
                this->gbump(1);
                return traits_type::to_int_type(ch);
            }
            return traits_type::eof(); // End of file
        }
    }
    return traits_type::eof(); // Failure
}


template<class I>
auto GenericTextFileStreambuf<I>::overflow(int_type ch) -> int_type
{
    ARCHON_ASSERT(this->pptr() == this->epptr());
    std::error_code ec; // Dummy
    if (ARCHON_LIKELY(ensure_writing(ec))) { // Throws
        if (ARCHON_LIKELY(push(ec))) { // Throws
            if (!traits_type::eq_int_type(ch, traits_type::eof())) {
                ARCHON_ASSERT(this->pptr() < this->epptr());
                *this->pptr() = traits_type::to_char_type(ch); // Throws
                this->pbump(1); // Throws
            }
            return traits_type::not_eof(ch); // Success
        }
    }
    return traits_type::eof(); // Failure
}


template<class I>
int GenericTextFileStreambuf<I>::sync()
{
    std::error_code ec; // Dummy
    if (ARCHON_LIKELY(do_sync(ec))) // Throws
        return 0; // Success
    return -1; // Failure
}


template<class I>
auto GenericTextFileStreambuf<I>::seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode) ->
    pos_type
{
    if (ARCHON_LIKELY(off == 0 && dir == std::ios_base::cur)) {
        pos_type pos = off_type(0);
        std::error_code ec; // Dummy
        if (ARCHON_LIKELY(m_writing)) {
            if (ARCHON_LIKELY(push(ec))) { // Throws
                if (ARCHON_LIKELY(m_text_file_impl.tell_write(pos, ec))) // Throws
                    return pos; // Success
            }
        }
        else {
            advance(); // Throws
            if (ARCHON_LIKELY(m_text_file_impl.tell_read(pos, ec))) // Throws
                return pos; // Success
        }
    }
    return pos_type(off_type(-1)); // Failure
}


template<class I>
auto GenericTextFileStreambuf<I>::seekpos(pos_type pos, std::ios_base::openmode) -> pos_type
{
    std::error_code ec; // Dummy
    if (ARCHON_LIKELY(!m_writing)) {
      seek:
        if (ARCHON_LIKELY(m_text_file_impl.seek(pos, ec))) { // Throws
            // Enter into neutral mode (or stay there)
            m_base = m_buffer.data();
            this->setg(m_base, m_base, m_base);
            m_reading = false;
            return pos; // Success
        }
    }
    else {
        if (ARCHON_LIKELY(flush(ec))) // Throws
            goto seek;
    }
    return pos_type(off_type(-1)); // Failure
}


template<class I>
inline bool GenericTextFileStreambuf<I>::do_sync(std::error_code& ec)
{
    if (ARCHON_LIKELY(m_writing))
        return flush(ec); // Throws
    if (ARCHON_LIKELY(!m_reading))
        return true;
    return discard(ec); // Throws
}


template<class I>
bool GenericTextFileStreambuf<I>::discard(std::error_code& ec)
{
    advance(); // Throws
    if (ARCHON_LIKELY(m_text_file_impl.discard(ec))) { // Throws
        // Enter into neutral mode (or stay there)
        m_base = m_buffer.data();
        this->setg(m_base, m_base, m_base);
        m_reading = false;
        return true;
    }
    return false;
}


template<class I>
inline void GenericTextFileStreambuf<I>::advance()
{
    ARCHON_ASSERT(!m_writing);
    ARCHON_ASSERT(this->gptr() >= m_base);
    std::size_t n = std::size_t(this->gptr() - m_base);
    m_text_file_impl.advance(n); // Throws
    m_base = this->gptr();
}


template<class I>
inline bool GenericTextFileStreambuf<I>::ensure_reading(std::error_code& ec)
{
    if (ARCHON_LIKELY(m_reading))
        return true;

    if (ARCHON_LIKELY(!m_writing)) {
      proceed:
        ARCHON_ASSERT(!m_reading);
        // Enter into reading mode
        m_reading = true;
        return true;
    }

    if (ARCHON_LIKELY(flush(ec))) // Throws
        goto proceed;
    return false;
}


template<class I>
inline bool GenericTextFileStreambuf<I>::ensure_writing(std::error_code& ec)
{
    if (ARCHON_LIKELY(m_writing))
        return true;

    if (ARCHON_LIKELY(!m_reading)) {
      proceed:
        ARCHON_ASSERT(!m_writing);
        // Enter into writing mode
        ARCHON_ASSERT(m_buffer.size() > 0);
        this->setp(m_base, m_base + m_buffer.size());
        m_writing = true;
        return true;
    }

    if (ARCHON_LIKELY(discard(ec))) // Throws
        goto proceed;
    return false;
}


template<class I>
inline bool GenericTextFileStreambuf<I>::read(core::Span<char_type> buffer, std::size_t& n, std::error_code& ec)
{
    ARCHON_ASSERT(m_reading);
    core::Span buffer_2 = buffer;
    std::size_t avail = std::size_t(this->egptr() - this->gptr());
  again:
    std::size_t n_2 = std::min(buffer_2.size(), avail);
    std::copy_n(this->gptr(), n_2, buffer_2.data());
    bump_get_ptr(n_2);
    buffer_2 = buffer_2.subspan(n_2);
    if (ARCHON_LIKELY(buffer_2.size() == 0)) {
        n = buffer.size();
        return true; // Success
    }
    if (ARCHON_LIKELY(pull(ec))) { // Throws
        avail = std::size_t(this->egptr() - this->gptr());
        bool end_of_file = (avail == 0);
        if (ARCHON_LIKELY(!end_of_file))
            goto again;
        n = std::size_t(buffer.size() - buffer_2.size());
        return true; // End of file
    }
    n = std::size_t(buffer.size() - buffer_2.size());
    return false; // Failure
}


template<class I>
inline bool GenericTextFileStreambuf<I>::write(core::Span<const char_type> data, std::size_t& n, std::error_code& ec)
{
    ARCHON_ASSERT(m_writing);
    core::Span data_2 = data;
  again:
    std::size_t capacity = std::size_t(this->epptr() - this->pptr());
    std::size_t n_2 = std::min(data_2.size(), capacity);
    std::copy_n(data_2.data(), n_2, this->pptr());
    bump_put_ptr(n_2);
    data_2 = data_2.subspan(n_2);
    if (ARCHON_LIKELY(data_2.size() == 0)) {
        n = data.size();
        return true; // Success
    }
    if (ARCHON_LIKELY(push(ec))) { // Throws
        ARCHON_ASSERT(this->pptr() < this->epptr());
        goto again;
    }
    n = std::size_t(data.size() - data_2.size());
    return false; // Failure
}


template<class I>
inline void GenericTextFileStreambuf<I>::bump_get_ptr(std::size_t n) noexcept
{
    std::size_t n_2 = n;
    int max = std::numeric_limits<int>::max();
    if (ARCHON_LIKELY(n_2 <= unsigned(max)))
        goto finish;
    do {
        this->gbump(max);
        n_2 -= unsigned(max);
    }
    while (n_2 > unsigned(max));
  finish:
    this->gbump(int(n_2));
}


template<class I>
inline void GenericTextFileStreambuf<I>::bump_put_ptr(std::size_t n) noexcept
{
    std::size_t n_2 = n;
    int max = std::numeric_limits<int>::max();
    if (ARCHON_LIKELY(n_2 <= unsigned(max)))
        goto finish;
    do {
        this->pbump(max);
        n_2 -= unsigned(max);
    }
    while (n_2 > unsigned(max));
  finish:
    this->pbump(int(n_2));
}


template<class I>
inline bool GenericTextFileStreambuf<I>::flush(std::error_code& ec)
{
    if (ARCHON_LIKELY(push(ec))) { // Throws
        if (ARCHON_LIKELY(!impl_type::has_degen_unshift && !m_disable_autounshift)) {
            ARCHON_ASSERT(m_writing);
            if (ARCHON_LIKELY(m_text_file_impl.unshift(ec))) // Throws
                goto proceed;
            goto fallback;
        }
      proceed:
        if (ARCHON_LIKELY(m_text_file_impl.flush(ec))) { // Throws
            // Enter into neutral mode
            m_base = m_buffer.data();
            this->setp(m_base, m_base);
            m_writing = false;
            return true; // Success
        }
        return false; // Failure
    }

  fallback:
    // Even when everything in the local buffer could not be written to the implementation
    // layer (`m_impl`), an attempt to recursively flush the part, that could be writen,
    // should still be made.
    std::error_code ec_2; // Dummy
    static_cast<void>(m_text_file_impl.flush(ec_2)); // Throws
    return false; // Failure
}


template<class I>
inline bool GenericTextFileStreambuf<I>::pull(std::error_code& ec)
{
    ARCHON_ASSERT(m_reading);
    ARCHON_ASSERT(this->gptr() == this->egptr());
    m_text_file_impl.advance(); // Throws
    m_base = m_buffer.data();
    this->setg(m_base, m_base, m_base);
    std::size_t n = 0;
    bool dynamic_eof = false; // Unfortunately not useful in context of STL streams
    if (m_text_file_impl.read_ahead(m_buffer, dynamic_eof, n, ec)) { // Throws
        this->setg(m_base, m_base, m_base + n);
        return true;
    }
    return false;
}


template<class I>
inline bool GenericTextFileStreambuf<I>::push(std::error_code& ec)
{
    ARCHON_ASSERT(m_writing);
    ARCHON_ASSERT(this->pptr() >= m_base);
    auto data = core::Span<char_type>(m_base, this->pptr());
    std::size_t n = 0;
    if (ARCHON_LIKELY(m_text_file_impl.write(data, n, ec))) { // Throws
        ARCHON_ASSERT(m_buffer.size() > 0);
        m_base = m_buffer.data();
        this->setp(m_base, m_base + m_buffer.size());
        return true;
    }
    m_base += n;
    return false;
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_TEXT_FILE_STREAM_HPP
