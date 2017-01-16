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

#ifndef ARCHON_UTIL_TICKER_HPP
#define ARCHON_UTIL_TICKER_HPP

#include<string>
#include<iostream>

#include <archon/core/time.hpp>
#include <archon/util/statistics.hpp>
#include <archon/util/progress.hpp>


namespace archon {
namespace util {

/// The purpose of this class is to produce "out ticks" at a specific rate
/// (ticks per second) given a reasonably regular rate of "in ticks". For
/// efficiency reasons, it does this by skipping a certain number of "in ticks",
/// and the challenge for this class is to predict in advance how many "in
/// ticks" to skip. It attempts continously to adapt itself, by measuring the
/// rate of "in ticks".
///
/// An "in tick" is defined as a call to the tick() method. An "out tick" is
/// defined as a call to the tick() method that returns true.
///
/// The tick() method is intended to be called at each iteration step in a
/// lengthy loop. Due to the fact that the execution of the tick() method on
/// average is very efficient, the call can be placed even in inner loops that
/// need to execute very efficiently.
///
/// Due to its ability to continuously adapt itself to the rate of "in ticks",
/// this class is especially usefull when the execution time of individual
/// iteration steps vary over time, or when the execution time of iteration
/// steps vary significantly from one execution of the loop to another.
///
/// Due to its adaptive nature, it should be expected that the rate of "out
/// ticks" is quite iregular at first, but it should be expected to stabilize
/// quickly, assuming that the rate of "in ticks" does not vary too wildly.
///
/// One case where this class is very usefull is when you want to report
/// progress of a lengthy computation having the form of a loop.
class AdaptiveTicker {
public:
    AdaptiveTicker(long millis_per_out_tick = 1000,
                   int checks_per_out_tick = 4);

    bool tick();

    /// Get the number of "in ticks" seen between the last out tick and the out
    /// tick before that.
    long get_num_in_ticks() const;

    /// Get the estimated number of milliseconds per in tick.
    double get_est_millis_per_in_tick() const;

private:
    const long m_out_tick_threshold_millis, m_millis_per_check;
    long m_in_ticks_per_check = 0, m_in_ticks_before_check = 1;
    long m_accum_in_ticks = 0, m_accum_in_ticks_copy = 1;
    util::WeightedMovingAverage<double, 10> m_millis_per_in_tick;
    core::Time m_time_of_last_check, m_next_out_tick_threshold_time;

    bool check();
};



class RateMeter: public AdaptiveTicker {
public:
    RateMeter(std::string prefix = "Rate: ", long millis_per_report = 1000,
              std::ostream& out = std::cout);

    void tick();

private:
    void update();

    const std::string m_prefix;
    std::ostream& m_out;
    core::Time m_time_of_last_update;
};



class ProgressTicker: public AdaptiveTicker {
public:
    /// \param width The number of characters that should be used to render the
    /// progress bar as a line of text. A negative value corresponds to the
    /// width of the terminal, or 80 if there is no terminal.
    ///
    /// \param tracker If you pass null, the ticker will be in a disabled state,
    /// where nothing happens regardless of how many times or for how long the
    /// tick() method is called.
    ProgressTicker(ProgressTracker* tracker, long expected_num_ticks,
                   long millis_per_update = 100);

    void tick();

private:
    void update();

    ProgressTracker* const m_tracker;
    const long m_expected_num_ticks;
    const core::Time m_start_time;
    long m_accum_ticks = 0;
};




// Implementation

inline bool AdaptiveTicker::tick()
{
    return !--m_in_ticks_before_check && check();
}

inline long AdaptiveTicker::get_num_in_ticks() const
{
    return m_accum_in_ticks_copy;
}

inline double AdaptiveTicker::get_est_millis_per_in_tick() const
{
    return m_millis_per_in_tick.get();
}

inline void RateMeter::tick()
{
    if (AdaptiveTicker::tick())
        update();
}

inline void ProgressTicker::tick()
{
    if (AdaptiveTicker::tick())
        update();
}

} // namespace util
} // namespace archon

#endif // ARCHON_UTIL_TICKER_HPP
