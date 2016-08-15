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

#ifndef ARCHON_UTIL_RANGE_MAP_HPP
#define ARCHON_UTIL_RANGE_MAP_HPP

#include <stdexcept>
#include <utility>
#include <algorithm>
#include <map>

#include <archon/core/proxy_iter.hpp>
#include <archon/core/iseq.hpp>

namespace archon
{
  namespace util
  {
    /**
     * The purpose of this class is to maintain a set of association
     * ranges and present them in a canonical form.
     *
     * It is an ordinary map in the sense that it represents a
     * function from keys to values. However instead of registering
     * each key/value pair individually it works fundamentally with
     * ranges of keys. The assumption is of course that the
     * associations of the modeled function tend to occur in
     * ranges. If this is not the case, then this class probably does
     * not suit your needs well.
     *
     * The basic idea is that you can perform a number of arbitrary
     * operations on the values of a range of keys, and when you are
     * done you can have the result presented in a canonical, compact
     * and unambiguous way.
     *
     * In the canonical form ranges are non-overlapping and ordered on
     * ascending keys. Further more two adjacent ranges (no keys in
     * between) cannot map to the same value.
     */
    template<typename K, typename V>
    struct RangeMap
    {
      typedef K Key;
      typedef V Value;

      /**
       * Set a new value for a range of keys.
       *
       * \param first The first key in the range whose value should be
       * updated.
       *
       * \param last The final key in the range (inclusive) whose
       * value should be updated.
       *
       * \param v The value to associate with all keys in the
       * specified range.
       */
      void assign(Key first, Key last, Value const &v);

      /**
       * Perform an operatrion on the value associated with each of
       * the keys in the specified range. The operation will be
       * carried out as few times as possible, that is, once for each
       * sub range with a distinct original value.
       *
       * \param first The first key in the range whose value should be
       * updated.
       *
       * \param last The final key in the range (inclusive) whose
       * value should be updated.
       *
       * \param op The operation to perform on the associated
       * values. The passed object is assumed to have a parenthesis
       * operator taking one argument of value reference type. The
       * operator must return true if and only if it changes the
       * argument such that the value is diffrent upon return than it
       * was at entry.
       */
      template<typename Op> void update(Key first, Key last, Op op);

      /**
       * Compare values for each key that is either in this map or the
       * map passed as argument, whenever a key is not in both maps
       * the specified default value will be used in the comparison in
       * place of the missing one. Any comparison function can be
       * used. The comparison function will be invoked as few times as
       * possible, that is, once for each sub key range that is
       * homogeneous in both maps.
       *
       * This function is usefull when when two maps should be
       * considered equal even when one has keys not present in the
       * other.
       *
       * \param m The map to be compare against.
       *
       * \param v The default value to use in the comparison when one
       * map lacks a key which is in the other map.
       *
       * \param cmp Your favorite comparison function object.
       *
       * \note If you want to test for strict equality, then just use
       * the standard equality operator.
       */
      template<typename Cmp> bool compare(RangeMap const &m, V const &v, Cmp const &cmp) const;

    private:
      typedef std::map<Key, std::pair<Key, Value> > Rep;
      typedef typename Rep::iterator RepIter;
      typedef typename Rep::const_iterator ConstRepIter;
      Rep rep;
      struct AssignOp;

    public:
      struct Range: core::ProxyBase<Range, ConstRepIter>
      {
        Key get_first() const { return this->i->first; }
        Key get_last()  const { return this->i->second.first; }
        Value const &get_value()  const { return this->i->second.second; }

        Range(ConstRepIter i): core::ProxyBase<Range, ConstRepIter>(i) {}
      };

      typedef core::IterSeq<core::ProxyIter<Range> > RangeSeq;

      RangeSeq get_ranges() const;
    };




    // Template implementations:


    template<typename K, typename V>
    inline void RangeMap<K, V>::assign(Key first, Key last, Value const &v)
    {
      update(first, last, AssignOp(v));
    }

    template<typename K, typename V> template<typename Op>
    void RangeMap<K, V>::update(Key first, Key last, Op op)
    {
      if(last < first) throw std::invalid_argument("Bad range");

      RepIter i = rep.lower_bound(first);

      // Conditions:
      //
      // - 'i' references the interval that starts where the incoming
      // interval starts or if no such interval exists 'i' references
      // the first interval starting after the start of the incoming
      // interval or if no such interval exists 'i' references a virtual
      // interval starting at infinity.
      //
      // - The incoming interval is not empty.

      // Detect an overlap with the existing interval (if it exists)
      // preceeding the interval referenced by 'i'.
      if(i != rep.begin())
      {
	// We may now assume that 0 < range.first

	RepIter j = i;
	--j;
	if(first <= j->second.first)
	{
	  // The new interval has an overlap with the interval starting
	  // before it

	  // Split the existing interval into two pieces such that the
	  // second piece starts where the incoming interval starts.
	  rep.insert(i, std::make_pair(first, j->second));
	  j->second.first = first-1;

	  // Let 'i' refer to the second of the two pieces resulting
	  // from the split
	  --i;
	}
      }

      // General loop
      for(;;)
      {
	// Conditions:
	//
	// - 'i' references an existing interval or the virtual interval
	// succeeding the last real interval. The virtual interval
	// conceptually starts at positive infinity.
	//
	// - The remaining part of the incoming interval does not start
	// after the start of the existing real/virtual interval
	// referenced by 'i'.
	//
	// - The remaining part of the incoming interval does not
	// overlap with the previous existing real interval 'i-1' if
	// such an interval exists.
	//
	// - The remaining part of the incoming interval is not empty.

	// Does the remaining part of the incloming interval start
	// before the existing interval referenced by 'i'. In this case
	// there is an uncovered range before the interval referenced by
	// 'i'. In the following we refer to the uncovered range as the
	// 'gap'.
	if(i == rep.end() || first < i->first)
	{
	  // If the incoming interval is adjacent to the previous
	  // interval 'i-1' and the epsilon closure of the incoming
	  // state is equal to the state set of the previous interval
	  // just expand the previous interval.
          Value value = Value();
          op(value);
	  bool expanded = false;
	  Key const gapEnd = i == rep.end() ? last :
            std::min(static_cast<Key>(i->first-1), last);
	  if(i != rep.begin())
	  {
	    RepIter j = i;
	    --j;
	    // We may now assume that j->second.fist < range.first
	    if(static_cast<Key>(first-1) == j->second.first &&
	       value == j->second.second)
	    {
	      j->second.first = gapEnd;
	      expanded = true;
	    }
	  }

	  if(!expanded)
	    rep.insert(i, make_pair(first, std::make_pair(gapEnd, value)));

	  // We may now assume that 'i-1' references a real interval

	  // Stop if the remaining part of the incoming interval ends in
	  // the gap without being adjacent to the next interval (the
	  // interval referenced by 'i')?
	  if(i == rep.end() || last < static_cast<Key>(i->first-1)) break;

	  // We may now assume that 'i' references a real interval

	  // Merge intervals if the remaining part of the incoming
	  // interval is adjacent to the next interval (the interval
	  // referenced by 'i') and they have equal state sets?
	  //
	  // Regardless of state set equality if the remaining part of
	  // the incoming interval is adjacent to the next interval
	  // then stop.
	  RepIter j = i;
	  --j;
	  if(last == static_cast<Key>(i->first-1))
	  {
	    if(j->second.second == i->second.second)
	    {
	      j->second.first = i->second.first;
	      rep.erase(i);
	    }
	    break;
	  }

	  // Shorten the incoming interval such that is starts where the
	  // next existing interval starts (the interval referenced by
	  // 'i').
	  first = i->first;
	}

	++i;

	// Conditions:
	//
	// - 'i' references an existing interval or the virtual interval
	// succeeding the last real interval. The virtual interval
	// conceptually starts at positive infinity.
	//
	// - The previous real interval exists.
	//
	// - The remaining part of the incoming interval starts where
	// the previous existing interval does.
	//
	// - The remaining part of the incoming interval is not empty.

	// Handle overlap with next interval
	RepIter j = i;
	--j;

	bool done = false;
	Value value = j->second.second;
	if(op(value))
	{
	  if(last < j->second.first)
	  {
	    rep.insert(i, std::make_pair(static_cast<Key>(last+1), j->second));
	    --i;
	    j->second.first = last;
	    done = true;
	  }
	  else
	  {
	    if(last == j->second.first) done = true;
	    else first = j->second.first+1;
	  }
	  j->second.second = value;
	}
	else
	{
	  if(last <= j->second.first) done = true;
	  else first = j->second.first+1;
	}

	// Should we merge 'i-1' and 'i-2'?
	if(j != rep.begin())
	{
	  RepIter k = j;
	  --k;

	  if(k->second.first == static_cast<Key>(j->first-1) &&
	     k->second.second == j->second.second)
	  {
	    k->second.first = j->second.first;
	    rep.erase(j);
	  }
	}

	if(done) break;
      }
    }


    template<typename K, typename V>
    inline typename RangeMap<K, V>::RangeSeq RangeMap<K, V>::get_ranges() const
    {
      return RangeSeq(Range(rep.begin()), Range(rep.end()));
    }


    template<typename K, typename V>
    struct RangeMap<K, V>::AssignOp
    {
      bool operator()(Value &v) { if(v == w) return false; v = w; return true; }
      AssignOp(Value const &v): w(v) {}
      Value const &w;
    };

    template<typename K, typename V> template<typename Cmp>
    bool RangeMap<K, V>::compare(RangeMap const &m, V const &v, Cmp const &cmp) const
    {
      Rep const &r1 = rep, &r2 = m.rep;
      ConstRepIter i1 = r1.begin(), i2 = r2.begin();
      for(;;)
      {
        // Finnish the second map if the first one has ended
        if(i1 == r1.end())
        {
          while(i2 != r2.end())
          {
          end_1:
            if(!cmp(v, i2->second.second)) return false;
            ++i2;
          }
          return true;
        }

        // Finnish the first map if the second one has ended
        if(i2 == r2.end())
        {
          while(i1 != r1.end())
          {
          end_2:
            if(!cmp(i1->second.second, v)) return false;
            ++i1;
          }
          return true;
        }

      single:
        if(i1->first < i2->first) // Range 1 begins first
        {
        lead_1:
          if(!cmp(i1->second.second, v)) return false;
          if(i2->first <= i1->second.first) goto both; // Overlap
          if(++i1  == r1.end()) goto end_1;
          goto single;
        }

        if(i2->first < i1->first) // Range 2 begins first
        {
        lead_2:
          if(!cmp(v, i2->second.second)) return false;
          if(i1->first <= i2->second.first) goto both; // Overlap
          if(++i2  == r2.end()) goto end_2;
          goto single;
        }

        // Ranges have same beginning if we get here

      both:
        if(!cmp(i1->second, i2->second)) return false;

        Key l1 = i1->second.first, l2 = i2->second.first;
        if(l1 < l2) // Range 1 ends first
        {
          if(++i1 == r1.end()) goto end_1;
          if(l1 + static_cast<Key>(1) < i1->range.first)
            goto lead_2; // Gap detected before range 1
          goto both;
        }
        if(l2 < l1) // Range 2 ends first
        {
          if(++i2 == r2.end()) goto end_2;
          if(l2 + static_cast<Key>(1) < i2->range.first)
            goto lead_1; // Gap detected before range 2
          goto both;
        }

        // Ranges have same end
        ++i1;
        ++i2;
      }
    }
  }
}

#endif // ARCHON_UTIL_RANGE_MAP_HPP
