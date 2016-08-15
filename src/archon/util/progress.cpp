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

#include <cmath>
#include <algorithm>
#include <utility>
#include <sstream>

#include <archon/core/functions.hpp>
#include <archon/core/term.hpp>
#include <archon/util/unit_frac.hpp>
#include <archon/util/progress.hpp>


using namespace std;
using namespace archon::core;
using namespace archon::Util;


namespace
{
  string format_time(long seconds)
  {
    ostringstream o;
    o.fill('0');
    ldiv_t const r1 = div(seconds, 60L);
    if(r1.quot != 0)
    {
      ldiv_t const r2 = div(r1.quot, 60L);
      if(r2.quot != 0)
      {
        o << r2.quot<<"h";
        o.width(2);
      }
      o << r2.rem<<"m";
      o.width(2);
    }
    o << r1.rem<<"s";
    return o.str();
  }
}


namespace archon
{
  namespace Util
  {
    ProgressBar::ProgressBar(int width, string prefix, ostream &out):
      prefix(prefix), out(out), start_time(Time::now())
    {
      if(width < 0)
      {
        try
        {
          pair<int, int> const term_size = Term::get_terminal_size();
          width = term_size.first;
        }
        catch(Term::NoTerminalException &)
        {
          width = 80;
        }
      }
      this->width = width;
      progress(0,0);
    }


    ProgressBar::~ProgressBar()
    {
      progress(1,0);
      out << endl;
    }


    void ProgressBar::progress(double frac, double seconds_left)
    {
      // "Prefix: [########----------] 2m16s / 5m06s"
      Time const time = Time::now() - start_time;
      long const seconds = time.get_as_seconds();
      string const time1 = format_time(seconds);
      double const total = seconds + seconds_left;
      string const time2 = format_time(clamp_float_to_int<long>(total));
      int const max_bar = max(width - int(prefix.size() + time1.size() + time2.size()) - 7, 0);
      int const bar = frac_float_to_int(frac, max_bar+1);
      ostringstream o;
      o << "\r";
      o << prefix << "[";
      o.fill('#');
      o.width(bar);
      o << "";
      o.fill('-');
      o.width(max_bar - bar);
      o << "" << "] " << time1 << " / " << time2;
      out << o.str() << flush;
    }
  }
}
