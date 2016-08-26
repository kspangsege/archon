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

#ifndef ARCHON_CORE_MEMORY_HPP
#define ARCHON_CORE_MEMORY_HPP

#include <cstddef>
#include <algorithm>
#include <iterator>
#include <vector>

#include <archon/core/functions.hpp>


namespace archon
{
  namespace core
  {
/*
  template<class T, int N=64> struct Alloc
  {
    T *alloc()
    {
      if(unused.empty())
      {
        chunks.reserve(chunks.size()+1);
        unused.reserve(unused.size()+N);
        T *const p = new T[N];
        chunks.push_back(p);
        for(int i=0; i<N; ++i) unused.push_back(p+i);
      }
      T *const p = unused.back();
      unused.pop_back();
      return p;
    }

    void free(T *p)
    {
      unused.push_back(p);
    }

    ~Alloc()
    {
      typedef vector<T *>::const_iterator iter;
      iter const e = chunks.end();
      for(iter i=chunks.begin(); i!=e; ++i) delete[] *i;
    }

  private:
    vector<T *> chunks;
    vector<T *> unused;
  };
*/



    /**
     * Vector is cleared
     *
     * If capacity is less than min_capacity, or greater than
     * max_capacity, it is set to min_capacity. Otherwise it is left
     * alone.
     */
    template<class V> void clear_vector(V &v, std::size_t min_capacity = 0, std::size_t max_capacity = 0)
    {
      if(max_capacity < v.capacity())
      {
        V w;
        std::swap(v,w); // Release memory
      }
      else v.clear();
      v.reserve(min_capacity);
    }



    /**
     * Extend a sequence of length N to length N+M by repeating the
     * original sequence. The last repetition produced will be
     * truncated if M is not divisible by N. For forward repetition
     * (\c m > 0) the beginning of the extended sequence will be
     * coincident with the beginning of the original sequence. For
     * backwards repetition (\c m < 0) the end of the extended
     * sequence will be coincident with the end of the original
     * sequence.
     *
     * \param iter Normally an iterator that points to the first
     * element of the original sequence, however, if \c n < 0, then it
     * is assumed to point one beyond the last element of the original
     * sequence.
     *
     * \param n The length of the original sequence. A positive value
     * indicates that the specified iterator points to the first
     * element of the sequence. A negative value indicates that it
     * points one beyond the last element of the sequence and N =
     * -<tt>n</tt>. Zero is not allowed.
     *
     * \param m The length of the extended sequence. A positive value
     * indicates forward extension (the beginning of the extended
     * sequence will be coincident with the beginning of the original
     * sequence). A negative value indicates backwards extension (the
     * end of the extended sequence will be coincident with the end of
     * the original sequence) and M = -<tt>m</tt>.
     *
     * \return The length of the final truncated repetition, or zero
     * if \c m is divisible by <tt>n</tt>. It is always either
     * positive or zero.
     */
    template<class I> typename std::iterator_traits<I>::difference_type
    repeat(I iter, typename std::iterator_traits<I>::difference_type n,
           typename std::iterator_traits<I>::difference_type m);

    /**
     * Faster version of \c repeat that assumes a positive <tt>m</tt>.
     *
     * \sa repeat
     */
    template<class I> typename std::iterator_traits<I>::difference_type
    repeatForward(I iter, typename std::iterator_traits<I>::difference_type n,
                  typename std::iterator_traits<I>::difference_type m);



    /**
     * Compare two endianness descriptions for compatibility. If any
     * of the two specified endianness vectors are empty, that
     * endianness is understood as the native endianness.
     *
     * The completely general endianness description used here, is a
     * bit vector where the first bit specifies the order of bytes
     * when combined to double bytes. The next bit specifies the order
     * of double bytes when combined to quadrupple bytes and so on.
     *
     * At a specific level (index in the bit vector) a value of false
     * indicates that the least significant part is at the lowest
     * address in memory (ie least significant bits first or little
     * endian). A value of true indicates that the most significant
     * part is at the lowest address in memory.
     *
     * You may always pass a vector with fewer elements than the
     * number of levels of byte combination on the platform. Any
     * required endianness bit not explicitly specified shall be taken
     * to be equal to the last one specified in the vector. So for
     * example, the easiest way of specifying "clean cut" little
     * endianness is by a vector with one element whose value is
     * false.
     *
     * An entirely empty vector may be passed to indicate that the
     * native endianness applies.
     *
     * \param a, b First and second endianness of the comparison. An
     * empty vector denotes the native endianness.
     *
     * \param levels The number of levels to compare starting at the
     * level where bytes are paired. A negative value indicates that
     * all levels must match.
     *
     * \sa native_endianness
     */
    bool compare_endianness(std::vector<bool> const &a,
                            std::vector<bool> const &b = std::vector<bool>(),
                            int levels = -1);


    /**
     * A representation of the native endianness of the platform.
     *
     * The number of elements in this vector is always equal
     * the number of times you need to double the width of a byte to
     * get the width of \c uintmax_t (an integer with the widest range
     * possible on your platform). This number is equal to
     * <tt>find_most_sig_bit(sizeof(uintmax_t))</tt>.
     *
     * This vector is never empty.
     *
     * \sa compare_endianness
     */
    extern std::vector<bool> const native_endianness;

    /**
     * A representation of little endianness.
     *
     * If the platform is "clean cut" little endian, then this vector
     * is empty, otherwise it is a vector with a single element whose
     * value is false.
     *
     * \sa compare_endianness
     */
    extern std::vector<bool> const little_endianness;

    /**
     * A representation of big endianness.
     *
     * If the platform is "clean cut" big endian, then this vector
     * is empty, otherwise it is a vector with a single element whose
     * value is true.
     *
     * \sa compare_endianness
     */
    extern std::vector<bool> const big_endianness;

    extern bool const is_clean_endian; ///< Is true iff (is_little_endian or is_big_endian).
    extern bool const is_little_endian;
    extern bool const is_big_endian;


    /**
     * Determine the byte permutation needed to simulate the specified
     * endianness. This is always a symmetric map.
     *
     * \param endianness An endianness description as accepted by
     * <tt>compare_endianness</tt>.
     *
     * \return A vector V with sizeof(T) byte indices. A byte
     * originally at index I must be moved to index V[I].
     *
     * \param levels Indicate the number of times you would double the
     * width of a byte to get the width of a word of the desired
     * type. In other words, it is the value of
     * <tt>find_most_sig_bit(sizeof(T))</tt>.
     *
     * \todo FIXME: It should be utilized that the byte permutation \c
     * perm(i) can be computed simply as <tt>(native_endianness ^
     * endianness) ^ i</tt> where \c native_endianness and \c
     * endianness are direct representation of the bits of the
     * corresponding endianness vectors such that the first bit of the
     * vector corresponds to the position of least significance.
     */
    std::vector<int> compute_byte_perm(std::vector<bool> const &endianness, int levels);


    /**
     * Read from a permuted sequence of bytes a value of some
     * primitive numeric type whose size in number of bytes is a power
     * of two.
     *
     * This has particular relevance when simulating some alien
     * endianness.
     *
     * \sa readWithSpecificEndianness
     *
     * \param memory A pointer to the source memory holding the
     * sizeof(T) bytes.
     *
     * \param bytePermutation A byte permutation as returned by
     * <tt>compute_byte_perm</tt>. If the number of element in this
     * permutation differ from \c sizeof(T) the result is undefined.
     *
     * \return The read value.
     *
     * \note Please make sure you never use this function when the
     * endianness of data is actually equal to the native endianness of
     * your achitecture. In this case you can access the data
     * conventionally which will perform vastly better.
     */
    template<class T>
    T readWithBytePermutation(T const *memory, std::vector<int> const &bytePermutation);


    /**
     * Write to a permuted sequence of bytes a value of some primitive
     * numeric type whose size in number of bytes is a power of two.
     *
     * This has particular relevance when simulating some alien
     * endianness.
     *
     * \sa writeWithSpecificEndianness
     *
     * \param v The value to be written.
     *
     * \param memory A pointer to the target memory with space for at
     * least sizeof(T) bytes.
     *
     * \param bytePermutation A byte permutation as returned by
     * <tt>compute_byte_perm</tt>. If the number of element in this
     * permutation differ from \c sizeof(T) the result is undefined.
     *
     * \note Please make sure you never use this function when the
     * endianness of data is actually equal to the native endianness of
     * your achitecture. In this case you can access the data
     * conventionally which will perform vastly better.
     */
    template<class T>
    void writeWithBytePermutation(T v, T *memory, std::vector<int> const &bytePermutation);


    /**
     * Read from a sequence of bytes a value of some primitive numeric
     * type whose size in number of bytes is a power of two while
     * simulating the specified endianness.
     *
     * \param memory A pointer to the source memory holding the
     * sizeof(T) bytes.
     *
     * \param endianness An endianness description as accepted by
     * <tt>compare_endianness</tt>.
     *
     * \return The read value.
     *
     * \note If you need to access many data elements this function
     * will perform horribly since it computes the native endianness
     * and the byte permutation every time it is called. Considder
     * precalculating the byte permutation with \c compute_byte_perm
     * the calling \c readWithBytePermutation afterwards.
     */
    template<class T>
    T readWithSpecificEndianness(T const *memory, std::vector<bool> const &endianness);


    /**
     * Write to a sequence of bytes a value of some primitive numeric
     * type whose size in number of bytes is a power of two while
     * simulating the specified endianness.
     *
     * \param v The value to be written.
     *
     * \param memory A pointer to the target memory with space for at
     * least sizeof(T) bytes.
     *
     * \param endianness An endianness description as accepted by
     * <tt>compare_endianness</tt>.
     *
     * \note If you need to access many data elements this function
     * will perform horribly since it computes the native endianness
     * and the byte permutation every time it is called. Considder
     * precalculating the byte permutation with \c compute_byte_perm
     * the calling \c readWithBytePermutation afterwards.
     */
    template<class T>
    void writeWithSpecificEndianness(T v, T *memory, std::vector<bool> const &endianness);







    // Implementation:

    template<class I> inline typename std::iterator_traits<I>::difference_type
    repeat(I iter, typename std::iterator_traits<I>::difference_type n,
           typename std::iterator_traits<I>::difference_type m)
    {
      return m < 0 ? repeatForward(std::reverse_iterator<I>(iter), -n, -m) :
        repeatForward(iter, n, m);
    }

    template<class I> inline typename std::iterator_traits<I>::difference_type
    repeatForward(I iter, typename std::iterator_traits<I>::difference_type n,
                  typename std::iterator_traits<I>::difference_type m)
    {
      I iter2 = iter;
      if(n < 0)
      {
        n = -n;
        iter -= n;
      }
      else iter2 += n;
      while(n < m)
      {
        iter2 = std::copy(iter, iter2, iter2);
        m -= n;
        n <<= 1;
      }
      std::copy(iter, iter+m, iter2);
      return m;
    }



    template<class T>
    inline T readWithBytePermutation(T const *memory, std::vector<int> const &byte_perm)
    {
      T v;
      char *const p = reinterpret_cast<char *>(&v);
      char const *const q = reinterpret_cast<char const *>(memory);
      for(int i=0; i<static_cast<int>(sizeof(T)); ++i) p[byte_perm[i]] = q[i];
      return v;
    }

    template<class T>
    inline void writeWithBytePermutation(T v, T *memory, std::vector<int> const &byte_perm)
    {
      char const *const p = reinterpret_cast<char const *>(&v);
      char *const q = reinterpret_cast<char *>(memory);
      for(int i=0; i<static_cast<int>(sizeof(T)); ++i) q[i] = p[byte_perm[i]];
    }

    template<class T>
    inline T readWithSpecificEndianness(T const *memory, std::vector<bool> const &endianness)
    {
      std::vector<int> byte_perm =
        compute_byte_perm(endianness, find_most_sig_bit(sizeof(T)));
      return readWithBytePermutation<T>(memory, byte_perm);
    }

    template<class T>
    inline void writeWithSpecificEndianness(T v, T *memory, std::vector<bool> const &endianness)
    {
      std::vector<int> byte_perm =
        compute_byte_perm(endianness, find_most_sig_bit(sizeof(T)));
      writeWithBytePermutation<T>(v, memory, byte_perm);
    }
  }
}

#endif // ARCHON_CORE_MEMORY_HPP
