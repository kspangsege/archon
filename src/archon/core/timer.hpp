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

#ifndef ARCHON_X_CORE_X_TIMER_HPP
#define ARCHON_X_CORE_X_TIMER_HPP

/// \file


#include <chrono>
#include <ostream>

#include <archon/core/time.hpp>
#include <archon/core/format_as.hpp>


namespace archon::core {


/// \brief A device for measuring elapsed time.
///
/// This class implements a timer device that can be used to measure elapsed time. Here is
/// how you can use it:
///
/// \code{.cpp}
///
///   archon::core::Timer timer;
///   // Do stuf here ...
///   std::cout << "Elapsed time: " << timer << "\n";
///
/// \endcode
///
/// Be carefult, however. In the example above, we write `"Elapsed time: "` to the output
/// stream before reading the stop time off of the clock, and writing to a stream can be
/// relatively slow. Therefore, if you are measuring short periods of time, do this instead:
///
/// \code{.cpp}
///
///   archon::core::Timer timer;
///   // Do stuf here ...
///   double time = timer.get_elapsed_time();
///   std::cout << "Elapsed time: " << archon::core::as_time_a(time) << "\n";
///
/// \endcode
///
/// \sa \ref core::as_time_a().
///
class Timer {
public:
    /// \brief Available types of timer clocks.
    ///
    /// This is an enumeration of the types of clocks that can be specidfied as preferred
    /// when constructing a timer (\ref Timer()). Support for some, or all of these may be
    /// missing on any particular platform.
    ///
    /// \sa \ref has_monotonic_clock(), \ref has_process_cputime(), \ref
    /// has_thread_cputime().
    ///
    enum class Type {
        /// \brief Monotonic clock.
        ///
        /// A real time clock that is not affected by discontinuous jumps such as when the
        /// current time is adjusted by an administrator. On POSIX systems, this corresponds
        /// to passing `MONOTONIC_CLOCK` to `clock_gettime()`.
        ///
        monotonic_clock,

        /// \brief Time spent by all threads in process.
        ///
        /// A clock that measures the sum of CPU-time spent by all threads in the system
        /// process. On POSIX systems, this corresponds to passing
        /// `CLOCK_PROCESS_CPUTIME_ID` to `clock_gettime()`.
        ///
        process_cputime,

        /// \brief Thread-specific CPU-time.
        ///
        /// A clock that measures the CPU time spent by a single thread. This only produces
        /// useful results if the thread, that creates / resets the timer, is also the one
        /// that reads off the stop time by calling get_elapsed_time(). On POSIX systems,
        /// this corresponds to passing `CLOCK_THREAD_CPUTIME_ID` to `clock_gettime()`.
        ///
        thread_cputime
    };

    /// \brief Construct a timer for specified clock type.
    ///
    /// Construct a timer that preferably uses a clock of the specified type to read stat
    /// and stop times from.
    ///
    /// If a monotonic clock is specified as preferred, but not available on this platform,
    /// a potentially nonmonotonic real time clock will be used instead. Any duration
    /// measurement that would become negative due to time adjustement will be reported as
    /// zero (negative results are changed to zero in all cases).
    ///
    /// If process CPU time is specified as preferred, but not available on this platform,
    /// the timer will use whatever it would use if a monotonic clock had been specified as
    /// preferred.
    ///
    /// If thread CPU time is specified as preferred, but not available on this platform,
    /// the timer will use whatever it would use if process CPU time had been specified as
    /// preferred.
    ///
    explicit Timer(Type type = Type::monotonic_clock);

    /// \brief Reset start time to "now".
    ///
    /// Reset the start time to "now". This is implicitly done as part of constructing a new
    /// timer.
    ///
    void reset();

    /// \brief Amount of elapsed time.
    ///
    /// Returns the amount of time elapsed since the construction of the timer, or since the
    /// last invocation of \ref reset(). The elapsed time is expressed in number of seconds.
    ///
    /// \sa \ref operator<<(std::basic_ostream<C, T>&, const Timer&)
    ///
    double get_elapsed_time() const;

    /// \brief A monotonic clock is available.
    ///
    /// This function returns true if, and only if \ref Type::monotonic_clock is available
    /// on this platform.
    ///
    static bool has_monotonic_clock() noexcept;

    /// \brief Process CPU-time is available.
    ///
    /// This function returns true if, and only if \ref Type::process_cputime is available
    /// on this platform.
    ///
    static bool has_process_cputime() noexcept;

    /// \brief Thread CPU-time is available.
    ///
    /// This function returns true if, and only if \ref Type::thread_cputime is available on
    /// this platform.
    ///
    static bool has_thread_cputime() noexcept;

private:
    union TimePoint {
        core::timespec_type timespec;
        std::chrono::steady_clock::rep duration;
    };

    struct Info;
    static auto get_info() noexcept -> const Info&;

    class Init {
    public:
        const Info& info;
        Init() noexcept;
    };

    static inline Init s_init;

    const Info& m_info;
    const Type m_type;
    TimePoint m_start;

    auto get_current_time() const -> TimePoint;
};


/// \brief Write elapsed time to output stream.
///
/// This function first requests the amount of elapsed time by calling \ref
/// core::Timer::get_elapsed_time(), then it uses \ref core::as_time_a() to write the
/// formatted time to the specified output stream.
///
template<class C, class T> auto operator<<(std::basic_ostream<C, T>&, const core::Timer&) -> std::basic_ostream<C, T>&;








// Implementation


inline Timer::Timer(Type type)
    : m_info(s_init.info)
    , m_type(type)
{
    reset(); // Throws

    // Work around silly warning from Apple clang 14.0.3 (clang-1403.0.22.14.1).
    static_cast<void>(m_info);
    static_cast<void>(m_type);
}


inline void Timer::reset()
{
    m_start = get_current_time(); // Throws
}


inline Timer::Init::Init() noexcept
    : info(get_info())
{
}


template<class C, class T> inline auto operator<<(std::basic_ostream<C, T>& out,
                                                  const core::Timer& timer) -> std::basic_ostream<C, T>&
{
    out << core::as_time_a(timer.get_elapsed_time()); // Throws
    return out;
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_TIMER_HPP
