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

#ifndef ARCHON_CORE_BIND_REF_HPP
#define ARCHON_CORE_BIND_REF_HPP


#include <algorithm>


namespace archon
{
  namespace Core
  {
    struct BindRefNullTag {};
    struct BindRefSafeTag {};




    struct DefaultBindTraits
    {
      template<class R> static void bind(R r)      throw () { r->bind_ref();             }
      template<class R> static bool bind_safe(R r) throw () { return r->bind_ref_safe(); }
      template<class R> static void unbind(R r)    throw () { r->unbind_ref();           }
    };




    /**
     * This class provides the fundamental properties of a particular
     * category of smart pointers. The characteristic property for
     * this category of smart pointers is, that the referenced object
     * is informed whenever a pointer is created or destroyed. The
     * obvious example is intrusive reference counting.
     *
     * The traits type must define the following static members, where
     * 'R' is the type passed as the first template argument (or a
     * type that the first template argument is convertible to):
     *
     * <pre>
     *
     *   void bind(R) throw()
     *   bool bind_safe(R) throw() // Only if target is bound already
     *   void unbind(R) // May throw
     *
     * </pre>
     *
     * 'bind_safe' is only necessary if you are going to call the
     * constructor that takes a BindRefSafeTag argument.
     *
     *
     * \tparam R The type of a "raw" reference. It must be copyable,
     * assignable, equality comparable, default constructible
     * (yielding a null reference), and convertible to \c bool with \c
     * false indicating a null reference.
     *
     * \tparam T The traits type.
     */
    template<class R, class T = DefaultBindTraits> struct BindRef
    {
      typedef R ref_type;
      typedef T traits_type;


      /**
       * Construct a counted reference to the specified resource.
       *
       * \param p An uncounted/raw resource reference.
       */
      explicit BindRef(ref_type const &r = ref_type()) throw() { bind(r); }


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
      BindRef(ref_type const &r, BindRefSafeTag) throw() { bind_safe(r); }


      /**
       * Retrieve a copy of the conventional pointer managed by this
       * reference object.
       *
       * Please note that the pointer can be rendered invalid at any
       * time unless you can guarantee that at least one counted
       * reference to the target object remains in existence for as
       * long as you access the target object through the the
       * conventional pointer.
       *
       * The easiest way to provide such a guarantee is by ensuring
       * that the reference object that acted as a source of the
       * conventional pointer stays contsant for as long as you access
       * the target object through the conventional pointer.
       */
      ref_type get() const throw() { return ref; }


      ref_type operator->() const throw() { return ref; }


      /**
       * Safely set a new target object for this reference object or if
       * the argument is null, change it safely into a null reference.
       *
       * Any previously referenced object will have its reference
       * count decremented and if the argument was not null, the new
       * target object will have its reference count incremented.
       *
       * \note This method may throw if traits_type::unbind()
       * throws. But even in that case, this reference will be updated
       * to point to the specified object.
       *
       * \note This method never throws if this reference is a null
       * reference before the call.
       */
      void reset(ref_type const &r = ref_type());


      /**
       * Efficient swapping that avoids binding and unbinding.
       */
      void swap(BindRef &r) throw() { using std::swap; swap(ref, r.ref); }


      /**
       * Allow comparison between related ref types.
       */
      template<class S, class U> bool operator==(BindRef<S,U> const &) const throw();

      /**
       * Allow comparison between related ref types.
       */
      template<class S, class U> bool operator!=(BindRef<S,U> const &) const throw();


    protected:
      typedef ref_type BindRef::*unspecified_bool_type;

    public:
      /**
       * Test if this is a proper reference (ie. not a null
       * reference.)
       *
       * \return False iff this is a null reference.
       */
      operator unspecified_bool_type() const throw();


      /**
       * Construct a null reference. This one allowes you to declare a
       * default argument with an initializer like
       * <tt>CntRefNullTag()</tt>.
       */
      BindRef(BindRefNullTag) throw(): ref(ref_type()) {}


      BindRef(BindRef const &r) throw() { bind(r.ref); }
      template<class S> BindRef(BindRef<S,T> const &r) throw() { bind(r.get()); }


      /**
       * \note This operator may throw if traits_type::unbind()
       * throws, but even in that case, the target reference will be
       * updated to point to the same object as the source reference
       * points to.
       */
      BindRef &operator=(BindRef const &);
      template<class S> BindRef &operator=(BindRef<S,T> const &);


      /**
       * \note This destructor may throw if traits_type::unbind()
       * throws.
       */
      ~BindRef() { unbind(ref); }


    private:
      void bind(ref_type const &r) throw();
      void bind_safe(ref_type const &r) throw();
      void unbind(ref_type const &r);

      ref_type ref;
    };



    /**
     * Efficient swapping that avoids access to the referenced object,
     * in particular, its reference count.
     *
     * There is some confusion about how to provide specializations
     * of swap. For a start see
     * http://std.dkuug.dk/jtc1/sc22/wg21/docs/lwg-active.html#226.
     */
    template<class R, class T>
    inline void swap(BindRef<R,T> &r, BindRef<R,T> &s) throw() { r.swap(s); }








    // Template implementations:


    template<class R, class T>
    inline void BindRef<R,T>::reset(ref_type const &r)
    {
      if(r == ref) return;
      ref_type s(ref);
      bind(r);   // Never throws.
      unbind(s); // May throw if traits_type::unbind() throws.
    }


    template<class R, class T> template<class S, class U>
    inline bool BindRef<R,T>::operator==(BindRef<S,U> const &r) const throw()
    {
      return ref == r.get();
    }


    template<class R, class T> template<class S, class U>
    inline bool BindRef<R,T>::operator!=(BindRef<S,U> const &r) const throw()
    {
      return ref != r.get();
    }


    template<class R, class T>
    inline BindRef<R,T>::operator unspecified_bool_type() const throw()
    {
      return ref ? &BindRef::ref : 0;
    }


    template<class R, class T>
    inline BindRef<R,T> &BindRef<R,T>::operator=(BindRef const &r)
    {
      reset(r.ref);
      return *this;
    }


    template<class R, class T> template<class S>
    inline BindRef<R,T> &BindRef<R,T>::operator=(BindRef<S,T> const &r)
    {
      reset(r.get());
      return *this;
    }


    template<class R, class T> inline void BindRef<R,T>::bind(ref_type const &r) throw()
    {
      if(r)
          traits_type::bind(r);
      ref = r;
    }


    template<class R, class T> inline void BindRef<R,T>::bind_safe(ref_type const &r) throw()
    {
      ref = r && traits_type::bind_safe(r) ? r : ref_type();
    }


    // May throw if traits_type::unbind() throws.
    template<class R, class T> inline void BindRef<R,T>::unbind(ref_type const &r)
    {
      if(r) traits_type::unbind(r);
    }
  }
}

#endif // ARCHON_CORE_BIND_REF_HPP
