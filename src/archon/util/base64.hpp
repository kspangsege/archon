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

#ifndef ARCHON_UTIL_BASE64_HPP
#define ARCHON_UTIL_BASE64_HPP

#include <stdexcept>
#include <string>

namespace archon
{
  namespace Util
  {
    namespace Base64
    {
      /**
       * /return -1 if the specified symbol is not a base-64 symbol.
       */
      int lookup_symbol(char);

      struct BadCharException: std::runtime_error
      {
        BadCharException(): std::runtime_error("") {}
      };

      /**
       * \param v Must be in the range [0,63].
       */
      char symbol_from_value(int v);

      int value_from_symbol(char) throw(BadCharException);

      std::string decode(std::string s) throw(BadCharException);

      /**
       * Decode a string containing a non-negative number written in
       * base-64.
       *
       * \param s The string containing the base-64 encoded number.
       *
       * \return The decoded value.
       */
      unsigned long decode_number(std::string s) throw(BadCharException);
    }
  }
}

#endif // ARCHON_UTIL_BASE64_HPP
