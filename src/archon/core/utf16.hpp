/*
 * This file is part of the Archon library framework.
 *
 * Copyright (C) 2012  Kristian Spangsege <kristian.spangsege@gmail.com>
 *
 * The Archon library framework is free software: You can redistribute
 * it and/or modify it under the terms of the GNU Lesser General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * The Archon library framework is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the Archon library framework.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * \author Kristian Spangsege
 */

#ifndef ARCHON_CORE_UTF16_HPP
#define ARCHON_CORE_UTF16_HPP

#include <stdexcept>
#include <cstring>
#include <cwchar>
#include <string>
#include <locale>
#include <ostream>

#include <archon/config_pub.h>
#include <archon/core/types.hpp>
#include <archon/core/functions.hpp>


namespace archon {
namespace core {

namespace _Impl { typedef UIntMin16 CharUtf16_int; }


/// Replacement for \c char16_t in C++0x.
struct CharUtf16 {
    _Impl::CharUtf16_int val;

    bool operator==(const CharUtf16& c) const { return val == c.val; }
    bool operator!=(const CharUtf16& c) const { return val != c.val; }
};


/// Replacement for std::u16string in C++0x.
typedef std::basic_string<CharUtf16> StringUtf16;


/// Construct a UTF-16 string from a null terminated multi-byte
/// character string containing characters from the portable character
/// set only.
///
/// Note that in the context of the Archon framework, a multi-byte
/// character string literal that contains characters from the
/// portable character set only, has identical encoding in all
/// locales. See \ref CharEnc.
StringUtf16 utf16_from_port(const char*);

StringUtf16 utf16_from_port(const std::string&);

void utf16_append_port(StringUtf16&, const char* port);

void utf16_append_port(StringUtf16&, const std::string& port);

// Returns false if, and only if the specified UCS code point is
// invalid or cannot be encoded by UTF-16.
bool utf16_append_ucs_char(StringUtf16&, core::UIntFast32 ucs_code_point);


/// Construct a UTF-16 string from a null terminated wide character
/// string, encoded accoding to the "C" locale.
///
/// Note that in the context of the Archon framework, a wide character
/// string literal that contains characters from the portable
/// character set only, is always assumed to be encoded according to
/// the "C" locale. See \ref CharEnc.
StringUtf16 utf16_from_cloc(const wchar_t*);


/// Convert the specified multi-byte string to a UTF-16 string. The
/// encoding used in the multi-byte represenation is determined by the
/// specified locale.
StringUtf16 utf16_from_narrow(const std::string&, const std::locale&);


/// Convert the specified wide character string to a UTF-16
/// string. The encoding used in the wide character represenation is
/// determined by the specified locale.
StringUtf16 utf16_from_wide(const std::wstring&, const std::locale&);


/// Convert the specified UTF-16 string to the equivalent multi-byte
/// representation determined by the specified locale.
std::string utf16_to_narrow(const StringUtf16&, const std::locale&);


/// Convert the specified UTF-16 string to a wide character string
/// where the encoding is determined by the specified locale.
std::wstring utf16_to_wide(const StringUtf16&, const std::locale&);


/// Transform the specified UTF-16 string into a multi-byte encoded
/// string of characters from the portable character set. Not that
/// portable characters have the same multi-byte encoding across all
/// locales. See \ref CharEnc.
///
/// This function is guaranteed to successfully convert any UTF-16
/// string that contains characters from the portable character set
/// only.
///
/// This function is guaranteed to fail if the UTF-16 string contains
/// a character that uses more than one byte in the multi-byte
/// encoding.
///
/// It is unspecified whether this function fails if the UTF-16 string
/// contains a character that uses only one byte in the multi-byte
/// encoding, but is not part of the portable character set.
///
/// \return True if conversion was successfull. Otherwise returns
/// false.
bool utf16_to_narrow_port(const StringUtf16&, std::string& port);


/// Perform full Unicode case folding (without the special 'T' rules).
StringUtf16 case_fold(const StringUtf16&);


/// Convert the specified string to upper case according to rules of
/// the Unicode specification.
StringUtf16 to_upper_case(const StringUtf16&);


/// Convert the specified string to lower case according to rules of
/// the Unicode specification.
StringUtf16 to_lower_case(const StringUtf16&);


inline std::ostream&  operator<<(std::ostream&,  CharUtf16);
inline std::wostream& operator<<(std::wostream&, CharUtf16);
inline std::ostream&  operator<<(std::ostream&,  const StringUtf16&);
inline std::wostream& operator<<(std::wostream&, const StringUtf16&);


} // namespace core
} // namespace archon








// Implementation:

namespace std {

template<> struct char_traits<archon::core::CharUtf16> {
    typedef archon::core::CharUtf16 char_type;
    typedef unsigned                int_type;
    typedef std::streampos          pos_type;
    typedef std::streamoff          off_type;
    typedef std::mbstate_t          state_type;

    static void assign(char_type& c1, const char_type& c2) { c1 = c2; }
    static bool eq(const char_type& c1, const char_type& c2) { return c1.val == c2.val; }
    static bool lt(const char_type& c1, const char_type& c2) { return c1.val <  c2.val; }
    static int compare(const char_type* s1, const char_type* s2, std::size_t n);
    static std::size_t length(const char_type* s);
    static const char_type* find(const char_type* s, std::size_t n, const char_type& a);
    static char_type *move(char_type* s1, const char_type* s2, std::size_t n);
    static char_type *copy(char_type* s1, const char_type* s2, std::size_t n);
    static char_type *assign(char_type* s, std::size_t n, char_type a);
    static char_type to_char_type(const int_type& i);
    static int_type to_int_type(const char_type& c) { return c.val; }
    static bool eq_int_type(const int_type& i1, const int_type& i2) { return i1 == i2; }
    static int_type eof() { return std::numeric_limits<int_type>::max(); }
    static int_type not_eof(const int_type& i) { return i == eof() ? 0 : i; }
};

} // namespace std


inline int std::char_traits<archon::core::CharUtf16>::
compare(const char_type* s1, const char_type* s2, std::size_t n)
{
    for (std::size_t i=0; i<n; ++i) {
        if (s1[i].val < s2[i].val)
            return -1;
        if (s2[i].val < s1[i].val)
            return 1;
    }
    return 0;
}

inline std::size_t std::char_traits<archon::core::CharUtf16>::
length(const char_type* s)
{
    std::size_t n = 0;
    while (!s[n].val)
        ++n;
    return n;
}

inline const archon::core::CharUtf16* std::char_traits<archon::core::CharUtf16>::
find(const char_type* s, std::size_t n, const char_type& a)
{
    for (std::size_t i = 0; i != n; ++i) {
        if (s[i].val == a.val)
            return s+i;
    }
    return 0;
}

inline archon::core::CharUtf16* std::char_traits<archon::core::CharUtf16>::
move(char_type* s1, const char_type* s2, std::size_t n)
{
    if (s1 < s2) {
        std::copy(s2, s2+n, s1);
    }
    else if (s2 < s1) {
        std::copy_backward(s2, s2+n, s1);
    }
    return s1;
}

inline archon::core::CharUtf16* std::char_traits<archon::core::CharUtf16>::
copy(char_type* s1, const char_type* s2, std::size_t n)
{
    std::copy(s2, s2+n, s1);
    return s1;
}

inline archon::core::CharUtf16* std::char_traits<archon::core::CharUtf16>::
assign(char_type* s, std::size_t n, char_type a)
{
    std::fill(s, s+n, a);
    return s;
}

inline archon::core::CharUtf16 std::char_traits<archon::core::CharUtf16>::
to_char_type(const int_type& i)
{
    char_type c;
    c.val = static_cast<archon::core::_Impl::CharUtf16_int>(i);
    return c;
}


namespace archon {
namespace core {

#ifdef ARCHON_WCHAR_ENC_IS_UCS

namespace _Impl {

template<class WideCharIter>
inline bool try_encode_utf16(WideCharIter begin, WideCharIter end, StringUtf16& out)
{
    typedef std::char_traits<wchar_t>   wide_traits;
    typedef std::char_traits<CharUtf16> utf16_traits;
    typedef typename wide_traits::int_type wide_int_type;
    StringUtf16 str;
    str.reserve(end - begin);
    for (WideCharIter i = begin; i != end; ++i) {
        wide_int_type v = wide_traits::to_int_type(*i);
        if (int_less_than(v, 0) || int_less_than_equal(0x110000, v))
            return false; // Cannot be encoded by UTF-16
        if (0xD800 <= v && v < 0xE000)
            return false; // Forbidden surrogate range
        if (v < 0x10000) {
            str += utf16_traits::to_char_type(v);
        }
        else {
            v -= 0x10000;
            wide_int_type v1 = 0xD800 + (v >> 10);
            wide_int_type v2 = 0xDC00 + (v & 0x3FF);
            str += utf16_traits::to_char_type(v1);
            str += utf16_traits::to_char_type(v2);
        }
    }
    out = str;
    return true;
}

template<class Utf16CharIter>
inline bool try_decode_utf16(Utf16CharIter begin, Utf16CharIter end, std::wstring& out)
{
    typedef std::char_traits<wchar_t>   wide_traits;
    typedef std::char_traits<CharUtf16> utf16_traits;
    typedef typename utf16_traits::int_type utf16_int_type;
    typedef typename wide_traits::int_type  wide_int_type;
    std::wstring str;
    str.reserve(end - begin);
    for (Utf16CharIter i = begin; i != end; ++i) {
        utf16_int_type v = utf16_traits::to_int_type(*i);
        if (v == 0xFFFE || v == 0xFFFF)
            return false; // Illegal UTF-16
        if (0xDC00 <= v && v < 0xE000)
            return false; // Unexpected high surrogate
        wide_int_type w;
        if (0xD800 <= v && v < 0xDC00) {
            // Combine UTF-16 surrogates
            if (++i == end)
                return false; // Incomplete surrogate pair
            utf16_int_type v2 = utf16_traits::to_int_type(*i);
            if (v2 < 0xDC00 || 0xE000 <= v2)
                return false; // Invalid high surrogate
            w = 0x10000 + (wide_int_type(v-0xD800)<<10) + (v2-0xDC00);
        }
        else {
            w = v;
        }
        str += wide_traits::to_char_type(w);
    }
    out = str;
    return true;
}

template<class CharIter>
inline void utf16_append_port(CharIter begin, CharIter end, StringUtf16& out)
{
    typedef std::char_traits<wchar_t>   wide_traits;
    typedef wide_traits::int_type       wide_int_type;
    typedef std::char_traits<CharUtf16> enc_traits;
    const std::ctype<wchar_t>& ctype2 = std::use_facet<std::ctype<wchar_t> >(std::locale::classic());
    out.reserve(end - begin);
    for (CharIter i = begin; i != end; ++i) {
        wide_int_type v = wide_traits::to_int_type(ctype2.widen(*i));
        // The portable character set is a subset of the ASCII
        // characters
        if (int_less_than(v, 0x0) || int_less_than_equal(0x7F, v)) {
            throw std::runtime_error("Unexpected failue while transcodinf portable characters "
                                     "from the multi-byte encoding to UTF-16");
        }
        out += enc_traits::to_char_type(v);
    }
}

template<class Utf16CharIter>
inline bool try_utf16_to_port(Utf16CharIter begin, Utf16CharIter end, std::string& out)
{
    typedef std::char_traits<wchar_t>    wide_traits;
    typedef wide_traits::int_type        wide_int_type;
    typedef std::char_traits<CharUtf16>  enc_traits;
    typedef enc_traits::int_type         enc_int_type;
    const std::ctype<wchar_t>& ctype2 = std::use_facet<std::ctype<wchar_t> >(std::locale::classic());
    std::string str;
    str.reserve(end - begin);
    for (Utf16CharIter i=begin; i!=end; ++i) {
        enc_int_type v = enc_traits::to_int_type(*i);
        if (v == 0xFFFE || v == 0xFFFF)
            return false; // Illegal UTF-16
        if (0xDC00 <= v && v < 0xE000)
            return false; // Unexpected high surrogate
        wide_int_type w;
        if (0xD800 <= v && v < 0xDC00) {
            // Combine UTF-16 surrogates
            if (++i == end)
                return false; // Incomplete surrogate pair
            enc_int_type v2 = enc_traits::to_int_type(*i);
            if (v2 < 0xDC00 || 0xE000 <= v2)
                return false; // Invalid high surrogate
            w = 0x10000 + (wide_int_type(v-0xD800)<<10) + (v2-0xDC00);
        }
        else {
            w = v;
        }
        wchar_t wide = wide_traits::to_char_type(w);
        char narrow = ctype2.narrow(wide, '\0');
        if (narrow == '\0' && ctype2.narrow(wide, 'x') == 'x')
            return false;
        str += narrow;
    }
    out = str;
    return true;
}

} // namespace _Impl


inline StringUtf16 utf16_from_port(const char* port)
{
    StringUtf16 str;
    _Impl::utf16_append_port(port, port+std::strlen(port), str);
    return str;
}

inline StringUtf16 utf16_from_port(const std::string& port)
{
    StringUtf16 str;
    _Impl::utf16_append_port(port.begin(), port.end(), str);
    return str;
}

inline void utf16_append_port(StringUtf16& str, const std::string& port)
{
    _Impl::utf16_append_port(port.begin(), port.end(), str);
}

inline void utf16_append_port(StringUtf16& str, const char* port)
{
    _Impl::utf16_append_port(port, port+std::strlen(port), str);
}

inline StringUtf16 utf16_from_cloc(const wchar_t* s)
{
    StringUtf16 t;
    if (_Impl::try_encode_utf16(s, s+std::wcslen(s), t))
        return t;
    throw std::runtime_error("Bad Unicode character in specified string");
}

inline StringUtf16 utf16_from_wide(const std::wstring& s, const std::locale&)
{
    StringUtf16 t;
    if (_Impl::try_encode_utf16(s.begin(), s.end(), t))
        return t;
    throw std::runtime_error("Bad Unicode character in specified string");
}

inline std::wstring utf16_to_wide(const StringUtf16& s, const std::locale&)
{
    std::wstring t;
    if (_Impl::try_decode_utf16(s.begin(), s.end(), t))
        return t;
    throw std::runtime_error("Bad UTF-16 element in specified string");
}

inline bool utf16_to_narrow_port(const StringUtf16& s, std::string& port)
{
    return _Impl::try_utf16_to_port(s.begin(), s.end(), port);
}

#else
#  error Unexpected wide character encoding
#endif // ARCHON_WCHAR_ENC_IS_UCS


inline bool utf16_append_ucs_char(StringUtf16& str, core::UIntFast32 ucs_code_point)
{
    if (0x110000 <= ucs_code_point)
        return false; // Cannot be encoded by UTF-16
    if (0xD800 <= ucs_code_point && ucs_code_point < 0xE000)
        return false; // Forbidden surrogate range
    typedef std::char_traits<CharUtf16> utf16_traits;
    if (ucs_code_point < 0x10000) {
        str += utf16_traits::to_char_type(ucs_code_point);
        return true;
    }
    ucs_code_point -= 0x10000;
    core::UIntFast16 v1 = 0xD800 + (ucs_code_point >> 10);
    core::UIntFast16 v2 = 0xDC00 + (ucs_code_point & 0x3FF);
    str += utf16_traits::to_char_type(v1);
    str += utf16_traits::to_char_type(v2);
    return true;
}

inline std::ostream& operator<<(std::ostream& out, CharUtf16 ch)
{
    return out << StringUtf16(1, ch);
}

inline std::wostream& operator<<(std::wostream& out, CharUtf16 ch)
{
    return out << StringUtf16(1, ch);
}

inline std::ostream& operator<<(std::ostream& out, const StringUtf16& str)
{
    return out << utf16_to_narrow(str, out.getloc());
}

inline std::wostream& operator<<(std::wostream& out, const StringUtf16& str)
{
    return out << utf16_to_wide(str, out.getloc());
}

} // namespace core
} // namespace archon

#endif // ARCHON_CORE_UTF16_HPP
