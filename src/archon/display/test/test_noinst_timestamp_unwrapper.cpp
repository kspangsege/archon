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


#include <ratio>
#include <chrono>

#include <archon/core/features.h>
#include <archon/core/integer.hpp>
#include <archon/check.hpp>
#include <archon/display/noinst/timestamp_unwrapper.hpp>


using namespace archon;
namespace impl = display::impl;


namespace {


template<class T> constexpr auto int_scale(T val, T num, T denom) noexcept -> T
{
    ARCHON_ASSERT(num >= 0);
    ARCHON_ASSERT(denom > 0);
    ARCHON_ASSERT(num <= denom);
    ARCHON_ASSERT(denom <= T(1) << core::num_value_bits<T>() / 2);
    return num * (val / denom) + core::int_div_round_half_down((num * (val % denom)), denom);
}


ARCHON_TEST_VARIANTS(variants,
                     ARCHON_TEST_TYPE_AND_VALUE(signed char,        0, FullSignedChar),
                     ARCHON_TEST_TYPE_AND_VALUE(signed char,        1, ReducedSignedChar),
                     ARCHON_TEST_TYPE_AND_VALUE(signed char,        2, DoublyReducedSignedChar),
                     ARCHON_TEST_TYPE_AND_VALUE(unsigned char,      0, FullUnsignedChar),
                     ARCHON_TEST_TYPE_AND_VALUE(unsigned char,      1, ReducedUnsignedChar),
                     ARCHON_TEST_TYPE_AND_VALUE(unsigned char,      2, DoublyReducedUnsignedChar),
                     ARCHON_TEST_TYPE_AND_VALUE(signed short,       0, FullSignedShort),
                     ARCHON_TEST_TYPE_AND_VALUE(signed short,       1, ReducedSignedShort),
                     ARCHON_TEST_TYPE_AND_VALUE(signed short,       2, DoublyReducedSignedShort),
                     ARCHON_TEST_TYPE_AND_VALUE(unsigned short,     0, FullUnsignedShort),
                     ARCHON_TEST_TYPE_AND_VALUE(unsigned short,     1, ReducedUnsignedShort),
                     ARCHON_TEST_TYPE_AND_VALUE(unsigned short,     2, DoublyReducedUnsignedShort),
                     ARCHON_TEST_TYPE_AND_VALUE(signed int,         0, FullSignedInt),
                     ARCHON_TEST_TYPE_AND_VALUE(signed int,         1, ReducedSignedInt),
                     ARCHON_TEST_TYPE_AND_VALUE(signed int,         2, DoublyReducedSignedInt),
                     ARCHON_TEST_TYPE_AND_VALUE(unsigned int,       0, FullUnsignedInt),
                     ARCHON_TEST_TYPE_AND_VALUE(unsigned int,       1, ReducedUnsignedInt),
                     ARCHON_TEST_TYPE_AND_VALUE(unsigned int,       2, DoublyReducedUnsignedInt),
                     ARCHON_TEST_TYPE_AND_VALUE(signed long,        0, FullSignedLong),
                     ARCHON_TEST_TYPE_AND_VALUE(signed long,        1, ReducedSignedLong),
                     ARCHON_TEST_TYPE_AND_VALUE(signed long,        2, DoublyReducedSignedLong),
                     ARCHON_TEST_TYPE_AND_VALUE(unsigned long,      0, FullUnsignedLong),
                     ARCHON_TEST_TYPE_AND_VALUE(unsigned long,      1, ReducedUnsignedLong),
                     ARCHON_TEST_TYPE_AND_VALUE(unsigned long,      2, DoublyReducedUnsignedLong),
                     ARCHON_TEST_TYPE_AND_VALUE(signed long long,   0, FullSignedLongLong),
                     ARCHON_TEST_TYPE_AND_VALUE(signed long long,   1, ReducedSignedLongLong),
                     ARCHON_TEST_TYPE_AND_VALUE(signed long long,   2, DoublyReducedSignedLongLong),
                     ARCHON_TEST_TYPE_AND_VALUE(unsigned long long, 0, FullUnsignedLongLong),
                     ARCHON_TEST_TYPE_AND_VALUE(unsigned long long, 1, ReducedUnsignedLongLong),
                     ARCHON_TEST_TYPE_AND_VALUE(unsigned long long, 2, DoublyReducedUnsignedLongLong));


template<class T, int R> struct Fixture {
public:
    using timestamp_type = T;

    static constexpr int reduction = R;

    static constexpr int width = core::num_value_bits<timestamp_type>() - reduction;

    using unwrapper_type = impl::TimestampUnwrapper<timestamp_type, width>;
    using time_point_type = typename unwrapper_type::time_point_type;
    using duration_type = typename time_point_type::duration;
    using millis_type = typename unwrapper_type::millis_type;
    using millis_rep_type = typename millis_type::rep;

    static constexpr bool can_unwrap_with(millis_rep_type millis_after_start) noexcept
    {
        static_assert(std::ratio_less_equal_v<typename duration_type::period, typename millis_type::period>);
        millis_type max = std::chrono::round<millis_type>(duration_type::max() - s_max_start_time.time_since_epoch());
        return (millis_after_start <= max.count());
    }

    Fixture(int num_halves_of_max_start_time) noexcept
        : m_start_time(determine_start_time(num_halves_of_max_start_time))
    {
        ARCHON_ASSERT(m_start_time <= s_max_start_time);
    }

    auto unwrap(timestamp_type timestamp, millis_rep_type millis_after_start) -> millis_rep_type
    {
        time_point_type now = m_start_time + millis_type(millis_after_start);
        using session_type = typename unwrapper_type::Session;
        session_type session(m_unwrapper, now);
        return session.unwrap_next_timestamp(timestamp).count(); // Throws
    }

private:
    static constexpr time_point_type s_max_start_time = time_point_type(duration_type::max() / 2);

    unwrapper_type m_unwrapper;
    time_point_type m_start_time;

    static auto determine_start_time(int num_halves_of_max_start_time) noexcept -> time_point_type
    {
        millis_rep_type ticks = int_scale<millis_rep_type>(s_max_start_time.time_since_epoch().count(),
                                                           num_halves_of_max_start_time, 2);
        return time_point_type(duration_type(ticks));
    }
};


} // unnamed namespace



ARCHON_TEST_BATCH(Display_Noinst_TimestampUnwrapper, variants)
{
    using timestamp_type = test_type;

    constexpr int reduction = test_value;

    using fixture_type = Fixture<timestamp_type, reduction>;
    using millis_rep_type = typename fixture_type::millis_rep_type;

    test_context.logger.trace("timestamp width: %s", fixture_type::width);
    test_context.logger.trace("num value bits in millis type: %s", core::num_value_bits<millis_rep_type>());

    if (ARCHON_UNLIKELY(fixture_type::width >= core::num_value_bits<millis_rep_type>())) {
        test_context.logger.trace("bail 1");
        return;
    }

    timestamp_type max = core::int_mask<timestamp_type>(fixture_type::width);
    millis_rep_type mod = millis_rep_type(max) + 1;

    // Below, `mod + (mod - 1)` is guaranteed to not overflow

    if (ARCHON_UNLIKELY(!fixture_type::can_unwrap_with(mod + (mod - 1)))) {
        test_context.logger.trace("bail 2");
        return;
    }

    {
        fixture_type fixture(0);
        ARCHON_CHECK_EQUAL(fixture.unwrap(max, 0), mod - 1);
        ARCHON_CHECK_EQUAL(fixture.unwrap(0, 1), mod);
    } {
        fixture_type fixture(0);
        ARCHON_CHECK_EQUAL(fixture.unwrap(0, 0), 0);
        ARCHON_CHECK_EQUAL(fixture.unwrap(1, 1), 1);
        ARCHON_CHECK_EQUAL(fixture.unwrap(2, 2), 2);
        ARCHON_CHECK_EQUAL(fixture.unwrap(max, mod - 1), mod - 1);
        ARCHON_CHECK_EQUAL(fixture.unwrap(0, mod), mod);
    } {
        fixture_type fixture(1);
        ARCHON_CHECK_EQUAL(fixture.unwrap(0, 0), 0);
        ARCHON_CHECK_EQUAL(fixture.unwrap(1, 1), 1);
        ARCHON_CHECK_EQUAL(fixture.unwrap(2, 2), 2);
        ARCHON_CHECK_EQUAL(fixture.unwrap(max, mod - 1), mod - 1);
        ARCHON_CHECK_EQUAL(fixture.unwrap(0, mod), mod);
    } {
        fixture_type fixture(2);
        ARCHON_CHECK_EQUAL(fixture.unwrap(0, 0), 0);
        ARCHON_CHECK_EQUAL(fixture.unwrap(1, 1), 1);
        ARCHON_CHECK_EQUAL(fixture.unwrap(2, 2), 2);
        ARCHON_CHECK_EQUAL(fixture.unwrap(max, mod - 1), mod - 1);
        ARCHON_CHECK_EQUAL(fixture.unwrap(0, mod), mod);
    } {
        fixture_type fixture(0);
        ARCHON_CHECK_EQUAL(fixture.unwrap(0, 0), 0);
        ARCHON_CHECK_EQUAL(fixture.unwrap(0, 0), 0);
        ARCHON_CHECK_EQUAL(fixture.unwrap(1, 1), 1);
        ARCHON_CHECK_EQUAL(fixture.unwrap(1, 1), 1);
        ARCHON_CHECK_EQUAL(fixture.unwrap(2, 2), 2);
        ARCHON_CHECK_EQUAL(fixture.unwrap(max, mod - 1), mod - 1);
        ARCHON_CHECK_EQUAL(fixture.unwrap(max, mod - 1), mod - 1);
        ARCHON_CHECK_EQUAL(fixture.unwrap(0, mod), mod);
        ARCHON_CHECK_EQUAL(fixture.unwrap(0, mod), mod);
        ARCHON_CHECK_EQUAL(fixture.unwrap(1, mod + 1), mod + 1);
        ARCHON_CHECK_EQUAL(fixture.unwrap(max, mod + (mod - 1)), mod + (mod - 1));
    } {
        fixture_type fixture(0);
        ARCHON_CHECK_EQUAL(fixture.unwrap(0, 0), 0);
        ARCHON_CHECK_EQUAL(fixture.unwrap(0, 1), 0);
        ARCHON_CHECK_EQUAL(fixture.unwrap(1, 1), 1);
        ARCHON_CHECK_EQUAL(fixture.unwrap(1, 1), 1);
        ARCHON_CHECK_EQUAL(fixture.unwrap(2, 1), 2);
        ARCHON_CHECK_EQUAL(fixture.unwrap(2, 2), 2);
        ARCHON_CHECK_EQUAL(fixture.unwrap(2, 3), 2);
        ARCHON_CHECK_EQUAL(fixture.unwrap(max, mod - 2), mod - 1);
        ARCHON_CHECK_EQUAL(fixture.unwrap(max, mod - 1), mod - 1);
        ARCHON_CHECK_EQUAL(fixture.unwrap(max, mod - 0), mod - 1);
        ARCHON_CHECK_EQUAL(fixture.unwrap(0, mod + 0), mod);
        ARCHON_CHECK_EQUAL(fixture.unwrap(0, mod + 1), mod);
        ARCHON_CHECK_EQUAL(fixture.unwrap(1, mod + 1), mod + 1);
        ARCHON_CHECK_EQUAL(fixture.unwrap(1, mod + 2), mod + 1);
        ARCHON_CHECK_EQUAL(fixture.unwrap(max, mod + (mod - 2)), mod + (mod - 1));
        ARCHON_CHECK_EQUAL(fixture.unwrap(max, mod + (mod - 1)), mod + (mod - 1));
    }
}
