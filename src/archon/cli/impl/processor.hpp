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

#ifndef ARCHON_X_CLI_X_IMPL_X_PROCESSOR_HPP
#define ARCHON_X_CLI_X_IMPL_X_PROCESSOR_HPP


#include <cstddef>
#include <algorithm>
#include <utility>
#include <string_view>
#include <string>
#include <vector>
#include <locale>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/char_mapper.hpp>
#include <archon/core/text_codec.hpp>
#include <archon/core/quote.hpp>
#include <archon/core/string_formatter.hpp>
#include <archon/cli/impl/value_parser.hpp>
#include <archon/cli/proc_error.hpp>
#include <archon/cli/attributes.hpp>
#include <archon/cli/impl/root_state.hpp>
#include <archon/cli/impl/error_accum.hpp>
#include <archon/cli/impl/option_occurrence.hpp>
#include <archon/cli/impl/option_invocation.hpp>
#include <archon/cli/impl/pattern_structure.hpp>
#include <archon/cli/impl/pattern_args_parser.hpp>
#include <archon/cli/impl/spec.hpp>
#include <archon/cli/impl/pattern_matcher.hpp>


namespace archon::cli::impl {


template<class C, class T> class Processor {
public:
    using char_type   = C;
    using traits_type = T;

    using string_view_type         = std::basic_string_view<C, T>;
    using logger_type              = log::BasicLogger<C, T>;
    using pattern_structure_type   = impl::PatternStructure<C, T>;
    using spec_type                = impl::Spec<C, T>;
    using value_parser_type        = impl::ValueParser<C, T>;
    using error_accum_type         = impl::ErrorAccum<C, T>;
    using pattern_args_parser_type = impl::PatternArgsParser<C, T>;
    using pattern_matcher_type     = impl::PatternMatcher<C, T>;
    using option_invocation_type   = impl::OptionInvocation<C, T>;
    using root_state_type          = impl::RootState<C, T>;

    using Pattern     = typename spec_type::Pattern;
    using ProtoOption = typename spec_type::ProtoOption;
    using Option      = typename spec_type::Option;
    using Desc        = typename pattern_args_parser_type::Desc;

    Processor(const root_state_type&, std::size_t args_offset, bool parent_has_error, const pattern_structure_type&,
              const spec_type&, value_parser_type&, error_accum_type&, impl::PatternMatcherConfig);

    // If a short-circuit option was acted upon, `true` is returned after setting `shorted`
    // to `true` and leaving `deleg`, `pattern`, and `deleg_args_offset` untouched.
    //
    // Otherwise, if a pattern was not matched, `false` is returned. `shorted`, `deleg`,
    // `pattern`, and `deleg_args_offset` are left untouched in this case.
    //
    // Otherwise, if the matched pattern is not a delegating pattern, `true` is returned
    // after setting `shorted` to `false`, `deleg` to `false`, and `pattern` to the matched
    // pattern (null for degenerate case where no patterns were
    // specified). `deleg_args_offset` is left untouched in this case. `has_error()` should
    // be called in order to know whethr any processing errors has occurred.
    //
    // Otherwise the matched pattern is a delegating pattern. In this case, `true` is
    // returned after setting `shorted` to `false`, `deleg` to `true`, `pattern` to the
    // matched pattern (never null), and `deleg_args_offset` to the offset of delegation.
    //
    bool process(std::vector<option_invocation_type>& option_invocations, bool& shorted, bool& deleg,
                 const Pattern*& pattern, std::size_t& deleg_args_offset);

    void invoke_options(core::Span<const option_invocation_type>);

    void generate_descs(std::size_t elem_seq_index, std::vector<Desc>&);

    bool has_error() const noexcept;

private:
    using text_codec_type = core::BasicTextCodec<C, T>;

    const root_state_type& m_root_state;
    const std::size_t m_args_offset;
    const pattern_structure_type& m_pattern_structure;
    const std::locale m_locale;
    const spec_type& m_spec;
    value_parser_type& m_value_parser;
    error_accum_type& m_error_accum;
    pattern_matcher_type m_pattern_matcher;
    bool m_has_error;
    string_view_type m_arg_1, m_arg_2;
    impl::OptionOccurrence m_option_occurrence;
    std::basic_string<C, T> m_opt_form_buf, m_opt_context_buf;
    core::BasicStringFormatter<C, T> m_formatter;

    auto unpack_option(std::size_t proto_index, bool& opt_in_pattern) const -> const Option*;
    auto build_opt_form() -> string_view_type;
    auto build_opt_context() -> string_view_type;
    template<class... P> void error(std::size_t arg_index, cli::ProcError, const char* message, const P&... params);
};








// Implementation


template<class C, class T>
inline Processor<C, T>::Processor(const root_state_type& root_state, std::size_t args_offset, bool parent_has_error,
                                  const pattern_structure_type& pattern_structure, const spec_type& spec,
                                  value_parser_type& value_parser, error_accum_type& error_accum,
                                  impl::PatternMatcherConfig pattern_matcher_config)
    : m_root_state(root_state)
    , m_args_offset(args_offset)
    , m_pattern_structure(pattern_structure)
    , m_locale(root_state.locale)
    , m_spec(spec)
    , m_value_parser(value_parser)
    , m_error_accum(error_accum)
    , m_pattern_matcher(m_locale, spec, pattern_structure, root_state.args.size(),
                        std::move(pattern_matcher_config)) // Throws
    , m_has_error(parent_has_error)
    , m_formatter(m_locale) // Throws
{
}


template<class C, class T>
bool Processor<C, T>::process(std::vector<option_invocation_type>& option_invocations, bool& shorted, bool& deleg,
                              const Pattern*& pattern, std::size_t& deleg_args_offset)
{
    core::Span<const string_view_type> args = m_root_state.args;
    std::size_t arg_index = m_args_offset;
    ARCHON_ASSERT(arg_index <= args.size());
    std::size_t max_val_size = m_root_state.show_arg_max_size;
    bool has_pattern_match_error = false;
    bool remaining_args_are_values = false;
    std::size_t offset = 0;
    bool opt_in_pattern = false;
    std::size_t proto_index = 0;
    const Pattern* pat = nullptr;
    const Option* opt = nullptr;

    core::BasicCharMapper<C, T> char_mapper(m_locale); // Throws
    char_type dash  = char_mapper.widen('-'); // Throws
    char_type equal = char_mapper.widen('='); // Throws

    // FIXME: Want: On pattern match error (too few args, wrong type of arg, too many args),
    // if there was only one possible match so far, report a partial pattern match, and
    // invoke pattern args parser with early stop condition.                       

    goto next_arg_2;

  next_arg_1:
    ++arg_index;

  next_arg_2:
    if (ARCHON_LIKELY(!has_pattern_match_error)) {
        if (ARCHON_LIKELY(!m_pattern_matcher.is_match(pat))) {
            if (ARCHON_UNLIKELY(arg_index == args.size())) {
                error(arg_index, cli::ProcError::no_pattern_match,
                      "Too few command-line arguments"); // Throws
                has_pattern_match_error = true;
                goto error;
            }
        }
        else {
            if (ARCHON_UNLIKELY(pat->action.is_deleg))
                goto deleg;
            if (ARCHON_UNLIKELY(arg_index == args.size())) {
                if (ARCHON_LIKELY(!m_has_error))
                    goto invoke;
                goto error;
            }
        }
    }
    else {
        ARCHON_ASSERT(m_has_error);
        if (ARCHON_UNLIKELY(arg_index == args.size()))
            goto error;
    }

    ARCHON_ASSERT(arg_index < args.size());
    m_arg_1 = args[arg_index];

    if (ARCHON_LIKELY(!remaining_args_are_values && m_arg_1.size() >= 2 && m_arg_1[0] == dash)) {
        m_option_occurrence = {};
        m_option_occurrence.arg_index = arg_index;
        if (m_arg_1[1] == dash) {
            m_option_occurrence.lead_end = 2;
            offset = m_option_occurrence.lead_end;
            goto long_form_option;
        }
        m_option_occurrence.lead_end = 1;
        offset = m_option_occurrence.lead_end;
        goto short_form_option;
    }

    if (ARCHON_LIKELY(!has_pattern_match_error)) {
        if (ARCHON_LIKELY(!remaining_args_are_values)) {
            std::size_t keyword_index = m_spec.find_keyword(m_arg_1);
            if (ARCHON_UNLIKELY(keyword_index != std::size_t(-1))) {
                if (ARCHON_UNLIKELY(m_pattern_matcher.consume_keyword(keyword_index, arg_index)))
                    goto next_arg_1;
                // If the DFA has no transition on that keyword, do not interpret this
                // argument as a keyword.
            }
        }

        if (ARCHON_LIKELY(m_pattern_matcher.consume_value(arg_index)))
            goto next_arg_1;

        offset = m_arg_1.size();
        goto rejected_pattern_arg;
    }
    goto next_arg_1;

  maybe_short_form_continuation:
    if (ARCHON_LIKELY(offset == m_arg_1.size()))
        goto next_arg_1;

  short_form_option:
    m_option_occurrence.name_begin = offset;
    m_option_occurrence.name_end = offset + 1;
    offset = m_option_occurrence.name_end;
    proto_index = m_spec.find_proto_option(m_arg_1[m_option_occurrence.name_begin]);
    if (ARCHON_LIKELY(proto_index != std::size_t(-1))) {
        opt = unpack_option(proto_index, opt_in_pattern);
        if (!opt || !opt->arg.allow) {
            if (ARCHON_LIKELY(offset == m_arg_1.size() || m_arg_1[offset] != dash))
                goto option_without_arg;
            goto no_option_arg_allowed;
        }
        if (offset < m_arg_1.size()) {
            m_option_occurrence.has_value = true;
            m_option_occurrence.value_begin = offset;
            goto option_with_arg;
        }
        if (!opt->arg.require)
            goto option_without_arg;
        bool is_last_arg = (arg_index == args.size() - 1);
        if (ARCHON_LIKELY(!is_last_arg)) {
            ++arg_index;
            m_option_occurrence.has_value = true;
            goto option_with_arg;
        }
        goto missing_option_arg;
    }
    goto unknown_option;

  long_form_option:
    m_option_occurrence.name_begin = offset;
    m_option_occurrence.name_end = std::size_t(std::find(m_arg_1.begin() + offset, m_arg_1.end(), equal) -
                                               m_arg_1.begin());
    offset = m_option_occurrence.name_end;
    {
        std::size_t size = std::size_t(m_option_occurrence.name_end - m_option_occurrence.name_begin);
        string_view_type name = m_arg_1.substr(m_option_occurrence.name_begin, size);
        proto_index = m_spec.find_proto_option(name);
    }
    if (ARCHON_LIKELY(proto_index != std::size_t(-1))) {
        opt = unpack_option(proto_index, opt_in_pattern);
        if (offset < m_arg_1.size()) {
            ARCHON_ASSERT(m_arg_1[offset] == equal);
            ++offset;
            if (opt && opt->arg.allow) {
                m_option_occurrence.has_value = true;
                m_option_occurrence.value_begin = offset;
                goto option_with_arg;
            }
            goto no_option_arg_allowed;
        }
        if (!opt || !opt->arg.require)
            goto option_without_arg;
        bool is_last_arg = (arg_index == std::size_t(args.size() - 1));
        if (ARCHON_LIKELY(!is_last_arg)) {
            ++arg_index;
            m_option_occurrence.has_value = true;
            goto option_with_arg;
        }
        goto missing_option_arg;
    }
    goto unknown_option;

  option_without_arg:
    if (opt) {
        if (ARCHON_UNLIKELY((opt->attr & cli::short_circuit) != 0)) {
            // FIXME: Execute actions for options with lower level (--width) than this short-circuit option (--help)        
            opt->action.invoke_without_arg(); // Throws
            shorted = true;
            return true; // Short circuit
        }
        option_invocations.push_back({ opt->action, m_option_occurrence }); // Throws
        if (ARCHON_UNLIKELY((opt->attr & cli::further_args_are_values) != 0))
            remaining_args_are_values = true;
    }
    if (opt_in_pattern && !has_pattern_match_error) {
        if (ARCHON_UNLIKELY(!m_pattern_matcher.consume_option(proto_index, arg_index)))
            goto rejected_pattern_arg;
    }
    goto maybe_short_form_continuation;

  option_with_arg:
    ARCHON_ASSERT(opt);
    ARCHON_ASSERT(!opt_in_pattern);
    option_invocations.push_back({ opt->action, m_option_occurrence }); // Throws
    goto next_arg_1;

  invoke:
    ARCHON_ASSERT(!has_pattern_match_error);
    shorted = false;
    deleg = false;
    pattern = pat;
    return true; // Invoke pattern

  deleg:
    ARCHON_ASSERT(!has_pattern_match_error);
    shorted = false;
    deleg = true;
    pattern = pat;
    deleg_args_offset = arg_index;
    return true; // Delegate to sub-processor

  unknown_option:
    offset = m_arg_1.size();
    if (m_option_occurrence.name_begin == m_option_occurrence.lead_end) {
        if (offset == m_arg_1.size()) {
            error(arg_index, cli::ProcError::bad_option,
                  "Unknown command-line option %s", build_opt_form()); // Throws
            goto pattern_match_error;
        }
        error(arg_index, cli::ProcError::bad_option,
              "Unknown command-line option %s in %s", build_opt_form(),
              build_opt_context()); // Throws
        goto pattern_match_error;
    }
    error(arg_index, cli::ProcError::bad_option,
          "Unknown command-line option %s in multi-option compound %s",
          build_opt_form(), build_opt_context()); // Throws
    goto pattern_match_error;

  missing_option_arg:
    if (m_option_occurrence.name_begin == m_option_occurrence.lead_end) {
        if (offset == m_arg_1.size()) {
            error(arg_index, cli::ProcError::bad_option,
                  "Missing argument for command-line option %s", build_opt_form()); // Throws
            goto next_arg_1;
        }
    }
    error(arg_index, cli::ProcError::bad_option,
          "Missing argument for command-line option %s in %s", build_opt_form(),
          build_opt_context()); // Throws
    goto next_arg_1;

  no_option_arg_allowed:
    error(arg_index, cli::ProcError::bad_option,
          "No argument (%s) allowed for command-line option %s in %s",
          core::quoted(m_arg_1.substr(offset), max_val_size), build_opt_form(),
          build_opt_context()); // Throws
    goto next_arg_1;

  rejected_pattern_arg:
    ARCHON_ASSERT(!has_pattern_match_error);
    if (m_pattern_matcher.can_consume()) {
        error(arg_index, cli::ProcError::no_pattern_match,
              "Mismatching command-line argument %s",
              core::quoted(m_arg_1, max_val_size)); // Throws
    }
    else {
        error(arg_index, cli::ProcError::no_pattern_match,
              "Extraneous command-line argument %s",
              core::quoted(m_arg_1, max_val_size)); // Throws
    }

  pattern_match_error:
    // After a pattern match error (unknown option or rejected pattern argument), further
    // pattern matching would be unreliable and could generate misleading error messages, so
    // no further pattern matching must take place. Further more, if this happens at a time
    // where delegation is still possible, further option parsing would be unreliable and
    // could generate misleading error messages, as it is unknown whether a delgation was
    // supposed to have taken place and thereby bring other options into scope.
    if (ARCHON_LIKELY(!m_pattern_matcher.can_delegate())) {
        has_pattern_match_error = true;
        goto maybe_short_form_continuation;
    }

  error:
    ARCHON_ASSERT(m_has_error);
    return false; // Syntax error
}


template<class C, class T>
void Processor<C, T>::invoke_options(core::Span<const option_invocation_type> option_invocations)
{
    core::Span<const string_view_type> args = m_root_state.args;

    for (const option_invocation_type& invoc : option_invocations) {
        if (ARCHON_LIKELY(!invoc.occurrence.has_value)) {
            invoc.action.invoke_without_arg(); // Throws
            continue;
        }
        ARCHON_ASSERT(invoc.occurrence.arg_index < args.size());
        bool has_2nd_arg = (invoc.occurrence.value_begin == 0);
        string_view_type value;
        if (!has_2nd_arg) {
            m_arg_1 = args[invoc.occurrence.arg_index];
            value = m_arg_1.substr(invoc.occurrence.value_begin);
        }
        else {
            ARCHON_ASSERT(std::size_t(invoc.occurrence.arg_index + 1) < args.size());
            m_arg_2 = args[invoc.occurrence.arg_index + 1];
            value = m_arg_2;
        }
        if (ARCHON_LIKELY(invoc.action.invoke_with_arg(value, m_value_parser))) // Throws
            continue;
        m_option_occurrence = invoc.occurrence;
        if (has_2nd_arg) {
            ARCHON_ASSERT(invoc.occurrence.arg_index < args.size());
            m_arg_1 = args[invoc.occurrence.arg_index];
        }
        std::size_t max_val_size = m_root_state.show_arg_max_size;
        error(invoc.occurrence.arg_index, cli::ProcError::bad_option_arg,
              "Bad argument %s for command-line option %s in %s",
              core::quoted(value, max_val_size), build_opt_form(), build_opt_context()); // Throws
    }
}


template<class C, class T>
void Processor<C, T>::generate_descs(std::size_t elem_seq_index, std::vector<Desc>& descs)
{
    using MatchPos = typename pattern_args_parser_type::MatchPos;
    std::vector<MatchPos> match_entries;
    m_pattern_matcher.backtrack(match_entries); // Throws
    // Generate descs from match entries
    pattern_args_parser_type::generate_descs(m_pattern_structure, elem_seq_index, match_entries, descs); // Throws
}


template<class C, class T>
inline bool Processor<C, T>::has_error() const noexcept
{
    return m_has_error;
}


template<class C, class T>
auto Processor<C, T>::unpack_option(std::size_t proto_index, bool& opt_in_pattern) const -> const Option*
{
    const ProtoOption& proto = m_spec.get_proto_option(proto_index);
    opt_in_pattern = (proto.pattern_index != std::size_t(-1));
    if (ARCHON_LIKELY(proto.option_index != std::size_t(-1)))
        return &m_spec.get_option(proto.option_index);
    return nullptr;
}


template<class C, class T>
auto Processor<C, T>::build_opt_form() -> string_view_type
{
    using sv = string_view_type;
    std::size_t name_size = m_option_occurrence.name_end - m_option_occurrence.name_begin;
    sv opt_lead = m_arg_1.substr(0, m_option_occurrence.lead_end);
    sv opt_name = m_arg_1.substr(m_option_occurrence.name_begin, name_size);
    m_opt_form_buf.clear();
    m_opt_form_buf.append(opt_lead.begin(), opt_lead.end()); // Throws
    m_opt_form_buf.append(opt_name.begin(), opt_name.end()); // Throws
    sv str = m_formatter.format("%s", core::smart_quoted(sv(m_opt_form_buf))); // Throws
    m_opt_form_buf.clear();
    m_opt_form_buf.append(str); // Throws
    return sv(m_opt_form_buf);
}


template<class C, class T>
auto Processor<C, T>::build_opt_context() -> string_view_type
{
    using sv = string_view_type;
    sv str;
    std::size_t max_val_size = m_root_state.show_arg_max_size;
    bool has_2nd_arg = (m_option_occurrence.has_value && m_option_occurrence.value_begin == 0);
    if (!has_2nd_arg) {
        std::size_t max = std::max(m_option_occurrence.name_end, m_option_occurrence.value_begin);
        if (ARCHON_UNLIKELY(!core::try_int_add(max, max_val_size)))
            max = core::int_max<decltype(max)>();
        str = m_formatter.format("%s", core::smart_quoted(m_arg_1, max)); // Throws
    }
    else {
        str = m_formatter.format("`%s %s`", core::smart_quoted(m_arg_1),
                                 core::smart_quoted(m_arg_2, max_val_size)); // Throws
    }
    m_opt_context_buf.clear();
    m_opt_context_buf.append(str); // Throws
    return sv(m_opt_context_buf);
}


template<class C, class T>
template<class... P> inline void Processor<C, T>::error(std::size_t arg_index, cli::ProcError code,
                                                        const char* message, const P&... params)
{
    m_error_accum.add_error(arg_index, code, message, params...); // Throws
    m_has_error = true;
}


} // namespace archon::cli::impl

#endif // ARCHON_X_CLI_X_IMPL_X_PROCESSOR_HPP
