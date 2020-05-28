// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


/// \file

#ifndef ARCHON__BASE__TIME_HPP
#define ARCHON__BASE__TIME_HPP

#include <ctime>
#include <chrono>

#include <archon/base/features.h>
#include <archon/base/assert.hpp>


namespace archon::base {


/// \brief Break down time point as local time.
///
/// Break down the specified point in time (seconds since the Epoch) with
/// respect to the local time zone. This is a thread-safe version of
/// `std::localtime()`. It probably uses `localtime_r()` on POSIX platforms.
///
std::tm time_breakdown_local(std::time_t);


/// \brief Break down time point as UTC.
///
/// Break down the specified point in time (seconds since the Epoch) with
/// respect to UTC (Coordinated Universal Time). This is a thread-safe version
/// of `std::gmtime()`. It probably uses `gmtime_r()` on POSIX platforms.
///
std::tm time_breakdown_utc(std::time_t);


#if ARCHON_LLVM_LIBCXX && ARCHON_APPLE
using timespec_type = ::timespec; // Due to bug in LLVM libc++ (LLVM 10.0) on macOS
#else
using timespec_type = std::timespec;
#endif


/// \brief Convert time point to `std::timespec`.
///
/// Convert an object of type `std::chrono::time_point` to an object of type
/// `std::timespec`.
///
template<class C> timespec_type time_point_to_timespec(std::chrono::time_point<C>) noexcept;








// Implementation


template<class C>
inline timespec_type time_point_to_timespec(std::chrono::time_point<C> time) noexcept
{
    using clock_type = C;
    using time_point_type = std::chrono::time_point<C>;
    std::time_t time_2 = clock_type::to_time_t(time);
    time_point_type time_3 = clock_type::from_time_t(time_2);
    if (ARCHON_UNLIKELY(time_3 > time)) {
        --time_2;
        time_3 = clock_type::from_time_t(time_2);
    }
    long nanoseconds =
        long(std::chrono::duration_cast<std::chrono::nanoseconds>(time - time_3).count());
    ARCHON_ASSERT(nanoseconds >= 0 && nanoseconds < 1000000000);
    return { time_2, nanoseconds };
}


} // namespace archon::base

#endif // ARCHON__BASE__TIME_HPP
