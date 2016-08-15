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

#ifndef ARCHON_UTIL_PRIME_HPP
#define ARCHON_UTIL_PRIME_HPP


namespace archon
{
  namespace Util
  {
    /**
     * Get a prime number that is greater than, or equal to the
     * specified number, but not too much greater.
     *
     * The table below shows how much greater than the argument, that
     * the returned prime number can be:
     *
     * <pre>
     *
     *     Argument range     Maximum increase
     *   ----------------------------------------
     *     170 < n         |       12%
     *      24 < n <= 170  |       16%
     *           n <= 24   |       38%
     *
     * </pre>
     *
     * The application may assume that this function is implemented as
     * a table lookup, and therefore can be considered a fast
     * operation.
     *
     * Here are some guaranteed properties of this function:
     *
     * <pre>
     *
     *   n <= f(n)
     *
     *   m <= n  ==>  f(m) <= f(n)      (monotonicity)
     *
     *   f(m) < f(n)  ==>  f(m) < n     (consistency)
     *
     * </pre>
     *
     * where 'n' and 'm' are integers and 'f()' stands for
     * get_prime_not_under().
     */
    long get_prime_not_under(long n);
  }
}

#endif // ARCHON_UTIL_PRIME_HPP
