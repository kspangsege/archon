// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2024 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_DISPLAY_X_NOINST_X_TIMESTAMP_UNWRAPPER_HPP
#define ARCHON_X_DISPLAY_X_NOINST_X_TIMESTAMP_UNWRAPPER_HPP


#include <chrono>

#include <archon/core/integer.hpp>


namespace archon::display::impl {


// Recover timestamps damaged by wrap-around due to limited bit-width of representation.
//
// A timestamp unwrapper takes a timestamp that may have wrapped around any number of times
// since some previous timestamp due to limited bit-width in the representation of those
// timestamps and determines what the timestamp would have been had the integer
// representation been wide enough to avoid wrap-arounds.
//
// It does that by using corresponding timestamps from a different secondary clock with a
// wider range of timestamp representation. This is used as a second opinion on the amount
// of elapsed time, and allows for detection of wrap-arounds with near perfect fidelity so
// long as the secondary timestamps are obtained not too long after the primary
// timestamps. Here, "not too long" has a lot of give. If, for example, the representation
// of primary timestamps uses 32 bits, corresponding to a wrap-around circa every 50 days,
// the primary and secondary timestamps need only be within days of each other in order to
// achieve perfect fidelity of recovery.
//
// The assumption is that the timestamps from the secondary clock cannot simply be used
// instead of the original timestamps, because the original timestamps carry precise
// information that cannot be replicated through use of the secondary clock, for example,
// because the original timestamps are received over the network (think X11 timestamps).
//
// If `n` is the number of integer bits used in the representation of timestamps, and `t` is
// a timestamp, the idea is to estimate the number of wrap-arounds by determining the value
// `m` such that `m * 2^n + diff` is as close to `secondary_diff` as possible. Here, `diff`
// is the signed difference between a previous timestamp, `p`, and `t`, and `secondary_diff`
// is the "undamaged" difference between the corresponding timestamps from the secondary
// clock. Note that `diff` can be negative because of the possibility of wrap-arounds.
//
// The class operates under the assumption that both the primary and secondary clocks are
// monotonic. If they are not, behavior is unspecified.
//
// The type, `T`, which is used for storage of primary timestamps, must be an unsigned
// integer type.
//
// The parameter, `N`, is the number of bits of `T` that are used for the timestamp
// representation. It is an error if `N` is greater than the number of value bits in `T`.
//
// This class assumes that primary timestamps, as they are passed to
// `Session::unwrap_next_timestamp()`, are always exactly the `N` lowest-order bits of the
// true "undamaged" original timestamp (regardless of whether undamaged versions of those
// timestamps ever existed).
//
template<class T, int N> class TimestampUnwrapper {
public:
    using timestamp_type = T;

    static constexpr int width = N;

    static_assert(N <= core::num_value_bits<T>());

    class Session;

    using millis_type     = std::chrono::milliseconds;
    using clock_type      = std::chrono::steady_clock;
    using time_point_type = std::chrono::time_point<clock_type>;

    TimestampUnwrapper() noexcept = default;

private:
    bool m_have_start_timestamps = false;
    timestamp_type m_start_timestamp = {};
    time_point_type m_start_timestamp_2 = {};

    auto unwrap_next_timestamp(T timestamp, time_point_type timestamp_2) -> millis_type;
};


// A session must only last for a short amount of time, i.e., only while processing a batch
// of immediately available events.
//
template<class T, int N>
class TimestampUnwrapper<T, N>::Session {
public:
    Session(TimestampUnwrapper&, time_point_type now = clock_type::now()) noexcept;

    void reset_now(time_point_type now = clock_type::now()) noexcept;

    auto unwrap_next_timestamp(T timestamp) -> millis_type;

private:
    TimestampUnwrapper& m_unwrapper;
    time_point_type m_now;
};








// Implementation


template<class T, int N>
auto TimestampUnwrapper<T, N>::unwrap_next_timestamp(T timestamp, time_point_type timestamp_2) -> millis_type
{
    ARCHON_ASSERT(timestamp <= core::int_mask<T>(N));
    using millis_rep_type = millis_type::rep;
    constexpr int n = core::num_value_bits<millis_rep_type>();
    if constexpr (N < n) {
        if (ARCHON_LIKELY(m_have_start_timestamps)) {
            // Second-opinion clock must be monotonic
            ARCHON_ASSERT(timestamp_2 >= m_start_timestamp_2);
            millis_rep_type millis = std::chrono::round<millis_type>(timestamp_2 - m_start_timestamp_2).count();
            constexpr millis_rep_type module = millis_rep_type(1) << N;
            millis_rep_type diff = millis_rep_type(timestamp) - millis_rep_type(m_start_timestamp);
            core::int_add(millis, module / 2 - diff); // Throws
            millis_rep_type offset = (millis / module) * module;
            // Overflow not possible below because `offset` has its N lower order bits equal
            // to zero and `timestamp` is a value representable in N bits. `offset` has its
            // N lower order bits equal to zero because it is an integer multiple of
            // `module`.
            return millis_type(offset + millis_rep_type(timestamp));
        }

        m_start_timestamp = timestamp;
        m_start_timestamp_2 = timestamp_2;
        m_have_start_timestamps = true;
    }
    else {
        ARCHON_ASSERT(timestamp <= core::int_mask<T>(n));
    }
    return millis_type(millis_rep_type(timestamp));
}


template<class T, int N>
inline TimestampUnwrapper<T, N>::Session::Session(TimestampUnwrapper& unwrapper, time_point_type now) noexcept
    : m_unwrapper(unwrapper)
    , m_now(now)
{
}


template<class T, int N>
inline void TimestampUnwrapper<T, N>::Session::reset_now(time_point_type now) noexcept
{
    m_now = now;
}


template<class T, int N>
inline auto TimestampUnwrapper<T, N>::Session::unwrap_next_timestamp(T timestamp) -> millis_type
{
    return m_unwrapper.unwrap_next_timestamp(timestamp, m_now); // Throws
}


} // namespace archon::display::impl

#endif // ARCHON_X_DISPLAY_X_NOINST_X_TIMESTAMP_UNWRAPPER_HPP
