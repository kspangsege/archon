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

#ifndef ARCHON_CORE_TIME_HPP
#define ARCHON_CORE_TIME_HPP

#include <cstdlib>
#include <ctime>
#include <cmath>
#include <limits>
#include <string>

#include <archon/core/assert.hpp>


namespace archon
{
  namespace Core
  {
    /**
     * Signed representation of time in nanosecond precision.
     *
     * When used as an absolute time, zero is taken to correspond to
     * the start of the UNIX Epoch, which is defined to be Thu Jan 1
     * 00:00:00 GMT 1970. Note that this implies that absolute time
     * values are time zone independant, or in other words, if two
     * people in two different time zones run <tt>Time::now()</tt>
     * exactly when a third person snaps his fingers, they will get
     * equal values.
     */
    struct Time
    {
      /**
       * Get the representation of zero time. When interpreted as an
       * absolute time, this corresponds to Thu Jan 1 00:00:00 GMT
       * 1970 (the start of the UNIX Epoch).
       */
      Time() { set_seconds_and_nanos(0,0); }


      /**
       * Initialize by the number of seconds and nanoseconds. See also
       * <tt>set_seconds_and_nanos</tt>.
       *
       * \param nanos The fractional part of the time mesured in
       * nanoseconds and must not be less than zero, nor greater than
       * 999,999,999.
       */
      Time(std::time_t seconds, long nanos=0) { set_seconds_and_nanos(seconds, nanos); }


      enum Unit { nanos, micros, millis, seconds, minutes, hours, days };

      Time(long v, Unit);


      /**
       * Get the time that corresponds to "now".
       *
       * \todo FIXME: \c clock_gettime is part of the POSIX.1-2001
       * Timers optional extension, and is only available if
       * _POSIX_TIMERS is defined by <tt>unistd.h</tt>. \c
       * gettimeofday is mandatory in POSIX.1-2001.
       */
      static Time now();

      Time &operator+= (const Time &t);
      Time &operator-= (const Time &t);
      Time operator+ (const Time &t) const;
      Time operator- (const Time &t) const;

      bool operator== (const Time &t) const;
      bool operator!= (const Time &t) const;
      bool operator<  (const Time &t) const;
      bool operator>  (const Time &t) const;
      bool operator<= (const Time &t) const;
      bool operator>= (const Time &t) const;

    private:
      typedef long Time::*unspecified_bool_type;

    public:
      operator unspecified_bool_type() const throw();

      /**
       * The result always lies in the range 0 to 999,999,999, and
       * assuming infinite precision arithmetic, <tt>get_as_seconds()
       * * 1,000,000,000 + get_nanos_part()</tt> is always precisely
       * the represented number of nanoseconds.
       */
      long get_nanos_part() const;

      /**
       * \param nanos The fractional part of the time mesured in
       * nanoseconds and must not be less than zero, nor greater than
       * 999,999,999. The total time becomes <tt>seconds +
       * nanos/1,000,000,000</tt>.
       */
      void set_seconds_and_nanos(std::time_t seconds, long nanos);

      double get_as_seconds_float() const;

      template<typename T> void get_as_seconds_float(T &v) const;

      template<typename T> void set_as_seconds_float(T v);

      /**
       * Rounding towards negative infinity.
       */
      std::time_t get_as_seconds() const;

      void set_as_seconds(std::time_t v);

      /**
       * Rounding towards negative infinity. Result is unspecified if
       * the time represented as a rounded number of milliseconds does
       * not fit in a long.
       */
      long get_as_millis() const;

      void set_as_millis(long v);

      /**
       * Rounding towards negative infinity. Result is unspecified if
       * the time represented as a rounded number of microseconds does
       * not fit in a long.
       */
      long get_as_micros() const;

      void set_as_micros(long v);

      /**
       * Result is unspecified if the time represented as a rounded
       * number of nanoseconds does not fit in a long.
       */
      long get_as_nanos() const;

      void set_as_nanos(long v);


      /**
       * Format the time according to RFC 1123. The formatted time is
       * always expressed in UTC (Coordinated Universal Time). Any
       * HTTP/1.1 compliant agent must produce protocol time stamps in
       * this format.
       *
       * \return A time expressed in UTC and formatted according to RFC 1123.
       *
       * \sa http://www.ietf.org/rfc/rfc1123.txt
       */
      std::string format_rfc_1123();

    private:
      std::time_t secs;  // Seconds since the Epoch
      long nsecs; // Should always be in the range [0;999,999,999]

      void adjust();

      /**
       * \param r Must be on the form 10^n where n is an integer in
       * the range 0 to 9.
       */
      long get_as_any(long r) const;

      /**
       * \param r Must be on the form 10^n where n is an integer in
       * the range 0 to 9.
       */
      void set_as_any(long v, long r);
    };






    // Implementation:

    inline Time::Time(long v, Unit unit)
    {
      switch(unit) {
      case nanos:   set_as_nanos(v);              break;
      case micros:  set_as_micros(v);             break;
      case millis:  set_as_millis(v);             break;
      case seconds: set_as_seconds(v);            break;
      case minutes: set_as_seconds(v/60);         break;
      case hours:   set_as_seconds(v/(60*60));    break;
      case days:    set_as_seconds(v/(60*60*24)); break;
      }
    }

    inline Time &Time::operator+=(const Time &t)
    {
      nsecs += t.nsecs;
      secs  += t.secs;
      adjust();
      return *this;
    }

    inline Time &Time::operator-=(const Time &t)
    {
      nsecs -= t.nsecs;
      secs  -= t.secs;
      adjust();
      return *this;
    }

    inline Time Time::operator+(const Time &t) const
    {
      Time u(*this);
      u += t;
      return u;
    }

    inline Time Time::operator-(const Time &t) const
    {
      Time u(*this);
      u -= t;
      return u;
    }

    inline bool Time::operator==(const Time &t) const
    {
      return secs == t.secs && nsecs == t.nsecs;
    }

    inline bool Time::operator!=(const Time &t) const
    {
      return secs != t.secs || nsecs != t.nsecs;
    }

    inline bool Time::operator<(const Time &t) const
    {
      return secs < t.secs || secs == t.secs && nsecs < t.nsecs;
    }

    inline bool Time::operator>(const Time &t) const
    {
      return secs > t.secs || secs == t.secs && nsecs > t.nsecs;
    }

    inline bool Time::operator<=(const Time &t) const
    {
      return secs < t.secs || secs == t.secs && nsecs <= t.nsecs;
    }

    inline bool Time::operator>=(const Time &t) const
    {
      return secs > t.secs || secs == t.secs && nsecs >= t.nsecs;
    }

    inline Time::operator unspecified_bool_type() const throw()
    {
      return secs || nsecs ? &Time::secs : 0;
    }

    inline long Time::get_nanos_part() const
    {
      return nsecs;
    }

    inline void Time::set_seconds_and_nanos(std::time_t seconds, long nanos)
    {
      secs  = seconds;
      nsecs = nanos;
    }

    inline double Time::get_as_seconds_float() const
    {
      double v;
      get_as_seconds_float(v);
      return v;
    }

    template<typename T> inline void Time::get_as_seconds_float(T &v) const
    {
      ARCHON_STATIC_ASSERT(!std::numeric_limits<T>::is_integer, "Need floating point type");
      v = secs + T(nsecs) / 1E9;
    }

    template<typename T> inline void Time::set_as_seconds_float(T v)
    {
      ARCHON_STATIC_ASSERT(!std::numeric_limits<T>::is_integer, "Need floating point type");
      T i;
      nsecs = std::modf(v, &i) * 1E9;
      secs = i;
    }

    inline std::time_t Time::get_as_seconds() const
    {
      return secs;
    }

    inline void Time::set_as_seconds(std::time_t v)
    {
      set_seconds_and_nanos(v,0);
    }

    inline long Time::get_as_millis() const
    {
      return get_as_any(1000);
    }

    inline void Time::set_as_millis(long v)
    {
      set_as_any(v, 1000);
    }

    inline long Time::get_as_micros() const
    {
      return get_as_any(1000000);
    }

    inline void Time::set_as_micros(long v)
    {
      set_as_any(v, 1000000);
    }

    inline long Time::get_as_nanos() const
    {
      return get_as_any(1000000000);
    }

    inline void Time::set_as_nanos(long v)
    {
      set_as_any(v, 1000000000);
    }

    inline void Time::adjust()
    {
      if(1000000000 <= nsecs)
      {
        nsecs -= 1000000000;
        ++secs;
      }
      else if(nsecs < 0)
      {
        nsecs += 1000000000;
        --secs;
      }
    }

    inline long Time::get_as_any(long r) const
    {
      return secs * r + nsecs / (1000000000/r);
    }

    inline void Time::set_as_any(long v, long r)
    {
      std::ldiv_t d = std::ldiv(v,r);
      secs  = d.quot;
      nsecs = d.rem * (1000000000/r);
    }
  }
}

#endif // ARCHON_CORE_TIME_HPP
