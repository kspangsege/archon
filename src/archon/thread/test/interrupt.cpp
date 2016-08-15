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

#include <iostream>

#include <archon/thread/thread.hpp>

using namespace std;
using namespace archon::Core;
using namespace archon::Thread;

namespace
{
  struct T: Thread
  {
    void main()
    {
      try
      {
        Thread::sleep(10);
      }
      catch(InterruptException &)
      {
        cerr << "INTERRUPTED\n";
        throw;
      }
    }
  };
}


int main() throw()
{
  CntRef<T> t(new T);
  Thread::start(t);

  for(int i=0; i<3; ++i)
  {
    Thread::sleep(1);
    cerr << "--" << i << "--\n";
  }

  t->interrupt();
  cerr << "INTERRUPTION REQUESTED\n";
	
  t->wait();
  cerr << "TERMINATED\n";

  return 0;
}
