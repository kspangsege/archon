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
 * \sa http://ootips.org/yonat/4dev/smart-pointers.html
 */

#ifndef ARCHON_CORE_REFCNT_HPP
#define ARCHON_CORE_REFCNT_HPP

#include <algorithm>

#include <archon/core/atomic.hpp>
#include <archon/core/bind_ref.hpp>


namespace archon
{
  namespace core
  {
    typedef BindRefNullTag CntRefNullTag;
    typedef BindRefSafeTag CntRefSafeTag;

    class CntRefObjectBase;



    namespace _impl
    {
      struct CntRefTraits
      {
        static void bind(CntRefObjectBase const *p) throw();
        static bool bind_safe(CntRefObjectBase const *p) throw();
        // May throw if CntRefObjectBase::cnt_ref_dec() throws.
        static void unbind(CntRefObjectBase const *p);
      };
    }



    template<class T> struct CntRef: BindRef<T *, _impl::CntRefTraits>
    {
    private:
      typedef BindRef<T *, _impl::CntRefTraits> Base;


    public:
      typedef T ObjectType;


      /**
       * Construct a counted reference to the specified object.
       *
       * \param p A pointer to a \e dynamically allocated instance of a
       * class that is probably derived from CntRefObjectBase.
       */
      explicit CntRef(T *p = 0) throw(): Base(p) {}


      /**
       * Construct a counted reference to the specified object, but
       * only if the current reference count is greater than zero.
       *
       * This contructor can be used in a multi-threaded environment
       * to achieve the effect of weak pointers, but it can only ever
       * work if you use a non trivial alternative to CntRefObjectBase
       * that does not delete immediately when the reference count
       * reaches zero.
       */
      CntRef(T *p, CntRefSafeTag) throw(): Base(p, BindRefSafeTag()) {}


      /**
       * Efficient swapping that avoids access to the referenced
       * object, in particular, its reference count.
       */
      void swap(CntRef &r) throw() { Base::swap(r); }


      /**
       * Allow comparison between refs to related types.
       */
      template<class U> bool operator==(CntRef<U> const &) const throw();


      /**
       * Allow comparison between refs to related types.
       */
      template<class U> bool operator!=(CntRef<U> const &) const throw();


      /**
       * Construct a null reference. This one allowes you to declare a
       * default argument with an initializer like
       * <tt>CntRefNullTag()</tt>.
       */
      CntRef(CntRefNullTag) throw() {}


      CntRef(CntRef const &r) throw(): Base(r) {}
      template<class U> CntRef(CntRef<U> const &r) throw(): Base(r) {}


      /**
       * \note This operator may throw if <tt>T</tt>'s destructor
       * throws. But even in that case, the target reference will be
       * updated to point to the same object as the source reference
       * points to.
       */
      CntRef &operator=(CntRef const &r) { Base::operator=(r); return *this; }
      template<class U> CntRef &operator=(CntRef<U> const &);
    };



    /**
     * Efficient swapping that avoids access to the referenced object,
     * in particular, its reference count.
     *
     * There is some confusion about how to provide specializations
     * of swap. For a start see
     * http://std.dkuug.dk/jtc1/sc22/wg21/docs/lwg-active.html#226.
     */
    template<class T> inline void swap(CntRef<T> &r, CntRef<T> &s) throw() { r.swap(s); }



    /**
     * Base class for object with intrusinve reference counting.
     *
     * Any class T that derives from this one can be used with
     * CntRef<T> since CntRefObjectBase contains a reference count and
     * the required methods for manipulating it. The manipulation is
     * done automatically and behind the scenes by CntRef<T>.
     *
     * Note: It is normally a good idea to derive virtually from
     * CntRefObjectBase as this allows you to use multiple inheritance
     * when you build up a hierarchy of reference counted objects. An
     * example of this is the X3D node hierachy, A counter example is
     * the stream endpoint objects: ReaderBase and WriterBase. In this
     * case it is important that we get distinct copies of the
     * reference count when we combine them into a BasicPipe.
     *
     * \sa CntRef
     */
    struct CntRefObjectBase
    {
    protected:
      CntRefObjectBase() throw(): n(0) {}

      virtual ~CntRefObjectBase() {}

      void cnt_ref_inc() const throw() { n.inc(); }
      bool cnt_ref_inc_safe() const throw() { return n.inc_if_not_zero(); }
      void cnt_ref_dec() const { if(n.dec_and_zero_test()) delete this; } // May throw if ~T throws

    private:
      friend struct _impl::CntRefTraits;

      mutable Atomic n;
    };


    template<class T> 
    struct CntRefDefs
    {
      typedef CntRef<T>        Ref;
      typedef CntRef<T const>  ConstRef;
      typedef Ref const       &RefArg;
      typedef ConstRef const  &ConstRefArg;
    };






    // Template implementations:

    inline void _impl::CntRefTraits::bind(CntRefObjectBase const *p) throw()
    {
      p->cnt_ref_inc();
    }

    inline bool _impl::CntRefTraits::bind_safe(CntRefObjectBase const *p) throw()
    {
      return p->cnt_ref_inc_safe();
    }

    inline void _impl::CntRefTraits::unbind(CntRefObjectBase const *p)
    {
      p->cnt_ref_dec();
    }

    template<class T> template<class U>
    inline bool CntRef<T>::operator==(CntRef<U> const &r) const throw()
    {
      return Base::operator==(r);
    }


    template<class T> template<class U>
    inline bool CntRef<T>::operator!=(CntRef<U> const &r) const throw()
    {
      return Base::operator!=(r);
    }


    template<class T> template<class U>
    inline CntRef<T> &CntRef<T>::operator=(CntRef<U> const &r)
    {
      Base::operator=(r);
      return *this;
    }
  }
}

#endif // ARCHON_CORE_REFCNT_HPP
