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

#ifndef ARCHON_X_CORE_X_SOURCE_HPP
#define ARCHON_X_CORE_X_SOURCE_HPP

/// \file


#include <cstddef>
#include <system_error>

#include <archon/core/features.h>
#include <archon/core/span.hpp>


namespace archon::core {


/// \brief Reading-endpoint of abstract byte stream.
///
/// A byte source is an endpoint of an abstract stream from which one can read bytes.
///
class Source {
public:
    /// \brief Read a chunk of bytes.
    ///
    /// This function is a shorthand for calling \ref try_read() and, on success, returning
    /// the number of bytes read, or, on failure, throwing an `std::system_error`.
    ///
    auto read(core::Span<char> buffer) -> std::size_t;

    /// \brief Read at least one byte.
    ///
    /// This function is a shorthand for calling \ref try_read_some() and, on success,
    /// returning the number of bytes read, or, on failure, throwing an `std::system_error`.
    ///
    auto read_some(core::Span<char> buffer) -> std::size_t;

    /// \brief Try to read a chunk of bytes.
    ///
    /// This function reads successive bytes from the stream, and places them in the
    /// specified buffer. Reading continues until the buffer is full, or the end of input is
    /// reached.
    ///
    /// If the size of specified buffer is zero, this function may, or may not block the
    /// calling thread until at least one byte could have been read.
    ///
    /// On success or failure, this function sets \p n to the number of bytes placed in the
    /// specified buffer. Buffer contents beyond \p n may have been clobbered.
    ///
    /// On success, this function returns `true` and leaves \p ec unchanged. If \p n is set
    /// to a value less than the size of the specified buffer, it means that the end of
    /// input has been reached.
    ///
    /// On failure, this function returns `false` after setting \p ec to an error code that
    /// reflects the cause of the failure.
    ///
    /// If an exception is thrown, \p n and \p ec are left unchanged. The contents of the
    /// buffer may have been clobbered.
    ///
    [[nodiscard]] bool try_read(core::Span<char> buffer, std::size_t& n, std::error_code& ec);

    /// \brief Try to read at least one byte.
    ///
    /// This function reads successive bytes from the stream, and places them in the
    /// specified buffer. At least one byte will be read provided that the end of input has
    /// not already been reached, and provided that the size of the specified buffer is not
    /// zero. If at least one byte can be read without blocking the calling thread, the
    /// calling thread will not be blocked. Otherwise, the calling thread is blocked until
    /// at least one byte can be read.
    ///
    /// If the size of specified buffer is zero, this function may, or may not block the
    /// calling thread until at least one byte could have been read.
    ///
    /// On success, this function sets \p n to the number of bytes placed in the specified
    /// buffer, and then returns `true`. \p ec is left unchanged in this case. Contents of
    /// the buffer beyond \p n may have been clobbered. If \p n is set to zero and the size
    /// of the specified buffer is greater than zero, it means that the end of input has
    /// been reached.
    ///
    /// On failure, this function sets \p ec to an error code that reflects the cause of the
    /// failure, and then returns `false`. \p n is left unchanged in this case. The contents
    /// of the buffer may have been clobbered.
    ///
    /// If an exception is thrown, \p n and \p ec are left unchanged. The contents of the
    /// buffer may have been clobbered.
    ///
    [[nodiscard]] bool try_read_some(core::Span<char> buffer, std::size_t& n, std::error_code& ec);

    virtual ~Source() noexcept = default;

protected:
    /// \brief Read from stream.
    ///
    /// The behavior of this function is exactly as described for \ref try_read_some(). This
    /// functino is called by the public functions of the source class.
    ///
    virtual bool do_try_read_some(core::Span<char> buffer, std::size_t& n, std::error_code& ec) = 0;
};








// Implementation


inline auto Source::read(core::Span<char> buffer) -> std::size_t
{
    std::size_t n = 0;
    std::error_code ec;
    if (ARCHON_LIKELY(try_read(buffer, n, ec))) // Throws
        return n; // Success
    throw std::system_error(ec, "Failed to read from source");
}


inline auto Source::read_some(core::Span<char> buffer) -> std::size_t
{
    std::size_t n = 0;
    std::error_code ec;
    if (ARCHON_LIKELY(try_read_some(buffer, n, ec))) // Throws
        return n; // Success
    throw std::system_error(ec, "Failed to read from source");
}


inline bool Source::try_read_some(core::Span<char> buffer, std::size_t& n, std::error_code& ec)
{
    return do_try_read_some(buffer, n, ec); // Throws
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_SOURCE_HPP
