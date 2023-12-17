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

#ifndef ARCHON_X_CORE_X_PROXY_ITERATOR_HPP
#define ARCHON_X_CORE_X_PROXY_ITERATOR_HPP

/// \file


#include <type_traits>
#include <utility>
#include <iterator>


namespace archon::core {


template<class I, class T> class ProxyIterator {
public:
    static_assert(std::is_nothrow_copy_constructible_v<I>);

    using difference_type   = typename std::iterator_traits<I>::difference_type;
    using value_type        = T;
    using pointer           = void;
    using reference         = T;
    using iterator_category = typename std::iterator_traits<I>::iterator_category;

    explicit ProxyIterator(I) noexcept;

    auto operator*() const noexcept(noexcept(*std::declval<I>())) -> T;
    auto operator[](difference_type i) const noexcept(noexcept(std::declval<I>()[i])) -> T;

    auto operator++() noexcept(noexcept(++std::declval<I>())) -> ProxyIterator&;
    auto operator--() noexcept(noexcept(--std::declval<I>())) -> ProxyIterator&;

    auto operator++(int) noexcept(noexcept(std::declval<I>()++)) -> ProxyIterator;
    auto operator--(int) noexcept(noexcept(std::declval<I>()--)) -> ProxyIterator;

    auto operator+=(difference_type i) noexcept(noexcept(std::declval<I>() += i)) -> ProxyIterator&;
    auto operator-=(difference_type i) noexcept(noexcept(std::declval<I>() -= i)) -> ProxyIterator&;

    auto operator+(difference_type i) const noexcept(noexcept(std::declval<I>() + i)) -> ProxyIterator;
    auto operator-(difference_type i) const noexcept(noexcept(std::declval<I>() - i)) -> ProxyIterator;

    template<class J, class U> auto operator-(const ProxyIterator<J, U>&) const
        noexcept(noexcept(std::declval<I>() - std::declval<J>())) -> difference_type;

    template<class J, class U> bool operator==(const ProxyIterator<J, U>&) const
        noexcept(noexcept(std::declval<I>() == std::declval<J>()));
    template<class J, class U> auto operator<=>(const ProxyIterator<J, U>&) const
        noexcept(noexcept(std::declval<I>() <=> std::declval<J>()));

private:
    I m_iter;
};


template<class I, class T>
auto operator+(typename ProxyIterator<I, T>::difference_type i, const ProxyIterator<I, T>& j)
    noexcept(noexcept(j + i)) -> ProxyIterator<I, T>;








// Implementation


template<class I, class T>
inline ProxyIterator<I, T>::ProxyIterator(I i) noexcept
    : m_iter(i)
{
}


template<class I, class T>
inline auto ProxyIterator<I, T>::operator*() const noexcept(noexcept(*std::declval<I>())) -> T
{
    return T(*m_iter); // Throws
}


template<class I, class T>
inline auto ProxyIterator<I, T>::operator[](difference_type i) const noexcept(noexcept(std::declval<I>()[i])) -> T
{
    return T(m_iter[i]); // Throws
}


template<class I, class T>
inline auto ProxyIterator<I, T>::operator++() noexcept(noexcept(++std::declval<I>())) -> ProxyIterator&
{
    ++m_iter; // Throws
    return *this;
}


template<class I, class T>
inline auto ProxyIterator<I, T>::operator--() noexcept(noexcept(--std::declval<I>())) -> ProxyIterator&
{
    --m_iter; // Throws
    return *this;
}


template<class I, class T>
inline auto ProxyIterator<I, T>::operator++(int) noexcept(noexcept(std::declval<I>()++)) -> ProxyIterator
{
    return ProxyIterator(m_iter++); // Throws
}


template<class I, class T>
inline auto ProxyIterator<I, T>::operator--(int) noexcept(noexcept(std::declval<I>()--)) -> ProxyIterator
{
    return ProxyIterator(m_iter--); // Throws
}


template<class I, class T>
inline auto ProxyIterator<I, T>::operator+=(difference_type i) noexcept(noexcept(std::declval<I>() += i)) ->
    ProxyIterator&
{
    m_iter += i; // Throws
    return *this;
}


template<class I, class T>
inline auto ProxyIterator<I, T>::operator-=(difference_type i) noexcept(noexcept(std::declval<I>() -= i)) ->
    ProxyIterator&
{
    m_iter -= i; // Throws
    return *this;
}


template<class I, class T>
inline auto ProxyIterator<I, T>::operator+(difference_type i) const noexcept(noexcept(std::declval<I>() + i)) ->
    ProxyIterator
{
    return ProxyIterator(m_iter + i); // Throws
}


template<class I, class T>
inline auto ProxyIterator<I, T>::operator-(difference_type i) const noexcept(noexcept(std::declval<I>() - i)) ->
    ProxyIterator
{
    return ProxyIterator(m_iter - i); // Throws
}


template<class I, class T>
template<class J, class U>
inline auto ProxyIterator<I, T>::operator-(const ProxyIterator<J, U>& other) const
    noexcept(noexcept(std::declval<I>() - std::declval<J>())) -> difference_type
{
    return m_iter - other.m_iter; // Throws
}


template<class I, class T>
template<class J, class U>
inline bool ProxyIterator<I, T>::operator==(const ProxyIterator<J, U>& other) const
    noexcept(noexcept(std::declval<I>() == std::declval<J>()))
{
    return (m_iter == other.m_iter); // Throws
}


template<class I, class T>
template<class J, class U>
inline auto ProxyIterator<I, T>::operator<=>(const ProxyIterator<J, U>& other) const
    noexcept(noexcept(std::declval<I>() <=> std::declval<J>()))
{
    return (m_iter <=> other.m_iter); // Throws
}


template<class I, class T>
inline auto operator+(typename ProxyIterator<I, T>::difference_type i, const ProxyIterator<I, T>& j)
    noexcept(noexcept(j + i)) -> ProxyIterator<I, T>
{
    return j + i; // Throws
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_PROXY_ITERATOR_HPP
