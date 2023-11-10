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

#ifndef ARCHON_X_CORE_X_BUFFER_HPP
#define ARCHON_X_CORE_X_BUFFER_HPP

/// \file


#include <cstddef>
#include <utility>
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <array>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/memory.hpp>


namespace archon::core {


/// \brief Buffer constructor selection tag.
///
/// See \ref core::Buffer.
///
struct BufferDataTag {};



/// \brief Expandable chunk of memory.
///
/// An instance of this class owns a chunk of memory, and offers ways of expanding and
/// accessing that memory.
///
/// \sa \ref core::ArraySeededBuffer.
/// \sa \ref core::BufferContents.
/// \sa \ref core::StringBufferContents.
///
template<class T> class Buffer {
public:
    using value_type = T;

    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;
    using span_type       = core::Span<T>;
    using const_span_type = core::Span<const T>;

    using reference              = T&;
    using const_reference        = const T&;
    using pointer                = T*;
    using const_pointer          = const T*;
    using iterator               = pointer;
    using const_iterator         = const_pointer;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    Buffer() noexcept;
    explicit Buffer(span_type seed_memory) noexcept;
    template<std::size_t N> explicit Buffer(T (& seed_memory)[N]) noexcept;
    template<std::size_t N> explicit Buffer(std::array<T, N>& seed_memory) noexcept;
    explicit Buffer(std::size_t size);
    explicit Buffer(span_type seed_memory, std::size_t size);
    template<class U> Buffer(core::BufferDataTag, core::Span<U> data);
    template<class U> Buffer(span_type seed_memory, core::BufferDataTag, core::Span<U> data);
    ~Buffer() noexcept = default;

    Buffer(Buffer&&) noexcept;
    auto operator=(Buffer&&) noexcept -> Buffer&;

    auto at(std::size_t)       -> T&;
    auto at(std::size_t) const -> const T&;

    auto operator[](std::size_t) noexcept       -> T&;
    auto operator[](std::size_t) const noexcept -> const T&;

    auto size() const noexcept -> std::size_t;

    auto data() noexcept       -> T*;
    auto data() const noexcept -> const T*;

    auto span() noexcept       -> span_type;
    auto span() const noexcept -> const_span_type;

    auto begin() noexcept        -> iterator;
    auto begin() const noexcept  -> const_iterator;
    auto cbegin() const noexcept -> const_iterator;

    auto end() noexcept          -> iterator;
    auto end() const noexcept    -> const_iterator;
    auto cend() const noexcept   -> const_iterator;

    auto rbegin() noexcept        -> reverse_iterator;
    auto rbegin() const noexcept  -> const_reverse_iterator;
    auto crbegin() const noexcept -> const_reverse_iterator;

    auto rend() noexcept          -> reverse_iterator;
    auto rend() const noexcept    -> const_reverse_iterator;
    auto crend() const noexcept   -> const_reverse_iterator;

    /// \brief Ensure extra buffer capacity.
    ///
    /// This function ensures that the buffer size is greater than the size of the current
    /// buffer contents (\p used_size) by at least \p min_extra_size. Specifically,
    /// `reserve_extra(min_extra_size, used_size, max_size) has the same effect as
    /// `reserve(used_size + min_extra_size, used_size, max_size)`, except that, if the sum
    /// overflows, this function throws `std::length_error`.
    ///
    /// This function offers a strong exception guarantee, which means that if it fails,
    /// which it does when it thrown an exception, then the buffer is left unchanged (both
    /// buffer address and contents remain unchanged).
    ///
    void reserve_extra(std::size_t min_extra_size, std::size_t used_size, std::size_t max_size = -1);

    /// \brief Ensure buffer capacity.
    ///
    /// This function expands the buffer as specified when necessary. Specifically,
    /// `buffer.reserve(min_size, used_size, max_size)` is a shorthand for:
    ///
    /// \code{.cpp}
    ///
    ///   buffer.reserve_a(min_size, [&](core::Span<const T> src, core::Span<T> dst) noexcept {
    ///       std::copy_n(src.data(), used_size, dst.data());
    ///   }, max_size);
    ///
    /// \endcode
    ///
    /// This function offers a strong exception guarantee, which means that if it fails,
    /// which it does when it thrown an exception, then the buffer is left unchanged (both
    /// buffer address and contents remain unchanged).
    ///
    void reserve(std::size_t min_size, std::size_t used_size = 0, std::size_t max_size = -1);

    /// \brief Ensure capacity and perform custom operation when memory is re-allocated.
    ///
    /// This function performs the same operation as \ref reserve(), except that when new
    /// memory is allocated, the specified function (\p func) is called before the buffer
    /// contents is copied to the new memory location. The specified function will be called
    /// with one argument of type `span_type` referring to the new memory. The old memory is
    /// available to the function through \ref data() and \ref size(). The specified
    /// function is not allowed to modify the buffer object, nor the contents of the
    /// buffer. If the specified function throws, the reserve operation fails and leaves the
    /// buffer unchanged.
    ///
    /// This function offers a strong exception guarantee, which means that if it fails,
    /// which it does when it thrown an exception, then the buffer is left unchanged (both
    /// buffer address and contents remain unchanged).
    ///
    template<class F> void reserve_f(std::size_t min_size, std::size_t used_size, F&& func, std::size_t max_size = -1);

    /// \brief Expand buffer.
    ///
    /// This function expands the buffer size by at least \p min_extra_size. Specifically,
    /// `buffer.expand(min_extra_size, used_size, max_size)` has the same effect as
    /// `buffer.reserve(buffer.size() + min_extra_size, used_size, max_size)`, except that,
    /// if the sum overflows, this function throws `std::length_error`.
    ///
    /// This function offers a strong exception guarantee, which means that if it fails,
    /// which it does when it thrown an exception, then the buffer is left unchanged (both
    /// buffer address and contents remain unchanged).
    ///
    void expand(std::size_t min_extra_size, std::size_t used_size, std::size_t max_size = -1);

    /// \brief Ensure extra buffer capacity with custom copy function.
    ///
    /// This function ensures that the buffer size is greater than the size of the current
    /// buffer contents (\p used_size) by at least \p min_extra_size. Specifically,
    /// `reserve_extra_a(min_extra_size, used_size, copy_func, max_size)` has the same
    /// effect as `reserve_a(used_size + min_extra_size, used_size, copy_func, max_size)`,
    /// except that, if the sum overflows, this function throws `std::length_error`.
    ///
    /// This function offers a strong exception guarantee, which means that if it fails,
    /// which it does when it thrown an exception, then the buffer is left unchanged (both
    /// buffer address and contents remain unchanged).
    ///
    template<class F> void reserve_extra_a(std::size_t min_extra_size, std::size_t used_size, F&& copy_func,
                                           std::size_t max_size = -1);

    /// \brief Ensure buffer capacity with custom copy function.
    ///
    /// If the current size of this buffer is greater than, or equal to the specified
    /// minimum size (\p min_size), this function does nothing. Otherwise, if the specified
    /// minimum size is greater than the specified maximum size (\p max_size), this function
    /// throws `std::length_error`. Otherwise, this function allocates a new larger chunk of
    /// memory and calls the specified copy function (\p copy_func) in order to get the
    /// buffer contents transferred to the new memory chunk. The copy function will be
    /// invoked with one argument of type `span_type` referring to the new memory chunk. The
    /// address and size of the old memory chunk is available as \ref data() and \ref size()
    /// respectively. The new memory chunk will never be smaller than that old one. The copy
    /// function will be invoked at most once. The copy function is not allowed to modify
    /// the buffer object, nor the contents of the buffer. If the copy function throws, the
    /// reservation operation fails and leaves the buffer unchanged. After a return from the
    /// copy function, `reserve_a()` is guaranteed to succeed. The new buffer size is
    /// determined as if by `core::suggest_new_buffer_size(size(), min_size, max_size)` (see
    /// \ref core::suggest_new_buffer_size()).
    ///
    /// This function offers a strong exception guarantee, which means that if it fails,
    /// which it does when it thrown an exception, then the buffer is left unchanged (both
    /// buffer address and contents remain unchanged).
    ///
    template<class F> void reserve_a(std::size_t min_size, F&& copy_func, std::size_t max_size = -1);

    /// \{
    ///
    /// \brief Place data in buffer at offset.
    ///
    /// These functions copy the specified data into the buffer at the specified offset
    /// after expanding the buffer as necessary.
    ///
    /// `buffer.append(data, offset)` is a shorthand for:
    ///
    /// \code{.cpp}
    ///
    ///   buffer.reserve_extra(data.size(), offset);
    ///   std::copy_n(data.data(), data.size(), buffer.data() + offset);
    ///   offset += data.size();
    ///
    /// \endcode
    ///
    /// `buffer.append_a(val, offset, n)` is a shorthand for:
    ///
    /// \code{.cpp}
    ///
    ///   buffer.reserve_extra(n, offset);
    ///   std::fill_n(buffer.data() + offset, n, val);
    ///   offset += n;
    ///
    /// \endcode
    ///
    /// Note that \p offset must be less than, or equal to the prior size of the buffer,
    /// and, on success, \p offset will be updated to refer to the end of the data as it was
    /// placed in the buffer.
    ///
    void append(const_span_type data, std::size_t& offset);
    void append_a(T val, std::size_t& offset, std::size_t n = 1);
    /// \}

    /// \{
    ///
    /// \brief Place data in buffer before offset.
    ///
    /// These functions copy the specified data into the buffer before the specified offset
    /// after expanding the buffer as necessary.
    ///
    /// `buffer.prepend(data, offset)` is a shorthand for:
    ///
    /// \code{.cpp}
    ///
    ///   std::size_t used_size = std::size_t(buffer.size() - offset);
    ///   buffer.reserve_extra_a(data.size(), used_size, [&](core::Span<T> new_mem) noexcept {
    ///       std::copy_n(buffer.data() + offset, used_size, new_mem.data() + new_mem.size() - used_size);
    ///       offset += new_mem.size() - buffer.size();
    ///   });
    ///   offset -= data.size();
    ///   std::copy_n(data.data(), data.size(), buffer.data() + offset);
    ///
    /// \endcode
    ///
    /// `buffer.prepend_a(val, offset, n)` is a shorthand for:
    ///
    /// \code{.cpp}
    ///
    ///   std::size_t used_size = std::size_t(buffer.size() - offset);
    ///   buffer.reserve_extra_a(n, used_size, [&](core::Span<T> new_mem) noexcept {
    ///       std::copy_n(buffer.data() + offset, used_size, new_mem.data() + (new_mem.size() - used_size));
    ///       offset += new_mem.size() - buffer.size();
    ///   });
    ///   offset -= n;
    ///   std::fill_n(buffer.data() + offset, n, val);
    ///
    /// \endcode
    ///
    /// Note that \p offset must be less than, or equal to the prior size of the buffer, and
    /// that, on success, \p offset will be updated to refer to the beginning of the data as
    /// it was placed in the buffer.
    ///
    void prepend(const_span_type data, std::size_t& offset);
    void prepend_a(T val, std::size_t& offset, std::size_t n = 1);
    /// \}

    // No copying
    Buffer(const Buffer&) = delete;
    auto operator=(const Buffer&) -> Buffer& = delete;

private:
    std::unique_ptr<T[]> m_memory_owner;
    span_type m_memory;

    void verify_index(std::size_t) const;
    template<class F> void do_reserve(std::size_t min_size, std::size_t min_extra_size, F&& copy_func,
                                      std::size_t max_size);
};








// Implementation


template<class T>
inline Buffer<T>::Buffer() noexcept
{
}


template<class T>
inline Buffer<T>::Buffer(span_type seed_memory) noexcept
    : m_memory(seed_memory)
{
}


template<class T>
template<std::size_t N> inline Buffer<T>::Buffer(T (& seed_memory)[N]) noexcept
    : Buffer(span_type(seed_memory))
{
}


template<class T>
template<std::size_t N> inline Buffer<T>::Buffer(std::array<T, N>& seed_memory) noexcept
    : Buffer(span_type(seed_memory))
{
}


template<class T>
inline Buffer<T>::Buffer(std::size_t size)
    : Buffer({}, size) // Throws
{
}


template<class T>
inline Buffer<T>::Buffer(span_type seed_memory, std::size_t size)
    : m_memory({ seed_memory.data(), size })
{
    if (ARCHON_LIKELY(size <= seed_memory.size()))
        return;
    m_memory_owner = std::make_unique<T[]>(size); // Throws
    m_memory = { m_memory_owner.get(), size };
}


template<class T>
template<class U> inline Buffer<T>::Buffer(core::BufferDataTag, core::Span<U> data)
    : Buffer({}, core::BufferDataTag(), data) // Throws
{
}


template<class T>
template<class U> inline Buffer<T>::Buffer(span_type seed_memory, core::BufferDataTag, core::Span<U> data)
    : Buffer(seed_memory, data.size()) // Throws
{
    std::copy_n(data.data(), data.size(), this->data());
}


template<class T>
inline Buffer<T>::Buffer(Buffer&& other) noexcept
    : m_memory_owner(std::move(other.m_memory_owner))
    , m_memory(other.m_memory)
{
    other.m_memory = {};
}


template<class T>
inline auto Buffer<T>::operator=(Buffer&& other) noexcept -> Buffer&
{
    m_memory_owner = std::move(other.m_memory_owner);
    m_memory = other.m_memory;
    other.m_memory = {};
    return *this;
}


template<class T>
inline auto Buffer<T>::at(size_type i) -> T&
{
    verify_index(i); // Throws
    return m_memory[i];
}


template<class T>
inline auto Buffer<T>::at(size_type i) const -> const T&
{
    verify_index(i); // Throws
    return m_memory[i];
}


template<class T>
inline auto Buffer<T>::operator[](std::size_t i) noexcept -> T&
{
    return m_memory[i];
}


template<class T>
inline auto Buffer<T>::operator[](std::size_t i) const noexcept -> const T&
{
    return m_memory[i];
}


template<class T>
inline auto Buffer<T>::size() const noexcept -> std::size_t
{
    return m_memory.size();
}


template<class T>
inline auto Buffer<T>::data() noexcept -> T*
{
    return m_memory.data();
}


template<class T>
inline auto Buffer<T>::data() const noexcept -> const T*
{
    return m_memory.data();
}


template<class T>
inline auto Buffer<T>::span() noexcept -> span_type
{
    return m_memory;
}


template<class T>
inline auto Buffer<T>::span() const noexcept -> const_span_type
{
    return m_memory;
}


template<class T>
inline auto Buffer<T>::begin() noexcept -> iterator
{
    return data();
}


template<class T>
inline auto Buffer<T>::begin() const noexcept -> const_iterator
{
    return data();
}


template<class T>
inline auto Buffer<T>::cbegin() const noexcept -> const_iterator
{
    return begin();
}


template<class T>
inline auto Buffer<T>::end() noexcept -> iterator
{
    return data() + size();
}


template<class T>
inline auto Buffer<T>::end() const noexcept -> const_iterator
{
    return data() + size();
}


template<class T>
inline auto Buffer<T>::cend() const noexcept -> const_iterator
{
    return end();
}


template<class T>
inline auto Buffer<T>::rbegin() noexcept -> reverse_iterator
{
    return reverse_iterator(end());
}


template<class T>
inline auto Buffer<T>::rbegin() const noexcept -> const_reverse_iterator
{
    return const_reverse_iterator(end());
}


template<class T>
inline auto Buffer<T>::crbegin() const noexcept -> const_reverse_iterator
{
    return rbegin();
}


template<class T>
inline auto Buffer<T>::rend() noexcept -> reverse_iterator
{
    return reverse_iterator(begin());
}


template<class T>
inline auto Buffer<T>::rend() const noexcept -> const_reverse_iterator
{
    return const_reverse_iterator(begin());
}


template<class T>
inline auto Buffer<T>::crend() const noexcept -> const_reverse_iterator
{
    return rend();
}


template<class T>
inline void Buffer<T>::reserve_extra(std::size_t min_extra_size, std::size_t used_size, std::size_t max_size)
{
    reserve_extra_a(min_extra_size, used_size, [&](span_type new_mem) noexcept {
        std::copy_n(data(), used_size, new_mem.data()); // Throws
    }, max_size); // Throws
}


template<class T>
inline void Buffer<T>::reserve(std::size_t min_size, std::size_t used_size, std::size_t max_size)
{
    reserve_a(min_size, [&](span_type new_mem) noexcept {
        std::copy_n(data(), used_size, new_mem.data()); // Throws
    }, max_size); // Throws
}


template<class T>
template<class F> inline void Buffer<T>::reserve_f(std::size_t min_size, std::size_t used_size, F&& func,
                                                   std::size_t max_size)
{
    reserve_a(min_size, [&](span_type new_mem) noexcept {
        func(new_mem); // Throws
        std::copy_n(data(), used_size, new_mem.data()); // Throws
    }, max_size); // Throws
}


template<class T>
inline void Buffer<T>::expand(std::size_t min_extra_size, std::size_t used_size, std::size_t max_size)
{
    reserve_extra_a(min_extra_size, size(), [&](span_type new_mem) noexcept {
        std::copy_n(data(), used_size, new_mem.data()); // Throws
    }, max_size); // Throws
}


template<class T>
template<class F> inline void Buffer<T>::reserve_extra_a(std::size_t min_extra_size, std::size_t used_size,
                                                         F&& copy_func, std::size_t max_size)
{
    ARCHON_ASSERT(used_size <= m_memory.size());
    if (ARCHON_LIKELY(min_extra_size <= std::size_t(m_memory.size() - used_size)))
        return;
    do_reserve(used_size, min_extra_size, std::forward<F>(copy_func), max_size); // Throws
}


template<class T>
template<class F> inline void Buffer<T>::reserve_a(std::size_t min_size, F&& copy_func, std::size_t max_size)
{
    if (ARCHON_LIKELY(min_size <= m_memory.size()))
        return;
    std::size_t min_extra_size = 0;
    do_reserve(min_size, min_extra_size, std::forward<F>(copy_func), max_size); // Throws
}


template<class T>
inline void Buffer<T>::append(const_span_type data, std::size_t& offset)
{
    reserve_extra(data.size(), offset); // Throws
    std::copy_n(data.data(), data.size(), m_memory.data() + offset); // Throws
    offset += data.size();
}


template<class T>
inline void Buffer<T>::append_a(T val, std::size_t& offset, std::size_t n)
{
    reserve_extra(n, offset); // Throws
    std::fill_n(m_memory.data() + offset, n, val); // Throws
    offset += n;
}


template<class T>
void Buffer<T>::prepend(const_span_type data, std::size_t& offset)
{
    std::size_t used_size = std::size_t(size() - offset);
    reserve_extra_a(data.size(), used_size, [&](span_type new_mem) noexcept {
        std::copy_n(this->data() + offset, used_size, new_mem.data() + new_mem.size() - used_size);
        offset += new_mem.size() - size();
    }); // Throws
    offset -= data.size();
    std::copy_n(data.data(), data.size(), m_memory.data() + offset);
}


template<class T>
void Buffer<T>::prepend_a(T val, std::size_t& offset, std::size_t n)
{
    std::size_t used_size = std::size_t(size() - offset);
    reserve_extra_a(n, used_size, [&](span_type new_mem) noexcept {
        std::copy_n(data() + offset, used_size, new_mem.data() + new_mem.size() - used_size);
        offset += new_mem.size() - size();
    }); // Throws
    offset -= n;
    std::fill_n(m_memory.data() + offset, n, val);
}


template<class T>
inline void Buffer<T>::verify_index(std::size_t i) const
{
    if (ARCHON_LIKELY(i <= size()))
        return;
    throw std::out_of_range("Buffer element index");
}


template<class T>
template<class F>
void Buffer<T>::do_reserve(std::size_t min_size, std::size_t min_extra_size, F&& copy_func, std::size_t max_size)
{
    if (ARCHON_LIKELY(min_size <= max_size && min_extra_size <= std::size_t(max_size - min_size))) {
        std::size_t new_size = core::suggest_new_buffer_size(m_memory.size(), min_size + min_extra_size, max_size);
        std::unique_ptr<T[]> new_memory_owner = std::make_unique<T[]>(new_size); // Throws
        span_type new_memory = { new_memory_owner.get(), new_size };
        copy_func(new_memory); // Throws
        m_memory_owner = std::move(new_memory_owner);
        m_memory = new_memory;
        return;
    }
    throw std::length_error("Buffer size");
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_BUFFER_HPP
