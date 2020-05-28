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

#ifndef ARCHON__BASE__INDEX_ITERATOR_HPP
#define ARCHON__BASE__INDEX_ITERATOR_HPP

#include <type_traits>
#include <utility>
#include <iterator>

#include <archon/base/type_traits.hpp>


namespace archon::base {


template<class C> class IndexIterator {
public:
    using container_type    = std::remove_reference_t<C>;
    using difference_type   = std::ptrdiff_t;
    using value_type        = CopyConst<container_type, typename C::value_type>;
    using pointer           = decltype(&std::declval<C>()[std::ptrdiff_t()]);
    using reference         = decltype(std::declval<C>()[std::ptrdiff_t()]);
    using iterator_category = std::random_access_iterator_tag;

    IndexIterator(C& container, std::ptrdiff_t index) noexcept;

    pointer operator->() const noexcept(noexcept(&std::declval<C>()[std::ptrdiff_t()]));
    reference operator*() const noexcept(noexcept(std::declval<C>()[std::ptrdiff_t()]));
    reference operator[](std::ptrdiff_t) const
        noexcept(noexcept(std::declval<C>()[std::ptrdiff_t()]));

    IndexIterator& operator++() noexcept;
    IndexIterator& operator--() noexcept;

    IndexIterator operator++(int) noexcept;
    IndexIterator operator--(int) noexcept;

    IndexIterator& operator+=(std::ptrdiff_t i) noexcept;
    IndexIterator& operator-=(std::ptrdiff_t i) noexcept;

    IndexIterator operator+(std::ptrdiff_t i) const noexcept;
    IndexIterator operator-(std::ptrdiff_t i) const noexcept;

    template<class D> std::ptrdiff_t operator-(const IndexIterator<D>&) const noexcept;

    template<class D> bool operator==(const IndexIterator<D>&) const noexcept;
    template<class D> bool operator!=(const IndexIterator<D>&) const noexcept;
    template<class D> bool operator<(const IndexIterator<D>&) const noexcept;
    template<class D> bool operator>(const IndexIterator<D>&) const noexcept;
    template<class D> bool operator<=(const IndexIterator<D>&) const noexcept;
    template<class D> bool operator>=(const IndexIterator<D>&) const noexcept;

private:
    C* m_container;
    std::ptrdiff_t m_index;
};


template<class C> IndexIterator<C> operator+(std::ptrdiff_t, const IndexIterator<C>&) noexcept;








// Implementation


template<class C>
inline IndexIterator<C>::IndexIterator(C& container, std::ptrdiff_t index) noexcept :
    m_container(&container),
    m_index(index)
{
}


template<class C> inline auto IndexIterator<C>::operator->() const
    noexcept(noexcept(&std::declval<C>()[std::ptrdiff_t()])) -> pointer
{
    return &(*m_container)[m_index];
}


template<class C> inline auto IndexIterator<C>::operator*() const
    noexcept(noexcept(std::declval<C>()[std::ptrdiff_t()])) -> reference
{
    return (*m_container)[m_index];
}


template<class C> inline auto IndexIterator<C>::operator[](std::ptrdiff_t i) const
    noexcept(noexcept(std::declval<C>()[std::ptrdiff_t()])) -> reference
{
    return (*m_container)[m_index + i];
}


template<class C> inline auto IndexIterator<C>::operator++() noexcept -> IndexIterator&
{
    ++m_index;
    return *this;
}


template<class C> inline auto IndexIterator<C>::operator--() noexcept -> IndexIterator&
{
    --m_index;
    return *this;
}


template<class C> inline auto IndexIterator<C>::operator++(int) noexcept -> IndexIterator
{
    return IndexIterator(*m_container, m_index++);
}


template<class C> inline auto IndexIterator<C>::operator--(int) noexcept -> IndexIterator
{
    return IndexIterator(*m_container, m_index--);
}


template<class C>
inline auto IndexIterator<C>::operator+=(std::ptrdiff_t i) noexcept -> IndexIterator&
{
    m_index += i;
    return *this;
}


template<class C>
inline auto IndexIterator<C>::operator-=(std::ptrdiff_t i) noexcept -> IndexIterator&
{
    m_index -= i;
    return *this;
}


template<class C>
inline auto IndexIterator<C>::operator+(std::ptrdiff_t i) const noexcept -> IndexIterator
{
    return IndexIterator(*m_container, m_index + i);
}


template<class C>
inline auto IndexIterator<C>::operator-(std::ptrdiff_t i) const noexcept -> IndexIterator
{
    return IndexIterator(*m_container, m_index - i);
}


template<class C> template<class D>
inline std::ptrdiff_t IndexIterator<C>::operator-(const IndexIterator<D>& other) const noexcept
{
    return std::ptrdiff_t(m_index - other.m_index);
}


template<class C> template<class D>
inline bool IndexIterator<C>::operator==(const IndexIterator<D>& other) const noexcept
{
    return (m_index == other.m_index);
}


template<class C> template<class D>
inline bool IndexIterator<C>::operator!=(const IndexIterator<D>& other) const noexcept
{
    return (m_index != other.m_index);
}


template<class C> template<class D>
inline bool IndexIterator<C>::operator<(const IndexIterator<D>& other) const noexcept
{
    return (m_index < other.m_index);
}


template<class C> template<class D>
inline bool IndexIterator<C>::operator>(const IndexIterator<D>& other) const noexcept
{
    return (m_index > other.m_index);
}


template<class C> template<class D>
inline bool IndexIterator<C>::operator<=(const IndexIterator<D>& other) const noexcept
{
    return (m_index <= other.m_index);
}


template<class C> template<class D>
inline bool IndexIterator<C>::operator>=(const IndexIterator<D>& other) const noexcept
{
    return (m_index >= other.m_index);
}


template<class C>
inline IndexIterator<C> operator+(std::ptrdiff_t i, const IndexIterator<C>& j) noexcept
{
    return j + i;
}


} // namespace archon::base

#endif // ARCHON__BASE__INDEX_ITERATOR_HPP
