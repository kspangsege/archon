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

#ifndef ARCHON_UTIL_PAGED_MAP_HPP
#define ARCHON_UTIL_PAGED_MAP_HPP

#include <map>

#include <archon/core/assert.hpp>
#include <archon/core/types.hpp>

namespace Archon
{
  namespace Util
  {
    /**
     * This map is much more efficient in terms of both memory
     * consumption and lookup speed when the keys have a strong
     * tendency to be grouped together.
     *
     * The entire range of <tt>K</tt>, which must be an integer type,
     * is devided into pages of a fixed size. The lookup operation
     * then starts by dividing the key into a page ID and an in-page
     * position. The page-ID is then looked up in a conventional map
     * to identify the relevant page. The page is a simple array which
     * offers constant time lookup.
     *
     * It is your responsibility to choose an appropriate page
     * size. If it is too big, too much memory is wasted because pages
     * are filled too sparsely. If it is too small, the lookup
     * operation becomes too slow because the page map becomes
     * unnecessarily big.
     *
     * \tparam K The 'key' type of this map. It must be an integer
     * type.
     *
     * \tparam V The 'value' type of this map.
     */
    template<class K, class V> struct PagedMap
    {
      PagedMap(int page_size): page_size(page_size) {}

      ~PagedMap();

      V &operator[](K k);

    private:
      ARCHON_STATIC_ASSERT(std::numeric_limits<K>::is_integer, "Key type must be integer");
      static int const uint_bits =
        (sizeof (unsigned) < sizeof (K) ? sizeof (K) : sizeof (unsigned)) *
        std::numeric_limits<unsigned char>::digits;
      typedef typename Core::SmallestUnsignedWithBits<uint_bits>::type uint_type;
      ARCHON_STATIC_ASSERT(sizeof (K) <= sizeof (uint_type), "Key type has too many bits");
      typedef std::map<uint_type, V *> map_type;

      int const page_size;
      map_type map;
    };







    // Implamentation:

    template<class K, class V> inline PagedMap<K,V>::~PagedMap()
    {
      for(typename map_type::iterator i = map.begin(); i != map.end(); ++i) delete[] i->second;
    }


    template<class K, class V> inline V &PagedMap<K,V>::operator[](K k)
    {
      uint_type u = k;
      unsigned const pos = u % uint_type(page_size);
      uint_type const page = u / uint_type(page_size);
      V *&p = map[page];
      if(!p)
      {
        p = new V[page_size];
        std::fill(p, p+page_size, V()); // FIXME: This is not necessary if V is of class type
      }
      return p[pos];
    }
  }
}

#endif // ARCHON_UTIL_PAGED_MAP_HPP
