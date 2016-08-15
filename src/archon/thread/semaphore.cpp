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

#include <archon/thread/semaphore.hpp>


using namespace archon::core;


namespace archon
{
  namespace thread
  {
    void Semaphore::down()
    {
      Mutex::Lock l(mutex);
      while(value == 0) non_zero.wait();
      --value;
    }

    void Semaphore::up()
    {
      Mutex::Lock l(mutex);
      ++value;
      l.release();
      non_zero.notify_all();
    }
  }
}
