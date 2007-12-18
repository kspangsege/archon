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

#ifndef ARCHON_CORE_TEXT_HIST_HPP
#define ARCHON_CORE_TEXT_HIST_HPP

#include <limits>
#include <vector>
#include <iosfwd>


namespace Archon
{
  namespace Core
  {
    struct Histogram
    {
      Histogram(double begin, double end, int n):
        begin(begin), end(end), num_bins_float(n),
        full_width(end-begin), bin_freq(num_bins_float/full_width),
        integer(false), bins(n), under(0), over(0) {}


      Histogram(int begin, int end):
        begin(begin), end(end), num_bins_float(end-begin),
        full_width(num_bins_float), bin_freq(1),
        integer(true), bins(end-begin), under(0), over(0) {}


      void add(double v)
      {
        // The main challenge here is to correcly map values from a
        // floating point domain to an integer domain while keeping
        // accurate count of which values fall below, and which fall
        // above the main interval, and without inadvertently
        // modifying a value such that it falls in a wrong bin.
        if(v < begin) ++under;
        else
        {
          double const w = (v-begin) * bin_freq;
          if(double(std::numeric_limits<unsigned>::max()) <= w) ++over;
          else
          {
            unsigned const i = unsigned(w);
            if(num_bins_float <= i) ++over;
            else ++bins[i];
          }
        }
      }


      unsigned long get_bin_count(int i) const { return bins.at(i); }
      unsigned long get_under_count() const { return under; }
      unsigned long get_over_count() const { return over; }


      /**
       * Print out a textual rendering of the histogram with
       * horizontal bars.
       *
       * \param max_width The maximum length in number of characters
       * of the generated lines of text. If a negative value is
       * specified, the width will be set to match the width of the
       * associated terminal, or to 80 if there is no terminal.
       */
      void print(std::ostream &out, bool include_under_over = false, int max_width = -1);


    private:
      double const begin, end;
      double const num_bins_float;
      double const full_width, bin_freq;
      bool integer;
      std::vector<unsigned long> bins;
      unsigned long under, over;
    };
  }
}

#endif // ARCHON_CORE_TEXT_HIST_HPP
