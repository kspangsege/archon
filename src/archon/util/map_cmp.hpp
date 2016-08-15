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

#ifndef ARCHON_UTIL_MAP_CMP_HPP
#define ARCHON_UTIL_MAP_CMP_HPP

#include <stdexcept>
#include <utility>
#include <algorithm>
#include <map>

#include <archon/core/proxy_iter.hpp>

namespace archon
{
  namespace Util
  {
    /**
     * Compare values for matching keys in two maps using a default
     * value whenever a key is not present on both maps. Any
     * comparison function can be used.
     *
     * This function is usefull when two maps should be
     * considered equal even when one has keys not present in the
     * other.
     *
     * \param m1, m2 The maps to be compared.
     *
     * \param v The default value to use in the comparison when one
     * map lacks a key which is in the other map.
     *
     * \param cmp Your favorite comparison function object.
     */
    template<typename Map, typename Cmp>
    bool compare_maps(Map const &m1, Map const &m2,
                      typename Map::mapped_type const &v, Cmp const &cmp)
    {
      typename Map::const_iterator i1 = m1.begin(), i2 = m2.begin();
      for(;;)
      {
        // Finnish the second map if the first one has ended
        if(i1 == m1.end())
        {
          while(i2 != m2.end())
          {
          end_1:
            if(!cmp(v, i2->second)) return false;
            ++i2;
          }
          return true;
        }

        // Finnish the first map if the second one has ended
        if(i2 == m2.end())
        {
          while(i1 != m1.end())
          {
          end_2:
            if(!cmp(i1->second, v)) return false;
            ++i1;
          }
          return true;
        }

      single:
        if(i1->first < i2->first)
        {
          if(!cmp(i1->second, v)) return false;
          if(++i1  == m1.end()) goto end_1;
          goto single;
        }

        if(i2->first < i1->first)
        {
          if(!cmp(v, i2->second)) return false;
          if(++i2  == m2.end()) goto end_2;
          goto single;
        }

        if(!cmp(i1->second, i2->second)) return false;
        ++i1;
        ++i2;
      }
    }



    /**
     * Compare values for matching keys in two maps using a default
     * value whenever a key is not present on both maps.
     *
     * This function is usefull when two maps should be
     * considered equal even when one has keys not present in the
     * other.
     *
     * \param m1, m2 The maps to be compared.
     *
     * \param v The default value to use in the comparison when one
     * map lacks a key which is in the other map.
     */
    template<typename Map>
    inline bool compare_maps(Map const &m1, Map const &m2,
                             typename Map::mapped_type const &v = typename Map::mapped_type())
    {
      return compare_maps(m1, m2, v, std::equal_to<typename Map::mapped_type>());
    }
  }
}

#endif // ARCHON_UTIL_MAP_CMP_HPP
