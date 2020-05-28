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

#ifndef ARCHON__BASE__ARRAY_SEEDED_VECTOR_HPP
#define ARCHON__BASE__ARRAY_SEEDED_VECTOR_HPP

#include <cstddef>
#include <type_traits>
#include <limits>
#include <algorithm>
#include <iterator>
#include <memory>
#include <utility>
#include <initializer_list>
#include <stdexcept>

#include <archon/base/features.h>
#include <archon/base/type_traits.hpp>
#include <archon/base/assert.hpp>
#include <archon/base/integer.hpp>
#include <archon/base/memory.hpp>


namespace archon::base {


namespace detail {
template<class T> class ArraySeededVectorCore;
} // namespace detail




template<class T, std::size_t N> class ArraySeededVector {
public:
    using value_type      = T;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference       = T&;
    using const_reference = const T&;
    using pointer         = T*;
    using const_pointer   = const T*;

    using iterator               = pointer;
    using const_iterator         = const_pointer;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    ArraySeededVector() noexcept;
    ArraySeededVector(const ArraySeededVector&) = delete;
    ArraySeededVector(std::initializer_list<T>);
    explicit ArraySeededVector(size_type size);
    ArraySeededVector(size_type size, const T& value);
    template<class I, class = NeedIter<I>> ArraySeededVector(I begin, I end);
    ~ArraySeededVector() noexcept;

    ArraySeededVector& operator=(const ArraySeededVector&) = delete;
    ArraySeededVector& operator=(std::initializer_list<T>);

    void assign(std::initializer_list<T>);
    void assign(size_type size, const T& value);
    template<class I, class = NeedIter<I>> void assign(I begin, I end);

    // Element access

    T& at(size_type);
    const T& at(size_type) const;

    T& operator[](size_type) noexcept;
    const T& operator[](size_type) const noexcept;

    T& front() noexcept;
    const T& front() const noexcept;

    T& back() noexcept;
    const T& back() const noexcept;

    T* data() noexcept;
    const T* data() const noexcept;

    // Iterators

    iterator begin() noexcept;
    const_iterator begin() const noexcept;
    const_iterator cbegin() const noexcept;

    iterator end() noexcept;
    const_iterator end() const noexcept;
    const_iterator cend() const noexcept;

    reverse_iterator rbegin() noexcept;
    const_reverse_iterator rbegin() const noexcept;
    const_reverse_iterator crbegin() const noexcept;

    reverse_iterator rend() noexcept;
    const_reverse_iterator rend() const noexcept;
    const_reverse_iterator crend() const noexcept;

    // Size / capacity

    bool empty() const noexcept;
    size_type size() const noexcept;
    size_type max_size() const noexcept;
    size_type capacity() const noexcept;

    void reserve_extra(size_type min_extra_capacity);
    void reserve(size_type min_capacity);
    void shrink_to_fit();

    // Modifiers

    reference push_back(const T&);
    reference push_back(T&&);
    template<class... A> reference emplace_back(A&&... args);
    void pop_back() noexcept;

    void append(std::initializer_list<T>);
    void append(size_type size, const T& value);
    template<class I, class = NeedIter<I>> void append(I begin, I end);

    void clear() noexcept;
    void resize(size_type size);
    void resize(size_type size, const T& value);

    // Comparison

    template<class U, std::size_t M> bool operator==(const ArraySeededVector<U, M>&) const
        noexcept(noexcept(std::declval<T>() == std::declval<U>()));
    template<class U, std::size_t M> bool operator!=(const ArraySeededVector<U, M>&) const
        noexcept(noexcept(std::declval<T>() == std::declval<U>()));
    template<class U, std::size_t M> bool operator<(const ArraySeededVector<U, M>&) const
        noexcept(noexcept(std::declval<T>() < std::declval<U>()));
    template<class U, std::size_t M> bool operator>(const ArraySeededVector<U, M>&) const
        noexcept(noexcept(std::declval<T>() < std::declval<U>()));
    template<class U, std::size_t M> bool operator<=(const ArraySeededVector<U, M>&) const
        noexcept(noexcept(std::declval<T>() < std::declval<U>()));
    template<class U, std::size_t M> bool operator>=(const ArraySeededVector<U, M>&) const
        noexcept(noexcept(std::declval<T>() < std::declval<U>()));

private:
    using Core = detail::ArraySeededVectorCore<T>;
    using strut_type = typename Core::strut_type;

    Core m_core;
    strut_type m_static_mem[N];

    template<class I> void do_append(I begin, I end);
    template<class I> void do_append(I begin, I end, std::input_iterator_tag);
    template<class I> void do_append(I begin, I end, std::random_access_iterator_tag);
};








// Implementation


// ============================ detail::ArraySeededVectorCore ============================


// The core of the array seeded vector implementation. It provides a minimal,
// but inconvenient API. It is designed to avoid having to know the size of the
// statically sized chunk of memory at compile time, which means that the same
// code can be used for different choices of the size of that chunk.
//
template<class T> class detail::ArraySeededVectorCore {
public:
    using size_type = std::size_t;
    using strut_type = Strut<T>;

    void init(strut_type* static_mem, size_type static_capacity) noexcept;
    void dealloc(strut_type* static_mem) noexcept;

    T* data() noexcept;
    const T* data() const noexcept;
    size_type size() const noexcept;
    size_type capacity() const noexcept;

    void reserve_extra(strut_type* static_mem, size_type min_extra_capacity);
    void reserve(strut_type* static_mem, size_type min_capacity);
    void shrink_to_fit(strut_type* static_mem, size_type static_capacity);

    template<class... A> void emplace_back(strut_type* static_mem, A&&... args);
    void pop_back() noexcept;

    void append(strut_type* static_mem, size_type size, const T& value);
    template<class I> void append(strut_type* static_mem, I begin, I end);

    void resize(strut_type* static_mem, size_type size);
    void resize(strut_type* static_mem, size_type size, const T& value);

private:
    strut_type* m_mem;
    size_type m_capacity;
    size_type m_size;

    void do_reserve_extra(strut_type* static_mem, size_type min_extra_capacity);
    void do_reserve(strut_type* static_mem, size_type min_capacity);

    bool has_allocation(strut_type* static_mem) const noexcept;
    void realloc(strut_type* static_mem, size_type new_capacity);
};


template<class T>
inline void detail::ArraySeededVectorCore<T>::init(strut_type* static_mem,
                                                   size_type static_capacity) noexcept
{
    m_mem      = static_mem;
    m_capacity = static_capacity;
    m_size     = 0;
}


template<class T>
inline void detail::ArraySeededVectorCore<T>::dealloc(strut_type* static_mem) noexcept
{
    base::uninit_destroy(data(), m_size);
    if (!has_allocation(static_mem))
        return;
    delete[] m_mem;
}


template<class T> inline T* detail::ArraySeededVectorCore<T>::data() noexcept
{
    return static_cast<T*>(static_cast<void*>(m_mem));
}


template<class T> inline const T* detail::ArraySeededVectorCore<T>::data() const noexcept
{
    return const_cast<ArraySeededVectorCore*>(this)->data();
}


template<class T> inline auto detail::ArraySeededVectorCore<T>::size() const noexcept -> size_type
{
    return m_size;
}


template<class T>
inline auto detail::ArraySeededVectorCore<T>::capacity() const noexcept -> size_type
{
    return m_capacity;
}


template<class T>
inline void detail::ArraySeededVectorCore<T>::reserve_extra(strut_type* static_mem,
                                                            size_type min_extra_capacity)
{
    if (ARCHON_LIKELY(min_extra_capacity <= std::size_t(m_capacity - m_size)))
        return;
    do_reserve_extra(static_mem, min_extra_capacity); // Throws
}


template<class T>
inline void detail::ArraySeededVectorCore<T>::reserve(strut_type* static_mem,
                                                      size_type min_capacity)
{
    if (ARCHON_LIKELY(min_capacity <= m_capacity))
        return;
    do_reserve_extra(static_mem, min_capacity); // Throws
}


template<class T>
void detail::ArraySeededVectorCore<T>::shrink_to_fit(strut_type* static_mem,
                                                     size_type static_capacity)
{
    if (m_size <= static_capacity) {
        if (!has_allocation(static_mem))
            return;
        T* new_data = static_cast<T*>(static_cast<void*>(static_mem));
        base::uninit_safe_move_or_copy(data(), m_size, new_data); // Throws
        dealloc(static_mem);
        m_mem      = static_mem;
        m_capacity = static_capacity;
        return;
    }
    if (m_capacity > m_size) {
        size_type new_capacity = m_size;
        realloc(static_mem, m_size, new_capacity); // Throws
    }
}


template<class T> template<class... A>
void detail::ArraySeededVectorCore<T>::emplace_back(strut_type* static_mem, A&&... args)
{
    reserve_extra(static_mem, 1); // Throws
    base::uninit_create(data() + m_size, std::forward<A>(args)...); // Throws
    m_size += 1;
}


template<class T> inline void detail::ArraySeededVectorCore<T>::pop_back() noexcept
{
    ARCHON_ASSERT(m_size > 0);
    size_type new_size = m_size - 1;
    base::uninit_destroy(data() + new_size, 1);
    m_size = new_size;
}


template<class T>
void detail::ArraySeededVectorCore<T>::append(strut_type* static_mem, size_type size,
                                              const T& value)
{
    reserve_extra(static_mem, size); // Throws
    base::uninit_safe_fill(size, value, data() + m_size); // Throws
    m_size += size;
}


template<class T> template<class I>
void detail::ArraySeededVectorCore<T>::append(strut_type* static_mem, I begin, I end)
{
    static_assert(std::is_convertible_v<typename std::iterator_traits<I>::iterator_category,
                  std::random_access_iterator_tag>);
    size_type size = std::size_t(end - begin);
    reserve_extra(static_mem, size); // Throws
    base::uninit_safe_copy(begin, end, data() + m_size); // Throws
    m_size += size;
}


template<class T>
void detail::ArraySeededVectorCore<T>::resize(strut_type* static_mem, size_type size)
{
    if (size <= m_size) {
        base::uninit_destroy(data() + size, m_size - size);
    }
    else {
        reserve(static_mem, size); // Throws
        base::uninit_safe_fill(size, data() + m_size);
    }
    m_size = size;
}


template<class T>
void detail::ArraySeededVectorCore<T>::resize(strut_type* static_mem, size_type size,
                                              const T& value)
{
    if (size <= m_size) {
        base::uninit_destroy(data() + size, m_size - size);
    }
    else {
        reserve(static_mem, size); // Throws
        base::uninit_safe_fill(size, value, data() + m_size);
    }
    m_size = size;
}


template<class T>
void detail::ArraySeededVectorCore<T>::do_reserve_extra(strut_type* static_mem,
                                                        size_type min_extra_capacity)
{
    size_type min_capacity = m_size;
    if (ARCHON_LIKELY(base::try_int_add(min_capacity, min_extra_capacity))) {
        do_reserve(static_mem, min_capacity); // Throws
        return;
    }
    throw std::length_error("Vector size");
}


template<class T>
inline void detail::ArraySeededVectorCore<T>::do_reserve(strut_type* static_mem,
                                                         size_type min_capacity)
{
    size_type new_capacity = base::suggest_new_buffer_size(m_capacity, min_capacity);
    realloc(static_mem, new_capacity); // Throws
}


template<class T>
inline bool detail::ArraySeededVectorCore<T>::has_allocation(strut_type* static_mem) const noexcept
{
    return (m_mem != static_mem);
}


template<class T>
void detail::ArraySeededVectorCore<T>::realloc(strut_type* static_mem, size_type new_capacity)
{
    ARCHON_ASSERT(new_capacity >= m_size);
    std::unique_ptr<strut_type[]> new_mem = std::make_unique<strut_type[]>(new_capacity); // Throws
    T* new_data = static_cast<T*>(static_cast<void*>(new_mem.get()));
    base::uninit_safe_move_or_copy(data(), m_size, new_data); // Throws
    dealloc(static_mem);
    m_mem      = new_mem.release();
    m_capacity = new_capacity;
}


// ============================ ArraySeededVector ============================


template<class T, std::size_t N> inline ArraySeededVector<T, N>::ArraySeededVector() noexcept
{
    m_core.init(m_static_mem, N);
}


template<class T, std::size_t N>
inline ArraySeededVector<T, N>::ArraySeededVector(std::initializer_list<T> list) :
    ArraySeededVector()
{
    append(list); // Throws
}


template<class T, std::size_t N>
inline ArraySeededVector<T, N>::ArraySeededVector(size_type size) :
    ArraySeededVector()
{
    resize(size); // Throws
}


template<class T, std::size_t N>
inline ArraySeededVector<T, N>::ArraySeededVector(size_type size, const T& value) :
    ArraySeededVector()
{
    append(size, value); // Throws
}


template<class T, std::size_t N>
template<class I, class> inline ArraySeededVector<T, N>::ArraySeededVector(I begin, I end) :
    ArraySeededVector()
{
    append(begin, end); // Throws
}


template<class T, std::size_t N> inline ArraySeededVector<T, N>::~ArraySeededVector() noexcept
{
    m_core.dealloc(m_static_mem);
}


template<class T, std::size_t N>
inline auto ArraySeededVector<T, N>::operator=(std::initializer_list<T> list) -> ArraySeededVector&
{
    assign(list); // Throws
    return *this;
}


template<class T, std::size_t N>
inline void ArraySeededVector<T, N>::assign(std::initializer_list<T> list)
{
    clear();
    append(list); // Throws
}


template<class T, std::size_t N>
inline void ArraySeededVector<T, N>::assign(size_type size, const T& value)
{
    clear();
    append(size, value); // Throws
}


template<class T, std::size_t N>
template<class I, class> inline void ArraySeededVector<T, N>::assign(I begin, I end)
{
    clear();
    append(begin, end); // Throws
}


template<class T, std::size_t N> inline T& ArraySeededVector<T, N>::at(size_type i)
{
    if (ARCHON_LIKELY(i < size()))
        return operator[](i);
    throw std::out_of_range("Index");
}


template<class T, std::size_t N> inline const T& ArraySeededVector<T, N>::at(size_type i) const
{
    return const_cast<ArraySeededVector*>(this)->at(i); // Throws
}


template<class T, std::size_t N>
inline T& ArraySeededVector<T, N>::operator[](size_type i) noexcept
{
    return data()[i];
}


template<class T, std::size_t N>
inline const T& ArraySeededVector<T, N>::operator[](size_type i) const noexcept
{
    return data()[i];
}


template<class T, std::size_t N> inline T& ArraySeededVector<T, N>::front() noexcept
{
    return operator[](0);
}


template<class T, std::size_t N> inline const T& ArraySeededVector<T, N>::front() const noexcept
{
    return operator[](0);
}


template<class T, std::size_t N> inline T& ArraySeededVector<T, N>::back() noexcept
{
    return operator[](size() - 1);
}


template<class T, std::size_t N> inline const T& ArraySeededVector<T, N>::back() const noexcept
{
    return operator[](size() - 1);
}


template<class T, std::size_t N> inline T* ArraySeededVector<T, N>::data() noexcept
{
    return m_core.data();
}


template<class T, std::size_t N> inline const T* ArraySeededVector<T, N>::data() const noexcept
{
    return m_core.data();
}


template<class T, std::size_t N> inline auto ArraySeededVector<T, N>::begin() noexcept -> iterator
{
    return data();
}


template<class T, std::size_t N>
inline auto ArraySeededVector<T, N>::begin() const noexcept -> const_iterator
{
    return const_cast<ArraySeededVector*>(this)->begin();
}


template<class T, std::size_t N>
inline auto ArraySeededVector<T, N>::cbegin() const noexcept -> const_iterator
{
    return begin();
}


template<class T, std::size_t N> inline auto ArraySeededVector<T, N>::end() noexcept -> iterator
{
    return data() + size();
}


template<class T, std::size_t N>
inline auto ArraySeededVector<T, N>::end() const noexcept -> const_iterator
{
    return const_cast<ArraySeededVector*>(this)->end();
}


template<class T, std::size_t N>
inline auto ArraySeededVector<T, N>::cend() const noexcept -> const_iterator
{
    return end();
}


template<class T, std::size_t N>
inline auto ArraySeededVector<T, N>::rbegin() noexcept -> reverse_iterator
{
    return std::reverse_iterator<iterator>(end());
}


template<class T, std::size_t N>
inline auto ArraySeededVector<T, N>::rbegin() const noexcept -> const_reverse_iterator
{
    return const_cast<ArraySeededVector*>(this)->rbegin();
}


template<class T, std::size_t N>
inline auto ArraySeededVector<T, N>::crbegin() const noexcept -> const_reverse_iterator
{
    return rbegin();
}


template<class T, std::size_t N>
inline auto ArraySeededVector<T, N>::rend() noexcept -> reverse_iterator
{
    return std::reverse_iterator<iterator>(begin());
}


template<class T, std::size_t N>
inline auto ArraySeededVector<T, N>::rend() const noexcept -> const_reverse_iterator
{
    return const_cast<ArraySeededVector*>(this)->rend();
}


template<class T, std::size_t N>
inline auto ArraySeededVector<T, N>::crend() const noexcept -> const_reverse_iterator
{
    return rend();
}


template<class T, std::size_t N> inline bool ArraySeededVector<T, N>::empty() const noexcept
{
    return (size() == 0);
}


template<class T, std::size_t N>
inline auto ArraySeededVector<T, N>::size() const noexcept -> size_type
{
    return m_core.size();
}


template<class T, std::size_t N>
inline auto ArraySeededVector<T, N>::max_size() const noexcept -> size_type
{
    return std::numeric_limits<difference_type>::max();
}


template<class T, std::size_t N>
inline auto ArraySeededVector<T, N>::capacity() const noexcept -> size_type
{
    return m_core.capacity();
}


template<class T, std::size_t N>
inline void ArraySeededVector<T, N>::reserve_extra(size_type min_extra_capacity)
{
    m_core.reserve_extra(m_static_mem, min_extra_capacity); // Throws
}


template<class T, std::size_t N>
inline void ArraySeededVector<T, N>::reserve(size_type min_capacity)
{
    m_core.reserve(m_static_mem, min_capacity); // Throws
}


template<class T, std::size_t N> inline void ArraySeededVector<T, N>::shrink_to_fit()
{
    m_core.shrink_to_fit(m_static_mem, N); // Throws
}


template<class T, std::size_t N> inline T& ArraySeededVector<T, N>::push_back(const T& value)
{
    return emplace_back(value); // Throws
}


template<class T, std::size_t N> inline T& ArraySeededVector<T, N>::push_back(T&& value)
{
    return emplace_back(std::move(value)); // Throws
}


template<class T, std::size_t N>
template<class... A> inline T& ArraySeededVector<T, N>::emplace_back(A&&... args)
{
    m_core.emplace_back(m_static_mem, std::forward<A>(args)...); // Throws
    return back();
}


template<class T, std::size_t N> inline void ArraySeededVector<T, N>::pop_back() noexcept
{
    m_core.pop_back();
}


template<class T, std::size_t N>
inline void ArraySeededVector<T, N>::append(std::initializer_list<T> list)
{
    do_append(list.begin(), list.end()); // Throws
}


template<class T, std::size_t N>
inline void ArraySeededVector<T, N>::append(size_type size, const T& value)
{
    m_core.append(size, value); // Throws
}


template<class T, std::size_t N>
template<class I, class> inline void ArraySeededVector<T, N>::append(I begin, I end)
{
    do_append(begin, end); // Throws
}


template<class T, std::size_t N> inline void ArraySeededVector<T, N>::clear() noexcept
{
    resize(0);
}


template<class T, std::size_t N> inline void ArraySeededVector<T, N>::resize(size_type size)
{
    m_core.resize(m_static_mem, size); // throws
}


template<class T, std::size_t N>
inline void ArraySeededVector<T, N>::resize(size_type size, const T& value)
{
    m_core.resize(m_static_mem, size, value); // throws
}


template<class T, std::size_t N> template<class U, std::size_t M>
inline bool ArraySeededVector<T, N>::operator==(const ArraySeededVector<U, M>& vector) const
    noexcept(noexcept(std::declval<T>() == std::declval<U>()))
{
    return std::equal(begin(), end(), vector.begin(), vector.end()); // Throws
}


template<class T, std::size_t N> template<class U, std::size_t M>
inline bool ArraySeededVector<T, N>::operator!=(const ArraySeededVector<U, M>& vector) const
    noexcept(noexcept(std::declval<T>() == std::declval<U>()))
{
    return !operator==(vector); // Throws
}


template<class T, std::size_t N> template<class U, std::size_t M>
inline bool ArraySeededVector<T, N>::operator<(const ArraySeededVector<U, M>& vector) const
    noexcept(noexcept(std::declval<T>() < std::declval<U>()))
{
    return std::lexicographical_compare(begin(), end(), vector.begin(), vector.end()); // Throws
}


template<class T, std::size_t N> template<class U, std::size_t M>
inline bool ArraySeededVector<T, N>::operator>(const ArraySeededVector<U, M>& vector) const
    noexcept(noexcept(std::declval<T>() < std::declval<U>()))
{
    return (vector < *this); // Throws
}


template<class T, std::size_t N> template<class U, std::size_t M>
inline bool ArraySeededVector<T, N>::operator<=(const ArraySeededVector<U, M>& vector) const
    noexcept(noexcept(std::declval<T>() < std::declval<U>()))
{
    return !operator>(vector); // Throws
}


template<class T, std::size_t N> template<class U, std::size_t M>
inline bool ArraySeededVector<T, N>::operator>=(const ArraySeededVector<U, M>& vector) const
    noexcept(noexcept(std::declval<T>() < std::declval<U>()))
{
    return !operator<(vector); // Throws
}


template<class T, std::size_t N>
template<class I> inline void ArraySeededVector<T, N>::do_append(I begin, I end)
{
    using iterator_category = typename std::iterator_traits<I>::iterator_category;
    do_append(begin, end, iterator_category()); // Throws
}


template<class T, std::size_t N> template<class I>
inline void ArraySeededVector<T, N>::do_append(I begin, I end, std::input_iterator_tag)
{
    for (I i = begin; i != end; ++i)
        push_back(*i); // Throws
}


template<class T, std::size_t N> template<class I>
inline void ArraySeededVector<T, N>::do_append(I begin, I end, std::random_access_iterator_tag)
{
    m_core.append(m_static_mem, begin, end); // Throws
}


} // namespace archon::base

#endif // ARCHON__BASE__ARRAY_SEEDED_VECTOR_HPP
