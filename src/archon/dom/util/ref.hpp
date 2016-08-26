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
    /**
     * All public members and functions defined here have the same
     * meaning as the corresponding members and functions defined for
     * std::shared_ptr<T>.
     */
    template<class T> struct ref
    {
      using element_type = T;

      constexpr ref() noexcept {}
      constexpr ref(std::nullptr_t) noexcept {}

      template<class U> explicit ref(U *r) noexcept: m_bond{r} {}

      ref(ref const &r) noexcept: m_bond{r.m_bond} {}
      template<class U> ref(ref<U> const &r) noexcept: m_bond{r.m_bond} {}

      explicit operator bool() const noexcept { return bool(m_bond); }

      T *get() const noexcept { return m_bond.get(); }
      T &operator*() const noexcept { return *m_bond.get(); }
      T *operator->() const noexcept { return m_bond.get(); }

      void swap(ref &r) noexcept { m_bond.swap(r.m_bond); }

      void reset() noexcept { ref().swap(*this); }
      template<class U> void reset(U *r) throw () { ref(r).swap(*this); }

      ref &operator=(ref const &r) noexcept { m_bond = r.m_bond; return *this; }
      template<class U> ref &operator=(ref<U> const &r) noexcept { m_bond = r.m_bond; return *this; }

    private:
      using Bond = core::BindRef<T *>;

      Bond m_bond;

      template<class> friend struct ref;
    };



    template<class T, class U> bool operator==(ref<T> const &p, ref<U> const &q);

    template<class T, class U> bool operator!=(ref<T> const &p, ref<U> const &q);

    template<class T, class U> bool operator<(ref<T> const &p, ref<U> const &q);

    template<class T> void swap(ref<T> &p, ref<T> &q) noexcept;

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
