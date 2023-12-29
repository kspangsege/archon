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

#ifndef ARCHON_X_CORE_X_DEQUE_HPP
#define ARCHON_X_CORE_X_DEQUE_HPP

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
#include <archon/core/type_traits.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/memory.hpp>


namespace archon::core {


/// \brief A double ended queue backed by a single contiguous memory buffer.
///
/// This container is similar to std::deque in that it offers efficient element insertion
/// and removal at both ends. Insertion at either end occurs in amortized constant
/// time. Removal at either end occurs in constant time.
///
/// As opposed to std::deque, this container allows for reservation of buffer space, such
/// that value insertion can be guaranteed to not reallocate buffer memory, and to not
/// throw. More specifically, a single insert operation, that inserts zero or more values at
/// either end, is guaranteed to not reallocate buffer memory if the prior capacity
/// (`capacity()`) is greater than, or equal to the prior size (`size()`) plus the number of
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
/// When the first element is removed (`pop_front()`), iterators pointing to the removed
/// element will be invalidated. All other iterators, including "end iterators" (`end()`),
/// will remain valid.
///
/// When the last element is removed (`pop_back()`), iterators pointing to the removed
/// element will become "end iterators" (`end()`), and "end iterators" will be
/// invalidated. All other iterators will remain valid.
///
/// When an element is inserted at the front (`push_front()`), and the prior capacity
/// (`capacity()`) is strictly greater than the prior size (`size()`), all iterators remain
/// valid.
///
/// When an element is inserted at the back (`push_back()`), and the prior capacity
/// (`capacity()`) is strictly greater than the prior size (`size()`), "end iterators"
/// (`end()`) become iterators to the inserted element, and all other iterators remain
/// valid.
///
/// Operations `pop_front()`, `pop_back()`, and `clear()` are guaranteed to leave the
/// capacity unchanged.
///
/// Iterators are of the "random access" kind (std::random_access_iterator_tag).
///
/// Erase operations (`erase()`) is only available when \p T has a non-throwing
/// move-constructor.
///
template<class T> class Deque {
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

    Deque() noexcept = default;
    Deque(const Deque&);
    Deque(Deque&&) noexcept;
    Deque(std::initializer_list<T>);
    explicit Deque(size_type size);
    Deque(size_type size, const T& value);
    template<class I, class = core::NeedIter<I>> Deque(I begin, I end);
    ~Deque() noexcept;

    auto operator=(const Deque&) -> Deque&;
    auto operator=(Deque&&) noexcept -> Deque&;
    auto operator=(std::initializer_list<T>) -> Deque&;

    void assign(std::initializer_list<T>);
    void assign(size_type size, const T& value);
    template<class I, class = core::NeedIter<I>> void assign(I begin, I end);

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
    template<class I, class = core::NeedIter<I>> void append(I begin, I end);

    auto erase(const_iterator) noexcept -> iterator;
    auto erase(const_iterator begin, const_iterator end) noexcept -> iterator;

    void clear() noexcept;
    void resize(size_type size);
    void resize(size_type size, const T& value);

    void swap(Deque&) noexcept;

    // Comparison

    template<class U> bool operator==(const Deque<U>&) const
        noexcept(noexcept(std::declval<T>() == std::declval<U>()));
    template<class U> bool operator!=(const Deque<U>&) const
        noexcept(noexcept(std::declval<T>() == std::declval<U>()));
    template<class U> bool operator<(const Deque<U>&) const
        noexcept(noexcept(std::declval<T>() < std::declval<U>()));
    template<class U> bool operator>(const Deque<U>&) const
        noexcept(noexcept(std::declval<T>() < std::declval<U>()));
    template<class U> bool operator<=(const Deque<U>&) const
        noexcept(noexcept(std::declval<T>() < std::declval<U>()));
    template<class U> bool operator>=(const Deque<U>&) const
        noexcept(noexcept(std::declval<T>() < std::declval<U>()));

private:
    std::unique_ptr<std::byte[]> m_memory;

    // Index of first element in allocated memory chunk.
    //
    // INVARIANT: m_allocated_size == 0 ? m_offset == 0 : m_offset < m_allocated_size
    //
    size_type m_offset = 0;

    // The number of elements within the allocated memory chunk, that are currently in use,
    // i.e., the logical size of the deque.
    //
    size_type m_size = 0;

    // Number of elements of type T that will fit into the currently allocated memory chunk.
    //
    // Except when m_size is zero, m_allocated_size must be strictly greater than
    // m_size. This is required to ensure that the iterators returned by begin() and end()
    // are equal only when the deque is empty.
    //
    // INVARIANT: m_size == 0 || m_allocated_size > m_size
    //
    size_type m_allocated_size = 0;

    void destroy(size_type offset = 0) noexcept;
    void realloc(size_type new_allocated_size);

    void do_append_1(size_type size);
    void do_append_1(size_type size, const T& value);

    template<class I> void do_append_2(I begin, I end);
    template<class I> void do_append_2(I begin, I end, std::input_iterator_tag);
    template<class I> void do_append_2(I begin, I end, std::random_access_iterator_tag);

    void do_erase(size_type begin, size_type end) noexcept;
    void erase_near_front(size_type begin, size_type end) noexcept;
    void erase_near_back(size_type begin, size_type end) noexcept;

    auto get_base() noexcept -> T*;

    // Assumption: index < m_allocated_size
    auto circular_inc(size_type index) noexcept -> size_type;
    auto circular_dec(size_type index) noexcept -> size_type;
    auto wrap(size_type index) noexcept   -> size_type;
    auto unwrap(size_type index) noexcept -> size_type;
};


template<class T> void swap(Deque<T>&, Deque<T>&) noexcept;








// Implementation


template<class T>
template<class U> class Deque<T>::Iter {
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
        // Check constness convertibility
        static_assert(std::is_convertible_v<V*, U*>);
        m_deque = i.m_deque;
        m_index = i.m_index;
        return *this;
    }

    auto operator*() const noexcept -> U&
    {
        T* base = m_deque->get_base();
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
        m_index = m_deque->circular_inc(m_index);
        return *this;
    }

    auto operator--() noexcept -> Iter&
    {
        m_index = m_deque->circular_dec(m_index);
        return *this;
    }

    auto operator++(int) noexcept -> Iter
    {
        size_type i = m_index;
        operator++();
        return { m_deque, i };
    }

    auto operator--(int) noexcept -> Iter
    {
        size_type i = m_index;
        operator--();
        return { m_deque, i };
    }

    auto operator+=(difference_type value) noexcept -> Iter&
    {
        // Care is needed to avoid unspecified arithmetic behavior here. We can assume that
        // if `i` is the unwrapped (logical) index of the element pointed to by this
        // iterator, then the mathematical value of `i + value` is representable in
        // `size_type` (since `i + value` must be less than, or equal to the size of the
        // deque, which must be representable in `size_type`). We can therefore safely
        // perform the addition in the unsigned domain of unwrapped element indexes, and
        // rely on two's complement representation for negative values.
        size_type i = m_deque->unwrap(m_index);
        i += size_type(value);
        m_index = m_deque->wrap(i);
        return *this;
    }

    auto operator-=(difference_type value) noexcept -> Iter&
    {
        // Care is needed to avoid unspecified arithmetic behavior here. See the comment in
        // the implementation of operator+=().
        size_type i = m_deque->unwrap(m_index);
        i -= size_type(value);
        m_index = m_deque->wrap(i);
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
        ARCHON_ASSERT(m_deque == i.m_deque);
        size_type i_1 = m_deque->unwrap(m_index);
        size_type i_2 = i.m_deque->unwrap(i.m_index);
        return core::cast_from_twos_compl_a<difference_type>(size_type(i_1 - i_2));
    }

    template<class V> bool operator==(const Iter<V>& i) const noexcept
    {
        ARCHON_ASSERT(m_deque == i.m_deque);
        return (m_index == i.m_index);
    }

    template<class V> bool operator!=(const Iter<V>& i) const noexcept
    {
        return !operator==(i);
    }

    template<class V> bool operator<(const Iter<V>& i) const noexcept
    {
        ARCHON_ASSERT(m_deque == i.m_deque);
        size_type i_1 = m_deque->unwrap(m_index);
        size_type i_2 = i.m_deque->unwrap(i.m_index);
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
    Deque* m_deque = nullptr;

    // Index of iterator position from beginning of allocated memory, i.e., from beginning
    // of m_deque->get_base().
    size_type m_index = 0;

    Iter(Deque* deque, size_type index) noexcept
        : m_deque(deque)
        , m_index(index)
    {
    }

    friend class Deque<T>;
    template<class> friend class Iter;
};


template<class T>
inline Deque<T>::Deque(const Deque& other)
    : Deque()
{
    append(other.begin(), other.end()); // Throws
}


template<class T>
inline Deque<T>::Deque(Deque&& other) noexcept
    : m_memory(std::move(other.m_memory))
    , m_offset(other.m_offset)
    , m_size(other.m_size)
    , m_allocated_size(other.m_allocated_size)
{
    other.m_offset         = 0;
    other.m_size           = 0;
    other.m_allocated_size = 0;
}


template<class T>
inline Deque<T>::Deque(std::initializer_list<T> list)
    : Deque()
{
    append(list.begin(), list.end()); // Throws
}


template<class T>
inline Deque<T>::Deque(size_type size)
    : Deque()
{
    do_append_1(size); // Throws
}


template<class T>
inline Deque<T>::Deque(size_type size, const T& value)
    : Deque()
{
    append(size, value); // Throws
}


template<class T>
template<class I, class> inline Deque<T>::Deque(I begin, I end)
    : Deque()
{
    append(begin, end); // Throws
}


template<class T>
inline Deque<T>::~Deque() noexcept
{
    destroy();
}


template<class T>
inline auto Deque<T>::operator=(const Deque& other) -> Deque&
{
    clear();
    append(other.begin(), other.end()); // Throws
    return *this;
}


template<class T>
inline auto Deque<T>::operator=(Deque&& other) noexcept -> Deque&
{
    destroy();
    m_memory         = std::move(other.m_memory);
    m_offset         = other.m_offset;
    m_size           = other.m_size;
    m_allocated_size = other.m_allocated_size;
    other.m_offset         = 0;
    other.m_size           = 0;
    other.m_allocated_size = 0;
    return *this;
}


template<class T>
inline auto Deque<T>::operator=(std::initializer_list<T> list) -> Deque&
{
    clear();
    append(list.begin(), list.end()); // Throws
    return *this;
}


template<class T>
inline void Deque<T>::assign(std::initializer_list<T> list)
{
    clear();
    append(list.begin(), list.end()); // Throws
}


template<class T>
inline void Deque<T>::assign(size_type size, const T& value)
{
    clear();
    append(size, value); // Throws
}


template<class T>
template<class I, class> inline void Deque<T>::assign(I begin, I end)
{
    clear();
    append(begin, end); // Throws
}


template<class T>
inline auto Deque<T>::at(size_type i) -> T&
{
    if (ARCHON_LIKELY(i < m_size))
        return operator[](i);
    throw std::out_of_range("Index");
}


template<class T>
inline auto Deque<T>::at(size_type i) const -> const T&
{
    return const_cast<Deque*>(this)->at(i); // Throws
}


template<class T>
inline auto Deque<T>::operator[](size_type i) noexcept -> T&
{
    ARCHON_ASSERT(i < m_size);
    T* base = get_base();
    size_type j = wrap(i);
    return base[j];
}


template<class T>
inline auto Deque<T>::operator[](size_type i) const noexcept -> const T&
{
    return const_cast<Deque*>(this)->operator[](i);
}


template<class T>
inline auto Deque<T>::front() noexcept -> T&
{
    return operator[](0);
}


template<class T>
inline auto Deque<T>::front() const noexcept -> const T&
{
    return operator[](0);
}


template<class T>
inline auto Deque<T>::back() noexcept -> T&
{
    return operator[](m_size - 1);
}


template<class T>
inline auto Deque<T>::back() const noexcept -> const T&
{
    return operator[](m_size - 1);
}


template<class T>
inline auto Deque<T>::begin() noexcept -> iterator
{
    return { this, m_offset };
}


template<class T>
inline auto Deque<T>::begin() const noexcept -> const_iterator
{
    return const_cast<Deque*>(this)->begin();
}


template<class T>
inline auto Deque<T>::cbegin() const noexcept -> const_iterator
{
    return begin();
}


template<class T>
inline auto Deque<T>::end() noexcept -> iterator
{
    size_type i = wrap(m_size);
    return { this, i };
}


template<class T>
inline auto Deque<T>::end() const noexcept -> const_iterator
{
    return const_cast<Deque*>(this)->end();
}


template<class T>
inline auto Deque<T>::cend() const noexcept -> const_iterator
{
    return end();
}


template<class T>
inline auto Deque<T>::rbegin() noexcept -> reverse_iterator
{
    return std::reverse_iterator<iterator>(end());
}


template<class T>
inline auto Deque<T>::rbegin() const noexcept -> const_reverse_iterator
{
    return const_cast<Deque*>(this)->rbegin();
}


template<class T>
inline auto Deque<T>::crbegin() const noexcept -> const_reverse_iterator
{
    return rbegin();
}


template<class T>
inline auto Deque<T>::rend() noexcept -> reverse_iterator
{
    return std::reverse_iterator<iterator>(begin());
}


template<class T>
inline auto Deque<T>::rend() const noexcept -> const_reverse_iterator
{
    return const_cast<Deque*>(this)->rend();
}


template<class T>
inline auto Deque<T>::crend() const noexcept -> const_reverse_iterator
{
    return rend();
}


template<class T>
inline bool Deque<T>::empty() const noexcept
{
    return (m_size == 0);
}


template<class T>
inline auto Deque<T>::size() const noexcept -> size_type
{
    return m_size;
}


template<class T>
void Deque<T>::reserve_extra(size_type min_extra_capacity)
{
    size_type min_capacity = m_size;
    if (ARCHON_LIKELY(core::try_int_add(min_capacity, min_extra_capacity))) {
        reserve(min_capacity); // Throws
        return;
    }
    throw std::length_error("Buffer size");
}


template<class T>
void Deque<T>::reserve(size_type min_capacity)
{
    if (min_capacity == 0)
        return;

    // An extra element of capacity is needed such that the end iterator can always point
    // one beyond the last element without becoming equal to an iterator to the first
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
inline void Deque<T>::shrink_to_fit()
{
    if (m_size > 0) {
        // An extra element of capacity is needed such that the end iterator can always
        // point one beyond the last element without becoming equal to an iterator to the
        // first element.
        size_type new_allocated_size = m_size + 1;
        if (new_allocated_size < m_allocated_size)
            realloc(new_allocated_size); // Throws
    }
    else {
        m_memory.reset();
        m_offset = 0;
        m_allocated_size = 0;
    }
}


template<class T>
inline auto Deque<T>::capacity() const noexcept -> size_type
{
    return (m_allocated_size > 0 ? m_allocated_size - 1 : 0);
}


template<class T>
inline auto Deque<T>::push_front(const T& value) -> T&
{
    return emplace_front(value); // Throws
}


template<class T>
inline auto Deque<T>::push_back(const T& value) -> T&
{
    return emplace_back(value); // Throws
}


template<class T>
inline auto Deque<T>::push_front(T&& value) -> T&
{
    return emplace_front(std::move(value)); // Throws
}


template<class T>
inline auto Deque<T>::push_back(T&& value) -> T&
{
    return emplace_back(std::move(value)); // Throws
}


template<class T>
template<class... A> inline auto Deque<T>::emplace_front(A&&... args) -> T&
{
    size_type new_size = m_size + 1;
    reserve(new_size); // Throws
    ARCHON_ASSERT(m_allocated_size > 0);
    T* base = get_base();
    size_type i = circular_dec(m_offset);
    core::uninit_create(base + i, std::forward<A>(args)...); // Throws
    m_offset = i;
    m_size = new_size;
    return base[i];
}


template<class T>
template<class... A> inline auto Deque<T>::emplace_back(A&&... args) -> T&
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
inline void Deque<T>::pop_front() noexcept
{
    ARCHON_ASSERT(m_size > 0);
    T* base = get_base();
    size_type i = m_offset;
    core::uninit_destroy(base + i, 1);
    m_offset = circular_inc(m_offset);
    --m_size;
}


template<class T>
inline void Deque<T>::pop_back() noexcept
{
    ARCHON_ASSERT(m_size > 0);
    T* base = get_base();
    size_type new_size = m_size - 1;
    size_type i = wrap(new_size);
    core::uninit_destroy(base + i, 1);
    m_size = new_size;
}


template<class T>
inline void Deque<T>::append(std::initializer_list<T> list)
{
    do_append_2(list.begin(), list.end()); // Throws
}


template<class T>
inline void Deque<T>::append(size_type size, const T& value)
{
    do_append_1(size, value); // Throws
}


template<class T>
template<class I, class> inline void Deque<T>::append(I begin, I end)
{
    do_append_2(begin, end); // Throws
}


template<class T>
inline auto Deque<T>::erase(const_iterator i) noexcept -> iterator
{
    size_type begin = unwrap(i.m_index);
    size_type end   = begin + 1;
    ARCHON_ASSERT(end <= m_size);
    do_erase(begin, end);
    return { this, wrap(begin) };
}


template<class T>
inline auto Deque<T>::erase(const_iterator begin, const_iterator end) noexcept -> iterator
{
    size_type begin_2 = unwrap(begin.m_index);
    size_type end_2   = unwrap(end.m_index);
    ARCHON_ASSERT(begin_2 <= end_2);
    ARCHON_ASSERT(end_2 <= m_size);
    if (ARCHON_LIKELY(begin_2 != end_2))
        do_erase(begin_2, end_2);
    return { this, wrap(begin_2) };
}


template<class T>
inline void Deque<T>::clear() noexcept
{
    destroy();
    m_offset = 0;
    m_size = 0;
}


template<class T>
inline void Deque<T>::resize(size_type size)
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
inline void Deque<T>::resize(size_type size, const T& value)
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
inline void Deque<T>::swap(Deque& other) noexcept
{
    std::swap(m_memory,         other.m_memory);
    std::swap(m_offset,         other.m_offset);
    std::swap(m_size,           other.m_size);
    std::swap(m_allocated_size, other.m_allocated_size);
}


template<class T>
template<class U> inline bool Deque<T>::operator==(const Deque<U>& other) const
    noexcept(noexcept(std::declval<T>() == std::declval<U>()))
{
    return std::equal(begin(), end(), other.begin(), other.end()); // Throws
}


template<class T>
template<class U> inline bool Deque<T>::operator!=(const Deque<U>& other) const
    noexcept(noexcept(std::declval<T>() == std::declval<U>()))
{
    return !operator==(other); // Throws
}


template<class T>
template<class U> inline bool Deque<T>::operator<(const Deque<U>& other) const
    noexcept(noexcept(std::declval<T>() < std::declval<U>()))
{
    return std::lexicographical_compare(begin(), end(), other.begin(), other.end()); // Throws
}


template<class T>
template<class U> inline bool Deque<T>::operator>(const Deque<U>& other) const
    noexcept(noexcept(std::declval<T>() < std::declval<U>()))
{
    return (other < *this); // Throws
}


template<class T>
template<class U> inline bool Deque<T>::operator<=(const Deque<U>& other) const
    noexcept(noexcept(std::declval<T>() < std::declval<U>()))
{
    return !operator>(other); // Throws
}


template<class T>
template<class U> inline bool Deque<T>::operator>=(const Deque<U>& other) const
    noexcept(noexcept(std::declval<T>() < std::declval<U>()))
{
    return !operator<(other); // Throws
}


template<class T>
inline void Deque<T>::destroy(size_type offset) noexcept
{
    ARCHON_ASSERT(offset <= m_size);
    T* base = get_base();
    size_type size = m_size - offset;
    size_type offset_2 = wrap(offset);
    size_type top = size_type(m_allocated_size - offset_2);
    if (size > top) {
        core::uninit_destroy(base, size_type(size - top));
        size = top;
    }
    core::uninit_destroy(base + offset_2, size);
}


template<class T>
void Deque<T>::realloc(size_type new_allocated_size)
{
    ARCHON_ASSERT(new_allocated_size > 1);
    ARCHON_ASSERT(new_allocated_size > m_size);
    size_type num_bytes = new_allocated_size;
    if (ARCHON_UNLIKELY(!core::try_int_mul(num_bytes, sizeof (T))))
        throw std::length_error("Buffer size");
    std::unique_ptr<std::byte[]> new_memory = std::make_unique<std::byte[]>(num_bytes); // Throws
    T* base = get_base();
    T* new_base = reinterpret_cast<T*>(new_memory.get());
    size_type top = size_type(m_allocated_size - m_offset);
    if (m_size <= top) {
        core::uninit_safe_move(base + m_offset, m_size, new_base); // Throws
    }
    else {
        core::uninit_safe_move_a(base + m_offset, top, new_base); // Throws
        try {
            core::uninit_safe_move(base, m_size - top, new_base + top); // Throws
        }
        catch (...) {
            core::uninit_destroy(new_base, top);
            throw;
        }
        core::uninit_destroy(base + m_offset, top);
    }
    m_memory = std::move(new_memory);
    m_offset = 0;
    m_allocated_size = new_allocated_size;
}


template<class T>
inline void Deque<T>::do_append_1(size_type size)
{
    reserve_extra(size); // Throws
    T* base = get_base();
    size_type offset = wrap(m_size);
    size_type top = size_type(m_allocated_size - offset);
    if (size <= top) {
        core::uninit_safe_fill(size, base + offset); // Throws
    }
    else {
        core::uninit_safe_fill(top, base + offset); // Throws
        try {
            core::uninit_safe_fill(size_type(size - top), base); // Throws
        }
        catch (...) {
            core::uninit_destroy(base + offset, top);
            throw;
        }
    }
    m_size += size;
}


template<class T>
inline void Deque<T>::do_append_1(size_type size, const T& value)
{
    reserve_extra(size); // Throws
    T* base = get_base();
    size_type offset = wrap(m_size);
    size_type top = size_type(m_allocated_size - offset);
    if (size <= top) {
        core::uninit_safe_fill(size, value, base + offset); // Throws
    }
    else {
        core::uninit_safe_fill(top, value, base + offset); // Throws
        try {
            core::uninit_safe_fill(size_type(size - top), value, base); // Throws
        }
        catch (...) {
            core::uninit_destroy(base + offset, top);
            throw;
        }
    }
    m_size += size;
}


template<class T>
template<class I> inline void Deque<T>::do_append_2(I begin, I end)
{
    using iterator_category = typename std::iterator_traits<I>::iterator_category;
    do_append_2(begin, end, iterator_category()); // Throws
}


template<class T>
template<class I> void Deque<T>::do_append_2(I begin, I end, std::input_iterator_tag)
{
    for (I i = begin; i != end; ++i)
        push_back(*i); // Throws
}


template<class T>
template<class I> void Deque<T>::do_append_2(I begin, I end, std::random_access_iterator_tag)
{
    size_type size = size_type(end - begin);
    reserve_extra(size); // Throws
    T* base = get_base();
    size_type offset = wrap(m_size);
    size_type top = size_type(m_allocated_size - offset);
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
void Deque<T>::do_erase(size_type begin, size_type end) noexcept
{
    if (ARCHON_LIKELY(begin != end)) {
        size_type before = begin;
        size_type after  = size_type(m_size - end);
        if (ARCHON_LIKELY(after <= before)) {
            erase_near_back(begin, end);
        }
        else {
            erase_near_front(begin, end);
        }
    }
}


template<class T>
void Deque<T>::erase_near_front(size_type begin, size_type end) noexcept
{
    T* base = get_base();
    size_type size = size_type(end - begin);
    ARCHON_ASSERT(size > 0);
    size_type begin_2 = begin;
    size_type top = size_type(m_allocated_size - m_offset);
    if (ARCHON_LIKELY(m_size > top)) {
        // Contents range is divided
        if (ARCHON_LIKELY(end <= top)) {
            // Erased range is entirely in back-aligned chunk
            core::uninit_destroy(base + m_offset + begin, size);
        }
        else {
            size_type after = size;
            size_type before = 0;
            if (ARCHON_LIKELY(begin >= top)) {
                // Erased range is entirely in front-aligned chunk
                core::uninit_destroy(base + (begin - top), size);
                core::uninit_move_upwards(base, size_type(begin - top), size);
                begin_2 = top;
            }
            else {
                // Erased range is divided
                after = size_type(end - top);
                before = size_type(size - after);
                core::uninit_destroy(base + m_offset + begin, before);
                core::uninit_destroy(base, after);
            }
            size_type n = std::min(after, begin);
            begin_2 = begin - n;
            core::uninit_safe_move(base + m_offset + begin_2, n, base + (after - n));
        }
    }
    else {
        // Contents range is contiguous
        core::uninit_destroy(base + m_offset + begin, size);
    }
    core::uninit_move_upwards(base + m_offset, begin_2, size);
    m_offset = wrap(size);
    m_size -= size;
}


template<class T>
void Deque<T>::erase_near_back(size_type begin, size_type end) noexcept
{
    T* base = get_base();
    size_type size = size_type(end - begin);
    ARCHON_ASSERT(size > 0);
    size_type end_2 = end;
    size_type end_3;
    size_type top = size_type(m_allocated_size - m_offset);
    if (ARCHON_LIKELY(m_size > top)) {
        // Contents range is divided
        if (ARCHON_LIKELY(begin >= top)) {
            // Erased range is entirely in front-aligned chunk
            core::uninit_destroy(base + (begin - top), size);
        }
        else {
            size_type after = 0;
            size_type before = size;
            if (ARCHON_LIKELY(end <= top)) {
                // Erased range is entirely in back-aligned chunk
                core::uninit_destroy(base + m_offset + begin, size);
                core::uninit_move_downwards(base + m_offset + end, size_type(top - end), size);
                end_2 = top;
            }
            else {
                // Erased range is divided
                after = size_type(end - top);
                before = size_type(size - after);
                core::uninit_destroy(base + m_offset + begin, before);
                core::uninit_destroy(base, after);
            }
            size_type remain = size_type(m_size - end);
            size_type n = std::min(before, remain);
            core::uninit_safe_move(base + after, n, base + m_offset + (top - before));
            end_2 += n;
        }
        end_3 = size_type(end_2 - top);
    }
    else {
        // Contents range is contiguous
        core::uninit_destroy(base + m_offset + begin, size);
        end_3 = size_type(m_offset + begin + size);
    }
    core::uninit_move_downwards(base + end_3, size_type(m_size - end_2), size);
    m_size -= size;
}


template<class T>
inline auto Deque<T>::get_base() noexcept -> T*
{
    return reinterpret_cast<T*>(m_memory.get());
}


template<class T>
inline auto Deque<T>::circular_inc(size_type index) noexcept -> size_type
{
    size_type index_2 = index + 1;
    if (ARCHON_LIKELY(index_2 < m_allocated_size))
        return index_2;
    return 0;
}


template<class T>
inline auto Deque<T>::circular_dec(size_type index) noexcept -> size_type
{
    if (ARCHON_LIKELY(index > 0))
        return index - 1;
    return m_allocated_size - 1;
}


template<class T>
inline auto Deque<T>::wrap(size_type index) noexcept -> size_type
{
    size_type top = m_allocated_size - m_offset;
    if (index < top)
        return m_offset + index;
    return index - top;
}


template<class T>
inline auto Deque<T>::unwrap(size_type index) noexcept -> size_type
{
    if (index >= m_offset)
        return index - m_offset;
    return m_allocated_size - (m_offset - index);
}


template<class T> inline void swap(Deque<T>& a, Deque<T>& b) noexcept
{
    a.swap(b);
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_DEQUE_HPP
