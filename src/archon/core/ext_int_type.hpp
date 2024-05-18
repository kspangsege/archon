// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2024 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_CORE_X_EXT_INT_TYPE_HPP
#define ARCHON_X_CORE_X_EXT_INT_TYPE_HPP

/// \file


#include <archon/core/impl/ext_int_type.hpp>


namespace archon::core {


/// \{
///
/// \brief Integer type of any width.
///
/// These types are aliases either for a standard or an extended integer type whose width is
/// greater than, or equal to the specified width (\p N). Types are available to accommodate
/// and width requirement. In all cases, the aliased types are guaranteed to use two's
/// complement representation of negative numbers.
///
/// \tparam N The required minimum width. For signed integer types, the width is the number
/// of value bits plus one if the type is signed. For unsigned integer types, it is just the
/// number of value bits. See also \ref core::int_width().
///
template<int N> using least_signed_ext_int_type = typename impl::LeastSignedExtIntType<N>::type;
template<int N> using least_unsigned_ext_int_type = typename impl::LeastUnsignedExtIntType<N>::type;
template<int N> using fast_signed_ext_int_type = typename impl::FastSignedExtIntType<N>::type;
template<int N> using fast_unsigned_ext_int_type = typename impl::FastUnsignedExtIntType<N>::type;
/// \}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_EXT_INT_TYPE_HPP
