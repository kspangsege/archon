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

#ifndef ARCHON_X_CORE_X_CONCEPTS_HPP
#define ARCHON_X_CORE_X_CONCEPTS_HPP

/// \file


#include <type_traits>
#include <iterator>
#include <functional>


namespace archon::core {


/// \brief Require iterator type.
///
/// This concept requires that `std::iterator_traits<T>::iterator_category` is convertible
/// to `std::input_iterator_tag`, which means that \p T is an iterator type.
///
template<class T> concept iter_type =
    std::is_convertible_v<typename std::iterator_traits<T>::iterator_category, std::input_iterator_tag>;


/// \brief Require enumeration type.
///
/// This concept requires \p T to be an enumeration type as determined by `std::is_enum<T>`.
///
template<class T> concept enum_type = std::is_enum_v<T>;


/// \brief Type must be function compatible with specific signature.
///
/// This concept requires \p T to be convertible to `std::function<F>`.
///
template<class T, class F> concept func_type = std::is_convertible_v<T, std::function<F>>;


} // namespace archon::core

#endif // ARCHON_X_CORE_X_CONCEPTS_HPP
