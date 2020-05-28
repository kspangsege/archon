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

#ifndef ARCHON__BASE__SEED_MEMORY_BUFFER_HPP
#define ARCHON__BASE__SEED_MEMORY_BUFFER_HPP

#include <cstddef>
#include <algorithm>
#include <memory>
#include <array>

#include <archon/base/integer.hpp>
#include <archon/base/memory.hpp>
#include <archon/base/span.hpp>


namespace archon::base {


template<class T> class SeedMemoryBuffer {
public:
    using value_type     = T;
    using iterator       = T*;
    using const_iterator = const T*;

    explicit SeedMemoryBuffer(Span<T> seed_memory = {}) noexcept;
    explicit SeedMemoryBuffer(Span<T> seed_memory, std::size_t size);
    template<class U> explicit SeedMemoryBuffer(Span<T> seed_memory, Span<U> data);
    template<std::size_t N> explicit SeedMemoryBuffer(T (&seed_memory)[N]) noexcept;
    template<std::size_t N> explicit SeedMemoryBuffer(std::array<T, N>& seed_memory) noexcept;
    ~SeedMemoryBuffer() noexcept = default;


    void reset(Span<T> seed_memory = {}) noexcept;
    template<std::size_t N> void reset(T (&seed_memory)[N]) noexcept;
    template<std::size_t N> void reset(std::array<T, N>& seed_memory) noexcept;

    T& operator[](std::size_t) noexcept;
    const T& operator[](std::size_t) const noexcept;

    T* data() noexcept;
    const T* data() const noexcept;
    std::size_t size() const noexcept;

    iterator begin() noexcept;
    iterator end() noexcept;
    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;

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
    /// If the current size of this buffer is greater than, or equal to the
    /// specified minimum size (\p min_size), this function does
    /// nothing. Otherwise, it allocates a new larger chunk of memory and copies
    /// contents from the old chunk into the new one as specified.
    ///
    /// The new buffer size is determined as if by \ref
    /// base::suggest_new_buffer_size().
    ///
    /// \param new_size Specifies the required minimum size of the buffer.
    ///
    /// \param used_begin, used_end Specifies the range of elements that must be
    /// copied to the new memory chunk if one is allocated. Behaviour is
    /// undefined if either is greater than the current size of the buffer, or
    /// if \p used_begin is greater than \p used_end.
    ///
    /// \param copy_to Specifies where to copy elements to in the new memory
    /// chunk if one is allocated. Behaviour is undefined if \p copy_to is
    /// greater than \p min_size, or if `min_size - copy_to < copy_end -
    /// copy_begin`.
    ///
    void reserve(std::size_t min_size, std::size_t used_begin, std::size_t used_end,
                 std::size_t copy_to);

    // No copying
    SeedMemoryBuffer(const SeedMemoryBuffer&) = delete;
    SeedMemoryBuffer& operator=(const SeedMemoryBuffer&) = delete;

    // No moving
    SeedMemoryBuffer(SeedMemoryBuffer&&) = delete;
    SeedMemoryBuffer& operator=(SeedMemoryBuffer&&) = delete;

private:
    std::unique_ptr<T[]> m_owned;
    T* m_data = nullptr;
    std::size_t m_size = 0;

    void do_reserve_extra(std::size_t min_extra_size, std::size_t used_size);
    void do_reserve(std::size_t min_size, std::size_t used_begin, std::size_t used_end,
                    std::size_t copy_to);
};




template<class T, std::size_t N> class ArraySeededBuffer :
        private std::array<T, N>,
        public SeedMemoryBuffer<T> {
public:
    ArraySeededBuffer() noexcept;
    ~ArraySeededBuffer() noexcept = default;

    explicit ArraySeededBuffer(std::size_t size);
    template<class U> explicit ArraySeededBuffer(Span<U> data);

    using SeedMemoryBuffer<T>::operator[];
    using SeedMemoryBuffer<T>::data;
    using SeedMemoryBuffer<T>::size;
    using SeedMemoryBuffer<T>::begin;
    using SeedMemoryBuffer<T>::end;
};




template<class T> class SeedMemoryBufferContents {
public:
    using value_type     = T;
    using iterator       = T*;
    using const_iterator = const T*;
    using buffer_type    = SeedMemoryBuffer<T>;

    explicit SeedMemoryBufferContents(buffer_type& buffer) noexcept;

    T* data() noexcept;
    const T* data() const noexcept;
    std::size_t size() const noexcept;

    iterator begin() noexcept;
    iterator end() noexcept;
    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;

    void push_back(const T&);

private:
    buffer_type& m_buffer;
    std::size_t m_size = 0;
};








// Implementation


// ============================ SeedMemoryBuffer ============================


template<class T> inline SeedMemoryBuffer<T>::SeedMemoryBuffer(Span<T> seed_memory) noexcept :
    m_data(seed_memory.data()),
    m_size(seed_memory.size())
{
}


template<class T>
inline SeedMemoryBuffer<T>::SeedMemoryBuffer(Span<T> seed_memory, std::size_t size) :
    m_data(seed_memory.data()),
    m_size(size)
{
    if (ARCHON_UNLIKELY(size > seed_memory.size())) {
        m_owned = std::make_unique<T[]>(size); // Throws
        m_data = m_owned.get();
    }
}


template<class T>
template<class U> inline SeedMemoryBuffer<T>::SeedMemoryBuffer(Span<T> seed_memory, Span<U> data) :
    SeedMemoryBuffer(seed_memory, data.size()) // Throws
{
    std::copy_n(data.data(), data.size(), m_data);
}


template<class T> template<std::size_t N>
inline SeedMemoryBuffer<T>::SeedMemoryBuffer(T (&seed_memory)[N]) noexcept :
    SeedMemoryBuffer(Span(seed_memory))
{
}


template<class T> template<std::size_t N>
inline SeedMemoryBuffer<T>::SeedMemoryBuffer(std::array<T, N>& seed_memory) noexcept :
    SeedMemoryBuffer(Span(seed_memory))
{
}


template<class T> inline void SeedMemoryBuffer<T>::reset(Span<T> seed_memory) noexcept
{
    m_owned.reset();
    m_data = seed_memory.data();
    m_size = seed_memory.size();
}


template<class T> template<std::size_t N>
inline void SeedMemoryBuffer<T>::reset(T (&seed_memory)[N]) noexcept
{
    reset(Span(seed_memory));
}


template<class T> template<std::size_t N>
inline void SeedMemoryBuffer<T>::reset(std::array<T, N>& seed_memory) noexcept
{
    reset(Span(seed_memory));
}


template<class T> inline T& SeedMemoryBuffer<T>::operator[](std::size_t i) noexcept
{
    return m_data[i];
}


template<class T> inline const T& SeedMemoryBuffer<T>::operator[](std::size_t i) const noexcept
{
    return m_data[i];
}


template<class T> inline T* SeedMemoryBuffer<T>::data() noexcept
{
    return m_data;
}


template<class T> inline const T* SeedMemoryBuffer<T>::data() const noexcept
{
    return m_data;
}


template<class T> inline std::size_t SeedMemoryBuffer<T>::size() const noexcept
{
    return m_size;
}


template<class T> inline auto SeedMemoryBuffer<T>::begin() noexcept -> iterator
{
    return data();
}


template<class T> inline auto SeedMemoryBuffer<T>::end() noexcept -> iterator
{
    return data() + size();
}


template<class T> inline auto SeedMemoryBuffer<T>::begin() const noexcept -> const_iterator
{
    return data();
}


template<class T> inline auto SeedMemoryBuffer<T>::end() const noexcept -> const_iterator
{
    return data() + size();
}


template<class T>
inline void SeedMemoryBuffer<T>::reserve_extra(std::size_t min_extra_size, std::size_t used_size)
{
    ARCHON_ASSERT(used_size <= m_size);
    if (ARCHON_LIKELY(min_extra_size <= std::size_t(m_size - used_size)))
        return;
    do_reserve_extra(min_extra_size, used_size); // Throws
}


template<class T>
inline void SeedMemoryBuffer<T>::reserve(std::size_t min_size, std::size_t used_size)
{
    std::size_t used_begin = 0;
    std::size_t used_end   = used_size;
    std::size_t copy_to    = 0;
    reserve(min_size, used_begin, used_end, copy_to); // Throws
}


template<class T>
inline void SeedMemoryBuffer<T>::reserve(std::size_t min_size, std::size_t used_begin,
                                         std::size_t used_end, std::size_t copy_to)
{
    if (ARCHON_LIKELY(min_size <= m_size))
        return;
    do_reserve(min_size, used_begin, used_end, copy_to); // Throws
}


template<class T>
void SeedMemoryBuffer<T>::do_reserve_extra(std::size_t min_extra_size, std::size_t used_size)
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


template<class T>
void SeedMemoryBuffer<T>::do_reserve(std::size_t min_size, std::size_t used_begin,
                                     std::size_t used_end, std::size_t copy_to)
{
    ARCHON_ASSERT(used_begin <= used_end);
    ARCHON_ASSERT(used_end <= m_size);
    ARCHON_ASSERT(copy_to <= min_size);
    ARCHON_ASSERT(used_end - used_begin <= min_size - copy_to);
    std::size_t new_size   = base::suggest_new_buffer_size(m_size, min_size);
    std::unique_ptr<T[]> owned = std::make_unique<T[]>(new_size); // Throws
    std::copy(m_data + used_begin, m_data + used_end, owned.get() + copy_to);
    m_owned = std::move(owned);
    m_data = m_owned.get();
    m_size = new_size;
}



// ============================ ArraySeededBuffer ============================


template<class T, std::size_t N> inline ArraySeededBuffer<T, N>::ArraySeededBuffer() noexcept :
    SeedMemoryBuffer<T>(static_cast<std::array<T, N>&>(*this))
{
}


template<class T, std::size_t N>
inline ArraySeededBuffer<T, N>::ArraySeededBuffer(std::size_t size) :
    SeedMemoryBuffer<T>(static_cast<std::array<T, N>&>(*this), size) // Throws
{
}


template<class T, std::size_t N>
template<class U> inline ArraySeededBuffer<T, N>::ArraySeededBuffer(Span<U> data) :
    SeedMemoryBuffer<T>(static_cast<std::array<T, N>&>(*this), data) // Throws
{
}



// ============================ SeedMemoryBufferContents ============================


template<class T>
inline SeedMemoryBufferContents<T>::SeedMemoryBufferContents(buffer_type& buffer) noexcept :
    m_buffer(buffer)
{
}


template<class T> inline T* SeedMemoryBufferContents<T>::data() noexcept
{
    return m_buffer.data();
}


template<class T> inline const T* SeedMemoryBufferContents<T>::data() const noexcept
{
    return m_buffer.data();
}


template<class T> inline std::size_t SeedMemoryBufferContents<T>::size() const noexcept
{
    return m_size;
}


template<class T> inline auto SeedMemoryBufferContents<T>::begin() noexcept -> iterator
{
    return data();
}


template<class T> inline auto SeedMemoryBufferContents<T>::end() noexcept -> iterator
{
    return data() + size();
}


template<class T> inline auto SeedMemoryBufferContents<T>::begin() const noexcept -> const_iterator
{
    return data();
}


template<class T> inline auto SeedMemoryBufferContents<T>::end() const noexcept -> const_iterator
{
    return data() + size();
}


template<class T> inline void SeedMemoryBufferContents<T>::push_back(const T& value)
{
    m_buffer.reserve_extra(1, m_size); // Throws
    m_buffer[m_size] = value; // Throws
    ++m_size;
}


} // namespace archon::base

#endif // ARCHON__BASE__SEED_MEMORY_BUFFER_HPP
