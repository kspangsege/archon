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

#ifndef ARCHON_X_CORE_X_BASIC_CHARACTER_SET_HPP
#define ARCHON_X_CORE_X_BASIC_CHARACTER_SET_HPP

/// \file


#include <utility>
#include <string>

#include <archon/core/features.h>
#include <archon/core/literal_hash_map.hpp>


namespace archon::core {


/// \brief Map basic character from native encoding to ASCII.
///
/// This function is used to map a character in the basic character set from its native
/// encoding to its encoding in ASCII.
///
/// If the value of the specified character (\p ch) is the encoded value with respect to the
/// native multi-byte character encoding of a character in the basic character set, then
/// this function returns `true` after setting \p ascii to the encoded value of that
/// character in ASCII. Otherwise, this function returns `false` and leaves \p ascii
/// unchanged.
///
/// The native multi-byte character encoding is to be understood as the multi-byte encoding
/// of the execution environment. It is the encoding of plain character and string
/// literals. All characters in the basic character set have native multi-byte encoding that
/// uses only one byte (only one element of type `char`).
///
/// \note The Archon project in general assumes that an application specified locale uses a
/// multi-byte character encoding that agrees on encoded values with the native multi-byte
/// encoding for all characters in the basic character set. Therefore, the encoded value of
/// a character literal, such as <tt>'*'</tt>, can be considered invariant across locales.
///
/// Until C++26, the basic character set contains the following 96 characters:
///
///     A B C D E F G H I J K L M N O P Q R S T U V W X Y Z         ( 26 )
///     a b c d e f g h i j k l m n o p q r s t u v w x y z         ( 26 )
///     0 1 2 3 4 5 6 7 8 9                                         ( 10 )
///     _ { } [ ] # ( ) < > % : ; . ? * + - / ^ & | ~ ! = , \ " '   ( 29 )
///     space, newline, horizontal tab, vertical tab, form feed     (  5 )
///
/// C++26 plans to adds the following three characters for a total of 99:
///
///     $ @ `
///
/// This would bring all printable characters of ASCII into the basic character set.
///
/// \sa \ref core::try_map_ascii_to_bcs()
///
constexpr bool try_map_bcs_to_ascii(char ch, char& ascii) noexcept;


/// \brief Map basic character from ASCII to native encoding.
///
/// This function is used to map a character in the basic character set from its encoding in
/// ASCII to its native encoding.
///
/// If the value of the specified character (\p ch) is the ASCII encoding of a character in
/// the basic character set, then this function returns `true` after setting \p bcs to the
/// encoded value of that character with respect to the native multi-byte character
/// encoding. Otherwise, this function returns `false` and leaves \p bcs unchanged.
///
/// See \ref core::try_map_bcs_to_ascii() for notes on the meaning of "native multi-byte
/// character encoding" and "basic character set".
///
/// \sa \ref core::try_map_bcs_to_ascii()
///
constexpr bool try_map_ascii_to_bcs(char ch, char& bcs) noexcept;








// Implementation


namespace impl {


// Characters of the basic character set indexed by their encoded value in ASCII.
//
// If `v` is the encoded value in ASCII of a character, `c`, and `g_bcs_by_ascii[v]` is
// `'\0'`, then `c` is not in the basic character set.
//
// FIXME: Fill in slots for '$', '@', and '`' when switching to C++26
//
constexpr char g_bcs_by_ascii[128] = {
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\t', '\n', '\v', '\f', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    ' ',  '!',  '"',  '#',  '\0', '%',  '&',  '\'', '(',  ')',  '*',  '+',  ',',  '-',  '.',  '/',
    '0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',  ':',  ';',  '<',  '=',  '>',  '?',
    '\0', 'A',  'B',  'C',  'D',  'E',  'F',  'G',  'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',
    'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',  'X',  'Y',  'Z',  '[',  '\\', ']',  '^',  '_',
    '\0', 'a',  'b',  'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
    'p',  'q',  'r',  's',  't',  'u',  'v',  'w',  'x',  'y',  'z',  '{',  '|',  '}',  '~',  '\0',
};


constexpr int bcs_size() noexcept
{
    int n = 0;
    for (int i = 0; i < 128; ++i) {
        char ch = g_bcs_by_ascii[i];
        n += int(ch != '\0');
    }
    return n;
}


constexpr bool bcs_is_ascii_subset() noexcept
{
    for (int i = 0; i < 128; ++i) {
        char ch = g_bcs_by_ascii[i];
        int val = std::char_traits<char>::to_int_type(ch);
        if (ARCHON_LIKELY(ch == '\0' || val == i))
            continue;
        return false;
    }
    return true;
}


constexpr auto make_bcs_map() noexcept
{
    constexpr int n = bcs_size();
    std::pair<char, char> assocs[n];
    int i = 0;
    for (int j = 0; j < 128; ++j) {
        char ch = g_bcs_by_ascii[j];
        if (ARCHON_LIKELY(ch != '\0')) {
            assocs[i] = { ch, char(j) };
            ++i;
            continue;
        }
    }
    return core::make_literal_hash_map(assocs);
}


constexpr core::LiteralHashMap g_bcs_map = make_bcs_map();


} // namespace impl


constexpr bool try_map_bcs_to_ascii(char ch, char& ascii) noexcept
{
    constexpr bool force_fallback = false;

    if constexpr (impl::bcs_is_ascii_subset() && !force_fallback) {
        int val = std::char_traits<char>::to_int_type(ch);
        bool in_bcs = (val >= 0 && val < 128 && impl::g_bcs_by_ascii[val] != '\0');
        if (ARCHON_LIKELY(in_bcs)) {
            ascii = ch;
            return true;
        }
        return false;
    }
    else {
        return impl::g_bcs_map.find(ch, ascii);
    }
}


constexpr bool try_map_ascii_to_bcs(char ch, char& bcs) noexcept
{
    constexpr bool force_fallback = false;

    int val = std::char_traits<char>::to_int_type(ch);
    bool in_bcs = (val >= 0 && val < 128 && impl::g_bcs_by_ascii[val] != '\0');
    if (ARCHON_LIKELY(in_bcs)) {
        if constexpr (impl::bcs_is_ascii_subset() && !force_fallback) {
            bcs = ch;
        }
        else {
            bcs = impl::g_bcs_by_ascii[val];
        }
        return true;
    }
    return false;
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_BASIC_CHARACTER_SET_HPP
