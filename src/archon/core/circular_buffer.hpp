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

#ifndef ARCHON_X_CORE_X_CIRCULAR_BUFFER_HPP
#define ARCHON_X_CORE_X_CIRCULAR_BUFFER_HPP

/// \file


#include <cstddef>
#include <type_traits>
#include <limits>
#include <memory>
#include <iterator>
#include <algorithm>
#include <utility>
#include <stdexcept>
#include <initializer_list>

#include <archon/core/features.h>
#include <archon/core/type.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/memory.hpp>


namespace archon::core {


/// \brief A container backed by a "circular buffer".
///
/// This container is similar to std::deque in that it offers efficient element insertion
/// and removal at both ends. Insertion at either end occurs in amortized constant
/// time. Removal at either end occurs in constant time.
///
/// As opposed to std::deque, this container allows for reservation of buffer space, such
/// that value insertion can be guaranteed to not reallocate buffer memory, and to not
/// throw. More specifically, a single insert operation, that inserts zero or more values at
/// either end, is guaranteed to not reallocate buffer memory if the prior capacity
/// (capacity()) is greater than, or equal to the prior size (size()) plus the number of
/// inserted values. Further more, such an operation is guaranteed to not throw if the
/// capacity is sufficient, and the relevant constructor of the value type does not throw,
/// and, in the case of inserting a range of values specified as a pair of iterators, if no
/// exception is thrown while operating on those iterators.
///
/// This container uses a single contiguous chunk of memory as backing storage, but it
/// allows for the logical sequence of values to wrap around from the end, to the beginning
/// of that chunk. Because the logical sequence of values can have a storage-wise
/// discontinuity of this kind, this container does not meet the requirements of
/// `ContiguousContainer` as defined by C++17.
///
/// When the first element is removed (pop_front()), iterators pointing to the removed
/// element will be invalidated. All other iterators, including "end iterators" (end()),
/// will remain valid.
///
/// When the last element is removed (pop_back()), iterators pointing to the removed element
/// will become "end iterators" (end()), and "end iterators" will be invalidated. All other
/// iterators will remain valid.
///
/// When an element is inserted at the front (push_front()), and the prior capacity
/// (capacity()) is strictly greater than the prior size (size()), all iterators remain
/// valid.
///
/// When an element is inserted at the back (push_back()), and the prior capacity
/// (capacity()) is strictly greater than the prior size (size()), "end iterators" (end())
/// become iterators to the inserted element, and all other iterators remain valid.
///
/// Operations pop_front(), pop_back(), and clear(), are guaranteed to leave the capacity
/// unchanged.
///
/// Iterators are of the "random access" kind (std::random_access_iterator_tag).
///
template<class T> class CircularBuffer {
private:
    template<class> class Iter;

public:
    static_assert(std::is_nothrow_destructible_v<T>);

    using value_type      = T;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference       = T&;
    using const_reference = const T&;
    using pointer         = T*;
    using const_pointer   = const T*;

    using iterator               = Iter<value_type>;
    using const_iterator         = Iter<const value_type>;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    CircularBuffer() noexcept = default;
    CircularBuffer(const CircularBuffer&);
    CircularBuffer(CircularBuffer&&) noexcept;
    CircularBuffer(std::initializer_list<T>);
    explicit CircularBuffer(size_type size);
    CircularBuffer(size_type size, const T& value);
    template<class I, class = NeedIter<I>> CircularBuffer(I begin, I end);
    ~CircularBuffer() noexcept;

    auto operator=(const CircularBuffer&) -> CircularBuffer&;
    auto operator=(CircularBuffer&&) noexcept -> CircularBuffer&;
    auto operator=(std::initializer_list<T>) -> CircularBuffer&;

    void assign(std::initializer_list<T>);
    void assign(size_type size, const T& value);
    template<class I, class = NeedIter<I>> void assign(I begin, I end);

    // Element access

    auto at(size_type)       -> T&;
    auto at(size_type) const -> const T&;

    auto operator[](size_type) noexcept       -> T&;
    auto operator[](size_type) const noexcept -> const T&;

    auto front() noexcept       -> T&;
    auto front() const noexcept -> const T&;

    auto back() noexcept       -> T&;
    auto back() const noexcept -> const T&;

    // Iterators

    auto begin() noexcept        -> iterator;
    auto begin() const noexcept  -> const_iterator;
    auto cbegin() const noexcept -> const_iterator;

    auto end() noexcept        -> iterator;
    auto end() const noexcept  -> const_iterator;
    auto cend() const noexcept -> const_iterator;

    auto rbegin() noexcept        -> reverse_iterator;
    auto rbegin() const noexcept  -> const_reverse_iterator;
    auto crbegin() const noexcept -> const_reverse_iterator;

    auto rend() noexcept        -> reverse_iterator;
    auto rend() const noexcept  -> const_reverse_iterator;
    auto crend() const noexcept -> const_reverse_iterator;

    // Size / capacity

    bool empty() const noexcept;
    auto size() const noexcept -> size_type;
    auto capacity() const noexcept -> size_type;

    void reserve_extra(size_type min_extra_capacity);
    void reserve(size_type min_capacity);
    void shrink_to_fit();

    // Modifiers

    auto push_front(const T&) -> T&;
    auto push_back(const T&)  -> T&;

    auto push_front(T&&) -> T&;
    auto push_back(T&&)  -> T&;

    template<class... A> auto emplace_front(A&&... args) -> T&;
    template<class... A> auto emplace_back(A&&... args)  -> T&;

    // FIXME: Consider emplace(const_iterator i, ...) -> j = unwrap(i.m_index);
    // if (j >= (m_size+1)/2) insert_near_back(j, ...); else insert_near_front(j, ...);

    void pop_front() noexcept;
    void pop_back() noexcept;

    void append(std::initializer_list<T>);
    void append(size_type size, const T& value);
    template<class I, class = NeedIter<I>> void append(I begin, I end);

    void clear() noexcept;
    void resize(size_type size);
    void resize(size_type size, const T& value);

    void swap(CircularBuffer&) noexcept;

    // Comparison

    template<class U> bool operator==(const CircularBuffer<U>&) const
        noexcept(noexcept(std::declval<T>() == std::declval<U>()));
    template<class U> bool operator!=(const CircularBuffer<U>&) const
        noexcept(noexcept(std::declval<T>() == std::declval<U>()));
    template<class U> bool operator<(const CircularBuffer<U>&) const
        noexcept(noexcept(std::declval<T>() < std::declval<U>()));
    template<class U> bool operator>(const CircularBuffer<U>&) const
        noexcept(noexcept(std::declval<T>() < std::declval<U>()));
    template<class U> bool operator<=(const CircularBuffer<U>&) const
        noexcept(noexcept(std::declval<T>() < std::declval<U>()));
    template<class U> bool operator>=(const CircularBuffer<U>&) const
        noexcept(noexcept(std::declval<T>() < std::declval<U>()));

private:
    std::unique_ptr<std::byte[]> m_memory;

    // Index of first element in allocated memory chunk.
    //
    // INVARIANT: m_allocated_size == 0 ? m_begin == 0 : m_begin < m_allocated_size
    //
    size_type m_begin = 0;

    // The number of elements within the allocated memory chunk, that are currently in use,
    // i.e., the logical size of the circular buffer.
    //
    size_type m_size = 0;

    // Number of elements of type T that will fit into the currently allocated memory chunk.
    //
    // Except when m_size is zero, m_allocated_size must be strictly greater than
    // m_size. This is required to ensure that the iterators returned by begin() and end()
    // are equal only when the buffer is empty.
    //
    // INVARIANT: m_size == 0 || m_allocated_size > m_size
    //
    size_type m_allocated_size = 0;

    void destroy(size_type offset = 0) noexcept;
    void realloc(size_type new_allocated_size);

    void do_append_1(std::size_t size);
    void do_append_1(std::size_t size, const T& value);

    template<class I> void do_append_2(I begin, I end);
    template<class I> void do_append_2(I begin, I end, std::input_iterator_tag);
    template<class I> void do_append_2(I begin, I end, std::random_access_iterator_tag);

    auto get_base() noexcept -> T*;

    // Assumption: index < m_allocated_size
    auto circular_inc(size_type index) noexcept -> size_type;
    auto circular_dec(size_type index) noexcept -> size_type;
    auto wrap(size_type index) noexcept   -> size_type;
    auto unwrap(size_type index) noexcept -> size_type;
};


template<class T> void swap(CircularBuffer<T>&, CircularBuffer<T>&) noexcept;








// Implementation


template<class T>
template<class U> class CircularBuffer<T>::Iter {
public:
    using difference_type   = std::ptrdiff_t;
    using value_type        = U;
    using pointer           = U*;
    using reference         = U&;
    using iterator_category = std::random_access_iterator_tag;

    Iter() noexcept
    {
    }

    template<class V> Iter(const Iter<V>& i) noexcept
    {
        operator=(i);
    }

    template<class V> auto operator=(const Iter<V>& i) noexcept -> Iter&
    {
        // Check constness convertability
        static_assert(std::is_convertible_v<V*, U*>);
        m_buffer = i.m_buffer;
        m_index = i.m_index;
        return *this;
    }

    auto operator*() const noexcept -> U&
    {
        T* base = m_buffer->get_base();
        return base[m_index];
    }

    auto operator->() const noexcept -> U*
    {
        return &operator*();
    }

    auto operator[](difference_type i) const noexcept -> U&
    {
        Iter j = *this;
        j += i;
        return *j;
    }

    auto operator++() noexcept -> Iter&
    {
        m_index = m_buffer->circular_inc(m_index);
        return *this;
    }

    auto operator--() noexcept -> Iter&
    {
        m_index = m_buffer->circular_dec(m_index);
        return *this;
    }

    auto operator++(int) noexcept -> Iter
    {
        size_type i = m_index;
        operator++();
        return { m_buffer, i };
    }

    auto operator--(int) noexcept -> Iter
    {
        size_type i = m_index;
        operator--();
        return { m_buffer, i };
    }

    auto operator+=(difference_type value) noexcept -> Iter&
    {
        // Care is needed to avoid unspecified arithmetic behaviour here. We can assume,
        // however, that if `i` is the unwrapped (logical) index of the element pointed to
        // by this iterator, then the mathematical value of i + value is representable in
        // `size_type` (otherwise the resulting iterator would escape the boundaries of the
        // buffer). We can therefore safely perform the addition in the unsigned domain of
        // unwrapped element indexes, and rely on two's complement representation for
        // negative values.
        size_type i = m_buffer->unwrap(m_index);
        i += size_type(value);
        m_index = m_buffer->wrap(i);
        return *this;
    }

    auto operator-=(difference_type value) noexcept -> Iter&
    {
        // Care is needed to avoid unspecified arithmetic behaviour here. See the comment in
        // the implementation of operator+=().
        size_type i = m_buffer->unwrap(m_index);
        i -= size_type(value);
        m_index = m_buffer->wrap(i);
        return *this;
    }

    auto operator+(difference_type value) const noexcept -> Iter
    {
        Iter i = *this;
        i += value;
        return i;
    }

    auto operator-(difference_type value) const noexcept -> Iter
    {
        Iter i = *this;
        i -= value;
        return i;
    }

    friend auto operator+(difference_type value, const Iter& i) noexcept -> Iter
    {
        Iter j = i;
        j += value;
        return j;
    }

    template<class V> auto operator-(const Iter<V>& i) const noexcept -> difference_type
    {
        ARCHON_ASSERT(m_buffer == i.m_buffer);
        size_type i_1 = m_buffer->unwrap(m_index);
        size_type i_2 = i.m_buffer->unwrap(i.m_index);
        return core::cast_from_twos_compl_a<difference_type>(size_type(i_1 - i_2));
    }

    template<class V> bool operator==(const Iter<V>& i) const noexcept
    {
        ARCHON_ASSERT(m_buffer == i.m_buffer);
        return (m_index == i.m_index);
    }

    template<class V> bool operator!=(const Iter<V>& i) const noexcept
    {
        return !operator==(i);
    }

    template<class V> bool operator<(const Iter<V>& i) const noexcept
    {
        ARCHON_ASSERT(m_buffer == i.m_buffer);
        size_type i_1 = m_buffer->unwrap(m_index);
        size_type i_2 = i.m_buffer->unwrap(i.m_index);
        return (i_1 < i_2);
    }

    template<class V> bool operator>(const Iter<V>& i) const noexcept
    {
        return (i < *this);
    }

    template<class V> bool operator<=(const Iter<V>& i) const noexcept
    {
        return !operator>(i);
    }

    template<class V> bool operator>=(const Iter<V>& i) const noexcept
    {
        return !operator<(i);
    }

private:
    CircularBuffer* m_buffer = nullptr;

    // Index of iterator position from beginning of allocated memory, i.e., from beginning
    // of m_buffer->get_base().
    size_type m_index = 0;

    Iter(CircularBuffer* buffer, size_type index) noexcept
        : m_buffer(buffer)
        , m_index(index)
    {
    }

    friend class CircularBuffer<T>;
    template<class> friend class Iter;
};


template<class T>
inline CircularBuffer<T>::CircularBuffer(const CircularBuffer& buffer)
    : CircularBuffer()
{
    append(buffer.begin(), buffer.end()); // Throws
}


template<class T>
inline CircularBuffer<T>::CircularBuffer(CircularBuffer&& buffer) noexcept
    : m_memory(std::move(buffer.m_memory))
    , m_begin(buffer.m_begin)
    , m_size(buffer.m_size)
    , m_allocated_size(buffer.m_allocated_size)
{
    buffer.m_begin          = 0;
    buffer.m_size           = 0;
    buffer.m_allocated_size = 0;
}


template<class T>
inline CircularBuffer<T>::CircularBuffer(std::initializer_list<T> list)
    : CircularBuffer()
{
    append(list.begin(), list.end()); // Throws
}


template<class T>
inline CircularBuffer<T>::CircularBuffer(size_type size)
    : CircularBuffer()
{
    do_append_1(size); // Throws
}


template<class T>
inline CircularBuffer<T>::CircularBuffer(size_type size, const T& value)
    : CircularBuffer()
{
    append(size, value); // Throws
}


template<class T>
template<class I, class> inline CircularBuffer<T>::CircularBuffer(I begin, I end)
    : CircularBuffer()
{
    append(begin, end); // Throws
}


template<class T>
inline CircularBuffer<T>::~CircularBuffer() noexcept
{
    destroy();
}


template<class T>
inline auto CircularBuffer<T>::operator=(const CircularBuffer& buffer) -> CircularBuffer&
{
    clear();
    append(buffer.begin(), buffer.end()); // Throws
    return *this;
}


template<class T>
inline auto CircularBuffer<T>::operator=(CircularBuffer&& buffer) noexcept -> CircularBuffer&
{
    destroy();
    m_memory         = std::move(buffer.m_memory);
    m_begin          = buffer.m_begin;
    m_size           = buffer.m_size;
    m_allocated_size = buffer.m_allocated_size;
    buffer.m_begin          = 0;
    buffer.m_size           = 0;
    buffer.m_allocated_size = 0;
    return *this;
}


template<class T>
inline auto CircularBuffer<T>::operator=(std::initializer_list<T> list) -> CircularBuffer&
{
    clear();
    append(list.begin(), list.end()); // Throws
    return *this;
}


template<class T>
inline void CircularBuffer<T>::assign(std::initializer_list<T> list)
{
    clear();
    append(list.begin(), list.end()); // Throws
}


template<class T>
inline void CircularBuffer<T>::assign(size_type size, const T& value)
{
    clear();
    append(size, value); // Throws
}


template<class T>
template<class I, class> inline void CircularBuffer<T>::assign(I begin, I end)
{
    clear();
    append(begin, end); // Throws
}


template<class T>
inline auto CircularBuffer<T>::at(size_type i) -> T&
{
    if (ARCHON_LIKELY(i < m_size))
        return operator[](i);
    throw std::out_of_range("Index");
}


template<class T>
inline auto CircularBuffer<T>::at(size_type i) const -> const T&
{
    return const_cast<CircularBuffer*>(this)->at(i); // Throws
}


template<class T>
inline auto CircularBuffer<T>::operator[](size_type i) noexcept -> T&
{
    ARCHON_ASSERT(i < m_size);
    T* base = get_base();
    size_type j = wrap(i);
    return base[j];
}


template<class T>
inline auto CircularBuffer<T>::operator[](size_type i) const noexcept -> const T&
{
    return const_cast<CircularBuffer*>(this)->operator[](i);
}


template<class T>
inline auto CircularBuffer<T>::front() noexcept -> T&
{
    return operator[](0);
}


template<class T>
inline auto CircularBuffer<T>::front() const noexcept -> const T&
{
    return operator[](0);
}


template<class T>
inline auto CircularBuffer<T>::back() noexcept -> T&
{
    return operator[](m_size - 1);
}


template<class T>
inline auto CircularBuffer<T>::back() const noexcept -> const T&
{
    return operator[](m_size - 1);
}


template<class T>
inline auto CircularBuffer<T>::begin() noexcept -> iterator
{
    return { this, m_begin };
}


template<class T>
inline auto CircularBuffer<T>::begin() const noexcept -> const_iterator
{
    return const_cast<CircularBuffer*>(this)->begin();
}


template<class T>
inline auto CircularBuffer<T>::cbegin() const noexcept -> const_iterator
{
    return begin();
}


template<class T>
inline auto CircularBuffer<T>::end() noexcept -> iterator
{
    size_type i = wrap(m_size);
    return { this, i };
}


template<class T>
inline auto CircularBuffer<T>::end() const noexcept -> const_iterator
{
    return const_cast<CircularBuffer*>(this)->end();
}


template<class T>
inline auto CircularBuffer<T>::cend() const noexcept -> const_iterator
{
    return end();
}


template<class T>
inline auto CircularBuffer<T>::rbegin() noexcept -> reverse_iterator
{
    return std::reverse_iterator<iterator>(end());
}


template<class T>
inline auto CircularBuffer<T>::rbegin() const noexcept -> const_reverse_iterator
{
    return const_cast<CircularBuffer*>(this)->rbegin();
}


template<class T>
inline auto CircularBuffer<T>::crbegin() const noexcept -> const_reverse_iterator
{
    return rbegin();
}


template<class T>
inline auto CircularBuffer<T>::rend() noexcept -> reverse_iterator
{
    return std::reverse_iterator<iterator>(begin());
}


template<class T>
inline auto CircularBuffer<T>::rend() const noexcept -> const_reverse_iterator
{
    return const_cast<CircularBuffer*>(this)->rend();
}


template<class T>
inline auto CircularBuffer<T>::crend() const noexcept -> const_reverse_iterator
{
    return rend();
}


template<class T>
inline bool CircularBuffer<T>::empty() const noexcept
{
    return (m_size == 0);
}


template<class T>
inline auto CircularBuffer<T>::size() const noexcept -> size_type
{
    return m_size;
}


template<class T>
void CircularBuffer<T>::reserve_extra(size_type min_extra_capacity)
{
    size_type min_capacity = m_size;
    if (ARCHON_LIKELY(core::try_int_add(min_capacity, min_extra_capacity))) {
        reserve(min_capacity); // Throws
        return;
    }
    throw std::length_error("Buffer size");
}


template<class T>
void CircularBuffer<T>::reserve(size_type min_capacity)
{
    if (min_capacity == 0)
        return;

    // An extra element of capacity is needed such that the end iterator can always point
    // one beyond the last element without becomeing equal to an iterator to the first
    // element.
    size_type min_allocated_size = min_capacity;
    if (ARCHON_LIKELY(core::try_int_add(min_allocated_size, 1))) {
        if (ARCHON_LIKELY(min_allocated_size <= m_allocated_size))
            return;

        size_type new_allocated_size = suggest_new_buffer_size(m_allocated_size, min_allocated_size);
        realloc(new_allocated_size); // Throws
        return;
    }
    throw std::length_error("Buffer size");
}


template<class T>
inline void CircularBuffer<T>::shrink_to_fit()
{
    if (m_size > 0) {
        // An extra element of capacity is needed such that the end iterator can always
        // point one beyond the last element without becomeing equal to an iterator to the
        // first element.
        size_type new_allocated_size = m_size + 1;
        if (new_allocated_size < m_allocated_size)
            realloc(new_allocated_size); // Throws
    }
    else {
        m_memory.reset();
        m_begin = 0;
        m_allocated_size = 0;
    }
}


template<class T>
inline auto CircularBuffer<T>::capacity() const noexcept -> size_type
{
    return (m_allocated_size > 0 ? m_allocated_size - 1 : 0);
}


template<class T>
inline auto CircularBuffer<T>::push_front(const T& value) -> T&
{
    return emplace_front(value); // Throws
}


template<class T>
inline auto CircularBuffer<T>::push_back(const T& value) -> T&
{
    return emplace_back(value); // Throws
}


template<class T>
inline auto CircularBuffer<T>::push_front(T&& value) -> T&
{
    return emplace_front(std::move(value)); // Throws
}


template<class T>
inline auto CircularBuffer<T>::push_back(T&& value) -> T&
{
    return emplace_back(std::move(value)); // Throws
}


template<class T>
template<class... A> inline auto CircularBuffer<T>::emplace_front(A&&... args) -> T&
{
    size_type new_size = m_size + 1;
    reserve(new_size); // Throws
    ARCHON_ASSERT(m_allocated_size > 0);
    T* base = get_base();
    size_type i = circular_dec(m_begin);
    core::uninit_create(base + i, std::forward<A>(args)...); // Throws
    m_begin = i;
    m_size = new_size;
    return base[i];
}


template<class T>
template<class... A> inline auto CircularBuffer<T>::emplace_back(A&&... args) -> T&
{
    size_type new_size = m_size + 1;
    reserve(new_size); // Throws
    ARCHON_ASSERT(m_allocated_size > 0);
    T* base = get_base();
    size_type i = wrap(m_size);
    core::uninit_create(base + i, std::forward<A>(args)...); // Throws
    m_size = new_size;
    return base[i];
}


template<class T>
inline void CircularBuffer<T>::pop_front() noexcept
{
    ARCHON_ASSERT(m_size > 0);
    T* base = get_base();
    size_type i = m_begin;
    core::uninit_destroy(base + i, 1);
    m_begin = circular_inc(m_begin);
    --m_size;
}


template<class T>
inline void CircularBuffer<T>::pop_back() noexcept
{
    ARCHON_ASSERT(m_size > 0);
    T* base = get_base();
    size_type new_size = m_size - 1;
    size_type i = wrap(new_size);
    core::uninit_destroy(base + i, 1);
    m_size = new_size;
}


template<class T>
inline void CircularBuffer<T>::append(std::initializer_list<T> list)
{
    do_append_2(list.begin(), list.end()); // Throws
}


template<class T>
inline void CircularBuffer<T>::append(size_type size, const T& value)
{
    do_append_1(size, value); // Throws
}


template<class T>
template<class I, class> inline void CircularBuffer<T>::append(I begin, I end)
{
    do_append_2(begin, end); // Throws
}


template<class T>
inline void CircularBuffer<T>::clear() noexcept
{
    destroy();
    m_begin = 0;
    m_size = 0;
}


template<class T>
inline void CircularBuffer<T>::resize(size_type size)
{
    if (size <= m_size) {
        size_type offset = size;
        destroy(offset);
        m_size = size;
        return;
    }
    do_append_1(size - m_size); // Throws
}


template<class T>
inline void CircularBuffer<T>::resize(size_type size, const T& value)
{
    if (size <= m_size) {
        size_type offset = size;
        destroy(offset);
        m_size = size;
        return;
    }
    do_append_1(size - m_size, value); // Throws
}


template<class T>
inline void CircularBuffer<T>::swap(CircularBuffer& buffer) noexcept
{
    std::swap(m_memory,         buffer.m_memory);
    std::swap(m_begin,          buffer.m_begin);
    std::swap(m_size,           buffer.m_size);
    std::swap(m_allocated_size, buffer.m_allocated_size);
}


template<class T>
template<class U>
inline bool CircularBuffer<T>::operator==(const CircularBuffer<U>& buffer) const
    noexcept(noexcept(std::declval<T>() == std::declval<U>()))
{
    return std::equal(begin(), end(), buffer.begin(), buffer.end()); // Throws
}


template<class T>
template<class U>
inline bool CircularBuffer<T>::operator!=(const CircularBuffer<U>& buffer) const
    noexcept(noexcept(std::declval<T>() == std::declval<U>()))
{
    return !operator==(buffer); // Throws
}


template<class T>
template<class U>
inline bool CircularBuffer<T>::operator<(const CircularBuffer<U>& buffer) const
    noexcept(noexcept(std::declval<T>() < std::declval<U>()))
{
    return std::lexicographical_compare(begin(), end(), buffer.begin(), buffer.end()); // Throws
}


template<class T>
template<class U>
inline bool CircularBuffer<T>::operator>(const CircularBuffer<U>& buffer) const
    noexcept(noexcept(std::declval<T>() < std::declval<U>()))
{
    return (buffer < *this); // Throws
}


template<class T>
template<class U>
inline bool CircularBuffer<T>::operator<=(const CircularBuffer<U>& buffer) const
    noexcept(noexcept(std::declval<T>() < std::declval<U>()))
{
    return !operator>(buffer); // Throws
}


template<class T>
template<class U>
inline bool CircularBuffer<T>::operator>=(const CircularBuffer<U>& buffer) const
    noexcept(noexcept(std::declval<T>() < std::declval<U>()))
{
    return !operator<(buffer); // Throws
}


template<class T>
inline void CircularBuffer<T>::destroy(size_type offset) noexcept
{
    ARCHON_ASSERT(offset <= m_size);
    T* base = get_base();
    std::size_t size = m_size - offset;
    std::size_t offset_2 = wrap(offset);
    std::size_t top = m_allocated_size - offset_2;
    if (size > top) {
        core::uninit_destroy(base, size - top);
        size = top;
    }
    core::uninit_destroy(base + offset_2, size);
}


template<class T>
void CircularBuffer<T>::realloc(size_type new_allocated_size)
{
    ARCHON_ASSERT(new_allocated_size > 1);
    ARCHON_ASSERT(new_allocated_size > m_size);
    std::size_t num_bytes = new_allocated_size;
    if (ARCHON_UNLIKELY(!core::try_int_mul(num_bytes, sizeof (T))))
        throw std::length_error("Buffer capacity");
    std::unique_ptr<std::byte[]> new_memory = std::make_unique<std::byte[]>(num_bytes); // Throws
    T* base = get_base();
    T* new_base = reinterpret_cast<T*>(new_memory.get());
    std::size_t top = std::size_t(m_allocated_size - m_begin);
    if (m_size <= top) {
        core::uninit_safe_move_or_copy(base + m_begin, m_size, new_base); // Throws
    }
    else {
        core::uninit_safe_move_or_copy(base + m_begin, top, new_base); // Throws
        try {
            core::uninit_safe_move_or_copy(base, m_size - top, new_base + top); // Throws
        }
        catch (...) {
            core::uninit_destroy(new_base, top);
            throw;
        }
    }
    destroy();
    m_memory = std::move(new_memory);
    m_begin = 0;
    m_allocated_size = new_allocated_size;
}


template<class T>
inline void CircularBuffer<T>::do_append_1(std::size_t size)
{
    reserve_extra(size); // Throws
    T* base = get_base();
    std::size_t offset = wrap(m_size);
    std::size_t top = m_allocated_size - offset;
    if (size <= top) {
        core::uninit_safe_fill(size, base + offset); // Throws
    }
    else {
        core::uninit_safe_fill(top, base + offset); // Throws
        try {
            core::uninit_safe_fill(size - top, base); // Throws
        }
        catch (...) {
            core::uninit_destroy(base + offset, top);
            throw;
        }
    }
    m_size += size;
}


template<class T>
inline void CircularBuffer<T>::do_append_1(std::size_t size, const T& value)
{
    reserve_extra(size); // Throws
    T* base = get_base();
    std::size_t offset = wrap(m_size);
    std::size_t top = m_allocated_size - offset;
    if (size <= top) {
        core::uninit_safe_fill(size, value, base + offset); // Throws
    }
    else {
        core::uninit_safe_fill(top, value, base + offset); // Throws
        try {
            core::uninit_safe_fill(size - top, value, base); // Throws
        }
        catch (...) {
            core::uninit_destroy(base + offset, top);
            throw;
        }
    }
    m_size += size;
}


template<class T>
template<class I> inline void CircularBuffer<T>::do_append_2(I begin, I end)
{
    using iterator_category = typename std::iterator_traits<I>::iterator_category;
    do_append_2(begin, end, iterator_category()); // Throws
}


template<class T>
template<class I> void CircularBuffer<T>::do_append_2(I begin, I end, std::input_iterator_tag)
{
    for (I i = begin; i != end; ++i)
        push_back(*i); // Throws
}


template<class T>
template<class I> void CircularBuffer<T>::do_append_2(I begin, I end, std::random_access_iterator_tag)
{
    size_type size = std::size_t(end - begin);
    reserve_extra(size); // Throws
    T* base = get_base();
    std::size_t offset = wrap(m_size);
    std::size_t top = m_allocated_size - offset;
    if (size <= top) {
        core::uninit_safe_copy(begin, end, base + offset); // Throws
    }
    else {
        core::uninit_safe_copy(begin, begin + top, base + offset); // Throws
        try {
            core::uninit_safe_copy(begin + top, end, base); // Throws
        }
        catch (...) {
            core::uninit_destroy(base + offset, top);
            throw;
        }
    }
    m_size += size;
}


template<class T>
inline auto CircularBuffer<T>::get_base() noexcept -> T*
{
    return reinterpret_cast<T*>(m_memory.get());
}


template<class T>
inline auto CircularBuffer<T>::circular_inc(size_type index) noexcept -> size_type
{
    size_type index_2 = index + 1;
    if (ARCHON_LIKELY(index_2 < m_allocated_size))
        return index_2;
    return 0;
}


template<class T>
inline auto CircularBuffer<T>::circular_dec(size_type index) noexcept -> size_type
{
    if (ARCHON_LIKELY(index > 0))
        return index - 1;
    return m_allocated_size - 1;
}


template<class T>
inline auto CircularBuffer<T>::wrap(size_type index) noexcept -> size_type
{
    size_type top = m_allocated_size - m_begin;
    if (index < top)
        return m_begin + index;
    return index - top;
}


template<class T>
inline auto CircularBuffer<T>::unwrap(size_type index) noexcept -> size_type
{
    if (index >= m_begin)
        return index - m_begin;
    return m_allocated_size - (m_begin - index);
}


template<class T> inline void swap(CircularBuffer<T>& a, CircularBuffer<T>& b) noexcept
{
    a.swap(b);
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_CIRCULAR_BUFFER_HPP
