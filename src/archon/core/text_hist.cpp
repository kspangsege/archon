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
#include <sstream>
#include <iostream>

#include <archon/core/term.hpp>
#include <archon/core/text_hist.hpp>

using namespace std;


namespace
{
  struct LinePrinter
  {
    void print(double a, double b, unsigned long n)
    {
      if(integer)
      {
        out.width(field_width);
        out << int(a) << " |";
      }
      else
      {
        out.width(field_width);
        out << a << "; ";
        out.width(field_width);
        out << b << " |";
      }
      out.fill('#');
      // FIXME: We really should have used
      // util::frac_adjust_denom(hist[i], max_count+1, max_bar+1)
      double const bar_scale  = 0 < max_count ? double(max_bar+1) / max_count : 0;
      out.width(min(int(n * bar_scale), max_bar));
      out << "";
      out.fill(' ');
      out << "\n";
    }

    LinePrinter(bool i, unsigned long c, int b, int w, ostream &o):
      integer(i), max_count(c), max_bar(b), field_width(w), out(o) {}

  private:
    bool const integer;
    unsigned long const max_count;
    int const max_bar;
    int const field_width;
    ostream &out;
  };
}


namespace archon
{
  namespace core
  {
    void Histogram::print(ostream &out, bool include_under_over, int max_width)
    {
      if(max_width < 0)
      {
        try
        {
          pair<int, int> const term_size = Term::get_terminal_size();
          max_width = term_size.first;
        }
        catch(Term::NoTerminalException &)
        {
          max_width = 80;
        }
      }

      int const n = bins.size();
      unsigned long max_count = max(under, over);
      for(int i=0; i<n; ++i)
      {
        unsigned long v = bins[i];
        if(max_count < v) max_count = v;
      }

      int const position    = integer ? 0 : floor(log10(max(abs(begin), abs(end))));
      int const precision   = ceil(log10(bins.size()) + 0.5);
      int const field_width = integer ? 1 + ceil(log10(bins.size())) : 3 + precision;

      // -0.467; +8.654 |#####
      // 01234567890123456789
      int const lead    = integer ? 2 + field_width : 4 + 2*field_width;
      int const max_bar = max(0, max_width - lead);

      double const val_scale  = pow(10.0, double(-position));

      ostringstream o;
      o.setf(ios_base::fixed, ios_base::floatfield);
      o.precision(precision);

      LinePrinter printer(integer, max_count, max_bar, field_width, o);

      double a = begin * val_scale;
      if(include_under_over) printer.print(-INFINITY, a, under);
      for(int i=0; i<n; ++i)
      {
        double const b = (begin + (i+1)/num_bins_float * full_width) * val_scale;
        printer.print(a, b, bins[i]);
        a = b;
      }
      if(include_under_over) printer.print(a, INFINITY, over);

      if(position != 0) o << "NOTE: Value scale is 1*10^"<<position<<"\n";

      out << o.str() << flush;
    }
  }
}
