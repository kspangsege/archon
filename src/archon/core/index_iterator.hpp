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

#ifndef ARCHON_X_CORE_X_INDEX_ITERATOR_HPP
#define ARCHON_X_CORE_X_INDEX_ITERATOR_HPP

/// \file


#include <type_traits>
#include <compare>
#include <utility>
#include <iterator>


namespace archon::core {


template<class C> struct IndexIteratorSubscr {
    static auto subscr(C& c, std::ptrdiff_t i) noexcept(noexcept(c[i])) -> decltype(auto);
};




template<class C, class S = core::IndexIteratorSubscr<C>> class IndexIterator {
public:
    using container_type    = C;
    using difference_type   = std::ptrdiff_t;
    using reference         = decltype(S::subscr(std::declval<C&>(), 0));
    using value_type        = std::remove_reference_t<reference>;
    using pointer           = value_type*;
    using iterator_category = std::random_access_iterator_tag;

    IndexIterator(C& container, std::ptrdiff_t index) noexcept;

    auto operator->() const noexcept(noexcept(&S::subscr(std::declval<C&>(), 0))) -> pointer;
    auto operator*() const noexcept(noexcept(S::subscr(std::declval<C&>(), 0))) -> reference;
    auto operator[](std::ptrdiff_t) const noexcept(noexcept(S::subscr(std::declval<C&>(), 0))) -> reference;

    auto operator++() noexcept -> IndexIterator&;
    auto operator--() noexcept -> IndexIterator&;

    auto operator++(int) noexcept -> IndexIterator;
    auto operator--(int) noexcept -> IndexIterator;

    auto operator+=(std::ptrdiff_t i) noexcept -> IndexIterator&;
    auto operator-=(std::ptrdiff_t i) noexcept -> IndexIterator&;

    auto operator+(std::ptrdiff_t i) const noexcept -> IndexIterator;
    auto operator-(std::ptrdiff_t i) const noexcept -> IndexIterator;

    template<class D, class T> auto operator-(const IndexIterator<D, T>&) const noexcept -> std::ptrdiff_t;

    template<class D, class T> bool operator==(const IndexIterator<D, T>&) const noexcept;
    template<class D, class T> auto operator<=>(const IndexIterator<D, T>&) const noexcept -> std::weak_ordering;

private:
    C* m_container;
    std::ptrdiff_t m_index;
};


template<class C, class S> auto operator+(std::ptrdiff_t, const IndexIterator<C, S>&) noexcept -> IndexIterator<C, S>;








// Implementation


template<class C>
inline auto IndexIteratorSubscr<C>::subscr(C& c, std::ptrdiff_t i) noexcept(noexcept(c[i])) -> decltype(auto)
{
    return c[i]; // Throws
}


template<class C, class S>
inline IndexIterator<C, S>::IndexIterator(C& container, std::ptrdiff_t index) noexcept
    : m_container(&container)
    , m_index(index)
{
}


template<class C, class S>
inline auto IndexIterator<C, S>::operator->() const noexcept(noexcept(&S::subscr(std::declval<C&>(), 0))) -> pointer
{
    return &(*m_container)[m_index];
}


template<class C, class S>
inline auto IndexIterator<C, S>::operator*() const noexcept(noexcept(S::subscr(std::declval<C&>(), 0))) -> reference
{
    return (*m_container)[m_index];
}


template<class C, class S>
inline auto IndexIterator<C, S>::operator[](std::ptrdiff_t i) const
    noexcept(noexcept(S::subscr(std::declval<C&>(), 0))) -> reference
{
    return (*m_container)[m_index + i];
}


template<class C, class S>
inline auto IndexIterator<C, S>::operator++() noexcept -> IndexIterator&
{
    ++m_index;
    return *this;
}


template<class C, class S>
inline auto IndexIterator<C, S>::operator--() noexcept -> IndexIterator&
{
    --m_index;
    return *this;
}


template<class C, class S>
inline auto IndexIterator<C, S>::operator++(int) noexcept -> IndexIterator
{
    return IndexIterator(*m_container, m_index++);
}


template<class C, class S>
inline auto IndexIterator<C, S>::operator--(int) noexcept -> IndexIterator
{
    return IndexIterator(*m_container, m_index--);
}


template<class C, class S>
inline auto IndexIterator<C, S>::operator+=(std::ptrdiff_t i) noexcept -> IndexIterator&
{
    m_index += i;
    return *this;
}


template<class C, class S>
inline auto IndexIterator<C, S>::operator-=(std::ptrdiff_t i) noexcept -> IndexIterator&
{
    m_index -= i;
    return *this;
}


template<class C, class S>
inline auto IndexIterator<C, S>::operator+(std::ptrdiff_t i) const noexcept -> IndexIterator
{
    return IndexIterator(*m_container, m_index + i);
}


template<class C, class S>
inline auto IndexIterator<C, S>::operator-(std::ptrdiff_t i) const noexcept -> IndexIterator
{
    return IndexIterator(*m_container, m_index - i);
}


template<class C, class S>
template<class D, class T>
inline auto IndexIterator<C, S>::operator-(const IndexIterator<D, T>& other) const noexcept -> std::ptrdiff_t
{
    return std::ptrdiff_t(m_index - other.m_index);
}


template<class C, class S>
template<class D, class T> inline bool IndexIterator<C, S>::operator==(const IndexIterator<D, T>& other) const noexcept
{
    return m_index == other.m_index;
}


template<class C, class S>
template<class D, class T>
inline auto IndexIterator<C, S>::operator<=>(const IndexIterator<D, T>& other) const noexcept -> std::weak_ordering
{
    return m_index <=> other.m_index;
}


template<class C, class S>
inline auto operator+(std::ptrdiff_t i, const IndexIterator<C, S>& j) noexcept -> IndexIterator<C, S>
{
    return j + i;
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_INDEX_ITERATOR_HPP
