// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2023 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_CHECK_X_IMPL_X_CHECK_ARG_HPP
#define ARCHON_X_CHECK_X_IMPL_X_CHECK_ARG_HPP

/// \file


#include <string_view>


namespace archon::check::impl {


template<class T, bool F> class CheckArg;


// Case 1 of 2: Non-formattable
template<class T> class CheckArg<T, false> {
public:
    CheckArg(std::string_view text, const T&) noexcept
        : m_text(text)
    {
    }

    auto get_text() const noexcept -> std::string_view
    {
        return m_text;
    }

private:
    std::string_view m_text;
};


// Case 2 of 2: Formattable
template<class T> class CheckArg<T, true> {
public:
    CheckArg(std::string_view text, const T& value) noexcept
        : m_text(text)
        , m_value(value)
    {
    }

    auto get_text() const noexcept -> std::string_view
    {
        return m_text;
    }

    auto get_value() const noexcept -> const T&
    {
        return m_value;
    }

private:
    std::string_view m_text;
    const T& m_value;
};


} // namespace archon::check::impl


#endif // ARCHON_X_CHECK_X_IMPL_X_CHECK_ARG_HPP
