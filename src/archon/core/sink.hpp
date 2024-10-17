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

#ifndef ARCHON_X_CORE_X_SINK_HPP
#define ARCHON_X_CORE_X_SINK_HPP

/// \file


#include <cstddef>
#include <system_error>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/string_span.hpp>


namespace archon::core {


/// \brief Writing-endpoint of abstract stream.
///
/// A sink is an endpoint of an abstract stream to which one can write bytes.
///
class Sink {
public:
    /// \brief Write a chunk of bytes.
    ///
    /// This function is a shorthand for calling \ref try_write() and throwing an
    /// `std::system_error` on failure.
    ///
    void write(core::StringSpan<char> data);

    /// \brief Try to write a chunk of bytes.
    ///
    /// This function writes the specified chunk of bytes to the stream.
    ///
    /// If the size of the specified chunk is zero, this function may, or may not block the
    /// calling thread until at least one byte could have been written.
    ///
    /// On success or failure, this function sets \p n to the number of written bytes.
    ///
    /// On success, this function returns `true` and leaves \p ec unchanged. \p n is
    /// guaranteed to be equal to `data.size()`.
    ///
    /// On failure, this function returns `false` after setting \p ec to an error code that
    /// reflects the cause of the failure.
    ///
    /// If an exception is thrown, \p n and \p ec are left unchanged. Some data may, or may
    /// not have been written.
    ///
    [[nodiscard]] bool try_write(core::StringSpan<char> data, std::size_t& n, std::error_code& ec);

    virtual ~Sink() noexcept = default;

protected:
    /// \brief Write to stream.
    ///
    /// The behavior of this function is exactly as described for \ref try_write(). This
    /// function is called by the public functions of the sink class.
    ///
    virtual bool do_try_write(core::Span<const char> data, std::size_t& n, std::error_code& ec) = 0;
};








// Implementation


inline void Sink::write(core::StringSpan<char> data)
{
    std::size_t n; // Dummy
    std::error_code ec;
    if (ARCHON_LIKELY(try_write(data, n, ec))) // Throws
        return; // Success
    throw std::system_error(ec, "Failed to write to sink");
}


inline bool Sink::try_write(core::StringSpan<char> data, std::size_t& n, std::error_code& ec)
{
    return do_try_write(data, n, ec); // Throws
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_SINK_HPP
