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

#ifndef ARCHON_X_CORE_X_SPAN_HPP
#define ARCHON_X_CORE_X_SPAN_HPP

/// \file


#include <cstddef>
#include <type_traits>
#include <iterator>
#include <array>


namespace archon::core {


template<class T> class Span {
public:
    using element_type = T;
    using value_type = std::remove_cv_t<T>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using iterator = pointer;
    using const_iterator = const_pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    constexpr Span() noexcept = default;
    constexpr Span(pointer data, size_type size) noexcept;
    constexpr Span(pointer begin, pointer end) noexcept;
    template<std::size_t N> constexpr Span(T (&)[N]) noexcept;
    template<class C, class = decltype(std::declval<C>().data() + std::declval<C>().size())>
    constexpr Span(C& container) noexcept(noexcept(container.data() + container.size()));
    template<class C, class = decltype(std::declval<C>().data() + std::declval<C>().size())>
    constexpr Span(const C& container) noexcept(noexcept(container.data() + container.size()));
    template<class U> constexpr Span(const Span<U>&) noexcept;
    constexpr Span(const Span&) noexcept = default;

    constexpr auto operator=(const Span&) noexcept -> Span& = default;

    constexpr auto begin() const noexcept -> iterator;
    constexpr auto end() const noexcept -> iterator;
    constexpr auto cbegin() const noexcept -> const_iterator;
    constexpr auto cend() const noexcept -> const_iterator;

    constexpr auto rbegin() const noexcept -> reverse_iterator;
    constexpr auto rend() const noexcept -> reverse_iterator;
    constexpr auto crbegin() const noexcept -> const_reverse_iterator;
    constexpr auto crend() const noexcept -> const_reverse_iterator;

    constexpr auto operator[](size_type) const noexcept -> reference;
    constexpr auto front() const noexcept -> reference;
    constexpr auto back() const noexcept -> reference;

    constexpr auto data() const noexcept -> pointer;
    constexpr auto size() const noexcept -> size_type;
    constexpr bool empty() const noexcept;

    constexpr auto first(size_type size) const noexcept -> Span<element_type>;
    constexpr auto last(size_type size) const noexcept -> Span<element_type>;
    constexpr auto subspan(size_type offset, size_type size = std::size_t(-1)) const noexcept -> Span<element_type>;

private:
    pointer m_data = nullptr;
    size_type m_size = 0;
};


template<class T, std::size_t N> Span(std::array<T, N>&) -> Span<T>;
template<class T, std::size_t N> Span(const std::array<T, N>&) -> Span<const T>;
template<class C> Span(C&) -> Span<std::remove_reference_t<decltype(*std::declval<C>().data())>>;
template<class C> Span(const C&) -> Span<std::remove_reference_t<decltype(*std::declval<const C>().data())>>;








// Implementation


template<class T>
constexpr Span<T>::Span(pointer data, size_type size) noexcept
    : m_data(data)
    , m_size(size)
{
}


template<class T>
constexpr Span<T>::Span(pointer begin, pointer end) noexcept
    : Span(begin, std::size_t(end - begin))
{
}


template<class T>
template<std::size_t N> constexpr Span<T>::Span(T (& array)[N]) noexcept
    : Span(array, N)
{
}


template<class T>
template<class C, class>
constexpr Span<T>::Span(C& container) noexcept(noexcept(container.data() + container.size()))
    : Span(container.data(), container.size()) // Throws
{
}


template<class T>
template<class C, class>
constexpr Span<T>::Span(const C& container) noexcept(noexcept(container.data() + container.size()))
    : Span(container.data(), container.size()) // Throws
{
}


template<class T>
template<class U> constexpr Span<T>::Span(const Span<U>& other) noexcept
    : Span(other.data(), other.size())
{
}


template<class T>
constexpr auto Span<T>::begin() const noexcept -> iterator
{
    return m_data;
}


template<class T>
constexpr auto Span<T>::end() const noexcept -> iterator
{
    return m_data + m_size;
}


template<class T>
constexpr auto Span<T>::cbegin() const noexcept -> const_iterator
{
    return begin();
}


template<class T>
constexpr auto Span<T>::cend() const noexcept -> const_iterator
{
    return end();
}


template<class T>
constexpr auto Span<T>::rbegin() const noexcept -> reverse_iterator
{
    return const_reverse_iterator(end());
}


template<class T>
constexpr auto Span<T>::rend() const noexcept -> reverse_iterator
{
    return const_reverse_iterator(begin());
}


template<class T>
constexpr auto Span<T>::crbegin() const noexcept -> const_reverse_iterator
{
    return rbegin();
}


template<class T>
constexpr auto Span<T>::crend() const noexcept -> const_reverse_iterator
{
    return rend();
}


template<class T>
constexpr auto Span<T>::operator[](size_type i) const noexcept -> reference
{
    return m_data[i];
}


template<class T>
constexpr auto Span<T>::front() const noexcept -> reference
{
    return m_data[0];
}


template<class T>
constexpr auto Span<T>::back() const noexcept -> reference
{
    return m_data[m_size - 1];
}


template<class T>
constexpr auto Span<T>::data() const noexcept -> pointer
{
    return m_data;
}


template<class T>
constexpr auto Span<T>::size() const noexcept -> size_type
{
    return m_size;
}


template<class T>
constexpr bool Span<T>::empty() const noexcept
{
    return (size() == 0);
}


template<class T>
constexpr auto Span<T>::first(size_type size) const noexcept -> Span<element_type>
{
    return { m_data, size };
}


template<class T>
constexpr auto Span<T>::last(size_type size) const noexcept -> Span<element_type>
{
    return { m_data + (m_size - size), size };
}


template<class T>
constexpr auto Span<T>::subspan(size_type offset, size_type size) const noexcept -> Span<element_type>
{
    std::size_t size_2 = size;
    if (size_2 == std::size_t(-1))
        size_2 = std::size_t(m_size - offset);
    return { m_data + offset, size_2 };
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_SPAN_HPP
