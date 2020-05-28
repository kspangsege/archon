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


/// \file

#ifndef ARCHON__BASE__HEX_DUMP_HPP
#define ARCHON__BASE__HEX_DUMP_HPP

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


template<class C, class T, class U> void hex_dump(std::basic_ostream<C, T>&, base::Span<const U> data, HexDumpConfig = {});
template<class C, class T, class U> void hex_dump(std::basic_ostream<C, T>&, base::Span<const U> data, std::basic_string_view<C, T> separator, HexDumpConfig = {});


template<class U> auto as_hex_dump(base::Span<const U> data, HexDumpConfig = {});
template<class C, class T, class U> auto as_hex_dump(base::Span<const U> data, std::basic_string_view<C, T> separator, HexDumpConfig = {});








// Implementation


namespace detail {


template<class C, class T, class U>
void do_hex_dump(std::basic_ostream<C, T>& out, const BasicCharMapper<C, T>& char_mapper,
                 base::Span<const U> data, std::basic_string_view<C, T> separator,
                 HexDumpConfig config)
{
    const std::size_t ellipsis_size = 3;

    constexpr int int_width = base::get_int_width<U>();
    using uint_type = base::FastestUnsignedWithBits<int_width>;
    static_assert(!std::is_same_v<uint_type, void>); // A hope more than a certainty
    uint_type mask = base::int_mask<uint_type>(int_width);

    int min_digits = config.min_digits;
    if (min_digits <= 0)
        min_digits = base::int_digits(mask, 16);

    const U* i   =     data.data();
    const U* end = i + data.size();

    typename std::basic_ostream<C, T>::sentry sentry(out); // Throws
    if (ARCHON_LIKELY(i != end && sentry)) {
        base::BasicIntegerFormatter integer_formatter(char_mapper);
        auto format = [&] {
            auto value = uint_type(*i) & mask;
            return integer_formatter.format_hex(value, min_digits); // Throws
        };
        using string_view_type = std::basic_string_view<C, T>;
        string_view_type str = format(); // Throws
        std::size_t max_size, size;
        const U* j;
        if (ARCHON_LIKELY(config.max_size != std::size_t(-1))) {
            max_size = std::max(config.max_size, ellipsis_size);
            size = str.size();
            if (ARCHON_LIKELY(size <= max_size - ellipsis_size)) {
                out.write(str.data(), str.size()); // Throws
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
                out.write(separator.data(), separator.size()); // Throws
                out.write(str.data(), str.size()); // Throws
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
            out.write(ellipsis, ellipsis_size); // Throws
            return;
        }

      unlimited_1:
        out.write(str.data(), str.size()); // Throws
        ++i;
        if (ARCHON_LIKELY(i != end)) {
          unlimited_2:
            out.write(separator.data(), separator.size()); // Throws
          unlimited_3:
            str = format(); // Throws
            goto unlimited_1;
        }
    }
}


template<class U> struct AsHexDump1 {
    base::Span<const U> data;
    HexDumpConfig config;
};


template<class C, class T, class U> struct AsHexDump2 {
    base::Span<const U> data;
    std::basic_string_view<C, T> separator;
    HexDumpConfig config;
};


template<class C, class T, class U>
inline std::basic_ostream<C, T>& operator<<(std::basic_ostream<C, T>& out,
                                            const AsHexDump1<U>& pod)
{
    base::hex_dump(out, pod.data, pod.config); // Throws
    return out;
}


template<class C, class T, class U>
inline std::basic_ostream<C, T>& operator<<(std::basic_ostream<C, T>& out,
                                            const AsHexDump2<C, T, U>& pod)
{
    base::hex_dump(out, pod.data, pod.separator, pod.config); // Throws
    return out;
}


} // namespace detail




template<class C, class T, class U>
inline void hex_dump(std::basic_ostream<C, T>& out, base::Span<const U> data, HexDumpConfig config)
{
    base::BasicCharMapper<C, T> char_mapper(out); // Throws
    C space = char_mapper.widen(' '); // Throws
    std::basic_string_view<C, T> separator(&space, 1);
    detail::do_hex_dump(out, char_mapper, data, separator, config); // Throws
}


template<class C, class T, class U>
inline void hex_dump(std::basic_ostream<C, T>& out, base::Span<const U> data,
                     std::basic_string_view<C, T> separator, HexDumpConfig config)
{
    base::BasicCharMapper<C, T> char_mapper(out); // Throws
    detail::do_hex_dump(out, char_mapper, data, separator, config); // Throws
}


template<class U> inline auto as_hex_dump(base::Span<const U> data, HexDumpConfig config)
{
    return detail::AsHexDump1<U> { data, config };
}


template<class C, class T, class U>
inline auto as_hex_dump(base::Span<const U> data, std::basic_string_view<C, T> separator,
                        HexDumpConfig config)
{
    return detail::AsHexDump2<C, T, U> { data, separator, config };
}


} // namespace archon::base

#endif // ARCHON__BASE__HEX_DUMP_HPP
