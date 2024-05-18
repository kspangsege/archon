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

#ifndef ARCHON_X_CORE_X_IMPL_X_VECTOR_IMPL_HPP
#define ARCHON_X_CORE_X_IMPL_X_VECTOR_IMPL_HPP


#include <cstddef>
#include <type_traits>
#include <iterator>
#include <memory>
#include <utility>
#include <stdexcept>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/memory.hpp>


namespace archon::core::impl {


// The vector implementation. It provides a minimal, but less convenient API.

template<class T> class VectorImpl {
public:
    ~VectorImpl() noexcept;

    void reset(void* static_memory, std::size_t static_capacity) noexcept;
    void move(VectorImpl&& other) noexcept;

    void verify_index(std::size_t) const;

    auto data() noexcept       -> T*;
    auto data() const noexcept -> const T*;
    auto size() const noexcept     -> std::size_t;
    auto capacity() const noexcept -> std::size_t;

    void reserve_extra(std::size_t min_extra_capacity);
    void reserve(std::size_t min_capacity);
    void shrink_to_fit();

    template<class... A> void emplace_back(A&&... args);
    void pop_back() noexcept;

    void append(std::size_t size, const T& value);
    template<class I> void append(I begin, I end);

    // Only supported when T has a non-throwing move constructor
    template<class... A> void insert(std::size_t offset, A&&... args);
    void erase(std::size_t offset, std::size_t n) noexcept;

    void clear() noexcept;
    void resize(std::size_t size);
    void resize(std::size_t size, const T& value);

private:
    std::unique_ptr<std::byte[]> m_owned_memory;
    T* m_data = nullptr;
    std::size_t m_capacity = 0;
    std::size_t m_size = 0;

    void do_reserve_extra(std::size_t min_extra_capacity);
    void do_reserve(std::size_t min_capacity);

    void realloc(std::size_t new_capacity);
    void destroy() noexcept;
};








// Implementation


template<class T>
inline VectorImpl<T>::~VectorImpl() noexcept
{
    destroy();
}


template<class T>
inline void VectorImpl<T>::reset(void* static_memory, std::size_t static_capacity) noexcept
{
    destroy();
    m_owned_memory = {};
    m_data         = static_cast<T*>(static_memory);
    m_capacity     = static_capacity;
    m_size         = 0;
}


template<class T>
void VectorImpl<T>::move(VectorImpl&& other) noexcept
{
    ARCHON_ASSERT(!m_owned_memory);
    ARCHON_ASSERT(m_size == 0);
    if (ARCHON_LIKELY(!other.m_owned_memory)) {
        static_assert(std::is_nothrow_move_constructible_v<T>);
        core::uninit_safe_move(other.m_data, other.m_size, m_data);
    }
    else {
        m_owned_memory = std::move(other.m_owned_memory);
        m_data = other.m_data;
        other.m_data = nullptr;
    }
    m_capacity = other.m_capacity;
    m_size     = other.m_size;
    other.m_capacity = 0;
    other.m_size     = 0;
}


template<class T>
inline void VectorImpl<T>::verify_index(std::size_t i) const
{
    if (ARCHON_LIKELY(i < size()))
        return;
    throw std::out_of_range("Vector element index");
}


template<class T>
inline auto VectorImpl<T>::data() noexcept -> T*
{
    return m_data;
}


template<class T>
inline auto VectorImpl<T>::data() const noexcept -> const T*
{
    return m_data;
}


template<class T>
inline auto VectorImpl<T>::size() const noexcept -> std::size_t
{
    return m_size;
}


template<class T>
inline auto VectorImpl<T>::capacity() const noexcept -> std::size_t
{
    return m_capacity;
}


template<class T>
inline void VectorImpl<T>::reserve_extra(std::size_t min_extra_capacity)
{
    if (ARCHON_LIKELY(min_extra_capacity <= std::size_t(m_capacity - m_size)))
        return;
    do_reserve_extra(min_extra_capacity); // Throws
}


template<class T>
inline void VectorImpl<T>::reserve(std::size_t min_capacity)
{
    if (ARCHON_LIKELY(min_capacity <= m_capacity))
        return;
    do_reserve_extra(min_capacity); // Throws
}


template<class T>
void VectorImpl<T>::shrink_to_fit()
{
    if (m_owned_memory && m_size < m_capacity) {
        std::size_t new_capacity = m_size;
        realloc(new_capacity); // Throws
    }
}


template<class T>
template<class... A> void VectorImpl<T>::emplace_back(A&&... args)
{
    reserve_extra(1); // Throws
    core::uninit_create(data() + m_size, std::forward<A>(args)...); // Throws
    m_size += 1;
}


template<class T>
inline void VectorImpl<T>::pop_back() noexcept
{
    ARCHON_ASSERT(m_size > 0);
    std::size_t new_size = m_size - 1;
    core::uninit_destroy(data() + new_size, 1);
    m_size = new_size;
}


template<class T>
void VectorImpl<T>::append(std::size_t size, const T& value)
{
    reserve_extra(size); // Throws
    core::uninit_safe_fill(size, value, data() + m_size); // Throws
    m_size += size;
}


template<class T>
template<class I> void VectorImpl<T>::append(I begin, I end)
{
    static_assert(std::is_convertible_v<typename std::iterator_traits<I>::iterator_category,
                  std::random_access_iterator_tag>);
    std::size_t size = std::size_t(end - begin);
    reserve_extra(size); // Throws
    core::uninit_safe_copy(begin, end, data() + m_size); // Throws
    m_size += size;
}


template<class T>
template<class... A> void VectorImpl<T>::insert(std::size_t offset, A&&... args)
{
    reserve_extra(1); // Throws
    T* base = data();
    std::size_t move_size = std::size_t(m_size - offset);
    std::size_t move_dist = 1;
    core::uninit_move_upwards(base + offset, move_size, move_dist);
    try {
        core::uninit_create(base + offset, std::forward<A>(args)...); // Throws
    }
    catch (...) {
        core::uninit_move_downwards(base + offset + move_dist, move_size, move_dist);
        throw;
    }
    m_size += 1;
}


template<class T>
void VectorImpl<T>::erase(std::size_t offset, std::size_t n) noexcept
{
    ARCHON_ASSERT(offset <= m_size && n <= std::size_t(m_size - offset));
    ARCHON_ASSERT(n > 0);
    T* base = data();
    core::uninit_destroy(base + offset, n);
    std::size_t offset_2 = std::size_t(offset + n);
    std::size_t move_size = std::size_t(m_size - offset_2);
    std::size_t move_dist = n;
    core::uninit_move_downwards(base + offset_2, move_size, move_dist);
    m_size -= n;
}


template<class T>
inline void VectorImpl<T>::clear() noexcept
{
    core::uninit_destroy(data(), m_size);
    m_size = 0;
}


template<class T>
void VectorImpl<T>::resize(std::size_t size)
{
    if (size <= m_size) {
        core::uninit_destroy(data() + size, m_size - size);
    }
    else {
        reserve(size); // Throws
        core::uninit_safe_fill(size, data() + m_size);
    }
    m_size = size;
}


template<class T>
void VectorImpl<T>::resize(std::size_t size, const T& value)
{
    if (size <= m_size) {
        core::uninit_destroy(data() + size, m_size - size);
    }
    else {
        reserve(size); // Throws
        core::uninit_safe_fill(size, value, data() + m_size);
    }
    m_size = size;
}


template<class T>
void VectorImpl<T>::do_reserve_extra(std::size_t min_extra_capacity)
{
    std::size_t min_capacity = m_size;
    if (ARCHON_LIKELY(core::try_int_add(min_capacity, min_extra_capacity))) {
        do_reserve(min_capacity); // Throws
        return;
    }
    throw std::length_error("Vector capacity");
}


template<class T>
inline void VectorImpl<T>::do_reserve(std::size_t min_capacity)
{
    std::size_t new_capacity = core::suggest_new_buffer_size(m_capacity, min_capacity);
    realloc(new_capacity); // Throws
}


template<class T>
void VectorImpl<T>::realloc(std::size_t new_capacity)
{
    ARCHON_ASSERT(new_capacity >= m_size);
    std::size_t num_bytes = new_capacity;
    if (ARCHON_UNLIKELY(!core::try_int_mul(num_bytes, sizeof (T))))
        throw std::length_error("Vector capacity");
    std::unique_ptr<std::byte[]> new_owned_memory = std::make_unique<std::byte[]>(num_bytes); // Throws
    T* new_data = reinterpret_cast<T*>(new_owned_memory.get());
    core::uninit_safe_move(data(), m_size, new_data); // Throws
    m_owned_memory = std::move(new_owned_memory);
    m_data = new_data;
    m_capacity = new_capacity;
}


template<class T>
void VectorImpl<T>::destroy() noexcept
{
    core::uninit_destroy(m_data, m_size);
}


} // namespace archon::core::impl

#endif // ARCHON_X_CORE_X_IMPL_X_VECTOR_IMPL_HPP
