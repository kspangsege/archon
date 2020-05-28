// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.


#include <cerrno>
#include <ctime>
#include <limits>
#include <chrono>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/timer.hpp>
#include <archon/core/platform_support.hpp>


#if ARCHON_WINDOWS
#  define HAS_CLOCK_GETTIME 0
#else
#  include <unistd.h>
#  if defined _POSIX_TIMERS && _POSIX_TIMERS > 0
#    define HAS_CLOCK_GETTIME 1
#  else
#    define HAS_CLOCK_GETTIME 0
#  endif
#endif


using namespace archon;
using core::Timer;


#if HAS_CLOCK_GETTIME


#if defined _POSIX_MONOTONIC_CLOCK
#  if _POSIX_MONOTONIC_CLOCK > 0
#    define HAS_MONOTONIC_CLOCK 1
#  elif _POSIX_MONOTONIC_CLOCK == 0
#    define HAS_MONOTONIC_CLOCK 0
#  else
#    define HAS_MONOTONIC_CLOCK -1
#  endif
#else // !defined _POSIX_MONOTONIC_CLOCK
#  if defined _SC_MONOTONIC_CLOCK && defined CLOCK_MONOTONIC
#    define HAS_MONOTONIC_CLOCK 0
#  else
#    define HAS_MONOTONIC_CLOCK -1
#  endif
#endif // !defined _POSIX_MONOTONIC_CLOCK


#if defined _POSIX_CPUTIME
#  if _POSIX_CPUTIME > 0
#    define HAS_PROCESS_CPUTIME 1
#  elif _POSIX_CPUTIME == 0
#    define HAS_PROCESS_CPUTIME 0
#  else
#    define HAS_PROCESS_CPUTIME -1
#  endif
#else // !defined _POSIX_CPUTIME
#  if defined _SC_CPUTIME && defined CLOCK_PROCESS_CPUTIME_ID
#    define HAS_PROCESS_CPUTIME 0
#  else
#    define HAS_PROCESS_CPUTIME -1
#  endif
#endif // !defined _POSIX_CPUTIME


#if defined _POSIX_THREAD_CPUTIME
#  if _POSIX_THREAD_CPUTIME > 0
#    define HAS_THREAD_CPUTIME 1
#  elif _POSIX_THREAD_CPUTIME == 0
#    define HAS_THREAD_CPUTIME 0
#  else
#    define HAS_THREAD_CPUTIME -1
#  endif
#else // !defined _POSIX_THREAD_CPUTIME
#  if defined _SC_THREAD_CPUTIME && defined CLOCK_THREAD_CPUTIME_ID
#    define HAS_THREAD_CPUTIME 0
#  else
#    define HAS_THREAD_CPUTIME -1
#  endif
#endif // !defined _POSIX_THREAD_CPUTIME


namespace {


bool get_monotonic_clock_id(clockid_t& id) noexcept
{
#if HAS_MONOTONIC_CLOCK > 0
    id = CLOCK_MONOTONIC;
    return true;
#elif HAS_MONOTONIC_CLOCK == 0
    long ret = ::sysconf(_SC_MONOTONIC_CLOCK);
    if (ARCHON_LIKELY(ret != -1)) {
        id = CLOCK_MONOTONIC;
        return true;
    }
    return false;
#else
    return false;
#endif
}


bool get_process_cputime_id(clockid_t& id) noexcept
{
#if HAS_PROCESS_CPUTIME > 0
    id = CLOCK_PROCESS_CPUTIME_ID;
    return true;
#elif HAS_PROCESS_CPUTIME == 0
    long ret = ::sysconf(_SC_CPUTIME);
    if (ARCHON_LIKELY(ret != -1)) {
        id = CLOCK_PROCESS_CPUTIME_ID;
        return true;
    }
    return false;
#else
    return false;
#endif
}


bool get_thread_cputime_id(clockid_t& id) noexcept
{
#if HAS_THREAD_CPUTIME > 0
    id = CLOCK_THREAD_CPUTIME_ID;
    return true;
#elif HAS_THREAD_CPUTIME == 0
    long ret = ::sysconf(_SC_THREAD_CPUTIME);
    if (ARCHON_LIKELY(ret != -1)) {
        id = CLOCK_THREAD_CPUTIME_ID;
        return true;
    }
    return false;
#else
    return false;
#endif
}


} // unnamed namespace


struct Timer::Info {
    clockid_t monotonic_clock_id;
    clockid_t process_cputime_id;
    clockid_t thread_cputime_id;

    bool has_monotonic_clock = false;
    bool has_process_cputime = false;
    bool has_thread_cputime  = false;

    Info() noexcept
    {
        monotonic_clock_id = CLOCK_REALTIME; // Fallback
        if (get_monotonic_clock_id(monotonic_clock_id))
            has_monotonic_clock = true;

        process_cputime_id = monotonic_clock_id; // Fallback
        if (get_process_cputime_id(process_cputime_id))
            has_process_cputime = true;

        thread_cputime_id = process_cputime_id; // Fallback
        if (get_thread_cputime_id(thread_cputime_id))
            has_thread_cputime = true;
    }
};


auto Timer::get_info() noexcept -> const Info&
{
    static Info info;
    return info;
}


double Timer::get_elapsed_time() const
{
    TimePoint time = get_current_time(); // Throws
    ARCHON_ASSERT(time.timespec.tv_nsec >= 0 && time.timespec.tv_nsec < 1000000000);
    ARCHON_ASSERT(m_start.timespec.tv_nsec >= 0 && m_start.timespec.tv_nsec < 1000000000);
    if (ARCHON_LIKELY(time.timespec.tv_sec >= m_start.timespec.tv_sec)) {
        std::time_t sec = time.timespec.tv_sec;
        if (ARCHON_LIKELY(core::try_int_sub(sec, m_start.timespec.tv_sec))) {
            ARCHON_ASSERT(sec >= 0);
            long nsec = time.timespec.tv_nsec - m_start.timespec.tv_nsec;
            if (nsec < 0) {
                if (ARCHON_LIKELY(sec > 0)) {
                    sec -= 1;
                    nsec += 1000000000;
                    ARCHON_ASSERT(nsec > 0);
                    goto good;
                }
                return 0;
            }
          good:
            return sec + nsec / 1E9;
        }
        return double(std::numeric_limits<std::time_t>::max());
    }
    return 0;
}


bool Timer::has_monotonic_clock() noexcept
{
    return s_init.info.has_monotonic_clock;
}


bool Timer::has_process_cputime() noexcept
{
    return s_init.info.has_process_cputime;
}


bool Timer::has_thread_cputime() noexcept
{
    return s_init.info.has_thread_cputime;
}


auto Timer::get_current_time() const -> TimePoint
{
    clockid_t clock_id = m_info.monotonic_clock_id;
    switch (m_type) {
        case Type::monotonic_clock:
            break;
        case Type::process_cputime:
            clock_id = m_info.process_cputime_id;
            break;
        case Type::thread_cputime:
            clock_id = m_info.thread_cputime_id;
            break;
    }
    TimePoint time = {};
    int ret = ::clock_gettime(clock_id, &time.timespec);
    if (ARCHON_LIKELY(ret != -1))
        return time;
    int errno_2 = errno; // Avoid clobbering
    core::throw_system_error(errno_2, "clock_gettime() failed"); // Throws
}


#else // !HAS_CLOCK_GETTIME


// Fallback to using std::chrono::steady_clock for everything


struct Timer::Info {};


auto Timer::get_info() noexcept -> const Info&
{
    static Info info;
    return info;
}


double Timer::get_elapsed_time() const
{
    auto stop = std::chrono::steady_clock::now().time_since_epoch();
    auto start = std::chrono::steady_clock::duration(m_start.duration);
    if (ARCHON_LIKELY(stop >= start))
        return std::chrono::duration<double>(stop - start).count();
    return 0;
}


bool Timer::has_monotonic_clock() noexcept
{
    return true;
}


bool Timer::has_process_cputime() noexcept
{
    return false;
}


bool Timer::has_thread_cputime() noexcept
{
    return false;
}


auto Timer::get_current_time() const -> TimePoint
{
    TimePoint time = {};
    time.duration = std::chrono::steady_clock::now().time_since_epoch().count();
    return time;
}


#endif // !HAS_CLOCK_GETTIME
