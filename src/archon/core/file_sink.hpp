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

#ifndef ARCHON_X_CORE_X_FILE_SINK_HPP
#define ARCHON_X_CORE_X_FILE_SINK_HPP

/// \file


#include <cstddef>
#include <system_error>

#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/sink.hpp>
#include <archon/core/file.hpp>


namespace archon::core {


/// \brief Sink connected to file.
///
/// This is a sink implementation that is connected to a file (\ref core::File). Writing to
/// the sink is writing to the file.
///
class FileSink
    : public core::Sink {
public:
    FileSink(core::File&) noexcept;

protected:
    // Overriding function in core::Source
    bool do_try_write(core::Span<const char>, std::size_t&, std::error_code&) override final;

private:
    core::File& m_file;
};



/// \brief Sink connected to file and equipped with buffer.
///
/// This is a buffered sink implementation that is connected to a file (\ref core::File). It
/// is like to \ref core::FileSink, but writing is buffered.
///
class BufferedFileSink
    : public core::Sink {
public:
    /// \brief Construct file sink with buffer.
    ///
    /// Construct a file sink that uses the specified buffer. The buffer must not be
    /// empty. Passing an empty buffer causes undefined behavior.
    ///
    BufferedFileSink(core::File&, core::Span<char> buffer) noexcept;

    /// \brief Flush buffered bytes to file.
    ///
    /// This function is a shorthand for calling \ref try_flush() and throwing an
    /// `std::system_error` on failure.
    ///
    void flush();

    /// \brief Try to flush buffered data to file.
    ///
    /// This function flushes any buffered data to the underlying file.
    ///
    /// On success, this function returns `true` and leaves \p ec unchanged.
    ///
    /// On failure, this function returns `false` after setting \p ec to an error code that
    /// reflects the cause of the failure.
    ///
    /// If an exception is thrown, \p ec is left unchanged. Some data may, or may not have
    /// been flushed.
    ///
    [[nodiscard]] bool try_flush(std::error_code& ec);

protected:
    // Overriding function in core::Sink
    bool do_try_write(core::Span<const char>, std::size_t&, std::error_code&) override final;

private:
    core::File& m_file;
    core::Span<char> m_buffer;
    std::size_t m_begin = 0;
    std::size_t m_end = 0;
};








// Implementation


inline FileSink::FileSink(core::File& file) noexcept
    : m_file(file)
{
}


inline BufferedFileSink::BufferedFileSink(core::File& file, core::Span<char> buffer) noexcept
    : m_file(file)
    , m_buffer(buffer)
{
    ARCHON_ASSERT(m_buffer.size() > 0);
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_FILE_SINK_HPP
