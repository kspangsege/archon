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

#ifndef ARCHON_X_CORE_X_TEXT_FILE_HPP
#define ARCHON_X_CORE_X_TEXT_FILE_HPP

/// \file


#include <cstddef>
#include <utility>
#include <string>
#include <system_error>
#include <locale>
#include <filesystem>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/string_span.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/char_mapper.hpp>
#include <archon/core/char_codec.hpp>
#include <archon/core/filesystem.hpp>
#include <archon/core/file.hpp>
#include <archon/core/text_file_impl.hpp>


namespace archon::core {


/// \brief File accessor specialized for text files.
///
/// A text file object generally represents an open text file. A text file is a file that
/// contains text as opposed to binary data.
///
/// With suitable choices of implementation (\p I), a text file object can efficiently
/// perform locale based translation between external and internal character
/// representations, and translation between external and internal newline representation on
/// the Windows platform. More generally, the exact behavior of a text file object depends
/// on the selected implementation. See \ref core::TextFile and \ref core::WideTextFile.
///
/// This class does not guarantee efficiency when reading or writing many small chunks of
/// data. It may, or may not be efficient based on whether buffers are present for other
/// purposes (character or newline translation). See \ref core::GenericBufferedTextFile for
/// an alternative that does guarantee efficiency when reading or writing many small chunks
/// of data.
///
/// This class is designed to work with text files. See \ref core::File for an alternative
/// that is appropriate for use with binary files.
///
/// \tparam I The text file implementation type to be used. This must be a type that
/// satisfies \ref Concept_Archon_Core_TextFileImpl.
///
/// FIXME: Consider adding sync() which will effectively flush() when in writing mode, and will revert file pointer and discard buffered data when in reading mode.          
///
template<class I> class GenericTextFile {
public:
    using impl_type = I;

    using char_type   = typename impl_type::char_type;
    using traits_type = typename impl_type::traits_type;
    using codec_type  = typename impl_type::codec_type;
    using pos_type    = typename impl_type::pos_type;
    using state_type  = typename impl_type::state_type;

    struct Config;

    using Mode = core::File::Mode;

    using AccessMode = core::File::AccessMode;
    using CreateMode = core::File::CreateMode;
    using WriteMode  = core::File::WriteMode;

    using string_type = std::basic_string<char_type, traits_type>;

    explicit GenericTextFile(core::FilesystemPathRef, const std::locale& = {});
    explicit GenericTextFile(core::FilesystemPathRef, Mode, const std::locale& = {});
    explicit GenericTextFile();
    explicit GenericTextFile(const std::locale&);
    explicit GenericTextFile(const std::locale&, Config);
    explicit GenericTextFile(const std::locale*, Config);

    void open(core::FilesystemPathRef, Mode = Mode::read);
    void open(core::FilesystemPathRef, AccessMode, CreateMode, WriteMode);
    void open(core::File, state_type = {}) noexcept;

    void close() noexcept;

    /// \{
    ///
    /// \brief Has association with open file.
    ///
    /// Returns true if, and only if this file object is currently associated with an open
    /// file, or file-like entity.
    ///
    bool is_open() const noexcept;
    explicit operator bool() const noexcept;
    /// \}

    auto read(core::Span<char_type> buffer) -> std::size_t;

    void write(core::StringSpan<char_type> data);

    auto read_some(core::Span<char_type> buffer) -> std::size_t;

    auto read_all() -> string_type;

    /// \brief Generate bytes to revert to initial shift state.
    ///
    /// If this file uses a stateful character codec, and if the shift state at the current
    /// position of the file pointer is not the initial shift state, this function produces
    /// a byte sequence that brings the shift state back to the initial shift state.
    ///
    /// A subsequent flushing operation (\ref flush()) is necessary to ensure that the
    /// produced bytes are written to the underlying medium.
    ///
    /// By default, unshifting happens automatically as part of every flush operation (\ref
    /// flsuh()), but see \ref Config::disable_autounshift.
    ///
    void unshift();

    /// \brief Write buffered data to file.
    ///
    /// If encoding of a character fails, `flush()` will still flush everything up to the
    /// point of the failure.
    ///
    void flush();

    auto tell() -> pos_type;

    void seek(pos_type);

    [[nodiscard]] bool try_open(core::FilesystemPathRef, Mode, std::error_code&);
    [[nodiscard]] bool try_open(core::FilesystemPathRef, AccessMode, CreateMode, WriteMode, std::error_code&);

    [[nodiscard]] bool try_read(core::Span<char_type> buffer, std::size_t& n, std::error_code&);

    [[nodiscard]] bool try_write(core::StringSpan<char_type> data, std::size_t& n, std::error_code&);

    [[nodiscard]] bool try_read_some(core::Span<char_type> buffer, std::size_t& n, std::error_code&);

    [[nodiscard]] bool try_read_all(core::Buffer<char_type>&, std::size_t& offset, std::error_code&);

    [[nodiscard]] bool try_unshift(std::error_code&);

    [[nodiscard]] bool try_flush(std::error_code&);

    [[nodiscard]] bool try_tell(pos_type&, std::error_code&);

    [[nodiscard]] bool try_seek(pos_type, std::error_code&);

    static auto load(core::FilesystemPathRef, const std::locale& = {}) -> string_type;

    static void save(core::FilesystemPathRef, core::StringSpan<char_type> data, const std::locale& = {});

    static auto load_and_chomp(core::FilesystemPathRef, const std::locale& = {}) -> string_type;

    /// \brief Try to load text from file.
    ///
    /// This function tries to load the contents of the specified text file into the
    /// specified buffer starting at the specified offset within the buffer. The buffer will
    /// be expanded as necessary. Characters will be decoded in accordance with the selected
    /// locale and the selected newline transformation mode.
    ///
    /// Behavior is undefined if \p offset is greater than the size of the buffer prior to
    /// the invocation of this function.
    ///
    /// On success, this function returns `true` after increasing \p offset by the number of
    /// loaded characters. On failure, it returns `false` after setting \p ec to an error
    /// code that reflects the cause of the failure. On failure, \p offset will still have
    /// been increased by the number of characters that were successfully read.
    ///
    [[nodiscard]] static bool try_load(core::FilesystemPathRef, core::Buffer<char_type>&, std::size_t& offset,
                                       std::error_code& ec, const std::locale& = {});

    /// \brief Try to save text to file.
    ///
    /// This function tries to save the specified text to a file at the specified path. If
    /// the file already exists, it will be truncated (\ref core::File::WriteMode::trunc)
    /// before the new data is written. Characters will be encoded in accordance with the
    /// selected locale and the selected newline transformation mode.
    ///
    /// On success, this function returns `true`. On failure, it returns `false` after
    /// setting \p ec to an error code that reflects the cause of the failure.
    ///
    [[nodiscard]] static bool try_save(core::FilesystemPathRef, core::StringSpan<char_type> data,
                                       std::error_code& ec, const std::locale& = {});

    /// \brief Try to load text from file and remove last newline character.
    ///
    /// This function has the same effect as \ref try_load() except that on success, if the
    /// last loaded character is a newline character, that newline character will be
    /// excluded from the resulting span of characters.
    ///
    [[nodiscard]] static bool try_load_and_chomp(core::FilesystemPathRef, core::Buffer<char_type>&,
                                                 std::size_t& offset, std::error_code&, const std::locale& = {});

protected:
    core::File m_file;
    impl_type m_impl;
    const bool m_dynamic_eof;
    const bool m_disable_autounshift;

    // Mode      `m_reading`    `m_writing`
    // --------------------------------------
    // neutral   false          false
    // reading   true           false
    // writing   false          true
    //
    // INVARIANT: If the implementation (`m_impl`) is in reading mode, this text file object
    // is in reading mode.
    //
    // INVARIANT: If the implementation (`m_impl`) is in writing mode, this text file object
    // is in writing mode.
    //
    // INVARIANT: If this text file object is in neutral mode, the implementation (`m_impl`)
    // is in neutral mode.
    //
    bool m_reading = false;
    bool m_writing = false;

    bool stop_reading(std::error_code&);
    bool stop_writing(std::error_code&);
    bool do_read_some(core::Span<char_type> buffer, std::size_t& n, std::error_code&);
};


template<class C, class T = std::char_traits<C>, class D = core::BasicCharCodec<C, T>> using BasicTextFile =
    GenericTextFile<core::TextFileImpl<C, T, D>>;
template<class C, class T = std::char_traits<C>, class D = core::BasicCharCodec<C, T>> using BasicPosixTextFile =
    GenericTextFile<core::PosixTextFileImpl<C, T, D>>;
template<class C, class T = std::char_traits<C>, class D = core::BasicCharCodec<C, T>> using BasicWindowsTextFile =
    GenericTextFile<core::WindowsTextFileImpl<C, T, D>>;


using TextFile        = BasicTextFile<char>;
using PosixTextFile   = BasicPosixTextFile<char>;
using WindowsTextFile = BasicWindowsTextFile<char>;

using WideTextFile        = BasicTextFile<wchar_t>;
using WidePosixTextFile   = BasicPosixTextFile<wchar_t>;
using WideWindowsTextFile = BasicWindowsTextFile<wchar_t>;


/// \brief Text file configuration parameters.
///
/// Objects of this type are used to pass configuration parameters to text files.
///
template<class I> struct GenericTextFile<I>::Config {
    /// \brief Dynamic end-of-file mode.
    ///
    /// When set to true, dynamic end-of-file mode is enabled.
    ///
    /// When dynamic end-of-file mode is disabled (it is disabled by default), a partial
    /// character at the end of the file will result in a read error. This behavior makes
    /// sense for files that remain static while being read. Its advantage is that it
    /// prevents a final partial byte from going unnoticed.
    ///
    /// When dynamic end-of-file mode is enabled, a partial character at the end of the file
    /// will cause end-of-file to be reported as though the partial character was not
    /// there. Later on, if more data is added to the file, completing the partial
    /// character, reading can be resumed without missing any bytes. This behavior makes
    /// sense for files that are read while also being appended to.
    ///
    bool dynamic_eof = false;

    /// \brief Disable automatic unshift mode.
    ///
    /// When set to `true`, automatic unshift mode is disabled.
    ///
    /// When automatic unshift mode is enabled (it is enabled by default), an unshift
    /// operation (\ref unshift()) is automatically carried out as part of every flush
    /// operation. The flush operation occurs whenever \ref flush(), \ref try_flush(), \ref
    /// seek(), or \ref try_seek() is called, and whenever there is a switch from writing
    /// mode to reading mode.
    ///
    /// When automatic unshift mode is disabled, unshifting only happens when explicitely
    /// called upon through an invocation of \ref unshift().
    ///
    bool disable_autounshift = false;

    /// \brief Text file implementation configuration.
    ///
    /// Configuration parameters specific to the selected text file implementation.
    ///
    typename impl_type::Config impl;
};








// Implementation


template<class I>
inline GenericTextFile<I>::GenericTextFile(core::FilesystemPathRef path, const std::locale& locale)
    : GenericTextFile(path, Mode::read, locale) // Throws
{
}


template<class I>
inline GenericTextFile<I>::GenericTextFile(core::FilesystemPathRef path, Mode mode, const std::locale& locale)
    : GenericTextFile(locale) // Throws
{
    open(path, mode); // Throws
}


template<class I>
inline GenericTextFile<I>::GenericTextFile()
    : GenericTextFile(nullptr, {}) // Throws
{
}


template<class I>
inline GenericTextFile<I>::GenericTextFile(const std::locale& locale)
    : GenericTextFile(&locale, {}) // Throws
{
}


template<class I>
inline GenericTextFile<I>::GenericTextFile(const std::locale& locale, Config config)
    : GenericTextFile(&locale, std::move(config)) // Throws
{
}


template<class I>
inline GenericTextFile<I>::GenericTextFile(const std::locale* locale, Config config)
    : m_impl(m_file, locale, std::move(config.impl)) // Throws
    , m_dynamic_eof(config.dynamic_eof)
    , m_disable_autounshift(config.disable_autounshift)
{
    state_type state = {};
    m_impl.reset(state);
}


template<class I>
inline void GenericTextFile<I>::open(core::FilesystemPathRef path, Mode mode)
{
    std::error_code ec;
    if (ARCHON_LIKELY(try_open(path, mode, ec))) // Throws
        return; // Success
    throw std::filesystem::filesystem_error("Failed to open file", path, ec);
}


template<class I>
inline void GenericTextFile<I>::open(core::FilesystemPathRef path, AccessMode access_mode, CreateMode create_mode,
                                     WriteMode write_mode)
{
    std::error_code ec;
    if (ARCHON_LIKELY(try_open(path, access_mode, create_mode, write_mode, ec))) // Throws
        return; // Success
    throw std::filesystem::filesystem_error("Failed to open file", path, ec);
}


template<class I>
inline void GenericTextFile<I>::open(core::File file, state_type state) noexcept
{
    m_file = std::move(file);
    m_impl.reset(state);
}


template<class I>
inline void GenericTextFile<I>::close() noexcept
{
    m_file.close();
}


template<class I>
inline bool GenericTextFile<I>::is_open() const noexcept
{
    return m_file.is_open();
}


template<class I>
inline GenericTextFile<I>::operator bool() const noexcept
{
    return is_open();
}


template<class I>
inline auto GenericTextFile<I>::read(core::Span<char_type> buffer) -> std::size_t
{
    std::size_t n = 0;
    std::error_code ec;
    if (ARCHON_LIKELY(try_read(buffer, n, ec))) // Throws
        return n; // Success
    throw std::system_error(ec, "Failed to read from file");
}


template<class I>
inline void GenericTextFile<I>::write(core::StringSpan<char_type> data)
{
    std::size_t n; // Dummy
    std::error_code ec;
    if (ARCHON_LIKELY(try_write(data, n, ec))) // Throws
        return; // Success
    throw std::system_error(ec, "Failed to write to file");
}


template<class I>
inline auto GenericTextFile<I>::read_some(core::Span<char_type> buffer) -> std::size_t
{
    std::size_t n = 0;
    std::error_code ec;
    if (ARCHON_LIKELY(try_read_some(buffer, n, ec))) // Throws
        return n; // Success
    throw std::system_error(ec, "Failed to read from file");
}


template<class I>
inline auto GenericTextFile<I>::read_all() -> string_type
{
    core::Buffer<char_type> buffer;
    std::size_t offset = 0;
    std::error_code ec;
    if (ARCHON_LIKELY(try_read_all(buffer, offset, ec))) // Throws
        return string_type(buffer.data(), offset); // Throws
    throw std::system_error(ec, "Failed to read from file");
}


template<class I>
inline void GenericTextFile<I>::unshift()
{
    std::error_code ec;
    if (ARCHON_LIKELY(try_unshift(ec))) // Throws
        return; // Success
    throw std::system_error(ec, "Failed to unshift");
}


template<class I>
inline void GenericTextFile<I>::flush()
{
    std::error_code ec;
    if (ARCHON_LIKELY(try_flush(ec))) // Throws
        return; // Success
    throw std::system_error(ec, "Failed to flush");
}


template<class I>
inline auto GenericTextFile<I>::tell() -> pos_type
{
    pos_type pos;
    std::error_code ec;
    if (ARCHON_LIKELY(try_tell(pos, ec))) // Throws
        return pos; // Success
    throw std::system_error(ec, "Failed to determine position of file pointer");
}


template<class I>
inline void GenericTextFile<I>::seek(pos_type pos)
{
    std::error_code ec;
    if (ARCHON_LIKELY(try_seek(pos, ec))) // Throws
        return; // Success
    throw std::system_error(ec, "Failed to update position of file pointer");
}


template<class I>
inline bool GenericTextFile<I>::try_open(core::FilesystemPathRef path, Mode mode, std::error_code& ec)
{
    if (ARCHON_LIKELY(m_file.try_open(path, mode, ec))) { // Throws
        state_type state = {};
        m_impl.reset(state);
        return true; // Success
    }
    return false; // Failure
}


template<class I>
inline bool GenericTextFile<I>::try_open(core::FilesystemPathRef path, AccessMode access_mode, CreateMode create_mode,
                                         WriteMode write_mode, std::error_code& ec)
{
    if (ARCHON_LIKELY(m_file.try_open(path, access_mode, create_mode, write_mode, ec))) { // Throws
        state_type state = {};
        m_impl.reset(state);
        return true; // Success
    }
    return false; // Failure
}


template<class I>
inline bool GenericTextFile<I>::try_read(core::Span<char_type> buffer, std::size_t& n, std::error_code& ec)
{
    if (ARCHON_LIKELY(!m_writing)) {
      proceed:
        m_reading = true;
        core::Span<char_type> buffer_2 = buffer;
        std::size_t n_2 = 0;
        while (ARCHON_LIKELY(do_read_some(buffer_2, n_2, ec))) {
            ARCHON_ASSERT(n_2 <= buffer_2.size());
            if (ARCHON_LIKELY(n_2 > 0 && n_2 < buffer_2.size())) {
                buffer_2 = buffer_2.subspan(n_2);
                continue;
            }
            n = std::size_t(buffer_2.data() + n_2 - buffer.data());
            return true; // Success
        }
        n = std::size_t(buffer_2.data() - buffer.data());
        return false; // Failure to read
    }
    if (ARCHON_LIKELY(stop_writing(ec))) // Throws
        goto proceed;
    n = 0;
    return false; // Failure to stop writing
}


template<class I>
inline bool GenericTextFile<I>::try_write(core::StringSpan<char_type> data, std::size_t& n, std::error_code& ec)
{
    if (ARCHON_LIKELY(!m_reading)) {
      proceed:
        m_writing = true;
        return m_impl.write(data, n, ec); // Throws
    }
    if (ARCHON_LIKELY(stop_reading(ec))) // Throws
        goto proceed;
    n = 0;
    return false; // Failure to stop reading
}


template<class I>
inline bool GenericTextFile<I>::try_read_some(core::Span<char_type> buffer, std::size_t& n, std::error_code& ec)
{
    if (ARCHON_LIKELY(!m_writing)) {
      proceed:
        m_reading = true;
        return do_read_some(buffer, n, ec); // Throws
    }
    if (ARCHON_LIKELY(stop_writing(ec))) // Throws
        goto proceed;
    return false; // Failure to stop writing
}


template<class I>
bool GenericTextFile<I>::try_read_all(core::Buffer<char_type>& buffer, std::size_t& offset, std::error_code& ec)
{
    std::size_t n = 0;
    std::size_t min_extra_capacity = 256;
  again:
    buffer.reserve_extra(min_extra_capacity, offset); // Throws
    bool success = try_read(core::Span<char_type>(buffer).subspan(offset), n, ec);
    ARCHON_ASSERT(n <= std::size_t(buffer.size() - offset));
    offset += n;
    if (ARCHON_LIKELY(success)) { // Throws
        if (ARCHON_LIKELY(n > 0))
            goto again;
        return true; // Success
    }
    return false; // Failure
}


template<class I>
inline bool GenericTextFile<I>::try_unshift(std::error_code& ec)
{
    if (ARCHON_LIKELY(!m_reading)) {
      proceed:
        m_writing = true;
        return m_impl.unshift(ec); // Throws
    }
    if (ARCHON_LIKELY(stop_reading(ec))) // Throws
        goto proceed;
    return false; // Failure to stop reading
}


template<class I>
inline bool GenericTextFile<I>::try_flush(std::error_code& ec)
{
    if (ARCHON_LIKELY(m_writing))
        return stop_writing(ec); // Throws
    return true; // Trivial success
}


template<class I>
inline bool GenericTextFile<I>::try_tell(pos_type& pos, std::error_code& ec)
{
    if (ARCHON_LIKELY(!m_reading))
        return m_impl.tell_write(pos, ec); // Throws
    m_impl.advance(); // Throws
    return m_impl.tell_read(pos, ec); // Throws
}


template<class I>
inline bool GenericTextFile<I>::try_seek(pos_type pos, std::error_code& ec)
{
    if (ARCHON_LIKELY(m_writing)) {
        if (ARCHON_LIKELY(stop_writing(ec))) // Throws
            goto proceed;
        return false; // Failure to stop writing
    }
  proceed:
    if (ARCHON_LIKELY(m_impl.seek(pos, ec))) { // Throws
        m_reading = false;
        return true; // Success
    }
    return false; // Failure
}


template<class I>
inline auto GenericTextFile<I>::load(core::FilesystemPathRef path, const std::locale& locale) -> string_type
{
    std::error_code ec;
    core::Buffer<char_type> buffer;
    std::size_t offset = 0;
    if (ARCHON_LIKELY(try_load(path, buffer, offset, ec, locale))) // Throws
        return string_type(buffer.data(), offset); // Throws
    throw std::filesystem::filesystem_error("Failed to load file", path, ec);
}


template<class I>
inline void GenericTextFile<I>::save(core::FilesystemPathRef path, core::StringSpan<char_type> data,
                                     const std::locale& locale)
{
    std::error_code ec;
    if (ARCHON_LIKELY(try_save(path, data, ec, locale)))
        return; // Success
    throw std::filesystem::filesystem_error("Failed to save data to file", path, ec);
}


template<class I>
inline auto GenericTextFile<I>::load_and_chomp(core::FilesystemPathRef path, const std::locale& locale) -> string_type
{
    std::error_code ec;
    core::Buffer<char_type> buffer;
    std::size_t offset = 0;
    if (ARCHON_LIKELY(try_load_and_chomp(path, buffer, offset, ec, locale))) // Throws
        return string_type(buffer.data(), offset); // Throws
    throw std::filesystem::filesystem_error("Failed to load file", path, ec);
}


template<class I>
bool GenericTextFile<I>::try_load(core::FilesystemPathRef path, core::Buffer<char_type>& buffer, std::size_t& offset,
                                  std::error_code& ec, const std::locale& locale)
{
    GenericTextFile file(locale); // Throws
    if (ARCHON_LIKELY(file.try_open(path, Mode::read, ec))) { // Throws
        if (ARCHON_LIKELY(file.try_read_all(buffer, offset, ec)))
            return true; // Success
    }
    return false; // Failure
}


template<class I>
bool GenericTextFile<I>::try_save(core::FilesystemPathRef path, core::StringSpan<char_type> data, std::error_code& ec,
                                  const std::locale& locale)
{
    GenericTextFile file(locale); // Throws
    if (ARCHON_LIKELY(file.try_open(path, Mode::write, ec))) {
        std::size_t n; // Dummy
        if (ARCHON_LIKELY(file.try_write(data, n, ec))) {
            if (ARCHON_LIKELY(file.try_flush(ec)))
                return true; // Success
        }
    }
    return false; // Failure
}


template<class I>
bool GenericTextFile<I>::try_load_and_chomp(core::FilesystemPathRef path, core::Buffer<char_type>& buffer,
                                            std::size_t& offset, std::error_code& ec, const std::locale& locale)
{
    std::size_t offset_2 = offset;
    if (ARCHON_LIKELY(try_load(path, buffer, offset_2, ec, locale))) { // Throws
        core::BasicCharMapper<char_type> mapper(locale); // Throws
        char_type newline = mapper.widen('\n'); // Throws
        if (ARCHON_LIKELY(offset_2 > offset && buffer[offset_2 - 1] == newline))
            --offset_2;
        offset = offset_2;
        return true; // Success
    }
    return false; // Failure
}


template<class I>
bool GenericTextFile<I>::stop_reading(std::error_code& ec)
{
    ARCHON_ASSERT(m_reading);
    ARCHON_ASSERT(!m_writing);
    m_impl.advance(); // Throws
    if (ARCHON_LIKELY(m_impl.discard(ec))) { // Throws
        m_reading = false;
        return true;
    }
    return false;
}


template<class I>
bool GenericTextFile<I>::stop_writing(std::error_code& ec)
{
    ARCHON_ASSERT(!m_reading);
    ARCHON_ASSERT(m_writing);
    if (ARCHON_LIKELY(!impl_type::has_degen_unshift && !m_disable_autounshift)) {
        if (ARCHON_LIKELY(m_impl.unshift(ec))) // Throws
            goto proceed;
        goto fallback;
    }

  proceed:
    if (ARCHON_LIKELY(m_impl.flush(ec))) { // Throws
        m_writing = false;
        return true; // Success
    }
    return false; // Failure

  fallback:
    // Even when everything in the local buffer could not be written to the implementation
    // layer (`m_impl`), an attempt to recursively flush the part, that could be writen,
    // must still be made.
    std::error_code ec_2; // Dummy
    static_cast<void>(m_impl.flush(ec_2)); // Throws
    return false; // Failure
}


template<class I>
inline bool GenericTextFile<I>::do_read_some(core::Span<char_type> buffer, std::size_t& n, std::error_code& ec)
{
    ARCHON_ASSERT(!m_writing);
    m_impl.advance(); // Throws
    return m_impl.read_ahead(buffer, m_dynamic_eof, n, ec); // Throws
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_TEXT_FILE_HPP
