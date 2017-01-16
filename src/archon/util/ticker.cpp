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

/// \file
///
/// \author Kristian Spangsege

#include <sstream>

#include <archon/core/functions.hpp>
#include <archon/core/term.hpp>
#include <archon/util/unit_frac.hpp>
#include <archon/util/ticker.hpp>


using namespace archon::core;


namespace archon {
namespace util {

AdaptiveTicker::AdaptiveTicker(long millis_per_out_tick,
                               int checks_per_out_tick):
    m_out_tick_threshold_millis{long(std::ceil(double(2*checks_per_out_tick-1)/
                                               checks_per_out_tick/2*millis_per_out_tick))},
    m_millis_per_check{millis_per_out_tick/checks_per_out_tick}
{
}


bool AdaptiveTicker::check()
{
    if (!m_in_ticks_per_check) {
        // First tick, just initialize, and tick
        m_time_of_last_check = Time::now();
        m_in_ticks_per_check = m_in_ticks_before_check = 1;
        m_next_out_tick_threshold_time.set_as_millis(m_out_tick_threshold_millis);
        m_next_out_tick_threshold_time += m_time_of_last_check;
        return true;
    }

    m_accum_in_ticks += m_in_ticks_per_check;

    Time now = Time::now();
    Time time = now - m_time_of_last_check;
    m_time_of_last_check = now;

    double millis_since_last_check = time.get_as_millis();
    if (!millis_since_last_check)
        millis_since_last_check = time.get_as_nanos()/1000000.0;

    m_millis_per_in_tick.add(millis_since_last_check / m_in_ticks_per_check);
    double millis_per_in_tick_estimate = m_millis_per_in_tick.get();

    double n;
    if (millis_per_in_tick_estimate) {
        n = m_millis_per_check / millis_per_in_tick_estimate;
    }
    else {
        // No estimate, so raise 'm_in_ticks_per_check' progressively
        n = m_in_ticks_per_check << 1;
    }

    // Allow it to rise by at most a factor of 1.7, fall by at most a factor of
    // 1.9, and clamp to [1,1E9].
    m_in_ticks_before_check = m_in_ticks_per_check =
        clamp<long>(std::round(clamp<double>(n, m_in_ticks_per_check/1.9,
                                             (m_in_ticks_per_check == 1 ? 2 :
                                              1.7*m_in_ticks_per_check))), 1, 1000000000L);

    if (m_next_out_tick_threshold_time <= now) {
        m_next_out_tick_threshold_time.set_as_millis(m_out_tick_threshold_millis);
        m_next_out_tick_threshold_time += m_time_of_last_check;
        m_accum_in_ticks_copy = m_accum_in_ticks;
        m_accum_in_ticks = 0;
        return true;
    }

    return false;
}




RateMeter::RateMeter(std::string prefix, long millis_per_report, std::ostream& out):
    AdaptiveTicker{millis_per_report},
    m_prefix{prefix},
    m_out{out}
{
}


void RateMeter::update()
{
    Time now = Time::now();
    if (m_time_of_last_update) {
        Time elapsed = now - m_time_of_last_update;
        std::ostringstream o;
        o << m_prefix << get_num_in_ticks() / elapsed.get_as_seconds_float() << "\n";
        m_out << o.str() << std::flush;
    }
    m_time_of_last_update = now;
}




ProgressTicker::ProgressTicker(ProgressTracker* t, long expected_num_ticks,
                               long millis_per_update):
    AdaptiveTicker{t ? millis_per_update : 10000},
    m_tracker{t},
    m_expected_num_ticks{expected_num_ticks},
    m_start_time{t ? Time::now() : Time{}}
{
}


void ProgressTicker::update()
{
    if (m_tracker && m_accum_ticks < m_expected_num_ticks) {
        m_accum_ticks += std::min(m_expected_num_ticks - m_accum_ticks, get_num_in_ticks());
        m_tracker->progress(frac_int_to_float<long, double>(m_accum_ticks, m_expected_num_ticks+1),
                            (m_expected_num_ticks-m_accum_ticks) * 1e-3 *
                            get_est_millis_per_in_tick());
    }
}

} // namespace util
} // namespace archon
