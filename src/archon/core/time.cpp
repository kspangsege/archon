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

#include <sstream>
#include <iomanip>




#include <iostream>




#include <archon/platform.hpp>
#include <archon/core/time.hpp>

#ifdef ARCHON_HAVE_LIBREALTIME
#else
#include <cerrno>
#include <stdexcept>
#include <sys/time.h>
#include <archon/core/sys.hpp>
#endif


using namespace std;


namespace Archon
{
  namespace Core
  {
    Time Time::now()
    {
#ifdef ARCHON_HAVE_LIBREALTIME
      timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts);
      return Time(ts.tv_sec , ts.tv_nsec);
#else // ARCHON_HAVE_LIBREALTIME
      ::timeval tv;
      int const r = gettimeofday(&tv, 0);
      if(r != 0) {
        int const errnum = errno;
        throw runtime_error("'gettimeofday' failed: "+Sys::error(errnum));
      }
      return Time(tv.tv_sec, tv.tv_usec*1000);
#endif // ARCHON_HAVE_LIBREALTIME
    }


    string Time::format_rfc_1123()
    {
      static const char *day[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
      static const char *month[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                     "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

      struct tm u;
      // gmtime_r is specified by POSIX.1-2001
      gmtime_r(&secs, &u);
      int y = (1900 + u.tm_year) % 10000; // Force year into 4 digits
      ostringstream o;
      o << setfill('0') << day[u.tm_wday] << ", " << setw(2) << u.tm_mday
        << " " << month[u.tm_mon] << " " << setw(4) << y << " " << setw(2)
        << u.tm_hour << ":" << setw(2) << u.tm_min << ":" << setw(2)
        << u.tm_sec << " GMT";
      return o.str();
    }
  }
}
