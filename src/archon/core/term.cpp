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

#include <archon/platform.hpp>
#include <archon/core/term.hpp>

#if ARCHON_HAVE_TERM_SIZE
#  include <cerrno>
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <fcntl.h>
#  include <unistd.h>
#  include <termios.h>
#  include <sys/ioctl.h>
#  include <archon/core/sys.hpp>
#endif


using namespace std;
using namespace Archon::Core;


namespace Archon
{
  namespace Core
  {
    namespace Term
    {
#if ARCHON_HAVE_TERM_SIZE

      pair<int, int> get_terminal_size()
      {
	int tty = open("/dev/tty", O_RDONLY);
	if(tty < 0)
        {
          int errnum = errno;
	  throw NoTerminalException("Could not open "
                                    "/dev/tty: "+Sys::error(errnum));
        }
	winsize s;
	if(ioctl(tty, TIOCGWINSZ, reinterpret_cast<char *>(&s)) < 0)
        {
          int errnum = errno;
	  throw runtime_error("get_terminal_size(): Could not do "
                              "TIOCGWINSZ on /dev/tty: "+Sys::error(errnum));
        }
	if(close(tty) < 0)
        {
          int errnum = errno;
	  throw runtime_error("get_terminal_size(): Could close "
                              "/dev/tty: " + Sys::error(errnum));
        }
        return pair<int,int>(s.ws_col, s.ws_row);
      }

#else // ARCHON_HAVE_TERM_SIZE

      pair<int, int> get_terminal_size()
      {
        return make_pair(80, 25);
      }

#endif // ARCHON_HAVE_TERM_SIZE

/*

Possible implementation on Windows using MinGW (http://rosettacode.org)

	HANDLE console;
	CONSOLE_SCREEN_BUFFER_INFO info;
	short rows;
	short columns;
	// Create a handle to the console screen.
	console = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE,
	    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
	    0, NULL);
	if (console == INVALID_HANDLE_VALUE)
		return 1;

	// Calculate the size of the console window.
	if (GetConsoleScreenBufferInfo(console, &info) == 0)
		return 1;
	CloseHandle(console);
	columns = info.srWindow.Right - info.srWindow.Left + 1;
	rows = info.srWindow.Bottom - info.srWindow.Top + 1;    }
*/
    }
  }
}
