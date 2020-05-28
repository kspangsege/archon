// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


/// \file

#ifndef ARCHON__BASE__APPEND_BUFFER_HPP
#define ARCHON__BASE__APPEND_BUFFER_HPP

#include <cstddef>
#include <algorithm>
#include <utility>

#include <archon/base/buffer.hpp>


namespace archon::base {


/// \brief A buffer whose contents can be efficiently grown.
///
/// This class offers a type of buffer where the size of the contents can be
/// efficiently change. It acheives this by using an underlying buffer that can
/// be larger than the logical size, and the unerlying buffer is automatically
/// expanded in progressively larger steps as required.
///
template<class T> class AppendBuffer {
public:
    using value_type = T;

    AppendBuffer() noexcept = default;
    ~AppendBuffer() noexcept = default;

    AppendBuffer(AppendBuffer&&) noexcept = default;
    AppendBuffer& operator=(AppendBuffer&&) noexcept = default;

    T* data() noexcept;
    const T* data() const noexcept;

    /// Returns the current size of the buffer.
    ///
    std::size_t size() const noexcept;

    /// Append the specified elements. This increases the size of this buffer by
    /// \p size.
    ///
    /// If the caller has previously requested a minimum capacity that is
    /// greater than, or equal to the resulting size, this function is
    /// guaranteed to not throw.
    ///
    void append(const T* data, std::size_t size);

    /// Append \p n copies of \p value to the specified buffer.
    ///
    /// If the caller has previously requested a minimum capacity that is
    /// greater than, or equal to the resulting size, this function is
    /// guaranteed to not throw.
    ///
    void append(std::size_t n, T value);

    /// If the specified size is less than the current size, then the buffer
    /// contents is truncated accordingly. If the specified size is greater than
    /// the current size, then the extra elements will have undefined values. If
    /// the caller has previously requested a minimum capacity that is greater
    /// than, or equal to the specified size, this function is guaranteed to not
    /// throw.
    ///
    void resize(std::size_t new_size);

    /// This operation does not change the size of the buffer as returned by
    /// size(). If the specified capacity is less than the current capacity,
    /// this operation has no effect.
    ///
    void reserve(std::size_t min_capacity);

    /// Set the size to zero. The capacity remains unchanged.
    ///
    void clear() noexcept;

    /// Release the underlying buffer and reset the size. Note: The returned
    /// buffer may be larger than the amount of data appended to this buffer.
    /// Callers should call `size()` prior to releasing the buffer to know the
    /// usable/logical size.
    ///
    Buffer<T> release() noexcept;

private:
    Buffer<T> m_buffer;
    std::size_t m_size = 0;
};








// Implementation


template<class T> inline T* AppendBuffer<T>::data() noexcept
{
    return m_buffer.data();
}


template<class T> inline const T* AppendBuffer<T>::data() const noexcept
{
    return m_buffer.data();
}


template<class T> inline std::size_t AppendBuffer<T>::size() const noexcept
{
    return m_size;
}


template<class T> inline void AppendBuffer<T>::append(const T* data, std::size_t size)
{
    m_buffer.reserve_extra(size, m_size); // Throws
    std::copy_n(data, size, m_buffer.data() + m_size);
    m_size += size;
}


template<class T> inline void AppendBuffer<T>::append(std::size_t n, T value)
{
    m_buffer.reserve_extra(n, m_size); // Throws
    T* begin = m_buffer.data() + m_size;
    T* end = begin + n;
    std::fill(begin, end, value);
    m_size += n;
}


template<class T> inline void AppendBuffer<T>::reserve(std::size_t min_capacity)
{
    m_buffer.reserve(min_capacity, m_size);
}


template<class T> inline void AppendBuffer<T>::resize(std::size_t new_size)
{
    reserve(new_size);
    m_size = new_size;
}


template<class T> inline void AppendBuffer<T>::clear() noexcept
{
    m_size = 0;
}


template<class T> inline Buffer<T> AppendBuffer<T>::release() noexcept
{
    m_size = 0;
    return std::move(m_buffer);
}


} // namespace archon::base

#endif // ARCHON__BASE__APPEND_BUFFER_HPP
