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

#ifndef ARCHON_DOM_IMPL_UTIL_HPP
#define ARCHON_DOM_IMPL_UTIL_HPP

#include <stdexcept>

#include <archon/core/types.hpp>
#include <archon/dom/util/string.hpp>


namespace Archon
{
  namespace DomImpl
  {
    /**
     * Perform full Unicode case folding (without the special 'T'
     * rules), but is optimized speed-wise for strings that normally
     * contain only ASCII characters.
     */
    void case_fold_ascii(dom::DOMString &);


    /**
     * Perform full Unicode aware up-casing, but optimized speed-wise
     * for strings that normally contain only ASCII characters.
     */
    void to_upper_case_ascii(dom::DOMString &);


    /**
     * Perform full Unicode aware down-casing, but optimized
     * speed-wise for strings that normally contain only ASCII
     * characters.
     */
    void to_lower_case_ascii(dom::DOMString &);


    /**
     * Check the specified name according to rules in XML 1.0
     * specification.
     */
    bool validate_xml_1_0_name(dom::DOMString const &);




    template<class T, int N> struct SmallFixedSizeQueue
    {
      int size() const { return n; }

      bool empty() const { return n == 0; }

      bool full() const { return n == N; }

      T get(int i) const
      {
        return entries[(offset+i) % N];
      }

      // Must not be empty
      T get_first() const
      {
        ARCHON_ASSERT(0 < n);
        return entries[offset];
      }

      // Must not be empty
      void remove_first()
      {
        ARCHON_ASSERT(0 < n);
        if (++offset == N) offset = 0;
        --n;
      }

      // Must not be full
      void prepend(T v)
      {
        ARCHON_ASSERT(n < N);
        if (offset == 0) offset = N;
        entries[--offset] = v;
        ++n;
      }

      // Must not be full
      void append(T v)
      {
        ARCHON_ASSERT(n < N);
        entries[(offset+n) % N] = v;
        ++n;
      }

      // Specified element must be present
      // Searches from back to front
      void remove(T v)
      {
        ARCHON_ASSERT(0 < n);
        int i = (offset + --n) % N;
        int const j = i;
        while (entries[i] != v) {
          ARCHON_ASSERT(i != offset);
          if (i == 0) i = N;
          --i;
        }
        while (i != j) {
          int i2 = (i+1) % N;
          entries[i] = entries[i2];
          i = i2;
        }
      }

      SmallFixedSizeQueue(): n(0), offset(0) {}

    private:
      T entries[N];
      int n, offset;
    };








    // Implementation:

    inline void case_fold_ascii(dom::DOMString &s)
    {
      typedef dom::DOMString::const_iterator iter;
      typedef dom::DOMString::value_type char_type;
      typedef dom::DOMString::traits_type traits;
      iter const b = s.begin();
      iter const e = s.end();
      iter i = b;
      traits::int_type v;
      for (;;) {
        if (i == e) return; // Nothing needs to be done
        v = traits::to_int_type(*i);
        if (0x7F < v) {
          s = dom::case_fold(s); // Full Unicode case folding
          return;
        }
        if (0x41 <= v && v <= 0x5A) break; // Detect upper case ASCII
        ++i;
      }

      dom::DOMString s2;
      s2.reserve(s.size());
      s2.append(b, i);
      s2.append(1, traits::to_char_type(v + 0x20)); // Convert detected character
      for (;;) {
        if (++i == e) break;
        char_type c = *i;
        v = traits::to_int_type(c);
        if (0x7F < v) {
          s = dom::case_fold(s); // Full Unicode case folding
          return;
        }
        if (0x41 <= v && v <= 0x5A) c = traits::to_char_type(v + 0x20);
        s2.append(1, c);
      }

      s = s2;
    }



    inline void to_upper_case_ascii(dom::DOMString &s)
    {
      typedef dom::DOMString::const_iterator iter;
      typedef dom::DOMString::value_type char_type;
      typedef dom::DOMString::traits_type traits;
      iter const b = s.begin();
      iter const e = s.end();
      iter i = b;
      traits::int_type v;
      for (;;) {
        if (i == e) return; // Nothing needs to be done
        v = traits::to_int_type(*i);
        if (0x7F < v) {
          s = dom::to_upper_case(s); // Full Unicode upper casing
          return;
        }
        if (0x61 <= v && v <= 0x7A) break; // Detect lower case ASCII
        ++i;
      }

      dom::DOMString s2;
      s2.reserve(s.size());
      s2.append(b, i);
      s2.append(1, traits::to_char_type(v - 0x20)); // Convert detected character
      for (;;) {
        if (++i == e) break;
        char_type c = *i;
        v = traits::to_int_type(c);
        if (0x7F < v) {
          s = dom::to_upper_case(s); // Full Unicode upper casing
          return;
        }
        if (0x61 <= v && v <= 0x7A) c = traits::to_char_type(v - 0x20);
        s2.append(1, c);
      }

      s = s2;
    }



    inline void to_lower_case_ascii(dom::DOMString &s)
    {
      typedef dom::DOMString::const_iterator iter;
      typedef dom::DOMString::value_type char_type;
      typedef dom::DOMString::traits_type traits;
      iter const b = s.begin();
      iter const e = s.end();
      iter i = b;
      traits::int_type v;
      for (;;) {
        if (i == e) return; // Nothing needs to be done
        v = traits::to_int_type(*i);
        if (0x7F < v) {
          s = dom::to_lower_case(s); // Full Unicode upper casing
          return;
        }
        if (0x41 <= v && v <= 0x5A) break; // Detect upper case ASCII
        ++i;
      }

      dom::DOMString s2;
      s2.reserve(s.size());
      s2.append(b, i);
      s2.append(1, traits::to_char_type(v + 0x20)); // Convert detected character
      for (;;) {
        if (++i == e) break;
        char_type c = *i;
        v = traits::to_int_type(c);
        if (0x7F < v) {
          s = dom::to_lower_case(s); // Full Unicode upper casing
          return;
        }
        if (0x41 <= v && v <= 0x5A) c = traits::to_char_type(v + 0x20);
        s2.append(1, c);
      }

      s = s2;
    }



    inline bool validate_xml_1_0_name(dom::DOMString const &name)
    {
      typedef dom::DOMString::const_iterator iter;
      typedef dom::DOMString::traits_type traits;
      typedef traits::int_type int_type;
      iter const begin = name.begin(), end = name.end();
      for (iter i=begin; i!=end; ++i) {
        int_type const v = traits::to_int_type(*i);
        if (v < 0xC0) { // 0x0 <= v < 0xC0
          if (v < 0x5B) { // 0x0 <= v < 0x5B
            if (v < 0x41) { // 0x0 <= v < 0x41
              if (v < 0x30) { // 0x0 <= v < 0x30
                if (v != 0x2D && v != 0x2E) return 0;
                // '-' or '.'  -->  good unless first char!
                if (i == begin) return 0;
              }
              else if (0x3A <= v) { // 0x3A <= v < 0x41
                if (v != 0x3A) return 0;
                // ':'  -->  good!
              }
              else { // '0' <= v <= '9'  -->  good unless first char!
                if (i == begin) return 0;
              }
            }
            else {} // 'A' <= v <= 'Z'  -->  good!
          }
          else if (v < 0x7B) { // 0x5B <= v < 0x7B
            if (v < 0x61) { // 0x5B <= v < 0x61
              if (v != 0x5F) return 0;
              // '_'  -->  good!
            }
            else {} // 'a' <= v <= 'z'  -->  good!
          }
          else { // 0x7B <= v < 0xC0
            if (v != 0xB7) return 0;
            // 0xB7  -->  good unless first char!
            if (i == begin) return 0;
          }
        }
        else if (v <= 0x3000) { // 0xC0 <= v <= 0x3000
          if (v < 0x2000) { // 0xC0 <= v < 0x2000
            if (v <= 0x37E) { // 0xC0 <= v <= 0x37E
              if (v < 0x300) { // 0xC0 <= v < 0x300
                if (v <= 0xF7) { // 0xC0 <= v <= 0xF7
                  if (v == 0xD7 || v == 0xF7) return 0;
                  // 0xC0 <= v < 0xD7  or  0xD7 < v < 0xF7  -->  good!
                }
                else {} // 0xF7 < v < 0x300  -->  good!
              }
              else { // 0x300 <= v <= 0x37E
                if (v < 0x370) { // 0x300 <= v < 0x370  -->  good unless first char!
                  if (i == begin) return 0;
                }
                else { // 0x370 <= v <= 0x37E
                  if (v == 0x37E) return 0;
                  // 0x370 <= v < 0x37E  -->  good!
                }
              }
            }
            else {} // 0x37E < v < 0x2000  -->  good!
          }
          else { // 0x2000 <= v <= 0x3000
            if (v < 0x2190) { // 0x2000 <= v < 0x2190
              if (v < 0x2070) { // 0x2000 <= v < 0x2070
                if (v <= 0x203E) { // 0x2000 <= v <= 0x203E
                  if (v < 0x200E) { // 0x2000 <= v < 0x200E
                    if (v <= 0x200B) return 0; // 0x2000 <= v <= 0x200B  -->  bad!
                    // 0x200B < v < 0x200E  -->  good!
                  }
                  else return 0; // 0x200E <= v <= 0x203E  -->  bad!
                }
                else { // 0x203E < v < 0x2070
                  if (v <= 0x2040) { // 0x203E < v <= 0x2040  -->  good unless first char!
                    if (i == begin) return 0;
                  }
                  else return 0; // 0x2040 < v < 0x2070  -->  bad!
                }
              }
              else {} // 0x2070 <= v < 0x2190  -->  good!
            }
            else { // 0x2190 <= v <= 0x3000
              if (v < 0x2C00) return 0; // 0x2190 <= v < 0x2C00  -->  bad!
              if (0x2FF0 <= v) return 0; // 0x2FF0 <= v <= 0x3000  -->  bad!
              // 0x2C00 <= v < 0x2FF0  -->  good!
            }
          }
        }
        else { // 0x3000 < v <= 0xFFFF
          if (0xD800 <= v) { // 0xD800 <= v <= 0xFFFF
            if (v < 0xDC00) { // 0xD800 <= v < 0xDC00
              // Combine UTF-16 surrogates
              if (++i == end) return 0; // Incomplete surrogate pair
              int_type const v2 = traits::to_int_type(*i);
              if (v2 < 0xDC00 || 0xE000 <= v2) return 0; // Invalid high surrogate
              Core::UIntFast32 w = 0x10000 + (Core::UIntFast32(v-0xD800)<<10) + (v2-0xDC00);
              if (0xF0000 <= w) return 0; // 0xF0000 <= w  -->  bad!
              // 0x10000 <= w < 0xF0000  -->  good!
            }
            else { // 0xDC00 <= v <= 0xFFFF
              if (v < 0xFDD0) { // 0xDC00 <= v < 0xFDD0
                if (v < 0xF900) return 0; // 0xDC00 <= v < 0xF900  -->  bad!
                // 0xF900 <= v < 0xFDD0  -->  good!
              }
              else { // 0xFDD0 <= v <= 0xFFFF
                if (v < 0xFDF0) return 0; // 0xFDD0 <= v < 0xFDF0  -->  bad!
                if (0xFFFE <= v) return 0; // 0xFFFE or 0xFFFF  -->  bad!
                // 0xFDF0 <= v < 0xFFFE  -->  good!
              }
            }
          }
          else {} // 0x3000 < v < 0xD800  -->  good!
        }
      }

      return 1;
    }
  }
}


#endif // ARCHON_DOM_IMPL_UTIL_HPP
