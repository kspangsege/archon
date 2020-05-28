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

#ifndef ARCHON_X_CORE_X_HASH_HPP
#define ARCHON_X_CORE_X_HASH_HPP

/// \file


#include <archon/core/impl/hash.hpp>


namespace archon::core {


/// \brief Hash function that can be used at compile time.
///
/// This class template delivers a hash function that can be evaluated at compile time,
/// i.e., its construction and invocation are `constexpr` operations. This sets it apart
/// from `std::hash`.
///
template<class T> class Hash;

/// \{
///
/// \brief Hash function specializations for integer types.
///
/// These are the integer type specializations of the hash function that can be used at
/// compile time. These specializations are all implemented in terms of \ref
/// core::Hash_FNV_1a_32.
///
template<> class Hash<bool>               : public core::impl::HashInt<bool> {};
template<> class Hash<char>               : public core::impl::HashInt<char> {};
template<> class Hash<signed char>        : public core::impl::HashInt<signed char> {};
template<> class Hash<unsigned char>      : public core::impl::HashInt<unsigned char> {};
template<> class Hash<wchar_t>            : public core::impl::HashInt<wchar_t> {};
template<> class Hash<char16_t>           : public core::impl::HashInt<char16_t> {};
template<> class Hash<char32_t>           : public core::impl::HashInt<char32_t> {};
template<> class Hash<short>              : public core::impl::HashInt<short> {};
template<> class Hash<unsigned short>     : public core::impl::HashInt<unsigned short> {};
template<> class Hash<int>                : public core::impl::HashInt<int> {};
template<> class Hash<unsigned>           : public core::impl::HashInt<unsigned> {};
template<> class Hash<long>               : public core::impl::HashInt<long> {};
template<> class Hash<unsigned long>      : public core::impl::HashInt<unsigned long> {};
template<> class Hash<long long>          : public core::impl::HashInt<long long> {};
template<> class Hash<unsigned long long> : public core::impl::HashInt<unsigned long long> {};
/// \}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_HASH_HPP
