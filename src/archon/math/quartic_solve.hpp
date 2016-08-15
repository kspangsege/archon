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

#ifndef ARCHON_MATH_QUARTIC_SOLVE_HPP
#define ARCHON_MATH_QUARTIC_SOLVE_HPP


namespace archon
{
  namespace math
  {
    /**
     * Solve the general 4th order equation:
     *
     * <pre>
     *
     *   x^4 + a x³ + b x² + c x + d = 0
     *
     * </pre>
     *
     * The number of roots will always be a multiple of 2. First root
     * will be in \c roots[0] etc.
     *
     * \return The number of roots.
     *
     * \note This function is thread safe.
     */
    int quartic_solve(double a, double b, double c, double d,
                      double roots[4], double root_errors[4] = 0);
  }
}

#endif // ARCHON_MATH_QUARTIC_SOLVE_HPP
