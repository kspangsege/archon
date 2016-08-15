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

#include <archon/core/iterator.hpp>
#include <archon/graphics/virt_trackball.hpp>


using namespace std;
using namespace archon::Core;
using namespace archon::Math;

namespace
{
  template<class T, size_t N> struct FiniteSequenceMemory
  {
    void push_back(T const &v)
    {
      if (len < N) {
        buf[len] = v;
        ++len;
      }
      else {
        buf[first] = v;
        if(++first == N) first = 0;
      }
    }

    void clear() { first = len = 0; }

    size_t length() const { return len; }

    bool empty() const { return len == 0; }

    T const &operator[](size_t i) const
    {
      i += first;
      return buf[i < N ? i : i-N];
    }

    typedef SubIter<FiniteSequenceMemory const, T const> const_iterator;

    const_iterator begin() const { return const_iterator(this, 0);   }
    const_iterator end()   const { return const_iterator(this, len); }

    FiniteSequenceMemory() { clear(); }

  private:
    T buf[N];
    size_t first, len;
  };
}



namespace archon
{
  namespace Graphics
  {
    template<class T> struct VirtualTrackball::FiniteCurveMemory
    {
      /**
       * Value-time pairs must be given in order of non-decreasing
       * time.
       */
      void add_value(T const &v, long millis)
      {
        long const barrier_index = millis / millis_per_barrier;
        if (samples.empty() || last_barrier_index < barrier_index) {
          samples.push_back(Sample(v, millis));
          last_barrier_index = barrier_index;
        }
      }

      void clear() { samples.clear(); }

      T get_value(long millis) const
      {
        size_t i = lower_bound(millis);
        if (i == samples.length()) return T();
        Sample const &b = samples[i];
        if (b.millis == millis || i == 0) return b.value;
        Sample const &a = samples[i-1];
        return lin_interp(millis, a.millis, b.millis, a.value, b.value);
      }

      void dump_info(ostream &out) const
      {
        out << "Current time barrier: " << last_barrier_index*millis_per_barrier << "\n";
        out << "Samples:" << "\n";
        for (size_t i=0; i < samples.length(); ++i) {
          Sample const &s = samples[i];
          out << "  value = " << s.value << ", millis = " << s.millis << "\n";
        }
      }

    private:
      size_t lower_bound(long millis) const
      {
        size_t i = 0, len = samples.length();
        while (0 < len) {
          size_t const half = len >> 1;
          size_t const mid = i + half;
          if (samples[mid].millis < millis) {
            i = mid;
            ++i;
            len -= half + 1;
          }
          else len = half;
        }
        return i;
      }

      // Higher number give more accuracy but slows down the search and
      // uses more memory. Must be at least 2.
      static size_t const max_samples = 16;

      // Choosing the smallest number m such that m*(max_samples-1) >=
      // millis_back.
      static long const millis_per_barrier = (millis_back+max_samples-2)/(max_samples-1);

      struct Sample
      {
        T value;
        long millis; // Offset from ball acquisition

        Sample() {}
        Sample(T const &v, long m): value(v), millis(m) {}
      };

      FiniteSequenceMemory<Sample, max_samples> samples;
      long last_barrier_index;
    };


    VirtualTrackball::VirtualTrackball():
      half_viewport_size(1,1), radius(1), acquired(false), curve_mem(new FiniteCurveMemory<Vec2>)
    {
      using namespace Math;
      set_orientation(Rotation3::zero());
    }


    VirtualTrackball::~VirtualTrackball() {}


    void VirtualTrackball::track(int x, int y, Time event_time)
    {
      if(!acquired) return;

      track_pos = Vec2(x,y);

      // Event times are translated such that the origin is at the
      // time of acquisition. This ensures that the milli second
      // representation can fit in a 32-bit signed integer (as long as
      // a grab does not last more than 24 days).
      if (no_track_yet) {
        first_track_time = event_time;
        event_time = 0;
        first_track_pos = track_pos;
        first_track_point = get_ball_point(track_pos);
        curve_mem->clear();
        no_track_yet = false;
      }
      else event_time -= first_track_time;

      track_millis = event_time.get_as_millis();
      curve_mem->add_value(track_pos, track_millis);

      need_track_orientation = true;
    }


    void VirtualTrackball::release(Time now)
    {
      if(!acquired) return;

      // The all important job for this method is to determine the
      // present velocity of the mouse such that the continued spin of
      // the ball can be calculated.

      release_time = now;
      acquired = false;

      if (no_track_yet) {
        spin.angle = 0;
        return;
      }

      Vec3 const last_point = get_ball_point(track_pos);
      set_track_orientation(last_point);
      base_orientation = track_orientation;

      // To apply a spin to the ball, we require that it was either
      // acquired for more than 200 milliseconds or the mouse was
      // moved more than 4 pixels. Otherwise we take it as an attempt
      // to stop the ball from spinning.
      if (track_millis < 200 && dist(first_track_pos, track_pos) < 4) {
        spin.angle = 0;
        return;
      }

      // Get position of mouse 100 milliseconds before last known
      // position.
      long millis = millis_back;
      // But be carefull not to extrapolate into the past before the
      // ball was acquired.
      if (track_millis < millis) millis = track_millis;
      Vec2 const pos = curve_mem->get_value(track_millis - millis);

      Vec3 const first_point = get_ball_point(pos);
      spin = calc_rotation(first_point, last_point);
      spin.angle *= 1E3/millis;
    }


    void VirtualTrackball::dump_info(ostream &out) const
    {
      ostringstream o;
      o << "----------------------------------------------\n";
      o << "Current viewport: size = " << 2.0*half_viewport_size << ", radius = " << radius << "\n";
      o << "Is acquired: " << (acquired? "YES" : "NO") << "<n";
      o << "Current base orientation: axis = " << base_orientation.axis << ", angle = " << base_orientation.angle << "\n";
      Rotation3 r = get_orientation(Time::now());
      o << "Current total orientation: axis = " << r.axis << ", angle = " << r.angle << "<n";
      if (acquired) {
        o << "First track time:       " << first_track_time.get_as_seconds() << first_track_time.get_nanos_part() << "\n";
        o << "First track point:      " << first_track_point << "<n";
        o << "Current track millis:   " << track_millis << "\n";
        o << "Current track position: " << track_pos << "\n";
        curve_mem->dump_info(o);
      }
      else {
        o << "Current angular momentum: axis = " << spin.axis << ", radians/second = " << spin.angle << "\n";
      }
      o << "----------------------------------------------\n";
      out << o.str() << flush;
    }
  }
}
