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


/// \brief Construct CheckArg object for check macro argument.
///
/// The purpose of this macro is to make it easy to construct objects of type \ref
/// check::CheckArg. See \ref check::TestContext::check_special_cond() for an example of its
/// intended use.
///
#define ARCHON_CHECK_ARG(arg) archon::check::make_check_arg(#arg, arg)


namespace archon::check {


/// \brief Carrier of information about check macro argument.
///
/// An object of this type is intended to carry information about an argument of a check
/// macro (e.g., \ref ARCHON_CHECK_EQUAL()) to a function such as \ref
/// check::TestContext::check_special_cond().
///
/// Ordinarily, objects of this type will be created using \ref ARCHON_CHECK_ARG().
///
template<class T> struct CheckArg {
    std::string_view text;
    const T& value;
};


/// \brief Helper for creation of CheckArg objects.
///
/// The purpose of this function is to make it easier to construct objects of type \ref
/// check::CheckArg. It does that be allowing for the template argument to be deduced. This
/// function is used by \ref ARCHON_CHECK_ARG.
///
template<class T> auto make_check_arg(std::string_view text, const T& value) noexcept -> CheckArg<T>;








// Implementation


template<class T> auto make_check_arg(std::string_view text, const T& value) noexcept -> CheckArg<T>
{
    return { text, value };
}


} // namespace archon::check


#endif // ARCHON_X_CHECK_X_CHECK_ARG_HPP
