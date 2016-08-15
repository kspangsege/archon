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

#ifndef ARCHON_CORE_SHARED_PTR_HPP
#define ARCHON_CORE_SHARED_PTR_HPP

#include <stdexcept>
#include <algorithm>
#include <ostream>

#include <archon/core/bind_ref.hpp>
#include <archon/core/atomic.hpp>


namespace archon
{
  namespace core
  {
    namespace _Impl { struct SharedPtrPrimaryCount; struct SharedPtrSecondaryCount; }

    template<class T> struct WeakPtr;
    struct BadWeakPtr;


    template<class T> struct SharedPtr
    {
      typedef T element_type;

      SharedPtr() throw (): ptr(0) {}

      /**
       * May throw.
       */
      template<class U> explicit SharedPtr(U *);

      SharedPtr(SharedPtr const &p) throw (): ptr(p.ptr), count(p.count) {}
      template<class U> SharedPtr(SharedPtr<U> const &) throw ();

      template<class U> explicit SharedPtr(WeakPtr<U> const &p) throw(BadWeakPtr);

      T *get() const throw () { return ptr; }
      T &operator*() const throw () { return *ptr; }
      T *operator->() const throw () { return ptr; }

      void swap(SharedPtr &p) throw () { using std::swap; swap(ptr, p.ptr); count.swap(p.count); }

      /**
       * May throw if <tt>T</tt>'s destructor throws, but even in this
       * case the pointer object is set to be a 'null pointer'.
       */
      void reset() { SharedPtr().swap(*this); }

      /**
       * May throw, in which case one of two things happens; if the
       * exception is thrown from <tt>T</tt>'s destructor, then this
       * pointer object is still updated to point to the specified
       * object. Otherwise, if this method throws, the pointer object
       * will be unchanged.
       */
      template<class U> void reset(U *p) { SharedPtr(p).swap(*this); }

    private:
      typedef T *SharedPtr::*unspecified_bool_type;

    public:
      operator unspecified_bool_type() const throw () { return ptr ? &SharedPtr::ptr : 0; }

      SharedPtr &operator=(SharedPtr const &);
      template<class U> SharedPtr &operator=(SharedPtr<U> const &);

    private:
      template<class U> friend struct SharedPtr;
      template<class U> friend struct WeakPtr;

      SharedPtr(T *p, _Impl::SharedPtrPrimaryCount *c) throw ();

      SharedPtr(T *p, _Impl::SharedPtrSecondaryCount *c) throw (): ptr(p), count(c) {}

      template<class V, class U>
      friend SharedPtr<V> static_pointer_cast(SharedPtr<U> const &);

      template<class V, class U>
      friend SharedPtr<V> dynamic_pointer_cast(SharedPtr<U> const &);

      // Invariant: 'ptr' is null if, and only if 'count' is null.
      T *ptr;
      BindRef<_Impl::SharedPtrSecondaryCount *> count;
    };


    template<class T, class U> bool operator==(SharedPtr<T> const &p, SharedPtr<U> const &q);

    template<class T, class U> bool operator!=(SharedPtr<T> const &p, SharedPtr<U> const &q);

    template<class T, class U> bool operator<(SharedPtr<T> const &p, SharedPtr<U> const &q);

    template<class T> void swap(SharedPtr<T> &p, SharedPtr<T> &q) throw ();

    template<class T, class U> SharedPtr<T> static_pointer_cast(SharedPtr<U> const &p);

    template<class T, class U> SharedPtr<T> dynamic_pointer_cast(SharedPtr<U> const &p);

    template<class C, class T, class U>
    std::basic_ostream<C,T> &operator<<(std::basic_ostream<C,T> &out, SharedPtr<U> const &p);









    // Implementation:


    struct BadWeakPtr: std::runtime_error
    {
      BadWeakPtr(): std::runtime_error("std::bad_weak_ptr") {}
    };


    namespace _Impl
    {
      struct SharedPtrPrimaryCount
      {
        SharedPtrPrimaryCount() throw (): n(1) {}
        void bind_ref() throw () { n.inc(); }
        bool bind_ref_safe() throw () { return n.inc_if_not_zero(); }
        void unbind_ref() throw ();
        Atomic n;
      };

      struct SharedPtrSecondaryCount
      {
        SharedPtrSecondaryCount() throw (): n(0) {}
        void bind_ref() throw () { n.inc(); }
        bool bind_ref_safe() throw () { return n.inc_if_not_zero(); }
        void unbind_ref();
        Atomic n;
      };

      struct SharedPtrCountBase: SharedPtrPrimaryCount, SharedPtrSecondaryCount
      {
        virtual void release() = 0;
        virtual void dispose() throw () = 0;
        virtual ~SharedPtrCountBase() {}
      };

      inline void SharedPtrPrimaryCount::unbind_ref() throw ()
      {
        if(n.dec_and_zero_test()) static_cast<SharedPtrCountBase *>(this)->dispose();
      }

      inline void SharedPtrSecondaryCount::unbind_ref()
      {
        if(n.dec_and_zero_test()) static_cast<SharedPtrCountBase *>(this)->release();
      }

      template<class T> struct SharedPtrCount: SharedPtrCountBase
      {
        SharedPtrCount(T *p) throw (): ptr(p) {}

        void release()
        {
          T *p = ptr;
          static_cast<SharedPtrPrimaryCount *>(this)->unbind_ref();
          delete p;
        }

        void dispose() throw () { delete this; }

        T *ptr;
      };

      /**
       * \param p May be null. Ownership is passed from caller to callee.
       *
       * \return Is null iff <tt>p</tt> is null. Ownership is passed
       * from callee to caller.
       */
      template<class T> inline SharedPtrSecondaryCount *make_shared_ptr_count(T *ptr)
      {
        if(!ptr) return 0;
        try
        {
          return new SharedPtrCount<T>(ptr);
        }
        catch(...)
        {
          delete ptr;
          throw;
        }
      }
    }


    template<class T> template<class U>
    inline SharedPtr<T>::SharedPtr(U *p): ptr(p), count(_Impl::make_shared_ptr_count(p)) {}

    template<class T> template<class U>
    inline SharedPtr<T>::SharedPtr(SharedPtr<U> const &p) throw (): ptr(p.ptr), count(p.count) {}

    template<class T> template<class U>
    inline SharedPtr<T>::SharedPtr(WeakPtr<U> const &p) throw(BadWeakPtr):
      count(static_cast<_Impl::SharedPtrCountBase *>(p.count.get()), BindRefSafeTag())
    {
      // The copying of p.ptr may involve polymorphic type conversion,
      // and therfore it may involve access to the object pointed to
      // by p.ptr. Because of this, it is crucial that we only attempt
      // the copy if a secondary reference to the 'count' object could
      // be acquired, since this guarantees that the object pointed to
      // be p.ptr is not yet deallocated.
      if(count) ptr = p.ptr;
      else throw BadWeakPtr();
    }

    template<class T>
    inline SharedPtr<T>::SharedPtr(T *p, _Impl::SharedPtrPrimaryCount *c) throw ():
      count(static_cast<_Impl::SharedPtrCountBase *>(c), BindRefSafeTag())
    {
      ptr = count ? p : 0;
    }

    template<class T>
    inline SharedPtr<T> &SharedPtr<T>::operator=(SharedPtr<T> const &p)
    {
      ptr = p.ptr;
      count = p.count;
      return *this;
    }

    template<class T> template<class U>
    inline SharedPtr<T> &SharedPtr<T>::operator=(SharedPtr<U> const &p)
    {
      ptr = p.ptr;
      count = p.count;
      return *this;
    }

    template<class T, class U>
    inline bool operator==(SharedPtr<T> const &p, SharedPtr<U> const &q)
    {
      return p.get() == q.get();
    }

    template<class T, class U>
    inline bool operator!=(SharedPtr<T> const &p, SharedPtr<U> const &q)
    {
      return p.get() != q.get();
    }

    template<class T, class U>
    inline bool operator<(SharedPtr<T> const &p, SharedPtr<U> const &q)
    {
      return p.get() < q.get();
    }

    template<class T>
    inline void swap(SharedPtr<T> &p, SharedPtr<T> &q) throw ()
    {
      p.swap(q);
    }

    template<class T, class U>
    inline SharedPtr<T> static_pointer_cast(SharedPtr<U> const &p)
    {
      return SharedPtr<T>(static_cast<T *>(p.ptr), p.count.get());
    }

    template<class T, class U>
    inline SharedPtr<T> dynamic_pointer_cast(SharedPtr<U> const &p)
    {
      T *q = dynamic_cast<T *>(p.ptr);
      return q ? SharedPtr<T>(q, p.count.get()) : SharedPtr<T>();
    }

    template<class C, class T, class U>
    inline std::basic_ostream<C,T> &operator<<(std::basic_ostream<C,T> &out, SharedPtr<U> const &p)
    {
      out << p.get();
      return out;
    }
  }
}

#endif // ARCHON_CORE_SHARED_PTR_HPP
