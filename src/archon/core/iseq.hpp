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
 *
 * This file prevides a few helper classes and function for working
 * with input sequences.
 */

#ifndef ARCHON_CORE_ISEQ_HPP
#define ARCHON_CORE_ISEQ_HPP

namespace Archon
{
  namespace Core
  {
    template<typename Iter> struct IterSeq
    {
      typedef Iter                      IterType;
      typedef typename Iter::value_type ValueType;

      IterSeq(Iter const &begin, Iter const &end): b(begin), e(end) {}

      Iter const &begin() const { return b; }
      Iter const &end()   const { return e; }

      operator bool() const { return b != e; }

      IterSeq &operator++() { ++b; return *this; }
      IterSeq operator++(int) { IterSeq i = *this; ++b; return i; }

      ValueType const &operator*()  const { return *b; }
      ValueType const *operator->() const { return b.operator->(); }

    private:
      Iter b, e;
    };

    template<typename Value> struct OneSeq
    {
      typedef Value const *IterType;
      typedef Value        ValueType;

      OneSeq(Value const &v): value(v), unused(true) {}

      Value const *begin() const { return unused ? &value : &value+1u; }
      Value const *end()   const { return &value+1u; }

      operator bool() const { return unused; }

      OneSeq &operator++() { unused = false; return *this; }
      OneSeq operator++(int) { OneSeq s = *this; unused = false; return s; }

      Value const &operator*()  const { return *begin(); }
      Value const *operator->() const { return  begin(); }

    private:
      Value const &value;
      bool unused;
    };

    template<typename Value> struct NullSeq
    {
      typedef Value const *IterType;
      typedef Value        ValueType;

      Value const *begin() const { return 0; }
      Value const *end()   const { return 0; }

      operator bool() const { return false; }

      NullSeq &operator++() { return *this; }
      NullSeq operator++(int) { return *this; }

      Value const &operator*()  const { return *begin(); }
      Value const *operator->() const { return  begin(); }
    };

    template<typename Container> inline IterSeq<typename Container::const_iterator> makeSeq(Container const &c)
    {
      return IterSeq<typename Container::const_iterator>(c.begin(), c.end());
    }

    template<typename Value> inline OneSeq<Value> oneSeq(Value const &v)
    {
      return OneSeq<Value>(v);
    }

    template<typename Value> inline NullSeq<Value> nullSeq()
    {
      return NullSeq<Value>();
    }
  }
}

#endif // ARCHON_CORE_ISEQ_HPP
