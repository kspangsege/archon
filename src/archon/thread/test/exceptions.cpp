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

#include <stdexcept>
#include <sstream>
#include <iostream>

#include <archon/thread/thread.hpp>


using namespace std;
using namespace Archon::Thread;


namespace
{
  void my_unexpected_1() { cerr << "*unexpected 1*\n"; }
  void my_terminate_1()  { cerr << "*terminate 1*\n";  }
  void my_unexpected_2() { cerr << "*unexpected 2*\n"; }
  void my_terminate_2()  { cerr << "*terminate 2*\n";  }
  void my_unexpected_3() { cerr << "*unexpected 3*\n"; }
  void my_terminate_3()  { cerr << "*terminate 3*\n";  }
  void my_unexpected_4() { cerr << "*unexpected 4*\n"; }
  void my_terminate_4()  { cerr << "*terminate 4*\n";  }
  void my_unexpected_5() { cerr << "*unexpected 5*\n"; }
  void my_terminate_5()  { cerr << "*terminate 5*\n";  }
  void my_unexpected_6() { cerr << "*unexpected 6*\n"; }
  void my_terminate_6()  { cerr << "*terminate 6*\n";  }
  void my_unexpected_7() { cerr << "*unexpected 7*\n"; }
  void my_terminate_7()  { cerr << "*terminate 7*\n";  }
  void my_unexpected_8() { cerr << "*unexpected 8*\n"; }
  void my_terminate_8()  { cerr << "*terminate 8*\n";  }

  void thread(int i) throw()
  {
    ostringstream o;
    o << i;
    string s = o.str();

    if(i==1) { set_unexpected(my_unexpected_1); set_terminate(my_terminate_1); }
    if(i==2) { set_unexpected(my_unexpected_2); set_terminate(my_terminate_2); }
    if(i==3) { set_unexpected(my_unexpected_3); set_terminate(my_terminate_3); }
    if(i==4) { set_unexpected(my_unexpected_4); set_terminate(my_terminate_4); }
    if(i==5) { set_unexpected(my_unexpected_5); set_terminate(my_terminate_5); }
    if(i==6) { set_unexpected(my_unexpected_6); set_terminate(my_terminate_6); }
    if(i==7) { set_unexpected(my_unexpected_7); set_terminate(my_terminate_7); }
    if(i==8) { set_unexpected(my_unexpected_8); set_terminate(my_terminate_8); }

    throw runtime_error("Die: " + s);
  }
}


int main() throw()
{
  Thread::run(thread, 1);
  Thread::run(thread, 3);
  Thread::run(thread, 3);
  Thread::run(thread, 4);
  Thread::run(thread, 5);
  Thread::run(thread, 6);
  Thread::run(thread, 7);
  Thread::run(thread, 8);

  Thread::main_exit_wait();

  return 0;
}
