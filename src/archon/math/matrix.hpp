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
 * This file contains a generic and quite elaborate implementation of
 * mathematical matrices. It utilizes the "expression template"
 * concept to improve performance of matrix expressions in a fashion
 * that is similar to what is done for vectors.
 *
 * \sa BasicVec
 */

#ifndef ARCHON_MATH_MATRIX_HPP
#define ARCHON_MATH_MATRIX_HPP

#include <cmath>
#include <iterator>

#include <archon/math/vector.hpp>


namespace Archon
{
  namespace Math
  {
    template<int M, int N, class T, class R, class E> struct MatVal;
    template<int M, int N, class T> struct BasicMat;



    /**
     * This is the base for any matrix type value regardless of
     * whether it is an immediate matrix or one that is given indirectly
     * by an expression.
     */
    template<int M, int N, class T, class R, class E>
    struct MatBase: VecVal<N*M,T,R,E>
    {
    private:
      typedef VecVal<N*M,T,R,E> B;

    public:
      typedef typename B::size_type      size_type;
      typedef typename B::sub_type       sub_type;
      typedef typename B::const_sub_type const_sub_type;

      /**
       * Get the number of rows in this matrix.
       */
      static size_type rows() { return M; }

      /**
       * Get the number of columns in this matrix.
       */
      static size_type cols() { return N; }

      typedef _VecImpl::Slice<N,1,T,E>              row_type;
      typedef _VecImpl::Slice<M,N,T,E>              col_type;
      typedef _VecImpl::Slice<N,1,T, E const> const const_row_type;
      typedef _VecImpl::Slice<M,N,T, E const> const const_col_type;

      row_type row(size_type i);
      col_type col(size_type j);

      const_row_type row(size_type i) const;
      const_col_type col(size_type j) const;


      /**
       * Read/write access to the specified element.
       *
       * \param i The row index.
       *
       * \param j The column index.
       */
      sub_type sub(size_type i, size_type j);

      /**
       * Retrieve the specified element.
       *
       * \param i The row index.
       *
       * \param j The column index.
       */
      const_sub_type sub(size_type i, size_type j) const;

      sub_type       operator()(size_type i, size_type j);
      const_sub_type operator()(size_type i, size_type j) const;


      /**
       * The length of the main diagonal.
       */
      static int const diag_len = M < N ? M : N;

      typedef _VecImpl::Slice<diag_len, N+1, T, E>                   diag_type;
      typedef _VecImpl::Slice<diag_len, N+1, T, E const> const const_diag_type;

      /**
       * Read/write access to the main diagonal as a vector.
       */
      diag_type diag();

      /**
       * Read access to the main diagonal as a vector.
       */
      const_diag_type diag() const;


      /**
       * Multiplication by an N by N square matrix.
       */
      template<class S, class F> E &operator*=(MatVal<N,N,T,S,F> const &);

      /**
       * Scale each column with the corresponding component of the
       * specified vector.
       */
      template<class S, class F> E &scale(VecVal<N,T,S,F> const &v);


      using B::operator=;
      using B::operator*=;
      using B::sub;

    protected:
      /**
       * Perform LU decomposition with partial pivoting. That is,
       * factorize this matrix, A, into a lower unitriangular matrix,
       * L, an upper triangular matrix, U, and a permutation matrix P,
       * such that A = P * L * U.
       *
       * Obviously, it is only when A is a square matrix, that L and U
       * can both be true triangular matrices. In general, if A is an
       * M x N matrix, then L is an M x min(M,N) matrix, U is a
       * min(M,N) x N matrix, and P is an M x M matrix. Thus, when A
       * is 'high', L will be 'high' too and when A is 'wide', U will
       * be 'wide'. Here is a 'high' example with M = 5 and N = 3:
       *
       * <pre>
       *
       *   [ A11 A12 A13 ]       [  1          ]
       *   [ A21 A22 A23 ]       [ L21  1      ]   [ U11 U12 U13 ]
       *   [ A31 A32 A33 ] = P * [ L31 L32  1  ] * [     U22 U23 ]
       *   [ A41 A42 A43 ]       [ L41 L42 L43 ]   [         U33 ]
       *   [ A51 A52 A53 ]       [ L51 L52 L53 ]
       *
       * </pre>
       *
       * Upon return, this matrix contains both L and U. For a 'high'
       * matrix, this is done by removing the unit diagonal of L and
       * then placing U in the vacated upper triange of L. Accoring to
       * the example above, we get:
       *
       * <pre>
       *
       *       [ U11 U12 U13 ]
       *       [ L21 U22 U23 ]
       *   B = [ L31 L32 U33 ]
       *       [ L41 L42 L43 ]
       *       [ L51 L52 L53 ]
       *
       * </pre>
       *
       * Where B is the value of this matrix upon return. In general
       * we have
       *
       * <pre>
       *
       *             {  U(i,j)   if i <= j
       *   B(i,j) =  {                       for i < M and j < N
       *             {  L(i,j)   otherwise
       *
       * </pre>
       *
       * The permutation matrix is given indirectly as a series of
       * min(M,N) transpositions stored in the specified \c pivots
       * array. This reflects the row transpositions that were caried
       * out in the decomposition process. The permutation matrix, P,
       * can be derived from \a pivots as follows:
       *
       * <pre>
       *
       *                {  k           if i = pivots[k]
       *   perm_k(i) =  {  pivots[k]   if i = k           for i < M
       *                {  i           otherwise
       *
       *               {  1   if perm_k(j) = i
       *   P_k(i,j) =  {                         for i < M and j < M
       *               {  0   otherwise
       *
       *   P = P_0 * P_2 * P_3 * .... * P_(min(M,N)-1)
       *
       * </pre>
       *
       * Where \c perm_k and \c P_k are the permutation function and
       * permutation matrix, respectively, that correspond to the
       * transposition at <tt>pivots[k]</tt>.
       *
       * The algorithm was 'lifted' from
       * http://www.alglib.net/translator/dl/matrixops.general.lu.cpp.zip.
       *
       * \note This method works for both regular and singular, and
       * for both square and non-square matrices.
       */
      void lu_decompose(size_type *pivots);

      MatBase() {}
      MatBase(R const &r): B(r) {}
    };



    template<int M, int N, class T, class R, class E> struct MatVal;
    template<int N, class T, class R, class E> T det(MatVal<N,N,T,R,E> const &);



    /**
     * The only purpose of this class is to allow a specialization to
     * add methods for square matrices.
     */
    template<int M, int N, class T, class R, class E>
    struct MatVal: MatBase<M,N,T,R,E>
    {
      typedef MatBase<M,N,T,R,E> B;
      using B::operator=;

    protected:
      MatVal() {}
      MatVal(R const &r): B(r) {}
    };


    /**
     * This specialization provides methods for square matrices.
     */
    template<int N, class T, class R, class E>
    struct MatVal<N,N,T,R,E>: MatBase<N,N,T,R,E>
    {
      typedef MatBase<N,N,T,R,E>                      B;

      /**
       * A statically allocated identity matrix.
       */
      static BasicMat<N,N,T> const &identity();

      /**
       * Invert this square matrix.
       *
       * If the matrix was singular, the result is undefined.
       *
       * The algorithm was 'lifted' from
       * http://www.alglib.net/translator/dl/matrixops.general.inv.cpp.zip.
       */
      E &inv();

      using B::operator=;

    protected:
      /**
       * Invert an upper or lower triangular matrix. The matrix need
       * not in fact be triangular, since the algorithm will
       * completely ignore the opposite triangle.
       *
       * The algorithm was 'lifted' from
       * http://www.alglib.net/translator/dl/matrixops.general.inv.cpp.zip.
       *
       * \tparam upper Set to true if this is an upper triangular
       * matrix, otherwisw set to false.
       *
       * \tparam unit Set to true if the inversion algorithm should
       * assume that this is a unitriangular matrix (has 1's along the
       * entire main diagonal). The diagonal elements do not in fact
       * be unitary, since the algorithm will completely ignore them
       * in this case.
       *
       * \return True if and only if the triangular matrix was
       * regular. In case it was singular, the matrix will be left in
       * an undefined state.
       */
      template<bool upper, bool unit> bool tri_inv();

      MatVal() {}
      MatVal(R const &r): B(r) {}

    private:
      friend T det<>(MatVal const &);
    };






    template<int M, int N, class T, class R, class E>
    struct MatMem: MatVal<M,N,T,R,E>
    {
      /**
       * Get direct read-only access to the elements of this matrix
       * which are guaranteed to be memory consecutive and in
       * row-major order.
       */
      T *get() { return this->begin(); }

      /**
       * Get direct read/write access to the elements of this matrix
       * which are guaranteed to be memory consecutive and in
       * row-major order.
       */
      T const *get() const { return this->begin(); }

    private:
      typedef MatVal<M,N,T,R,E> B;

    public:
      using B::operator=;

    protected:
      MatMem() {}
      MatMem(R const &r): B(r) {}
    };






    /**
     * This is the base for immediate matrices, that is, matrices that
     * define their elements and element storage directly.
     */
    template<int M, int N, class T, class E>
    struct MatBuf: MatMem<M,N,T, _VecImpl::RepBuf<N*M,T>, E>
    {
      /**
       * A statically allocated matrix whose elements are
       * initialized to zero.
       */
      static E const &zero();

    private:
      typedef MatMem<M,N,T, _VecImpl::RepBuf<N*M,T>, E> B;

    public:
      using B::operator=;
    };






    /**
     * An M x N matrix. It has M rows and N columns.
     *
     * Elements are stored in row major order, thus all the elements
     * of the first row comes before all those in the second
     * row. Also, the first element in memory is the top left
     * one. Note that this coincides with multidimensional C arrays,
     * while for example, OpenGL uses column major order in its
     * matrices.
     */
    template<int M, int N = M, class T = double>
    struct BasicMat: MatBuf<M,N,T, BasicMat<M,N,T> >
    {
      /**
       * The default constructor does not initialize the elements.
       */
      BasicMat() {}

      /**
       * Initialize all elements with the specified value.
       */
      explicit BasicMat(T v) { this->set(v); }

       /**
       * Initialize all elements on the main diagonal with the first
       * specified value, and all the remaining elements with the
       * second.
       */
      BasicMat(T v, T w) { this->set(w); this->diag().set(v); }

     /**
       * Initialize the elements with the corresponding values of the
       * specified array. The specified array must have at least as
       * many elements as this matrix. The elements in the array are
       * assumed to be in row-major order.
       *
       * \tparam U Must be a type that is implicitely convertible to
       * <tt>T</tt>.
       */
      template<class U> explicit BasicMat(U const *a) { this->set(a); }

      /**
       * Initialize the elements of this matrix with the corresponding
       * elements of the specified matrix.
       */
      template<class R, class E> BasicMat(MatVal<M,N,T,R,E> const &e) { this->set(e); }


      using MatBuf<M,N,T, BasicMat<M,N,T> >::operator=;
    };



    typedef BasicMat<2> Mat2;
    typedef BasicMat<3> Mat3;
    typedef BasicMat<4> Mat4;

    typedef BasicMat<2,2> Mat2x2;
    typedef BasicMat<2,3> Mat2x3;
    typedef BasicMat<2,4> Mat2x4;

    typedef BasicMat<3,2> Mat3x2;
    typedef BasicMat<3,3> Mat3x3;
    typedef BasicMat<3,4> Mat3x4;

    typedef BasicMat<4,2> Mat4x2;
    typedef BasicMat<4,3> Mat4x3;
    typedef BasicMat<4,4> Mat4x4;


    typedef BasicMat<2,2, float> Mat2F;
    typedef BasicMat<3,3, float> Mat3F;
    typedef BasicMat<4,4, float> Mat4F;

    typedef BasicMat<2,2, float> Mat2x2F;
    typedef BasicMat<2,3, float> Mat2x3F;
    typedef BasicMat<2,4, float> Mat2x4F;

    typedef BasicMat<3,2, float> Mat3x2F;
    typedef BasicMat<3,3, float> Mat3x3F;
    typedef BasicMat<3,4, float> Mat3x4F;

    typedef BasicMat<4,2, float> Mat4x2F;
    typedef BasicMat<4,3, float> Mat4x3F;
    typedef BasicMat<4,4, float> Mat4x4F;


    typedef BasicMat<2,2, long double> Mat2L;
    typedef BasicMat<3,3, long double> Mat3L;
    typedef BasicMat<4,4, long double> Mat4L;

    typedef BasicMat<2,2, long double> Mat2x2L;
    typedef BasicMat<2,3, long double> Mat2x3L;
    typedef BasicMat<2,4, long double> Mat2x4L;

    typedef BasicMat<3,2, long double> Mat3x2L;
    typedef BasicMat<3,3, long double> Mat3x3L;
    typedef BasicMat<3,4, long double> Mat3x4L;

    typedef BasicMat<4,2, long double> Mat4x2L;
    typedef BasicMat<4,3, long double> Mat4x3L;
    typedef BasicMat<4,4, long double> Mat4x4L;






    namespace _MatImpl
    {
      template<int, int, typename, class> struct Op;
      template<int, typename, class> struct Inv;
      template<int, int, int, typename, class, class> struct Mul;
      template<int, int, typename, class, class> struct MapColVec;
      template<int, int, typename, class, class> struct MapRowVec;
    }






    /**
     * Compute the trace of the specified square matrix. That is, the
     * sum of the elements on the main diagonal.
     */
    template<int N, class T, class R, class E> T tr(MatVal<N,N,T,R,E> const &);


    /**
     * Compute the determinant of the specified square matrix.
     */
    template<int N, class T, class R, class E> T det(MatVal<N,N,T,R,E> const &);


    /**
     * Compute the inverse of the specified square matrix.
     *
     * If the specified matrix is singular, the contents of the
     * resulting matrix is undefined.
     */
    template<int N, class T, class R, class E>
    _MatImpl::Op<N,N,T, _MatImpl::Inv<N,T,E> > const inv(MatVal<N,N,T,R,E> const &);


    /**
     * Multiply an M x N matrix by an N x P matrix yielding an M x P
     * matrix.
     *
     * \note The number of columns in the first matrix must be the
     * same as the number of rows in the second.
     */
    template<int M, int N, int P, class T, class R, class S, class F, class G>
    _MatImpl::Op<M,P,T, _MatImpl::Mul<M,N,P,T,F,G> > const
    operator*(MatVal<M,N,T,R,F> const &, MatVal<N,P,T,S,G> const &);


    /**
     * Multiply an M x N matrix by a column vector of length N
     * yielding a column vector of length M.
     */
    template<int M, int N, class T, class R, class S, class F, class G>
    _VecImpl::Op<M,T, _MatImpl::MapColVec<M,N,T,F,G> > const
    operator*(MatVal<M,N,T,R,F> const &, VecVal<N,T,S,G> const &);


    /**
     * Multiply a row vector of length M by an M x N matrix yielding a
     * row vector of length N.
     */
    template<int M, int N, class T, class R, class S, class F, class G>
    _VecImpl::Op<N,T, _MatImpl::MapRowVec<M,N,T,F,G> > const
    operator*(VecVal<M,T,S,G> const &, MatVal<M,N,T,R,F> const &);


    /**
     * Print the specified matrix onto the specified output stream.
     */
    template<typename C, class S, int M, int N, class T, class R, class E>
    std::basic_ostream<C,S> &operator<<(std::basic_ostream<C,S> &, MatVal<M,N,T,R,E> const &);


    /**
     * Parse characters from the specified input stream as a matrix of
     * the same size as the one specified. If successful, assign the
     * result to the specified matrix.
     */
    template<typename C, class S, int M, int N, class T, class R, class E>
    std::basic_istream<C,S> &operator>>(std::basic_istream<C,S> &, MatVal<M,N,T,R,E> &);









    namespace _MatImpl
    {
      template<int N, class T, class R> struct OpRep: _VecImpl::OpRep<T,R>
      {
      private:
        typedef _VecImpl::OpRep<T,R> B;

      public:
        typedef typename B::size_type size_type;
        T const operator[](size_type i) const
        {
          size_type j = i / N;
          return static_cast<R const &>(*this)(j, i-N*j);
        }
      };

      template<int M, int N, class T, class R>
      struct Op: MatVal<M,N,T,R, Op<M,N,T,R> >
      {
      private:
        typedef Op<M,N,T,R> E;       // Self
        typedef MatVal<M,N,T,R,E> B; // Base

      public:
        Op(R const &r): B(r) {}
      };






      template<int M, int N, class T, class E, bool C> struct Operand
      {
        Operand(E const &e): e(e) {}
        E const &e;
        bool safe(void const *p, bool x) const { return e._safe(p,x); }
      };

      template<int M, int N, class T, class E> struct Operand<M,N,T,E, true>
      {
        Operand(E const &e): e(e) {}
        BasicMat<M,N,T> const e;
        bool safe(void const *, bool) const { return true; }
      };






      template<int N, class T, class E>
      struct Inv: OpRep<N,T, Inv<N,T,E> >
      {
      private:
        typedef OpRep<N,T, Inv<N,T,E> > B;

      public:
        typedef typename B::size_type size_type;
        T const operator()(size_type i, size_type j) const
        {
          return e(i,j);
        }
        Inv(E const &e): e(e) { this->e.inv(); }
        bool safe(void const *, bool) const { return true; }

      private:
        BasicMat<N,N,T> e;
      };


      template<int M, int N, int P, class T, class F, class G>
      struct Mul: OpRep<P,T, Mul<M,N,P,T,F,G> >
      {
      private:
        typedef OpRep<P,T, Mul<M,N,P,T,F,G> > B;

      public:
        typedef typename B::size_type size_type;
        T const operator()(size_type i, size_type j) const
        {
          return dot(this->f.e.row(i), this->g.e.col(j));
        }
        Mul(F const &f, G const &g): f(f), g(g) {}
        bool safe(void const *p, bool) const
        {
          return f.safe(p,true) && g.safe(p,true);
        }

      private:
        Operand<M,N,T,F, !F::_is_lval> f;
        Operand<N,P,T,G, !G::_is_lval> g;
      };


      template<int M, int N, class T, class F, class G>
      struct MapColVec: _VecImpl::OpRep<T, MapColVec<M,N,T,F,G> >
      {
      private:
        typedef _VecImpl::OpRep<T, MapColVec<M,N,T,F,G> > B;

      public:
        typedef typename B::size_type size_type;
        T const operator[](size_type i) const { return dot(f.e.row(i), g.e); }
        MapColVec(F const &f, G const &g): f(f), g(g) {}
        bool safe(void const *p, bool) const
        {
          return f.safe(p,true) && g.safe(p,true);
        }

      private:
        Operand<M,N,T,F, false> f;
        _VecImpl::Operand<N,T,G, !G::_is_lval> g;
      };


      template<int M, int N, class T, class F, class G>
      struct MapRowVec: _VecImpl::OpRep<T, MapRowVec<M,N,T,F,G> >
      {
      private:
        typedef _VecImpl::OpRep<T, MapRowVec<M,N,T,F,G> > B;

      public:
        typedef typename B::size_type size_type;
        T const operator[](size_type i) const { return dot(f.e.col(i), g.e); }
        MapRowVec(F const &f, G const &g): f(f), g(g) {}
        bool safe(void const *p, bool) const
        {
          return f.safe(p,true) && g.safe(p,true);
        }

      private:
        Operand<M,N,T,F, false> f;
        _VecImpl::Operand<M,T,G, !G::_is_lval> g;
      };
    }






    // Template definitions for:  MatBase

    template<int M, int N, class T, class R, class E>
    inline typename MatBase<M,N,T,R,E>::row_type
    MatBase<M,N,T,R,E>::row(size_type i)
    {
      return row_type(static_cast<E &>(*this), N*i);
    }

    template<int M, int N, class T, class R, class E>
    inline typename MatBase<M,N,T,R,E>::col_type
    MatBase<M,N,T,R,E>::col(size_type j)
    {
      return col_type(static_cast<E &>(*this), j);
    }

    template<int M, int N, class T, class R, class E>
    inline typename MatBase<M,N,T,R,E>::const_row_type
    MatBase<M,N,T,R,E>::row(size_type i) const
    {
      return const_row_type(static_cast<E const &>(*this), N*i);
    }

    template<int M, int N, class T, class R, class E>
    inline typename MatBase<M,N,T,R,E>::const_col_type
    MatBase<M,N,T,R,E>::col(size_type j) const
    {
      return const_col_type(static_cast<E const &>(*this), j);
    }

    template<int M, int N, class T, class R, class E>
    inline typename MatBase<M,N,T,R,E>::sub_type
    MatBase<M,N,T,R,E>::sub(size_type i, size_type j)
    {
      return sub(N*i+j);
    }

    template<int M, int N, class T, class R, class E>
    inline typename MatBase<M,N,T,R,E>::const_sub_type
    MatBase<M,N,T,R,E>::sub(size_type i, size_type j) const
    {
      return sub(N*i+j);
    }

    template<int M, int N, class T, class R, class E>
    inline typename MatBase<M,N,T,R,E>::sub_type
    MatBase<M,N,T,R,E>::operator()(size_type i, size_type j)
    {
      return sub(i,j);
    }

    template<int M, int N, class T, class R, class E>
    inline typename MatBase<M,N,T,R,E>::const_sub_type
    MatBase<M,N,T,R,E>::operator()(size_type i, size_type j) const
    {
      return sub(i,j);
    }

    template<int M, int N, class T, class R, class E>
    inline typename MatBase<M,N,T,R,E>::diag_type MatBase<M,N,T,R,E>::diag()
    {
      return diag_type(static_cast<E &>(*this), 0);
    }

    template<int M, int N, class T, class R, class E>
    inline typename MatBase<M,N,T,R,E>::const_diag_type MatBase<M,N,T,R,E>::diag() const
    {
      return const_diag_type(static_cast<E const &>(*this), 0);
    }

    template<int M, int N, class T, class R, class E> template<class S, class F>
    inline E &MatBase<M,N,T,R,E>::operator*=(MatVal<N,N,T,S,F> const &f)
    {
      return *this = static_cast<E const &>(*this) * f;
    }

    template<int M, int N, class T, class R, class E> template<class S, class F>
    E &MatBase<M,N,T,R,E>::scale(VecVal<N,T,S,F> const &v)
    {
      for(int i=0; i<N; ++i) col(i) *= v[i];
      return static_cast<E &>(*this);
    }

    template<int M, int N, class T, class R, class E>
    void MatBase<M,N,T,R,E>::lu_decompose(size_type *pivots)
    {
      if(!M || !N) return;
      size_type n = std::min(M,N);
      for(size_type j=0; j<n; ++j)
      {
        // Find pivot
        size_type p = j;
        T v = std::abs(sub(p,j));
        for(size_type i=j+1; i<M; ++i)
        {
          T w = std::abs(sub(i,j));
          if(v < w) { p = i; v = w; }
        }
        pivots[j] = p;

        if(v)
        {
          // Apply pivot
          if(p != j) std::swap_ranges(row(j).begin(), row(j).end(), row(p).begin());

          // Compute elements J+1:M of J-th column.
          if(j < M-1)
          {
            v = T(1)/sub(j,j);
            for(size_type i=j+1; i<M; ++i) sub(i,j) *= v;
          }
        }

        // Update trailing submatrix.
        if(j < N-1)
        {
          for(size_type i=j+1; i<M; ++i)
          {
            typename row_type::iterator r = row(i).begin();
            std::transform(r+j+1, r+N, row(j).begin()+j+1, r+j+1, SubAlpha<T>(r[j]));
          }
        }
      }
    }





    // Template definitions for:  MatVal

    template<int N, class T, class R, class E>
    BasicMat<N,N,T> const &MatVal<N,N,T,R,E>::identity()
    {
      static BasicMat<N,N,T> const m(1,0);
      return m;
    }

    template<int N, class T, class R, class E>
    inline E &MatVal<N,N,T,R,E>::inv()
    {
      typedef typename E::size_type size_type;

      size_type pivots[N];
      this->lu_decompose(pivots);

      // Invert U
      if (tri_inv<true, false>()) {
        // Solve the equation inv(A)*L = inv(U) for inv(A).
        for (size_type j=N-1; 0<j; --j) {
          typename E::col_type::iterator c = this->col(j-1).begin();
          BasicVec<N,T> t;
          std::copy(c+j, c+N, t.begin());
          for (size_type i=0; i<N; ++i) {
            typename E::row_type::iterator r = this->row(i).begin();
            T v = std::inner_product(r+j, r+N, t.begin(), T());
            if (i<j) r[j-1] -= v;
            else r[j-1] = -v;
          }
        }

        // Apply column transpositions to undo the effect of the
        // pivoting done by the LU-decomposition.
        for (size_type j=N-1; 0<j; --j) {
          size_type p = pivots[j-1];
          if (p == j-1) continue;
          typename E::col_type::iterator c = this->col(j-1).begin();
          std::swap_ranges(c, c+N, this->col(p).begin());
        }
      }
      return static_cast<E &>(*this);
    }

    template<int N, class T, class R, class E>
    template<bool upper, bool unit_diag> bool MatVal<N,N,T,R,E>::tri_inv()
    {
      typedef typename E::size_type size_type;

      if (upper) {
        for (size_type i=0; i<N; ++i) {
          typename E::row_type::reverse_iterator r = this->row(N-(i+1)).rbegin();
          T a;
          if (!unit_diag) {
            a = r[i];
            if (!a) return false;
            r[i] = a = T(1)/a;
            a = -a;
          }
          else a = -1;

          for (size_type j=0; j<i; ++j) {
            typename E::col_type::reverse_iterator c = this->col(N-(j+1)).rbegin();
            T v = r[j];
            if (!unit_diag) v *= c[j];
            if (j < i-1) v += std::inner_product(c+j+1, c+i, r+j+1, T());
            r[j] = a*v;
          }
        }
      }
      else {
        for (size_type i=0; i<N; ++i) {
          typename E::row_type::iterator r = this->row(i).begin();
          T a;
          if (!unit_diag) {
            a = r[i];
            if (!a) return false;
            r[i] = a = T(1)/a;
            a = -a;
          }
          else a = -1;

          for (size_type j=0; j<i; ++j) {
            typename E::col_type::iterator c = this->col(j).begin();
            T v = r[j];
            if (!unit_diag) v *= c[j];
            if (j < i-1) v += std::inner_product(c+j+1, c+i, r+j+1, T());
            r[j] = a*v;
          }
        }
      }

      return true;
    }






    // Template definitions for:  MatBuf

    template<int M, int N, class T, class E>
    inline E const &MatBuf<M,N,T,E>::zero()
    {
      static E const e((T()));
      return e;
    }






    // Template definitions for:  namespace Math


    template<int N, class T, class R, class E> inline T tr(MatVal<N,N,T,R,E> const &e)
    {
      return Math::fold(e.diag(), T(), std::plus<T>());
    }


    template<int N, class T, class R, class E> inline T det(MatVal<N,N,T,R,E> const &e)
    {
      typedef BasicMat<N,N,T> M;
      M f(e);
      typename M::size_type pivots[N];
      f.lu_decompose(pivots);
      int c = 0;
      for(typename M::size_type i=0; i<N; ++i) if(pivots[i] != i) ++c;
      T v = Math::fold(f.diag(), T(1), std::multiplies<T>());
      return c&1 ? -v : v;
    }


    template<int N, class T, class R, class E>
    _MatImpl::Op<N,N,T, _MatImpl::Inv<N,T,E> > const inv(MatVal<N,N,T,R,E> const &e)
    {
      typedef _MatImpl::Inv<N,T,E> S;
      return _MatImpl::Op<N,N,T,S>(S(static_cast<E const &>(e)));
    }


    template<int M, int N, int P, class T, class R, class S, class F, class G>
    inline _MatImpl::Op<M,P,T, _MatImpl::Mul<M,N,P,T,F,G> > const
    operator*(MatVal<M,N,T,R,F> const &f, MatVal<N,P,T,S,G> const &g)
    {
      typedef _MatImpl::Mul<M,N,P,T,F,G> _R;
      return _MatImpl::Op<M,P,T,_R>(_R(static_cast<F const &>(f),
                                       static_cast<G const &>(g)));
    }


    template<int M, int N, class T, class R, class S, class F, class G>
    _VecImpl::Op<M,T, _MatImpl::MapColVec<M,N,T,F,G> > const
    operator*(MatVal<M,N,T,R,F> const &f, VecVal<N,T,S,G> const &g)
    {
      typedef _MatImpl::MapColVec<M,N,T,F,G> _R;
      return _VecImpl::Op<M,T,_R>(_R(static_cast<F const &>(f),
                                     static_cast<G const &>(g)));
    }


    template<int M, int N, class T, class R, class S, class F, class G>
    _VecImpl::Op<N,T, _MatImpl::MapRowVec<M,N,T,F,G> > const
    operator*(VecVal<M,T,S,G> const &g, MatVal<M,N,T,R,F> const &f)
    {
      typedef _MatImpl::MapRowVec<M,N,T,F,G> _R;
      return _VecImpl::Op<N,T,_R>(_R(static_cast<F const &>(f),
                                     static_cast<G const &>(g)));
    }


    template<typename C, class S, int M, int N, class T, class R, class E>
    std::basic_ostream<C,S> &operator<<(std::basic_ostream<C,S> &out, MatVal<M,N,T,R,E> const &m)
    {
      C const left(out.widen('[')), comma(out.widen(',')),
        nl(out.widen('\n')), right(out.widen(']'));
      out << left;
      if(0 < M)
      {
        out << m.row(0);
        for(int i=1; i<M; ++i) out << comma << nl << m.row(i);
      }
      out << right;
      return out;
    }


    template<typename C, class S, int M, int N, class T, class R, class E>
    std::basic_istream<C,S> &operator>>(std::basic_istream<C,S> &in, MatVal<M,N,T,R,E> &m)
    {
      BasicMat<M,N,T> n;
      bool bad = false;
      C const left(in.widen('[')), comma(in.widen(',')), right(in.widen(']'));
      C ch;
      in >> ch;
      if(!in || ch != left) bad = true;
      else
      {
        Core::BasicIosFormatResetter<C,T> sentry(in);
        in.setf(std::ios_base::skipws);
        if(0 < M)
        {
          in >> m.row(0);
          if(!in) bad = true;
          else
          {
            for(int i=1; i<M; ++i)
            {
              in >> ch >> m.row(i);
              if(!in || ch != comma)
              {
                bad = true;
                break;
              }
            }
            if(!bad)
            {
              in >> ch;
              if(!in || ch != right) bad = true;
            }
          }
        }
      }
      if(bad) in.setstate(std::ios_base::badbit);
      else m = n;
      return in;
    }
  }
}

#endif // ARCHON_MATH_MATRIX_HPP
