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

#ifndef ARCHON_X_CORE_X_TYPE_HPP
#define ARCHON_X_CORE_X_TYPE_HPP

/// \file


#include <compare>


namespace archon::core {


/// \brief Empty class type.
///
/// This is an empty class type.
///
/// \sa \ref core::Wrap
///
struct Empty {
    constexpr auto operator<=>(const Empty&) const noexcept = default;
};


/// \brief Empty class template for type wrapping.
///
/// This is an empty class template with one type parameter. It is similar in purpose to
/// `std::identity_type`, but is shorter. Its uses include the following:
///
///  * Creation of non-deduced context for template argument deduction (see \ref
///    core::Type).
///
///  * Passage of a type into a generic lambda (see \ref core::for_each_type()).
///
///  * Preservation of exact type (see \ref core::get_wrapped_type_name()).
///
/// \sa \ref core::Empty
/// \sa \ref core::Type
///
template<class T> struct Wrap {
    using type = T;
};


/// \brief Identity type alias for creation of non-deduced contexts.
///
/// This is an identity type alias, which means that `core::Type<T>` is `T` in all
/// cases. The purpose of this type alias is to create non-deduced contexts in template
/// argument deduction. For an example, consider the following function template:
///
/// \code{.cpp}
///
///   template<class T> void func(T a, core::Type<T> b);
///
/// \endcode
///
/// Here, both arguments are of type \p T, but the deduction of \p T ignores the second
/// argument (\p b). A a consequence, \p T is deduced just from the first argument (\p a).
///
/// This type alias is similar in purpose to `std::identity_type_t`.
///
/// \sa \ref core::Wrap
///
template<class T> using Type = typename core::Wrap<T>::type;


} // namespace archon::core

#endif // ARCHON_X_CORE_X_TYPE_HPP
