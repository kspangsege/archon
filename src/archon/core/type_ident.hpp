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

#ifndef ARCHON_X_CORE_X_TYPE_IDENT_HPP
#define ARCHON_X_CORE_X_TYPE_IDENT_HPP

/// \file


#include <stdexcept>

#include <archon/core/features.h>
#include <archon/core/impl/type_ident_impl.hpp>


namespace archon::core {


/// \brief Type of type identifier.
///
/// This is the type of a type identifier as returned by \ref core::get_type_ident(). It is
/// always an integer type.
///
using type_ident_type = impl::type_ident_type;


/// \brief Get integer identifier for type.
///
/// This function is a shorthand for calling \ref core::try_get_type_ident(), and then
/// throwing a `std::length_error` if a type identifier could not be obtained.
///
template<class T> auto get_type_ident() -> type_ident_type;


/// \brief Try to get integer identifier for type.
///
/// This function attempts to obtain an integer identifier for the specified type. This
/// operation can only fail in extreme cases, such as when the number of previously obtained
/// distinct type identifiers exceeds the maximum representable value in \ref
/// type_ident_type.
///
/// On success, this function returns `true` after setting \p ident to the obtained
/// identifier. On failure, this function returns `false` and leaves \p ident unchanged.
///
/// \sa \ref core::get_type_ident()
///
template<class T> bool try_get_type_ident(type_ident_type& ident) noexcept;








// Implementation


template<class T> auto get_type_ident() -> type_ident_type
{
    type_ident_type ident = 0;
    if (ARCHON_LIKELY(core::try_get_type_ident<T>(ident)))
        return ident;
    throw std::length_error("Too many type identifiers");
}


template<class T> bool try_get_type_ident(type_ident_type& ident) noexcept
{
    return impl::try_get_type_ident<T>(ident);
}

} // namespace archon::core

#endif // ARCHON_X_CORE_X_TYPE_IDENT_HPP
