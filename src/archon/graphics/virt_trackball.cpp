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

#include <sstream>

#include <archon/core/iterator.hpp>
#include <archon/graphics/virt_trackball.hpp>


using namespace archon::core;
using namespace archon::math;

namespace {

template<class T, std::size_t N> class FiniteSequenceMemory {
public:
    void push_back(const T& v)
    {
        if (len < N) {
            buf[len] = v;
            ++len;
        }
        else {
            buf[first] = v;
            if (++first == N)
                first = 0;
        }
    }

    void clear()
    {
        first = len = 0;
    }

    std::size_t length() const
    {
        return len;
    }

    bool empty() const
    {
        return len == 0;
    }

    const T& operator[](std::size_t i) const
    {
        i += first;
        return buf[i < N ? i : i-N];
    }

    using const_iterator = SubIter<const FiniteSequenceMemory, const T>;

    const_iterator begin() const
    {
        return const_iterator(this, 0);
    }

    const_iterator end() const
    {
        return const_iterator(this, len);
    }

    FiniteSequenceMemory()
    {
        clear();
    }

private:
    T buf[N];
    std::size_t first, len;
};

} // unnamed namespace


namespace archon {
namespace graphics {

template<class T> class VirtualTrackball::FiniteCurveMemory {
public:
    /// Value-time pairs must be given in order of non-decreasing time.
    void add_value(const T& v, long millis)
    {
        long barrier_index = millis / s_millis_per_barrier;
        if (m_samples.empty() || m_last_barrier_index < barrier_index) {
            m_samples.push_back(Sample{v, millis});
            m_last_barrier_index = barrier_index;
        }
    }

    void clear()
    {
        m_samples.clear();
    }

    T get_value(long millis) const
    {
        std::size_t i = lower_bound(millis);
        if (i == m_samples.length())
            return T();
        const Sample& b = m_samples[i];
        if (b.millis == millis || i == 0)
            return b.value;
        const Sample& a = m_samples[i-1];
        return lin_interp(millis, a.millis, b.millis, a.value, b.value);
    }

    void dump_info(std::ostream& out) const
    {
        out << "Current time barrier: "<<m_last_barrier_index*s_millis_per_barrier<<"\n";
        out << "Samples:\n";
        for (const Sample& s: m_samples)
            out << "  value = "<<s.value<<", millis = "<<s.millis<<"\n";
    }

private:
    // Higher number give more accuracy but slows down the search and uses more
    // memory. Must be at least 2.
    static constexpr std::size_t s_max_samples = 16;

    // Choosing the smallest number m such that m*(s_max_samples-1) >=
    // s_millis_back.
    static constexpr long s_millis_per_barrier = (s_millis_back+s_max_samples-2) / (s_max_samples-1);

    struct Sample {
        T value;
        long millis; // Offset from ball acquisition

        Sample() {}
        Sample(const T& v, long m): value(v), millis(m) {}
    };

    FiniteSequenceMemory<Sample, s_max_samples> m_samples;
    long m_last_barrier_index;

    std::size_t lower_bound(long millis) const
    {
        std::size_t i = 0, len = m_samples.length();
        while (0 < len) {
            std::size_t half = len >> 1;
            std::size_t mid = i + half;
            if (m_samples[mid].millis < millis) {
                i = mid;
                ++i;
                len -= half + 1;
            }
            else {
                len = half;
            }
        }
        return i;
    }
};


VirtualTrackball::VirtualTrackball():
    m_curve_mem(new FiniteCurveMemory<Vec2>)
{
    using namespace math;
    set_orientation(Rotation3::zero());
}


VirtualTrackball::~VirtualTrackball()
{
}


void VirtualTrackball::track(int x, int y, TrackTime track_time)
{
    if (!m_acquired)
        return;

    m_track_pos = Vec2(x,y);

    // Event times are translated such that the origin is at the time of
    // acquisition. This ensures that the milli second representation can fit in
    // a 32-bit signed integer (as long as a grab does not last more than 24
    // days).
    TrackTime track_time_2 = track_time;
    if (m_no_track_yet) {
        m_first_track_time = track_time_2;
        track_time_2 = TrackTime::zero();
        m_first_track_pos = m_track_pos;
        m_first_track_point = get_ball_point(m_track_pos);
        m_curve_mem->clear();
        m_no_track_yet = false;
    }
    else {
        track_time_2 -= m_first_track_time;
    }

    m_track_millis = long(track_time_2.count());
    m_curve_mem->add_value(m_track_pos, m_track_millis);

    m_need_track_orientation = true;
}


void VirtualTrackball::release(clock::time_point now)
{
    if (!m_acquired)
        return;

    // The all important job for this method is to determine the present
    // velocity of the mouse such that the continued spin of the ball can be
    // calculated.

    m_release_time = now;
    m_acquired = false;

    if (m_no_track_yet) {
        m_spin.angle = 0;
        return;
    }

    Vec3 last_point = get_ball_point(m_track_pos);
    set_track_orientation(last_point);
    m_base_orientation = m_track_orientation;

    // To apply a spin to the ball, we require that it was either acquired for
    // more than 200 milliseconds or the mouse was moved more than 4
    // pixels. Otherwise we take it as an attempt to stop the ball from
    // spinning.
    if (m_track_millis < 200 && dist(m_first_track_pos, m_track_pos) < 4) {
        m_spin.angle = 0;
        return;
    }

    // Get position of mouse 100 milliseconds before last known position.
    long millis = s_millis_back;
    // But be carefull not to extrapolate into the past before the ball was
    // acquired.
    if (m_track_millis < millis)
        millis = m_track_millis;
    Vec2 pos = m_curve_mem->get_value(m_track_millis - millis);

    Vec3 first_point = get_ball_point(pos);
    m_spin = calc_rotation(first_point, last_point);
    m_spin.angle *= 1E3/millis;
}


void VirtualTrackball::dump_info(std::ostream& out) const
{
    std::ostringstream out_2;
    out_2 << "----------------------------------------------\n";
    out_2 << "Current viewport: size = "<<(2.0*m_half_viewport_size)<<", radius = "<<m_radius<<"\n";
    out_2 << "Is acquired: "<<(m_acquired?"YES":"NO")<<"<n";
    out_2 << "Current base orientation: axis = "<<m_base_orientation.axis<<", angle = "<<m_base_orientation.angle<<"\n";
    Rotation3 r = get_orientation(clock::now());
    out_2 << "Current total orientation: axis = "<<r.axis<<", angle = "<<r.angle<<"<n";
    if (m_acquired) {
        out_2 << "First track time:       "<<m_first_track_time.count()<<"ms\n";
        out_2 << "First track point:      "<<m_first_track_point<<"<n";
        out_2 << "Current track millis:   "<<m_track_millis<<"ms\n";
        out_2 << "Current track position: "<<m_track_pos<<"\n";
        m_curve_mem->dump_info(out_2);
    }
    else {
        out_2 << "Current angular momentum: axis = "<<m_spin.axis<<", radians/second = "<<m_spin.angle<<"\n";
    }
    out_2 << "----------------------------------------------\n";
    out << out_2.str() << std::flush;
}

} // namespace graphics
} // namespace archon
