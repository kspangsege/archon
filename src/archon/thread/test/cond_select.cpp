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

#include <unistd.h>
#include <stdexcept>
#include <string>
#include <iostream>

#include <archon/core/sys.hpp>
#include <archon/thread/thread.hpp>

using namespace std;
using namespace archon::core;
using namespace archon::thread;

namespace
{
  struct T: Thread
  {
    Mutex mux;
    Condition cond;

    T(): cond(mux) {}
        
    void main()
    {
      try
      {
        size_t const buf_size = 1024;
        char buf[buf_size];

        SelectSpec spec;
        spec.read_in.insert(STDIN_FILENO);
        Time timeout = Time::now();
        Mutex::Lock l(mux);
        for(;;)
        {
          timeout += 4;
          for(;;)
          {
            bool const timed_out = cond.select(spec, timeout);
            if(timed_out)
            {
              cerr << "TIMED OUT\n";
              break;
            }
            if(spec.read_out.find(STDIN_FILENO) != spec.read_out.end())
            {
              ssize_t n = read(STDIN_FILENO, buf, buf_size);
              if(n < 0)
              {
                int const errnum = errno;
                throw runtime_error("Read from STDIN failed: "+Sys::error(errnum));
              }
              if(!n)
              {
                cerr << "QUIT\n";
                return;
              }
              cerr << "> " << string(buf, n);
            }
            else cerr << "SPURIOUS WAKE-UP\n";
          }
        }
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

  for(int i=0; i<10; ++i)
  {
    Thread::sleep(1);
    cerr << "--" << (i+1) << "--\n";
  }

  t->interrupt();
  cerr << "INTERRUPTION REQUESTED\n";
        
  t->wait();
  cerr << "TERMINATED\n";

  return 0;
}
