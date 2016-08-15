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

#ifndef ARCHON_DOM_UTIL_REF_HPP
#define ARCHON_DOM_UTIL_REF_HPP

#include <ostream>

#include <archon/core/bind_ref.hpp>


namespace archon
{
  namespace dom
  {
    struct null_type {};
    null_type const null = null_type();



    /**
     * All public members and functions defined here have the same
     * meaning as the corresponding members and functions defined for
     * std::shared_ptr<T>.
     */
    template<class T> struct ref
    {
      typedef T element_type;


      ref() throw () {}
      ref(null_type) throw () {}

      template<class U> explicit ref(U *r) throw (): bond(r) {}

      ref(ref const &r) throw (): bond(r.bond) {}
      template<class U> ref(ref<U> const &r) throw (): bond(r.bond) {}

      T *get() const throw () { return bond.get(); }
      T &operator*() const throw () { return *bond.get(); }
      T *operator->() const throw () { return bond.get(); }

      void swap(ref &r) throw () { bond.swap(r.bond); }

      void reset() throw () { ref().swap(*this); }
      template<class U> void reset(U *r) throw () { ref(r).swap(*this); }

      ref &operator=(ref const &r) throw () { bond = r.bond; return *this; }
      template<class U> ref &operator=(ref<U> const &r) throw () { bond = r.bond; return *this; }


    private:
      template<class> friend struct ref;

      typedef core::BindRef<T *> Bond;
      typedef Bond ref::*unspecified_bool_type;

      Bond bond;

    public:
      operator unspecified_bool_type() const throw () { return bond ? &ref::bond : 0; }
    };



    template<class T, class U> bool operator==(ref<T> const &p, ref<U> const &q);

    template<class T, class U> bool operator!=(ref<T> const &p, ref<U> const &q);

    template<class T, class U> bool operator<(ref<T> const &p, ref<U> const &q);

    template<class T> void swap(ref<T> &p, ref<T> &q) throw();

    template<class T, class U> ref<T> static_pointer_cast(ref<U> const &p);

    template<class T, class U> ref<T> dynamic_pointer_cast(ref<U> const &p);

    template<class C, class T, class U>
    std::basic_ostream<C,T> &operator<<(std::basic_ostream<C,T> &out, ref<U> const &p);








    // Template implementations:


    template<class T, class U> inline bool operator==(ref<T> const &r, ref<U> const &s)
    {
      return r.get() == s.get();
    }

    template<class T, class U> inline bool operator!=(ref<T> const &r, ref<U> const &s)
    {
      return r.get() != s.get();
    }

    template<class T, class U> inline bool operator<(ref<T> const &r, ref<U> const &s)
    {
      return r.get() < s.get();
    }

    template<class T> inline void swap(ref<T> &r, ref<T> &s) throw ()
    {
      r.swap(s);
    }

    template<class T, class U> inline ref<T> static_pointer_cast(ref<U> const &r)
    {
      return ref<T>(static_cast<T *>(r.get()));
    }

    template<class T, class U> inline ref<T> dynamic_pointer_cast(ref<U> const &r)
    {
      return ref<T>(dynamic_cast<T *>(r.get()));
    }

    template<class C, class T, class U>
    inline std::basic_ostream<C,T> &operator<<(std::basic_ostream<C,T> &out, ref<U> const &r)
    {
      out << r.get();
      return out;
    }
  }
}


#endif // ARCHON_DOM_UTIL_REF_HPP
