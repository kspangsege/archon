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

#ifndef ARCHON_CORE_ATOMIC_HPP
#define ARCHON_CORE_ATOMIC_HPP

#if defined __GNUC__ && (defined __i386__ || defined __x86_64__)
#else // The default fall-back implementation (bad performance)
#include <archon/core/mutex.hpp>
#endif


namespace archon
{
  namespace Core
  {
    /**
     * Atomic integer operations. Useful for reference counting
     * etc..
     *
     * Inspired by the following implementations of similar
     * functionality:
     *
     * - GCC 3.3 STL: bits/atomicity.h
     * - GNOME/GLIB:  glib-2.0/glib/gatomic.h
     * - BOOST:       detail/sp_counted_base_gcc_x86.hpp
     *
     * Currently a fast (lock-free) implementations is only available
     * on IA-32 (Intel 486 and above) plus IA-64 with GCC.
     *
     * If you need to port this class to other platforms, then please
     * consult the other implementations mentioned above.
     *
     * \note Since mutex locking may be involved on some platforms, it
     * is not safe to access or manipulate variables of this type in a
     * signal handler.
     */
    struct Atomic
    {
      Atomic() {}
      Atomic(const Atomic &a) { set(a.get()); }
      Atomic(int w) { set(w); }

      /**
       * \sa get
       */
      operator int() const { return get(); }

      /**
       * \sa set
       */
      Atomic &operator=(Atomic const &a) { set(a.get());  return *this; }

      /**
       * \sa set
       */
      Atomic &operator=(int w) { set(w);  return *this; }

      /**
       * \sa inc
       */
      Atomic &operator++() { inc();  return *this; }

      /**
       * \sa fetch_and_add
       */
      int operator++(int) { return fetch_and_add(1); }

      /**
       * \sa dec
       */
      Atomic &operator--() { dec(); return *this; }

      /**
       * \sa fetch_and_add
       */
      int operator--(int) { return fetch_and_add(-1); }

      /**
       * \sa add
       */
      Atomic &operator+=(int w) { add(w);  return *this; }

      /**
       * \sa sub
       */
      Atomic &operator-=(int w) { sub(w); return *this; }

      /**
       * Read the value of this integer object atomically.
       *
       * \return The value of this integer object as it was when read.
       */
      int get() const;

      /**
       * Assign a new value to this integer object atomically.
       *
       * \param w The value to assign.
       */
      Atomic &set(int w);

      /**
       * Increment the value of this integer object atomically.
       */
      Atomic &inc();

      /**
       * Decrement the value of this integer object atomically.
       */
      Atomic &dec();

      /**
       * Add to the value of this integer object atomically.
       *
       * \param w The value to add.
       */
      Atomic &add(int w);

      /**
       * Subtract from the value of this integer object atomically.
       *
       * \param w The value to subtract.
       */
      Atomic &sub(int w);

      /**
       * Decrement the value of this integer object and test if the
       * result is zero. The decrement and the test operation are
       * performed as one single atomic operation.
       *
       * \return True iff the result was zero.
       */
      bool dec_and_zero_test();

      /**
       * Increment the value of this integer object, but only if its
       * original value is not zero. The test and the increment
       * operation are performed as one single atomic operation.
       *
       * \return The original value.
       */
      int inc_if_not_zero();

      /**
       * Add to the value of this integer object and return its
       * original value. The read and the add operation are performed
       * as one single atomic operation.
       *
       * \param w The value to add.
       *
       * \return The original value.
       */
      int fetch_and_add(int w);

      /**
       * Assign a new value to this integer object but only if it has
       * a certain original value. The test and the assignment
       * operation (when done) are performed as one single atomic
       * operation.
       *
       * \param t The required original value.
       *
       * \param w The new value to be assigned.
       *
       * \return True iff the assignment was done (ie. the test was
       * successful.)
       */
      bool test_and_set(int t, int w);

    private:

#if defined __GNUC__ && (defined __i386__ || defined __x86_64__)
      volatile int v;
#else // The default fall-back implementation (bad performance)
      Mutex m;
      int v;
#endif
    };

#if defined __GNUC__ && (defined __i386__ || defined __x86_64__)

    /*
     * This is the general form of an inline assember statement in
     * GCC:
     *
     *   asm [volatile] (template : output : input : clobbers);
     *
     * 'template' is a string containing the assembler template which
     * is a list of instructions. Each instruction should be delimited
     * by '\n\t'.
     *
     * 'input' is a list of input operands. If these operands are not
     * mentioned in 'output' nor in 'clobbers', GCC assumes they will
     * retain their value throughout the asm statement.
     *
     * 'clobbers' is a list of specific registers that are clobbered
     * by the asm statement. Clobbered registers will be considered
     * unusable as both input and output operand to the register
     * allocator. If any of the flags in the 'status register' (or
     * condition code register) are altered by the statement, "cc"
     * must be added to the clobber list (for historic reasons, GCC
     * adds "cc" to the clobbers on x86 architectures by default, but
     * this should not be relied on.) If memory not covered by output
     * operands is clobbered (or if memory not covered by input
     * operands is read from), "memory" must be added to the clobber
     * list.
     *
     * For efficiency, GCC assumes by default that a statement which
     * declares output, has no side effects (ie, is a pure function.)
     * The "volatile" keyword is used to tell GCC that the statement
     * does have important side-effects. It is implicit if there is no
     * declared output.
     *
     * http://siyobik.info/index.php?module=x86  (x86 instruction set reference)
     *
     * http://sourceware.org/binutils/docs-2.17/as/i386_002dDependent.html#i386_002dDependent
     */



    /*
     * We can just use a simple read. We always read from memory
     * since we have declared v to be volatile. Also, on the Intel
     * platform we do not need to concern ourselves with memory
     * ordering, thus we do not need memory barriers here (explicit
     * synchronizarion of memory update/access accross several CPUs.)
     */
    inline int Atomic::get() const
    {
      return v;
    }

    /*
     * We can just use a simple assignment. We always write to memory
     * since we have declared v to be volatile. Also, on the Intel
     * platform we do not need to concern ourselves with memory
     * ordering, thus we do not need memory barriers here (explicit
     * synchronizarion of memory update/access accross several CPUs.)
     */
    inline Atomic &Atomic::set(int w)
    {
      v = w;
      return *this;
    }

    inline Atomic &Atomic::inc()
    {
      // Assumes v is a 32-bit signed or unsigned integer.
      asm("lock\n\t"   // 'lock' makes 'inc' atomic.
          "incl %0" :
          "+m"(v) ::   // 'lock' requires a memory destination
                       // operand.
          "cc");       // 'inc' modifies some flags.
      return *this;
    }

    inline Atomic &Atomic::dec()
    {
      // Assumes v is a 32-bit signed or unsigned integer.
      asm("lock\n\t"   // 'lock' makes 'dec' atomic.
          "decl %0" :
          "+m"(v) ::   // 'lock' requires a memory destination
                       // operand.
          "cc");       // 'dec' modifies some flags.
      return *this;
    }

    inline Atomic &Atomic::add(int w)
    {
      // Assumes v and w are 32-bit signed or unsigned integers.
      asm("lock\n\t"   // 'lock' makes 'add' atomic.
          "addl %1, %0" :
          "+m"(v) :    // 'lock' requires a memory destination
                       // operand.
          "ir"(w) :    // 'add' requires that when destination is.
                       // memory, other operand must be immediate or
                       // register.
          "cc");       // 'add' modifies some flags.
      return *this;
    }

    inline Atomic &Atomic::sub(int w)
    {
      // Assumes v and w are 32-bit signed or unsigned integers.
      asm("lock\n\t"   // 'lock' makes 'sub' atomic.
          "subl %1, %0" :
          "+m"(v) :    // 'lock' requires a memory destination
                       // operand.
          "ir"(w) :    // 'sub' requires that when destination is.
                       // memory, other operand must be immediate or
                       // register.
          "cc");       // 'sub' modifies some flags.
      return *this;
    }

    inline bool Atomic::dec_and_zero_test()
    {
      // Assumes v is a 32-bit signed or unsigned integer, and that
      // 'char' is 8-bit.
      char is_zero;
      asm("lock\n\t"       // 'lock' makes 'dec' atomic.
          "decl %0\n\t"
          "sete %1" :       // byte operand = ZF ? 1 : 0
          "+m"(v),          // 'lock' requires a memory destination
                            // operand.
          "=qm"(is_zero) :: // 'sete' allows only one of the byte
                            // sized registers or a byte sized memory
                            // operand.
          "cc");            // 'dec' modifies some flags.
      return is_zero != 0;
    }

    inline int Atomic::inc_if_not_zero()
    {
      // Assumes v is a 32-bit signed or unsigned integer, and that 'int'
      // is 32-bit.
      int orig_val, tmp;
      asm("movl %0, %%eax\n\t"  // Simple load, no lock needed
          "0:\n\t"
          "test %%eax, %%eax\n\t"
          "jz 1f\n\t"           // Abort if eax is was has become zero.
          "movl %%eax, %2\n\t"
          "incl %2\n\t"
          "lock\n\t"            // 'lock' makes 'cmpxchg' atomic.
          "cmpxchgl %2, %0\n\t" // if(eax == %0) %0 = %2; else eax = %0;
          "jne 0b\n\t"          // Repeat if test failed. This is
                                // supposed to be a rare event.
          "1:":
          "+m"(v),              // 'lock' requires a memory
                                // destination operand.
          "=&a"(orig_val),      // The early clobber is not strictly
                                // necessary, since no input registers
                                // needs to be allocated.
          "=&r"(tmp) ::         // Not actually an output, it is just
                                // a way of saying that GCC can choose
                                // the register.
          "cc");                // Flags are modified.

      return orig_val;
    }

    inline int Atomic::fetch_and_add(int w)
    {
      // Assumes v and w are 32-bit signed or unsigned integers.
      asm("lock\n\t"       // 'lock' makes 'xadd' atomic.
          "xaddl %1, %0" : // tmp = %1; %1 = %0; %0 += tmp
          "+m"(v),         // 'lock' requires a memory destination
                           // operand.
          "+r"(w) ::       // The "source" operand of 'xadd' must be a
                           // register.
          "cc");           // 'xadd' modifies some flags.
      return w;
    }

    inline bool Atomic::test_and_set(int t, int w)
    {
      int a = t;
      // Assumes v, a, and w are 32-bit signed or unsigned integers.
      asm("lock\n\t"          // 'lock' makes 'cmpxchg' atomic.
          "cmpxchgl %2, %0" : // if(eax == %0) %0 = %2; else eax = %0;
          "+m"(v),            // 'lock' requires a memory destination
                              // operand.
          "+a"(a) :
          "r"(w) :            // The "source" operand of 'cmpxchg'
                              // must be a register.
          "cc");              // 'cmpxchg' modifies some flags.
      return a == t;
    }


#else // The default fall-back implementation (bad performance)

    inline int Atomic::get() const
    {
      Mutex::Lock l(m);
      return v;
    }

    inline Atomic &Atomic::set(int w)
    {
      Mutex::Lock l(m);
      v = w;
      return *this;
    }

    inline Atomic &Atomic::inc()
    {
      Mutex::Lock l(m);
      ++v;
      return *this;
    }

    inline Atomic &Atomic::dec()
    {
      Mutex::Lock l(m);
      --v;
      return *this;
    }

    inline Atomic &Atomic::add(int w)
    {
      Mutex::Lock l(m);
      v += w;
      return *this;
    }

    inline Atomic &Atomic::sub(int w)
    {
      Mutex::Lock l(m);
      v -= w;
      return *this;
    }

    inline bool Atomic::dec_and_zero_test()
    {
      Mutex::Lock l(m);
      return --v == 0;
    }

    inline int Atomic::inc_if_not_zero()
    {
      Mutex::Lock l(m);
      return v ? v++ : 0;
    }

    inline int Atomic::fetch_and_add(int w)
    {
      Mutex::Lock l(m);
      int u = v;
      v += w;
      return u;
    }

    inline bool Atomic::test_and_set(int t, int w)
    {
      Mutex::Lock l(m);
      if(v!=t) return false;
      v = w;
      return true;
    }

#endif

  }
}

#endif // ARCHON_CORE_ATOMIC_HPP
