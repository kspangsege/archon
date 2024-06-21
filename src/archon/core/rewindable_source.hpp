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
#include <archon/core/assert.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/source.hpp>


namespace archon::core {


/// \brief Byte source with rewinding capability.
///
/// This class adds rewinding capability to a byte source (the reading endpoint of a byte
/// stream). It is itself a byte source (\ref core::Source), and it wraps the sub-source
/// which is the underlying source where the bytes will be read from by the rewindable
/// source.
///
/// The rewinding capability is made possible through buffering. That is, all the bytes that
/// are read through the rewindable source are also stored in a buffer inside the rewindable
/// source. This is necessary because bytes can only be read from the underlying source
/// once.
///
/// When the source is rewound (\ref rewind()), the subsequent reading process will start by
/// going through the buffered bytes, and then resume reading from the underlying source
/// when all the buffered bytes have been read. The bytes that are read from the underlying
/// stream, after all the buffered bytes have been read, will also be stored so they are
/// available to be reread if the source is rewound again.
///
/// A rewindable source can be rewound any number of times.
///
/// If the application reaches a point where it knows that there will be no more need for
/// rewinding, it can call \ref release(). After \ref release() has been called, bytes read
/// from the underlying source will no longer be stored in the rewindable source. Behavior
/// is undefined if \ref rewind() is called after an invocation of \ref release().
///
class RewindableSource
    : public core::Source {
public:
    RewindableSource(core::Source& subsource, core::Buffer<char>&) noexcept;

    /// \brief Rewind source to beginning of input.
    ///
    /// This function rewinds the source, so that reading starts from the beginning
    /// again. This can be done any number of times so long as \ref release() has not been
    /// called.
    ///
    /// \sa \ref release()
    ///
    void rewind() noexcept;

    /// \brief Relinquish further rewinding capability.
    ///
    /// This function puts the source into the released state. In this state bytes read from
    /// the underlying source will not be stored in the source, and can therefore not be
    /// reread. Behavior is undefined if \ref rewind() is called after `release()` has been
    /// called.
    ///
    /// This function has no effect if the source is already in the released state
    /// (idempotency).
    ///
    /// \sa \ref rewind()
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
    ARCHON_ASSERT(!m_released);
    m_offset = 0;
}


inline void RewindableSource::release() noexcept
{
    m_released = true;
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_REWINDABLE_SOURCE_HPP
