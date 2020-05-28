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

#ifndef ARCHON_X_CORE_X_RANGE_MAP_HPP
#define ARCHON_X_CORE_X_RANGE_MAP_HPP

/// \file


#include <type_traits>
#include <utility>
#include <functional>
#include <map>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/proxy_iterator.hpp>


namespace archon::core {


template<class K> struct RangeMapRange {
    K first;
    K last;
};


template<class K, class T> class RangeMap {
private:
    static_assert(std::is_nothrow_copy_constructible_v<K>);

    class Proxy;

    // Key is lowest value in range. First component of value is highest value in range.
    using underlying_map_type = std::map<K, std::pair<K, T>>;

public:
    using key_type       = K;
    using range_type     = RangeMapRange<K>;
    using mapped_type    = T;
    using value_type     = Proxy;
    using iterator       = ProxyIterator<typename underlying_map_type::const_iterator, Proxy>;

    auto begin() const noexcept -> iterator;
    auto end() const noexcept -> iterator;

    void assign(K key, const T& value);
    void assign(const range_type& range, const T& value);

    template<class F> void update(K key, const F& func);
    template<class F> void update(const range_type& range, const F& func);

    void defrag();
    template<class E> void defrag(const E& equal);

private:
    // INVARIANT: No ranges in the map overlap
    // INVARIANT: No ranges in the map are empty
    underlying_map_type m_map;
};




template<class K, class T> class RangeMap<K, T>::Proxy {
public:
    explicit Proxy(const typename underlying_map_type::value_type&) noexcept;

    auto range() const noexcept -> range_type;
    auto value() const noexcept -> const T&;

private:
    const typename underlying_map_type::value_type& m_entry;
};








// Implementation


template<class K, class T>
inline auto RangeMap<K, T>::begin() const noexcept -> iterator
{
    return iterator(m_map.begin());
}


template<class K, class T>
inline auto RangeMap<K, T>::end() const noexcept -> iterator
{
    return iterator(m_map.end());
}


template<class K, class T>
inline void RangeMap<K, T>::assign(K key, const T& value)
{
    assign(range_type({ key, key }), value); // Throws
}


template<class K, class T>
inline void RangeMap<K, T>::assign(const range_type& range, const T& value)
{
    update(range, [&](T& value_2) { value_2 = value; }); // Throws
}


template<class K, class T>
template<class F> inline void RangeMap<K, T>::update(K key, const F& func)
{
    update(range_type({ key, key }), func); // Throws
}


template<class K, class T>
template<class F> void RangeMap<K, T>::update(const range_type& range, const F& func)
{
    ARCHON_ASSERT(range.first <= range.last);
    range_type range_2 = range;

    // INVARIANT: range_2.first <= range_2.last

    auto i = m_map.lower_bound(range_2.first);

    // If `range` has overlap with the end of the range that precedes `i`, split that
    // preceding range into two pieces such that the second piece starts where the incoming
    // range starts.
    if (i != m_map.begin()) {
        auto j = i;
        --j;
        K& j_last = j->second.first;
        if (range_2.first <= j_last) {
            i = m_map.insert(i, std::pair(range_2.first, j->second)); // Throws
            j_last = K(range_2.first - 1);
        }
    }

    for (;;) {
        // ASSUMING: `range` has no overlap with entries before `i`
        if (i == m_map.end())
            break;

        // ASSUMING: `i` starts no earlier than at the start of `range`
        K i_first = i->first;

        if (range_2.last < i_first)
            break;

        if (range_2.first < i_first) {
            K last = K(i_first - 1);
            auto j = m_map.insert(i, std::pair(range_2.first, std::pair(last, T()))); // Throws
            func(j->second.second); // Throws
            range_2.first = i_first;
        }

        // ASSUMING: `range` has overlap with `i`
        K& i_last = i->second.first;
        if (range_2.last < i_last) {
            auto j = i;
            ++j;
            K first = K(range_2.last + 1);
            m_map.insert(j, std::pair(first, i->second)); // Throws
            i_last = range_2.last;
            goto update;
        }

        func(i->second.second); // Throws

        if (range_2.last == i_last)
            return;

        range_2.first = i_last + 1;
        ++i;
    }

    i = m_map.insert(i, std::pair(range_2.first, std::pair(range_2.last, T()))); // Throws

  update:
    func(i->second.second); // Throws
}


template<class K, class T>
inline void RangeMap<K, T>::defrag()
{
    defrag(std::equal_to<T>()); // Throws
}


template<class K, class T>
template<class E> void RangeMap<K, T>::defrag(const E& equal)
{
    auto i = m_map.begin();
    auto end = m_map.end();
    if (ARCHON_LIKELY(i != end)) {
        auto j = i;
        ++j;
        while (ARCHON_LIKELY(j != end)) {
            bool mergeable = (j->first == K(i->second.first + 1) && equal(i->second.second, j->second.second));
            if (ARCHON_LIKELY(!mergeable)) {
                i = j;
                ++j;
                continue;
            }
            // Merge
            i->second.first = j->second.first;
            auto k = j;
            ++k;
            m_map.erase(j);
            j = k;
        }
    }
}


template<class K, class T>
inline RangeMap<K, T>::Proxy::Proxy(const typename underlying_map_type::value_type& entry) noexcept
    : m_entry(entry)
{
}


template<class K, class T>
inline auto RangeMap<K, T>::Proxy::range() const noexcept -> range_type
{
    return { m_entry.first, m_entry.second.first };
}


template<class K, class T>
inline auto RangeMap<K, T>::Proxy::value() const noexcept -> const T&
{
    return m_entry.second.second;
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_RANGE_MAP_HPP
