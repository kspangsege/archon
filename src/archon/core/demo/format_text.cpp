// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2022 Kristian Spangsege <kristian.spangsege@gmail.com>
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


#include <cstddef>
#include <algorithm>
#include <memory>
#include <vector>
#include <locale>
#include <ios>
#include <ostream>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/locale.hpp>
#include <archon/core/format.hpp>
#include <archon/core/as_list.hpp>
#include <archon/core/file.hpp>
#include <archon/core/text_file_stream.hpp>
#include <archon/core/terminal.hpp>
#include <archon/core/text_formatter.hpp>
#include <archon/log.hpp>
#include <archon/cli.hpp>


using namespace archon;


int main(int argc, char* argv[])
{
    std::locale locale = core::get_default_locale(); // Throws

    core::terminal::When color = core::terminal::When::auto_;

    cli::Spec spec;
    opt(cli::help_tag, spec); // Throws

    opt("-c, --color", "<when>", cli::no_attributes, spec,
        "Control when output is colorized. @A can be \"auto\", \"never\", or \"always\". It is @Q by default.",
        cli::assign(color)); // Throws

    int exit_status = 0;
    if (ARCHON_UNLIKELY(cli::process(argc, argv, spec, exit_status, locale))) // Throws
        return exit_status;

    using text_formatter_type = core::TextFormatter;
    using Weight = text_formatter_type::Weight;
    using Color  = text_formatter_type::Color;

    core::File& file = core::File::get_stdout();
    core::TextFileStream out(&file); // Throws
    out.exceptions(std::ios_base::badbit | std::ios_base::failbit); // Throws
    out.imbue(locale); // Throws
    text_formatter_type::Config config;
    config.high_quality_word_wrapper = true;
    config.enable_ansi_escape_sequences =
        core::terminal::should_enable_escape_sequences(color, file.is_terminal(), locale); // Throws
    text_formatter_type formatter(out, config);

    formatter.push_format();
    formatter.begin_hold();
    formatter.set_padding(1);
    formatter.set_width(9);
    formatter.set_fill_color(Color::red);
    formatter.writeln("alpha");
    formatter.writeln("beta");
    formatter.writeln("gamma");
    formatter.writeln("delta");
    formatter.writeln("epsilon");
    formatter.jump_back();
    formatter.set_offset(10);
    formatter.set_width(7);
    formatter.set_fill_color(Color::green);
    formatter.writeln("zeta");
    formatter.writeln("eta");
    formatter.writeln("theta");
    formatter.writeln("iota");
    formatter.writeln("kappa");
    formatter.jump_back();
    formatter.set_offset(18);
    formatter.set_width(9);
    formatter.set_fill_color(Color::blue);
    formatter.writeln("lambda");
    formatter.writeln("mu");
    formatter.writeln("nu");
    formatter.end_hold();
    formatter.writeln("xi");
    formatter.push_style();
    formatter.set_weight(Weight::bold);
    formatter.writeln("omicron");
    formatter.pop_style();
    formatter.writeln("pi");
    formatter.writeln("rho");
    formatter.writeln("sigma");
    formatter.writeln("tau");
    formatter.set_offset(0);
    formatter.pop_format();

    formatter.set_offset(4);
    formatter.set_width(30);
    formatter.set_padding(2);
    formatter.set_fill_color(Color::black);

    formatter.skip_line();
    formatter.begin_compile();
    formatter.write("Otorhinolaryngological ");
    formatter.close_section();
    formatter.format_section(0);
    formatter.format_section(0);
    formatter.write("immunoelectrophoretically psychophysicotherapeutics thyroparathyro");
    formatter.push_style();
    formatter.set_color(Color::red);
    formatter.write("idectomized pneumoencephalographically");
    formatter.pop_style();
    formatter.write(" radioimmunoelectrophoresis psychoneuroendocrinological ");
    formatter.push_style();
    formatter.set_color(Color::green);
    formatter.write("hepaticocholangiogastrostomy");
    formatter.pop_style();
    formatter.write(" spectrophotofluorometrically pseudopseudohypoparathyroidism ");
    formatter.close_section();
    formatter.format_section(0);
    formatter.format_section(1);
    formatter.format_section(1);
    formatter.format_section(1);
    formatter.write("   x x ");
    formatter.close_section();
    formatter.flush();
    {
        auto info = formatter.get_section_info(2);
        log::info("num_words                 = %s", info.num_words);
        log::info("num_lines                 = %s", info.num_lines);
        log::info("last_line_is_unterminated = %s", info.last_line_is_unterminated);
        auto result = formatter.simulate(2, formatter.get_format().get_width());
        log::info("width  = %s", result.width);
        log::info("height = %s", result.height);
    }
    formatter.end_compile();
    formatter.write("\n");

    formatter.skip_line();
    formatter.write("Hest ged lama gnu kat gris ko panda struds hund.\nKofoed Viggo Banach Hil");
    formatter.writeln("bert Minkowski Hausdorf.\nBlue yellow black brown violet red white green.");
    std::ostream& input_out = formatter.input_out();
    int value = 26727;
    input_out << "Hula hoop " << value << " cyr wheel lyra aerial hula hoop cyr wheel lyra aerial.\n";
    formatter.write("Alpha beta gamma delta epsilon zeta eta theta iota kappa lambda mu nu xi ");
    formatter.write("omicron pi rho sigma tau upsilon phi chi psi omega.\n");
    std::vector<int> vec = {
        58659, 10934, 34860, 51944, 40696, 56572, 17122,
        38167, 27147, 15981, 48028, 16923, 43738, 58659
    };
    input_out << core::as_list(vec) << " " << core::formatted("<%s, %s>", 43738, 15981) << ".\n";
    formatter.writeln("Xxxxxxxxxxx xxxxxxxx xxxxxxxxx xxxxxxxxxx.");
    formatter.flush();
    if (!input_out)
        log::error("FAIL");
    formatter.push_style();
    formatter.set_background_color(Color::blue);
    formatter.write("Hej ");
    formatter.push_style();
    formatter.set_underline(true);
    formatter.write("m");
    formatter.push_style();
    formatter.set_blink(true);
    formatter.write("e");
    formatter.pop_style();
    formatter.write("d");
    formatter.pop_style();
    formatter.write(" dig\n");
    formatter.pop_style();
    formatter.write("Time, ");
    formatter.push_style();
    formatter.set_background_color(Color::red);
    formatter.write("Dr. Freeman? Is it really that time");
    formatter.pop_style();
    formatter.write(" again?\n");
    formatter.flush();
    formatter.write("a");
    formatter.close_section();
    formatter.write(" b");
    formatter.close_section();
    formatter.write(" c\n");
    formatter.close_section();
    formatter.write("abcdefghij");
    formatter.close_section();
    formatter.write("a b c d e f g h i j k l m n o p q r s t u v w x y z\n");

    formatter.skip_line();
    formatter.push_format();
    formatter.reset_format();
    formatter.begin_hold();
    formatter.write("\n");
    formatter.writeln("background");
    formatter.write("\n");
    formatter.jump_back();
    formatter.set_offset(4);
    formatter.set_width(8);
    formatter.set_padding_left(4);
    formatter.set_fill_color(Color::green);
    formatter.writeln("x");
    formatter.writeln("y");
    formatter.writeln("z");
    formatter.end_hold();
    formatter.pop_format();

    formatter.skip_line();
    formatter.push_format();
    formatter.set_width(70);
    formatter.set_indent(0, 4, 0);
    formatter.set_justify(true);
    formatter.begin_compile();
    formatter.write("Until 1912, Hilbert was almost exclusively a \"pure\" mathematician. "
                    "When planning a visit from Bonn, where he was immersed in studying "
                    "physics, his fellow mathematician and friend Hermann Minkowski joked he "
                    "had to spend 10 days in quarantine before being able to visit Hilbert. "
                    "In fact, Minkowski seems responsible for most of Hilbert's physics "
                    "investigations prior to 1912, including their joint seminar in the "
                    "subject in 1905.\nIn 1912, three years after his friend's death, Hilbert "
                    "turned his focus to the subject almost exclusively. He arranged to have "
                    "a \"physics tutor\" for himself. He started studying kinetic gas theory "
                    "and moved on to elementary radiation theory and the molecular theory of "
                    "matter. Even after the war started in 1914, he continued seminars and "
                    "classes where the works of Albert Einstein and others were followed "
                    "closely.\nBy 1907 Einstein had framed the fundamentals of the theory of "
                    "gravity, but then struggled for nearly 8 years with a confounding "
                    "problem of putting the theory into final form. By early summer 1915, "
                    "Hilbert's interest in physics had focused on general relativity, and he "
                    "invited Einstein to Goettingen to deliver a week of lectures on the "
                    "subject. Einstein received an enthusiastic reception at Goettingen. Over "
                    "the summer Einstein learned that Hilbert was also working on the field "
                    "equations and redoubled his own efforts. During November 1915 Einstein "
                    "published several papers culminating in \"The Field Equations of "
                    "Gravitation\" (see Einstein field equations). Nearly simultaneously "
                    "David Hilbert published \"The Foundations of Physics\", an axiomatic "
                    "derivation of the field equations (see Einstein-Hilbert action). Hilbert "
                    "fully credited Einstein as the originator of the theory, and no public "
                    "priority dispute concerning the field equations ever arose between the "
                    "two men during their lives. See more at priority.\nAdditionally, "
                    "Hilbert's work anticipated and assisted several advances in the "
                    "mathematical formulation of quantum mechanics. His work was a key aspect "
                    "of Hermann Weyl and John von Neumann's work on the mathematical "
                    "equivalence of Werner Heisenberg's matrix mechanics and Erwin "
                    "Schroedinger's wave equation and his namesake Hilbert space plays an "
                    "important part in quantum theory. In 1926 von Neumann showed that if "
                    "quantum states were understood as vectors in Hilbert space, then they "
                    "would correspond with both Schroedinger's wave function theory and "
                    "Heisenberg's matrices.\nThroughout this immersion in physics, Hilbert "
                    "worked on putting rigor into the mathematics of physics. While highly "
                    "dependent on higher mathematics, physicists tended to be \"sloppy\" with "
                    "it. To a \"pure\" mathematician like Hilbert, this was both \"ugly\" and "
                    "difficult to understand. As he began to understand physics and how "
                    "physicists were using mathematics, he developed a coherent mathematical "
                    "theory for what he found, most importantly in the area of integral "
                    "equations. When his colleague Richard Courant wrote the now classic "
                    "Methoden der mathematischen Physik (Methods of Mathematical Physics) "
                    "including some of Hilbert's ideas, he added Hilbert's name as author "
                    "even though Hilbert had not directly contributed to the writing. Hilbert "
                    "said \"Physics is too hard for physicists\", implying that the necessary "
                    "mathematics was generally beyond them; the Courant-Hilbert book made it "
                    "easier for them.\n");
    formatter.close_section();
    formatter.flush();
    {
        auto cursor = formatter.get_cursor_state();
        auto result = formatter.measure(0, cursor);
        log::info("min_width_no_oflow = %s", result.min_width_no_oflow);
        log::info("min_width_no_break = %s", result.min_width_no_break);
        formatter.format_section(0);
    }
    formatter.write("Alpha beta gamma delta pi.\nEpsilon zeta eta theta.\nIota kappa lambda mu.\n");
    formatter.close_section();
    formatter.flush();
    {
        auto cursor = formatter.get_cursor_state();
        auto result_1 = formatter.measure(1, cursor);
        log::info("min_width_no_oflow = %s", result_1.min_width_no_oflow);
        log::info("min_width_no_break = %s", result_1.min_width_no_break);
        formatter.set_width(result_1.min_width_no_oflow);
        auto result_2 = formatter.simulate(1, formatter.get_format().get_width());
        log::info("width  = %s", result_2.width);
        log::info("height = %s", result_2.height);
        formatter.format_section(1);
    }
    formatter.end_compile();
    formatter.pop_format();

    formatter.skip_line();
    formatter.push_format();
    formatter.set_width(48);
    formatter.write("Options:\n");
    formatter.set_adv_continuation(true);
    formatter.set_min_separation(3);
    formatter.set_max_displacement(6);
    formatter.set_justify(false);
    formatter.set_indent(2, 4);
    {
        bool first = true;
        for (const char* form : { "--help", "--halp", "--hylp", "--holp", "--hilp", "--hulp",
                                  "--help", "--halp", "--hylp", "--holp", "--hilp", "--hulp" }) {
            if (!first)
                formatter.write(", ");
            first = false;
            formatter.push_style();
            formatter.set_weight(Weight::bold);
            formatter.write(form);
            formatter.pop_style();
        }
    }
    formatter.close_section();
    formatter.set_indent(24);
    formatter.set_justify(true);
    formatter.write("Lorem ipsum lorem ipsum lorem ipsum lorem ipsum lorem ipsum lorem ipsum "
                    "lorem ipsum lorem ipsum.\n");
    formatter.pop_format();

    formatter.skip_line();
    formatter.push_format();
    formatter.begin_compile();
    {
        class Table {
        public:
            Table(text_formatter_type& formatter)
                : m_formatter(formatter)
                , m_sections_begin(formatter.get_num_sections())
            {
            }
            void close_cell()
            {
                m_formatter.close_section();
            }
            void close_row()
            {
                close_cell();
                std::size_t sections_end = m_formatter.get_num_sections();
                std::size_t height = 0;
                m_rows.push_back({ sections_end, height });
            }
            void format(std::size_t max_width, std::size_t col_spacing = 0)
            {
                std::size_t num_columns = 0;
                {
                    std::size_t sections_begin = m_sections_begin;
                    for (const Row& row : m_rows) {
                        std::size_t sections_end = row.sections_end;
                        ARCHON_ASSERT(sections_end >= sections_begin);
                        std::size_t num_columns_2 = std::size_t(sections_end - sections_begin);
                        if (num_columns_2 > num_columns)
                            num_columns = num_columns_2;
                        sections_begin = sections_end;
                    }
                }
                log::info("num_columns = %s", num_columns);
                if (ARCHON_UNLIKELY(num_columns == 0))
                    return;
                using Cursor         = text_formatter_type::Cursor;
                using MeasureResult  = text_formatter_type::MeasureResult;
                using SimulateResult = text_formatter_type::SimulateResult;
                struct Col {
                    MeasureResult measure_result;
                    std::size_t width_1, width_2;
                };
                std::unique_ptr<Col[]> columns = std::make_unique<Col[]>(num_columns);
                {
                    std::size_t sections_begin = m_sections_begin;
                    for (const Row& row : m_rows) {
                        std::size_t sections_end = row.sections_end;
                        std::size_t num_columns_2 = sections_end - sections_begin;
                        for (std::size_t i = 0; i < num_columns_2; ++i) {
                            Col& col = columns[i];
                            std::size_t section_index = std::size_t(sections_begin + i);
                            Cursor cursor;
                            MeasureResult result = m_formatter.measure(section_index, cursor);
                            if (result.min_width_no_oflow > col.measure_result.min_width_no_oflow)
                                col.measure_result.min_width_no_oflow = result.min_width_no_oflow;
                            if (result.min_width_no_break > col.measure_result.min_width_no_break)
                                col.measure_result.min_width_no_break = result.min_width_no_break;
                        }
                        sections_begin = sections_end;
                    }
                }
                std::size_t max = std::size_t(-1);
                std::size_t aggr_col_spacing = col_spacing;
                if (ARCHON_UNLIKELY(!core::try_int_mul(aggr_col_spacing, num_columns - 1)))
                    aggr_col_spacing = max;
                std::size_t remaining_width = 0;
                if (ARCHON_LIKELY(aggr_col_spacing <= max_width))
                    remaining_width = std::size_t(max_width - aggr_col_spacing);
                std::size_t num_remaining_columns = num_columns;
                std::unique_ptr<std::size_t[]> remaining_columns =
                    std::make_unique<std::size_t[]>(num_remaining_columns);
                for (std::size_t i = 0; i < num_remaining_columns; ++i)
                    remaining_columns[i] = i;
                for (;;) {
                    ARCHON_ASSERT(num_remaining_columns > 0);
                    std::size_t limit = std::size_t(remaining_width / num_remaining_columns);
                    std::size_t i_1 = 0, i_2 = 0;
                  next:
                    {
                        std::size_t col_index = remaining_columns[i_1];
                        Col& col = columns[col_index];
                        std::size_t desired_width = col.measure_result.min_width_no_break;
                        if (desired_width <= limit) {
                            col.width_1 = desired_width;
                            remaining_width -= desired_width;
                        }
                        else {
                            remaining_columns[i_2] = col_index;
                            ++i_2;
                        }
                    }
                    ++i_1;
                    if (ARCHON_LIKELY(i_1 < num_remaining_columns))
                        goto next;
                    if (i_2 == 0)
                        break;
                    num_remaining_columns = i_2;
                    if (i_2 == i_1) {
                        for (std::size_t i = 0; i < num_remaining_columns; ++i) {
                            std::size_t col_index = remaining_columns[i];
                            Col& col = columns[col_index];
                            col.width_1 = std::max(limit, col.measure_result.min_width_no_oflow);
                        }
                        break;
                    }
                }
                // FIXME: Not good enough: This can currently expand the width beyond max_width while affording some columns more than they need. Must somehow .....    
                for (std::size_t i = 0; i < num_columns; ++i) {
                    const Col& col = columns[i];
                    log::info("Column width (stage 1): %s / %s / %s", col.measure_result.min_width_no_oflow, col.width_1, col.measure_result.min_width_no_break);
                }
                // Calculate row heights
                {
                    std::size_t sections_begin = m_sections_begin;
                    for (Row& row : m_rows) {
                        std::size_t sections_end = row.sections_end;
                        std::size_t num_columns_2 = std::size_t(sections_end - sections_begin);
                        for (std::size_t i = 0; i < num_columns_2; ++i) {
                            Col& col = columns[i];
                            std::size_t section_index = std::size_t(sections_begin + i);
                            SimulateResult result = m_formatter.simulate(section_index, col.width_1);
                            ARCHON_ASSERT(result.width <= col.width_1);
                            if (result.width > col.width_2)
                                col.width_2 = result.width;
                            if (result.height > row.height)
                                row.height = result.height;
                        }
                        sections_begin = sections_end;
                    }
                }
                for (std::size_t i = 0; i < num_columns; ++i) {
                    const Col& col = columns[i];
                    log::info("Column width (stage 2): %s / %s / %s", col.measure_result.min_width_no_oflow, col.width_2, col.measure_result.min_width_no_break);
                }
                for (Row& row : m_rows)
                    log::info("Row height: %s", row.height);
                std::size_t sections_begin = m_sections_begin;
                for (Row& row : m_rows) {
                    m_formatter.begin_hold();
                    std::size_t sections_end = row.sections_end;
                    std::size_t num_columns_2 = std::size_t(sections_end - sections_begin);
                    std::size_t i = 0;
                    for (;;) {
                        Col& col = columns[i];
                        std::size_t section_index = std::size_t(sections_begin + i);
                        m_formatter.set_width(col.width_2);
                        m_formatter.format_section(section_index);
                        ++i;
                        if (ARCHON_UNLIKELY(i == num_columns_2))
                            break;
                        m_formatter.jump_back();
                    }
                    sections_begin = sections_end;
                    m_formatter.end_hold();
                }
            }
        private:
            struct Row {
                std::size_t sections_end;
                std::size_t height;
            };
            text_formatter_type& m_formatter;
            const std::size_t m_sections_begin;
            std::vector<Row> m_rows;
        };
        Table table(formatter);
        formatter.write("Alpha\nbalpha\n");
        table.close_cell();
        formatter.write("Beta\n");
        table.close_row();
        formatter.write("Gamma\n");
        table.close_cell();
        formatter.write("Delta\nfelta\n");
        table.close_row();
        table.format(70);
    }
/*
    {
        Cursor cursor;
        MeasureResult result;
        {
            auto result_2 = formatter.measure(0, cursor);
        }
        auto result_2 = formatter.measure(2, cursor);
    }
    formatter.begin_hold();
    formatter.set_fill_color(Color::red);
    formatter.set_width(10);
    formatter.format_section(0);
    formatter.jump_back();
    formatter.set_fill_color(Color::green);
    formatter.set_width(20);
    formatter.format_section(1);
    formatter.end_hold();
    formatter.begin_hold();
    formatter.set_fill_color(Color::yellow);
    formatter.set_width(10);
    formatter.format_section(2);
    formatter.jump_back();
    formatter.set_fill_color(Color::blue);
    formatter.set_width(20);
    formatter.format_section(3);
    formatter.end_hold();
*/
    formatter.end_compile();
    formatter.pop_format();

    formatter.finalize();
    out.flush();
}
