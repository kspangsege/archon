// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2023 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_RENDER_X_VIRT_TRACKBALL_HPP
#define ARCHON_X_RENDER_X_VIRT_TRACKBALL_HPP

/// \file


#include <cmath>
#include <algorithm>
#include <chrono>
#include <ostream>

#include <archon/math/vector.hpp>
#include <archon/math/rotation.hpp>
#include <archon/util/pixel_size.hpp>
#include <archon/util/pixel_pos.hpp>
#include <archon/render/impl/finite_curve_memory.hpp>


namespace archon::render {


/// \brief Mouse controlled virtual trackball.
///
/// This class implements a virtual trackball. The purpose of such a virtual trackball is to
/// take input from a pointer device (mouse) and use that to control the orientation of of
/// the virtual space of an object of interest relative to the virtual space of the camera.
///
/// Any orientation in 3-D space, that keeps at least one point fixed, can be expressed as a
/// certain amount of rotation about an axis with a certain fixed direction. This assumes
/// that there is some given default orientation, after which the axial rotation is applied
/// to give the new desired orientation.
///
/// Thus, the state of the trackball is expressed as an axial rotation (axis and an angle).
///
/// The default rotation is zero (zero degrees about the zero-vector). This means that by
/// default, all three axes of the object coordinate system are directed exactly as they are
/// in the camera/eye coordinate system.
///
/// FIXME: Pixel aspect ratio is not taken into account. It should be, such that the ball is never "egg-shaped" on the screen.                                 
///
/// FIXME: Perspective projection is not taken into account. An orthographic projection is assumed. This means that the mouse does not follow a point on a rendered sphere with the same center and radius as the trackball. It should.                    
///
class VirtualTrackball {
public:
    /// \brief Steady / monotonic clock used to animate trackball during free spin.
    ///
    /// This is a steady / monotonic clock used to animate the trackball during free spin.
    ///
    /// \sa \ref acquire()
    /// \sa \ref release()
    /// \sa \ref get_orientation()
    ///
    using Clock = std::chrono::steady_clock;

    /// \brief Time of mouse tracking event.
    ///
    /// Objects of this type record an amount of time since a fixed point in the past. This
    /// type is used exclusively to specify the times of individual mouse tracking events
    /// (\ref track()), and because it is always the difference between such times that
    /// matter, not knowing what zero corresponds to, is not a problem.
    ///
    /// \sa \ref track()
    ///
    using TrackTime = std::chrono::milliseconds;

    /// \brief Default-construct virtual trackball.
    ///
    /// This constructor default-constructs a virtual trackball.
    ///
    VirtualTrackball();

    /// \brief Update trackball size to match rendering viewport.
    ///
    /// This function updates the size of the trackball to fit the rendering viewport.
    ///
    void set_viewport_size(util::pixel::Size) noexcept;

    /// \brief Bring trackball into acquired state.
    ///
    /// This function brings the trackball into the acquired state. In this state the
    /// trackball follows the mouse movement strictly.
    ///
    /// Call this method whenever the mouse button, that controls this trackball, is pressed
    /// down. This call should be followed immediately by a call to \ref track() with the
    /// time of the mouse button press event, and the corresponding mouse coordinates as
    /// arguments.
    ///
    /// This method has no effect if called when the trackball is already in the acquired
    /// state.
    ///
    /// \param now The current time according to \ref Clock. This is used to calculate the
    /// final orientation of the trackball in case it was spinning freely.
    ///
    /// \sa \ref track()
    /// \sa \ref release()
    ///
    void acquire(Clock::time_point now) noexcept;

    /// \brief Record mouse movement event.
    ///
    /// This function records the specified mouse movement event. If should be called for
    /// all mouse movement events while the trackball is in the acquired state (see \ref
    /// acquire()). It must also be called immediately after calling \ref acquire() (passing
    /// parameters of the button press event) and immediately before calling \ref release()
    /// (passing parameters of the button release event).
    ///
    /// This function has no effect unless the trackball is in the acquired state (see class
    /// documentation for further details).
    ///
    /// \param time The time the event occurred. The origin for this time is arbitrary, but
    /// must be consistent across all calls to this function. The origin does not have to be
    /// the start of the UNIX Epoch, nor does it have to agree with the origin of \ref
    /// Clock.
    ///
    /// \sa \ref TrackTime
    /// \sa \ref acquire()
    /// \sa \ref release()
    ///
    void track(util::pixel::Pos pos, TrackTime time) noexcept;

    /// \brief Release trackball from acquired state.
    ///
    /// This function releases the trackball from the acquired state. When not in the
    /// acquired state, the trackball has a constant spin (or is at rest). This constant
    /// spin is a continuation of the spin that was forced by the mouse immediately before
    /// the trackball was released. This simulates conservation of angular momentum when no
    /// force is applied.
    ///
    /// Call this method whenever the mouse button, that controls this trackball, is
    /// released. This call should be immediately preceded by a call to \ref track() with
    /// the time of the mouse button release event, and the corresponding mouse coordinates
    /// as arguments.
    ///
    /// This method has no effect if called while the trackball is not in the acquired
    /// state.
    ///
    /// \param now The current time according to \ref Clock. This is used as a baseline for
    /// the subsequent animation of the trackball during its free spin. See \ref
    /// get_orientation().
    ///
    /// \sa \ref track()
    /// \sa \ref acquire()
    ///
    void release(Clock::time_point now) noexcept;

    /// \brief Get orientation of trackball.
    ///
    /// This function returns the orientation of the trackball. If the trackball is in the
    /// acquired state, this function simply returns the current orientation of the
    /// trackball. Otherwise, the trackball is spinning freely (or is at rest), and this
    /// function determines the instantaneous orientation at the specified point in time,
    /// which should generally be "now".
    ///
    auto get_orientation(Clock::time_point now) const noexcept -> math::Rotation;

    /// \brief Set orientation of trackball.
    ///
    /// This function orients the trackball as specified, such that \ref get_orientation()
    /// returns the specified orientation. If the trackball was spinning, it is brought to
    /// rest.
    ///
    void set_orientation(const math::Rotation& r) noexcept;

    /// \brief Impart spin on trackball.
    ///
    /// This function imparts a spin on the trackball as specified. The angle of the
    /// specified rotation is interpreted as an angular momentum (radians per second). If
    /// the trackball was in the acquire state, it is release from the acquired state. The
    /// current time is required for the same reason it is required by \ref release().
    ///
    void set_spin(const math::Rotation& spin, Clock::time_point now) noexcept;

    /// \brief Dump information about internal state of trackball.
    ///
    /// This function dumps information about the internal state of the trackball to the
    /// specified output stream.
    ///
    void dump_info(std::ostream&) const;

private:
    math::Vector2 m_half_viewport_size = { 1, 1 };
    double m_radius = 1;

    bool m_acquired = false;

    Clock::time_point m_release_time;
    math::Rotation m_base_orientation;
    math::Rotation m_spin;

    bool m_no_track_yet;

    TrackTime m_first_track_time;
    math::Vector2 m_first_track_pos;
    math::Vector3 m_first_track_point;

    long m_track_millis;
    math::Vector2 m_track_pos;

    impl::FiniteCurveMemory<math::Vector2> m_curve_mem;

    // Calculate the orientation of the free spinning ball at the specified time
    auto get_free_orientation(Clock::time_point) const noexcept -> math::Rotation;

    // Assumes ball in acquired mode and m_no_track_yet is false
    auto get_track_orientation() const noexcept -> math::Rotation;

    auto get_ball_point(const math::Vector2& pos) const noexcept -> math::Vector3;

    static auto calc_rotation(const math::Vector3& a, const math::Vector3& b) noexcept -> math::Rotation;

};








// Implementation


inline void VirtualTrackball::set_viewport_size(util::pixel::Size size) noexcept
{
    m_half_viewport_size = { double(size.width) / 2.0, double(size.height) / 2.0 };
    m_radius = std::min(m_half_viewport_size[0], m_half_viewport_size[1]);
}


inline void VirtualTrackball::acquire(Clock::time_point now) noexcept
{
    if (m_acquired)
        return;
    m_base_orientation = get_free_orientation(now);
    m_acquired = true;
    m_no_track_yet = true;
}


inline void VirtualTrackball::set_orientation(const math::Rotation& rot) noexcept
{
    m_base_orientation = rot;
    m_spin.angle = 0;
    m_acquired = false;
}


inline void VirtualTrackball::set_spin(const math::Rotation& spin, Clock::time_point now) noexcept
{
    m_base_orientation = get_orientation(now);
    m_release_time = now;
    m_acquired = false;
    m_spin = spin;
}


inline auto VirtualTrackball::get_orientation(Clock::time_point now) const noexcept -> math::Rotation
{
    return (m_acquired ? get_track_orientation() : get_free_orientation(now));
}


inline auto VirtualTrackball::get_free_orientation(Clock::time_point time) const noexcept -> math::Rotation
{
    if (m_spin.angle == 0)
        return m_base_orientation;
    std::chrono::duration<double> time_2 = time - m_release_time;
    return m_base_orientation + time_2.count() * m_spin;
}


inline auto VirtualTrackball::get_track_orientation() const noexcept -> math::Rotation
{
    if (ARCHON_LIKELY(!m_no_track_yet))
        return m_base_orientation + calc_rotation(m_first_track_point, get_ball_point(m_track_pos));
    return m_base_orientation;
}


inline auto VirtualTrackball::get_ball_point(const math::Vector2& pos) const noexcept -> math::Vector3
{
    math::Vector2 p = { pos[0] - m_half_viewport_size[0], m_half_viewport_size[1] - pos[1] };
    p /= m_radius;
    double s = math::sq_sum(p);

    // Clamp to unit disc
    if (1 < s) {
        p /= std::sqrt(s);
        s = 1;
    }

    return { p[0], p[1], std::sqrt(1 - s) };
}


inline auto VirtualTrackball::calc_rotation(const math::Vector3& a, const math::Vector3& b) noexcept -> math::Rotation
{
    math::Vector3 axis = math::cross(a, b);
    double ss = math::sq_sum(axis);
    double dp = math::dot(a, b);
    if (ARCHON_LIKELY(ss != 0 && dp <= 1))
        return { axis / std::sqrt(ss), std::acos(dp) };
    return {};
}


} // namespace archon::render

#endif // ARCHON_X_RENDER_X_VIRT_TRACKBALL_HPP
