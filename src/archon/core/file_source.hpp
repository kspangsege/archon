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

#ifndef ARCHON_X_CORE_X_FILE_SOURCE_HPP
#define ARCHON_X_CORE_X_FILE_SOURCE_HPP

/// \file


#include <cstddef>
#include <system_error>

#include <archon/core/span.hpp>
#include <archon/core/source.hpp>
#include <archon/core/file.hpp>


namespace archon::core {


/// \brief Source connected to file.
///
/// This is a source implementation that is connected to a file (\ref core::File). Reading
/// from the source is reading from the file.
///
class FileSource
    : public core::Source {
public:
    FileSource(core::File&) noexcept;

protected:
    // Overriding function in core::Source
    bool do_try_read_some(core::Span<char>, std::size_t&, std::error_code&) override final;

private:
    core::File& m_file;
};



/// \brief Source connected to file and equipped with buffer.
///
/// This is a buffered source implementation that is connected to a file (\ref
/// core::File). It is like to \ref core::FileSource, but reading is buffered.
///
class BufferedFileSource
    : public core::Source {
public:
    /// \brief Construct file source with buffer.
    ///
    /// Construct a file source that uses the specified buffer. The buffer must not be
    /// empty. Passing an empty buffer causes undefined behavior.
    ///
    BufferedFileSource(core::File&, core::Span<char> buffer) noexcept;

protected:
    // Overriding function in core::Source
    bool do_try_read_some(core::Span<char>, std::size_t&, std::error_code&) override final;

private:
    core::File& m_file;
    core::Span<char> m_buffer;
    std::size_t m_begin = 0;
    std::size_t m_end = 0;
};








// Implementation


inline FileSource::FileSource(core::File& file) noexcept
    : m_file(file)
{
}


inline BufferedFileSource::BufferedFileSource(core::File& file, core::Span<char> buffer) noexcept
    : m_file(file)
    , m_buffer(buffer)
{
    ARCHON_ASSERT(m_buffer.size() > 0);
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_FILE_SOURCE_HPP
