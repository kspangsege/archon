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
 * This file prevides a few utility iterators.
 */

#ifndef ARCHON_CORE_ITERATOR_HPP
#define ARCHON_CORE_ITERATOR_HPP

#include <cstddef>
#include <iterator>

#include <archon/core/functions.hpp>


namespace archon
{
  namespace Core
  {
    /**
     * An iterator with a custom way of dereferencing the wrapped iterator.
     *
     * \tparam D The custom dereferincing function. The result type of
     * this function sould be <tt>T &</tt>.
     *
     * \tparam T The type of dereferenced values.
     *
     * \tparam I The underlying iterator type.
     */
    template<class D, class T, class I = typename D::argument_type>
    struct CustomDerefIter: std::iterator<typename std::iterator_traits<I>::iterator_category, T,
                                          typename std::iterator_traits<I>::difference_type,
                                          T *, T &>
    {
      typedef typename std::iterator_traits<I>::difference_type difference_type;

      CustomDerefIter(): i(), deref() {}
      explicit CustomDerefIter(I i): i(i), deref() {}
      CustomDerefIter(I i, D const &d): i(i), deref(d) {}

      template<class _D, class _T, class _I>
      CustomDerefIter(CustomDerefIter<_D, _T, _I> const &j): i(j.i), deref(j.deref) {}

      T &operator* ()                  const { return  deref(i);   }
      T *operator->()                  const { return &deref(i);   }
      T &operator[](difference_type j) const { return  deref(i+j); }

      CustomDerefIter &operator++() { ++i; return *this; }
      CustomDerefIter &operator--() { --i; return *this; }
      CustomDerefIter  operator++(int) { return CustomDerefIter(i++); }
      CustomDerefIter  operator--(int) { return CustomDerefIter(i--); }
      CustomDerefIter &operator+=(difference_type j) { i += j; return *this; }
      CustomDerefIter &operator-=(difference_type j) { i -= j; return *this; }
      CustomDerefIter  operator+(difference_type j) const { return CustomDerefIter(i+j); }
      CustomDerefIter  operator-(difference_type j) const { return CustomDerefIter(i-j); }

      friend CustomDerefIter operator+(difference_type i, CustomDerefIter const &j)
      {
        return CustomDerefIter(i+j.i);
      }

      template<class _D, class _T, class _I>
      difference_type operator-(CustomDerefIter<_D, _T, _I> const &j) const { return i - j.i; }

      template<class _D, class _T, class _I>
      bool operator==(CustomDerefIter<_D, _T, _I> const &j) const { return i == j.i; }

      template<class _D, class _T, class _I>
      bool operator!=(CustomDerefIter<_D, _T, _I> const &j) const { return i != j.i; }

      template<class _D, class _T, class _I>
      bool operator< (CustomDerefIter<_D, _T, _I> const &j) const { return i <  j.i; }

      template<class _D, class _T, class _I>
      bool operator> (CustomDerefIter<_D, _T, _I> const &j) const { return i >  j.i; }

      template<class _D, class _T, class _I>
      bool operator<=(CustomDerefIter<_D, _T, _I> const &j) const { return i <= j.i; }

      template<class _D, class _T, class _I>
      bool operator>=(CustomDerefIter<_D, _T, _I> const &j) const { return i >= j.i; }

      I i;
      D deref;
    };


    template<class I, class R = typename std::iterator_traits<I>::reference>
    struct Deref: std::unary_function<I,R>
    {
      R operator()(I i) const { return *i; }
    };

    template<class I,
             class J = typename std::iterator_traits<I>::value_type,
             class R = typename std::iterator_traits<J>::reference>
    struct DoubleDeref: std::unary_function<I,R>
    {
      R operator()(I i) const { return **i; }
    };



    /**
     * A member selection iterator. That is, an iterator which when
     * dereferenced yields a member of the object obtained by
     * dereferecing the underlying iterator.
     *
     * \tparam I The underlying iterator type.
     *
     * \tparam C The class of objects obtained by dereferecing the
     * underlying iterator.
     *
     * \tparam T The type of the selected member of <tt>C</tt>.
     *
     * \tparam M The selected member of <tt>C</tt>.
     *
     * \todo FIXME: This iterator is just a special case of
     * <tt>CustomDerefIter</tt>. Maybe we can get rid of it.
     */
    template<class I, class C, class T, T C::*M>
    struct MembIter:
      std::iterator<typename std::iterator_traits<I>::iterator_category, T,
                    typename std::iterator_traits<I>::difference_type, T *, T &>
    {
      typedef typename std::iterator_traits<I>::difference_type difference_type;

      MembIter(): i() {}
      explicit MembIter(I i): i(i) {}

      template<class _I, class _C, class _T, _T _C::*_M>
      MembIter(MembIter<_I,_C,_T,_M> const &j): i(j.i) {}

      T &operator* ()                  const { return  (*i).*M; }
      T *operator->()                  const { return &(*i).*M; }
      T &operator[](difference_type j) const { return &i[j].*M; }

      MembIter &operator++() { ++i; return *this; }
      MembIter &operator--() { --i; return *this; }
      MembIter  operator++(int) { return MembIter(i++); }
      MembIter  operator--(int) { return MembIter(i--); }
      MembIter &operator+=(difference_type j) { i += j; return *this; }
      MembIter &operator-=(difference_type j) { i -= j; return *this; }
      MembIter  operator+(difference_type j) const { return MembIter(i+j); }
      MembIter  operator-(difference_type j) const { return MembIter(i-j); }

      friend MembIter operator+(difference_type i, MembIter const &j) {return MembIter(i+j.i); }

      template<class _I, class _C, class _T, _T _C::*_M>
      difference_type operator-(MembIter<_I,_C,_T,_M> const &j) const { return i - j.i; }

      template<class _I, class _C, class _T, _T _C::*_M>
      bool operator==(MembIter<_I,_C,_T,_M> const &j) const { return i == j.i; }

      template<class _I, class _C, class _T, _T _C::*_M>
      bool operator!=(MembIter<_I,_C,_T,_M> const &j) const { return i != j.i; }

      template<class _I, class _C, class _T, _T _C::*_M>
      bool operator< (MembIter<_I,_C,_T,_M> const &j) const { return i <  j.i; }

      template<class _I, class _C, class _T, _T _C::*_M>
      bool operator> (MembIter<_I,_C,_T,_M> const &j) const { return i >  j.i; }

      template<class _I, class _C, class _T, _T _C::*_M>
      bool operator<=(MembIter<_I,_C,_T,_M> const &j) const { return i <= j.i; }

      template<class _I, class _C, class _T, _T _C::*_M>
      bool operator>=(MembIter<_I,_C,_T,_M> const &j) const { return i >= j.i; }

      I i;
    };




    /**
     * A forward iterator with a plugable custom increment
     * operation. This is for example usefull for iterating over
     * linked lists.
     */
    template<class I, class Inc>
    struct IncIter:
      std::iterator<std::forward_iterator_tag, typename Inc::value_type, std::ptrdiff_t,
                    typename Inc::value_type *, typename Inc::value_type &>
    {
      typedef typename Inc::value_type &reference;
      typedef typename Inc::value_type *pointer;

      IncIter(): i() {}
      explicit IncIter(I i): i(i) {}

      template<class _I, class _Inc> IncIter(IncIter<_I,_Inc> const &j): i(j.i) {}

      reference operator* () const { return  Inc::deref(i); }
      pointer   operator->() const { return &Inc::deref(i); }

      IncIter &operator++()    { i = Inc::next(i); return *this; }
      IncIter  operator++(int) { IncIter j(*this); ++i; return j; }

      template<class _I, class _Inc>
      bool operator==(IncIter<_I,_Inc> const &j) const { return i == j.i; }

      template<class _I, class _Inc>
      bool operator!=(IncIter<_I,_Inc> const &j) const { return i != j.i; }

      I i;
    };




    /**
     * A subscription iterator.
     */
    template<class O, typename T, typename P = T *, typename R = T &,
             typename D = ssize_t, typename I = size_t>
    struct SubIter:
      std::iterator<std::random_access_iterator_tag, T,D,P,R>
    {
      SubIter(): o(), i() {}
      SubIter(O *o, I i=0): o(o), i(i) {}

      template<class _O, typename _T,
               typename _P, typename _R, typename _D, typename _I>
      SubIter(SubIter<_O,_T,_P,_R,_D,_I> const &j): o(j.o), i(j.i) {}

      R operator* () const { return  (*o)[i]; }
      P operator->() const { return &(*o)[i]; }
      R operator[](D j) const { return (*o)[i+j]; }

      SubIter &operator++() { ++i; return *this; }
      SubIter &operator--() { --i; return *this; }
      SubIter  operator++(int) { return SubIter(o, i++); }
      SubIter  operator--(int) { return SubIter(o, i--); }
      SubIter &operator+=(D j) { i += j; return *this; }
      SubIter &operator-=(D j) { i -= j; return *this; }
      SubIter operator+(D j) const { return SubIter(o, i+j); }
      SubIter operator-(D j) const { return SubIter(o, i-j); }

      friend SubIter operator+(D i, SubIter const &j)
      {
        return SubIter(j.o, j.i+i);
      }

      template<class _O, typename _T,
               typename _P, typename _R, typename _D, typename _I>
      D operator-(SubIter<_O,_T,_P,_R,_D,_I> const &j) const
      {
        return static_cast<D>(i) - static_cast<_D>(j.i);
      }

      template<class _O, typename _T,
               typename _P, typename _R, typename _D, typename _I>
      bool operator==(SubIter<_O,_T,_P,_R,_D,_I> const &j) const
      {
        return i == j.i;
      }

      template<class _O, typename _T,
               typename _P, typename _R, typename _D, typename _I>
      bool operator!=(SubIter<_O,_T,_P,_R,_D,_I> const &j) const
      {
        return i != j.i;
      }

      template<class _O, typename _T,
               typename _P, typename _R, typename _D, typename _I>
      bool operator<(SubIter<_O,_T,_P,_R,_D,_I> const &j) const
      {
        return i < j.i;
      }

      template<class _O, typename _T,
               typename _P, typename _R, typename _D, typename _I>
      bool operator>(SubIter<_O,_T,_P,_R,_D,_I> const &j) const
      {
        return i > j.i;
      }

      template<class _O, typename _T,
               typename _P, typename _R, typename _D, typename _I>
      bool operator<=(SubIter<_O,_T,_P,_R,_D,_I> const &j) const
      {
        return i <= j.i;
      }

      template<class _O, typename _T,
               typename _P, typename _R, typename _D, typename _I>
      bool operator>=(SubIter<_O,_T,_P,_R,_D,_I> const &j) const
      {
        return i >= j.i;
      }

      O *o;
      I i;
    };




    /**
     * A stride iterator. That is, an iterator that picks out every
     * N'th element of the wrapped iterator.
     */
    template<int N, typename I>
    struct StrideIter:
      std::iterator<typename std::iterator_traits<I>::iterator_category,
                    typename std::iterator_traits<I>::value_type,
                    typename std::iterator_traits<I>::difference_type,
                    typename std::iterator_traits<I>::pointer,
                    typename std::iterator_traits<I>::reference>
    {
      typedef I iterator_type;
      typedef typename std::iterator_traits<I>::difference_type difference_type;
      typedef typename std::iterator_traits<I>::reference       reference;
      typedef typename std::iterator_traits<I>::pointer         pointer;

      StrideIter(): i() {}
      explicit StrideIter(I i): i(i) {}

      template<int M, typename J>
      StrideIter(StrideIter<M,J> const &j): i(j.i) {}

      reference operator* () const { return *i; }
      pointer   operator->() const { return  i.operator->(); }
      reference operator[](difference_type j) const { return i[N*j]; }

      StrideIter &operator++() { std::advance(i, N); return *this; }
      StrideIter &operator--() { std::advance(i,-N); return *this; }
      StrideIter  operator++(int) { StrideIter j(i); std::advance(i, N); return j; }
      StrideIter  operator--(int) { StrideIter j(i); std::advance(i,-N); return j; }
      StrideIter &operator+=(difference_type j) { i += N*j; return *this; }
      StrideIter &operator-=(difference_type j) { i -= N*j; return *this; }
      StrideIter operator+(difference_type j) const
      {
        return StrideIter(i + N*j);
      }

      StrideIter operator-(difference_type j) const
      {
        return StrideIter(i - N*j);
      }

      friend StrideIter operator+(difference_type i, StrideIter const &j)
      {
        return StrideIter(N*i + j.i);
      }

      template<int M, typename J>
      difference_type operator-(StrideIter<M,J> const &j) const
      {
        return (i - j.i)/N;
      }

      template<int M, typename J>
      bool operator==(StrideIter<M,J> const &j) const
      {
        return i == j.i;
      }

      template<int M, typename J>
      bool operator!=(StrideIter<M,J> const &j) const
      {
        return i != j.i;
      }

      template<int M, typename J>
      bool operator<(StrideIter<M,J> const &j) const
      {
        return i < j.i;
      }

      template<int M, typename J>
      bool operator>(StrideIter<M,J> const &j) const
      {
        return i > j.i;
      }

      template<int M, typename J>
      bool operator<=(StrideIter<M,J> const &j) const
      {
        return i <= j.i;
      }

      template<int M, typename J>
      bool operator>=(StrideIter<M,J> const &j) const
      {
        return i >= j.i;
      }

      I i;
    };




    template<typename I>
    struct RowIter:
      std::iterator<typename std::iterator_traits<I>::iterator_category, I,
                    typename std::iterator_traits<I>::difference_type, I const *, I const &>
    {
      typedef I iterator_type;
      typedef typename std::iterator_traits<I>::difference_type difference_type;
      typedef I const &reference;
      typedef I const *pointer; // Makes no sense for this kind of iterator

      RowIter(): i(), width(1) {}
      RowIter(I i, difference_type width): i(i), width(width) {}
      template<typename J> RowIter(RowIter<J> const &j): i(j.i), width(j.width) {}

      reference operator* () const { return  i; }
      pointer   operator->() const { return &i; } // Makes no sense for this kind of iterator
      reference operator[](difference_type j) const { return i + width*j; }

      RowIter &operator++() { std::advance(i, width); return *this; }
      RowIter &operator--() { std::advance(i,-width); return *this; }
      RowIter  operator++(int) { RowIter j(*this); std::advance(i, width); return j; }
      RowIter  operator--(int) { RowIter j(*this); std::advance(i,-width); return j; }
      RowIter &operator+=(difference_type j) { i += width*j; return *this; }
      RowIter &operator-=(difference_type j) { i -= width*j; return *this; }
      RowIter operator+(difference_type j) const
      {
        return RowIter(i + width*j, width);
      }

      RowIter operator-(difference_type j) const
      {
        return RowIter(i - width*j, width);
      }

      friend RowIter operator+(difference_type i, RowIter const &j)
      {
        return RowIter(j.width*i + j.i, j.width);
      }

      template<typename J>
      difference_type operator-(RowIter<J> const &j) const
      {
        return (i - j.i)/width;
      }

      template<class J> bool operator==(RowIter<J> const &j) const { return i == j.i; }
      template<class J> bool operator!=(RowIter<J> const &j) const { return i != j.i; }
      template<class J> bool operator< (RowIter<J> const &j) const { return i <  j.i; }
      template<class J> bool operator> (RowIter<J> const &j) const { return i >  j.i; }
      template<class J> bool operator<=(RowIter<J> const &j) const { return i <= j.i; }
      template<class J> bool operator>=(RowIter<J> const &j) const { return i >= j.i; }

      I i;
      int width;
    };




    /**
     * A periodic iterator. That is, an iterator that reproduces the
     * first N elements of the wrapped iterator, then skips the next M
     * elements, and repeating this cycle endlessly.
     */
    template<typename I>
    struct PeriodIter:
      std::iterator<typename std::iterator_traits<I>::iterator_category,
                    typename std::iterator_traits<I>::value_type,
                    typename std::iterator_traits<I>::difference_type,
                    typename std::iterator_traits<I>::pointer,
                    typename std::iterator_traits<I>::reference>
    {
      typedef I iterator_type;
      typedef typename std::iterator_traits<I>::difference_type difference_type;
      typedef typename std::iterator_traits<I>::reference       reference;
      typedef typename std::iterator_traits<I>::pointer         pointer;

      PeriodIter(): i(), use(1), skip(0), count(0) {}

      /**
       * \param count Must be strictly less than <tt>use</tt>.
       */
      explicit PeriodIter(I i, difference_type use, difference_type skip, difference_type count = 0):
        i(i), use(use), skip(skip), count(count) {}

      template<typename J>
      PeriodIter(PeriodIter<J> const &j):
        i(j.i), use(j.use), skip(j.skip), count(j.count) {}

      reference operator* () const { return *i; }
      pointer   operator->() const { return  i.operator->(); }
      reference operator[](difference_type j) const { return *(*this + j); }

      PeriodIter &operator++()
      {
        if(++count == use)
        {
          std::advance(i, 1+skip);
          count = 0;
        }
        else ++i;
        return *this;
      }

      PeriodIter &operator--()
      {
        if(count == 0)
        {
          std::advance(i, -skip-1);
          count = use-1;
        }
        else
        {
          --count;
          --i;
        }
        return *this;
      }

      PeriodIter operator++(int)
      {
        PeriodIter j(*this);
        operator++();
        return j;
      }

      PeriodIter operator--(int)
      {
        PeriodIter j(*this);
        operator--();
        return j;
      }

      PeriodIter &operator+=(difference_type j)
      {
        count += j;
        if(use <= count || count < 0)
        {
          long n;
          count = modulo(count, use, n);
          i += j + n*skip;
        }
        else i += j;
        return *this;
      }

      PeriodIter &operator-=(difference_type j)
      {
        count -= j;
        if(count < 0 || use <= count)
        {
          long n;
          count = modulo(count, use, n);
          i -= j - n*skip;
        }
        else i -= j;
        return *this;
      }

      PeriodIter operator+(difference_type j) const
      {
        return PeriodIter(*this) += j;
      }

      PeriodIter operator-(difference_type j) const
      {
        return PeriodIter(*this) -= j;
      }

      friend PeriodIter operator+(difference_type i, PeriodIter const &j)
      {
        return j + i;
      }

      template<typename J>
      difference_type operator-(PeriodIter<J> const &j) const
      {
        difference_type d = i - j.i;
        // If the division has a remainder then the two iterators have
        // different frequencey or different phase.
        return d - skip * ((d - (count - j.count)) / (use + skip));
      }

      template<typename J>
      bool operator==(PeriodIter<J> const &j) const { return i == j.i; }

      template<typename J>
      bool operator!=(PeriodIter<J> const &j) const { return i != j.i; }

      template<typename J>
      bool operator< (PeriodIter<J> const &j) const { return i <  j.i; }

      template<typename J>
      bool operator> (PeriodIter<J> const &j) const { return i >  j.i; }

      template<typename J>
      bool operator<=(PeriodIter<J> const &j) const { return i <= j.i; }

      template<typename J>
      bool operator>=(PeriodIter<J> const &j) const { return i >= j.i; }

      I i;
      difference_type use, skip, count;
    };
  }
}

#endif // ARCHON_CORE_ITERATOR_HPP
