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

#ifndef ARCHON_X_CLI_X_OPTION_ACTIONS_HPP
#define ARCHON_X_CLI_X_OPTION_ACTIONS_HPP

/// \file


#include <utility>


namespace archon::cli {


/// \{
///
/// \brief Option actions.
///
/// 
///
auto raise_flag(bool& flag) noexcept;
template<class T> auto assign(T& var);
template<class T, class U> auto assign(T& var, U&& default_arg);
/// \}








// Implementation


namespace detail {


template<class T> struct Assign {
    T* var;
    T default_arg;
};


} // namespace detail


inline auto raise_flag(bool& flag) noexcept
{
    return detail::Assign<bool> { &flag, true };
}


template<class T> inline auto assign(T& var)
{
    return detail::Assign<T> { &var, T() }; // Throws
}


template<class T, class U> inline auto assign(T& var, U&& default_arg)
{
    return detail::Assign<T> { &var, std::forward<U>(default_arg) }; // Throws
}


} // namespace archon::cli

#endif // ARCHON_X_CLI_X_OPTION_ACTIONS_HPP
