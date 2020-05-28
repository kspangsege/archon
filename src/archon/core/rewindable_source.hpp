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

#ifndef ARCHON_X_CORE_X_REWINDABLE_SOURCE_HPP
#define ARCHON_X_CORE_X_REWINDABLE_SOURCE_HPP

/// \file


#include <cstddef>
#include <system_error>

#include <archon/core/span.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/source.hpp>


namespace archon::core {


/// \brief     
///
///     
///
class RewindableSource
    : public core::Source {
public:
    RewindableSource(core::Source& subsource, core::Buffer<char>&) noexcept;

    /// \brief     
    ///
    ///     
    ///
    void rewind() noexcept;

    /// \brief     
    ///
    ///     
    ///
    void release() noexcept;

protected:
    // Overriding function in core::Source
    bool do_try_read_some(core::Span<char>, std::size_t&, std::error_code&) override final;

private:
    core::Source& m_subsource;
    core::Buffer<char>& m_buffer;
    std::size_t m_offset = 0, m_size = 0;
    bool m_released = false;
};








// Implementation


inline RewindableSource::RewindableSource(core::Source& subsource, core::Buffer<char>& buffer) noexcept
    : m_subsource(subsource)
    , m_buffer(buffer)
{
}


inline void RewindableSource::rewind() noexcept
{
    m_offset = 0;
}


inline void RewindableSource::release() noexcept
{
    m_released = true;
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_REWINDABLE_SOURCE_HPP
