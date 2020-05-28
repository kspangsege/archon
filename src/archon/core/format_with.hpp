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

#ifndef ARCHON_X_CORE_X_FORMAT_WITH_HPP
#define ARCHON_X_CORE_X_FORMAT_WITH_HPP

/// \file


#include <ostream>


namespace archon::core {


/// \brief Format value with width.
///
/// Construct an object that, if written to an output stream, formats the specified value
/// with the "width" property of that output stream (`std::ios_base::width()`) temporarily
/// set as specified.
///
/// The specified value must be something that can be written to the output stream using the
/// `<<` operator.
///
template<class T> auto with_width(const T& value, std::streamsize width) noexcept;


/// \brief Format floating point value with precision.
///
/// Construct an object that, if written to an output stream, formats the specified value
/// with the "precision" property of that output stream (`std::ios_base::precision()`)
/// temporarily set as specified.
///
/// The specified value must be something that can be written to the output stream using the
/// `<<` operator. It should probably either be a floating point value, or something that
/// involves the formatting of floating point values.
///
template<class T> auto with_precision(const T& value, int precision) noexcept;


/// \brief Format floating point value using 'fixed' notation.
///
/// Construct an object that, if written to an output stream, formats the specified value
/// using the 'fixed' floating point notation (`std::ios_base::fixed`), and with the
/// specified precision (as if through \ref core::with_precision()). When using the fixed
/// floating point notation, the precision is the number of digits after the decimal point.
///
/// The specified value must be something that can be written to the output stream using the
/// `<<` operator. It should probably either be a floating point value, or something that
/// involves the formatting of floating point values.
///
template<class T> auto with_fixed(const T& value, int precision) noexcept;








// Implementation


namespace impl {


template<class C, class T, class R>
inline auto with_width(std::basic_ostream<C, T>& out, std::streamsize width, const R& ref) -> std::basic_ostream<C, T>&
{
    std::basic_ostream<C, T> out_2(out.rdbuf()); // Throws
    out_2.copyfmt(out); // Throws
    out_2.width(width); // Throws
    out_2.clear(out.rdstate()); // Throws
    out_2 << ref; // Throws
    out.clear(out_2.rdstate()); // Throws
    return out;
}


template<class V> struct WithWidth {
    const V& value;
    std::streamsize width;
};


template<class C, class T, class U>
auto operator<<(std::basic_ostream<C, T>& out, const impl::WithWidth<U>& pod) -> std::basic_ostream<C, T>&
{
    return impl::with_width(out, pod.width, pod.value); // Throws
}


template<class C, class T, class R>
inline auto with_precision(std::basic_ostream<C, T>& out, int prec, const R& ref) -> std::basic_ostream<C, T>&
{
    std::basic_ostream<C, T> out_2(out.rdbuf()); // Throws
    out_2.copyfmt(out); // Throws
    out_2.precision(prec); // Throws
    out_2.clear(out.rdstate()); // Throws
    out_2 << ref; // Throws
    out.clear(out_2.rdstate()); // Throws
    return out;
}


template<class V> struct WithPrecision {
    const V& value;
    int precision;
};


template<class C, class T, class V>
auto operator<<(std::basic_ostream<C, T>& out, const impl::WithPrecision<V>& pod) -> std::basic_ostream<C, T>&
{
    return impl::with_precision(out, pod.precision, pod.value); // Throws
}


template<class C, class T, class R>
inline auto with_fixed(std::basic_ostream<C, T>& out, const R& ref, int prec) -> std::basic_ostream<C, T>&
{
    std::basic_ostream<C, T> out_2(out.rdbuf()); // Throws
    out_2.copyfmt(out); // Throws
    out_2.setf(std::ios_base::fixed, std::ios_base::floatfield); // Throws
    out_2.precision(prec); // Throws
    out_2.clear(out.rdstate()); // Throws
    out_2 << ref; // Throws
    out.clear(out_2.rdstate()); // Throws
    return out;
}


template<class V> struct WithFixed {
    const V& value;
    int precision;
};


template<class C, class T, class V>
auto operator<<(std::basic_ostream<C, T>& out, const impl::WithFixed<V>& pod) -> std::basic_ostream<C, T>&
{
    return impl::with_fixed(out, pod.value, pod.precision); // Throws
}


} // namespace impl


template<class T> inline auto with_width(const T& value, std::streamsize width) noexcept
{
    return impl::WithWidth<T> { value, width };
}


template<class T> inline auto with_precision(const T& value, int precision) noexcept
{
    return impl::WithPrecision<T> { value, precision };
}


template<class T> inline auto with_fixed(const T& value, int precision) noexcept
{
    return impl::WithFixed<T> { value, precision };
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_FORMAT_WITH_HPP
