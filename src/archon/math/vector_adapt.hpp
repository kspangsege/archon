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

#ifndef ARCHON_MATH_VECTOR_ADAPT_HPP
#define ARCHON_MATH_VECTOR_ADAPT_HPP

#include <archon/math/vector.hpp>


namespace Archon
{
  namespace Math
  {
    namespace _Impl
    {
      template<int, class> struct VecAdapt;
      template<int, class> struct VecAdaptConst;
    }


    template<int N, class T> _Impl::VecAdapt<N,T> vec_adapt(T *p);

    template<int N, class T> _Impl::VecAdaptConst<N,T> vec_adapt(T const *p);


    template<class T> inline _Impl::VecAdapt<2,T> vec2_adapt(T *p) { return vec_adapt<2,T>(p); }

    template<class T> inline _Impl::VecAdaptConst<2,T> vec2_adapt(T const *p) { return vec_adapt<2,T>(p); }

    template<class T> inline _Impl::VecAdapt<3,T> vec3_adapt(T *p) { return vec_adapt<3,T>(p); }

    template<class T> inline _Impl::VecAdaptConst<3,T> vec3_adapt(T const *p) { return vec_adapt<3,T>(p); }

    template<class T> inline _Impl::VecAdapt<4,T> vec4_adapt(T *p) { return vec_adapt<4,T>(p); }

    template<class T> inline _Impl::VecAdaptConst<4,T> vec4_adapt(T const *p) { return vec_adapt<4,T>(p); }








    // Implementation:

    namespace _Impl
    {
      template<class T> struct VecAdaptRep
      {
        typedef size_t   size_type;
        typedef T       *iterator;
        typedef T const *const_iterator;
        iterator       iter()       { return p; }
        const_iterator iter() const { return p; }

        static bool const is_lval = true;

        void const *lval_rep(bool &) const { return p; }

        bool safe(void const *q, bool x) const { return q!=p || !x; }

        VecAdaptRep(T *p): p(p) {}

      private:
        T *const p;
      };


      template<int N, class T>
      struct VecAdapt: VecMem<N, T, VecAdaptRep<T>, VecAdapt<N,T> >
      {
      private:
        typedef VecAdaptRep<T> R;
        typedef VecMem<N, T, R, VecAdapt<N,T> > B;

      public:
        VecAdapt(T *p): B(R(p)) {}
        using B::operator=;
      };


      template<int N, class T>
      struct VecAdaptConst: VecMem<N, T, VecAdaptRep<T const>, VecAdaptConst<N,T> >
      {
      private:
        typedef VecAdaptRep<T const> R;
        typedef VecMem<N, T, R, VecAdaptConst<N,T> > B;

      public:
        VecAdaptConst(T const *p): B(R(p)) {}
        using B::operator=;
      };
    }


    template<int N, class T> inline _Impl::VecAdapt<N,T> vec_adapt(T *p)
    {
      return _Impl::VecAdapt<N,T>(p);
    }


    template<int N, class T> inline _Impl::VecAdaptConst<N,T> vec_adapt(T const *p)
    {
      return _Impl::VecAdaptConst<N,T>(p);
    }
  }
}

#endif // ARCHON_MATH_VECTOR_ADAPT_HPP
