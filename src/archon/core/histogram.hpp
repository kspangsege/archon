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

#ifndef ARCHON_X_CORE_X_HISTOGRAM_HPP
#define ARCHON_X_CORE_X_HISTOGRAM_HPP

/// \file


#include <cstddef>
#include <cmath>
#include <limits>
#include <algorithm>
#include <memory>
#include <string_view>
#include <string>
#include <locale>
#include <ios>
#include <ostream>
#include <iomanip>

#include <archon/core/features.h>
#include <archon/core/integer.hpp>
#include <archon/core/float.hpp>
#include <archon/core/seed_memory_output_stream.hpp>
#include <archon/core/file.hpp>
#include <archon/core/text_file_stream.hpp>


namespace archon::core {


/// \brief Histogram aggregation and rendition as text.
///
/// An instance of this class holds an aggregated record of a sequence of values, and allows
/// for a histogram to be rendered from that aggregated record.
///
/// Values are added individually using \ref add().
///
/// A histogram representing all the previously added values is rendered using \ref print().
///
template<class T> class Histogram {
public:
    using value_type = T;

    /// \brief Construct histogram object.
    ///
    /// This function constructs a new histogram object with the specified number of bins
    /// (\p num_bins). These bins correspond to a division of the specified range (\p from
    /// -> \p to) into \p num_bins subranges.
    ///
    Histogram(value_type from, value_type to, std::size_t num_bins);

    /// \brief Add valæue to histogram.
    ///
    /// This function add the specified value the the aggregated record.
    ///
    void add(value_type);

    /// \brief Print histogram to STDOUT.
    ///
    /// This function prints a rendition of the histogram onto STDOUT using the current
    /// global locale.
    ///
    void print(int width = 0, bool include_under_over = false) const;

    /// \brief Print histogram to file.
    ///
    /// This function prints a rendition of the histogram to the specified file using the
    /// specified locale.
    ///
    /// \param width The desired width of the histogram in number of character positions
    /// (assuming a fixed-width font). If set to zero, and the specified file refers to a
    /// text terminal, and the width of that terminal can be determined, that width will be
    /// used as the desired width of the histogram.
    ///
    void print(core::File&, const std::locale&, int width = 0, bool include_under_over = false) const;

    /// \brief Print histogram to output stream.
    ///
    /// This function prints a rendition of the histogram onto the specified output stream
    /// using horizontal bars (one text line per bar).
    ///
    /// \param The desired width of the histogram in number of character positions (assuming
    /// a fixed-width font).
    ///
    /// \param include_under_over If true, include a leading bin for values that are less
    /// than \p from as passed to the constructor (\ref Histogram()), as well as a trailing
    /// bin for values that are greater than \p to as passed to the constructor.
    ///
    template<class C, class U> void print(std::basic_ostream<C, U>&, int width, bool include_under_over = false) const;

private:
    using count_type = long long;

    value_type m_base;
    value_type m_scale;
    std::size_t m_num_bins;
    std::unique_ptr<count_type[]> m_bins;
    count_type m_under = 0, m_over = 0;
};








// Implementation


template<class T>
Histogram<T>::Histogram(value_type from, value_type to, std::size_t num_bins)
    : m_base(from)
    , m_scale(num_bins / (to - from))
    , m_num_bins(num_bins)
{
    m_bins = std::make_unique<count_type[]>(num_bins); // Throws
}


template<class T>
void Histogram<T>::add(value_type val)
{
    value_type val_2 = m_scale * (val - m_base);
    if (ARCHON_LIKELY(val_2 >= 0)) {
        value_type val_3 = std::trunc(val_2);
        value_type max = core::max_float_for_int<value_type, std::size_t>(); // Throws
        std::size_t val_4 = std::size_t(val_2);
        if (ARCHON_LIKELY(val_3 <= max && val_4 < m_num_bins)) {
            m_bins[val_4] += 1;
        }
        else {
            m_over += 1;
        }
    }
    else {
        m_under += 1;
    }
}


template<class T>
inline void Histogram<T>::print(int width, bool include_under_over) const
{
    std::locale loc;
    print(core::File::get_cout(), loc, width, include_under_over); // Throws
}


template<class T>
void Histogram<T>::print(core::File& file, const std::locale& loc, int width, bool include_under_over) const
{
    core::TextFileStream stream(&file); // Throws
    stream.exceptions(std::ios_base::badbit | std::ios_base::failbit); // Throws
    stream.imbue(loc); // Throws
    int width_2 = width;
    if (width_2 == 0) {
        core::File::TerminalInfo info = {};
        bool is_term = file.get_terminal_info(info); // Throws
        if (is_term && info.size.has_value()) {
            width_2 = info.size.value().width;
        }
        else {
            width_2 = 80;
        }
    }
    print(stream, width_2, include_under_over); // Throws
    stream.flush(); // Throws
}


template<class T>
template<class C, class U> void Histogram<T>::print(std::basic_ostream<C, U>& out, int width,
                                                    bool include_under_over) const
{
    using char_type   = C;
    using traits_type = U;

    value_type bin_width = 1 / m_scale;
    using limits_type = std::numeric_limits<value_type>;
    value_type min = limits_type::lowest();
    value_type max = limits_type::max();
    if (limits_type::has_infinity) {
        min = -limits_type::infinity();
        max = limits_type::infinity();
    }
    bool include_under = include_under_over && min < m_base;
    bool include_over = include_under_over && max > m_base + m_num_bins * bin_width;

    count_type max_count = std::max(m_under, m_over);
    std::size_t num_bins = m_num_bins;
    for (std::size_t i = 0; i < num_bins; ++i) {
        count_type n = m_bins[i];
        if (ARCHON_LIKELY(n <= max_count))
            continue;
        max_count = n;
    }

    struct Text {
        std::size_t begin;
        std::size_t end;
    };

    struct BinTexts {
        Text from;
        Text to;
        Text count;
    };

    core::BasicSeedMemoryOutputStream<char_type, traits_type> formatter_1; // Throws
    std::locale loc = out.getloc(); // Throws
    formatter_1.exceptions(std::ios_base::badbit | std::ios_base::failbit); // Throws
    formatter_1.imbue(loc); // Throws
    std::unique_ptr<BinTexts[]> bin_texts_entries = std::make_unique<BinTexts[]>(num_bins); // Throws
    int from_column_width = 0;
    int to_column_width = 0;
    int count_column_width = 0;
    BinTexts under_bin_texts, over_bin_texts;
    {
        bool showpos = false;
        auto format_value = [&](auto val) {
            std::size_t begin = formatter_1.view().size();
            if (!showpos) {
                formatter_1 << val; // Throws
            }
            else {
                formatter_1 << std::showpos << val << std::noshowpos; // Throws
            }
            std::size_t end = formatter_1.view().size();
            return Text { begin, end };
        };
        auto format_count = [&](auto n) {
            std::size_t begin = formatter_1.view().size();
            formatter_1 << n; // Throws
            std::size_t end = formatter_1.view().size();
            return Text { begin, end };
        };
        auto make_texts = [&](Text from, Text to, count_type n) {
            int size = 0;
            core::int_cast(from.end - from.begin, size); // Throws
            if (size > from_column_width)
                from_column_width = size;
            core::int_cast(to.end - to.begin, size); // Throws
            if (size > to_column_width)
                to_column_width = size;
            Text count = format_count(n); // Throws
            core::int_cast(count.end - count.begin, size); // Throws
            if (size > count_column_width)
                count_column_width = size;
            return BinTexts { from, to, count };
        };
        value_type from = m_base;
        if (include_under)
            from = min;
        if (from < 0)
            showpos = true;
        Text from_text = format_value(from); // Throws
        if (include_under) {
            value_type to = m_base;
            Text to_text = format_value(to); // Throws
            under_bin_texts = make_texts(from_text, to_text, m_under); // Throws
            from_text = to_text;
        }
        for (std::size_t i = 0; i < num_bins; ++i) {
            value_type to = m_base + (i + 1) * bin_width;
            Text to_text = format_value(to); // Throws
            bin_texts_entries[i] = make_texts(from_text, to_text, m_bins[i]); // Throws
            from_text = to_text;
        }
        if (include_over) {
            value_type to = max;
            Text to_text = format_value(to); // Throws
            over_bin_texts = make_texts(from_text, to_text, m_over); // Throws
        }
    }

    using string_view_type = std::basic_string_view<char_type, traits_type>;
    const char_type* text_base = formatter_1.view().data();
    auto get_string = [&](Text text) {
        const char_type* data = text_base + text.begin;
        std::size_t size = std::size_t(text.end - text.begin);
        return string_view_type(data, size); // Throws
    };

    std::basic_string<char_type, traits_type> bar_buffer;
    char_type hash = out.widen('#'); // Throws
    auto get_bar = [&](int size) {
        std::size_t size_2 = 0;
        core::int_cast(size, size_2); // Throws
        if (ARCHON_UNLIKELY(size_2 > bar_buffer.size())) {
            std::size_t n = std::size_t(size_2 - bar_buffer.size());
            bar_buffer.append(n, hash); // Throws
        }
        return string_view_type(bar_buffer.data(), size_2); // Throws
    };

    int max_bar_size = width;
    auto deduct = [&](int n) noexcept {
        if (ARCHON_LIKELY(max_bar_size >= n)) {
            max_bar_size -= n;
        }
        else {
            max_bar_size = 0;
        }
    };
    deduct(from_column_width);
    deduct(4);
    deduct(to_column_width);
    deduct(3);
    deduct(count_column_width);
    deduct(3);
    double scale = double(max_bar_size) / max_count;

    core::BasicSeedMemoryOutputStream<char_type, traits_type> formatter_2; // Throws
    formatter_2.exceptions(std::ios_base::badbit | std::ios_base::failbit); // Throws
    formatter_2.imbue(loc); // Throws
    formatter_2 << std::left;
    auto format_bin = [&](const BinTexts& texts, count_type n) {
        formatter_2 << std::setw(from_column_width) << get_string(texts.from); // Throws
        formatter_2 << " -> "; // Throws
        formatter_2 << std::setw(to_column_width) << get_string(texts.to); // Throws
        formatter_2 << " : "; // Throws
        formatter_2 << std::right;
        formatter_2 << std::setw(count_column_width) << get_string(texts.count); // Throws
        formatter_2 << std::left;
        formatter_2 << " |"; // Throws
        formatter_2 << std::setw(max_bar_size) << get_bar(int(std::round(n * scale))); // Throws
        formatter_2 << "|\n"; // Throws
        out << formatter_2.view(); // Throws
        formatter_2.full_clear();
    };

    if (include_under)
        format_bin(under_bin_texts, m_under); // Throws
    for (std::size_t i = 0; i < num_bins; ++i)
        format_bin(bin_texts_entries[i], m_bins[i]); // Throws
    if (include_over)
        format_bin(over_bin_texts, m_over); // Throws
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_HISTOGRAM_HPP
