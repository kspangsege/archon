/**
 * \file
 *
 * \author Kristian Spangsege
 *
 * This file contains a generic and quite elaborate implementation of
 * mathematical vectors. It utilizes the "expression template" concept
 * to improve performance of matrix expressions similar to the case of
 * vectors.
 *
 *
 * \sa BasicVector
 */

/*

Thoughts:

Matrix cannot derive directly from VectorBase since then it would be
possible to add a 2x2 matrixt with a vector of length 4. Also it would
be applicaple to many namespace functions that make no sense on a
vector. It would have to derive private, but that hardly makes sence.

The most immediate problem is how the matrix product can be
imeplemented using expression templates.

No matter what, the columns and rows of the matrix must be viewable as
vectors.



All iteration on matrixes is bi-linear.

Any matrix expression (including L-values) support iteration over its element. The iteration is in row-major order. In all cases the iterator is of "random access" type such that the row/col subscriptions are fast (disputable: a generator type may allow more temporaries to be eliminated.)  For an "operation" type expression the iteration is based on the two-argument subscription operator


New:

It shall be possible to construct a vector like entity basen on a single element iterator of random access type.

Iteration on a 


*/

#ifndef ARCHON_MATH_MATRIX_HPP
#define ARCHON_MATH_MATRIX_HPP

#include <iterator>

#include <archon/math/vector.hpp>

namespace archon
{
  namespace Math
  {
    template<typename I, int S>
    struct StrideIterator: std::iterator<typename std::iterator_traits<I>::iterator_category,
                                         typename std::iterator_traits<I>::value_type,
                                         typename std::iterator_traits<I>::difference_type,
                                         typename std::iterator_traits<I>::pointer,
                                         typename std::iterator_traits<I>::reference>
    {
      typedef I iterator_type;
      typedef typename std::iterator_traits<I>::difference_type difference_type;
      typedef typename std::iterator_traits<I>::reference       reference;
      typedef typename std::iterator_traits<I>::pointer         pointer;
      StrideIterator(): i() {}
      explicit StrideIterator(I i): i(i) {}
      template<class J, int T> StrideIterator(StrideIterator<J,T> const &j): i(j.base()) {}
      I base() const { return i; }
      reference operator* () const { return *i; }
      pointer   operator->() const { return  i.operator->(); }
      reference operator[](difference_type n) const { return i[S*n]; }
      StrideIterator &operator++() { advance(i, S); return *this; }
      StrideIterator &operator--() { advance(i,-S); return *this; }
      StrideIterator  operator++(int) { StrideIterator j(i); advance(i, S); return j; }
      StrideIterator  operator--(int) { StrideIterator j(i); advance(i,-S); return j; }
      StrideIterator &operator+=(difference_type n) { i += S*n; return *this; }
      StrideIterator &operator-=(difference_type n) { i -= S*n; return *this; }
      StrideIterator  operator+ (difference_type n) const { return StrideIterator(i+S*n); }
      StrideIterator  operator- (difference_type n) const { return StrideIterator(i-S*n); }
      I i;
    };


    /// \todo FIXME: Make the subscription operator a template parameter
    template<typename T, typename R, class O>
    struct SubscrIterator: std::iterator<std::random_access_iterator_tag, T, ssize_t, T *, R>
    {
      SubscrIterator(): o(), i() {}
      explicit SubscrIterator(O *o, ssize_t i=0): o(o), i(i) {}
      template<typename _R, class _O>
      SubscrIterator(SubscrIterator<T,_R,_O> const &_i): o(_i.o), i(_i.i) {}
      R  operator* () const { return  (*o)[i]; }
      T *operator->() const { return &(*o)[i]; }
      R  operator[](ssize_t j) const { return (*o)[i+j]; }
      SubscrIterator &operator++() { ++i; return *this; }
      SubscrIterator &operator--() { --i; return *this; }
      SubscrIterator  operator++(int) { SubscrIterator j(*this); ++i; return j; }
      SubscrIterator  operator--(int) { SubscrIterator j(*this); --i; return j; }
      SubscrIterator &operator+=(ssize_t j) { i += j; return *this; }
      SubscrIterator &operator-=(ssize_t j) { i -= j; return *this; }
      SubscrIterator  operator+ (ssize_t j) const { return SubscrIterator(o, i+j); }
      SubscrIterator  operator- (ssize_t j) const { return SubscrIterator(o, i-j); }
      O *o;
      ssize_t i;
    };



    template<typename T, int N, int S, class F, template<typename,int,class> class B_T>
    struct MatrixVector: B_T<T, N, MatrixVector<T,N,S,F,B_T> >
    {
      typedef MatrixVector<T,N,S,F,B_T> E; // This
      typedef B_T<T,N,E> B;                // Base
      typedef typename B::SizeType SizeType;
      typedef StrideIterator<typename F::const_iterator, S> const_iterator;
      typedef StrideIterator<typename F::iterator,       S> iterator;

      const_iterator begin() const { return const_iterator(f.begin()+o); }
      const_iterator end()   const { return begin()+S*N; }

      iterator begin() { return iterator(f.begin()+o); }
      iterator end()   { return begin()+S*N; }

      typename const_iterator::reference sub(SizeType i) const { return *(begin()+i); }
      typename       iterator::reference sub(SizeType i)       { return *(begin()+i); }

      template<typename G> MatrixVector &operator=(VectorExp<T,N,G> const &g)
      {
        return set(g);
      }

      /// \todo FIXEME: Hide the constructor and the data
      MatrixVector(F &f, SizeType o): f(f), o(o) {}
      F &f;       // Matrix
      SizeType o; // Offset
    };



    /**
     * Any final subclass must define the following members:
     *
     * <pre>
     *
     *  static bool isLval
     *  T sub(SizeType i, SizeType j) const
     *
     * </pre>
     */
    template<typename T, int M, int N, class E>
    struct MatrixExp
    {
      typedef size_t SizeType;

      /**
       * Subclasses must override this if they represent L-values.
       */
      static bool const isLval = false;

      /**
       * Retrieve the element at the specified index.
       */
      T operator[](SizeType i) const
      {
        return static_cast<E const *>(this)->sub(i);
      }

      typedef SubscrIterator<T,T,E const> const_iterator;
      typedef const_iterator iterator;

      iterator begin() const
      {
        return iterator(static_cast<E const *>(this), 0);
      }

      iterator end() const
      {
        return iterator(static_cast<E const *>(this), N*M);
      }

      MatrixVector<T,N,1,E const, VectorExp> const row(SizeType i) const
      {
        return MatrixVector<T,N,1,E const, VectorExp>(static_cast<E const &>(*this),
                                                      static_cast<SizeType>(N)*i);
      }
    };




    template<typename T, int M, int N, class E>
    struct MatrixRval: MatrixExp<T,M,N,E>
    {
      typedef MatrixExp<T,M,N,E> B; // Base
      typedef typename B::SizeType SizeType;

      static bool const isLval = false;

      IterVector<T,N, typename E::> row(SizeType i)
    };




    template<typename T, int M, int N, class E>
    struct MatrixLvalOps: MatrixExp<T,M,N,E>
    {
      typedef MatrixExp<T,M,N,E> B; // Base
      typedef typename B::SizeType SizeType;

      static bool const isLval = true;
    };



    template<typename T, int M, int N, class E>
    struct MatrixLval: MatrixLvalOps<T,M,N,E>
    {
      typedef MatrixLvalOps<T,M,N,E> B; // Base
      typedef typename B::SizeType SizeType;
  
      /**
       * Present a row of this matrix as a vector. The row is a vector
       * L-value, i.e. a derivative of \c VectorLval.
       *
       * \param i The row index.
       *
       * \todo FIXME: Provide for \c row on constant matrix.
       */
      MatrixVector<T,N,1,E,VectorLval> row(SizeType i)
      {
        return MatrixVector<T,N,1,E,VectorLval>(static_cast<E &>(*this),
                                                static_cast<SizeType>(N)*i);
      }

      /**
       * Present a column of this matrix as a vector. The column is a
       * vector L-value, i.e. a derivative of \c VectorLval.
       *
       * \param i The column index.
       *
       * \todo FIXME: Provide for \c col on constant matrix.
       */
      MatrixVector<T,M,N,E,VectorLval> col(SizeType i)
      {
        return MatrixVector<T,M,N,E,VectorLval>(static_cast<E &>(*this), i);
      }
    };
    

    template<typename T, int M, int N, class E>
    struct MatrixBase: MatrixLval<T,M,N,E>
    {
      typedef MatrixLval<T,M,N,E> B; // Base
      typedef typename B::SizeType SizeType;

      typedef T const *const_iterator;
      typedef T       *iterator;

      const_iterator begin() const { return v;     }
      const_iterator end()   const { return v+N*M; }
      iterator       begin()       { return v;     }
      iterator       end()         { return v+N*M; }

      T  sub(SizeType i) const { return v[i]; }
      T &sub(SizeType i)       { return v[i]; }

    private:
      T v[N*M];
    };
    

    /**
     * An M x N matrix. It has M rows and N columns.
     *
     * Elements are stored in row major order, thus all the elements
     * of the first row comes before all those in the second
     * row. Also, the first element in memory is the top left
     * one. Note that this coincides with multidimensional C arrays.
     */
    template<typename T, int M, int N>
    struct BasicMatrix: MatrixBase<T, M, N, BasicMatrix<T,M,N> >
    {
    };



    typedef BasicMatrix<double, 2, 2> Matrix2x2;
    typedef BasicMatrix<double, 3, 3> Matrix3x3;
    typedef BasicMatrix<double, 4, 4> Matrix4x4;

    typedef Matrix2x2 Matrix2;
    typedef Matrix3x3 Matrix3;
    typedef Matrix4x4 Matrix4;




    /**
     * The standard operand for unary and binary operators. It is a
     * simple wrapper around a reference to the matrix expression.
     *
     * If the boolean template argument is true, it indicates that the
     * operand must cache the result of the expression, supposedly
     * because the involved operator needs to read the elements
     * multiple times. The caching is achieved through
     * specialization. Other specializations may exist but all must
     * define a field named \c e which must contain the wrapped
     * expression.
     */
    template<typename T, int M, int N, class E, bool> struct MatrixOperand
    {
      MatrixOperand(E const &e): e(e) {}
      E const &e;
    };

    /**
     * An operand specialization that caches the result of the wrapped
     * expression.
     */
    template<typename T, int M, int N, class E>
    struct MatrixOperand<T, M, N, E, true>
    {
      MatrixOperand(E const &e): e(e) {}
      BasicMatrix<T,M,N> e;
    };




    template<typename T, int M, int N, class E, class F, class G, bool C>
    struct MatrixBinOp: MatrixExp<T,M,N,E>
    {
    protected:
      typedef MatrixExp<T,M,N,E> B; // Base
      typedef typename B::SizeType SizeType;
      MatrixBinOp(F const &f, G const &g): f(f), g(g) {}
      MatrixOperand<T, M, N, F, C && !F::isLval> f;
      MatrixOperand<T, M, N, G, C && !G::isLval> g;
    };


    /**
     * Binary multiplication operator for matrix expressions.
     */
    template<typename T, int M, int N, class F, class G>
    struct MatrixMul: MatrixBinOp<T, M, N, MatrixMul<T,M,N,F,G>, F, G, true>
    {
      typedef MatrixMul<T,M,N,F,G> E;          // This
      typedef MatrixBinOp<T,M,N,E,F,G,true> B; // Base
      typedef typename B::SizeType SizeType;
      T sub(SizeType i, SizeType j) const
      {
        return dot(this->f.e.row(i), this->g.e.col(j));
      }
      MatrixMul(F const &f, G const &g): B(f,g) {}
    };


    /**
     * The number of columns of the first matrix must be the same as
     * the number of rows of the second.
     */
    template<typename T, int M, int N, int P, class F, class G>
    MatrixMul<T,M,P,F,G> operator*(MatrixExp<T,M,N,F> const &f,
                                   MatrixExp<T,N,P,G> const &g)
    {
      return MatrixMul<T,M,P,F,G>(static_cast<F const &>(f),
                                  static_cast<G const &>(g));
    }

    template<typename T, int M, int N, class E>
    std::ostream &operator<<(std::ostream &o, MatrixExp<T,M,N,E> const &e)
    {
      o << "[";
      if(0<M)
      {
        o << e.row(0);
        for(typename E::SizeType i=1; i<M; ++i) o << "," << std::endl << e.row(i);
      }
      o << "]";
      return o;
    }

    template<typename T, int M, int N, class E>
    std::istream &operator>>(std::istream &i, MatrixLval<T,M,N,E> &e)
    {
      BasicMatrix<T,M,N> f;
      bool b = false;
      char c;
      i >> c;
      if(!i || c != '[') b = true;
      else
      {
        if(M)
        {
          i >> std::ws >> f.row(0);
          if(!i) b = true;
          else
          {
            for(typename E::SizeType j=1; j<M; ++j)
            {
              i >> std::ws >> c >> std::ws >> f.row(j);
              if(!i || c != ',')
              {
                b = true;
                break;
              }
            }
            if(!b)
            {
              i >> std::ws >> c;
              if(!i || c != ']') b = true;
            }
          }
        }
      }
      if(b) i.clear(std::ios_base::badbit);
      else e.set(f);
      return i;
    }
  }
}

#endif // ARCHON_MATH_MATRIX_HPP
