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


#include <cstddef>
#include <ostream>
#include <sstream>

#include <archon/math/vector.hpp>
#include <archon/render/virt_trackball.hpp>


using namespace archon;
using render::VirtualTrackball;


namespace {

// Considder the last 100 milliseconds of the mouse movement when determining the free spin.
constexpr long g_millis_back = 100;

// Higher number give more accuracy but slows down the search and uses more memory. Must be
// at least 2.
constexpr std::size_t g_max_samples = 16;

} // unnamed namespace


VirtualTrackball::VirtualTrackball()
    : m_curve_mem(g_millis_back, g_max_samples) // Throws
{
}


void VirtualTrackball::set_viewport_size(util::pixel::Size size) noexcept
{
    m_half_viewport_size = {
        double(size.width) / 2.0,
        double(size.height) / 2.0,
    };
    update_radii();
}


void VirtualTrackball::set_pixel_aspect_ratio(double ratio) noexcept
{
    m_pixel_aspect_ratio = ratio;
    update_radii();
}


void VirtualTrackball::track(util::pixel::Pos pos, TrackTime track_time) noexcept
{
    if (!m_acquired)
        return;

    m_track_pos = { double(pos.x), double(pos.y) };

    // Event times are translated such that the origin is at the time of acquisition. This
    // ensures that the milli second representation can fit in a 32-bit signed integer (as
    // long as a grab does not last more than 24 days).
    TrackTime track_time_2 = track_time;
    if (m_no_track_yet) {
        m_first_track_time = track_time_2;
        track_time_2 = TrackTime::zero();
        m_first_track_pos = m_track_pos;
        m_first_track_point = get_ball_point(m_track_pos);
        m_curve_mem.clear();
        m_no_track_yet = false;
    }
    else {
        track_time_2 -= m_first_track_time;
    }

    m_track_millis = long(track_time_2.count());
    m_curve_mem.add_value(m_track_pos, m_track_millis);
}


void VirtualTrackball::release(Clock::time_point now) noexcept
{
    if (!m_acquired)
        return;

    // The all important job for this method is to determine the present velocity of the
    // mouse such that the continued spin of the ball can be calculated.

    m_release_time = now;
    m_acquired = false;

    if (m_no_track_yet) {
        m_spin.angle = 0;
        return;
    }

    math::Vector3 last_point = get_ball_point(m_track_pos);
    m_base_orientation += calc_rotation(m_first_track_point, last_point);

    // To apply a spin to the ball, we require that it was either acquired for more than 200
    // milliseconds or the mouse was moved more than 4 pixels. Otherwise we take it as an
    // attempt to stop the ball from spinning.
    if (m_track_millis < 200 && math::len(m_first_track_pos - m_track_pos) < 4) {
        m_spin.angle = 0;
        return;
    }

    // Get position of mouse 100 milliseconds before last known position.
    long millis = g_millis_back;
    // But be carefull not to extrapolate into the past before the ball was acquired.
    if (m_track_millis < millis)
        millis = m_track_millis;
    math::Vector2 pos = m_curve_mem.get_value(m_track_millis - millis);

    math::Vector3 first_point = get_ball_point(pos);
    m_spin = calc_rotation(first_point, last_point);
    m_spin *= 1000.0 / millis;
}


void VirtualTrackball::dump_info(std::ostream& out) const
{
    std::ostringstream out_2; // Throws
    out_2 << "----------------------------------------------\n"; // Throws
    out_2 << "Half viewport size:        " << m_half_viewport_size << "\n"; // Throws
    out_2 << "Pixel aspect ratio:        " << m_pixel_aspect_ratio << "\n"; // Throws
    out_2 << "Radius:                    " << math::Vector2(m_horz_radius, m_vert_radius) << "\n"; // Throws
    out_2 << "Is acquired:               " << (m_acquired ? "YES" : "NO") << "\n"; // Throws
    out_2 << "Current base orientation:  " << m_base_orientation << "\n"; // Throws
    out_2 << "Current total orientation: " << get_orientation(Clock::now()) << "\n"; // Throws
    if (m_acquired) {
        out_2 << "First track time:          " << m_first_track_time.count() << "ms\n"; // Throws
        out_2 << "First track point:         " << m_first_track_point << "\n"; // Throws
        out_2 << "Current track millis:      " << m_track_millis << "ms\n"; // Throws
        out_2 << "Current track position:    " << m_track_pos << "\n"; // Throws
        m_curve_mem.dump_info(out_2); // Throws
    }
    else {
        out_2 << "Current angular momentum:  " << m_spin << "\n"; // Throws
    }
    out_2 << "----------------------------------------------\n"; // Throws
    out << out_2.str() << std::flush; // Throws
}
