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
 * mathematical vectors. It utilizes the concept of "expression
 * templates" to improve performance of vector expressions.
 *
 * The basic function of expression templates is to build up a
 * composite type at compile-time that models some naturally written
 * expression. Because this is done at compile-time, the expression can
 * afterwards be applied to varying types, in varying contexts, and
 * multiple times, and due to inlining the generated code can be
 * optimal for each case.
 *
 * In this context the idea is to evaluate vector expressions one
 * element at a time. This way a lot of intermediate memory storing
 * is avoided - especially for bigger vectors.
 *
 * This works well for addition and subtraction, but less well for
 * vector cross product, for example, since here each of the
 * components of the operands are needed in the computation of
 * multiple elements of the result, and the naive evaluation scheme
 * would unnecessarily repeat the calculation of the operand elements
 * many times over. So the product, and similar operations, are
 * handled by a hybrid solution that does store complete intermediate
 * matrices in some cases.
 *
 * The solution employed in the below implementation, is to introduce
 * an operand wrapper for all operations. The wrapper will then deduce
 * from the type of its wrapped expression and the operator whether or
 * not the intermediate result needs to be cached.
 *
 * Special care is required to deal properly with "memory aliasing" in
 * an assignment when the target vector also occurs on the right side
 * and evaluation of an element position depends on other element
 * positions in the vector (an example is the cross product.) Since
 * the first element is both evaluated and assigned before the second
 * element is evaluated; if the evaluation of the second element
 * depends on the first element of the target vector, that first
 * element will have been clobbered. This condition, however, is
 * automatically detected and resolved by introducing an intermediate
 * vector for the result.
 *
 *
 * From the point of view of the application, a vector variable
 * (L-value) is an instance of <tt>BasicVec</tt>. Such vectors are
 * mutable (unless declared const.)
 *
 * A function that takes a 3-D vector as argument should generally
 * look like this:
 *
 * <pre>
 *
 *   template<class R, class I> void func(VecVal<3, double, R, I> const &);
 *
 * </pre>
 *
 * This allows calls such as <tt>func(v1 + v2)</tt>.
 *
 * If your function needs to do vector arithmetic, you should just
 * work on the argument, but if you need the actual component values,
 * it is best to do something like this:
 *
 * <pre>
 *
 *   template<class R, class I> void func(VecVal<3, double, R, I> const &);
 *   {
 *     BasicVec<3, double> w(v);
 *     // Do something whith w
 *   }
 *
 * </pre>
 *
 *
 * The following shows the inheritance hierarchy of a BasicVec:
 *
 * <pre>
 *
 *   BasicVec <- VecBuf <- VecVal <- VecLenSpec
 *
 * </pre>
 *
 * The purpose of \c VecBase is only to contain the representation of
 * the vector which is passed to it using a template argument.
 *
 * The purpose of \c VecLenSpec is to allow for specialization on
 * various vector lengths, such that methods can be added that make
 * sense only for specific lengths.
 *
 * All VecBase and VecLenSpec instances must also be VecVal
 * instances. This ensures that all \c VecVal methods are available to
 * any \c VecLenSpec method. It is enforced by explicit friendship
 * declaration.
 *
 * \c VecBuf is the base of a vector which has an immediate
 * representation, and therefore is a potentially mutable vector
 * variable (L-value). Its only function is to plug the appropriate
 * representation class into the right template argument.
 */

#ifndef ARCHON_MATH_VECTOR_HPP
#define ARCHON_MATH_VECTOR_HPP

#include <cmath>
#include <limits>
#include <iterator>
#include <functional>
#include <algorithm>
#include <numeric>
#include <ios>

#include <archon/core/functions.hpp>
#include <archon/core/iterator.hpp>
#include <archon/core/series.hpp>
#include <archon/math/functions.hpp>
#include <archon/math/vec_ops.hpp>


namespace archon
{
  namespace Math
  {
    namespace _VecImpl
    {
      template<int, int, class, class> struct Slice;
    }

    template<int N, class T, class Rep, class Inst> struct VecVal;
    template<int N, class T, class Rep, class Inst> struct VecLenSpec;



    /**
     * Every vector value (R or L) must be of a type that derives from
     * this class. This class defines all public methods (and
     * operators) that are available on any vector value. Derived
     * classes must make sure to explicitly unhide the assignment
     * operator available in this class. By default the assignment
     * operator gets hidden in derived classes.
     *
     * Any deriving class must pass the type of the ultimate instance
     * as the template argument <tt>E</tt>. The only reason this type
     * must be known, is that some operators (assignment for example)
     * and methods wants to return a reference to the affected object.
     *
     * <h1>Representation</h1>
     *
     * The representation of the vector is defined by a separate
     * class, This call must be passed as the template argument,
     * <tt>R</tt>. It must define the type \c size_type as the type
     * used for element subscription and the iterator types \c
     * iterator and \c cont_iterator which must be random access
     * iterators. It must also define a constant static boolean member
     * named \c is_lval that indicates whether or not the represented
     * vector value is an L-value. Further more, it must define all
     * of the following methods:
     *
     * <pre>
     *
     *   iterator       iter()
     *   const_iterator iter() const
     *   void const *lval_rep(bool &) const
     *   bool safe(void const *p, bool x) const
     *
     * </pre>
     *
     * The \c iter methods must return an iterator to the first
     * element of the represented vector value. For a L-value
     * expression, the \c lval_rep method must return the pointer to
     * the directly or indirectly wrapped representation object that
     * directly defines the storage for the elements, and it must also
     * set the boolean argument to true if the expression involves an
     * operation that causes element positions to not be independent
     * (for example, a slice operation with a non-zero offset.) The \c
     * safe method is called by the \c _safe method of this class, and
     * must function equivalently to it.
     *
     * \tparam N The number of elements in this vector.
     *
     * \tparam T The type of the elements of this vector.
     *
     * \tparam Rep The class that defines the representation of this
     * vector.
     *
     * \tparam Inst The ultimate instance type of this vector.
     */
    template<int N, class T, class Rep, class Inst> struct VecBase
    {
      /**
       * The type used for vector size and element subscription.
       */
      typedef typename Rep::size_type size_type;

      /**
       * Get the number of elements in this vector.
       */
      static size_type size() { return N; }

      /**
       * Check whether this vector is the zero-vector.
       *
       * \return True if, and only if all elements are zero.
       */
      bool is_zero() const;

      typedef typename Rep::iterator       iterator;
      typedef typename Rep::const_iterator const_iterator;

      iterator       begin()       { return r.iter();    }
      iterator       end()         { return begin() + N; }
      const_iterator begin() const { return r.iter();    }
      const_iterator end()   const { return begin() + N; }

      typedef std::reverse_iterator<iterator>       reverse_iterator;
      typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

      reverse_iterator       rbegin();
      reverse_iterator       rend();
      const_reverse_iterator rbegin() const;
      const_reverse_iterator rend()   const;

      typedef typename
      std::iterator_traits<iterator>::reference sub_type;
      typedef typename
      std::iterator_traits<const_iterator>::reference const_sub_type;

      /**
       * Read/write access to the element at the specified index.
       */
      sub_type sub(size_type i) { return *(begin()+i); }

      /**
       * Retrieve the element at the specified index.
       */
      const_sub_type sub(size_type i) const { return *(begin()+i); }

      sub_type       operator[](size_type i)       { return sub(i); }
      const_sub_type operator[](size_type i) const { return sub(i); }

      /**
       * Set all the elements of this vector equal to the specified
       * value.
       *
       * \return A reference to this vector.
       */
      Inst &set(T);

      /**
       * Set the elements of this vector to the corresponding values
       * of the specified array. The specified array must have at
       * least as many elements as this vector.
       *
       * \tparam U Must be a type that is implicitely convertible to
       * <tt>T</tt>.
       *
       * \return A reference to this vector.
       */
      template<class U> Inst &set(U const *);

      /**
       * Set this vector equal to the specified one.
       *
       * \return A reference to this vector.
       */
      template<class Rep2, class Inst2>
      Inst &set(VecBase<N, T, Rep2, Inst2> const &);

      /**
       * Set this vector equal to the specified one.
       */
      Inst &operator=(VecBase const &);

      /**
       * Set this vector equal to the specified one.
       */
      template<class Rep2, class Inst2>
      Inst &operator=(VecVal<N, T, Rep2, Inst2> const &);

      template<class S, class F> void swap(VecBase<N,T,S,F> &f);

      /**
       * Update this vector by applying the specified operation to
       * each element individually.
       *
       * \param o The unary function object whose application
       * operator is used to map the individual elements. The
       * application operator should have a signature like <tt>T
       * operator()(T)</tt>.
       *
       * \return A reference to this vector.
       */
      template<class Op> Inst &map(Op o);

      /**
       * Update this vector by applying the specified binary
       * operation to corresponding elements in this and the
       * specified vector.
       *
       * \param o The binary function object whose application
       * operator is used to map the individual elements. The
       * application operator should have a signature like <tt>T
       * operator()(T,T)</tt>.
       *
       * \return A reference to this vector.
       */
      template<class Rep2, class Inst2, class Op>
      Inst &map(VecVal<N, T, Rep2, Inst2> const &, Op o);

      /**
       * Update this vector by scaling it by the specified factor.
       */
      Inst &operator*=(T);

      /**
       * Update this vector by scaling it by the inverse of the
       * specified factor.
       */
      Inst &operator/=(T);

      /**
       * Update this vector by adding the specified vector to it.
       */
      template<class Rep2, class Inst2>
      Inst &operator+=(VecVal<N, T, Rep2, Inst2> const &);

      /**
       * Update this vector by subtracting the specified vector from
       * it.
       */
      template<class Rep2, class Inst2>
      Inst &operator-=(VecVal<N, T, Rep2, Inst2> const &);

      /**
       * Update this vector by negating each element in it.
       *
       * \return A reference to this vector.
       */
      Inst &neg() { return map(std::negate<T>()); }

      /**
       * Update this vector by normalizing it such that it becomes a
       * vector of unit length.
       *
       * \return A reference to this vector.
       */
      Inst &unit() { return *this /= len(static_cast<Inst &>(*this)); }

      /**
       * Update this vector by projecting it onto the specified
       * vector.
       *
       * \return A reference to this vector.
       */
      template<class Rep2, class Inst2>
      Inst &proj(VecVal<N, T, Rep2, Inst2> const &);


      /**
       * Isolate a slice of this vector. While being a proxy for the
       * specified elements, the slice is itself a fully qualified
       * vector. In this case the slice gives read and write access
       * to the original vector.
       *
       * \tparam M the number of adjacent elements of this vector to
       * be included in the slice.
       *
       * \param i The index within this vector of the first element to
       * be included in the slice.
       */
      template<int M>
      _VecImpl::Slice<M, 1, T, Inst> slice(size_type i = 0);

      /**
       * Isolate a slice of this vector. While being a proxy for the
       * specified elements, the slice is itself a fully qualified
       * vector. In this case the slice gives read-only access to
       * the original vector.
       *
       * \tparam M the number of adjacent elements of this vector to
       * be included in the slice.
       *
       * \param i The index within this vector of the first element to
       * be included in the slice.
       */
      template<int M>
      _VecImpl::Slice<M, 1, T, Inst const> const slice(size_type i = 0) const;

      operator Core::Series<N,T>() const
      {
        Core::Series<N,T> s;
        std::copy(begin(), end(), s.get());
        return s;
      }

      /**
       * Indicates whether this class represents an L-value, i.e. a
       * value whose elements are directly addressable. This
       * contrasts an implicit vector value, such as the result of
       * an expression.
       */
      static bool const _is_lval = Rep::is_lval;

      void const *_lval_rep(bool &x) const { return r.lval_rep(x); }

      /**
       * Check whether the vector valued sub-expression represented by
       * this 'node' can be evaluated and assigned safely to the
       * specified vector representation one element position at a
       * time. This is always the case if the expression does not
       * involve the target vector, or if it only involves the target
       * vector as a part of a cached operand. An operation with an
       * operand that involves an uncached reference to the target
       * vector is safe only if the evaluation of its elements have no
       * inter position dependencies. An example of an unsafe
       * expression is a cross product with the target vector as a
       * direct operand.
       *
       * Evaluation of this method involves a recursion through the
       * nodes of the expression. At the leafs the result is \c
       * false if and only if the leaf is the target vector and the
       * surrounding expression involves it in an uncached and
       * element position dependant way. Of course, the leaf can
       * only decide this if the expression nodes pass the
       * dependency information to it, and that is the purpose of
       * the second argument <tt>x</tt>. When called recursively,
       * true must be passed for \c x if the node is an element
       * position dependant operation or if it was itself called
       * with <tt>x = true</tt>.
       *
       * \param p A pointer to the representation object of the
       * target vector of an assigment operation.
       *
       * \param x True if the surrounding expression causes element
       * positions of this sub-expression to not be element position
       * independent.
       */
      bool _safe(void const *p, bool x) const { return r.safe(p,x); }

    private:
      friend struct VecLenSpec<N, T, Rep, Inst>;

      VecBase() {}
      VecBase(Rep const &r): r(r) {}

      template<class Rep2, class Inst2>
      bool safe_assign(VecBase<N, T, Rep2, Inst2> const &) const;

      Rep r;
    };






    /**
     * This class exists only to allow specializations on specific
     * lengths of the vector, such that it can add methods that only
     * make sence for those lengths. For unspecialized lengths, this
     * class adds nothing to the vector.
     *
     * The template arguments are the same as those of
     * <tt>VecVal</tt>.
     */
    template<int N, class T, class Rep, class Inst>
    struct VecLenSpec: VecBase<N, T, Rep, Inst>
    {
    private:
      typedef VecBase<N, T, Rep, Inst> B;

    public:
      using B::operator=;

    private:
      friend struct VecVal<N, T, Rep, Inst>;

      VecLenSpec() {}
      VecLenSpec(Rep const &r): B(r) {}
    };


    /**
     * Specialization providing extra methods for 2-D vectors.
     */
    template<class T, class Rep, class Inst>
    struct VecLenSpec<2, T, Rep, Inst>: VecBase<2, T, Rep, Inst>
    {
    private:
      typedef VecBase<2, T, Rep, Inst> B;

    public:
      /**
       * Assign the specified values to the respective elements of
       * this 2-D vector.
       */
      Inst &set(T,T);

      /**
       * Turn this 2-D vector 90 degrees in the counterclockwise
       * direction.
       */
      Inst &perp();

      using B::operator=;
      using B::set;

    private:
      friend struct VecVal<2, T, Rep, Inst>;

      VecLenSpec() {}
      VecLenSpec(Rep const &r): B(r) {}
    };


    /**
     * Specialization providing extra methods for 3-D vectors.
     */
    template<class T, class Rep, class Inst>
    struct VecLenSpec<3, T, Rep, Inst>: VecBase<3, T, Rep, Inst>
    {
    private:
      typedef VecBase<3, T, Rep, Inst> B;

    public:
      /**
       * Assign the specified values to the respective elements of
       * this 3-D vector.
       */
      Inst &set(T,T,T);

      /**
       * Set this 3-D vector to be the cross product of itself and the
       * specified 3-D vector.
       */
      template<class Rep2, class Inst2> Inst &operator*=(VecVal<3, T, Rep2, Inst2> const &);

      using B::operator=;
      using B::set;
      using B::operator*=;

    private:
      friend struct VecVal<3, T, Rep, Inst>;

      VecLenSpec() {}
      VecLenSpec(Rep const &r): B(r) {}
    };


    /**
     * Specialization providing extra methods for 4-D vectors.
     */
    template<class T, class Rep, class Inst>
    struct VecLenSpec<4, T, Rep, Inst>: VecBase<4, T, Rep, Inst>
    {
    private:
      typedef VecBase<4, T, Rep, Inst> B;

    public:
      /**
       * Assign the specified values to the respective elements of
       * this 4-D vector.
       */
      Inst &set(T,T,T,T);

      using B::operator=;
      using B::set;

    private:
      friend struct VecVal<4, T, Rep, Inst>;

      VecLenSpec() {}
      VecLenSpec(Rep const &r): B(r) {}
    };




    template<int N, class T, class Rep, class Inst> struct VecVal: VecLenSpec<N, T, Rep, Inst>
    {
    private:
      typedef VecLenSpec<N, T, Rep, Inst> B;

    public:
      using B::operator=;

    protected:
      VecVal() {}
      VecVal(Rep const &r): B(r) {}
    };



    template<int N, class T, class Rep, class Inst>
    struct VecMem: VecVal<N, T, Rep, Inst>
    {
      /**
       * Get direct read/write access to the elements of this vector
       * which are guaranteed to be memory consecutive and in the
       * natural order.
       */
      T *get() { return this->begin(); }

      /**
       * Get direct read-only access to the elements of this vector
       * which are guaranteed to be memory consecutive and in the
       * natural order.
       */
      T const *get() const { return this->begin(); }

    private:
      typedef VecVal<N, T, Rep, Inst> B;

    public:
      using B::operator=;

    protected:
      VecMem() {}
      VecMem(Rep const &r): B(r) {}
    };



    namespace _VecImpl
    {
      /**
       * This class defines the representation of genuine vectors. It
       * is a simple wrapper around a fundamental array.
       *
       * \tparam N The number of elements.
       *
       * \tparam T The element type.
       */
      template<int N, class T> struct RepBuf
      {
        typedef size_t   size_type;
        typedef T       *iterator;
        typedef T const *const_iterator;
        iterator       iter()       { return a; }
        const_iterator iter() const { return a; }

        static bool const is_lval = true;

        void const *lval_rep(bool &) const { return a; }

        bool safe(void const *p, bool x) const { return p!=a || !x; }

      private:
        T a[N];
      };
    }



    /**
     * This class is the base of all genuine vectors. That is, vectors
     * which contain their elements directly. The storage of the
     * elements is guaranteed to be memory consecutive and in the
     * natural order.
     *
     * The template arguments are the same as those of \c VecBase.
     */
    template<int N, class T, class Inst>
    struct VecBuf: VecMem<N, T, _VecImpl::RepBuf<N,T>, Inst>
    {
      /**
       * A statically allocated vector whose elements are
       * initialized to zero.
       */
      static Inst const &zero();

    private:
      typedef VecMem<N, T, _VecImpl::RepBuf<N,T>, Inst> B;

    public:
      using B::operator=;
    };




    namespace _VecImpl
    {
      /**
       * This class is a minimalistic version of Math::BasicVec
       * allowing the definition of the latter to be postponed.
       */
      template<int N, class T>
      struct Cache: VecBuf<N, T, Cache<N,T> >
      {
        template<class Rep, class Inst> Cache(VecBase<N, T, Rep, Inst> const &e)
        {
          this->set(e);
        }

        using VecBuf<N, T, Cache<N,T> >::operator=;
      };



      /**
       * The standard operand for unary and binary operators. It is a
       * simple wrapper around a reference to the vector expression.
       *
       * \tparam N The number of element in the wrapped vector
       * expression.
       *
       * \tparam T The type of the vector elements.
       *
       * \tparam E The ultimate instance type of the wrapped vector
       * expression.
       *
       * \tparam C If this boolean template argument is true, it
       * indicates that the result of the sub expression must be
       * cached in the operand, supposedly because the involved
       * operator needs to read the elements multiple times. The
       * caching is achieved through specialization. Other
       * specializations may exist but all must define a field named
       * \c e which must contain the wrapped expression.
       */
      template<int N, class T, class E, bool C> struct Operand
      {
        Operand(E const &e): e(e) {}
        E const &e;
        bool safe(void const *p, bool x) const { return e._safe(p,x); }
      };

      /**
       * An operand specialization that caches the result of the
       * wrapped sub-expression.
       */
      template<int N, class T, class E> struct Operand<N,T,E, true>
      {
        Operand(E const &e): e(e) {}
        Cache<N,T> const e;
        bool safe(void const *, bool) const { return true; }
      };



      /**
       * This class is the base for all unary and binary vector
       * operation representations.
       *
       * \tparam T The type of the vector elements.
       *
       * \tparam R The ultimate representation type.
       */
      template<class T, class R> struct OpRep
      {
        typedef size_t size_type;

        typedef Core::SubIter<R const, T, void, T,
                              ssize_t, size_t> const_iterator;

        // There is no non-const iterator in this case
        typedef const_iterator iterator;

        const_iterator iter() const
        {
          return const_iterator(static_cast<R const *>(this), 0);
        }

        static bool const is_lval = false;

        void const *lval_rep(bool &) const { return 0; }
      };



      /**
       * This is the ultimate instance type of all unary and binary
       * vector operations.
       *
       * \tparam N The number of elements in the result of this vector
       * expression.
       *
       * \tparam T The type of the vector elements.
       *
       * \tparam R The class that defines the representation of the
       * operation.
       */
      template<int N, class T, class R>
      struct Op: VecVal<N, T, R, Op<N,T,R> >
      {
      private:
        typedef Op<N,T,R> E;     // Self
        typedef VecVal<N,T,R,E> B; // Base

      public:
        Op(R const &r): B(r) {}
      };



      /**
       * The representation of element-wise unary operations, that is,
       * operations where the elements are computed independently of
       * each other.
       *
       * \tparam T The type of the vector elements.
       *
       * \tparam F The ultimate instance type of the sub-expression
       * functioning as operand.
       *
       * \tparam O The type of the unary function object that defines
       * the per-element operation.
       */
      template<class T, class F, class O>
      struct UnMap: OpRep<T, UnMap<T,F,O> >
      {
      private:
        typedef OpRep<T, UnMap<T,F,O> > B;

      public:
        typedef typename B::size_type size_type;
        T const operator[](size_type i) const { return o(f[i]); }
        UnMap(F const &f, O o): f(f), o(o) {}
        bool safe(void const *p, bool x) const { return f._safe(p,x); }

      private:
        F const &f;
        O o;
      };



      /**
       * The representation of element-wise binary operations, that
       * is, operations where the elements are computed independently
       * of each other.
       *
       * \tparam T The type of the vector elements.
       *
       * \tparam F The ultimate instance type of the sub-expression
       * functioning as left operand.
       *
       * \tparam G The ultimate instance type of the sub-expression
       * functioning as right operand.
       *
       * \tparam O The type of the binary function object that defines
       * the per-element operation.
       */
      template<class T, class F, class G, class O>
      struct BinMap: OpRep<T, BinMap<T,F,G,O> >
      {
      private:
        typedef OpRep<T, BinMap<T,F,G,O> > B;

      public:
        typedef typename B::size_type size_type;
        T const operator[](size_type i) const { return o(f[i], g[i]); }
        BinMap(F const &f, G const &g, O o): f(f), g(g), o(o) {}
        bool safe(void const *p, bool x) const
        {
          return f._safe(p,x) && g._safe(p,x);
        }

      private:
        F const &f;
        G const &g;
        O o;
      };



      /**
       * The representation of the vector normalization operation.
       *
       * \tparam N the number of elements in each of the operands.
       *
       * \tparam T The type of the vector elements.
       *
       * \tparam F The ultimate instance type of the sub-expression
       * functioning as operand.
       */
      template<int N, class T, class F>
      struct Unit: OpRep<T, Unit<N,T,F> >
      {
      private:
        typedef OpRep<T, Unit<N,T,F> > B;

      public:
        typedef typename B::size_type size_type;
        T const operator[](size_type i) const { return f.e[i] / g; }
        Unit(F const &f): f(f), g(len(this->f.e)) {}
        bool safe(void const *p, bool x) const { return f.safe(p,x); }

      private:
        Operand<N,T,F, !F::_is_lval> f;
        T g;
      };



      /**
       * The representation of the vector projection operation.
       *
       * \tparam N the number of elements in each of the operands.
       *
       * \tparam T The type of the vector elements.
       *
       * \tparam F The ultimate instance type of the sub-expression
       * functioning as left operand.
       *
       * \tparam G The ultimate instance type of the sub-expression
       * functioning as right operand.
       */
      template<int N, class T, class F, class G>
      struct Proj: OpRep<T, Proj<N,T,F,G> >
      {
      private:
        typedef OpRep<T, Proj<N,T,F,G> > B;

      public:
        typedef typename B::size_type size_type;
        T const operator[](size_type i) const { return h * g.e[i]; }
        Proj(F const &f, G const &g):
          f(f), g(g), h(dot(this->f.e,this->g.e) / sq_sum(this->g.e)) {}
        bool safe(void const *p, bool x) const { return g.safe(p,x); }

      private:
        Operand<N,T,F,        false> f;
        Operand<N,T,G, !G::_is_lval> g;
        T h;
      };



      /**
       * The representation of the 2-D perpendicular vector
       * derivation.
       *
       * \tparam T The type of the vector elements.
       *
       * \tparam F The ultimate instance type of the sub-expression
       * functioning as operand.
       */
      template<class T, class F>
      struct Perp: OpRep<T, Perp<T,F> >
      {
      private:
        typedef OpRep<T, Perp<T,F> > B;

      public:
        typedef typename B::size_type size_type;
        T const operator[](size_type i) const
        {
          return i == 0 ? -f[1] : f[0];
        }
        Perp(F const &f): f(f) {}
        bool safe(void const *p, bool) const
        {
          return f.safe(p,true);
        }

      private:
        F const &f;
      };



      /**
       * The representation of the cross product operation for 3-D
       * vectors.
       *
       * \tparam T The type of the vector elements.
       *
       * \tparam F The ultimate instance type of the sub-expression
       * functioning as left operand.
       *
       * \tparam G The ultimate instance type of the sub-expression
       * functioning as right operand.
       */
      template<class T, class F, class G>
      struct Cross: OpRep<T, Cross<T,F,G> >
      {
      private:
        typedef OpRep<T, Cross<T,F,G> > B;

      public:
        typedef typename B::size_type size_type;
        T const operator[](size_type i) const
        {
          return i == 0 ?
            f.e[1]*g.e[2] - f.e[2]*g.e[1] : i == 1 ?
            f.e[2]*g.e[0] - f.e[0]*g.e[2] :
            f.e[0]*g.e[1] - f.e[1]*g.e[0];
        }
        Cross(F const &f, G const &g): f(f), g(g) {}
        bool safe(void const *p, bool) const
        {
          return f.safe(p,true) && g.safe(p,true);
        }

      private:
        Operand<3,T,F, !F::_is_lval> f;
        Operand<3,T,G, !G::_is_lval> g;
      };






      /**
       * The representation of a vector slice.
       *
       * \tparam S The stride.
       *
       * \tparam F The ultimate instance type of the underlying vector
       * (or vector expression).
       */
      template<int S, class F> struct SliceRep
      {
        typedef typename F::size_type size_type;
        typedef Core::StrideIter<S, typename F::iterator>       iterator;
        typedef Core::StrideIter<S, typename F::const_iterator> const_iterator;
        iterator       iter()       { return       iterator(f.begin()+o); }
        const_iterator iter() const { return const_iterator(f.begin()+o); }
        SliceRep(F &f, size_type o): f(f), o(o) {}
        static bool const is_lval = F::_is_lval;
        void const *lval_rep(bool &x) const
        {
          if(o) x=true;
          return f._lval_rep(x);
        }
        bool safe(void const *p, bool x) const
        {
          return f._safe(p, x||o||S!=1);
        }

      private:
        F &f;
        size_type const o;
      };



      /**
       * The ultimate instance type of vector slices.
       *
       * \tparam N The number of elements in the slice.
       *
       * \tparam T The type of the elements of the slice.
       *
       * \tparam F The ultimate instance type of the underlying vector
       * (or vector expression).
       */
      template<int N, int S, class T, class F>
      struct Slice: VecVal<N, T, SliceRep<S,F>, Slice<N,S,T,F> >
      {
      private:
        typedef SliceRep<S,F> R;
        typedef Slice<N,S,T,F> E;
        typedef VecVal<N,T,R,E> B;

      public:
        typedef typename B::size_type size_type;

        Slice(F &f, size_type o): B(R(f,o)) {}

        using B::operator=;
      };
    }









    /**
     * A general purpose mathematical vector template. It utilizes the
     * concept of expression templates to optimize expressions
     * involving such vectors.
     *
     * Specializations of this class are used to provide convenience
     * constructors for various numbers of elements.
     *
     * \tparam N The number of elements in the vector.
     *
     * \tparam T The type of the vector elements.
     *
     * \note The bulk of the available methods on this class are
     * inherited from <tt>_VecImpl::Base</tt>. Besides that, a large
     * number of vector operators and functions are defined the the \c
     * Math namespace.
     */
    template<int N, class T = double>
    struct BasicVec: VecBuf<N, T, BasicVec<N,T> >
    {
      /**
       * The default constructor does not initialize the elements.
       */
      BasicVec() {}

      /**
       * Initialize all elements with the specified value.
       */
      explicit BasicVec(T);

      /**
       * Initialize the elements with the corresponding values of the
       * specified array. The specified array must have at least as
       * many elements as this vector.
       *
       * \tparam U Must be a type that is implicitely convertible to
       * <tt>T</tt>.
       */
      template<class U> explicit BasicVec(U const *);

      /**
       * Initialize the elements of this vector with the corresponding
       * elements of the specified vector.
       */
      template<class Rep, class Inst> BasicVec(VecVal<N, T, Rep, Inst> const &);

      /**
       * Initialize this vector from a series of the same length and type.
       */
      BasicVec(Core::Series<N,T> const &s) { set(s.get()); }

      using VecBuf<N, T, BasicVec<N,T> >::operator=;
    };



    typedef BasicVec<2> Vec2;
    typedef BasicVec<3> Vec3;
    typedef BasicVec<4> Vec4;

    typedef BasicVec<2, float> Vec2F;
    typedef BasicVec<3, float> Vec3F;
    typedef BasicVec<4, float> Vec4F;

    typedef BasicVec<2, long double> Vec2L;
    typedef BasicVec<3, long double> Vec3L;
    typedef BasicVec<4, long double> Vec4L;






    template<int N, class T, class R, class S, class F, class G>
    inline void swap(VecVal<N,T,R,F> &f, VecVal<N,T,S,G> &g)
    {
      f.swap(g);
    }


    /**
     * Fold the specified vector to a scalar by applying the specified
     * binary operator repeatedly for each element. The folding
     * operation can be described as follows:
     *
     * <pre>
     *
     *   (((i + v0) + v1) + v2) + ...
     *
     * </pre>
     *
     * where \c + stands for the specified operator.
     */
    template<int N, class T, class R, class F, class O>
    inline T fold(VecVal<N,T,R,F> const &f, T i, O o)
    {
      return std::accumulate(f.begin(), f.end(), i, o);
    }


    /**
     * Apply the specified unary operator to each element of the
     * specified vector.
     *
     * \note The result is not evaluated until used, that is, the
     * concept of expression templates is used to delay and optimize
     * the evaluation.
     */
    template<int N, class T, class R, class F, class O>
    inline _VecImpl::Op<N, T, _VecImpl::UnMap<T,F,O> > const
    map(VecVal<N,T,R,F> const &f, O o)
    {
      typedef _VecImpl::UnMap<T,F,O> _R;
      return _VecImpl::Op<N,T,_R>(_R(static_cast<F const &>(f), o));
    }


    /**
     * Merge the two specified vectors by apply the specified binary
     * operator to each pair of corresponding elements.
     *
     * \note The result is not evaluated until used, that is, the
     * concept of expression templates is used to delay and optimize
     * the evaluation.
     */
    template<int N, class T, class R, class S, class F, class G, class O>
    inline _VecImpl::Op<N, T, _VecImpl::BinMap<T,F,G,O> > const
    map(VecVal<N,T,R,F> const &f, VecVal<N,T,S,G> const &g, O o)
    {
      typedef _VecImpl::BinMap<T,F,G,O> _R;
      return _VecImpl::Op<N,T,_R>(_R(static_cast<F const &>(f),
                                     static_cast<G const &>(g), o));
    }


    /**
     * Test for equality between the two specified vectors. The test
     * is based on equality of the elements, and thus, depends on the
     * element type.
     */
    template<int N, class T, class R, class S, class F, class G>
    inline bool operator==(VecVal<N,T,R,F> const &f, VecVal<N,T,S,G> const &g)
    {
      return std::equal(f.begin(), f.end(), g.begin());
    }


    /**
     * Test for inequality between the two specified vectors. The test
     * is based on equality of the elements, and thus, depends on the
     * element type.
     */
    template<int N, class T, class R, class S, class F, class G>
    inline bool operator!=(VecVal<N,T,R,F> const &f, VecVal<N,T,S,G> const &g)
    {
      return !std::equal(f.begin(), f.end(), g.begin());
    }


    /**
     * Compute the sum of the squares of each element in this
     * vector. This is the same as the dot product of this vector with
     * itself. It is also the same as the square of the length of the
     * vector.
     */
    template<int N, class T, class R, class F>
    inline T sq_sum(VecVal<N,T,R,F> const &f)
    {
      return Math::fold(Math::map(f, Sq<T>()), T(), std::plus<T>());
    }


    /**
     * Compute the length of this vector.
     */
    template<int N, class T, class R, class F>
    inline T len(VecVal<N,T,R,F> const &f)
    {
      return std::sqrt(sq_sum(f));
    }


    /**
     * Get the minimum value amongst the elements of this vector.
     */
    template<int N, class T, class R, class F>
    inline T min(VecVal<N,T,R,F> const &f)
    {
      return Math::fold(f, std::numeric_limits<T>::is_integer ?
                        std::numeric_limits<T>::max() :
                        std::numeric_limits<T>::infinity(), Core::Min<T>());
    }


    /**
     * Get the maximum value amongst the elements of this vector.
     */
    template<int N, class T, class R, class F>
    inline T max(VecVal<N,T,R,F> const &f)
    {
      return Math::fold(f, std::numeric_limits<T>::is_integer ?
                        std::numeric_limits<T>::min() :
                        -std::numeric_limits<T>::infinity(), Core::Max<T>());
    }


    /**
     * Compute the dot product of the specified vectors. This is also
     * known as inner product.
     */
    template<int N, class T, class R, class S, class F, class G>
    inline T
    dot(VecVal<N,T,R,F> const &v, VecVal<N,T,S,G> const &w)
    {
      return vec_dot(v.begin(), v.end(), w.begin());
    }


    /**
     * Compute the angle of the specified 2-D vector.
     *
     * \return The angle in the range <tt>[-pi;pi]</tt>.
     *
     * \sa pol_ang
     */
    template<class T, class R, class F>
    inline T ang(VecVal<2,T,R,F> const &f)
    {
      return Math::pol_ang(f[0], f[1]);
    }


    /**
     * Compute the angle (in radians) between two specified vectors.
     */
    template<int N, class T, class R, class S, class F, class G>
    inline T
    ang(VecVal<N,T,R,F> const &f, VecVal<N,T,S,G> const &g)
    {
      _VecImpl::Operand<N,T,F, !F::_is_lval> _f(static_cast<F const &>(f));
      _VecImpl::Operand<N,T,G, !G::_is_lval> _g(static_cast<G const &>(g));
      return std::acos(dot(_f.e, _g.e)/(Math::len(_f.e)*Math::len(_g.e)));
    }


    /**
     * Compute the square of the distance between the two specified
     * points.
     */
    template<int N, class T, class R, class S, class F, class G>
    inline T
    sq_dist(VecVal<N,T,R,F> const &v, VecVal<N,T,S,G> const &w)
    {
      return vec_sq_dist(v.begin(), v.end(), w.begin());
    }


    /**
     * Compute the distance between the two specified points.
     */
    template<int N, class T, class R, class S, class F, class G>
    inline T
    dist(VecVal<N,T,R,F> const &v, VecVal<N,T,S,G> const &w)
    {
      return std::sqrt(vec_sq_dist(v.begin(), v.end(), w.begin()));
    }


    /**
     * Negate the specified vector.
     */
    template<int N, class T, class R, class F>
    inline _VecImpl::Op<N, T, _VecImpl::UnMap<T, F, std::negate<T> > > const
    operator-(VecVal<N,T,R,F> const &f)
    {
      return Math::map(f, std::negate<T>());
    }


    /**
     * Multiply the specified scalar by the specified vector.
     */
    template<int N, class T, class R, class F>
    inline auto operator*(T g, const VecVal<N,T,R,F>& f)
    {
      return Math::map(f, std::bind1st(std::multiplies<T>(), g));
    }


    /**
     * Multiply the specified vector by the specified scalar.
     */
    template<int N, class T, class R, class F>
    inline auto operator*(const VecVal<N,T,R,F>& f, T g)
    {
      return Math::map(f, std::bind2nd(std::multiplies<T>(), g));
    }


    /**
     * Divide the specified vector by the specified scalar. That
     * corresponds to multiplying it by the inverse of the specified
     * scalar.
     */
    template<int N, class T, class R, class F>
    inline auto operator/(VecVal<N,T,R,F> const &f, T g)
    {
      return Math::map(f, std::bind2nd(std::divides<T>(), g));
    }


    /**
     * Add the two specified vectors.
     *
     * \note The result is not evaluated until used, that is, the
     * concept of expression templates is used to delay and optimize
     * the evaluation.
     */
    template<int N, class T, class R, class S, class F, class G>
    inline _VecImpl::Op<N, T, _VecImpl::BinMap<T, F, G, std::plus<T> > > const
    operator+(VecVal<N,T,R,F> const &f, VecVal<N,T,S,G> const &g)
    {
      return Math::map(f, g, std::plus<T>());
    }


    /**
     * Subtract the second vector from the first one.
     *
     * \note The result is not evaluated until used, that is, the
     * concept of expression templates is used to delay and optimize
     * the evaluation.
     */
    template<int N, class T, class R, class S, class F, class G>
    inline _VecImpl::Op<N, T, _VecImpl::BinMap<T, F, G, std::minus<T> > > const
    operator-(VecVal<N,T,R,F> const &f, VecVal<N,T,S,G> const &g)
    {
      return Math::map(f, g, std::minus<T>());
    }


    /**
     * Compute the unit vector that is parallel to the specified
     * vector. This is also called the normalization of the specified
     * vector.
     */
    template<int N, class T, class R, class F>
    inline _VecImpl::Op<N, T, _VecImpl::Unit<N,T,F> > const
    unit(VecVal<N,T,R,F> const &f)
    {
      typedef _VecImpl::Unit<N,T,F> _R;
      return _VecImpl::Op<N,T,_R>(_R(static_cast<F const &>(f)));
    }


    /**
     * Compute the vector projection of \c v on \c w. This is also
     * known as the vector resolute of \c v on \c w.
     *
     * \note The result is not evaluated until used, that is, the
     * concept of expression templates is used to delay and optimize
     * the evaluation.
     */
    template<int N, class T, class R, class S, class F, class G>
    inline _VecImpl::Op<N, T, _VecImpl::Proj<N,T,F,G> > const
    proj(VecVal<N,T,R,F> const &f, VecVal<N,T,S,G> const &g)
    {
      typedef _VecImpl::Proj<N,T,F,G> _R;
      return _VecImpl::Op<N,T,_R>(_R(static_cast<F const &>(f),
                                     static_cast<G const &>(g)));
    }


    /**
     * Compute the perpendicular vector of the specified 2-D
     * vector. The perpendicular vector is a counterclockwise rotation
     * by 90 degrees.
     *
     * \note The result is not evaluated until used, that is, the
     * concept of expression templates is used to delay and optimize
     * the evaluation.
     */
    template<class T, class R, class F>
    inline _VecImpl::Op<2, T, _VecImpl::Perp<T,F> > const
    perp(VecVal<2,T,R,F> const &f)
    {
      typedef _VecImpl::Perp<T,F> _R;
      return _VecImpl::Op<2,T,_R>(_R(static_cast<F const &>(f)));
    }


    /**
     * Compute the cross product of the two specified 3-D vectors.
     *
     * \note The result is not evaluated until used, that is, the
     * concept of expression templates is used to delay and optimize
     * the evaluation.
     */
    template<class T, class R, class S, class F, class G>
    inline _VecImpl::Op<3, T, _VecImpl::Cross<T,F,G> > const
    operator*(VecVal<3,T,R,F> const &f, VecVal<3,T,S,G> const &g)
    {
      typedef _VecImpl::Cross<T,F,G> _R;
      return _VecImpl::Op<3,T,_R>(_R(static_cast<F const &>(f),
                                     static_cast<G const &>(g)));
    }


    /**
     * Check whether the first vector is less than the second one.
     *
     * The order is the natural lexicographic extension of the order
     * on the elements with the last element in the vector having the
     * greatest significance.
     *
     * One does not normally define a total order amongst vertors, but
     * it is usefull because it allows vectors to be sorted and
     * therfore used as keys in a map.
     */
    template<int N, class T, class R, class S, class F, class G>
    inline bool operator<(VecVal<N,T,R,F> const &f, VecVal<N,T,S,G> const &g)
    {
      for(int i=N-1; 0<=i; --i)
      {
        if(f[i] < g[i]) return true;
        if(g[i] < f[i]) return false;
      }
      return false;
    }


    /**
     * Print the specified vector onto the specified output stream.
     */
    template<class C, class S, int N, class T, class R, class F>
    std::basic_ostream<C,S> &operator<<(std::basic_ostream<C,S> &out,
                                        VecVal<N,T,R,F> const &v)
    {
      vec_print(out, v.begin(), v.end());
      return out;
    }


    /**
     * Parse characters from the specified input stream as a vector of
     * the same type as the one specified. If successful, assign the
     * result to the specified vector.
     */
    template<class C, class S, int N, class T, class R, class F>
    std::basic_istream<C,S> &operator>>(std::basic_istream<C,S> &in,
                                        VecVal<N,T,R,F> &v)
    {
      BasicVec<N,T> w;
      vec_parse(in, w.begin(), w.end());
      if(!in.fail() && !in.bad()) v = w;
      return in;
    }





    /**
     * Specialization for 2-D vectors providing an extra convenience
     * constructor.
     */
    template<class T>
    struct BasicVec<2,T>: VecBuf<2, T, BasicVec<2,T> >
    {
      /**
       * The default constructor does not initialize the elements.
       */
      BasicVec() {}

      /**
       * Initialize both elements with the specified value.
       */
      explicit BasicVec(T);

      /**
       * Initialize the corresponding elements with the two specified
       * values.
       */
      explicit BasicVec(T,T);

      /**
       * Initialize the elements with the corresponding values of the
       * specified array. The specified array must have at least two
       * elements.
       *
       * \tparam U Must be a type that is implicitely convertible to
       * <tt>T</tt>.
       */
      template<class U> explicit BasicVec(U const *);

      /**
       * Initialize the elements of this vector with the corresponding
       * elements of the specified vector.
       */
      template<class R, class E> BasicVec(VecVal<2,T,R,E> const &);

      /**
       * Initialize this vector from a series of the same length and type.
       */
      BasicVec(Core::Series<2,T> const &s) { this->set(s.get()); }

      using VecBuf<2, T, BasicVec<2,T> >::operator=;
    };



    /**
     * Specialization for 3-D vectors providing extra convenience
     * constructors.
     */
    template<class T>
    struct BasicVec<3,T>: VecBuf<3, T, BasicVec<3,T> >
    {
      /**
       * The default constructor does not initialize the elements.
       */
      BasicVec() {}

      /**
       * Initialize all three elements with the specified value.
       */
      explicit BasicVec(T);

      /**
       * Initialize the corresponding elements with the three
       * specified values.
       */
      explicit BasicVec(T,T,T);

      /**
       * Initialize the elements with the corresponding values of the
       * specified array. The specified array must have at least three
       * elements.
       *
       * \tparam U Must be a type that is implicitely convertible to
       * <tt>T</tt>.
       */
      template<class U> explicit BasicVec(U const *);

      /**
       * Initialize the elements of this vector with the corresponding
       * elements of the specified vector.
       */
      template<class R, class E> BasicVec(VecVal<3,T,R,E> const &);

      /**
       * Initialize the first two elements of this vector with the
       * corresponding elements of the specified vector. The final
       * element is initialized with the value specified as the second
       * argument.
       */
      template<class R, class E>
      explicit BasicVec(VecVal<2,T,R,E> const &, T = T());

      /**
       * Initialize this vector from a series of the same length and type.
       */
      BasicVec(Core::Series<3,T> const &s) { this->set(s.get()); }

      using VecBuf<3, T, BasicVec<3,T> >::operator=;
    };



    /**
     * Specialization for 4-D vectors providing extra convenience
     * constructors.
     */
    template<class T>
    struct BasicVec<4,T>: VecBuf<4, T, BasicVec<4,T> >
    {
      /**
       * The default constructor does not initialize the elements.
       */
      BasicVec() {}

      /**
       * Initialize all four elements with the specified value.
       */
      explicit BasicVec(T);

      /**
       * Initialize the corresponding elements with the four specified
       * values.
       */
      explicit BasicVec(T,T,T,T);

      /**
       * Initialize the elements with the corresponding values of the
       * specified array. The specified array must have at least four
       * elements.
       *
       * \tparam U Must be a type that is implicitely convertible to
       * <tt>T</tt>.
       */
      template<class U> explicit BasicVec(U const *);

      /**
       * Initialize the elements of this vector with the corresponding
       * elements of the specified vector.
       */
      template<class R, class E> BasicVec(VecVal<4,T,R,E> const &);

      /**
       * Initialize the first three elements of this vector with the
       * corresponding elements of the specified vector. The final
       * element is initialized with the value specified as the second
       * argument.
       */
      template<class R, class E>
      explicit BasicVec(VecVal<3,T,R,E> const &, T = T());

      /**
       * Initialize this vector from a series of the same length and type.
       */
      BasicVec(Core::Series<4,T> const &s) { this->set(s.get()); }

      using VecBuf<4, T, BasicVec<4,T> >::operator=;
    };









    // Template definitions for:  VecBase

    template<int N, class T, class R, class E>
    bool const VecBase<N,T,R,E>::_is_lval;

    template<int N, class T, class R, class E>
    bool VecBase<N,T,R,E>::is_zero() const
    {
      const_iterator const e = end();
      return std::find_if(begin(), e, std::bind2nd(std::not_equal_to<T>(), 0)) == e;
    }

    template<int N, class T, class R, class E>
    inline typename VecBase<N,T,R,E>::reverse_iterator VecBase<N,T,R,E>::rbegin()
    {
      return reverse_iterator(end());
    }

    template<int N, class T, class R, class E>
    inline typename VecBase<N,T,R,E>::reverse_iterator VecBase<N,T,R,E>::rend()
    {
      return rbegin() + N;
    }

    template<int N, class T, class R, class E>
    inline typename VecBase<N,T,R,E>::const_reverse_iterator
    VecBase<N,T,R,E>::rbegin() const
    {
      return const_reverse_iterator(end());
    }

    template<int N, class T, class R, class E>
    inline typename VecBase<N,T,R,E>::const_reverse_iterator
    VecBase<N,T,R,E>::rend() const
    {
      return rbegin() + N;
    }

    template<int N, class T, class R, class E>
    inline E &VecBase<N,T,R,E>::set(T e)
    {
      std::fill(begin(), end(), e);
      return static_cast<E &>(*this);
    }

    template<int N, class T, class R, class E> template<class U>
    inline E &VecBase<N,T,R,E>::set(U const *e)
    {
      std::copy(e, e+N, begin());
      return static_cast<E &>(*this);
    }

    template<int N, class T, class R, class E> template<class S, class F>
    inline E &VecBase<N,T,R,E>::set(VecBase<N,T,S,F> const &f)
    {
      if(safe_assign(f)) std::copy(f.begin(), f.end(), begin());
      else set(_VecImpl::Cache<N,T>(f));
      return static_cast<E &>(*this);
    }

    template<int N, class T, class R, class E>
    inline E &VecBase<N,T,R,E>::operator=(VecBase const &f)
    {
      return set(f);
    }

    template<int N, class T, class R, class E> template<class S, class F>
    inline E &VecBase<N,T,R,E>::operator=(VecVal<N,T,S,F> const &f)
    {
      return set(f);
    }

    template<int N, class T, class R, class E> template<class S, class F>
    inline void VecBase<N,T,R,E>::swap(VecBase<N,T,S,F> &f)
    {
      if(safe_assign(f))
      {
        std::swap_ranges(f.begin(), f.end(), begin());
        return;
      }

      _VecImpl::Cache<N,T> g(f);
      f = *this;
      *this = g;
    }

    template<int N, class T, class R, class E> template<class O>
    inline E &VecBase<N,T,R,E>::map(O o)
    {
      vec_unop_assign(begin(), end(), o);
      return static_cast<E &>(*this);
    }

    template<int N, class T, class R, class E>
    template<class S, class F, class O>
    inline E &VecBase<N,T,R,E>::map(VecVal<N,T,S,F> const &f, O o)
    {
      if(safe_assign(f)) vec_binop_assign(begin(), end(), f.begin(), o);
      else map(_VecImpl::Cache<N,T>(f), o);
      return static_cast<E &>(*this);
    }

    template<int N, class T, class R, class E>
    inline E &VecBase<N,T,R,E>::operator*=(T v)
    {
      return map(std::bind2nd(std::multiplies<T>(), v));
    }

    template<int N, class T, class R, class E>
    inline E &VecBase<N,T,R,E>::operator/=(T v)
    {
      return map(std::bind2nd(std::divides<T>(), v));
    }

    template<int N, class T, class R, class E> template<class S, class F>
    inline E &VecBase<N,T,R,E>::operator+=(VecVal<N,T,S,F> const &f)
    {
      return map(f, std::plus<T>());
    }

    template<int N, class T, class R, class E> template<class S, class F>
    inline E &VecBase<N,T,R,E>::operator-=(VecVal<N,T,S,F> const &f)
    {
      return map(f, std::minus<T>());
    }

    template<int N, class T, class R, class E> template<class S, class F>
    inline E &VecBase<N,T,R,E>::proj(VecVal<N,T,S,F> const &f)
    {
      return set(Math::proj(static_cast<E &>(*this), f));
    }

    template<int N, class T, class R, class E> template<int M>
    inline _VecImpl::Slice<M,1,T,E> VecBase<N,T,R,E>::slice(size_type i)
    {
      return _VecImpl::Slice<M,1,T,E>(static_cast<E &>(*this), i);
    }

    template<int N, class T, class R, class E> template<int M>
    inline _VecImpl::Slice<M,1,T, E const> const VecBase<N,T,R,E>::slice(size_type i) const
    {
      return _VecImpl::Slice<M,1,T, E const>(static_cast<E const &>(*this), i);
    }

    template<int N, class T, class R, class E> template<class S, class F>
    inline bool VecBase<N,T,R,E>::safe_assign(VecBase<N,T,S,F> const &f) const
    {
      bool x = false;
      void const *p = _lval_rep(x);
      return f._safe(p,x);
    }






    // Template definitions for:  _VecImpl::Spec

    template<class T, class R, class I>
    inline I &VecLenSpec<2,T,R,I>::set(T e0, T e1)
    {
      this->sub(0) = e0;
      this->sub(1) = e1;
      return static_cast<I &>(*this);
    }

    template<class T, class R, class I>
    inline I &VecLenSpec<2,T,R,I>::perp()
    {
      T e0 = this->sub(0);
      this->sub(0) = -this->sub(1);
      this->sub(1) =  e0;
      return static_cast<I &>(*this);
    }


    template<class T, class R, class I>
    inline I &VecLenSpec<3,T,R,I>::set(T e0, T e1, T e2)
    {
      this->sub(0) = e0;
      this->sub(1) = e1;
      this->sub(2) = e2;
      return static_cast<I &>(*this);
    }

    template<class T, class R, class I> template<class S, class J>
    inline I &VecLenSpec<3,T,R,I>::operator*=(VecVal<3,T,S,J> const &v)
    {
      // This one relies on automatic conflict detection
      return set(static_cast<I &>(*this) * v);
    }


    template<class T, class R, class I>
    inline I &VecLenSpec<4,T,R,I>::set(T e0, T e1, T e2, T e3)
    {
      this->sub(0) = e0;
      this->sub(1) = e1;
      this->sub(2) = e2;
      this->sub(3) = e3;
      return static_cast<I &>(*this);
    }






      // Template definitions for:  _VecImpl::RepBuf  and  VecBuf

    namespace _VecImpl
    {
      template<int N, class T> bool const RepBuf<N,T>::is_lval;
    }

    template<int N, class T, class I> inline I const &VecBuf<N,T,I>::zero()
    {
      static I const e((T()));
      return e;
    }






    // Template definitions for:  _VecImpl::OpRep  and  _VecImpl::Op

    namespace _VecImpl
    {
      template<class T, class R> bool const OpRep<T,R>::is_lval;
    }





    // Template definitions for:  _VecImpl::SliceRep  and _VecImpl::Slice

    namespace _VecImpl
    {
      template<int S, class F> bool const SliceRep<S,F>::is_lval;
    }






    // Template definitions for:  BasicVec

    template<int N, class T>
    inline BasicVec<N,T>::BasicVec(T e)
    {
      this->set(e);
    }

    template<int N, class T> template<class U>
    inline BasicVec<N,T>::BasicVec(U const *e)
    {
      this->set(e);
    }

    template<int N, class T> template<class R, class E>
    inline BasicVec<N,T>::BasicVec(VecVal<N,T,R,E> const &e)
    {
      this->set(e);
    }


    template<class T>
    inline BasicVec<2,T>::BasicVec(T e)
    {
      this->set(e);
    }

    template<class T>
    inline BasicVec<2,T>::BasicVec(T e0, T e1)
    {
      this->set(e0, e1);
    }

    template<class T> template<class U>
    inline BasicVec<2,T>::BasicVec(U const *e)
    {
      this->set(e);
    }

    template<class T> template<class R, class E>
    inline BasicVec<2,T>::BasicVec(VecVal<2,T,R,E> const &e)
    {
      this->set(e);
    }


    template<class T>
    inline BasicVec<3,T>::BasicVec(T e)
    {
      this->set(e);
    }

    template<class T>
    inline BasicVec<3,T>::BasicVec(T e0, T e1, T e2)
    {
      this->set(e0, e1, e2);
    }

    template<class T> template<class U>
    inline BasicVec<3,T>::BasicVec(U const *e)
    {
      this->set(e);
    }

    template<class T> template<class R, class E>
    inline BasicVec<3,T>::BasicVec(VecVal<3,T,R,E> const &e)
    {
      this->set(e);
    }

    template<class T> template<class R, class E>
    inline BasicVec<3,T>::BasicVec(VecVal<2,T,R,E> const &e, T f)
    {
      this->template slice<2>().set(e);
      this->sub(2) = f;
    }


    template<class T>
    inline BasicVec<4,T>::BasicVec(T e)
    {
      this->set(e);
    }

    template<class T>
    inline BasicVec<4,T>::BasicVec(T e0, T e1, T e2, T e3)
    {
      this->set(e0, e1, e2, e3);
    }

    template<class T> template<class U>
    inline BasicVec<4,T>::BasicVec(U const *e)
    {
      this->set(e);
    }

    template<class T> template<class R, class E>
    inline BasicVec<4,T>::BasicVec(VecVal<4,T,R,E> const &e)
    {
      this->set(e);
    }

    template<class T> template<class R, class E>
    inline BasicVec<4,T>::BasicVec(VecVal<3,T,R,E> const &e, T f)
    {
      this->template slice<3>().set(e);
      this->sub(3) = f;
    }
  }
}

#endif // ARCHON_MATH_VECTOR_HPP
