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

#ifndef ARCHON_X_CORE_X_STRING_BUFFER_CONTENTS_HPP
#define ARCHON_X_CORE_X_STRING_BUFFER_CONTENTS_HPP

/// \file


#include <cstddef>
#include <algorithm>

#include <archon/core/string_span.hpp>
#include <archon/core/buffer.hpp>


namespace archon::core {


/// \brief String buffer contents tracker.
///
/// An object of this type is used to track the size of the contents in a particular string
/// buffer (\ref core::Buffer). It offers methods for appending to the contents and
/// expanding the underlying buffer as necessary.
///
/// \sa \ref core::Buffer.
/// \sa \ref core::BufferContents.
///
template<class C> class StringBufferContents {
public:
    using char_type = C;

    using iterator       = C*;
    using const_iterator = const C*;

    using buffer_type = core::Buffer<C>;

    /// \brief Construct string buffer contents tracker.
    ///
    /// \param size The initial size of the buffer contents. Behavior is undefined if the
    /// specified value is greater than the current size of the specified buffer.
    ///
    explicit StringBufferContents(buffer_type& buffer, std::size_t size = 0) noexcept;

    auto operator[](std::size_t) noexcept -> C&;
    auto operator[](std::size_t) const noexcept -> const C&;

    bool empty() const noexcept;
    auto size() const noexcept -> std::size_t;

    auto data() noexcept -> C*;
    auto data() const noexcept -> const C*;

    auto begin() noexcept -> C*;
    auto end() noexcept -> C*;
    auto begin() const noexcept -> const C*;
    auto end() const noexcept -> const C*;

    void append(core::StringSpan<C>);
    void append(std::size_t n, C ch);

    void clear() noexcept;

private:
    buffer_type& m_buffer;
    std::size_t m_size;
};








// Implementation


template<class C>
inline StringBufferContents<C>::StringBufferContents(buffer_type& buffer, std::size_t size) noexcept
    : m_buffer(buffer)
    , m_size(size)
{
}


template<class C>
inline auto StringBufferContents<C>::operator[](std::size_t i) noexcept -> C&
{
    return data()[i];
}


template<class C>
inline auto StringBufferContents<C>::operator[](std::size_t i) const noexcept -> const C&
{
    return data()[i];
}


template<class C>
inline bool StringBufferContents<C>::empty() const noexcept
{
    return m_size == 0;
}


template<class C>
inline auto StringBufferContents<C>::size() const noexcept -> std::size_t
{
    return m_size;
}


template<class C>
inline auto StringBufferContents<C>::data() noexcept -> C*
{
    return m_buffer.data();
}


template<class C>
inline auto StringBufferContents<C>::data() const noexcept -> const C*
{
    return m_buffer.data();
}


template<class C>
inline auto StringBufferContents<C>::begin() noexcept -> C*
{
    return data();
}


template<class C>
inline auto StringBufferContents<C>::end() noexcept -> C*
{
    return data() + size();
}


template<class C>
inline auto StringBufferContents<C>::begin() const noexcept -> const C*
{
    return data();
}


template<class C>
inline auto StringBufferContents<C>::end() const noexcept -> const C*
{
    return data() + size();
}


template<class C>
inline void StringBufferContents<C>::append(core::StringSpan<C> string)
{
    std::size_t size = string.size();
    m_buffer.reserve_extra(size, m_size); // Throws
    std::copy_n(string.data(), size, m_buffer.data() + m_size); // Throws
    m_size += size;
}


template<class C>
inline void StringBufferContents<C>::append(std::size_t n, C ch)
{
    m_buffer.reserve_extra(n, m_size); // Throws
    C* base = m_buffer.data() + m_size;
    std::fill(base, base + n, ch); // Throws
    m_size += n;
}


template<class C>
inline void StringBufferContents<C>::clear() noexcept
{
    m_size = 0;
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_STRING_BUFFER_CONTENTS_HPP
