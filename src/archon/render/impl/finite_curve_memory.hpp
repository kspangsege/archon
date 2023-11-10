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

#ifndef ARCHON_X_RENDER_X_IMPL_X_FINITE_CURVE_MEMORY_HPP
#define ARCHON_X_RENDER_X_IMPL_X_FINITE_CURVE_MEMORY_HPP

/// \file


#include <cstddef>
#include <ostream>

#include <archon/core/integer.hpp>
#include <archon/core/math.hpp>
#include <archon/render/impl/finite_sequence_memory.hpp>


namespace archon::render::impl {


template<class T> class FiniteCurveMemory {
public:
    // A higher maximum number of samples (max_samples) gives more accuracy but slows down
    // the search and uses more memory. Must be at least 2.
    FiniteCurveMemory(long millis_back, std::size_t max_samples);

    // Value-time pairs must be given in order of non-decreasing time.
    void add_value(const T& v, long millis) noexcept;

    void clear() noexcept;

    auto get_value(long millis) const noexcept -> T;

    void dump_info(std::ostream& out) const;

private:
    struct Sample {
        T value;
        long millis;
    };

    impl::FiniteSequenceMemory<Sample> m_samples;
    long m_millis_per_barrier = 0;
    long m_last_barrier_index = 0;

    auto lower_bound(long millis) const noexcept -> std::size_t;
};








// Implementation


template<class T>
FiniteCurveMemory<T>::FiniteCurveMemory(long millis_back, std::size_t max_samples)
    : m_samples(max_samples) // Throws
{
    // Choosing the smallest number N such that N * (max_samples - 1) >= millis_back
    m_millis_per_barrier = core::int_div_round_up(millis_back, max_samples - 1);
}


template<class T>
void FiniteCurveMemory<T>::add_value(const T& v, long millis) noexcept
{
    long barrier_index = millis / m_millis_per_barrier;
    if (m_samples.empty() || m_last_barrier_index < barrier_index) {
        m_samples.push_back({ v, millis });
        m_last_barrier_index = barrier_index;
    }
}


template<class T>
inline void FiniteCurveMemory<T>::clear() noexcept
{
    m_samples.clear();
}


template<class T>
auto FiniteCurveMemory<T>::get_value(long millis) const noexcept -> T
{
    std::size_t i = lower_bound(millis);
    if (i == m_samples.size())
        return {};
    const Sample& b = m_samples[i];
    if (b.millis == millis || i == 0)
        return b.value;
    const Sample& a = m_samples[i - 1];
    return core::lerp_a(a.millis, a.value, b.millis, b.value, millis);
}


template<class T>
void FiniteCurveMemory<T>::dump_info(std::ostream& out) const
{
    out << "Current time barrier: " << m_last_barrier_index * m_millis_per_barrier << "\n"; // Throws
    out << "Samples:\n"; // Throws
    for (const Sample& s : m_samples)
        out << "  value = " << s.value << ", millis = " << s.millis << "\n"; // Throws
}


template<class T>
auto FiniteCurveMemory<T>::lower_bound(long millis) const noexcept -> std::size_t
{
    std::size_t i = 0, size = m_samples.size();
    while (size > 0) {
        std::size_t half = size / 2;
        std::size_t mid = i + half;
        if (m_samples[mid].millis < millis) {
            i = mid;
            ++i;
            size -= half + 1;
        }
        else {
            size = half;
        }
    }
    return i;
}


} // namespace archon::render::impl

#endif // ARCHON_X_RENDER_X_IMPL_X_FINITE_CURVE_MEMORY_HPP
