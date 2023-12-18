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

#ifndef ARCHON_X_CORE_X_IMPL_X_FLAT_MAP_IMPL_HPP
#define ARCHON_X_CORE_X_IMPL_X_FLAT_MAP_IMPL_HPP

/// \file


#include <cstddef>
#include <type_traits>
#include <utility>

#include <archon/core/features.h>
#include <archon/core/pair.hpp>
#include <archon/core/vector.hpp>


namespace archon::core::impl {


template<class K, class V, std::size_t N> class FlatMapImpl {
public:
    using key_type   = K;
    using value_type = V;

    static constexpr bool is_set = std::is_same_v<value_type, void>;

    using entry_type = std::conditional_t<is_set, const key_type, core::Pair<const key_type, value_type>>;
    using nonconst_entry_type = std::remove_const_t<entry_type>;

    bool empty() const noexcept;
    auto size() const noexcept -> std::size_t;
    auto data() noexcept -> entry_type*;
    auto data() const noexcept -> const entry_type*;

    auto find(const key_type&) const noexcept -> std::size_t;
    auto lower_bound(const key_type&) const noexcept -> std::size_t;
    auto upper_bound(const key_type&) const noexcept -> std::size_t;
    auto equal_range(const key_type&) const noexcept -> std::pair<std::size_t, std::size_t>;

    template<class... A> auto insert(A&&... args) -> std::pair<entry_type*, bool>;
    template<class... A> auto insert_multi(A&&... args) -> entry_type*;
    auto erase(const key_type&) noexcept -> std::size_t;
    void clear() noexcept;

    auto capacity() const noexcept -> std::size_t;
    void reserve_extra(std::size_t min_extra_capacity);
    void reserve(std::size_t min_capacity);
    void shrink_to_fit();

    auto max_size() const noexcept -> std::size_t;

private:
    core::Vector<std::remove_const_t<entry_type>, N> m_entries;

    auto do_upper_bound(const key_type&, std::size_t) const noexcept -> std::size_t;
    bool do_find(const key_type&, std::size_t&) const noexcept;
    static auto get_key(const entry_type&) noexcept -> const key_type&;
    static bool less(const key_type&, const key_type&) noexcept;
};








// Implementation


template<class K, class V, std::size_t N>
inline bool FlatMapImpl<K, V, N>::empty() const noexcept
{
    return m_entries.empty();
}


template<class K, class V, std::size_t N>
inline auto FlatMapImpl<K, V, N>::size() const noexcept -> std::size_t
{
    return m_entries.size();
}


template<class K, class V, std::size_t N>
inline auto FlatMapImpl<K, V, N>::data() noexcept -> entry_type*
{
    return m_entries.data();
}


template<class K, class V, std::size_t N>
inline auto FlatMapImpl<K, V, N>::data() const noexcept -> const entry_type*
{
    return m_entries.data();
}


template<class K, class V, std::size_t N>
inline auto FlatMapImpl<K, V, N>::find(const key_type& key) const noexcept -> std::size_t
{
    std::size_t i = 0;
    if (ARCHON_LIKELY(do_find(key, i)))
        return i;
    return size();
}


template<class K, class V, std::size_t N>
auto FlatMapImpl<K, V, N>::lower_bound(const key_type& key) const noexcept -> std::size_t
{
    const entry_type* base = m_entries.data();
    const entry_type* begin = base;
    const entry_type* end = begin + size();
    while (ARCHON_LIKELY(begin != end)) {
        const entry_type* mid = begin + (end - begin) / 2;
        if (less(get_key(*mid), key)) {
            begin = mid + 1;
        }
        else {
            end = mid;
        }
    }
    return std::size_t(begin - base);
}


template<class K, class V, std::size_t N>
inline auto FlatMapImpl<K, V, N>::upper_bound(const key_type& key) const noexcept -> std::size_t
{
    return do_upper_bound(key, 0);
}


template<class K, class V, std::size_t N>
inline auto FlatMapImpl<K, V, N>::equal_range(const key_type& key) const noexcept ->
    std::pair<std::size_t, std::size_t>
{
    std::size_t begin = lower_bound(key);
    std::size_t end   = do_upper_bound(key, begin);
    return { begin, end };
}


template<class K, class V, std::size_t N>
template<class... A> auto FlatMapImpl<K, V, N>::insert(A&&... args) -> std::pair<entry_type*, bool>
{
    nonconst_entry_type entry(std::forward<A>(args)...); // Throws
    const key_type& key = get_key(entry);
    entry_type* base = m_entries.data();
    std::size_t i = 0;
    if (ARCHON_LIKELY(!do_find(key, i))) {
        entry_type* j = m_entries.emplace(base + i, std::move(entry)); // Throws
        return { j, true }; // Inserted
    }
    return { base + i, false }; // Not inserted
}


template<class K, class V, std::size_t N>
template<class... A> inline auto FlatMapImpl<K, V, N>::insert_multi(A&&... args) -> entry_type*
{
    nonconst_entry_type entry(std::forward<A>(args)...); // Throws
    const key_type& key = get_key(entry);
    std::size_t i = upper_bound(key);
    entry_type* base = m_entries.data();
    return m_entries.emplace(base + i, std::move(entry)); // Throws
}


template<class K, class V, std::size_t N>
auto FlatMapImpl<K, V, N>::erase(const key_type& key) noexcept -> std::size_t
{
    auto pair = equal_range(key);
    entry_type* base = m_entries.data();
    entry_type* begin = base + pair.first;
    entry_type* end   = base + pair.second;
    m_entries.erase(begin, end);
    return std::size_t(end - begin);
}


template<class K, class V, std::size_t N>
inline void FlatMapImpl<K, V, N>::clear() noexcept
{
    m_entries.clear();
}


template<class K, class V, std::size_t N>
inline auto FlatMapImpl<K, V, N>::capacity() const noexcept -> std::size_t
{
    return m_entries.capacity();
}


template<class K, class V, std::size_t N>
inline void FlatMapImpl<K, V, N>::reserve_extra(std::size_t min_extra_capacity)
{
    return m_entries.reserve_extra(min_extra_capacity); // Throws
}


template<class K, class V, std::size_t N>
inline void FlatMapImpl<K, V, N>::reserve(std::size_t min_capacity)
{
    return m_entries.reserve(min_capacity); // Throws
}


template<class K, class V, std::size_t N>
inline void FlatMapImpl<K, V, N>::shrink_to_fit()
{
    return m_entries.shrink_to_fit(); // Throws
}


template<class K, class V, std::size_t N>
inline auto FlatMapImpl<K, V, N>::max_size() const noexcept -> std::size_t
{
    return m_entries.max_size();
}


template<class K, class V, std::size_t N>
auto FlatMapImpl<K, V, N>::do_upper_bound(const key_type& key, std::size_t i) const noexcept -> std::size_t
{
    const entry_type* base = m_entries.data();
    const entry_type* begin = base + i;
    const entry_type* end   = base + size();
    while (ARCHON_LIKELY(begin != end)) {
        const entry_type* mid = begin + (end - begin) / 2;
        if (!less(key, get_key(*mid))) {
            begin = mid + 1;
        }
        else {
            end = mid;
        }
    }
    return std::size_t(begin - base);
}


template<class K, class V, std::size_t N>
inline bool FlatMapImpl<K, V, N>::do_find(const key_type& key, std::size_t& i) const noexcept
{
    i = lower_bound(key);
    bool was_found = (i < size() && !less(key, get_key(m_entries[i])));
    return was_found;
}


template<class K, class V, std::size_t N>
inline auto FlatMapImpl<K, V, N>::get_key(const entry_type& entry) noexcept -> const key_type&
{
    if constexpr (is_set) {
        return entry;
    }
    else {
        return entry.first;
    }
}


template<class K, class V, std::size_t N>
inline bool FlatMapImpl<K, V, N>::less(const key_type& a, const key_type& b) noexcept
{
    static_assert(noexcept(a < b));
    return (a < b);
}


} // namespace archon::core::impl

#endif // ARCHON_X_CORE_X_IMPL_X_FLAT_MAP_IMPL_HPP
