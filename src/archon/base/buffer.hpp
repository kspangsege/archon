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

#ifndef ARCHON_X_BASE_X_BUFFER_HPP
#define ARCHON_X_BASE_X_BUFFER_HPP

/// \file


#include <cstddef>
#include <type_traits>
#include <limits>
#include <algorithm>
#include <stdexcept>
#include <utility>
#include <memory>
#include <streambuf>

#include <archon/base/features.h>
#include <archon/base/integer.hpp>
#include <archon/base/memory.hpp>
#include <archon/base/span.hpp>


namespace archon::base {


/// A simple buffer concept that owns a region of memory and knows its size.
///
template<class T> class Buffer {
public:
    using value_type     = T;
    using iterator       = T*;
    using const_iterator = const T*;

    Buffer() noexcept = default;
    ~Buffer() noexcept = default;

    explicit Buffer(std::size_t initial_size);
    template<class U> explicit Buffer(Span<U> initial_data);

    Buffer(Buffer<T>&&) noexcept = default;
    Buffer<T>& operator=(Buffer<T>&&) noexcept = default;

    T& operator[](std::size_t) noexcept;
    const T& operator[](std::size_t) const noexcept;

    T* data() noexcept;
    const T* data() const noexcept;
    std::size_t size() const noexcept;

    iterator begin() noexcept;
    iterator end() noexcept;
    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;

    /// \brief Whether size is non-zero.
    ///
    /// This function returns true if, and only if the size of this buffer is
    /// nonzero. \ref data() returns a non-null pointer when, and only when this
    /// function returns true.
    ///
    explicit operator bool() const noexcept;

    /// \brief Change size of buffer.
    ///
    /// This function changes the size of the buffer by allocating new
    /// memory. Any original contents is discarded.
    ///
    void set_size(std::size_t new_size);

    /// \brief Ensure extra buffer capacity.
    ///
    /// This function is a shorthand for calling `reserve(used_size +
    /// min_extra_size, used_size)`, except that, if the sum overflows, this
    /// function throws `std::length_error`.
    ///
    void reserve_extra(std::size_t min_extra_size, std::size_t used_size);

    /// \brief Ensure buffer capacity.
    ///
    /// This function is a shorthand for calling `reserve(min_size, 0,
    /// used_size, 0)`.
    ///
    void reserve(std::size_t min_size, std::size_t used_size = 0);

    /// \brief Ensure buffer capacity.
    ///
    /// This function is a shorthand for calling \ref resize(), but only if the
    /// current buffer size is less than \p min_size.
    ///
    /// When \ref resize() is called, the new buffer size is determined as if by
    /// \ref base::suggest_new_buffer_size(), and arguments \p used_begin, \p
    /// used_end, and \p copy_to are passed along unmodified.
    ///
    /// This function returns to new offset of the retained section, which is \p
    /// copy_to if \ref resize() was called, and \p used_begin otherwise. If
    /// this function throws, nothing will have changed.
    ///
    std::size_t reserve(std::size_t min_size, std::size_t used_begin, std::size_t used_end,
                        std::size_t copy_to);

    /// \brief Resize the buffer while retaining a part of it.
    ///
    /// This function allocates a new chunk of memory and copies contents from
    /// the old chunk into the new one as specified.
    ///
    /// \param new_size Specifies the new size of the buffer.
    ///
    /// \param used_begin, used_end Specifies the range of elements that must be
    /// copied to the new memory chunk. Behaviour is undefined if either is
    /// greater than the current size of the buffer, or if \p used_begin is
    /// greater than \p used_end.
    ///
    /// \param copy_to Specifies where to copy elements to in the new memory
    /// chunk. Behaviour is undefined if \p copy_to is greater than \p min_size,
    /// or if `min_size - copy_to < copy_end - copy_begin`.
    ///
    void resize(std::size_t new_size, std::size_t used_begin, std::size_t used_end,
                std::size_t copy_to);

    T* release() noexcept;

    friend void swap(Buffer& a, Buffer& b) noexcept
    {
        std::swap(a.m_data, b.m_data);
        std::swap(a.m_size, b.m_size);
    }

    /// \brief Suggest a new buffer size.
    ///
    /// This function is a shorthand for calling the free-standing function \ref
    /// base::suggest_new_buffer_size() with the current size of this buffer as
    /// the first argument.
    ///
    std::size_t suggest_new_size(std::size_t min_size) noexcept;

    /// \brief Suggest a new buffer size.
    ///
    /// This function is a shorthand for calling suggest_new_size() with
    /// `used_size + min_extra_size` passed for \p min_size. Although, if the
    /// sum overflows, this function throws std::length_error.
    ///
    std::size_t suggest_new_size_extra(std::size_t used_size, std::size_t min_extra_size);

private:
    std::unique_ptr<T[]> m_data;
    std::size_t m_size = 0;

    void do_reserve_extra(std::size_t min_extra_size, std::size_t used_size);
    void do_reserve(std::size_t min_size, std::size_t used_begin, std::size_t used_end,
                    std::size_t copy_to);
    static std::unique_ptr<T[]> alloc(std::size_t);
};


template<class T> Buffer(Span<T>) -> Buffer<std::remove_const_t<T>>;




/// While the the stream buffer object exists, the associated buffer (`Buffer`)
/// may not be accessed in any other way that can cause the underlying memory to
/// be reallocted. Doing so leads to undefined behaviour, and probable memory
/// corruption.
///
template<class C, class T = std::char_traits<C>> class BasicBufferOutputStreambuf :
        public std::basic_streambuf<C, T> {
public:
    using int_type    = typename std::basic_streambuf<C, T>::int_type;
    using traits_type = typename std::basic_streambuf<C, T>::traits_type;

    explicit BasicBufferOutputStreambuf(Buffer<C>&, std::size_t initial_offset = 0);
    ~BasicBufferOutputStreambuf() noexcept = default;

    // Disable copying
    BasicBufferOutputStreambuf(const BasicBufferOutputStreambuf&) = delete;
    BasicBufferOutputStreambuf& operator=(const BasicBufferOutputStreambuf&) = delete;

    /// Returns the offset within the buffer of the current write position.
    ///
    std::size_t get_offset() const noexcept;

    /// Change the current writing position.
    ///
    /// It is an error if the specified offset is greater than the current size
    /// of the associated buffer (\ref Buffer::size()).
    ///
    void set_offset(std::size_t) noexcept;

    /// Return a view of the part of the buffer that precedes the current
    /// writing position.
    ///
    std::basic_string_view<C, T> view() const noexcept;

    /// Ensure that the amount of available buffer space after the current
    /// writing position (\ref get_offset()) is at least \p min_extra_capacity.
    ///
    void reserve_extra(std::size_t min_extra_capacity);

    // Maximally efficient append operations
    void append(const C* data, std::size_t size);
    void append(std::size_t size, C value);

private:
    Buffer<C>& m_buffer;

    std::streamsize xsputn(const C*, std::streamsize) override final;
    int_type overflow(int_type) override final;

    void reset_put_area(std::size_t offset) noexcept;
    void bump_put_ptr(std::size_t size) noexcept;
};


using BufferOutputStreambuf     = BasicBufferOutputStreambuf<char>;
using WideBufferOutputStreambuf = BasicBufferOutputStreambuf<wchar_t>;








// Implementation


// ============================ Buffer ============================


template<class T> inline Buffer<T>::Buffer(std::size_t initial_size) :
    m_data(alloc(initial_size)), // Throws
    m_size(initial_size)
{
}


template<class T> template<class U> inline Buffer<T>::Buffer(Span<U> initial_data) :
    Buffer(initial_data.size()) // Throws
{
    std::copy_n(initial_data.data(), initial_data.size(), data());
}


template<class T> inline T& Buffer<T>::operator[](std::size_t i) noexcept
{
    return m_data[i];
}


template<class T> inline const T& Buffer<T>::operator[](std::size_t i) const noexcept
{
    return m_data[i];
}


template<class T> inline T* Buffer<T>::data() noexcept
{
    return m_data.get();
}


template<class T> inline const T* Buffer<T>::data() const noexcept
{
    return m_data.get();
}


template<class T> inline std::size_t Buffer<T>::size() const noexcept
{
    return m_size;
}


template<class T> inline auto Buffer<T>::begin() noexcept -> iterator
{
    return data();
}


template<class T> inline auto Buffer<T>::end() noexcept -> iterator
{
    return data() + size();
}


template<class T> inline auto Buffer<T>::begin() const noexcept -> const_iterator
{
    return data();
}


template<class T> inline auto Buffer<T>::end() const noexcept -> const_iterator
{
    return data() + size();
}


template<class T> inline Buffer<T>::operator bool() const noexcept
{
    return bool(m_data);
}


template<class T> inline void Buffer<T>::set_size(std::size_t new_size)
{
    m_data = alloc(new_size); // Throws
    m_size = new_size;
}


template<class T>
inline void Buffer<T>::reserve_extra(std::size_t min_extra_size, std::size_t used_size)
{
    ARCHON_ASSERT(used_size <= m_size);
    if (ARCHON_LIKELY(min_extra_size <= std::size_t(m_size - used_size)))
        return;
    do_reserve_extra(min_extra_size, used_size); // Throws
}


template<class T> inline void Buffer<T>::reserve(std::size_t min_size, std::size_t used_size)
{
    std::size_t used_begin = 0;
    std::size_t used_end   = used_size;
    std::size_t copy_to    = 0;
    reserve(min_size, used_begin, used_end, copy_to); // Throws
}


template<class T>
inline std::size_t Buffer<T>::reserve(std::size_t min_size, std::size_t used_begin,
                                      std::size_t used_end, std::size_t copy_to)
{
    if (ARCHON_LIKELY(min_size <= m_size))
        return used_begin;
    do_reserve(min_size, used_begin, used_end, copy_to); // Throws
    return copy_to;
}


template<class T>
inline void Buffer<T>::resize(std::size_t new_size, std::size_t used_begin, std::size_t used_end,
                              std::size_t copy_to)
{
    ARCHON_ASSERT(used_begin <= used_end);
    ARCHON_ASSERT(used_end <= m_size);
    ARCHON_ASSERT(copy_to <= new_size);
    ARCHON_ASSERT(used_end - used_begin <= new_size - copy_to);
    std::unique_ptr<T[]> new_data = alloc(new_size); // Throws
    std::copy_n(m_data.get() + used_begin, used_end - used_begin, new_data.get() + copy_to);
    m_data = std::move(new_data);
    m_size = new_size;
}


template<class T> inline T* Buffer<T>::release() noexcept
{
    m_size = 0;
    return m_data.release();
}


template<class T> inline std::size_t Buffer<T>::suggest_new_size(std::size_t min_size) noexcept
{
    return base::suggest_new_buffer_size(m_size, min_size);
}


template<class T>
inline std::size_t Buffer<T>::suggest_new_size_extra(std::size_t used_size,
                                                     std::size_t min_extra_size)
{
    std::size_t min_size = used_size;
    if (ARCHON_LIKELY(base::try_int_add(min_size, min_extra_size)))
        return suggest_new_size(min_size);
    throw std::length_error("Buffer size");
}


template<class T>
void Buffer<T>::do_reserve_extra(std::size_t min_extra_size, std::size_t used_size)
{
    std::size_t min_size = used_size;
    if (ARCHON_LIKELY(base::try_int_add(min_size, min_extra_size))) {
        std::size_t used_begin = 0;
        std::size_t used_end   = used_size;
        std::size_t copy_to    = 0;
        do_reserve(min_size, used_begin, used_end, copy_to); // Throws
        return;
    }
    throw std::length_error("Buffer size");
}


template<class T> inline void Buffer<T>::do_reserve(std::size_t min_size, std::size_t used_begin,
                                                    std::size_t used_end, std::size_t copy_to)
{
    std::size_t new_size = suggest_new_size(min_size);
    resize(new_size, used_begin, used_end, copy_to); // Throws
}


template<class T> inline std::unique_ptr<T[]> Buffer<T>::alloc(std::size_t size)
{
    if (ARCHON_LIKELY(size > 0))
        return std::make_unique<T[]>(size); // Throws
    return nullptr;
}



// ============================ BasicBufferOutputStreambuf ============================


template<class C, class T>
inline BasicBufferOutputStreambuf<C,T>::BasicBufferOutputStreambuf(Buffer<C>& buffer,
                                                                   std::size_t initial_offset):
    std::basic_streambuf<C,T>(), // Throws
    m_buffer(buffer)
{
    reset_put_area(initial_offset);
}


template<class C, class T>
inline std::size_t BasicBufferOutputStreambuf<C,T>::get_offset() const noexcept
{
    return std::size_t(this->pptr() - m_buffer.data());
}


template<class C, class T>
inline void BasicBufferOutputStreambuf<C,T>::set_offset(std::size_t offset) noexcept
{
    reset_put_area(offset);
}


template<class C, class T>
inline std::basic_string_view<C, T> BasicBufferOutputStreambuf<C, T>::view() const noexcept
{
    return { m_buffer.data(), get_offset() };
}


template<class C, class T>
inline void BasicBufferOutputStreambuf<C, T>::reserve_extra(std::size_t min_extra_capacity)
{
    std::size_t offset = get_offset();
    m_buffer.reserve_extra(min_extra_capacity, offset); // Throws
    reset_put_area(offset);
}


template<class C, class T>
inline void BasicBufferOutputStreambuf<C, T>::append(const C* data, std::size_t size)
{
    reserve_extra(size); // Throws
    std::copy_n(data, size, this->pptr());
    bump_put_ptr(size);
}


template<class C, class T>
inline void BasicBufferOutputStreambuf<C, T>::append(std::size_t size, C value)
{
    reserve_extra(size); // Throws
    std::fill_n(this->pptr(), size, value);
    bump_put_ptr(size);
}


template<class C, class T>
std::streamsize BasicBufferOutputStreambuf<C, T>::xsputn(const C* s, std::streamsize count)
{
    if (ARCHON_LIKELY(count >= 0)) {
        std::streamsize offset = 0;
        using uint = std::make_unsigned_t<std::streamsize>;
        std::size_t max = std::numeric_limits<std::size_t>::max();
        if (ARCHON_UNLIKELY(uint(count) >= max)) {
            do {
                append(s + offset, max); // Throws
                offset += std::streamsize(max);
            }
            while (uint(count - offset) >= max);
        }
        append(s + offset, std::streamsize(count - offset)); // Throws
    }
    return count;
}


template<class C, class T>
inline auto BasicBufferOutputStreambuf<C,T>::overflow(int_type ch) -> int_type
{
    // `ch == traits_type::eof()` means "flush", which is a no-op for this type
    // of stream buffer.
    if (traits_type::eq_int_type(ch, traits_type::eof()))
        return traits_type::not_eof(ch);
    std::size_t offset = get_offset();
    if (ARCHON_LIKELY(offset < m_buffer.size()))
        goto write;
    reserve_extra(1); // Throws
  write:
    *this->pptr() = traits_type::to_char_type(ch);
    this->pbump(1);
    return ch;
}


template<class C, class T>
inline void BasicBufferOutputStreambuf<C,T>::reset_put_area(std::size_t offset) noexcept
{
    C* begin = m_buffer.data();
    C* end   = begin + m_buffer.size();
    this->setp(begin + offset, end);
}


template<class C, class T>
void BasicBufferOutputStreambuf<C, T>::bump_put_ptr(std::size_t size) noexcept
{
    std::size_t size_2 = size;
    int max = std::numeric_limits<int>::max();
    if (ARCHON_UNLIKELY(size_2 >= unsigned(max))) {
        do {
            this->pbump(max);
            size_2 -= unsigned(max);
        }
        while (size_2 >= unsigned(max));
    }
    this->pbump(int(size_2));
}


} // namespace archon::base

#endif // ARCHON_X_BASE_X_BUFFER_HPP
