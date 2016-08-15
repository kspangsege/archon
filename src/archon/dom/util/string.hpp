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

#ifndef ARCHON_DOM_UTIL_STRING_HPP
#define ARCHON_DOM_UTIL_STRING_HPP

#include <archon/core/utf16.hpp>


namespace archon
{
  namespace dom
  {
    typedef Core::StringUtf16 DOMString;


    /**
     * Construct a DOM string from a null terminated multi-byte
     * character string containing characters from the portable
     * character set only.
     *
     * Note that in the context of the Archon framework, a multi-byte
     * character string literal that contains characters from the
     * portable character set only, has identical encoding in all
     * locales. See \ref CharEnc.
     */
    inline DOMString str_from_port(char const *port) { return Core::utf16_from_port(port); }

    inline DOMString str_from_port(std::string const &port) { return Core::utf16_from_port(port); }

    inline void str_append_port(DOMString &s, char const *port)
    {
      Core::utf16_append_port(s, port);
    }

    inline void str_append_port(DOMString &s, std::string const &port)
    {
      Core::utf16_append_port(s, port);
    }


    /**
     * Construct a DOM string from a null terminated wide character
     * string, encoded accoding to the "C" locale.
     *
     * Note that in the context of the Archon framework, a wide
     * character string literal that contains characters from the
     * portable character set only, is always assumed to be encoded
     * according to the "C" locale. See \ref CharEnc.
     */
    inline DOMString str_from_cloc(wchar_t const *s) { return Core::utf16_from_cloc(s); }


    inline std::wstring str_to_wide(DOMString const &s, std::locale const &l = std::locale())
    {
      return Core::utf16_to_wide(s,l);
    }

    /**
     * Transform the specified DOM string into a multi-byte encoded
     * string of characters from the portable character set. Not that
     * portable characters have the same multi-byte encoding across
     * all locales. See \ref CharEnc.
     *
     * This function is guaranteed to successfully convert any DOM
     * string that contains characters from the portable character set
     * only.
     *
     * This function is guaranteed to fail if the DOM string contains
     * a character that uses more than one byte in the multi-byte
     * encoding.
     *
     * It is unspecified whether this function fails if the DOM string
     * contains a character that uses only one byte in the multi-byte
     * encoding, but is not part of the portable character set.
     *
     * \return True if conversion was successfull. Otherwise returns
     * false.
     */
    inline bool str_to_narrow_port(DOMString const &s, std::string &port)
    {
      return Core::utf16_to_narrow_port(s, port);
    }


    inline DOMString case_fold(DOMString const &s) { return Core::case_fold(s); }


    inline DOMString to_upper_case(DOMString const &s) { return Core::to_upper_case(s); }


    inline DOMString to_lower_case(DOMString const &s) { return Core::to_lower_case(s); }
  }
}


#endif // ARCHON_DOM_UTIL_STRING_HPP
