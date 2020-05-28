// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_CORE_X_TIMESTAMP_FORMATTER_HPP
#define ARCHON_X_CORE_X_TIMESTAMP_FORMATTER_HPP

/// \file


#include <cstddef>
#include <iterator>
#include <string_view>
#include <locale>
#include <ostream>

#include <archon/core/features.h>
#include <archon/core/time.hpp>
#include <archon/core/char_mapper.hpp>
#include <archon/core/seed_memory_output_stream.hpp>
#include <archon/core/integer_formatter.hpp>


namespace archon::core {


/// \brief Timestamp formatter base class.
///
/// This is the base class for timestamp formatters (\ref BasicTimestampFormatter).
///
class TimestampFormatterBase {
public:
    /// \brief Available subsecond precisions.
    ///
    /// These are the possible choices for subsecond precision when formatting timestamps.
    ///
    enum class Precision { seconds, milliseconds, microseconds, nanoseconds };

    struct Params;
};




/// \brief Timestamp formatting parameters.
///
/// These are the parameters that can be passed to \ref BasicTimestampFormatter::format(),
/// and friends, to control the formatiing process.
///
struct TimestampFormatterBase::Params {
    /// \brief Subsecond precision.
    ///
    /// This specifies the subsecond precision for timestamp formatting operations.
    ///
    Precision precision = Precision::seconds;

    /// \brief Timestamp format.
    ///
    /// If nonempty, this specifies how timestamps are to be formatted. The syntax of the
    /// string is as understood by `std::put_time()` (e.g., `"%FT%T%z"`), except that the
    /// first occurrence of `%S` (also taking into account the `%S` that is an implicit part
    /// of `%T`) is expanded to `SS.fff` if \ref precision is Precision::milliseconds, or to
    /// `SS.ffffff` if \ref precision is Precision::microseconds, or to `SS.fffffffff` if
    /// \ref precision is Precision::nanoseconds, where `SS` is what `%S` expands to
    /// conventionally, i.e., according to `std::put_time()`.
    ///
    /// If empty (the default), an effective format is chosen depending on context. For
    /// instance, see \ref BasicTimestampFormatter::format_local().
    ///
    std::string_view format;
};




/// \brief A timestamp formatter with support for subsecond precision.
///
/// A timestamp formatter that extends the usual formatting capabilities of
/// `std::put_time()` with subsecond precision (\ref Precision).
///
template<class C, class T = std::char_traits<C>> class BasicTimestampFormatter
    : public TimestampFormatterBase
    , private BasicCharMapper<C, T> {
public:
    using string_view_type = std::basic_string_view<C, T>;

    /// \{
    ///
    /// \brief Format timestamp in local time zone.
    ///
    /// Format the specified point in time as a timestamp expressed in the local time
    /// zone. The default timestamp format is `"%FT%T%z"` (see \ref Params::format).
    ///
    /// Arguments of type `std::time_t` and `std::timespec` are expressed as seconds, or
    /// nanoseconds since the Epoch (Unix time).
    ///
    auto format_local(std::time_t, const Params& = {}) -> string_view_type;
    auto format_local(core::timespec_type, const Params& = {}) -> string_view_type;
    template<class D> auto format_local(std::chrono::time_point<D>, const Params& = {}) -> string_view_type;
    /// \}

    /// \{
    ///
    /// \brief Format timestamp in UTC.
    ///
    /// Format the specified point in time as a timestamp expressed in UTC (Coordinated
    /// Universal Time). The default timestamp format is `"%FT%TZ"` (see \ref
    /// Params::format).
    ///
    /// Arguments of type `std::time_t` and `std::timespec` are expressed as seconds, or
    /// nanoseconds since the Epoch (Unix time).
    ///
    auto format_utc(std::time_t, const Params& = {}) -> string_view_type;
    auto format_utc(core::timespec_type, const Params& = {}) -> string_view_type;
    template<class D> auto format_utc(std::chrono::time_point<D>, const Params& = {}) -> string_view_type;
    /// \}

    /// \brief Format broken down time.
    ///
    /// Format the specifed broken down time as described by the specified parameters. The
    /// default timestamp format is `"%FT%T%z"` (see \ref Params::format).
    ///
    auto format(const std::tm& time, long nanoseconds = 0, const Params& = {}) -> string_view_type;

    /// \brief Construct a timestamp formatter.
    ///
    /// Construct a timestamp formatter imbued with the specified locale.
    ///
    explicit BasicTimestampFormatter(const std::locale& = {});

private:
    static constexpr std::size_t s_widen_seed_memory_size = 20;
    static constexpr std::size_t s_streambuf_seed_memory_size = 72;

    using char_mapper_type = BasicCharMapper<C, T>;
    using WidenBuffer = typename char_mapper_type::template ArraySeededWidenBuffer<s_widen_seed_memory_size>;

    WidenBuffer m_widen_buffer;
    C m_streambuf_seed_memory[s_streambuf_seed_memory_size] = {};
    BasicSeedMemoryOutputStream<C, T> m_out;

    auto do_format(const std::tm&, long nanoseconds, std::string_view format, Precision precision) -> string_view_type;
};


using TimestampFormatter     = BasicTimestampFormatter<char>;
using WideTimestampFormatter = BasicTimestampFormatter<wchar_t>;








// Implementation


template<class C, class T>
inline auto BasicTimestampFormatter<C, T>::format_local(std::time_t time, const Params& params) -> string_view_type
{
    return format_local(core::timespec_type { time, 0 }, params); // Throws
}


template<class C, class T>
inline auto BasicTimestampFormatter<C, T>::format_local(core::timespec_type time, const Params& params) ->
    string_view_type
{
    std::string_view format = params.format;
    if (format.empty())
        format = "%FT%T%z";
    return do_format(time_breakdown_local(time.tv_sec), time.tv_nsec, format, params.precision); // Throws
}


template<class C, class T>
template<class D>
inline auto BasicTimestampFormatter<C, T>::format_local(std::chrono::time_point<D> time, const Params& params) ->
    string_view_type
{
    return format_local(time_point_to_timespec(time), params); // Throws
}


template<class C, class T>
inline auto BasicTimestampFormatter<C, T>::format_utc(std::time_t time, const Params& params) -> string_view_type
{
    return format_utc(core::timespec_type { time, 0 }, params); // Throws
}


template<class C, class T>
inline auto BasicTimestampFormatter<C, T>::format_utc(core::timespec_type time, const Params& params) ->
    string_view_type
{
    std::string_view format = params.format;
    if (format.empty())
        format = "%FT%TZ";
    return do_format(time_breakdown_utc(time.tv_sec), time.tv_nsec, format, params.precision); // Throws
}


template<class C, class T>
template<class D>
inline auto BasicTimestampFormatter<C, T>::format_utc(std::chrono::time_point<D> time, const Params& params) ->
    string_view_type
{
    return format_utc(time_point_to_timespec(time), params); // Throws
}


template<class C, class T>
auto BasicTimestampFormatter<C, T>::format(const std::tm& time, long nanoseconds, const Params& params) ->
    string_view_type
{
    std::string_view format = params.format;
    if (format.empty())
        format = "%FT%T%z";
    return do_format(time, nanoseconds, format, params.precision); // Throws
}


template<class C, class T>
BasicTimestampFormatter<C, T>::BasicTimestampFormatter(const std::locale& locale)
    : BasicCharMapper<C, T>(locale) // Throws
    , m_out(m_streambuf_seed_memory) // Throws
{
    m_out.exceptions(std::ios_base::badbit | std::ios_base::failbit);
    m_out.imbue(locale);
}


template<class C, class T>
auto BasicTimestampFormatter<C, T>::do_format(const std::tm& time, long nanoseconds, std::string_view format,
                                              Precision precision) -> string_view_type
{
    m_out.full_clear();
    int subsecond_digits = 0;
    long subsecond_value = 0;
    switch (precision) {
        case Precision::seconds:
            break;
        case Precision::milliseconds:
            subsecond_digits = 3;
            subsecond_value = nanoseconds / 1000000;
            break;
        case Precision::microseconds:
            subsecond_digits = 6;
            subsecond_value = nanoseconds / 1000;
            break;
        case Precision::nanoseconds:
            subsecond_digits = 9;
            subsecond_value = nanoseconds;
            break;
    }
    const auto& time_put = std::use_facet<std::time_put<C>>(m_out.getloc()); // Throws
    auto format_part = [&](std::size_t i, std::size_t j) {
        std::size_t n = j - i;
        string_view_type part = this->widen(format.substr(i, n), m_widen_buffer); // Throws
        const C* begin = part.data();
        const C* end = begin + part.size();
        time_put.put(std::ostreambuf_iterator<C>(&m_out.streambuf()), m_out, this->widen(' '), &time,
                     begin, end); // Throws
    };
    std::size_t i_0 = 0;
    std::size_t n = format.size();
    if (subsecond_digits > 0) {
        std::size_t i_1 = 0;
        for (;;) {
            std::size_t i_2 = format.find('%', i_1);
            if (ARCHON_LIKELY(i_2 != std::string_view::npos && ++i_2 < n)) {
                char ch = format[i_2++];
                if (ARCHON_LIKELY(ch != 'S' && ch != 'T'))
                    goto next;
                format_part(i_0, i_2); // Throws
                BasicIntegerFormatter<C, T> integer_formatter(*this);
                m_out << this->widen('.'); // Throws
                m_out << integer_formatter.format(subsecond_value, subsecond_digits); // Throws
                i_0 = i_2;
                goto next;
            }
            break;
          next:
            i_1 = i_2;
        }
    }
    format_part(i_0, n); // Throws
    return m_out.view();
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_TIMESTAMP_FORMATTER_HPP
