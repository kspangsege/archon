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

#if __GNUC__ < 3 || __GNUC__ == 3 && __GNUC_MINOR__ < 2
#else
#include <cxxabi.h>
#endif

#include <cstdlib>
#include <iostream>

#include <archon/core/cxx.hpp>

using namespace std;

namespace archon
{
  namespace Core
  {
    namespace Cxx
    {
      /**
       * \sa http://gcc.gnu.org/onlinedocs/libstdc++/latest-doxygen/namespaceabi.html
       *
       * \todo FIXME: Should utilize the Autoconf macro
       * 'ax_cxx_gcc_abi_demangle'. See
       * http://autoconf-archive.cryp.to.
       */
      string demangle(string n)
      {
#if __GNUC__ < 3 || __GNUC__ == 3 && __GNUC_MINOR__ < 2
        return n;
#else
	int s = 0;
	char *r = abi::__cxa_demangle(n.c_str(), 0, 0, &s);
	if(!r) return n;
	string m = r;
	free(r);
	return m;
#endif
      }


      void terminate_handler()
      {
	try { throw; }
	catch (exception &e) {
	  cerr << "terminate called after throwing an instance of '" << type(e) << "'" << endl;
	  cerr << "  what(): " << e.what() << endl;
	}
	catch (...) {
	  cerr << "terminate called after throwing an instance of  an unknown type" << endl;
	}
	abort();
      }
    }
  }
}
