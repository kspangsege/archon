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

#ifndef ARCHON_UTIL_NULLABLE_STRING_HPP
#define ARCHON_UTIL_NULLABLE_STRING_HPP

#include <cstddef>
#include <cstring>
#include <algorithm>
#include <stdexcept>

#include <archon/core/atomic.hpp>
#include <archon/core/bind_ref.hpp>


namespace archon
{
  namespace util
  {
    /**
     * This class implements a nullable string, that is, a string
     * whose value can be 'null', which is distinct from the empty
     * string.
     *
     * A 'null' string is created by the constructor that takes no
     * arguments.
     *
     * One can test whether a string is the null string by converting
     * it to 'bool' in which case 'false' means that it is null.
     *
     * The character type is assumed to be a POD type.
     */
    template<class T> struct NullableString
    {
      typedef std::size_t size_type;
      typedef T value_type;

      NullableString() throw () {}
      NullableString(T const *p, size_type n) throw (std::bad_alloc): rep(make_rep(p,n)) {}
      NullableString(T const *p) throw (std::bad_alloc); // Null terminated

      T const *data() const throw () { return rep ? &rep->first_char : 0; }
      size_type size() const throw () { return rep ? rep->size : 0; }

      typedef T const *const_iterator;
      typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

      const_iterator begin() const throw () { return data(); }
      const_iterator end() const throw () { return data() + size(); }

      const_reverse_iterator rbegin() const throw () { return end(); }
      const_reverse_iterator rend() const throw () { return begin(); }

      bool operator==(NullableString const &s) const throw ();
      bool operator!=(NullableString const &s) const throw ();

      size_type find(T c, size_type pos = 0) const throw ();

      NullableString substr(size_type pos=0, size_type n=npos) const
        throw (std::out_of_range, std::bad_alloc);

      static const size_type npos = static_cast<size_type>(-1);

    private:
      struct Rep
      {
        core::Atomic num_refs;
        size_type const size;
        T first_char;
        Rep(size_type s): num_refs(0), size(s) {}
      };

      struct BindTraits
      {
        static void bind(Rep *r) throw () { r->num_refs.inc(); }
        static void unbind(Rep *r) throw ();
      };

      typedef core::BindRef<Rep *, BindTraits> NullableString::*unspecified_bool_type;

    public:
      operator unspecified_bool_type() const throw () { return rep ? &NullableString::rep : 0; }

    private:
      static Rep *make_rep(T const *p, size_type n) throw (std::bad_alloc);

      core::BindRef<Rep *, BindTraits> rep;
    };







    // Implementation:

    template<class T> NullableString<T>::NullableString(T const *p) throw (std::bad_alloc):
      rep(make_rep(p, std::strlen(p))) {}


    template<class T> bool NullableString<T>::operator==(NullableString const &s) const throw ()
    {
      if (rep) {
        if (!s.rep) return false;
      }
      else return !s.rep;
      if (rep == s.rep) return true;
      size_t const n = rep->size;
      if (n != s.rep->size) return false;
      T const *const p = &rep->first_char;
      return std::equal(p, p+n, &s.rep->first_char);
    }


    template<class T> bool NullableString<T>::operator!=(NullableString const &s) const throw ()
    {
      if (rep) {
        if (!s.rep) return true;
      }
      else return s.rep;
      if (rep == s.rep) return false;
      size_t const n = rep->size;
      if (n != s.rep->size) return true;
      T const *const p = &rep->first_char;
      return !std::equal(p, p+n, &s.rep->first_char);
    }


    template<class T>
    typename NullableString<T>::size_type NullableString<T>::find(T c, size_type pos) const
      throw ()
    {
      size_type const n = size();
      if (n < pos) return npos;
      const_iterator const b = begin();
      const_iterator const e = b + n;
      const_iterator const i = std::find(b+pos, e, c);
      return i == e ? npos : i - b;
    }


    template<class T>
    NullableString<T> NullableString<T>::substr(size_type pos, size_type n) const
      throw (std::out_of_range, std::bad_alloc)
    {
      size_type const s = size();
      if (s < pos) throw std::out_of_range("Substring position beyond end of string");
      return NullableString(data() + pos, std::min(n, s-pos));
    }


    template<class T>
    typename NullableString<T>::Rep *NullableString<T>::make_rep(T const *p, size_type n)
      throw (std::bad_alloc)
    {
      char *const mem = new char[sizeof(Rep) - sizeof(T) + n*sizeof(T)];
      Rep *const r = new (mem) Rep(n); // Placement new
      std::copy(p, p+n, &r->first_char);
      return r;
    }


    template<class T> void NullableString<T>::BindTraits::unbind(Rep *r) throw ()
    {
      if(r && r->num_refs.dec_and_zero_test()) {
        r->~Rep(); // due to placement new
        delete[] reinterpret_cast<char *>(r);
      }
    }
  }
}

#endif // ARCHON_UTIL_NULLABLE_STRING_HPP
