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

#ifndef ARCHON_CORE_TEXT_JOIN_HPP
#define ARCHON_CORE_TEXT_JOIN_HPP

#include <string>
#include <ostream>
#include <sstream>


namespace archon
{
  namespace core
  {
    namespace _Impl { template<class, class> struct TextJoin; }


    /**
     * Produce a character string by joining the elements of the
     * specified sequence using the specified delimiter.
     *
     * Examples of use:
     *
     * <pre>
     *
     *   void func(std::vector<int> const &vec)
     *   {
     *     cout  << text_join(v.begin(), v.end(),  ",") << endl;
     *     wcout << text_join(v.begin(), v.end(), L",") << endl;
     *     string s = text_join(v.begin(), v.end(), ",");
     *   }
     *
     * </pre>
     *
     * Note that for the sake of efficiency no intermediate result is
     * constructed from the joining when writing to a stream. The
     * individual elements are written directly to the target stream.
     */
    template<class FwdIn, class Delim>
    _Impl::TextJoin<FwdIn, Delim> text_join(FwdIn begin, FwdIn end, Delim const &delim);







    // Implementation:

    namespace _Impl
    {
      template<class FwdIn, class Delim> struct TextJoin
      {
        TextJoin(FwdIn b, FwdIn e, Delim const &d): begin(b), end(e), delim(d) {}
        FwdIn const begin, end;
        Delim const &delim;
        template<class Ch> operator std::basic_string<Ch>() const
        {
          std::basic_stringstream<Ch> out;
          out << *this;
          return out.str();
        }
      };

      template<class Ch, class Tr, class FwdIn, class Delim>
      inline std::basic_ostream<Ch, Tr> &operator<<(std::basic_ostream<Ch, Tr> &out,
                                                    TextJoin<FwdIn, Delim> const &join)
      {
        if(join.begin != join.end)
        {
          out << *join.begin;
          FwdIn i = join.begin;
          while(++i != join.end) out << join.delim << *i;
        }
        return out;
      }
    }

    template<class FwdIn, class Delim>
    inline _Impl::TextJoin<FwdIn, Delim> text_join(FwdIn begin, FwdIn end, Delim const &delim)
    {
      return _Impl::TextJoin<FwdIn, Delim>(begin, end, delim);
    }
  }
}

#endif // ARCHON_CORE_TEXT_JOIN_HPP
