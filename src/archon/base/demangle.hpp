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


/// \file

#ifndef ARCHON__BASE__DEMANGLE_HPP
#define ARCHON__BASE__DEMANGLE_HPP

#include <typeinfo>
#include <string>
#include <iostream>

#include <archon/base/type_traits.hpp>


namespace archon::base {


/// \brief Try to demangle the specified C++ ABI identifier.
///
/// If demangling is supported on this platform, and if the specified string is
/// a valid mangled name, this function returns the unmangled name. Otherwise it
/// returns the mangled name.
///
std::string demangle(const char* mangled_name);


/// \brief Get name of specified type.
///
/// Get the name of the specified type. Demangling occurs as if by \ref
/// demangle().
///
template<class T> std::string get_type_name();


/// \brief Get name of type of specified value.
///
/// Get the name of the type of the specified value. Demangling occurs as if by
/// \ref demangle().
///
template<class T> std::string get_type_name(const T&);


/// \brief Get wrapped name of specified type.
///
/// Get the name of the type resulting from wrapping the specified type using
/// \ref base::Wrap. Demangling occurs as if by \ref demangle().
///
template<class T> std::string get_wrapped_type_name();


/// \brief Dump wrapped name of specified type.
///
/// Write the result of \ref get_wrapped_type_name() to `std::cerr`.
///
template<class T> void dump_wrapped_type_name();








// Implementation


template<class T> inline std::string get_type_name()
{
    return demangle(typeid(T).name()); // Throws
}


template<class T> inline std::string get_type_name(const T& v)
{
    return demangle(typeid(v).name()); // Throws
}


template<class T> std::string get_wrapped_type_name()
{
    return get_type_name<Wrap<T>>(); // Throws
}


template<class T> void dump_wrapped_type_name()
{
    std::cerr << get_wrapped_type_name<T>() << "\n"; // Throws
}


} // namespace archon::base

#endif // ARCHON__BASE__DEMANGLE_HPP
