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

#ifndef ARCHON_X_CORE_X_BUFFER_CONTENTS_HPP
#define ARCHON_X_CORE_X_BUFFER_CONTENTS_HPP

/// \file


#include <cstddef>
#include <utility>
#include <algorithm>

#include <archon/core/span.hpp>
#include <archon/core/buffer.hpp>


namespace archon::core {


/// \brief Buffer contents tracker.
///
/// An object of this type is used to track the size of the contents in a particular buffer
/// (\ref core::Buffer). It offers methods for appending to the contents and expanding the
/// underlying buffer as necessary.
///
/// \sa \ref core::Buffer
/// \sa \ref core::BasicStringBufferContents
///
template<class T> class BufferContents {
public:
    using value_type = T;

    using iterator       = T*;
    using const_iterator = const T*;

    using buffer_type = core::Buffer<T>;

    explicit BufferContents(buffer_type& buffer) noexcept;

    auto operator[](std::size_t) noexcept       -> T&;
    auto operator[](std::size_t) const noexcept -> const T&;

    bool empty() const noexcept;
    auto size() const noexcept -> std::size_t;

    auto data() noexcept       -> T*;
    auto data() const noexcept -> const T*;

    auto begin() noexcept -> iterator;
    auto end() noexcept   -> iterator;
    auto begin() const noexcept -> const_iterator;
    auto end() const noexcept   -> const_iterator;

    void push_back(const T&);
    void push_back(T&&);

    void append(core::Span<const T>);
    void append(std::size_t n, const T& value);

    void clear() noexcept;

private:
    buffer_type& m_buffer;
    std::size_t m_size = 0;
};








// Implementation


template<class T>
inline BufferContents<T>::BufferContents(buffer_type& buffer) noexcept
    : m_buffer(buffer)
{
}


template<class T>
inline auto BufferContents<T>::operator[](std::size_t i) noexcept -> T&
{
    return data()[i];
}


template<class T>
inline auto BufferContents<T>::operator[](std::size_t i) const noexcept -> const T&
{
    return data()[i];
}


template<class T>
inline bool BufferContents<T>::empty() const noexcept
{
    return m_size == 0;
}


template<class T>
inline auto BufferContents<T>::size() const noexcept -> std::size_t
{
    return m_size;
}


template<class T>
inline auto BufferContents<T>::data() noexcept -> T*
{
    return m_buffer.data();
}


template<class T>
inline auto BufferContents<T>::data() const noexcept -> const T*
{
    return m_buffer.data();
}


template<class T>
inline auto BufferContents<T>::begin() noexcept -> iterator
{
    return data();
}


template<class T>
inline auto BufferContents<T>::end() noexcept -> iterator
{
    return data() + size();
}


template<class T>
inline auto BufferContents<T>::begin() const noexcept -> const_iterator
{
    return data();
}


template<class T>
inline auto BufferContents<T>::end() const noexcept -> const_iterator
{
    return data() + size();
}


template<class T>
inline void BufferContents<T>::push_back(const T& value)
{
    append(1, value); // Throws
}


template<class T>
inline void BufferContents<T>::push_back(T&& value)
{
    m_buffer.reserve_extra(1, m_size); // Throws
    m_buffer[m_size] = std::move(value); // Throws
    ++m_size;
}


template<class T>
inline void BufferContents<T>::append(core::Span<const T> elems)
{
    std::size_t size = elems.size();
    m_buffer.reserve_extra(size, m_size); // Throws
    std::copy_n(elems.data(), size, m_buffer.data() + m_size);
    m_size += size;
}


template<class T>
inline void BufferContents<T>::append(std::size_t n, const T& value)
{
    m_buffer.reserve_extra(n, m_size); // Throws
    T* base = m_buffer.data() + m_size;
    std::fill(base, base + n, value); // Throws
    m_size += n;
}


template<class T>
inline void BufferContents<T>::clear() noexcept
{
    m_size = 0;
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_BUFFER_CONTENTS_HPP
