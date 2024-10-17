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


#include <locale>

#include <archon/core/integer.hpp>
#include <archon/core/file.hpp>
#include <archon/core/text_formatter.hpp>
#include <archon/core/with_text_formatter.hpp>
#include <archon/image/file_format.hpp>
#include <archon/image/file_format_registry.hpp>
#include <archon/image/list_file_formats.hpp>


using namespace archon;


void image::list_file_formats(core::File& file, const std::locale& locale, const image::FileFormatRegistry& registry)
{
    core::with_text_formatter(file, locale, [&](core::TextFormatter& formatter) {
        formatter.begin_hold(); // Throws
        formatter.begin_compile(); // Throws
        int n = registry.get_num_file_formats();
        for (int i = 0; i < n; ++i) {
            const image::FileFormat& format = registry.get_file_format(i); // Throws
            using Weight = core::TextFormatter::Weight;
            formatter.set_weight(Weight::bold); // Throws
            formatter.writeln(format.get_ident()); // Throws
            formatter.set_weight(Weight::normal); // Throws
        }
        formatter.close_section(); // Throws
        core::TextFormatter::MeasureResult result_1 = formatter.measure(0, formatter.get_cursor_state()); // Throws
        int offset_1 = result_1.min_width_no_break;
        core::saturating_add(offset_1, 2);
        formatter.format_section(0); // Throws
        formatter.end_compile();
        formatter.begin_compile();
        for (int i = 0; i < n; ++i) {
            const image::FileFormat& format = registry.get_file_format(i); // Throws
            using Color = core::TextFormatter::Color;
            if (format.is_available()) {
                formatter.set_color(Color::green); // Throws
                formatter.writeln("available"); // Throws
                formatter.unset_color(); // Throws
            }
            else {
                formatter.set_color(Color::red); // Throws
                formatter.writeln("unavailable"); // Throws
                formatter.unset_color(); // Throws
            }
        }
        formatter.close_section(); // Throws
        core::TextFormatter::MeasureResult result_2 = formatter.measure(0, formatter.get_cursor_state()); // Throws
        int offset_2 = offset_1;
        core::saturating_add(offset_2, result_2.min_width_no_break);
        core::saturating_add(offset_2, 2);
        formatter.jump_back(); // Throws
        formatter.set_offset(offset_1); // Throws
        formatter.format_section(0); // Throws
        formatter.end_compile();
        formatter.jump_back(); // Throws
        formatter.set_offset(offset_2); // Throws
        for (int i = 0; i < n; ++i) {
            const image::FileFormat& format = registry.get_file_format(i); // Throws
            formatter.writeln(format.get_descr()); // Throws
        }
        formatter.end_hold(); // Throws
    }); // Throws
}
