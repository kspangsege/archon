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

#ifndef ARCHON_X_CORE_X_VECTOR_HPP
#define ARCHON_X_CORE_X_VECTOR_HPP

/// \file


#include <cstddef>
#include <type_traits>
#include <limits>
#include <compare>
#include <algorithm>
#include <iterator>
#include <utility>
#include <initializer_list>
#include <array>
#include <stdexcept>

#include <archon/core/features.h>
#include <archon/core/type.hpp>
#include <archon/core/span.hpp>
#include <archon/core/memory.hpp>
#include <archon/core/impl/vector_impl.hpp>


namespace archon::core {


/// \brief Alternative vector implementation.
///
/// This is an implementation of a dynamically sized array that stores elements contiguously
/// in memory. It is similar in function to `std::vector`, but it offers stronger exception
/// guarantees. It also allows for an arbitrary initial capacity to be made statically
/// available inside the vector object (see \p N). It meets the requirements of
/// `ContiguousContainer` as defined by C++17.
///
/// \p T must either have a non-throwing move-constructor
/// (`std::is_nothrow_move_constructible`) or be copy-constructible
/// (`std::is_copy_constructible`). \p T must also have a non-throwing destructor
/// (`std::is_nothrow_destrucible`).
///
/// Move-construction and move-assignment (`Vector(Vector&&)` and `operator=(Vector&&)`), as
/// well as positioned insertion and removal (`emplace()` and `erase()`) are only supported
/// when \p T has a non-throwing move constructor (`std::is_nothrow_move_constructible`).
///
/// If `v` is a vector and `s` is `v.size()`, then `v.reserve_extra(n)` has the same effect
/// as `v.reserve(s + n)` except that if `s + n` overflows, then `v.reserve_extra(n)` throws
/// `std::length_error`.
///
/// When discounting move-construction, a newly constructed vector has a capacity equal to
/// `max(N, M)` where `N` is the capacity made statically available and `M` is the number of
/// elements associated with the construction operation. For default construction, `N` is
/// zero. For construction from initializer list, `N` is the size of the initializer
/// list. For `Vector(size_type size)` and `Vector(size_type size, const T& value)`, `N` is
/// `size`. For `Vector(I begin, I end)`, `N` is `std::distance(begin, end)`. For
/// move-construction, the capacity is equal to the capacity of the origin vector.
///
/// While the vector remains backed by the statically provided capacity, `shrink_to_fit()`
/// has no effect. On the other hand, when the vector owns dynamically allocated memory,
/// `shrink_to_fit()` will always reduce the capacity to the current size.
///
/// Some modifying operations may cause reallocation of memory. When memory is reallocated,
/// all iterators and pointers to stored values are invalidated.
///
/// A modifying operation is guaranteed to not cause a reallocation of memory if the final
/// size of the vector (`size()`) is less than, or equal to the original capacity
/// (`capacity()`). If the operation fails, memory is still guaranteed to not have been
/// reallocated so long as the final size of the vector would have been less than, or equal
/// to the original capacity. It follows from this that element removing operations
/// (`pop_back()`, `erase()`, `clear()`) never cause memory reallocation. Explicit
/// reallocation functions (`reserve_extra()`, `reserve()`, and `shrink_to_fit()`) do not
/// count as modifying operations here. Assignment and resize operations do count
/// (`assign()`, `resize()`).
///
/// After an operation that inserts one or more elements (`push_back()`, `emplace_back()`,
/// `append()`, and `emplace()`, `resize()`) and does not cause reallocation of memory,
/// iterators and pointers to stored elements beyond the point of insertion are
/// invalidated. This includes the end-iterator. Iterators and pointers to stored elements
/// before the point of insertion are not invalidated. This also applies to resize
/// operations where the specified size is greater than the current size.
///
/// After an operation that removes one or more elements (`pop_back()`, `erase()`,
/// `clear()`, `resize()`), iterators and pointers to stored elements in and beyond the
/// removed range are invalidated. This includes the end-iterator. Iterators and pointers to
/// stored elements before the removed range are not invalidated.
///
/// A reserve operation (`reserve_extra()`, `reserve()`) specifying a total capacity less
/// than, or equal to the current capacity is guaranteed to not cause reallocation of
/// memory, and therefore also guaranteed to not invalidate any iterators or pointers to
/// stored elements.
///
/// An operation, that inserts elements at the end and does not cause memory to be
/// reallocated, is guaranteed to not throw so long as the construction of the elements does
/// not throw. An operation, that inserts elements elsewhere, is guaranteed to not throw so
/// long as the construction of the elements does not throw and \p T has a non-throwing
/// move-constructor.
///
/// An operation, that removes elements at the end, is guaranteed to not throw (this
/// includes `clear()`). An operation, that removes elements elsewhere, is guaranteed to not
/// throw so long as \p T has a non-throwing move-constructor.
///
/// The only operations that can reduce the capacity of a vector are move-assignment
/// (`operator=(Vector&&)`) and `shrink_to_fit()`.
///
/// So long as \p N is zero, a vector type can be instantiated for an incomplete value
/// type. Instantiation of the member functions of a vector type, on the other hand, can
/// generally only happen once the value type is complete.
///
template<class T, std::size_t N = 0> class Vector {
public:
    using value_type = T;

    static constexpr std::size_t static_capacity = N;

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

    Vector() noexcept;
    Vector(Vector&&) noexcept;
    Vector(std::initializer_list<T>);
    explicit Vector(size_type size);
    Vector(size_type size, const T& value);
    template<class I, class = core::NeedIter<I>> Vector(I begin, I end);

    auto operator=(Vector&&) noexcept -> Vector&;

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

    auto data() noexcept       -> T*;
    auto data() const noexcept -> const T*;

    auto span() noexcept       -> span_type;
    auto span() const noexcept -> const_span_type;

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
    auto max_size() const noexcept -> size_type;
    auto capacity() const noexcept -> size_type;

    void reserve_extra(size_type min_extra_capacity);
    void reserve(size_type min_capacity);
    void shrink_to_fit();

    // Modifiers

    auto push_back(const T&) -> reference;
    auto push_back(T&&)      -> reference;

    template<class... A> auto emplace_back(A&&... args) -> reference;

    void pop_back() noexcept;

    void append(std::initializer_list<T>);
    void append(size_type size, const T& value);
    template<class I, class = NeedIter<I>> void append(I begin, I end);

    template<class... A> auto emplace(const_iterator pos, A&&... args) -> iterator;

    auto erase(const_iterator) noexcept -> iterator;
    auto erase(const_iterator begin, const_iterator end) noexcept -> iterator;

    void clear() noexcept;
    void resize(size_type size);
    void resize(size_type size, const T& value);

    // Comparison

    template<class U, std::size_t M> bool operator==(const Vector<U, M>&) const
        noexcept(noexcept(std::declval<T>() == std::declval<U>()));
    template<class U, std::size_t M> auto operator<=>(const Vector<U, M>&) const
        noexcept(noexcept(std::declval<T>() <=> std::declval<U>())) -> std::weak_ordering;

private:
    using Impl = impl::VectorImpl<T>;
    using aligned_storage_type = core::AlignedStorage<T, N>;

    class StaticMemAndImpl
        : public aligned_storage_type {
    public:
        Impl impl;
    };
    StaticMemAndImpl m_static_mem_and_impl;

    template<class I> void do_append(I begin, I end);
    template<class I> void do_append(I begin, I end, std::input_iterator_tag);
    template<class I> void do_append(I begin, I end, std::random_access_iterator_tag);

    auto static_mem() noexcept -> void*;

    auto impl() noexcept       -> Impl&;
    auto impl() const noexcept -> const Impl&;
};








// Implementation


template<class T, std::size_t N>
inline Vector<T, N>::Vector() noexcept
{
    impl().reset(static_mem(), N);
}


template<class T, std::size_t N>
inline Vector<T, N>::Vector(Vector&& other) noexcept
    : Vector()
{
    impl().move(std::move(other.impl()));
}


template<class T, std::size_t N>
inline Vector<T, N>::Vector(std::initializer_list<T> list)
    : Vector()
{
    append(list); // Throws
}


template<class T, std::size_t N>
inline Vector<T, N>::Vector(size_type size)
    : Vector()
{
    resize(size); // Throws
}


template<class T, std::size_t N>
inline Vector<T, N>::Vector(size_type size, const T& value)
    : Vector()
{
    append(size, value); // Throws
}


template<class T, std::size_t N>
template<class I, class> inline Vector<T, N>::Vector(I begin, I end)
    : Vector()
{
    append(begin, end); // Throws
}


template<class T, std::size_t N>
inline auto Vector<T, N>::operator=(Vector&& other) noexcept -> Vector&
{
    impl().reset(static_mem(), N);
    impl().move(std::move(other.impl()));
    return *this;
}


template<class T, std::size_t N>
inline void Vector<T, N>::assign(std::initializer_list<T> list)
{
    clear();
    append(list); // Throws
}


template<class T, std::size_t N>
inline void Vector<T, N>::assign(size_type size, const T& value)
{
    clear();
    append(size, value); // Throws
}


template<class T, std::size_t N>
template<class I, class> inline void Vector<T, N>::assign(I begin, I end)
{
    clear();
    append(begin, end); // Throws
}


template<class T, std::size_t N>
inline auto Vector<T, N>::at(size_type i) -> T&
{
    impl().verify_index(i); // Throws
    return operator[](i);
}


template<class T, std::size_t N>
inline auto Vector<T, N>::at(size_type i) const -> const T&
{
    impl().verify_index(i); // Throws
    return operator[](i);
}


template<class T, std::size_t N>
inline auto Vector<T, N>::operator[](size_type i) noexcept -> T&
{
    return data()[i];
}


template<class T, std::size_t N>
inline auto Vector<T, N>::operator[](size_type i) const noexcept -> const T&
{
    return data()[i];
}


template<class T, std::size_t N>
inline auto Vector<T, N>::front() noexcept -> T&
{
    return operator[](0);
}


template<class T, std::size_t N>
inline auto Vector<T, N>::front() const noexcept -> const T&
{
    return operator[](0);
}


template<class T, std::size_t N>
inline auto Vector<T, N>::back() noexcept -> T&
{
    return operator[](size() - 1);
}


template<class T, std::size_t N>
inline auto Vector<T, N>::back() const noexcept -> const T&
{
    return operator[](size() - 1);
}


template<class T, std::size_t N>
inline auto Vector<T, N>::data() noexcept -> T*
{
    return impl().data();
}


template<class T, std::size_t N>
inline auto Vector<T, N>::data() const noexcept -> const T*
{
    return impl().data();
}


template<class T, std::size_t N>
inline auto Vector<T, N>::span() noexcept -> span_type
{
    return { data(), size() };
}


template<class T, std::size_t N>
inline auto Vector<T, N>::span() const noexcept -> const_span_type
{
    return { data(), size() };
}


template<class T, std::size_t N>
inline auto Vector<T, N>::begin() noexcept -> iterator
{
    return data();
}


template<class T, std::size_t N>
inline auto Vector<T, N>::begin() const noexcept -> const_iterator
{
    return data();
}


template<class T, std::size_t N>
inline auto Vector<T, N>::cbegin() const noexcept -> const_iterator
{
    return begin();
}


template<class T, std::size_t N>
inline auto Vector<T, N>::end() noexcept -> iterator
{
    return data() + size();
}


template<class T, std::size_t N>
inline auto Vector<T, N>::end() const noexcept -> const_iterator
{
    return data() + size();
}


template<class T, std::size_t N>
inline auto Vector<T, N>::cend() const noexcept -> const_iterator
{
    return end();
}


template<class T, std::size_t N>
inline auto Vector<T, N>::rbegin() noexcept -> reverse_iterator
{
    return reverse_iterator(end());
}


template<class T, std::size_t N>
inline auto Vector<T, N>::rbegin() const noexcept -> const_reverse_iterator
{
    return const_reverse_iterator(end());
}


template<class T, std::size_t N>
inline auto Vector<T, N>::crbegin() const noexcept -> const_reverse_iterator
{
    return rbegin();
}


template<class T, std::size_t N>
inline auto Vector<T, N>::rend() noexcept -> reverse_iterator
{
    return reverse_iterator(begin());
}


template<class T, std::size_t N>
inline auto Vector<T, N>::rend() const noexcept -> const_reverse_iterator
{
    return const_reverse_iterator(begin());
}


template<class T, std::size_t N>
inline auto Vector<T, N>::crend() const noexcept -> const_reverse_iterator
{
    return rend();
}


template<class T, std::size_t N>
inline bool Vector<T, N>::empty() const noexcept
{
    return (size() == 0);
}


template<class T, std::size_t N>
inline auto Vector<T, N>::size() const noexcept -> size_type
{
    return impl().size();
}


template<class T, std::size_t N>
inline auto Vector<T, N>::max_size() const noexcept -> size_type
{
    return std::numeric_limits<difference_type>::max();
}


template<class T, std::size_t N>
inline auto Vector<T, N>::capacity() const noexcept -> size_type
{
    return impl().capacity();
}


template<class T, std::size_t N>
inline void Vector<T, N>::reserve_extra(size_type min_extra_capacity)
{
    impl().reserve_extra(min_extra_capacity); // Throws
}


template<class T, std::size_t N>
inline void Vector<T, N>::reserve(size_type min_capacity)
{
    impl().reserve(min_capacity); // Throws
}


template<class T, std::size_t N>
inline void Vector<T, N>::shrink_to_fit()
{
    impl().shrink_to_fit(); // Throws
}


template<class T, std::size_t N>
inline auto Vector<T, N>::push_back(const T& value) -> T&
{
    return emplace_back(value); // Throws
}


template<class T, std::size_t N>
inline auto Vector<T, N>::push_back(T&& value) -> T&
{
    return emplace_back(std::move(value)); // Throws
}


template<class T, std::size_t N>
template<class... A> inline auto Vector<T, N>::emplace_back(A&&... args) -> T&
{
    impl().emplace_back(std::forward<A>(args)...); // Throws
    return back();
}


template<class T, std::size_t N>
inline void Vector<T, N>::pop_back() noexcept
{
    impl().pop_back();
}


template<class T, std::size_t N>
inline void Vector<T, N>::append(std::initializer_list<T> list)
{
    do_append(list.begin(), list.end()); // Throws
}


template<class T, std::size_t N>
inline void Vector<T, N>::append(size_type size, const T& value)
{
    impl().append(size, value); // Throws
}


template<class T, std::size_t N>
template<class I, class> inline void Vector<T, N>::append(I begin, I end)
{
    do_append(begin, end); // Throws
}


template<class T, std::size_t N>
template<class... A> auto Vector<T, N>::emplace(const_iterator i, A&&... args) -> iterator
{
    T* base = data();
    std::size_t offset = std::size_t(i - base);
    impl().insert(offset, std::forward<A>(args)...); // Throws
    base = data(); // May have changed due to reallocation
    return base + offset;
}


template<class T, std::size_t N>
inline auto Vector<T, N>::erase(const_iterator i) noexcept -> iterator
{
    T* base = data();
    std::size_t offset = std::size_t(i - base);
    std::size_t n = 1;
    impl().erase(offset, n);
    return base + offset;
}


template<class T, std::size_t N>
inline auto Vector<T, N>::erase(const_iterator begin, const_iterator end) noexcept -> iterator
{
    T* base = data();
    std::size_t offset = std::size_t(begin - base);
    std::size_t n = std::size_t(end - begin);
    if (ARCHON_LIKELY(n > 0))
        impl().erase(offset, n);
    return base + offset;
}


template<class T, std::size_t N>
inline void Vector<T, N>::clear() noexcept
{
    resize(0);
}


template<class T, std::size_t N>
inline void Vector<T, N>::resize(size_type size)
{
    impl().resize(size); // throws
}


template<class T, std::size_t N>
inline void Vector<T, N>::resize(size_type size, const T& value)
{
    impl().resize(size, value); // throws
}


template<class T, std::size_t N>
template<class U, std::size_t M>
inline bool Vector<T, N>::operator==(const Vector<U, M>& vector) const
    noexcept(noexcept(std::declval<T>() == std::declval<U>()))
{
    return std::equal(begin(), end(), vector.begin(), vector.end()); // Throws
}


template<class T, std::size_t N>
template<class U, std::size_t M>
inline auto Vector<T, N>::operator<=>(const Vector<U, M>& vector) const
    noexcept(noexcept(std::declval<T>() <=> std::declval<U>())) -> std::weak_ordering
{
    return std::lexicographical_compare_three_way(begin(), end(), vector.begin(), vector.end()); // Throws
}


template<class T, std::size_t N>
template<class I> inline void Vector<T, N>::do_append(I begin, I end)
{
    using iterator_category = typename std::iterator_traits<I>::iterator_category;
    do_append(begin, end, iterator_category()); // Throws
}


template<class T, std::size_t N>
template<class I> inline void Vector<T, N>::do_append(I begin, I end, std::input_iterator_tag)
{
    for (I i = begin; i != end; ++i)
        push_back(*i); // Throws
}


template<class T, std::size_t N>
template<class I> inline void Vector<T, N>::do_append(I begin, I end, std::random_access_iterator_tag)
{
    impl().append(begin, end); // Throws
}


template<class T, std::size_t N>
inline auto Vector<T, N>::static_mem() noexcept -> void*
{
    return m_static_mem_and_impl.addr();
}


template<class T, std::size_t N>
inline auto Vector<T, N>::impl() noexcept -> Impl&
{
    return m_static_mem_and_impl.impl;
}


template<class T, std::size_t N>
inline auto Vector<T, N>::impl() const noexcept -> const Impl&
{
    return m_static_mem_and_impl.impl;
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_VECTOR_HPP
