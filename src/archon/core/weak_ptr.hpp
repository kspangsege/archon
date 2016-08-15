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

#ifndef ARCHON_CORE_WEAK_PTR_HPP
#define ARCHON_CORE_WEAK_PTR_HPP

#include <algorithm>

#include <archon/core/shared_ptr.hpp>


namespace archon
{
  namespace core
  {
    template<typename T> struct WeakPtr
    {
      typedef T element_type;

      WeakPtr() throw(): ptr(0) {}
      WeakPtr(WeakPtr const &p) throw(): ptr(p.ptr), count(p.count) {}

      template<typename U> WeakPtr(WeakPtr<U> const &p) throw();

      template<typename U> WeakPtr(SharedPtr<U> const &p) throw();

      WeakPtr &operator=(WeakPtr const &p) throw();

      template<typename U> WeakPtr &operator=(WeakPtr<U> const &p) throw();

      template<typename U> WeakPtr &operator=(SharedPtr<U> const &p) throw();

      void swap(WeakPtr &p) throw() { std::swap(ptr, p.ptr); count.swap(p.count); }
      void reset() throw() { ptr = 0; count.reset(); }

      SharedPtr<T> lock() const throw();

    private:
      template<typename U> friend struct SharedPtr;
      template<typename U> friend struct WeakPtr;

      // Invariant: 'ptr' is null if 'count' is null.
      // Invariant: 'ptr' is not null if 'count' has secondary reference count greater than zero.
      T *ptr;
      BindRef<_impl::SharedPtrPrimaryCount *> count;
    };


    template<typename T> void swap(WeakPtr<T> &p, WeakPtr<T> &q) throw();









    // Implementation:


    template<typename T> template<typename U>
    inline WeakPtr<T>::WeakPtr(WeakPtr<U> const &p) throw(): count(p.count)
    {
      // We cannot simply copy p.ptr, since that may involve
      // polymorphic type conversion, and therfore it may involve
      // access to the object pointed to by p.ptr, which may or may
      // not be destroyed at this point.
      ptr = p.lock().get();
    }

    template<typename T> template<typename U>
    inline WeakPtr<T>::WeakPtr(SharedPtr<U> const &p) throw():
      ptr(p.ptr), count(static_cast<_impl::SharedPtrCountBase *>(p.count.get())) {}

    template<typename T>
    inline WeakPtr<T> &WeakPtr<T>::operator=(WeakPtr<T> const &p) throw()
    {
      ptr = p.ptr;
      count = p.count;
      return *this;
    }

    template<typename T> template<typename U>
    inline WeakPtr<T> &WeakPtr<T>::operator=(WeakPtr<U> const &p) throw()
    {
      // We cannot simply copy p.ptr, since that may involve
      // polymorphic type conversion, and therfore it may involve
      // access to the object pointed to by p.ptr, which may or may
      // not be destroyed at this point.
      ptr = p.lock().get();
      count = p.count;
      return *this;
    }

    template<typename T> template<typename U>
    inline WeakPtr<T> &WeakPtr<T>::operator=(SharedPtr<U> const &p) throw()
    {
      ptr = p.ptr;
      count.reset(static_cast<_impl::SharedPtrCountBase *>(p.count.get()));
      return *this;
    }

    template<typename T>
    inline SharedPtr<T> WeakPtr<T>::lock() const throw()
    {
      return SharedPtr<T>(ptr, count.get());
    }

    template<typename T>
    inline void swap(WeakPtr<T> &p, WeakPtr<T> &q) throw()
    {
      p.swap(q);
    }
  }
}

#endif // ARCHON_CORE_WEAK_PTR_HPP
