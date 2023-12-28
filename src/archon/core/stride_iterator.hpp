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

#ifndef ARCHON_X_CORE_X_STRIDE_ITERATOR_HPP
#define ARCHON_X_CORE_X_STRIDE_ITERATOR_HPP

/// \file


#include <type_traits>
#include <iterator>


namespace archon::core {


template<class I, std::size_t S> class StrideIterator {
public:
    static_assert(std::is_nothrow_copy_constructible_v<I>);
    static_assert(std::is_nothrow_copy_assignable_v<I>);

    using difference_type   = typename std::iterator_traits<I>::difference_type;
    using value_type        = typename std::iterator_traits<I>::value_type;
    using pointer           = typename std::iterator_traits<I>::pointer;
    using reference         = typename std::iterator_traits<I>::reference;
    using iterator_category = typename std::iterator_traits<I>::iterator_category;

    StrideIterator(const I& iter) noexcept;

    auto operator->() const noexcept(noexcept(std::declval<const I&>().operator->())) -> pointer;
    auto operator*() const noexcept(noexcept(*std::declval<const I&>())) -> reference;
    auto operator[](std::ptrdiff_t i) const noexcept(noexcept(std::declval<const I&>()[i * S])) -> reference;

    auto operator++() noexcept(noexcept(std::declval<I&>() += S)) -> StrideIterator&;
    auto operator--() noexcept(noexcept(std::declval<I&>() -= S)) -> StrideIterator&;

    auto operator++(int) noexcept(noexcept(std::declval<I&>() += S)) -> StrideIterator;
    auto operator--(int) noexcept(noexcept(std::declval<I&>() -= S)) -> StrideIterator;

    auto operator+=(std::ptrdiff_t i) noexcept(noexcept(std::declval<I&>() += i * S)) -> StrideIterator&;
    auto operator-=(std::ptrdiff_t i) noexcept(noexcept(std::declval<I&>() -= i * S)) -> StrideIterator&;

    auto operator+(std::ptrdiff_t i) const noexcept(noexcept(std::declval<const I&>() + i * S)) -> StrideIterator;
    auto operator-(std::ptrdiff_t i) const noexcept(noexcept(std::declval<const I&>() - i * S)) -> StrideIterator;

    template<class J> auto operator-(const StrideIterator<J, S>&) const
        noexcept(noexcept((std::declval<const I&>() - std::declval<const J&>()) / S)) -> difference_type;

    template<class J> bool operator==(const StrideIterator<J, S>&) const
        noexcept(noexcept(std::declval<const I&>() == std::declval<const J&>()));
    template<class J> auto operator<=>(const StrideIterator<J, S>&) const
        noexcept(noexcept(std::declval<const I&>() <=> std::declval<const J&>()));

private:
    I m_iter;
};


template<class I, std::size_t S>
auto operator+(typename StrideIterator<I, S>::difference_type i, const StrideIterator<I, S>& j)
    noexcept(noexcept(j + i)) -> StrideIterator<I, S>;


template<std::size_t S, class I> auto make_stride_iterator(const I& iter) noexcept -> StrideIterator<I, S>;








// Implementation


template<class I, std::size_t S>
inline StrideIterator<I, S>::StrideIterator(const I& iter) noexcept
    : m_iter(iter)
{
}


template<class I, std::size_t S>
inline auto StrideIterator<I, S>::operator->() const noexcept(noexcept(std::declval<const I&>().operator->())) ->
    pointer
{
    return m_iter.operator->(); // Throws
}


template<class I, std::size_t S>
inline auto StrideIterator<I, S>::operator*() const noexcept(noexcept(*std::declval<const I&>())) -> reference
{
    return *m_iter; // Throws
}


template<class I, std::size_t S>
inline auto StrideIterator<I, S>::operator[](std::ptrdiff_t i) const
    noexcept(noexcept(std::declval<const I&>()[i * S])) -> reference
{
    return m_iter[i * S]; // Throws
}


template<class I, std::size_t S>
inline auto StrideIterator<I, S>::operator++() noexcept(noexcept(std::declval<I&>() += S)) -> StrideIterator&
{
    m_iter += S; // Throws
    return *this;
}


template<class I, std::size_t S>
inline auto StrideIterator<I, S>::operator--() noexcept(noexcept(std::declval<I&>() -= S)) -> StrideIterator&
{
    m_iter -= S; // Throws
    return *this;
}


template<class I, std::size_t S>
inline auto StrideIterator<I, S>::operator++(int) noexcept(noexcept(std::declval<I&>() += S)) -> StrideIterator
{
    I i = m_iter;
    m_iter += S; // Throws
    return { i };
}


template<class I, std::size_t S>
inline auto StrideIterator<I, S>::operator--(int) noexcept(noexcept(std::declval<I&>() -= S)) -> StrideIterator
{
    I i = m_iter;
    m_iter -= S; // Throws
    return { i };
}


template<class I, std::size_t S>
inline auto StrideIterator<I, S>::operator+=(std::ptrdiff_t i) noexcept(noexcept(std::declval<I&>() += i * S)) ->
    StrideIterator&
{
    m_iter += i * S; // Throws
    return *this;
}


template<class I, std::size_t S>
inline auto StrideIterator<I, S>::operator-=(std::ptrdiff_t i) noexcept(noexcept(std::declval<I&>() -= i * S)) ->
    StrideIterator&
{
    m_iter -= i * S; // Throws
    return *this;
}


template<class I, std::size_t S>
inline auto StrideIterator<I, S>::operator+(std::ptrdiff_t i) const
    noexcept(noexcept(std::declval<const I&>() + i * S)) -> StrideIterator
{
    return { m_iter + i * S }; // Throws
}


template<class I, std::size_t S>
inline auto StrideIterator<I, S>::operator-(std::ptrdiff_t i) const
    noexcept(noexcept(std::declval<const I&>() - i * S)) -> StrideIterator
{
    return { m_iter - i * S }; // Throws
}


template<class I, std::size_t S>
template<class J> inline auto StrideIterator<I, S>::operator-(const StrideIterator<J, S>& other) const
    noexcept(noexcept((std::declval<const I&>() - std::declval<const J&>()) / S)) -> difference_type
{
    return difference_type((m_iter - other.m_iter) / S); // Throws
}


template<class I, std::size_t S>
template<class J> inline bool StrideIterator<I, S>::operator==(const StrideIterator<J, S>& other) const
    noexcept(noexcept(std::declval<const I&>() == std::declval<const J&>()))
{
    return m_iter == other.m_iter; // Throws
}


template<class I, std::size_t S>
template<class J> inline auto StrideIterator<I, S>::operator<=>(const StrideIterator<J, S>& other) const
    noexcept(noexcept(std::declval<const I&>() <=> std::declval<const J&>()))
{
    return m_iter <=> other.m_iter; // Throws
}


template<class I, std::size_t S>
auto operator+(typename StrideIterator<I, S>::difference_type i, const StrideIterator<I, S>& j)
    noexcept(noexcept(j + i)) -> StrideIterator<I, S>
{
    return j + i; // Throws
}


template<class I, std::size_t S> auto make_stride_iterator(const I& iter) noexcept -> StrideIterator<I, S>
{
    return { iter };
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_STRIDE_ITERATOR_HPP
