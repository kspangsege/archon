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

#ifndef ARCHON_UTIL_REMEM_ORDER_MAP_HPP
#define ARCHON_UTIL_REMEM_ORDER_MAP_HPP

#include <utility>
#include <map>
#include <vector>

#include <archon/core/iterator.hpp>


namespace Archon
{
  namespace Util
  {
    /**
     * \note Erasing an element is slow since it involved a linear
     * search through all elements.
     */
    template<class K, class V> struct RememOrderMap
    {
    private:
      typedef std::map<K,V> Map;
      typedef std::vector<typename Map::iterator> Vec;
      typedef Core::DoubleDeref<typename Vec::iterator> Deref;
      typedef Core::DoubleDeref<typename Vec::const_iterator> ConstDeref;

    public:
      typedef K                            key_type;
      typedef V                            mapped_type;
      typedef typename Map::value_type     value_type;
      typedef typename Vec::size_type      size_type;
      typedef typename Map::iterator       iterator;
      typedef typename Map::const_iterator const_iterator;

      size_type size() const { return vec.size(); }

      iterator begin()          { return map.begin(); }
      iterator end()            { return map.end(); }
      iterator find(K const &k) { return map.find(k); }

      const_iterator begin()          const { return map.begin(); }
      const_iterator end()            const { return map.end(); }
      const_iterator find(K const &k) const { return map.find(k); }

      std::pair<iterator, bool> insert(value_type const &v);
      size_type erase(K const &k);
      void erase(iterator i);
      void clear();


      /**
       * Special random access iterator that remembers the order in
       * which associations are added to this map.
       */
      typedef Core::CustomDerefIter<Deref,      value_type>       remem_order_iterator;
      typedef Core::CustomDerefIter<ConstDeref, value_type const> const_remem_order_iterator;

      remem_order_iterator remem_order_begin() { return remem_order_iterator(vec.begin()); }
      remem_order_iterator remem_order_end()   { return remem_order_iterator(vec.end());   }

      const_remem_order_iterator remem_order_begin() const
      {
        return const_remem_order_iterator(vec.begin());
      }

      const_remem_order_iterator remem_order_end() const
      {
        return const_remem_order_iterator(vec.end());
      }

    private:
      Map map;
      Vec vec;
    };







    // Implamentation:

    template<class K, class V>
    inline std::pair<typename RememOrderMap<K,V>::iterator, bool>
    RememOrderMap<K,V>::insert(value_type const &v)
    {
      std::pair<iterator, bool> const r = map.insert(v);
      if(r.second) vec.push_back(r.first);
      return r;
    }

    template<class K, class V>
    inline typename RememOrderMap<K,V>::size_type RememOrderMap<K,V>::erase(K const &k)
    {
      iterator const i = find(k);
      if(i == end()) return 0;
      erase(i);
      return 1;
    }

    template<class K, class V> inline void RememOrderMap<K,V>::erase(iterator i)
    {
      vec.erase(i);
      map.erase(i);
    }

    template<class K, class V> inline void RememOrderMap<K,V>::clear()
    {
      vec.clear();
      map.clear();
    }
  }
}

#endif // ARCHON_UTIL_REMEM_ORDER_MAP_HPP
