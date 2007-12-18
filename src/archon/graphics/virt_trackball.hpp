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

#ifndef ARCHON_GRAPHICS_VIRT_TRACKBALL_HPP
#define ARCHON_GRAPHICS_VIRT_TRACKBALL_HPP

#include <cmath>
#include <ostream>

#include <archon/core/unique_ptr.hpp>
#include <archon/core/time.hpp>
#include <archon/math/vector.hpp>
#include <archon/math/rotation.hpp>


namespace Archon
{
  namespace Graphics
  {
    /**
     * The virtual trackball controls the orientation of the object
     * coordinate system relative to the camera/eye coordinate system.
     *
     * Any orientation in 3-D space, that keeps at least one point
     * fixed, can be expressed as a certain amount of rotation about
     * an axis with a certain fixed direction. This assumes that there
     * is some given default orientation, after which the axial
     * rotation is applied to give the new desired orientation.
     *
     * Thus, the state of the trackball is expressed as an axial
     * rotation (axis and an angle).
     *
     * The default rotation is zero (zero degrees about the
     * zero-vector). This means that by default, all three axes of the
     * object coordinate system are directed exacly as they are in the
     * camera/eye coordinate system.
     *
     * \note The methods of this class are not thread-safe, not even
     * when the instance is logically constant and all threads are
     * reading. This is due to the mutable cache related variables. It
     * is safe however to have multiple trackball instances being
     * accessed simultaneously as long as only on thread accesses one
     * trackbal at a time.
     *
     * \todo FIXME: Pixel aspect ratio is not taken into account. It
     * should be, such that the ball is never "egg-shaped" on the
     * screen.
     *
     * \todo FIXME: Perspective projection is not taken into
     * account. An orthographic projection is assumed. This means that
     * the mouse does not follow a point on a rendered sphere with the
     * same center and radius as the trackball. It should.
     */
    struct VirtualTrackball
    {
      /**
       * Update the size of the trackball to fit the rendering
       * viewport.
       */
      void set_viewport_size(int w, int h);


      /**
       * Bring the trackball into the acquired state. In this state
       * the trackball follows the mouse movement stricktly.
       *
       * Call this method whenever the mouse button, that controls
       * this trackball, is pressed down. This call should be followed
       * immediately by a call to the \c track method giving the mouse
       * coordinates at the time of pressing.
       *
       * This method has no effect if called when the trackball is
       * already in the acquired state.
       *
       * \param now The absolute current system time. Note that this
       * is not necessarily the same time value as you would pass to
       * the succeeding \c track method.
       *
       * \sa track
       */
      void acquire(Core::Time now);


      /**
       * Call this method for each mouse motion event, immediately
       * after a call to <tt>acquire</tt>, and immediately before a
       * call to release. This method has no effect unless the
       * trackball is in the acquired state (see class documentation
       * for further details).
       *
       * \param event_time The time the event occured. The origin for
       * this time is arbitrary, but must be consitent across all
       * calls to this method. In particular, the origin does not have
       * to be the start of the UNIX Epoch, nor does it have to be the
       * same as for the time arguments of \c release and
       * <tt>get_rotation</tt>.
       *
       * \sa release
       */
      void track(int x, int y, Core::Time event_time);


      /**
       * Bring the trackball out of the acquired state. When not in
       * the acquired state, the trackball has a constant spin (or is
       * at rest). This constant spin is a continuation of the forced
       * spin immediately before the trackabll was released. This
       * simulates conservation of angular momentum when no force is
       * applied.
       *
       * Call this method whenever the mouse button, that controls
       * this trackball, is released. This call should followed
       * immediately after a call to the \c track method giving the
       * mouse coordinates at the time of button release.
       *
       * This method has no effect if called while the trackball is
       * not in the acquired state.
       *
       * \param now The absolute current system time. Note that this
       * is not necessarily the same time value as you would pass to
       * the preceeding \c track method.
       *
       * \sa track
       */
      void release(Core::Time now);


      /**
       * Get the orientation of the trackball at the specified
       * time. To get reliable results, the specified time shold be as
       * close to 'now' as possible.
       */
      Math::Rotation3 get_orientation(Core::Time now) const;


      /**
       * Reorient the trackball by first placing it in its default
       * orientation, then applying the specified rotation. The
       * default orientation is described in the class
       * documentation. The trackball will stop spinning.
       */
      void set_orientation(Math::Rotation3 r);


      /**
       * Spin the trackball according to the specified angular
       * momentum, that is, the angle component of the specified
       * rotation is interpreted as a scalar angular momentum
       * (radians/second). The spin will be based on its current
       * orientation. The current time is required such that its
       * current orientation can be known, and such that subsequent
       * calls to \c get_orientation get correct results based on the
       * time of those calls.
       */
      void set_spin(Math::Rotation3 spin, Core::Time now);


      void dump_info(std::ostream &) const;

      VirtualTrackball();
      ~VirtualTrackball();

    private:
      Math::Rotation3 get_free_orientation(Core::Time time) const;
      Math::Rotation3 get_track_orientation() const;
      void set_track_orientation(Math::Vec3 p) const;
      Math::Vec3 get_ball_point(Math::Vec2 pos) const;
      Math::Rotation3 calc_rotation(Math::Vec3 const &a, Math::Vec3 const &b) const;

      // Considder the last 100 milliseconds of the mouse movement
      // when determining the free spin. Must be at least 2.
      static long const millis_back = 100;

      Math::Vec2 half_viewport_size;
      double radius;

      bool acquired;

      Core::Time release_time;
      Math::Rotation3 base_orientation;
      Math::Rotation3 spin;

      bool no_track_yet;

      Core::Time first_track_time;
      Math::Vec2 first_track_pos;
      Math::Vec3 first_track_point;

      long track_millis;
      Math::Vec2 track_pos;

      template<class> struct FiniteCurveMemory;
      Core::UniquePtr<FiniteCurveMemory<Math::Vec2> > const curve_mem;

      // Caching
      mutable Math::Rotation3 track_orientation;
      mutable bool need_track_orientation;
    };





    // Implementation:

    void VirtualTrackball::set_viewport_size(int w, int h)
    {
      using namespace Math;
      half_viewport_size.set(w/2.0, h/2.0);
      radius = min(half_viewport_size);
    }


    void VirtualTrackball::acquire(Core::Time now)
    {
      if(acquired) return;
      base_orientation = get_free_orientation(now);
      acquired = no_track_yet = need_track_orientation = true;
    }


    void VirtualTrackball::set_orientation(Math::Rotation3 r)
    {
      base_orientation = r;
      spin.angle = 0;
      acquired = false;
    }


    void VirtualTrackball::set_spin(Math::Rotation3 s, Core::Time now)
    {
      base_orientation = get_orientation(now);
      release_time = now;
      acquired = false;
      spin = s;
    }


    Math::Rotation3 VirtualTrackball::get_orientation(Core::Time now) const
    {
      return acquired ? get_track_orientation() : get_free_orientation(now);
    }


    /**
     * Calculate the orientation of the free spinning ball at the
     * specified time.
     */
    Math::Rotation3 VirtualTrackball::get_free_orientation(Core::Time time) const
    {
      using namespace Math;
      if (!spin.angle) return base_orientation;
      time -= release_time;
      Rotation3 s = spin, r = base_orientation;
      s.angle *= time.get_as_seconds_float();
      r.combine_with(s);
      return r;
    }


    /**
     * Assumes ball in acquired mode and no_track_yet = false.
     */
    Math::Rotation3 VirtualTrackball::get_track_orientation() const
    {
      if(need_track_orientation) {
        if (no_track_yet) track_orientation = base_orientation;
        else set_track_orientation(get_ball_point(track_pos));
        need_track_orientation = false;
      }
      return track_orientation;
    }


    void VirtualTrackball::set_track_orientation(Math::Vec3 p) const
    {
      track_orientation = base_orientation;
      track_orientation.combine_with(calc_rotation(first_track_point, p));
    }


    Math::Vec3 VirtualTrackball::get_ball_point(Math::Vec2 pos) const
    {
      using namespace std;
      using namespace Math;
      Vec2 p(pos[0]-half_viewport_size[0], half_viewport_size[1]-pos[1]);
      p /= radius;
      double s = sq_sum(p);

      // Clamp to sphere
      if (1 < s) {
        p /= sqrt(s);
        s = 1;
      }

      return Vec3(p[0], p[1], sqrt(1-s));
    }


    Math::Rotation3 VirtualTrackball::calc_rotation(Math::Vec3 const &a, Math::Vec3 const &b) const
    {
      using namespace std;
      using namespace Math;
      Vec3 const axis = a * b;
      double const s = sq_sum(axis), d = dot(a,b);
      return s && d<1 ? Rotation3(axis/sqrt(s), acos(d)) : Rotation3::zero();
    }
  }
}

#endif // ARCHON_GRAPHICS_VIRT_TRACKBALL_HPP
