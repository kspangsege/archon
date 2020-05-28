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

#ifndef ARCHON__BASE__FORMAT_WITH_HPP
#define ARCHON__BASE__FORMAT_WITH_HPP

#include <ostream>

#include <archon/base/scope_exit.hpp>


namespace archon::base {


namespace detail {
template<class T> struct WithWidth;
template<class T> struct WithPrecision;
} // namespace detail



/// \brief Format value with width.
///
/// Construct an object that, if written to an output stream, formats the
/// specified value with the "width" property of that output stream
/// (`std::ios_base::width()`) temporarily set as specified.
///
/// The specified value must be something that can be written to the
/// outputstream using the `<<` operator.
///
template<class T> detail::WithWidth<T> with_width(const T& value, std::streamsize width) noexcept;


/// \brief Format floating point value with precision.
///
/// Construct an object that, if written to an output stream, formats the
/// specified value with the "precision" property of that output stream
/// (`std::ios_base::precision()`) temporarily set as specified.
///
/// The specified value must be something that can be written to the
/// outputstream using the `<<` operator. It should probably either be a
/// floating point value, or at least something that involves floating point
/// values.
///
template<class T> detail::WithPrecision<T> with_precision(const T& value, int precision) noexcept;








// Implementation


namespace detail {


template<class T> struct WithWidth {
    const T& value;
    std::streamsize width;
};


template<class C, class T, class U>
inline std::basic_ostream<C, T>& operator<<(std::basic_ostream<C, T>& out,
                                            const WithWidth<U>& pod)
{
    auto orig_width = out.width(pod.width);
    ARCHON_SCOPE_EXIT {
        out.width(orig_width);
    };
    out << pod.value; // Throws
    return out;
}


template<class T> struct WithPrecision {
    const T& value;
    int precision;
};


template<class C, class T, class U>
inline std::basic_ostream<C, T>& operator<<(std::basic_ostream<C, T>& out,
                                            const WithPrecision<U>& pod)
{
    auto orig_prec = out.precision(pod.precision);
    ARCHON_SCOPE_EXIT {
        out.precision(orig_prec);
    };
    out << pod.value; // Throws
    return out;
}


} // namespace detail


template<class T>
inline detail::WithWidth<T> with_width(const T& value, std::streamsize width) noexcept
{
    return { value, width };
}


template<class T>
inline detail::WithPrecision<T> with_precision(const T& value, int precision) noexcept
{
    return { value, precision };
}


} // namespace archon::base

#endif // ARCHON__BASE__FORMAT_WITH_HPP
