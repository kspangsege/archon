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

#include <archon/util/base64.hpp>


using namespace std;

namespace
{
  char const fwd[64] =
  {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3',
    '4', '5', '6', '7', '8', '9', '-', '_'
  };

  int const rev[128] =
  {
    //        0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
    /*  0 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    /* 10 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    /* 20 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1,
    /* 30 */ 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
    /* 40 */ -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    /* 50 */ 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, 63,
    /* 60 */ -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    /* 70 */ 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1
  };
}

namespace archon
{
  namespace util
  {
    namespace Base64
    {
      int lookup_symbol(char c)
      {
        unsigned char d = static_cast<unsigned char>(c);
        return d < 128 ? rev[d] : -1;
      }

      char symbol_from_value(int v)
      {
        return fwd[v];
      }

      int value_from_symbol(char c) throw(BadCharException)
      {
        int r = lookup_symbol(c);
        if(r < 0) throw BadCharException();
        return r;
      }

      string decode(string s) throw(BadCharException)
      {
        string::size_type i=0, n=s.size(), m=n%4u;
        string t;
        t.reserve((n+3u)/4u*3u);
        n -= m;
        for(i=0; i<n; i+=4)
        {
          int a[4] =
            {
              value_from_symbol(s[i+0u]), value_from_symbol(s[i+1u]),
              value_from_symbol(s[i+2u]), value_from_symbol(s[i+3u])
            };
          t += static_cast<unsigned char>(a[0]<<2|a[1]>>4);
          t += static_cast<unsigned char>((a[1]&15)<<4|a[2]>>2);
          t += static_cast<unsigned char>((a[2]&3)<<6|a[3]);
        }
        if(m)
          switch(m)
          {
          case 1:
            t += static_cast<unsigned char>(value_from_symbol(s[i])<<2);
            break;

          case 2:
            t += static_cast<unsigned char>(value_from_symbol(s[i])<<2 |
                                            value_from_symbol(s[i+1])>>4);
            break;

          case 3:
          {
            int a[3] =
              {
                value_from_symbol(s[i]), value_from_symbol(s[i+1]),
                value_from_symbol(s[i+2])
              };
            t += static_cast<unsigned char>(a[0]<<2|a[1]>>4);
            t += static_cast<unsigned char>((a[1]&15)<<4|a[2]>>2);
          }
          break;
          }

        return t;
      }

      unsigned long decode_number(string s) throw(BadCharException)
      {
        if(s.empty()) return 0;
        unsigned long v=value_from_symbol(s[0]);
        for(string::size_type i=1; i<s.size(); ++i)
        {
          v *= 64;
          v += value_from_symbol(s[i]);
        }
        return v;
      }
    }
  }
}
