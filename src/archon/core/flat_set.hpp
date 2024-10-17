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

#ifndef ARCHON_X_CORE_X_FLAT_SET_HPP
#define ARCHON_X_CORE_X_FLAT_SET_HPP

/// \file


#include <cstddef>
#include <type_traits>
#include <utility>
#include <iterator>

#include <archon/core/type_traits.hpp>
#include <archon/core/impl/flat_map_impl.hpp>


namespace archon::core {


/// \brief Set implementation with contiguous storage.
///
/// This is an implementation of a set that stores its elements sequentially and in sorted
/// order. The elements are stored contiguously in memory.
///
/// The contiguous storage of elements means that the set will generally have a more
/// localized memory access pattern. It also allows for stronger exception guarantees
/// compared to `std::set`.
///
/// One disadvantage relative to `std::set` is that insertion is slower (`insert()` and
/// `emplace()`). Insertion complexity is O(N) for this set implementation, and O(log N) for
/// `std::set` where N is the number of elements in the set.
///
/// Another disadvantage compared to `std::set` is that iterators and pointers referring to
/// stored elements are invalidated after every modifying operation.
///
/// Requirement: Elements (objects of type \p K) must be less-than comparable.
///
/// Requirement: Elements (objects of type \p K) must be copy-constructible and
/// copy-construction must be a non-throwing operation
/// (`std::is_nothrow_copy_constructible`). The element type must also have a non-throwing
/// destructor (`std::is_nothrow_destructible`).
///
/// An initial capacity can be made statically available inside the set object. The number
/// of elements of initial static capacity is specified by \p N.
///
/// So long as \p N is zero, a flat set type can be instantiated for an incomplete element
/// type (\p K). Instantiation of the member functions of a flat set type, on the other
/// hand, can generally only happen once the element type is complete.
///
template<class K, std::size_t N = 0> class FlatSet {
public:
    using key_type = K;

    static constexpr std::size_t static_capacity = N;

    using value_type      = key_type;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;

    using reference              = const value_type&;
    using const_reference        = const value_type&;
    using pointer                = const value_type*;
    using const_pointer          = const value_type*;
    using iterator               = pointer;
    using const_iterator         = const_pointer;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    FlatSet() noexcept = default;
    FlatSet(std::initializer_list<value_type>);

    // Iterators

    auto begin() const noexcept  -> const_iterator;
    auto cbegin() const noexcept -> const_iterator;

    auto end() const noexcept  -> const_iterator;
    auto cend() const noexcept -> const_iterator;

    auto rbegin() const noexcept  -> const_reverse_iterator;
    auto crbegin() const noexcept -> const_reverse_iterator;

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

    template<class... A> auto emplace(A&&... args) -> std::pair<iterator, bool>;

    auto insert(const value_type&) -> std::pair<iterator, bool>;
    auto insert(value_type&&) -> std::pair<iterator, bool>;
    template<class I> void insert(I begin, I end);

    auto erase(const key_type&) noexcept(core::is_nothrow_less_comparable<K>) -> size_type;
    void clear() noexcept;

    // Lookup

    bool contains(const key_type&) const noexcept(core::is_nothrow_less_comparable<K>);
    auto count(const key_type&) const noexcept(core::is_nothrow_less_comparable<K>) -> size_type;

    auto find(const key_type&) noexcept(core::is_nothrow_less_comparable<K>)       -> iterator;
    auto find(const key_type&) const noexcept(core::is_nothrow_less_comparable<K>) -> const_iterator;

    auto lower_bound(const key_type&) noexcept(core::is_nothrow_less_comparable<K>)       -> iterator;
    auto lower_bound(const key_type&) const noexcept(core::is_nothrow_less_comparable<K>) -> const_iterator;

    auto upper_bound(const key_type&) noexcept(core::is_nothrow_less_comparable<K>)       -> iterator;
    auto upper_bound(const key_type&) const noexcept(core::is_nothrow_less_comparable<K>) -> const_iterator;

    auto equal_range(const key_type&) noexcept(core::is_nothrow_less_comparable<K>) -> std::pair<iterator, iterator>;
    auto equal_range(const key_type&) const noexcept(core::is_nothrow_less_comparable<K>) ->
        std::pair<const_iterator, const_iterator>;

private:
    impl::FlatMapImpl<K, void, N> m_impl;
};








// Implementation


template<class K, std::size_t N>
inline FlatSet<K, N>::FlatSet(std::initializer_list<value_type> entries)
    : FlatSet()
{
    insert(entries.begin(), entries.end()); // Throws
}


template<class K, std::size_t N>
inline auto FlatSet<K, N>::begin() const noexcept -> const_iterator
{
    return m_impl.data();
}


template<class K, std::size_t N>
inline auto FlatSet<K, N>::cbegin() const noexcept -> const_iterator
{
    return begin();
}


template<class K, std::size_t N>
inline auto FlatSet<K, N>::end() const noexcept -> const_iterator
{
    return begin() + size();
}


template<class K, std::size_t N>
inline auto FlatSet<K, N>::cend() const noexcept -> const_iterator
{
    return end();
}


template<class K, std::size_t N>
inline auto FlatSet<K, N>::rbegin() const noexcept -> const_reverse_iterator
{
    return const_reverse_iterator(end());
}


template<class K, std::size_t N>
inline auto FlatSet<K, N>::crbegin() const noexcept -> const_reverse_iterator
{
    return rbegin();
}


template<class K, std::size_t N>
inline auto FlatSet<K, N>::rend() const noexcept -> const_reverse_iterator
{
    return const_reverse_iterator(begin());
}


template<class K, std::size_t N>
inline auto FlatSet<K, N>::crend() const noexcept -> const_reverse_iterator
{
    return rend();
}


template<class K, std::size_t N>
inline bool FlatSet<K, N>::empty() const noexcept
{
    return m_impl.empty();
}


template<class K, std::size_t N>
inline auto FlatSet<K, N>::size() const noexcept -> size_type
{
    return m_impl.size();
}


template<class K, std::size_t N>
inline auto FlatSet<K, N>::max_size() const noexcept -> size_type
{
    return m_impl.max_size();
}


template<class K, std::size_t N>
inline auto FlatSet<K, N>::capacity() const noexcept -> size_type
{
    return m_impl.capacity();
}


template<class K, std::size_t N>
inline void FlatSet<K, N>::reserve_extra(size_type min_extra_capacity)
{
    m_impl.reserve_extra(min_extra_capacity); // Throws
}


template<class K, std::size_t N>
inline void FlatSet<K, N>::reserve(size_type min_capacity)
{
    m_impl.reserve(min_capacity); // Throws
}


template<class K, std::size_t N>
inline void FlatSet<K, N>::shrink_to_fit()
{
    m_impl.shrink_to_fit(); // Throws
}


template<class K, std::size_t N>
template<class... A> inline auto FlatSet<K, N>::emplace(A&&... args) -> std::pair<iterator, bool>
{
    return m_impl.insert(std::forward<A>(args)...); // Throws
}


template<class K, std::size_t N>
inline auto FlatSet<K, N>::insert(const value_type& elem) -> std::pair<iterator, bool>
{
    return m_impl.insert(elem); // Throws
}


template<class K, std::size_t N>
inline auto FlatSet<K, N>::insert(value_type&& elem) -> std::pair<iterator, bool>
{
    return m_impl.insert(std::move(elem)); // Throws
}


template<class K, std::size_t N>
template<class I> void FlatSet<K, N>::insert(I begin, I end)
{
    for (I i = begin; i != end; ++i)
        insert(*i); // Throws
}


template<class K, std::size_t N>
inline auto FlatSet<K, N>::erase(const key_type& key) noexcept(core::is_nothrow_less_comparable<K>) -> size_type
{
    return m_impl.erase(key);
}


template<class K, std::size_t N>
inline void FlatSet<K, N>::clear() noexcept
{
    m_impl.clear();
}


template<class K, std::size_t N>
inline bool FlatSet<K, N>::contains(const key_type& key) const noexcept(core::is_nothrow_less_comparable<K>)
{
    std::size_t i = m_impl.find(key);
    return (i != size());
}


template<class K, std::size_t N>
inline auto FlatSet<K, N>::count(const key_type& key) const noexcept(core::is_nothrow_less_comparable<K>) -> size_type
{
    return size_type(int(contains(key)));
}


template<class K, std::size_t N>
inline auto FlatSet<K, N>::find(const key_type& key) noexcept(core::is_nothrow_less_comparable<K>) -> iterator
{
    std::size_t i = m_impl.find(key);
    return m_impl.data() + i;
}


template<class K, std::size_t N>
inline auto FlatSet<K, N>::find(const key_type& key) const noexcept(core::is_nothrow_less_comparable<K>) ->
    const_iterator
{
    std::size_t i = m_impl.find(key);
    return m_impl.data() + i;
}


template<class K, std::size_t N>
inline auto FlatSet<K, N>::lower_bound(const key_type& key) noexcept(core::is_nothrow_less_comparable<K>) -> iterator
{
    std::size_t i = m_impl.lower_bound(key);
    return m_impl.data() + i;
}


template<class K, std::size_t N>
inline auto FlatSet<K, N>::lower_bound(const key_type& key) const noexcept(core::is_nothrow_less_comparable<K>) ->
    const_iterator
{
    std::size_t i = m_impl.lower_bound(key);
    return m_impl.data() + i;
}


template<class K, std::size_t N>
inline auto FlatSet<K, N>::upper_bound(const key_type& key) noexcept(core::is_nothrow_less_comparable<K>) -> iterator
{
    std::size_t i = m_impl.upper_bound(key);
    return m_impl.data() + i;
}


template<class K, std::size_t N>
inline auto FlatSet<K, N>::upper_bound(const key_type& key) const noexcept(core::is_nothrow_less_comparable<K>) ->
    const_iterator
{
    std::size_t i = m_impl.upper_bound(key);
    return m_impl.data() + i;
}


template<class K, std::size_t N>
inline auto FlatSet<K, N>::equal_range(const key_type& key) noexcept(core::is_nothrow_less_comparable<K>) ->
    std::pair<iterator, iterator>
{
    std::pair<std::size_t, std::size_t> p = m_impl.equal_range(key);
    return { m_impl.data() + p.first, m_impl.data() + p.second };
}


template<class K, std::size_t N>
inline auto FlatSet<K, N>::equal_range(const key_type& key) const noexcept(core::is_nothrow_less_comparable<K>) ->
    std::pair<const_iterator, const_iterator>
{
    std::pair<std::size_t, std::size_t> p = m_impl.equal_range(key);
    return { m_impl.data() + p.first, m_impl.data() + p.second };
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_FLAT_SET_HPP
