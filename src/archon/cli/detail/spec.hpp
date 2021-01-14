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

#ifndef ARCHON_X_CLI_X_DETAIL_X_SPEC_HPP
#define ARCHON_X_CLI_X_DETAIL_X_SPEC_HPP

/// \file


#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <string>
#include <vector>
#include <map>
#include <locale>

#include <archon/base/span.hpp>
#include <archon/base/format.hpp>
#include <archon/base/format_enc.hpp>
#include <archon/base/format_as.hpp>
#include <archon/cli/detail/pattern_action.hpp>
#include <archon/cli/detail/option_action.hpp>


namespace archon::cli::detail {


template<class C, class T> class Spec {
public:
    struct OptionForm;
    struct ArgSpec;

    using char_type        = C;
    using traits_type      = T;
    using string_view_type = std::basic_string_view<C, T>;

    using pattern_action_type = PatternAction<C, T>;
    using option_action_type  = OptionAction<C, T>;

    Spec(const std::locale&) noexcept;

    std::size_t get_num_options() const noexcept;

    void add_option(base::Span<const OptionForm> forms, ArgSpec arg, int attr,
                    string_view_type descr, std::unique_ptr<option_action_type>);

private:
    struct ProtoOption;
    struct Pattern;
    struct Option;

    const std::locale m_locale;
    std::vector<string_view_type> m_keywords;
    std::vector<ProtoOption> m_proto_options;
    std::vector<Pattern> m_patterns;
    std::vector<Option> m_options;

    std::map<string_view_type, std::size_t> m_keyword_map;        // Index into `m_keywords`
    std::map<char_type,        std::size_t> m_short_option_forms; // Index into `m_proto_option`
    std::map<string_view_type, std::size_t> m_long_option_forms;  // Index into `m_proto_option`

//    Nfa m_nfa;
};


template<class C, class T> struct Spec<C, T>::OptionForm {
    bool is_long;
    string_view_type lexeme;
};


template<class C, class T> struct Spec<C, T>::ArgSpec {
    bool allow = false;
    bool need  = false;
    string_view_type lexeme;
};








// Implementation


template<class C, class T> struct Spec<C, T>::ProtoOption {
    bool in_pattern = false;
    std::size_t option_index = std::size_t(-1); // Index into `m_options`
};


template<class C, class T> struct Spec<C, T>::Pattern {
    string_view_type spec;
    string_view_type descr;
    std::unique_ptr<pattern_action_type> action;
};


template<class C, class T> struct Spec<C, T>::Option {
    base::Span<const OptionForm> forms;
    ArgSpec arg;
    int attr = 0; // Bitwise OR of OptionAttributes
    string_view_type descr;
    std::unique_ptr<option_action_type> action;
};


template<class C, class T> inline Spec<C, T>::Spec(const std::locale& locale) noexcept :
    m_locale(locale)
{
}


template<class C, class T> inline std::size_t Spec<C, T>::get_num_options() const noexcept
{
    return m_options.size();
}


template<class C, class T>
void Spec<C, T>::add_option(base::Span<const OptionForm> forms, ArgSpec arg, int attr,
                            string_view_type descr, std::unique_ptr<option_action_type> action)
{
    std::size_t opt_ndx = m_options.size();
    auto error = [&](const char* message, const auto&... params) {
        std::string message_2 =
            base::format_enc<char_type>(m_locale, "Error in %s option specification: %s",
                                        base::as_ordinal(opt_ndx + 1),
                                        base::formatted(message, params...)); // Throws
        throw std::invalid_argument(std::move(message_2));
    };

    if (forms.size() == 0)
        error("No option forms"); // throws
    int all_attributes = short_circuit | end_of_options;
    if ((attr & ~all_attributes) != 0)
        error("Invalid attributes"); // Throws
    if (arg.allow) {
        if ((attr & short_circuit) != 0)
            error("Option argument is not allowed for 'short circuit' options"); // Throws
        if ((attr & end_of_options) != 0)
            error("Option argument is not allowed for 'end of options' options"); // Throws
        bool action_allows_arg = (action && action->allow_arg());
        if (!action_allows_arg)
            error("Option action does not accept an option argument"); // Throws
    }

    // Check if this option is a promotion of a proto option
    std::size_t proto_ndx = std::size_t(-1);
    for (OptionForm form : forms) {
        std::size_t proto_ndx_2 = std::size_t(-1);
        if (!form.is_long) {
            // Short form
            auto i = m_short_option_forms.find(form.lexeme[1]);
            bool was_found = (i != m_short_option_forms.end());
            if (ARCHON_LIKELY(!was_found))
                continue;
            proto_ndx_2 = i->second;
        }
        else {
            // Long form
            auto i = m_long_option_forms.find(form.lexeme.substr(2));
            bool was_found = (i != m_long_option_forms.end());
            if (ARCHON_LIKELY(!was_found))
                continue;
            proto_ndx_2 = i->second;
        }
        const ProtoOption& proto = m_proto_options[proto_ndx_2];
        if (ARCHON_LIKELY(proto.option_index == std::size_t(-1))) {
            ARCHON_ASSERT(proto.in_pattern);
            if (ARCHON_LIKELY(proto_ndx == std::size_t(-1) || proto_ndx == proto_ndx_2)) {
                proto_ndx = proto_ndx_2;
                continue;
            }
            // FIXME: Use `util::format_2("Foo %s bar", 7)`                                                 
            throw std::runtime_error("Ambiguous reference to implicitely specified option");                                          
        }
        error("Option form %s also used in %s option specification", form.lexeme,
              base::as_ordinal(proto.option_index + 1)); // Throws
    }

    // Register option
    m_options.push_back({ forms, arg, attr, descr, std::move(action) }); // Throws

    // Upsert proto option
    if (ARCHON_LIKELY(proto_ndx == std::size_t(-1))) {
        // New option
        proto_ndx = m_proto_options.size();
        bool in_pattern = false;
        m_proto_options.push_back({ in_pattern, opt_ndx });
    }
    else {
        // Promotion
        m_proto_options[proto_ndx].option_index = opt_ndx;
    }

    // Register new option forms
    for (OptionForm form : forms) {
        if (!form.is_long) {
            // Short form
            m_short_option_forms.emplace(form.lexeme[1], proto_ndx); // Throws
        }
        else {
            // Long form
            m_long_option_forms.emplace(form.lexeme.substr(2), proto_ndx); // Throws
        }
    }
}


} // namespace archon::cli::detail

#endif // ARCHON_X_CLI_X_DETAIL_X_SPEC_HPP
