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

#ifndef ARCHON_MATH_MATRIX_ADAPT_HPP
#define ARCHON_MATH_MATRIX_ADAPT_HPP

#include <archon/math/vector_adapt.hpp>
#include <archon/math/matrix.hpp>


namespace archon
{
  namespace math
  {
    namespace _Impl
    {
      template<int, int, class> struct MatAdapt;
      template<int, int, class> struct MatAdaptConst;
    }


    /**
     * Elements are stored in row major order, thus all the elements
     * of the first row comes before all those in the second
     * row. Also, the first element in memory is the top left
     * one. Note that this coincides with multidimensional C arrays,
     * while for example, OpenGL uses column major order in its
     * matrices.
     */
    template<int M, int N, class T> _Impl::MatAdapt<M,N,T> mat_adapt(T *p);

    template<int M, int N, class T> _Impl::MatAdaptConst<M,N,T> mat_adapt(T const *p);


    template<class T> inline _Impl::MatAdapt<2,2,T> mat2x2_adapt(T *p) { return mat_adapt<2,2,T>(p); }

    template<class T> inline _Impl::MatAdaptConst<2,2,T> mat2x2_adapt(T const *p) { return mat_adapt<2,2,T>(p); }


    template<class T> inline _Impl::MatAdapt<3,3,T> mat3x3_adapt(T *p) { return mat_adapt<3,3,T>(p); }

    template<class T> inline _Impl::MatAdaptConst<3,3,T> mat3x3_adapt(T const *p) { return mat_adapt<3,3,T>(p); }


    template<class T> inline _Impl::MatAdapt<4,4,T> mat4x4_adapt(T *p) { return mat_adapt<4,4,T>(p); }

    template<class T> inline _Impl::MatAdaptConst<4,4,T> mat4x4_adapt(T const *p) { return mat_adapt<4,4,T>(p); }








    // Implementation:

    namespace _Impl
    {
      template<int M, int N, class T>
      struct MatAdapt: MatMem<M, N, T, VecAdaptRep<T>, MatAdapt<M,N,T> >
      {
      private:
        typedef VecAdaptRep<T> R;
        typedef MatMem<M, N, T, R, MatAdapt<M,N,T> > B;

      public:
        MatAdapt(T *p): B(R(p)) {}
        using B::operator=;
      };


      template<int M, int N, class T>
      struct MatAdaptConst: MatMem<M, N, T, VecAdaptRep<T const>, MatAdaptConst<M,N,T> >
      {
      private:
        typedef VecAdaptRep<T const> R;
        typedef MatMem<M, N, T, R, MatAdaptConst<M,N,T> > B;

      public:
        MatAdaptConst(T const *p): B(R(p)) {}
        using B::operator=;
      };
    }


    template<int M, int N, class T> inline _Impl::MatAdapt<M,N,T> mat_adapt(T *p)
    {
      return _Impl::MatAdapt<M,N,T>(p);
    }


    template<int M, int N, class T> inline _Impl::MatAdaptConst<M,N,T> mat_adapt(T const *p)
    {
      return _Impl::MatAdaptConst<M,N,T>(p);
    }
  }
}

#endif // ARCHON_MATH_MATRIX_ADAPT_HPP
