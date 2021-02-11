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

#ifndef ARCHON_X_BASE_X_FORMAT_ENC_HPP
#define ARCHON_X_BASE_X_FORMAT_ENC_HPP

/// \file


#include <array>
#include <string>

#include <archon/base/string_codec.hpp>
#include <archon/base/seed_memory_output_stream.hpp>
#include <archon/base/format.hpp>


namespace archon::base {


/// \{
///
/// \brief Format and encode string in global locale.
///
/// These functions format the specified parameterized string as if by
/// `format(string, params...)`, then leniently encode the result as if by \ref
/// encode_string() using the global locale.
///
/// When specifying a null-terminated parameterized string (\p c_str), if the
/// character type of the string is `char`, but `C` is not `char`, the
/// characters of the parameterized string will be widened as if by `widen()` of
/// an output stream with character type `C` and imbued with the global
/// locale. It is therefore only safe to use characters from the basic source
/// character set in the parameterized string in this case.
///
/// \throws BadFormatString If the specified parameterized string is invalid.
///
template<class C, class... P> std::string format_enc(const char* c_str, const P&... params);
template<class C, class... P> std::string format_enc(const C* c_str, const P&... params);
template<class C, class T, class... P>
std::string format_enc(std::basic_string_view<C, T> string, const P&... params);
/// \}



/// \{
///
/// \brief Format and encode string in specified locale.
///
/// These functions format the specified parameterized string as if by
/// `format(locale, string, params...)`, then leniently encode the result as if
/// by \ref encode_string() using the specied locale.
///
/// When specifying a null-terminated parameterized string (\p c_str), if the
/// character type of the string is `char`, but `C` is not `char`, the
/// characters of the parameterized string will be widened as if by `widen()` of
/// an output stream with character type `C` and imbued with the specified
/// locale. It is therefore only safe to use characters from the basic source
/// character set in the parameterized string in this case.
///
/// \throws BadFormatString If the specified parameterized string is invalid.
///
template<class C, class... P>
std::string format_enc(const std::locale& locale, const char* c_str, const P&... params);
template<class C, class... P>
std::string format_enc(const std::locale& locale, const C* c_str, const P&... params);
template<class C, class T, class... P>
std::string format_enc(const std::locale& locale, std::basic_string_view<C, T> string,
                       const P&... params);
/// \}








// Implementation


template<class C, class... P> std::string format_enc(const char* c_str, const P&... params)
{
    std::locale locale; // Global locale
    return format_enc<C>(locale, c_str, params...); // Throws
}


template<class C, class... P> std::string format_enc(const C* c_str, const P&... params)
{
    std::basic_string_view<C> string(c_str);
    return format_enc(string, params...); // Throws
}


template<class C, class T, class... P> std::string format_enc(std::basic_string_view<C, T> string, const P&... params)
{
    std::locale locale; // Global locale
    return format_enc(locale, string, params...); // Throws
}


template<class C, class... P> std::string format_enc(const std::locale& locale, const char* c_str, const P&... params)
{
    std::array<C, 512> seed_memory;
    BasicSeedMemoryOutputStream out(seed_memory); // Throws
    out.exceptions(std::ios_base::badbit | std::ios_base::failbit);
    out.imbue(locale); // Throws
    format(out, c_str, params...); // Throws
    return encode_string(out.view(), locale); // Throws
}


template<class C, class... P>
inline std::string format_enc(const std::locale& locale, const C* c_str, const P&... params)
{
    std::basic_string_view<C> string(c_str);
    return format_enc(locale, string, params...); // Throws
}


template<class C, class T, class... P>
std::string format_enc(const std::locale& locale, std::basic_string_view<C, T> string,
                       const P&... params)
{
    std::array<C, 512> seed_memory;
    BasicSeedMemoryOutputStream out(seed_memory); // Throws
    out.exceptions(std::ios_base::badbit | std::ios_base::failbit);
    out.imbue(locale); // Throws
    format(out, string, params...); // Throws
    return encode_string(out.view(), locale); // Throws
}


} // namespace archon::base

#endif // ARCHON_X_BASE_X_FORMAT_ENC_HPP
