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

#ifndef ARCHON_X_CORE_X_STRING_SPAN_HPP
#define ARCHON_X_CORE_X_STRING_SPAN_HPP

/// \file


#include <cstddef>
#include <utility>

#include <archon/core/span.hpp>


namespace archon::core {


/// \brief Span of characters safe for use as function parameter.
///
/// This class modifies a span to make it suitible for use as the type of a function
/// parameter that needs to pass a reference to a span of characters.
///
/// It differs from a regular span (\ref core::Span) by not allowing construction from a
/// plain character array, which also means that it cannot be constructed from a string
/// literal. This eliminates the risk of a string literal being interpreted as an array, and
/// therfore having its terminating null included.
///
/// Conversely, it differs from a string view (`std::basic_string_view`) by not allowing
/// construction from a pointer, which also means that it cannot be constructed from an
/// array. This eliminates the risk of an array being interpreted as a string literal, and
/// therfore having internal nulls prematurely terminating the span.
///
/// Just like a regular span, a string span can be implicitely constructed from anything
/// that has suitable `data()` and `size()` methods. This includes regular spans and string
/// views.
///
template<class C> class StringSpan
    : public core::Span<const C> {
public:
    constexpr StringSpan() noexcept = default;

    constexpr StringSpan(const C* data, std::size_t size) noexcept;

    constexpr StringSpan(const C* begin, const C* end) noexcept;

    template<class D, class = decltype(std::declval<D>().data() + std::declval<D>().size())>
    constexpr StringSpan(const D& container) noexcept(noexcept(container.data() + container.size()));
};








// Implementation


template<class C>
constexpr StringSpan<C>::StringSpan(const C* data, std::size_t size) noexcept
    : core::Span<const C>(data, size)
{
}


template<class C>
constexpr StringSpan<C>::StringSpan(const C* begin, const C* end) noexcept
    : core::Span<const C>(begin, end)
{
}


template<class C>
template<class D, class>
constexpr StringSpan<C>::StringSpan(const D& container) noexcept(noexcept(container.data() + container.size()))
    : StringSpan(container.data(), container.size()) // Throws
{
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_STRING_SPAN_HPP
