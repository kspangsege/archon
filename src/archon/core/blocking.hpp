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
 * Defines a number of tools taht are helpfull when dealing with
 * blocking conditions, that is, operation which are capable of
 * blocking (or suspending) the calling process/thread.
 */

#ifndef ARCHON_CORE_BLOCKING_HPP
#define ARCHON_CORE_BLOCKING_HPP

#include <exception>


namespace archon
{
  namespace Core
  {
    /**
     * Some operations are capable of blocking the caller for extended
     * periods of time while waiting for some condition to be
     * satisfied, for example, the arrival of data over the network,
     * or a worker thread that is waiting for a job submission that
     * even may never occur.
     *
     * This exception is thrown by such operations if a blocked caller
     * is interrupted.
     *
     * A process or thread can be interrupted by another party
     * (process/thread) as a request to abort any indefinate/unbounded
     * waiting.
     */
    struct InterruptException: std::exception {};
  }
}

#endif // ARCHON_CORE_BLOCKING_HPP
