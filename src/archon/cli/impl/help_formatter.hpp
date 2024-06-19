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

#ifndef ARCHON_X_CLI_X_IMPL_X_HELP_FORMATTER_HPP
#define ARCHON_X_CLI_X_IMPL_X_HELP_FORMATTER_HPP


#include <cstddef>
#include <utility>
#include <string_view>
#include <string>
#include <locale>
#include <ios>
#include <ostream>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/locale.hpp>
#include <archon/core/seed_memory_output_stream.hpp>
#include <archon/core/format.hpp>
#include <archon/core/format_as.hpp>
#include <archon/core/format_enc.hpp>
#include <archon/core/quote.hpp>
#include <archon/core/string_template.hpp>
#include <archon/core/text_formatter.hpp>
#include <archon/cli/help_spec_error.hpp>
#include <archon/cli/exception.hpp>
#include <archon/cli/help_config.hpp>


namespace archon::cli::impl {


template<class C, class T> class HelpFormatter {
public:
    using char_type   = C;
    using traits_type = T;

    using string_view_type = std::basic_string_view<C, T>;
    using ostream_type     = std::basic_ostream<C, T>;
    using help_config_type = cli::BasicHelpConfig<C, T>;
    using spec_type        = impl::Spec<C, T>;

    using Pattern    = typename spec_type::Pattern;
    using Option     = typename spec_type::Option;
    using OptionForm = typename spec_type::OptionForm;

    HelpFormatter(string_view_type argv0, core::Span<const string_view_type> parent_patterns, const spec_type&,
                  ostream_type&, const std::locale&, help_config_type&&, bool terminal_supports_sgr_esc_seq,
                  int terminal_width);

    void format();

private:
    using text_formatter_type  = core::BasicTextFormatter<C, T>;
    using string_template_type = core::BasicStringTemplate<string_view_type, Option>;

    using TemplateParseError = typename string_template_type::Parser::Error;

    string_view_type m_argv0;
    core::Span<const string_view_type> m_parent_patterns;
    const spec_type& m_spec;
    std::locale m_locale;
    help_config_type m_help_config;
    bool m_terminal_supports_sgr_esc_seq;
    int m_terminal_width;

    impl::ValueFormatter<C, T> m_value_formatter;
    text_formatter_type m_text_formatter;
    core::BasicSeedMemoryOutputStream<C, T> m_out_value;

    static auto get_text_formatter_config(const help_config_type&, bool terminal_supports_sgr_esc_seq,
                                          const std::locale&) -> typename text_formatter_type::Config;

    void format_patterns();
    void format_options();
    bool format_option_orig_val(ostream_type&, const Option&, bool& has_value);
    bool format_option_default_arg(ostream_type&, const Option&, bool& has_value);
    bool format_option_enum_values(ostream_type&, const Option&, bool disjunctive, bool quote);
    void format_absent_value(ostream_type&);

    static auto map_template_parse_error(TemplateParseError) -> cli::HelpSpecError;
    template<class... P> [[noreturn]] void error(cli::HelpSpecError, const char* message, const P&... params) const;
};








// Implementation


template<class C, class T>
inline HelpFormatter<C, T>::HelpFormatter(string_view_type argv0, core::Span<const string_view_type> parent_patterns,
                                          const spec_type& spec, ostream_type& out, const std::locale& locale,
                                          help_config_type&& help_config, bool terminal_supports_sgr_esc_seq,
                                          int terminal_width)
    : m_argv0(argv0)
    , m_parent_patterns(parent_patterns)
    , m_spec(spec)
    , m_locale(locale)
    , m_help_config(std::move(help_config))
    , m_terminal_supports_sgr_esc_seq(terminal_supports_sgr_esc_seq)
    , m_terminal_width(terminal_width)
    , m_value_formatter(m_locale) // Throws
    , m_text_formatter(out, get_text_formatter_config(help_config, terminal_supports_sgr_esc_seq, locale)) // Throws
{
    m_out_value.exceptions(std::ios_base::badbit | std::ios_base::failbit); // Throws
    m_out_value.imbue(m_locale); // Throws
}


template<class C, class T>
void HelpFormatter<C, T>::format()
{
    std::size_t width = m_help_config.width;
    if (m_help_config.allow_adjust_width_to_terminal && m_terminal_width >= 0)
        core::int_cast_clamp(m_terminal_width, width, m_help_config.min_width, m_help_config.max_width);
    m_text_formatter.set_width(width); // Throws
    m_text_formatter.set_padding_right(m_help_config.right_side_margin); // Throws
    format_patterns(); // Throws
    format_options(); // Throws
    m_text_formatter.write("\n"); // Throws
    m_text_formatter.finalize(); // Throws
}


template<class C, class T>
auto HelpFormatter<C, T>::get_text_formatter_config(const help_config_type& help_config,
                                                    bool terminal_supports_sgr_esc_seq,
                                                    const std::locale& locale) -> typename text_formatter_type::Config
{
    typename text_formatter_type::Config config;
    config.enable_ansi_escape_sequences = (help_config.allow_terminal_sgr_esc_seq && terminal_supports_sgr_esc_seq &&
                                           core::assume_locale_has_escape(locale)); // Throws
    return config;
}


template<class C, class T>
void HelpFormatter<C, T>::format_patterns()
{
    m_text_formatter.write("\n"); // Throws
    m_text_formatter.push_format(); // Throws
    std::size_t n = m_spec.get_num_patterns();
    for (std::size_t i = 0; i < n; ++i) {
        const Pattern& pattern = m_spec.get_pattern(i);
        m_text_formatter.set_indent(0);
        if (i == 0) {
            m_text_formatter.write("Usage: "); // Throws
        }
        else {
            m_text_formatter.write("   or: "); // Throws
        }
        m_text_formatter.close_section(); // Throws
        std::size_t pos = m_text_formatter.get_cursor_pos();
        m_text_formatter.set_indent(pos, std::size_t(pos + 2));
        m_text_formatter.format("%s", core::smart_quoted(m_argv0)); // Throws
        for (string_view_type parent_pattern : m_parent_patterns) {
            if (!parent_pattern.empty()) {
                m_text_formatter.write("  "); // Throws
                m_text_formatter.write(parent_pattern); // Throws
            }
        }
        if (!pattern.pattern.empty()) {
            m_text_formatter.write("  "); // Throws
            m_text_formatter.write(pattern.pattern); // Throws
        }
        m_text_formatter.write("\n"); // Throws
    }
    m_text_formatter.pop_format(); // Throws
}


template<class C, class T>
void HelpFormatter<C, T>::format_options()
{
    core::Vector<std::size_t, 12> options;
    std::size_t n = m_spec.get_num_options();
    for (std::size_t i = 0; i < n; ++i) {
        const Option& opt = m_spec.get_option(i);
        bool unlisted = ((opt.attr & cli::OptionAttributes::unlisted) != 0);
        if (!unlisted)
            options.push_back(i); // Throws
    }
    if (options.empty())
        return;

    m_text_formatter.write("\n"); // Throws

    std::size_t max_quoted_size = 32;

    std::size_t option_index = 0;
    auto descr_error = [&](cli::HelpSpecError code, const char* message, const auto&... params) {
        error(code, "Error in description of %s command-line option: %s", core::as_ordinal(1 + option_index),
              core::formatted(message, params...)); // Throws
    };

    typename string_template_type::Parser parser(m_locale); // Throws
    typename string_template_type::Parameters params;
    params["A"] = [&](ostream_type& out, const Option& opt) {
        if (ARCHON_LIKELY(opt.arg.allow)) {
            out << opt.arg.lexeme; // Throws
            return;
        }
        descr_error(cli::HelpSpecError::illegal_combination,
                    "Illegal parameter reference `@A`: Option does not accept an argument"); // Throws
    }; // Throws
    params["V"] = [&](ostream_type& out, const Option& opt) {
        bool has_value = false;
        if (ARCHON_LIKELY(format_option_orig_val(out, opt, has_value))) { // Throws
            if (ARCHON_LIKELY(has_value))
                return;
            format_absent_value(out); // Throws
            return;
        }
        descr_error(cli::HelpSpecError::illegal_combination,
                    "Illegal parameter reference `@V`: Option action does not provide an 'original value'"); // Throws
    }; // Throws
    params["W"] = [&](ostream_type& out, const Option& opt) {
        bool has_value = false;
        if (ARCHON_LIKELY(format_option_default_arg(out, opt, has_value))) { // Throws
            if (ARCHON_LIKELY(has_value))
                return;
            format_absent_value(out); // Throws
            return;
        }
        descr_error(cli::HelpSpecError::illegal_combination,
                    "Illegal parameter reference `@W`: Option action does not provide a 'default argument'"); // Throws
    }; // Throws
    params["Q"] = [&](ostream_type& out, const Option& opt) {
        m_out_value.full_clear();
        bool has_value = false;
        if (ARCHON_LIKELY(format_option_orig_val(m_out_value, opt, has_value))) { // Throws
            if (ARCHON_LIKELY(has_value)) {
                out << core::quoted(m_out_value.view(), max_quoted_size); // Throws
                return;
            }
            format_absent_value(out); // Throws
            return;
        }
        descr_error(cli::HelpSpecError::illegal_combination,
                    "Illegal parameter reference `@Q`: Option action does not provide an 'original value'"); // Throws
    }; // Throws
    params["R"] = [&](ostream_type& out, const Option& opt) {
        m_out_value.full_clear();
        bool has_value = false;
        if (ARCHON_LIKELY(format_option_default_arg(m_out_value, opt, has_value))) { // Throws
            if (ARCHON_LIKELY(has_value)) {
                out << core::quoted(m_out_value.view(), max_quoted_size); // Throws
                return;
            }
            format_absent_value(out); // Throws
            return;
        }
        descr_error(cli::HelpSpecError::illegal_combination,
                    "Illegal parameter reference `@R`: Option action does not provide a 'default argument'"); // Throws
    }; // Throws
    params["E"] = [&](ostream_type& out, const Option& opt) {
        bool disjunctive = false;
        bool quote = false;
        if (ARCHON_LIKELY(format_option_enum_values(out, opt, disjunctive, quote))) // Throws
            return;
        descr_error(cli::HelpSpecError::illegal_combination,
                    "Illegal parameter reference `@E`: Option value is not of enumeration type"); // Throws
    }; // Throws
    params["F"] = [&](ostream_type& out, const Option& opt) {
        bool disjunctive = true;
        bool quote = false;
        if (ARCHON_LIKELY(format_option_enum_values(out, opt, disjunctive, quote))) // Throws
            return;
        descr_error(cli::HelpSpecError::illegal_combination,
                    "Illegal parameter reference `@E`: Option value is not of enumeration type"); // Throws
    }; // Throws
    params["G"] = [&](ostream_type& out, const Option& opt) {
        bool disjunctive = false;
        bool quote = true;
        if (ARCHON_LIKELY(format_option_enum_values(out, opt, disjunctive, quote))) // Throws
            return;
        descr_error(cli::HelpSpecError::illegal_combination,
                    "Illegal parameter reference `@E`: Option value is not of enumeration type"); // Throws
    }; // Throws
    params["H"] = [&](ostream_type& out, const Option& opt) {
        bool disjunctive = true;
        bool quote = true;
        if (ARCHON_LIKELY(format_option_enum_values(out, opt, disjunctive, quote))) // Throws
            return;
        descr_error(cli::HelpSpecError::illegal_combination,
                    "Illegal parameter reference `@E`: Option value is not of enumeration type"); // Throws
    }; // Throws

    m_text_formatter.write("Options:\n"); // Throws
    m_text_formatter.push_format(); // Throws
    m_text_formatter.set_adv_continuation(true); // Throws
    m_text_formatter.set_min_separation(m_help_config.option_descr_min_separation); // Throws
    m_text_formatter.set_max_displacement(m_help_config.option_descr_max_displacement); // Throws

    string_template_type templ;
    for (std::size_t i : options) {
        option_index = i;
        const Option& opt = m_spec.get_option(i);
        m_text_formatter.set_justify(false); // Throws
        m_text_formatter.set_indent(m_help_config.option_form_indent_1, m_help_config.option_form_indent_2); // Throws
        core::Span<const OptionForm> opt_forms = m_spec.get_option_forms(opt.forms);
        std::size_t j = 0, m = opt_forms.size();
        ARCHON_ASSERT(m > 0);
        for (;;) {
            const OptionForm& form = opt_forms[j++];
            using Weight = typename text_formatter_type::Weight;
            m_text_formatter.set_weight(Weight::bold); // Throws
            m_text_formatter.write(form.lexeme); // Throws
            m_text_formatter.set_weight(Weight::normal); // Throws
            bool is_last = (j == m);
            if (is_last || m_help_config.show_opt_arg_on_all_forms) {
                if (opt.arg.allow) {
                    if (!opt.arg.require)
                        m_text_formatter.write("["); // Throws
                    if (form.is_long) {
                        m_text_formatter.set_weight(Weight::bold); // Throws
                        m_text_formatter.write("="); // Throws
                        m_text_formatter.set_weight(Weight::normal); // Throws
                    }
                    m_text_formatter.write(opt.arg.lexeme);
                    if (!opt.arg.require)
                        m_text_formatter.write("]"); // Throws
                }
            }
            if (is_last)
                break;
            m_text_formatter.write(", "); // Throws
        }
        m_text_formatter.close_section(); // Throws
        auto error_handler = [&](TemplateParseError parse_error, string_view_type message) -> bool {
            cli::HelpSpecError spec_error = map_template_parse_error(parse_error);
            descr_error(spec_error, "%s", message); // Throws
            return false;
        };
        bool success = parser.try_parse(opt.descr, params, templ, error_handler); // Throws
        ARCHON_ASSERT(success);
        m_text_formatter.set_indent(m_help_config.option_descr_indent); // Throws
        if (m_help_config.justify)
            m_text_formatter.set_justify(true); // Throws
        m_text_formatter.writeln(expand(templ, opt)); // Throws
    }
    m_text_formatter.pop_format(); // Throws
}


template<class C, class T>
inline bool HelpFormatter<C, T>::format_option_orig_val(ostream_type& out, const Option& opt, bool& has_value)
{
    return opt.action.format_orig_val(out, m_value_formatter, has_value); // Throws
}


template<class C, class T>
inline bool HelpFormatter<C, T>::format_option_default_arg(ostream_type& out, const Option& opt, bool& has_value)
{
    return opt.action.format_default_arg(out, m_value_formatter, has_value); // Throws
}


template<class C, class T>
inline bool HelpFormatter<C, T>::format_option_enum_values(ostream_type& out, const Option& opt,
                                                           bool disjunctive, bool quote)
{
    return opt.action.format_enum_values(out, m_value_formatter, disjunctive, quote); // Throws
}


template<class C, class T>
inline void HelpFormatter<C, T>::format_absent_value(ostream_type& out)
{
    out << m_help_config.absent_value_label; // Throws
}


template<class C, class T>
inline auto HelpFormatter<C, T>::map_template_parse_error(TemplateParseError parse_error) -> cli::HelpSpecError
{
    switch (parse_error) {
        case TemplateParseError::bad_syntax:
            return cli::HelpSpecError::bad_syntax;
        case TemplateParseError::bad_param_ref:
            return cli::HelpSpecError::bad_param_ref;
    }
    ARCHON_ASSERT_UNREACHABLE();
    return {};
}


template<class C, class T>
template<class... P> void HelpFormatter<C, T>::error(cli::HelpSpecError code, const char* message,
                                                     const P&... params) const
{
    std::string message_2 = core::format_enc<char_type>(m_locale, message, params...); // Throws
    throw cli::BadHelpSpec(code, std::move(message_2));
}


} // namespace archon::cli::impl

#endif // ARCHON_X_CLI_X_IMPL_X_HELP_FORMATTER_HPP
