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

#ifndef ARCHON_GRAPHICS_VIRT_TRACKBALL_HPP
#define ARCHON_GRAPHICS_VIRT_TRACKBALL_HPP

#include <cmath>
#include <memory>
#include <chrono>
#include <ostream>

#include <archon/math/vector.hpp>
#include <archon/math/rotation.hpp>


namespace archon {
namespace graphics {

/// The virtual trackball controls the orientation of the object coordinate
/// system relative to the camera/eye coordinate system.
///
/// Any orientation in 3-D space, that keeps at least one point fixed, can be
/// expressed as a certain amount of rotation about an axis with a certain fixed
/// direction. This assumes that there is some given default orientation, after
/// which the axial rotation is applied to give the new desired orientation.
///
/// Thus, the state of the trackball is expressed as an axial rotation (axis and
/// an angle).
///
/// The default rotation is zero (zero degrees about the zero-vector). This
/// means that by default, all three axes of the object coordinate system are
/// directed exacly as they are in the camera/eye coordinate system.
///
/// \note The methods of this class are not thread-safe, not even when the
/// instance is logically constant and all threads are reading. This is due to
/// the mutable cache related variables. It is safe however to have multiple
/// trackball instances being accessed simultaneously as long as only on thread
/// accesses one trackbal at a time.
///
/// \todo FIXME: Pixel aspect ratio is not taken into account. It should be,
/// such that the ball is never "egg-shaped" on the screen.
///
/// \todo FIXME: Perspective projection is not taken into account. An
/// orthographic projection is assumed. This means that the mouse does not
/// follow a point on a rendered sphere with the same center and radius as the
/// trackball. It should.
class VirtualTrackball {
public:
    using clock = std::chrono::steady_clock;

    /// Time since a fixed, but arbitrary origin. This class will only use these
    /// timestamps to measure time between tracking events.
    using TrackTime = std::chrono::milliseconds;

    /// Update the size of the trackball to fit the rendering viewport.
    void set_viewport_size(int w, int h);

    /// Bring the trackball into the acquired state. In this state the trackball
    /// follows the mouse movement stricktly.
    ///
    /// Call this method whenever the mouse button, that controls this
    /// trackball, is pressed down. This call should be followed immediately by
    /// a call to the \c track method giving the mouse coordinates at the time
    /// of pressing.
    ///
    /// This method has no effect if called when the trackball is already in the
    /// acquired state.
    ///
    /// \param now The absolute current system time. Note that this is not
    /// necessarily the same time value as you would pass to the succeeding \c
    /// track method.
    ///
    /// \sa track
    void acquire(clock::time_point now);

    /// Call this method for each mouse motion event, immediately after a call
    /// to <tt>acquire</tt>, and immediately before a call to release. This
    /// method has no effect unless the trackball is in the acquired state (see
    /// class documentation for further details).
    ///
    /// \param time The time the event occured. The origin for this time is
    /// arbitrary, but must be consitent across all calls to this method. In
    /// particular, the origin does not have to be the start of the UNIX Epoch,
    /// nor does it have to be the same as for the time arguments of \c release
    /// and <tt>get_rotation</tt>.
    ///
    /// \sa release
    void track(int x, int y, TrackTime time);

    /// Bring the trackball out of the acquired state. When not in the acquired
    /// state, the trackball has a constant spin (or is at rest). This constant
    /// spin is a continuation of the forced spin immediately before the
    /// trackabll was released. This simulates conservation of angular momentum
    /// when no force is applied.
    ///
    /// Call this method whenever the mouse button, that controls this
    /// trackball, is released. This call should followed immediately after a
    /// call to the \c track method giving the mouse coordinates at the time of
    /// button release.
    ///
    /// This method has no effect if called while the trackball is not in the
    /// acquired state.
    ///
    /// \param now The absolute current system time. Note that this is not
    /// necessarily the same time value as you would pass to the preceeding \c
    /// track method.
    ///
    /// \sa track
    void release(clock::time_point now);

    /// Get the orientation of the trackball at the specified time. To get
    /// reliable results, the specified time shold be as close to 'now' as
    /// possible.
    math::Rotation3 get_orientation(clock::time_point now) const;

    /// Reorient the trackball by first placing it in its default orientation,
    /// then applying the specified rotation. The default orientation is
    /// described in the class documentation. The trackball will stop spinning.
    void set_orientation(math::Rotation3 r);

    /// Spin the trackball according to the specified angular momentum, that is,
    /// the angle component of the specified rotation is interpreted as a scalar
    /// angular momentum (radians/second). The spin will be based on its current
    /// orientation. The current time is required such that its current
    /// orientation can be known, and such that subsequent calls to
    /// get_orientation() get correct results based on the time of those calls.
    void set_spin(math::Rotation3 spin, clock::time_point now);

    void dump_info(std::ostream&) const;

    VirtualTrackball();
    ~VirtualTrackball();

private:
    math::Rotation3 get_free_orientation(clock::time_point) const;
    math::Rotation3 get_track_orientation() const;
    void set_track_orientation(math::Vec3 p) const;
    math::Vec3 get_ball_point(math::Vec2 pos) const;
    static math::Rotation3 calc_rotation(const math::Vec3& a, const math::Vec3& b);

    // Considder the last 100 milliseconds of the mouse movement when
    // determining the free spin. Must be at least 2.
    static const long s_millis_back = 100;

    math::Vec2 m_half_viewport_size{1,1};
    double m_radius = 1;

    bool m_acquired = false;

    clock::time_point m_release_time;
    math::Rotation3 m_base_orientation;
    math::Rotation3 m_spin;

    bool m_no_track_yet;

    TrackTime m_first_track_time;
    math::Vec2 m_first_track_pos;
    math::Vec3 m_first_track_point;

    long m_track_millis;
    math::Vec2 m_track_pos;

    template<class> class FiniteCurveMemory;
    const std::unique_ptr<FiniteCurveMemory<math::Vec2>> m_curve_mem;

    // Caching
    mutable math::Rotation3 m_track_orientation;
    mutable bool m_need_track_orientation;
};




// Implementation

inline void VirtualTrackball::set_viewport_size(int width, int height)
{
    using namespace math;
    m_half_viewport_size.set(width/2.0, height/2.0);
    m_radius = min(m_half_viewport_size);
}

inline void VirtualTrackball::acquire(clock::time_point now)
{
    if (m_acquired)
        return;
    m_base_orientation = get_free_orientation(now);
    m_acquired = m_no_track_yet = m_need_track_orientation = true;
}

inline void VirtualTrackball::set_orientation(math::Rotation3 rot)
{
    m_base_orientation = rot;
    m_spin.angle = 0;
    m_acquired = false;
}

inline void VirtualTrackball::set_spin(math::Rotation3 spin, clock::time_point now)
{
    m_base_orientation = get_orientation(now);
    m_release_time = now;
    m_acquired = false;
    m_spin = spin;
}

inline math::Rotation3 VirtualTrackball::get_orientation(clock::time_point now) const
{
    return (m_acquired ? get_track_orientation() : get_free_orientation(now));
}

/// Calculate the orientation of the free spinning ball at the specified time.
inline math::Rotation3 VirtualTrackball::get_free_orientation(clock::time_point time) const
{
    if (!m_spin.angle)
        return m_base_orientation;
    std::chrono::duration<double> time_2 = time - m_release_time;
    math::Rotation3 spin = m_spin, rot = m_base_orientation;
    spin.angle *= time_2.count();
    rot.combine_with(spin);
    return rot;
}

/// Assumes ball in acquired mode and no_track_yet = false.
inline math::Rotation3 VirtualTrackball::get_track_orientation() const
{
    if (m_need_track_orientation) {
        if (m_no_track_yet) {
            m_track_orientation = m_base_orientation;
        }
        else {
            set_track_orientation(get_ball_point(m_track_pos));
        }
        m_need_track_orientation = false;
    }
    return m_track_orientation;
}

inline void VirtualTrackball::set_track_orientation(math::Vec3 p) const
{
    m_track_orientation = m_base_orientation;
    m_track_orientation.combine_with(calc_rotation(m_first_track_point, p));
}

inline math::Vec3 VirtualTrackball::get_ball_point(math::Vec2 pos) const
{
    math::Vec2 p(pos[0]-m_half_viewport_size[0], m_half_viewport_size[1]-pos[1]);
    p /= m_radius;
    double s = sq_sum(p);

    // Clamp to unit disc
    if (1 < s) {
        p /= std::sqrt(s);
        s = 1;
    }

    return math::Vec3(p[0], p[1], std::sqrt(1-s));
}

inline math::Rotation3 VirtualTrackball::calc_rotation(const math::Vec3& a, const math::Vec3& b)
{
    math::Vec3 axis = a * b;
    double s = sq_sum(axis), d = dot(a,b);
    return s && d<1 ? math::Rotation3(axis/sqrt(s), std::acos(d)) : math::Rotation3::zero();
}

} // namespace graphics
} // namespace archon

#endif // ARCHON_GRAPHICS_VIRT_TRACKBALL_HPP
