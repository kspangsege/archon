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

#ifndef ARCHON_X_CORE_X_FLAT_MULTIMAP_HPP
#define ARCHON_X_CORE_X_FLAT_MULTIMAP_HPP

/// \file


#include <cstddef>
#include <type_traits>
#include <utility>
#include <iterator>

#include <archon/core/pair.hpp>
#include <archon/core/impl/flat_map_impl.hpp>


namespace archon::core {


/// \brief Multi-map implementation with contiguous storage.
///
/// This is an implementation of a multi-map (associative container) that stores its entries
/// sequentially, and ordered according to the keys (first component of each entry). The
/// entries are stored contiguously in memory.
///
/// When multiple entries with the same key are inserted into the multi-map, the entries
/// will occur in the multi-map in the order that they were inserted.
///
/// The contiguous storage of entries means that the map will generally have a more
/// localized memory access pattern. It also allows for stronger exception guarantees
/// compared to `std::multimap`.
///
/// The major disadvantage, relative to `std::multimap`, is that insertion is
/// slower. Insertion complexity is O(N) for this map implementation, and O(log N) for
/// `std::multimap`.
///
/// Requirement: Both the key type (\p K) and the value type (\p V) must have a non-throwing
/// move constructor (`std::is_nothrow_move_constructible`) and a non-throwing destructor
/// (`std::is_nothrow_destructible`).
///
/// Requirement: If `a` and `b` are `const`-references to keys (`const K&`), then `a < b`
/// must be a non-throwing operation, i.e., `noexcept(a < b)` must be `true`.
///
/// An initial capacity can be made statically available inside the map object. The number
/// of entries of initial static capacity is specified by \p N.
///
/// So long as \p N is zero, a flat multi-map type can be instantiated for an incomplete key
/// and / or value type (\p K and \p V). Instantiation of the member functions of a flat
/// multi-map type, on the other hand, can generally only happen once both the key and the
/// value types are complete.
///
template<class K, class V, std::size_t N = 0> class FlatMultimap {
public:
    using key_type    = K;
    using mapped_type = V;

    static constexpr std::size_t static_capacity = N;

    using value_type      = core::Pair<const key_type, mapped_type>;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;

    using reference              = value_type&;
    using const_reference        = const value_type&;
    using pointer                = value_type*;
    using const_pointer          = const value_type*;
    using iterator               = pointer;
    using const_iterator         = const_pointer;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    FlatMultimap() noexcept = default;
    FlatMultimap(std::initializer_list<value_type>);

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

    template<class... A> auto emplace(A&&... args) -> iterator;

    auto insert(const value_type&) -> iterator;
    template<class I> void insert(I begin, I end);

    auto erase(const key_type&) noexcept -> size_type;
    void clear() noexcept;

    // Lookup

    auto lower_bound(const key_type&) noexcept       -> iterator;
    auto lower_bound(const key_type&) const noexcept -> const_iterator;

    auto upper_bound(const key_type&) noexcept       -> iterator;
    auto upper_bound(const key_type&) const noexcept -> const_iterator;

    auto equal_range(const key_type&) noexcept       -> std::pair<iterator, iterator>;
    auto equal_range(const key_type&) const noexcept -> std::pair<const_iterator, const_iterator>;

private:
    impl::FlatMapImpl<K, V, N> m_impl;
};








// Implementation


template<class K, class V, std::size_t N>
inline FlatMultimap<K, V, N>::FlatMultimap(std::initializer_list<value_type> entries)
    : FlatMultimap()
{
    insert(entries.begin(), entries.end()); // Throws
}


template<class K, class V, std::size_t N>
inline auto FlatMultimap<K, V, N>::begin() noexcept -> iterator
{
    return m_impl.data();
}


template<class K, class V, std::size_t N>
inline auto FlatMultimap<K, V, N>::begin() const noexcept -> const_iterator
{
    return m_impl.data();
}


template<class K, class V, std::size_t N>
inline auto FlatMultimap<K, V, N>::cbegin() const noexcept -> const_iterator
{
    return begin();
}


template<class K, class V, std::size_t N>
inline auto FlatMultimap<K, V, N>::end() noexcept -> iterator
{
    return begin() + size();
}


template<class K, class V, std::size_t N>
inline auto FlatMultimap<K, V, N>::end() const noexcept -> const_iterator
{
    return begin() + size();
}


template<class K, class V, std::size_t N>
inline auto FlatMultimap<K, V, N>::cend() const noexcept -> const_iterator
{
    return end();
}


template<class K, class V, std::size_t N>
inline auto FlatMultimap<K, V, N>::rbegin() noexcept -> reverse_iterator
{
    return reverse_iterator(end());
}


template<class K, class V, std::size_t N>
inline auto FlatMultimap<K, V, N>::rbegin() const noexcept -> const_reverse_iterator
{
    return const_reverse_iterator(end());
}


template<class K, class V, std::size_t N>
inline auto FlatMultimap<K, V, N>::crbegin() const noexcept -> const_reverse_iterator
{
    return rbegin();
}


template<class K, class V, std::size_t N>
inline auto FlatMultimap<K, V, N>::rend() noexcept -> reverse_iterator
{
    return reverse_iterator(begin());
}


template<class K, class V, std::size_t N>
inline auto FlatMultimap<K, V, N>::rend() const noexcept -> const_reverse_iterator
{
    return const_reverse_iterator(begin());
}


template<class K, class V, std::size_t N>
inline auto FlatMultimap<K, V, N>::crend() const noexcept -> const_reverse_iterator
{
    return rend();
}


template<class K, class V, std::size_t N>
inline bool FlatMultimap<K, V, N>::empty() const noexcept
{
    return m_impl.empty();
}


template<class K, class V, std::size_t N>
inline auto FlatMultimap<K, V, N>::size() const noexcept -> size_type
{
    return m_impl.size();
}


template<class K, class V, std::size_t N>
inline auto FlatMultimap<K, V, N>::max_size() const noexcept -> size_type
{
    return m_impl.max_size();
}


template<class K, class V, std::size_t N>
inline auto FlatMultimap<K, V, N>::capacity() const noexcept -> size_type
{
    return m_impl.capacity();
}


template<class K, class V, std::size_t N>
inline void FlatMultimap<K, V, N>::reserve_extra(size_type min_extra_capacity)
{
    m_impl.reserve_extra(min_extra_capacity); // Throws
}


template<class K, class V, std::size_t N>
inline void FlatMultimap<K, V, N>::reserve(size_type min_capacity)
{
    m_impl.reserve(min_capacity); // Throws
}


template<class K, class V, std::size_t N>
inline void FlatMultimap<K, V, N>::shrink_to_fit()
{
    m_impl.shrink_to_fit(); // Throws
}


template<class K, class V, std::size_t N>
template<class... A> inline auto FlatMultimap<K, V, N>::emplace(A&&... args) -> iterator
{
    return m_impl.insert_multi(std::forward<A>(args)...); // Throws
}


template<class K, class V, std::size_t N>
inline auto FlatMultimap<K, V, N>::insert(const value_type& entry) -> iterator
{
    return m_impl.insert_multi(entry); // Throws
}


template<class K, class V, std::size_t N>
template<class I> void FlatMultimap<K, V, N>::insert(I begin, I end)
{
    for (I i = begin; i != end; ++i)
        insert(*i); // Throws
}


template<class K, class V, std::size_t N>
inline auto FlatMultimap<K, V, N>::erase(const key_type& key) noexcept -> size_type
{
    return m_impl.erase(key);
}


template<class K, class V, std::size_t N>
inline void FlatMultimap<K, V, N>::clear() noexcept
{
    m_impl.clear();
}


template<class K, class V, std::size_t N>
inline auto FlatMultimap<K, V, N>::lower_bound(const key_type& key) noexcept -> iterator
{
    std::size_t i = m_impl.lower_bound(key);
    return m_impl.data() + i;
}


template<class K, class V, std::size_t N>
inline auto FlatMultimap<K, V, N>::lower_bound(const key_type& key) const noexcept -> const_iterator
{
    std::size_t i = m_impl.lower_bound(key);
    return m_impl.data() + i;
}


template<class K, class V, std::size_t N>
inline auto FlatMultimap<K, V, N>::upper_bound(const key_type& key) noexcept -> iterator
{
    std::size_t i = m_impl.upper_bound(key);
    return m_impl.data() + i;
}


template<class K, class V, std::size_t N>
inline auto FlatMultimap<K, V, N>::upper_bound(const key_type& key) const noexcept -> const_iterator
{
    std::size_t i = m_impl.upper_bound(key);
    return m_impl.data() + i;
}


template<class K, class V, std::size_t N>
inline auto FlatMultimap<K, V, N>::equal_range(const key_type& key) noexcept -> std::pair<iterator, iterator>
{
    std::pair<std::size_t, std::size_t> p = m_impl.equal_range(key);
    return { m_impl.data() + p.first, m_impl.data() + p.second };
}


template<class K, class V, std::size_t N>
inline auto FlatMultimap<K, V, N>::equal_range(const key_type& key) const noexcept ->
    std::pair<const_iterator, const_iterator>
{
    std::pair<std::size_t, std::size_t> p = m_impl.equal_range(key);
    return { m_impl.data() + p.first, m_impl.data() + p.second };
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_FLAT_MULTIMAP_HPP
