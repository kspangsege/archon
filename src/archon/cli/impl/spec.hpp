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

#ifndef ARCHON_X_CLI_X_IMPL_X_SPEC_HPP
#define ARCHON_X_CLI_X_IMPL_X_SPEC_HPP


#include <cstddef>
#include <utility>
#include <string_view>
#include <string>
#include <vector>
#include <map>
#include <locale>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/index_range.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/format.hpp>
#include <archon/core/format_as.hpp>
#include <archon/core/format_enc.hpp>
#include <archon/cli/spec_error.hpp>
#include <archon/cli/exception.hpp>
#include <archon/cli/attributes.hpp>
#include <archon/cli/impl/option_action.hpp>
#include <archon/cli/impl/pattern_structure.hpp>
#include <archon/cli/impl/pattern_func_checker.hpp>
#include <archon/cli/impl/pattern_action.hpp>


namespace archon::cli::impl {


template<class C, class T> class Spec {
public:
    using char_type   = C;
    using traits_type = T;

    struct OptionForm;
    struct ArgSpec;
    struct ProtoOption;
    struct Pattern;
    struct Option;

    using string_view_type       = std::basic_string_view<C, T>;
    using pattern_structure_type = impl::PatternStructure<C, T>;
    using pattern_action_type    = impl::PatternAction<C, T>;
    using option_action_type     = impl::OptionAction<C, T>;

    Spec(const std::locale&) noexcept;

    auto get_num_patterns() const noexcept -> std::size_t;
    auto get_num_options() const noexcept -> std::size_t;

    auto ensure_keyword(string_view_type lexeme) -> std::size_t;
    auto ensure_pattern_option(OptionForm, std::size_t pattern_index) -> std::size_t;
    auto add_option_forms(core::Span<const OptionForm>) -> core::IndexRange;

    // Note: All options must be added before any patterns are added.
    void add_pattern(string_view_type pattern, int attr, string_view_type descr, std::size_t elem_seq_index,
                     const pattern_action_type&, const pattern_structure_type&);
    void add_option(core::IndexRange forms, ArgSpec arg, int attr, string_view_type descr, const option_action_type&);

    void shrink_to_fit();

    auto find_keyword(string_view_type name) const noexcept -> std::size_t;
    auto find_proto_option(char_type short_name) const noexcept -> std::size_t;
    auto find_proto_option(string_view_type long_name) const noexcept -> std::size_t;

    auto get_pattern(std::size_t pattern_index) const noexcept -> const Pattern&;
    auto get_proto_option(std::size_t proto_index) const noexcept -> const ProtoOption&;
    auto get_option(std::size_t option_index) const noexcept -> const Option&;
    auto get_option_forms(core::IndexRange) const noexcept -> core::Span<const OptionForm>;

private:
    const std::locale m_locale;

    // Note: If more vectors are added here, remember to deal with them in shrink_to_fit().
    std::vector<string_view_type> m_keywords;
    std::vector<ProtoOption> m_proto_options;
    std::vector<OptionForm> m_option_forms;
    std::vector<Pattern> m_patterns;
    std::vector<Option> m_options;

    std::map<string_view_type, std::size_t> m_keyword_map;      // Index into `m_keywords`
    std::map<char_type,        std::size_t> m_short_option_map; // Index into `m_proto_option`
    std::map<string_view_type, std::size_t> m_long_option_map;  // Index into `m_proto_option`
};


template<class C, class T> struct Spec<C, T>::OptionForm {
    bool is_long;
    string_view_type lexeme;
};


template<class C, class T> struct Spec<C, T>::ArgSpec {
    bool allow = false;
    bool require = false;
    string_view_type lexeme;
};


// If `pattern_index` is specified, this proto option is referenced from at least one
// pattern, and `pattern_index` refers to the first one.
//
// If `options_index` is specified, this proto option is associated with an explicitely
// specified option and `option_index` refers to that option.
//
template<class C, class T> struct Spec<C, T>::ProtoOption {
    std::size_t pattern_index = std::size_t(-1); // Index into `m_patterns`
    std::size_t option_index = std::size_t(-1); // Index into `m_options`
};


template<class C, class T> struct Spec<C, T>::Pattern {
    string_view_type pattern;
    int attr; // Bitwise OR of pattern attributes (cli::PatternAttributes)
    string_view_type descr;
    std::size_t elem_seq_index; // Index into impl::PatternStructure::seqs
    const pattern_action_type& action;
};


template<class C, class T> struct Spec<C, T>::Option {
    core::IndexRange forms; // Range of option form indexes
    ArgSpec arg;
    int attr; // Bitwise OR of option attributes (cli::OptionAttributes)
    string_view_type descr;
    const option_action_type& action;
};








// Implementation


template<class C, class T>
inline Spec<C, T>::Spec(const std::locale& locale) noexcept
    : m_locale(locale)
{
}


template<class C, class T>
inline auto Spec<C, T>::get_num_patterns() const noexcept -> std::size_t
{
    return m_patterns.size();
}


template<class C, class T>
inline auto Spec<C, T>::get_num_options() const noexcept -> std::size_t
{
    return m_options.size();
}


template<class C, class T>
auto Spec<C, T>::ensure_keyword(string_view_type lexeme) -> std::size_t
{
    std::size_t keyword_index = m_keywords.size();
    std::size_t required_capacity = keyword_index;
    core::int_add(required_capacity, 1); // Throws
    m_keywords.reserve(required_capacity); // Throws
    auto p = m_keyword_map.emplace(lexeme, keyword_index); // Throws
    bool was_inserted = p.second;
    if (was_inserted) {
        m_keywords.push_back(lexeme);
    }
    else {
        keyword_index = p.first->second;
    }
    return keyword_index;
}


template<class C, class T>
auto Spec<C, T>::ensure_pattern_option(OptionForm form, std::size_t pattern_index) -> std::size_t
{
    std::size_t proto_index = m_proto_options.size();
    std::size_t required_capacity = proto_index;
    core::int_add(required_capacity, 1); // Throws
    m_proto_options.reserve(required_capacity); // Throws
    if (!form.is_long) {
        // Short form
        auto p = m_short_option_map.emplace(form.lexeme[1], proto_index); // Throws
        bool was_inserted = p.second;
        if (was_inserted) {
            m_proto_options.push_back({});
        }
        else {
            proto_index = p.first->second;
        }
    }
    else {
        // Long form
        auto p = m_long_option_map.emplace(form.lexeme.substr(2), proto_index); // Throws
        bool was_inserted = p.second;
        if (was_inserted) {
            m_proto_options.push_back({});
        }
        else {
            proto_index = p.first->second;
        }
    }
    ProtoOption& proto = m_proto_options[proto_index];
    auto error = [&](const char* message, const auto&... params) {
        std::string message_2 = core::format_enc<char_type>(m_locale, "Error in %s pattern specification: %s",
                                                            core::as_ordinal(1 + pattern_index),
                                                            core::formatted(message, params...)); // Throws
        throw cli::BadSpec(cli::SpecError::bad_option_ref, std::move(message_2));
    };
    if (proto.option_index != std::size_t(-1)) {
        const Option& option = m_options[proto.option_index];
        if (ARCHON_UNLIKELY(option.arg.allow))
            error("Reference to option (%s) that takes argument", form.lexeme); // Throws
        if (ARCHON_UNLIKELY((option.attr & cli::short_circuit) != 0))
            error("Reference to 'short circuit' option (%s)", form.lexeme); // Throws
        if (ARCHON_UNLIKELY((option.attr & cli::further_args_are_values) != 0))
            error("Reference to 'further args are values' option (%s)", form.lexeme); // Throws
    }
    if (proto.pattern_index == std::size_t(-1))
        proto.pattern_index = pattern_index;
    return proto_index;
}


template<class C, class T>
inline auto Spec<C, T>::add_option_forms(core::Span<const OptionForm> forms) -> core::IndexRange
{
    core::IndexRange range { m_option_forms.size(), forms.size() };
    m_option_forms.insert(m_option_forms.end(), forms.begin(), forms.end()); // Throws
    return range;
}


template<class C, class T>
inline void Spec<C, T>::add_pattern(string_view_type pattern, int attr, string_view_type descr,
                                    std::size_t elem_seq_index, const pattern_action_type& action,
                                    const pattern_structure_type& pattern_structure)
{
    std::size_t pattern_index = m_patterns.size();
    auto error = [&](cli::SpecError code, const char* message, const auto&... params) {
        std::string message_2 = core::format_enc<char_type>(m_locale, "Error in %s pattern specification: %s",
                                                            core::as_ordinal(1 + pattern_index),
                                                            core::formatted(message, params...)); // Throws
        throw cli::BadSpec(code, std::move(message_2));
    };

    int all_attributes = cli::completing;
    if (ARCHON_UNLIKELY((attr & ~all_attributes) != 0))
        error(cli::SpecError::bad_pattern_attr, "Invalid attributes"); // Throws

    // Note: A delegating pattern with value slots will be rejected during parsing of the
    // pattern.
    if (ARCHON_LIKELY(!action.is_deleg)) {
        impl::PatternFuncChecker<C, T> pattern_func_checker(pattern_structure);
        if (ARCHON_UNLIKELY(!action.check(pattern_func_checker, elem_seq_index))) {
            error(cli::SpecError::pattern_func_mismatch,
                  "Mismatch between pattern and pattern function"); // Throws
        }
    }

    m_patterns.push_back({ pattern, attr, descr, elem_seq_index, action }); // Throws
}


template<class C, class T>
void Spec<C, T>::add_option(core::IndexRange forms, ArgSpec arg, int attr, string_view_type descr,
                            const option_action_type& action)
{
    ARCHON_ASSERT(m_patterns.empty());

    std::size_t option_index = m_options.size();
    auto error = [&](cli::SpecError code, const char* message, const auto&... params) {
        std::string message_2 = core::format_enc<char_type>(m_locale, "Error in %s option specification: %s",
                                                            core::as_ordinal(1 + option_index),
                                                            core::formatted(message, params...)); // Throws
        throw cli::BadSpec(code, std::move(message_2));
    };

    if (ARCHON_UNLIKELY(forms.size == 0))
        error(cli::SpecError::bad_option_forms_syntax, "No option forms"); // throws
    int all_attributes = cli::short_circuit | cli::further_args_are_values | cli::unlisted;
    if (ARCHON_UNLIKELY((attr & ~all_attributes) != 0))
        error(cli::SpecError::bad_option_attr, "Invalid attributes"); // Throws
    if (arg.allow) {
        if (ARCHON_UNLIKELY((attr & cli::short_circuit) != 0)) {
            error(cli::SpecError::option_arg_not_allowed,
                  "Option argument is not allowed for 'short circuit' options"); // Throws
        }
        if (ARCHON_UNLIKELY((attr & cli::further_args_are_values) != 0)) {
            error(cli::SpecError::option_arg_not_allowed, "Option argument is not allowed for "
                  "'further args are values' options"); // Throws
        }
        if (ARCHON_UNLIKELY(!action.allow_arg())) {
            error(cli::SpecError::option_func_mismatch,
                  "Option action does not accept an option argument"); // Throws
        }
        if (ARCHON_UNLIKELY(!arg.require && action.require_arg())) {
            error(cli::SpecError::option_func_mismatch,
                  "Option action does not allow for option argument to be optional"); // Throws
        }
    }

    // Verify that no option form is used in previously added option
    for (OptionForm form : get_option_forms(forms)) {
        std::size_t proto_index_2 = std::size_t(-1);
        if (!form.is_long) {
            // Short form
            auto i = m_short_option_map.find(form.lexeme[1]);
            bool was_found = (i != m_short_option_map.end());
            if (ARCHON_LIKELY(!was_found))
                continue;
            proto_index_2 = i->second;
        }
        else {
            // Long form
            auto i = m_long_option_map.find(form.lexeme.substr(2));
            bool was_found = (i != m_long_option_map.end());
            if (ARCHON_LIKELY(!was_found))
                continue;
            proto_index_2 = i->second;
        }
        const ProtoOption& proto = m_proto_options[proto_index_2];
        // The following assertion follows from the requirement that all optiions are added
        // before any patterns are added.
        ARCHON_ASSERT(proto.option_index != std::size_t(-1));
        error(cli::SpecError::option_form_reuse, "Option form %s also used in %s option "
              "specification", form.lexeme, core::as_ordinal(1 + proto.option_index)); // Throws
    }

    // Register option
    m_options.push_back({ forms, arg, attr, descr, action }); // Throws
    std::size_t proto_index = m_proto_options.size();
    m_proto_options.push_back({}); // Throws
    m_proto_options.back().option_index = option_index;

    // Register new option forms
    for (OptionForm form : get_option_forms(forms)) {
        if (!form.is_long) {
            // Short form
            m_short_option_map.emplace(form.lexeme[1], proto_index); // Throws
        }
        else {
            // Long form
            m_long_option_map.emplace(form.lexeme.substr(2), proto_index); // Throws
        }
    }
}


template<class C, class T>
void Spec<C, T>::shrink_to_fit()
{
    m_keywords.shrink_to_fit(); // Throws
    m_proto_options.shrink_to_fit(); // Throws
    m_option_forms.shrink_to_fit(); // Throws
    m_patterns.shrink_to_fit(); // Throws
    m_options.shrink_to_fit(); // Throws
}


template<class C, class T>
inline auto Spec<C, T>::find_keyword(string_view_type name) const noexcept -> std::size_t
{
    auto i = m_keyword_map.find(name);
    if (ARCHON_LIKELY(i == m_keyword_map.end()))
        return std::size_t(-1);
    std::size_t keyword_index = i->second;
    return keyword_index;
}


template<class C, class T>
inline auto Spec<C, T>::find_proto_option(char_type short_name) const noexcept -> std::size_t
{
    auto i = m_short_option_map.find(short_name);
    if (ARCHON_LIKELY(i == m_short_option_map.end()))
        return std::size_t(-1);
    std::size_t proto_index = i->second;
    return proto_index;
}


template<class C, class T>
inline auto Spec<C, T>::find_proto_option(string_view_type long_name) const noexcept -> std::size_t
{
    auto i = m_long_option_map.find(long_name);
    if (ARCHON_LIKELY(i == m_long_option_map.end()))
        return std::size_t(-1);
    std::size_t proto_index = i->second;
    return proto_index;
}


template<class C, class T>
inline auto Spec<C, T>::get_pattern(std::size_t pattern_index) const noexcept -> const Pattern&
{
    return m_patterns[pattern_index];
}


template<class C, class T>
inline auto Spec<C, T>::get_proto_option(std::size_t proto_index) const noexcept -> const ProtoOption&
{
    return m_proto_options[proto_index];
}


template<class C, class T>
inline auto Spec<C, T>::get_option(std::size_t option_index) const noexcept -> const Option&
{
    return m_options[option_index];
}


template<class C, class T>
inline auto Spec<C, T>::get_option_forms(core::IndexRange range) const noexcept -> core::Span<const OptionForm>
{
    return range.resolve(m_option_forms.data());
}


} // namespace archon::cli::impl

#endif // ARCHON_X_CLI_X_IMPL_X_SPEC_HPP
