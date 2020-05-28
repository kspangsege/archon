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

#ifndef ARCHON_X_CORE_X_BUFFERED_TEXT_FILE_HPP
#define ARCHON_X_CORE_X_BUFFERED_TEXT_FILE_HPP

/// \file


#include <cstddef>
#include <utility>
#include <string_view>
#include <system_error>
#include <locale>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/string_span.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/char_mapper.hpp>
#include <archon/core/char_codec.hpp>
#include <archon/core/filesystem.hpp>
#include <archon/core/file.hpp>
#include <archon/core/text_file_impl.hpp>
#include <archon/core/text_file.hpp>


namespace archon::core {


/// \brief Text file accessor with buffering.
///
/// In addition to what is offered by \ref core::GenericTextFile, this class guarantees
/// efficient operation when reading or writing many small chunks. Efficiency is ensured by
/// adding a buffering layer.
///
/// \tparam I The implementation type to be used. This must be a type that satisfies \ref
/// Concept_Archon_Core_BufferedTextFileImpl.
///
template<class I> class GenericBufferedTextFile
    : public core::GenericTextFile<I>
    , private core::BasicCharMapper<typename I::char_type> {
public:
    using impl_type = I;

    using char_type   = typename impl_type::char_type;
    using traits_type = typename impl_type::traits_type;
    using codec_type  = typename impl_type::codec_type;
    using pos_type    = typename impl_type::pos_type;
    using state_type  = typename impl_type::state_type;

    using string_view_type = std::basic_string_view<char_type, traits_type>;

    using Mode   = core::File::Mode;

    using Config = typename core::GenericTextFile<I>::Config;

    explicit GenericBufferedTextFile(core::FilesystemPathRef, const std::locale& = {});
    explicit GenericBufferedTextFile(core::FilesystemPathRef, Mode, const std::locale& = {});
    explicit GenericBufferedTextFile(const std::locale& = {}, Config = {});

    /// \brief Read next line.
    ///
    /// This function is like \ref try_read_line() except that on success, it returns the
    /// value of \p found, and on failure, it throws an instance of `std::system_error`
    /// carrying the generated error code.
    ///
    bool read_line(core::Buffer<char_type>&, string_view_type& line);

    /// \brief Write a line.
    ///
    /// This function is like \ref try_write_line() except that on failure, it throws an
    /// instance of `std::system_error` carrying the generated error code.
    ///
    void write_line(core::StringSpan<char_type> line);

    /// \brief Try to read next line.
    ///
    /// This function attempts to read the next line from the file. Characters are extracted
    /// starting from the current logical file pointer. Extraction stops when a newline
    /// characters has been extracted, or when the end of file is reached, whichever comes
    /// first. Upon return, the logical file pointer will have been advanced to point to the
    /// position that follows the last extracted character.
    ///
    /// The returned string view (\p line) does not contain the terminating newline
    /// character, if one was present. The file pointer is still advanced across the newline
    /// character, though.
    ///
    /// As is customary on POSIX platforms, if the last character in the file is a newline
    /// character, this function considers the preceding line as the last line in the
    /// file. On the other hand, if the last character is not a newline character, the last
    /// line in the file is the one that succeeds the last newline character. This means
    /// that the last line in the file either is nonempty or is terminated by a newline
    /// character. If the file contains N newline characters, it also means that the number
    /// of lines in the file is N if the last character is a newline characters, otherwise
    /// it is N + 1.
    ///
    /// On success, this function returns `true` after setting \p found to `true` if another
    /// line was present in the file, and to `false` otherwise. When \p found is set to
    /// `true`, \p line is set to refer to the extracted line, excluding the terminating
    /// newline character. The string view will refer to memory owned by the specified
    /// buffer (\p buffer).
    ///
    /// On failure, this function returns `false` after setting `ec` to an error code that
    /// reflects the cause of the failure.
    ///
    /// On success, \p ec is left "untouched", and on failure, \p found is left
    /// "untouched". \p line is left "untouched" except when \p found is set to `true`.
    ///
    [[nodiscard]] bool try_read_line(core::Buffer<char_type>& buffer, string_view_type& line, bool& found,
                                     std::error_code& ec);

    /// \brief Try to write a line.
    ///
    /// This function attempts to write the specified string (\p line) to the file, and to
    /// then write a terminating newline character. Writing starts at the current position
    /// of the logical file pointer. Upon return, the logical file pointer will have been
    /// advanced to the position following the terminating newline character.
    ///
    /// On success, this function returns `true`.
    ///
    /// On failure, this function returns `false` after setting `ec` to an error code that
    /// reflects the cause of the failure.
    ///
    [[nodiscard]] bool try_write_line(core::StringSpan<char_type> line, std::error_code&);

private:
    const char_type m_newline;
};


template<class C, class T = std::char_traits<C>, class D = core::BasicCharCodec<C, T>>
using BasicBufferedTextFile = GenericBufferedTextFile<core::BufferedTextFileImpl<C, T, D>>;

template<class C, class T = std::char_traits<C>, class D = core::BasicCharCodec<C, T>>
using BasicBufferedPosixTextFile = GenericBufferedTextFile<core::BufferedPosixTextFileImpl<C, T, D>>;

template<class C, class T = std::char_traits<C>, class D = core::BasicCharCodec<C, T>>
using BasicBufferedWindowsTextFile = GenericBufferedTextFile<core::BufferedWindowsTextFileImpl<C, T, D>>;


using BufferedTextFile        = BasicBufferedTextFile<char>;
using BufferedPosixTextFile   = BasicBufferedPosixTextFile<char>;
using BufferedWindowsTextFile = BasicBufferedWindowsTextFile<char>;

using WideBufferedTextFile        = BasicBufferedTextFile<wchar_t>;
using WideBufferedPosixTextFile   = BasicBufferedPosixTextFile<wchar_t>;
using WideBufferedWindowsTextFile = BasicBufferedWindowsTextFile<wchar_t>;








// Implementation


template<class I>
inline GenericBufferedTextFile<I>::GenericBufferedTextFile(core::FilesystemPathRef path, const std::locale& locale)
    : GenericBufferedTextFile(path, Mode::read, locale) // Throws
{
}


template<class I>
inline GenericBufferedTextFile<I>::GenericBufferedTextFile(core::FilesystemPathRef path, Mode mode,
                                                           const std::locale& locale)
    : GenericBufferedTextFile(locale) // Throws
{
    this->open(path, mode); // Throws
}


template<class I>
inline GenericBufferedTextFile<I>::GenericBufferedTextFile(const std::locale& locale, Config config)
    : core::GenericTextFile<I>(locale, std::move(config)) // Throws
    , core::BasicCharMapper<char_type>(locale) // Throws
    , m_newline(this->widen('\n')) // Throws
{
}


template<class I>
inline bool GenericBufferedTextFile<I>::read_line(core::Buffer<char_type>& buffer, string_view_type& line)
{
    bool found = false;
    std::error_code ec;
    if (ARCHON_LIKELY(try_read_line(buffer, line, found, ec))) // Throws
        return found; // Success
    throw std::system_error(ec, "Failed to read line");
}


template<class I>
inline void GenericBufferedTextFile<I>::write_line(core::StringSpan<char_type> line)
{
    std::error_code ec;
    if (ARCHON_LIKELY(try_write_line(line, ec))) // Throws
        return; // Success
    throw std::system_error(ec, "Failed to write line");
}


template<class I>
inline bool GenericBufferedTextFile<I>::try_read_line(core::Buffer<char_type>& buffer, string_view_type& line,
                                                      bool& found, std::error_code& ec)
{
    if (ARCHON_LIKELY(!this->m_writing)) {
      proceed:
        this->m_reading = true;
        std::size_t offset = 0;
        bool found_delim = false;
        if (ARCHON_LIKELY(this->m_impl.read_until(m_newline, buffer, this->m_dynamic_eof, offset, found_delim,
                                                  ec))) { // Throws
            bool found_2 = offset > 0;
            if (ARCHON_LIKELY(found_delim)) {
                ARCHON_ASSERT(offset > 0);
                --offset;
            }
            line = { buffer.data(), offset };
            found = found_2;
            return true; // Success
        }
        return false; // Failure to read
    }
    if (ARCHON_LIKELY(this->stop_writing(ec))) // Throws
        goto proceed;
    return false; // Failure to stop writing
}


template<class I>
inline bool GenericBufferedTextFile<I>::try_write_line(core::StringSpan<char_type> line, std::error_code& ec)
{
    std::size_t n = 0; // Dummy
    if (ARCHON_LIKELY(this->try_write(line, n, ec))) // Throws
        return this->try_write(string_view_type(&m_newline, 1), n, ec); // Throws
    return false; // Failure
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_BUFFERED_TEXT_FILE_HPP
