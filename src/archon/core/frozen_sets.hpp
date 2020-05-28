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

#ifndef ARCHON_X_CORE_X_FROZEN_SETS_HPP
#define ARCHON_X_CORE_X_FROZEN_SETS_HPP

/// \file


#include <cstddef>
#include <type_traits>
#include <iterator>
#include <algorithm>
#include <functional>
#include <initializer_list>
#include <vector>
#include <set>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/index_iterator.hpp>


namespace archon::core {


template<class T, class C = std::less<T>> class FrozenSets {
public:
    class Ident;

    explicit FrozenSets(const C& compare = C());

    auto freeze(const T&) -> Ident;
    auto freeze_unordered(std::initializer_list<T>) -> Ident;
    auto freeze_unordered(core::Span<const T>) -> Ident;
    template<class I> auto freeze_unordered(I begin, I end) -> Ident;
    template<class I> auto freeze_ordered(I begin, I end) -> Ident;
    template<class U, class D, class A> auto freeze(const std::set<U, D, A>&) -> Ident;

    auto unite(Ident, Ident) -> Ident;
    auto unite(Ident, const T&) -> Ident;
    auto unite_unordered(Ident, std::initializer_list<T>) -> Ident;
    auto unite_unordered(Ident, Span<const T>) -> Ident;
    template<class I> auto unite_unordered(Ident, I begin, I end) -> Ident;
    template<class I> auto unite_ordered(Ident, I begin, I end) -> Ident;
    template<class U, class D, class A> auto unite(Ident, const std::set<U, D, A>&) -> Ident;

    auto operator[](Ident) const noexcept -> core::Span<const T>;

    void clear() noexcept;
    void discard_from(Ident) noexcept;

private:
    class Wrap
        : public C {
    public:
        std::vector<T> elems;
        Wrap(const C&);
    };
    Wrap m_wrap;
};




template<class T, class C> class FrozenSets<T, C>::Ident {
public:
    Ident() noexcept = default;

    bool empty() const noexcept;
    auto size() const noexcept -> std::size_t;

private:
    // Designates a range of elements in FrozenSets::m_elems
    std::size_t m_begin = 0;
    std::size_t m_end = 0;

    Ident(std::size_t begin, std::size_t end) noexcept;

    friend class FrozenSets<T, C>;
};








// Implementation


template<class T, class C>
inline FrozenSets<T, C>::FrozenSets(const C& compare)
    : m_wrap(compare) // Throws
{
}


template<class T, class C>
inline auto FrozenSets<T, C>::freeze(const T& elem) -> Ident
{
    return freeze_ordered(&elem, &elem + 1); // Throws
}


template<class T, class C>
inline auto FrozenSets<T, C>::freeze_unordered(std::initializer_list<T> list) -> Ident
{
    return freeze_unordered(list.begin(), list.end()); // Throws
}


template<class T, class C>
inline auto FrozenSets<T, C>::freeze_unordered(Span<const T> span) -> Ident
{
    return freeze_unordered(span.begin(), span.end()); // Throws
}


template<class T, class C>
template<class I> auto FrozenSets<T, C>::freeze_unordered(I begin, I end) -> Ident
{
    std::size_t begin_2 = m_wrap.elems.size();
    try {
        for (I i = begin; i != end; ++i)
            m_wrap.elems.push_back(*i); // Throws
        std::size_t end_2 = m_wrap.elems.size();
        auto base = m_wrap.elems.data();
        std::sort(base + begin_2, base + end_2, m_wrap);
        auto equal = [&](const T& a, const T& b) noexcept {
            const C& less = m_wrap;
            return (!less(a, b) && !less(b, a));
        };
        auto i = std::unique(base + begin_2, base + end_2, equal);
        end_2 = std::size_t(i - base);
        m_wrap.elems.resize(end_2);
        return { begin_2, end_2 };
    }
    catch (...) {
        m_wrap.elems.resize(begin_2);
        throw;
    }
}


template<class T, class C>
template<class I> auto FrozenSets<T, C>::freeze_ordered(I begin, I end) -> Ident
{
    std::size_t begin_2 = m_wrap.elems.size();
    try {
        for (I i = begin; i != end; ++i)
            m_wrap.elems.push_back(*i); // Throws
        std::size_t end_2 = m_wrap.elems.size();
        return { begin_2, end_2 };
    }
    catch (...) {
        m_wrap.elems.resize(begin_2);
        throw;
    }
}


template<class T, class C>
template<class U, class D, class A> inline auto FrozenSets<T, C>::freeze(const std::set<U, D, A>& set) -> Ident
{
    if constexpr (std::is_same_v<C, std::less<T>> && std::is_same_v<D, std::less<U>>)
        return freeze_ordered(set.begin(), set.end()); // Throws
    return freeze_unordered(set.begin(), set.end()); // Throws
}


template<class T, class C>
auto FrozenSets<T, C>::unite(Ident i, Ident j) -> Ident
{
    core::IndexIterator j_begin = { m_wrap.elems, std::ptrdiff_t(j.m_begin) };
    core::IndexIterator j_end   = { m_wrap.elems, std::ptrdiff_t(j.m_end)   };
    Ident k = unite_ordered(i, j_begin, j_end); // Throws
    if (ARCHON_LIKELY(k.size() > j.size()))
        return k;
    discard_from(k);
    return j;
}


template<class T, class C>
inline auto FrozenSets<T, C>::unite(Ident i, const T& elem) -> Ident
{
    return unite_ordered(i, &elem, &elem + 1); // Throws
}


template<class T, class C>
inline auto FrozenSets<T, C>::unite_unordered(Ident i, std::initializer_list<T> list) -> Ident
{
    return unite_unordered(i, list.begin(), list.end()); // Throws
}


template<class T, class C>
inline auto FrozenSets<T, C>::unite_unordered(Ident i, Span<const T> span) -> Ident
{
    return unite_unordered(i, span.begin(), span.end()); // Throws
}


template<class T, class C>
template<class I> auto FrozenSets<T, C>::unite_unordered(Ident i, I begin, I end) -> Ident
{
    std::size_t begin_2 = m_wrap.elems.size();
    try {
        Ident j = freeze_unordered(begin, end); // Throws
        Ident k = unite(i, j); // Throws
        if (ARCHON_LIKELY(k.size() > i.size())) {
            T* base = m_wrap.elems.data();
            std::size_t end_2 = std::size_t(std::copy_n(base + k.m_begin, k.size(), base + begin_2) - base);
            m_wrap.elems.resize(end_2);
            return { begin_2, end_2 };
        }
        m_wrap.elems.resize(begin_2);
        return i;
    }
    catch (...) {
        m_wrap.elems.resize(begin_2);
        throw;
    }
}


template<class T, class C>
template<class I> auto FrozenSets<T, C>::unite_ordered(Ident i, I begin, I end) -> Ident
{
    core::IndexIterator i_begin = { m_wrap.elems, std::ptrdiff_t(i.m_begin) };
    core::IndexIterator i_end   = { m_wrap.elems, std::ptrdiff_t(i.m_end)   };
    std::size_t begin_2 = m_wrap.elems.size();
    try {
        std::set_union(i_begin, i_end, begin, end, std::back_inserter(m_wrap.elems), m_wrap); // Throws
        std::size_t end_2 = m_wrap.elems.size();
        std::size_t size = std::size_t(end_2 - begin_2);
        if (ARCHON_LIKELY(size > i.size()))
            return { begin_2, end_2 };
        m_wrap.elems.resize(begin_2);
        return i;
    }
    catch (...) {
        m_wrap.elems.resize(begin_2);
        throw;
    }
}


template<class T, class C>
template<class U, class D, class A> inline auto FrozenSets<T, C>::unite(Ident i, const std::set<U, D, A>& set) -> Ident
{
    if constexpr (std::is_same_v<C, std::less<T>> && std::is_same_v<D, std::less<U>>)
        return unite_ordered(i, set.begin(), set.end()); // Throws
    return unite_unordered(i, set.begin(), set.end()); // Throws
}


template<class T, class C>
inline auto FrozenSets<T, C>::operator[](Ident i) const noexcept -> Span<const T>
{
    const T* data = m_wrap.elems.data() + i.m_begin;
    std::size_t size = std::size_t(i.m_end - i.m_begin);
    return { data, size };
}

template<class T, class C>
inline void FrozenSets<T, C>::clear() noexcept
{
    m_wrap.elems.clear();
}


template<class T, class C>
inline void FrozenSets<T, C>::discard_from(Ident i) noexcept
{
    std::size_t pos = i.m_begin;
    ARCHON_ASSERT(pos <= m_wrap.elems.size());
    m_wrap.elems.resize(pos);
}


template<class T, class C>
inline FrozenSets<T, C>::Wrap::Wrap(const C& compare)
    : C(compare) // Throws
{
}


template<class T, class C>
inline bool FrozenSets<T, C>::Ident::empty() const noexcept
{
    return (m_begin == m_end);
}


template<class T, class C>
inline auto FrozenSets<T, C>::Ident::size() const noexcept -> std::size_t
{
    return std::size_t(m_end - m_begin);
}


template<class T, class C>
inline FrozenSets<T, C>::Ident::Ident(std::size_t begin, std::size_t end) noexcept
    : m_begin(begin)
    , m_end(end)
{
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_FROZEN_SETS_HPP
