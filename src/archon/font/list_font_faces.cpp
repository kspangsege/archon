// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2026 Kristian Spangsege <kristian.spangsege@gmail.com>
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


#include <memory>
#include <string>
#include <vector>
#include <locale>

#include <archon/core/integer.hpp>
#include <archon/core/as_int.hpp>
#include <archon/core/file.hpp>
#include <archon/core/text_formatter.hpp>
#include <archon/core/with_text_formatter.hpp>
#include <archon/font/face.hpp>
#include <archon/font/loader.hpp>
#include <archon/font/list_font_faces.hpp>


using namespace archon;


void font::list_font_faces(font::Loader& loader, core::File& file, const std::locale& locale)
{
    struct entry {
        bool bold, italic, monospace, scalable;
        std::string family;
    };
    std::vector<entry> entries;
    int n = loader.get_num_faces(); // Throws
    for (int i = 0; i < n; ++i) {
        std::unique_ptr<font::Face> face = loader.load_face(i); // Throws
        entry e = {
            face->is_bold(),
            face->is_italic(),
            face->is_monospace(),
            face->is_scalable(),
            std::string(face->get_family_name()), // Throws
        };
        entries.push_back(e); // Throws
    }

    core::with_text_formatter(file, locale, [&](core::TextFormatter& formatter) {
        formatter.begin_hold(); // Throws

        // 1st column: Ordinal
        formatter.begin_compile(); // Throws
        for (int i = 0; i < n; ++i)
            formatter.writeln(core::as_int(1 + i)); // Throws
        formatter.close_section(); // Throws
        core::TextFormatter::MeasureResult result_1 = formatter.measure(0, formatter.get_cursor_state()); // Throws
        int offset_1 = result_1.min_width_no_break;
        core::saturating_add(offset_1, 2);
        formatter.format_section(0); // Throws
        formatter.end_compile();

        // 2nd column: Flags
        formatter.begin_compile();
        for (const entry& e : entries) {
            formatter.write(e.bold      ? "B" : "-"); // Throws
            formatter.write(e.italic    ? "I" : "-"); // Throws
            formatter.write(e.monospace ? "M" : "-"); // Throws
            formatter.write(e.scalable  ? "S" : "-"); // Throws
            formatter.write("\n"); // Throws
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

        // 3rd column: Family name
        formatter.jump_back(); // Throws
        formatter.set_offset(offset_2); // Throws
        for (const entry& e : entries)
            formatter.writeln(e.family); // Throws
        formatter.end_hold(); // Throws
    }); // Throws
}
