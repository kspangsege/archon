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

#ifndef ARCHON_UTIL_REP_MAP_LOOKUP_BOOST_HPP
#define ARCHON_UTIL_REP_MAP_LOOKUP_BOOST_HPP

#include <algorithm>
#include <utility>


namespace archon
{
  namespace util
  {
    /**
     * A heuristic map lookup efficiency booster based on the
     * assumption that the sequence of lookups contain sections where
     * all the lookups are on the same key or from a small group of
     * keys. The longer these sections are, the more you gain from
     * using this booster.
     *
     * The entries in the underlying map must remain constant during
     * the usage of an instance of this class. New entries may be
     * added at any time though.
     *
     * \tparam Any type similar to std::map.
     *
     * \tparam N A small number greater than or equal to one,
     * corresponding to the assumed characteristic group size of the
     * lookup sequence.
     */
    template<class Map, int N=3> struct RepMapLookupBooster
    {
      RepMapLookupBooster(Map *m): map(*m), num_fast_refs(0) {}

      typedef typename Map::key_type Key;
      typedef typename Map::mapped_type Value;
      typedef typename Map::iterator Iter;

      Iter find(Key k);

      Value &operator[](Key k) { return find_or_insert(k)->second; }

      /**
       * Same as find() but inserts an entry with the default value if
       * not found.
       */
      Iter find_or_insert(Key k);

    private:
      Iter fast_find(Key k);
      void fast_insert(Key k, Iter i);

      Map &map;

      struct FastRef
      {
        Key  key;
        Iter iter;
      };
      FastRef fast_refs[N];
      int num_fast_refs;
    };






    // Implementation:

    template<class M, int N>
    inline typename RepMapLookupBooster<M,N>::Iter RepMapLookupBooster<M,N>::find(Key k)
    {
      Iter i = fast_find(k);
      Iter const e = map.end();
      if(i != e) return i;
      i = map.find(k);
      if(i == e) return e;
      fast_insert(k,i);
      return i;
    }


    template<class M, int N>
    inline typename RepMapLookupBooster<M,N>::Iter RepMapLookupBooster<M,N>::find_or_insert(Key k)
    {
      Iter i = fast_find(k);
      Iter const e = map.end();
      if(i != e) return i;
      i = map.lower_bound(k);
      // i->first is greater than or equal to k
      if(i == e || i->first != k) i = map.insert(i, std::make_pair(k, Value()));
      fast_insert(k,i);
      return i;
    }


    template<class M, int N>
    inline typename RepMapLookupBooster<M,N>::Iter RepMapLookupBooster<M,N>::fast_find(Key k)
    {
      if(0 < num_fast_refs)
      {
        if(fast_refs[0].key == k) return fast_refs[0].iter;
        for(int i=1; i<num_fast_refs; ++i)
        {
          FastRef &r = fast_refs[i];
          if(r.key == k)
          {
            FastRef &s = fast_refs[i-1];
            // Move one step closer to the front
            using std::swap;
            swap(r.key, s.key);
            swap(r.iter, s.iter);
            return s.iter;
          }
        }
      }
      return map.end();
    }


    template<class M, int N> inline void RepMapLookupBooster<M,N>::fast_insert(Key k, Iter i)
    {
      FastRef &r = fast_refs[num_fast_refs == N ? N-1 : num_fast_refs++];
      r.key = k;
      r.iter = i;
    }
  }
}

#endif // ARCHON_UTIL_REP_MAP_LOOKUP_BOOST_HPP
