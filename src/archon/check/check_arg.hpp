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

#ifndef ARCHON_X_CHECK_X_CHECK_ARG_HPP
#define ARCHON_X_CHECK_X_CHECK_ARG_HPP

/// \file


#include <string_view>


namespace archon::check {


/// \brief Carrier of information about check macro argument.
///
/// Objects of this type are used to carry information about arguments of a check macro
/// (e.g., \ref ARCHON_CHECK_EQUAL()) to a function such as \ref
/// check::TestContext::check_special_cond().
///
template<class T> class CheckArg {
public:
    /// \brief Construct check argument.
    ///
    /// This constructor constructs a check argument from the specified text string (\p
    /// test) and value reference (\p value). The text string is supposed to be the argument
    /// as it appears in the source code. The value reference must be a reference to the
    /// value of the evaluated check argument.
    ///
    CheckArg(std::string_view text, const T& value) noexcept;

    /// \brief Get check argument as it appears in source code.
    ///
    /// This function returns the check argument as it appears in the source code.
    ///
    auto get_text() const noexcept -> std::string_view;

    /// \brief Get value reference.
    ///
    /// This function returns a reference to the value of the check argument.
    ///
    auto get_value() const noexcept -> const T&;

private:
    std::string_view m_text;
    const T& m_value;
};








// Implementation


template<class T>
inline CheckArg<T>::CheckArg(std::string_view text, const T& value) noexcept
    : m_text(text)
    , m_value(value)
{
}


template<class T>
inline auto CheckArg<T>::get_text() const noexcept -> std::string_view
{
    return m_text;
}


template<class T>
inline auto CheckArg<T>::get_value() const noexcept -> const T&
{
    return m_value;
}


} // namespace archon::check


#endif // ARCHON_X_CHECK_X_CHECK_ARG_HPP
