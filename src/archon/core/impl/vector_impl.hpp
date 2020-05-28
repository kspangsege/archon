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

/// \file


#include <cstddef>
#include <type_traits>
#include <iterator>
#include <memory>
#include <utility>
#include <stdexcept>

#include <archon/core/features.h>
#include <archon/core/type.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/memory.hpp>


namespace archon::core::impl {


// The vector implementation. It provides a minimal, but inconvenient API. It is designed to
// avoid having to know the size of the statically sized chunk of memory at compile time,
// which means that the same code can be used for different choices of the size of that
// chunk.

template<class T> class VectorImpl {
public:
    using strut_type = core::Strut<T>;

    void init(strut_type* static_mem, std::size_t static_capacity) noexcept;
    void dealloc(strut_type* static_mem) noexcept;

    void verify_index(std::size_t) const;

    auto data() noexcept       -> T*;
    auto data() const noexcept -> const T*;
    auto size() const noexcept     -> std::size_t;
    auto capacity() const noexcept -> std::size_t;

    void reserve_extra(strut_type* static_mem, std::size_t min_extra_capacity);
    void reserve(strut_type* static_mem, std::size_t min_capacity);
    void shrink_to_fit(strut_type* static_mem, std::size_t static_capacity);

    template<class... A> void emplace_back(strut_type* static_mem, A&&... args);
    void pop_back() noexcept;

    void append(strut_type* static_mem, std::size_t size, const T& value);
    template<class I> void append(strut_type* static_mem, I begin, I end);

    template<class... A> void insert(std::size_t offset, strut_type* static_mem, A&&... args);

    void erase(std::size_t offset, std::size_t n) noexcept;

    void resize(strut_type* static_mem, std::size_t size);
    void resize(strut_type* static_mem, std::size_t size, const T& value);

private:
    strut_type* m_mem;
    std::size_t m_capacity;
    std::size_t m_size;

    void do_reserve_extra(strut_type* static_mem, std::size_t min_extra_capacity);
    void do_reserve(strut_type* static_mem, std::size_t min_capacity);

    bool has_allocation(strut_type* static_mem) const noexcept;
    void realloc(strut_type* static_mem, std::size_t new_capacity);
};








// Implementation


template<class T>
inline void VectorImpl<T>::init(strut_type* static_mem, std::size_t static_capacity) noexcept
{
    m_mem      = static_mem;
    m_capacity = static_capacity;
    m_size     = 0;
}


template<class T>
inline void VectorImpl<T>::dealloc(strut_type* static_mem) noexcept
{
    core::uninit_destroy(data(), m_size);
    if (!has_allocation(static_mem))
        return;
    delete[] m_mem;
}


template<class T>
inline void VectorImpl<T>::verify_index(std::size_t i) const
{
    if (ARCHON_LIKELY(i <= size()))
        return;
    throw std::out_of_range("Vector element index");
}


template<class T>
inline auto VectorImpl<T>::data() noexcept -> T*
{
    return static_cast<T*>(static_cast<void*>(m_mem));
}


template<class T>
inline auto VectorImpl<T>::data() const noexcept -> const T*
{
    return static_cast<T*>(static_cast<void*>(m_mem));
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
inline void VectorImpl<T>::reserve_extra(strut_type* static_mem, std::size_t min_extra_capacity)
{
    if (ARCHON_LIKELY(min_extra_capacity <= std::size_t(m_capacity - m_size)))
        return;
    do_reserve_extra(static_mem, min_extra_capacity); // Throws
}


template<class T>
inline void VectorImpl<T>::reserve(strut_type* static_mem, std::size_t min_capacity)
{
    if (ARCHON_LIKELY(min_capacity <= m_capacity))
        return;
    do_reserve_extra(static_mem, min_capacity); // Throws
}


template<class T>
void VectorImpl<T>::shrink_to_fit(strut_type* static_mem, std::size_t static_capacity)
{
    if (m_size <= static_capacity) {
        if (!has_allocation(static_mem))
            return;
        T* new_data = static_cast<T*>(static_cast<void*>(static_mem));
        core::uninit_safe_move_or_copy(data(), m_size, new_data); // Throws
        dealloc(static_mem);
        m_mem      = static_mem;
        m_capacity = static_capacity;
        return;
    }
    if (m_capacity > m_size) {
        std::size_t new_capacity = m_size;
        realloc(static_mem, m_size, new_capacity); // Throws
    }
}


template<class T>
template<class... A> void VectorImpl<T>::emplace_back(strut_type* static_mem, A&&... args)
{
    reserve_extra(static_mem, 1); // Throws
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
void VectorImpl<T>::append(strut_type* static_mem, std::size_t size, const T& value)
{
    reserve_extra(static_mem, size); // Throws
    core::uninit_safe_fill(size, value, data() + m_size); // Throws
    m_size += size;
}


template<class T>
template<class I> void VectorImpl<T>::append(strut_type* static_mem, I begin, I end)
{
    static_assert(std::is_convertible_v<typename std::iterator_traits<I>::iterator_category,
                  std::random_access_iterator_tag>);
    std::size_t size = std::size_t(end - begin);
    reserve_extra(static_mem, size); // Throws
    core::uninit_safe_copy(begin, end, data() + m_size); // Throws
    m_size += size;
}


template<class T>
template<class... A> void VectorImpl<T>::insert(std::size_t offset, strut_type* static_mem, A&&... args)
{
    reserve_extra(static_mem, 1); // Throws
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
void VectorImpl<T>::resize(strut_type* static_mem, std::size_t size)
{
    if (size <= m_size) {
        core::uninit_destroy(data() + size, m_size - size);
    }
    else {
        reserve(static_mem, size); // Throws
        core::uninit_safe_fill(size, data() + m_size);
    }
    m_size = size;
}


template<class T>
void VectorImpl<T>::resize(strut_type* static_mem, std::size_t size, const T& value)
{
    if (size <= m_size) {
        core::uninit_destroy(data() + size, m_size - size);
    }
    else {
        reserve(static_mem, size); // Throws
        core::uninit_safe_fill(size, value, data() + m_size);
    }
    m_size = size;
}


template<class T>
void VectorImpl<T>::do_reserve_extra(strut_type* static_mem, std::size_t min_extra_capacity)
{
    std::size_t min_capacity = m_size;
    if (ARCHON_LIKELY(core::try_int_add(min_capacity, min_extra_capacity))) {
        do_reserve(static_mem, min_capacity); // Throws
        return;
    }
    throw std::length_error("Vector size");
}


template<class T>
inline void VectorImpl<T>::do_reserve(strut_type* static_mem, std::size_t min_capacity)
{
    std::size_t new_capacity = core::suggest_new_buffer_size(m_capacity, min_capacity);
    realloc(static_mem, new_capacity); // Throws
}


template<class T>
inline bool VectorImpl<T>::has_allocation(strut_type* static_mem) const noexcept
{
    return (m_mem != static_mem);
}


template<class T>
void VectorImpl<T>::realloc(strut_type* static_mem, std::size_t new_capacity)
{
    ARCHON_ASSERT(new_capacity >= m_size);
    std::unique_ptr<strut_type[]> new_mem = std::make_unique<strut_type[]>(new_capacity); // Throws
    T* new_data = static_cast<T*>(static_cast<void*>(new_mem.get()));
    core::uninit_safe_move_or_copy(data(), m_size, new_data); // Throws
    dealloc(static_mem);
    m_mem      = new_mem.release();
    m_capacity = new_capacity;
}


} // namespace archon::core::impl

#endif // ARCHON_X_CORE_X_IMPL_X_VECTOR_IMPL_HPP
