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

#ifndef ARCHON_X_CORE_X_HEX_DUMP_HPP
#define ARCHON_X_CORE_X_HEX_DUMP_HPP

/// \file


#include <cstddef>
#include <type_traits>
#include <algorithm>
#include <string_view>
#include <ostream>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/type.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/char_mapper.hpp>
#include <archon/core/stream_output.hpp>
#include <archon/core/integer_formatter.hpp>


namespace archon::core {


/// \brief Hex dump parameters.
///
/// These are the available parameters for controlling the output of hex dump operations.
///
struct HexDumpConfig {
    /// \brief The minimum number of digits to generate for each data element.
    ///
    /// If the value is less than, or equal to zero, the number of digits will be determined
    /// by the type of dumped data.
    ///
    int min_digits = 0;

    /// If specified (not equal to `std::size_t(-1)`), the total size of the hex dump will
    /// be limited to the specified size. This is done by replacing a trailing section of
    /// the specified data sequence by an ellipsis (`...`).
    ///
    std::size_t max_size = std::size_t(-1);
};



template<class C, class T, class D> void hex_dump(std::basic_ostream<C, T>&, D&& data, HexDumpConfig = {});
template<class C, class T, class D> void hex_dump(std::basic_ostream<C, T>&, D&& data,
                                                  std::basic_string_view<C, T> separator, HexDumpConfig = {});



/// \{
///
/// \brief Dump data in hexadecimal form.
///
/// Construct an object that, if written to an output stream, formats the specified sequence
/// of integers according to the specified parameters. Each integer will be expressed in
/// hexadecimal form.
///
/// The field width of the target stream will be respected, and the effect will be as if all
/// of the generated output was written to the stream as a single string object.
///
/// The specified data (\p data) must be something from which a span (\ref core::Span) can
/// be constructed. Specifically, `core::Span(data)` must be a valid expression.
///
/// The specified data must consiste of integer elements. The integer type must conform to
/// \ref Concept_Archon_Core_Integer.
///
template<class D> auto as_hex_dump(D&& data, HexDumpConfig = {});
template<class C, class T, class D> auto as_hex_dump(D&& data, std::basic_string_view<C, T> separator,
                                                     HexDumpConfig = {});
/// \}








// Implementation


namespace impl {


template<class C, class T, class I>
void do_hex_dump(core::BasicStreamOutputHelper<C, T>& helper, const BasicCharMapper<C, T>& char_mapper,
                 core::Span<const I> data, std::basic_string_view<C, T> separator, core::HexDumpConfig config)
{
    const std::size_t ellipsis_size = 3;

    constexpr int int_width = core::int_width<I>();
    using uint_type = decltype(core::to_unsigned(core::promote_strongly(std::declval<I>())));
    uint_type mask = core::int_mask<uint_type>(int_width);

    int min_digits = config.min_digits;
    if (min_digits <= 0)
        min_digits = core::int_num_digits(mask, 16);

    const I* i   =     data.data();
    const I* end = i + data.size();

    if (ARCHON_LIKELY(i != end)) {
        core::BasicIntegerFormatter integer_formatter(char_mapper);
        auto format = [&] {
            auto value = core::int_cast_a<uint_type>(*i) & mask;
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
        if (ARCHON_LIKELY(core::try_int_add(size, str.size()))) {
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
            if (ARCHON_LIKELY(core::try_int_add(size, str.size())))
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
    core::Span<const I> data;
    core::HexDumpConfig config;
};


template<class C, class T, class I> struct AsHexDump2 {
    core::Span<const I> data;
    std::basic_string_view<C, T> separator;
    core::HexDumpConfig config;
};


template<class C, class T, class I>
inline auto operator<<(std::basic_ostream<C, T>& out, const impl::AsHexDump1<I>& pod) -> std::basic_ostream<C, T>&
{
    std::array<C, 64> seed_memory;
    return core::ostream_sentry(out, [&](core::BasicStreamOutputHelper<C, T>& helper) {
        core::BasicCharMapper<C, T> char_mapper(out); // Throws
        C space = char_mapper.widen(' '); // Throws
        std::basic_string_view<C, T> separator(&space, 1);
        impl::do_hex_dump(helper, char_mapper, pod.data, separator, pod.config); // Throws
    }, core::Span(seed_memory)); // Throws
}


template<class C, class T, class I>
inline auto operator<<(std::basic_ostream<C, T>& out,
                       const impl::AsHexDump2<C, T, I>& pod) -> std::basic_ostream<C, T>&
{
    std::array<C, 64> seed_memory;
    return core::ostream_sentry(out, [&](core::BasicStreamOutputHelper<C, T>& helper) {
        core::BasicCharMapper<C, T> char_mapper(out); // Throws
        impl::do_hex_dump(helper, char_mapper, pod.data, pod.separator, pod.config); // Throws
    }, core::Span(seed_memory)); // Throws
}


} // namespace impl




template<class C, class T, class D>
inline void hex_dump(std::basic_ostream<C, T>& out, D&& data, core::HexDumpConfig config)
{
    core::Span data_2(std::forward<D>(data));
    core::BasicStreamOutputHelper helper(out);
    helper.init(); // Throws
    core::BasicCharMapper<C, T> char_mapper(out); // Throws
    C space = char_mapper.widen(' '); // Throws
    std::basic_string_view<C, T> separator(&space, 1);
    impl::do_hex_dump(helper, char_mapper, data_2, separator, config); // Throws
}


template<class C, class T, class D>
inline void hex_dump(std::basic_ostream<C, T>& out, D&& data, std::basic_string_view<C, T> separator,
                     core::HexDumpConfig config)
{
    core::Span data_2(std::forward<D>(data));
    core::BasicStreamOutputHelper helper(out);
    helper.init(); // Throws
    core::BasicCharMapper<C, T> char_mapper(out); // Throws
    impl::do_hex_dump(helper, char_mapper, data_2, separator, config); // Throws
}


template<class D> inline auto as_hex_dump(D&& data, core::HexDumpConfig config)
{
    core::Span data_2(std::forward<D>(data));
    using span_type = decltype(data_2);
    using elem_type = typename span_type::element_type;
    return impl::AsHexDump1<elem_type> { data_2, config };
}


template<class C, class T, class D>
inline auto as_hex_dump(D&& data, std::basic_string_view<C, T> separator, core::HexDumpConfig config)
{
    core::Span data_2(std::forward<D>(data));
    using span_type = decltype(data_2);
    using elem_type = typename span_type::element_type;
    return impl::AsHexDump2<C, T, elem_type> { data_2, separator, config };
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_HEX_DUMP_HPP
