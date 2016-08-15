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

#ifndef ARCHON_CORE_UNIQUE_PTR_HPP
#define ARCHON_CORE_UNIQUE_PTR_HPP

#include <algorithm>


namespace archon
{
  namespace core
  {
    template<class T> struct DefaultDelete
    {
      void operator()(T *p) const { delete p; }
    };


    /**
     * This class is a replacement for <tt>std::auto_ptr</tt>
     * providing a higher degree of safety in the presence of
     * exceptions. That is, \c std::auto_ptr has serious flaws in its
     * design that can easily lead to memory leaks. The problem is the
     * implicit conversion to <tt>std::auto_ptr_ref</tt> which is
     * performed every time an <tt>std::auto_ptr</tt> is copied. The
     * following example illustrates this:
     *
     * <pre>
     *
     *   void f(int, auto_ptr<int>) {}                 // 1
     *   int g() { throw runtime_error("oops"); }      // 2
     *   void h() { f(g(), auto_ptr<int>(new int)); }  // 3
     *
     * </pre>
     *
     * Assuming that arguments are evaluated right-to-left, the \c
     * std::auto_ptr in line 3 is first converted to a
     * <tt>std::auto_ptr_ref</tt>, then \c g is called which
     * throws. Now, since the \c std::auto_ptr no longer owns the
     * memory, and, by design, the <tt>auto_ptr_ref</tt> does not
     * deallocate its memory, we have a leak.
     *
     * The solution is to only reproduce the safe subset of
     * <tt>std::auto_ptr</tt>'s functionality, such that the compiler
     * will catch unsafe constructs like the one above.
     *
     * Passing ownership into function:
     *
     * <pre>
     *
     *   struct A {};
     *   struct B: A {};
     *
     *   void f(UniquePtr<A> a) { ... }
     *   void g(UniquePtr<B> b) { f(b); } // Not possible - hmm...
     *
     * </pre>
     *
     * Passing ownership out of function:
     *
     * <pre>
     *
     *   UniquePtr<B> f() { ... }
     *   UniquePtr<A> g() { UniquePtr<B> b(f().release()); return b; }
     *   void h() { UniquePtr<A> a(f().release()); ... }
     *
     * </pre>
     *
     * Be sure to never assign a \c UniquePtr to itself:
     *
     * <pre>
     *
     *   A *p = new A;
     *   UniquePtr<A> a(p);
     *   a.reset(p); // Illegal - unspecified behaviour
     *   a = a;      // Illegal - unspecified behaviour
     *   UniquePtr<A> &b = a;
     *   b = a;      // Illegal - unspecified behaviour
     *
     * </pre>
     *
     *
     * \sa http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2005/n1908.html#463
     * \sa http://www.aristeia.com/BookErrata/auto_ptr-update.html
     * \sa http://www.boost.org/doc/libs/1_37_0/libs/smart_ptr/scoped_ptr.htm
     * \sa http://anubis.dkuug.dk/jtc1/sc22/wg21/docs/cwg_defects.html#84 (low relevance)
     */
    template<class T, class D = DefaultDelete<T> > struct UniquePtr
    {
      typedef T *pointer;
      typedef T element_type;
      typedef D deleter_type;

      explicit UniquePtr(T *p = 0) throw (): ptr(p) {}

      T *get() const throw () { return ptr; }
      T &operator*() const throw () { return *ptr; }
      T *operator->() const throw () { return ptr; }

      void swap(UniquePtr &p) throw () { using std::swap; swap(ptr, p.ptr); }
      void reset(T *p = 0) { UniquePtr(p).swap(*this); }
      T *release() throw () { T *p = ptr; ptr = 0; return p; }

    private:
      typedef T *UniquePtr::*unspecified_bool_type;

    public:
      operator unspecified_bool_type() const throw () { return ptr ? &UniquePtr::ptr : 0; }

      template<class U> UniquePtr(UniquePtr<U,D> &p) throw (): ptr(p.release()) {}
      template<class U> UniquePtr &operator=(UniquePtr<U,D> &p);

      ~UniquePtr() { D()(ptr); }

    private:
      UniquePtr(UniquePtr const &); // Hide
      UniquePtr &operator=(UniquePtr const &); // Hide

      T *ptr;
    };


    template<class T, class D> void swap(UniquePtr<T,D> &p, UniquePtr<T,D> &q) throw ();






    // Template implementations:


    template<class T, class D> template<class U>
    inline UniquePtr<T,D> &UniquePtr<T,D>::operator=(UniquePtr<U,D> &p)
    {
      reset(p.release());
      return *this;
    }


    template<class T, class D>
    inline void swap(UniquePtr<T,D> &p, UniquePtr<T,D> &q) throw ()
    {
      p.swap(q);
    }
  }
}

#endif // ARCHON_CORE_UNIQUE_PTR_HPP
