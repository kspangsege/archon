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

#include <archon/core/functions.hpp>
#include <archon/core/term.hpp>
#include <archon/util/unit_frac.hpp>
#include <archon/util/ticker.hpp>


using namespace std;
using namespace archon::Core;


namespace archon
{
  namespace Util
  {
    AdaptiveTicker::AdaptiveTicker(long millis_per_out_tick,
                                   int checks_per_out_tick):
      out_tick_threshold_millis(ceill(double(2*checks_per_out_tick-1)/
                                      checks_per_out_tick/2*millis_per_out_tick)),
      millis_per_check(millis_per_out_tick/checks_per_out_tick),
      in_ticks_per_check(0), in_ticks_before_check(1),
      accum_in_ticks(0), accum_in_ticks_copy(1) {}


    bool AdaptiveTicker::check()
    {
      if(!in_ticks_per_check)
      {
        // First tick, just initialize, and tick
        time_of_last_check = Time::now();
        in_ticks_per_check = in_ticks_before_check = 1;
        next_out_tick_threshold_time.set_as_millis(out_tick_threshold_millis);
        next_out_tick_threshold_time += time_of_last_check;
        return true;
      }

      accum_in_ticks += in_ticks_per_check;

      Time const now = Time::now();
      Time const time = now - time_of_last_check;
      time_of_last_check = now;

      double millis_since_last_check = time.get_as_millis();
      if(!millis_since_last_check)
        millis_since_last_check = time.get_as_nanos()/1000000.0;

      millis_per_in_tick.add(millis_since_last_check / in_ticks_per_check);
      double const millis_per_in_tick_estimate = millis_per_in_tick.get();

      double n;
      if(millis_per_in_tick_estimate) n = millis_per_check / millis_per_in_tick_estimate;
      else // No estimate, so raise 'in_ticks_per_check' progressively
        n = in_ticks_per_check << 1;

      // Allow it to rise by at most a factor of 1.7, fall by at most
      // a factor of 1.9, and clamp to [1,1E9].
      in_ticks_before_check = in_ticks_per_check =
        clamp<long>(archon_round(clamp<double>(n, in_ticks_per_check/1.9,
                                                      in_ticks_per_check==1 ? 2 :
                                                      1.7*in_ticks_per_check)), 1, 1000000000L);

      if(next_out_tick_threshold_time <= now)
      {
        next_out_tick_threshold_time.set_as_millis(out_tick_threshold_millis);
        next_out_tick_threshold_time += time_of_last_check;
        accum_in_ticks_copy = accum_in_ticks;
        accum_in_ticks = 0;
        return true;
      }

      return false;
    }




    RateMeter::RateMeter(string prefix, long millis_per_report, ostream &out):
      AdaptiveTicker(millis_per_report), prefix(prefix), out(out) {}


    void RateMeter::update()
    {
      Time const now = Time::now();
      if(time_of_last_update)
      {
        Time const elapsed = now - time_of_last_update;
        ostringstream o;
        o << prefix << get_num_in_ticks() / elapsed.get_as_seconds_float() << "\n";
        out << o.str() << flush;
      }
      time_of_last_update = now;
    }




    ProgressTicker::ProgressTicker(ProgressTracker *t, unsigned long expected_num_ticks,
                                   long millis_per_update):
      AdaptiveTicker(t ? millis_per_update : 10000), tracker(t),
      expected_num_ticks(expected_num_ticks),
      start_time(t ? Time::now() : Time()), accum_ticks(0) {}


    void ProgressTicker::update()
    {
      if(tracker && accum_ticks < expected_num_ticks)
      {
        accum_ticks += min(expected_num_ticks - accum_ticks, get_num_in_ticks());
        tracker->progress(frac_int_to_float<long, double>(accum_ticks, expected_num_ticks+1),
                          (expected_num_ticks-accum_ticks) * 1e-3 * get_est_millis_per_in_tick());
      }
    }
  }
}
