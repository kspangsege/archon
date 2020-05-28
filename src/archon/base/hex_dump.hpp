// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef ARCHON_X_BASE_X_HEX_DUMP_HPP
#define ARCHON_X_BASE_X_HEX_DUMP_HPP

/// \file


#include <cstddef>
#include <type_traits>
#include <algorithm>
#include <string_view>
#include <ostream>

#include <archon/base/features.h>
#include <archon/base/type_traits.hpp>
#include <archon/base/integer.hpp>
#include <archon/base/span.hpp>
#include <archon/base/char_mapper.hpp>
#include <archon/base/stream_output.hpp>
#include <archon/base/integer_formatter.hpp>


namespace archon::base {


/// \brief Hex dump parameters.
///
/// These are the available parameters for controlling the output of hex dump
/// operations.
///
struct HexDumpConfig {
    /// \brief The minimum number of digits to generate for each data element.
    ///
    /// If the value is less than, or equal to zero, the number of digits will
    /// be determined by the type of dumped data.
    ///
    int min_digits = 0;

    /// If specified (not equal to `std::size_t(-1)`), the total size of the hex
    /// dump will be limited to the specified size. This is done by replacing a
    /// trailing section of the specified data sequence by an ellipsis (`...`).
    ///
    std::size_t max_size = std::size_t(-1);
};


template<class C, class T, class I>
void hex_dump(std::basic_ostream<C, T>&, base::Span<const I> data, HexDumpConfig = {});

template<class C, class T, class I>
void hex_dump(std::basic_ostream<C, T>&, base::Span<const I> data,
              std::basic_string_view<C, T> separator, HexDumpConfig = {});

/// \{
///
/// \brief Dump data in hexadecimal form.
///
/// Construct an object that, if written to an output stream, formats the
/// specified sequence of integers according to the specified parameters. Each
/// integer will be expressed in hexadecimal form.
///
/// The field width of the target stream will be respected, and the effect will
/// be as if all of the generated output was written to the stream as a single
/// string object.
///
template<class I> auto as_hex_dump(base::Span<const I> data, HexDumpConfig = {});
template<class C, class T, class I>
auto as_hex_dump(base::Span<const I> data, std::basic_string_view<C, T> separator,
                 HexDumpConfig = {});
/// \}








// Implementation


namespace detail {


template<class C, class T, class I>
void do_hex_dump(base::BasicStreamOutputHelper<C, T>& helper, const BasicCharMapper<C, T>& char_mapper,
                 base::Span<const I> data, std::basic_string_view<C, T> separator,
                 HexDumpConfig config)
{
    const std::size_t ellipsis_size = 3;

    constexpr int int_width = base::get_int_width<I>();
    using uint_type = base::FastestUnsignedWithBits<int_width>;
    static_assert(!std::is_same_v<uint_type, void>); // A hope more than a certainty
    uint_type mask = base::int_mask<uint_type>(int_width);

    int min_digits = config.min_digits;
    if (min_digits <= 0)
        min_digits = base::int_digits(mask, 16);

    const I* i   =     data.data();
    const I* end = i + data.size();

    if (ARCHON_LIKELY(i != end)) {
        base::BasicIntegerFormatter integer_formatter(char_mapper);
        auto format = [&] {
            auto value = uint_type(*i) & mask;
            return integer_formatter.format_hex(value, min_digits); // Throws
        };
        using string_view_type = std::basic_string_view<C, T>;
        string_view_type str = format(); // Throws
        std::size_t max_size, size;
        const I* j;
        if (ARCHON_LIKELY(config.max_size != std::size_t(-1))) {
            max_size = std::max(config.max_size, ellipsis_size);
            size = str.size();
            if (ARCHON_LIKELY(size <= max_size - ellipsis_size)) {
                helper.write(str); // Throws
                max_size -= size;
                ++i;
                if (ARCHON_LIKELY(i != end))
                    goto next;
                return;
            }
            j = i;
            goto look_ahead;
        }
        goto unlimited_1;

      next:
        str = format(); // Throws
        size = separator.size();
        if (ARCHON_LIKELY(base::try_int_add(size, str.size()))) {
            if (ARCHON_LIKELY(size <= max_size - ellipsis_size)) {
                helper.write(separator); // Throws
                helper.write(str); // Throws
                max_size -= size;
                ++i;
                if (ARCHON_LIKELY(i != end))
                    goto next;
                return;
            }
            j = i;
            goto look_ahead;
        }
        goto ellipsis;

      look_ahead:
        if (ARCHON_LIKELY(size > max_size))
            goto ellipsis;
        max_size -= size;
        ++i;
        if (ARCHON_LIKELY(i != end)) {
            str = format();
            size = separator.size();
            if (ARCHON_LIKELY(base::try_int_add(size, str.size())))
                goto look_ahead;
            goto ellipsis;
        }
        i = j;
        if (ARCHON_LIKELY(i != data.data()))
            goto unlimited_2;
        goto unlimited_3;

      ellipsis:
        {
            C dot = char_mapper.widen('.'); // Throws
            C ellipsis[ellipsis_size];
            std::fill(ellipsis, ellipsis + ellipsis_size, dot);
            helper.write({ ellipsis, ellipsis_size }); // Throws
            return;
        }

      unlimited_1:
        helper.write(str); // Throws
        ++i;
        if (ARCHON_LIKELY(i != end)) {
          unlimited_2:
            helper.write(separator); // Throws
          unlimited_3:
            str = format(); // Throws
            goto unlimited_1;
        }
    }
}


template<class I> struct AsHexDump1 {
    base::Span<const I> data;
    HexDumpConfig config;
};


template<class C, class T, class I> struct AsHexDump2 {
    base::Span<const I> data;
    std::basic_string_view<C, T> separator;
    HexDumpConfig config;
};


template<class C, class T, class I>
inline std::basic_ostream<C, T>& operator<<(std::basic_ostream<C, T>& out,
                                            const AsHexDump1<I>& pod)
{
    std::array<C, 64> seed_memory;
    return base::ostream_sentry(out, [&](base::BasicStreamOutputHelper<C, T>& helper) {
        base::BasicCharMapper<C, T> char_mapper(out); // Throws
        C space = char_mapper.widen(' '); // Throws
        std::basic_string_view<C, T> separator(&space, 1);
        detail::do_hex_dump(helper, char_mapper, pod.data, separator, pod.config); // Throws
    }, base::Span(seed_memory)); // Throws
}


template<class C, class T, class I>
inline std::basic_ostream<C, T>& operator<<(std::basic_ostream<C, T>& out,
                                            const AsHexDump2<C, T, I>& pod)
{
    std::array<C, 64> seed_memory;
    return base::ostream_sentry(out, [&](base::BasicStreamOutputHelper<C, T>& helper) {
        base::BasicCharMapper<C, T> char_mapper(out); // Throws
        detail::do_hex_dump(helper, char_mapper, pod.data, pod.separator, pod.config); // Throws
    }, base::Span(seed_memory)); // Throws
}


} // namespace detail




template<class C, class T, class I>
inline void hex_dump(std::basic_ostream<C, T>& out, base::Span<const I> data, HexDumpConfig config)
{
    base::BasicStreamOutputHelper helper(out);
    helper.init(); // Throws
    base::BasicCharMapper<C, T> char_mapper(out); // Throws
    C space = char_mapper.widen(' '); // Throws
    std::basic_string_view<C, T> separator(&space, 1);
    detail::do_hex_dump(helper, char_mapper, data, separator, config); // Throws
}


template<class C, class T, class I>
inline void hex_dump(std::basic_ostream<C, T>& out, base::Span<const I> data,
                     std::basic_string_view<C, T> separator, HexDumpConfig config)
{
    base::BasicStreamOutputHelper helper(out);
    helper.init(); // Throws
    base::BasicCharMapper<C, T> char_mapper(out); // Throws
    detail::do_hex_dump(helper, char_mapper, data, separator, config); // Throws
}


template<class I> inline auto as_hex_dump(base::Span<const I> data, HexDumpConfig config)
{
    return detail::AsHexDump1<I> { data, config };
}


template<class C, class T, class I>
inline auto as_hex_dump(base::Span<const I> data, std::basic_string_view<C, T> separator,
                        HexDumpConfig config)
{
    return detail::AsHexDump2<C, T, I> { data, separator, config };
}


} // namespace archon::base

#endif // ARCHON_X_BASE_X_HEX_DUMP_HPP
