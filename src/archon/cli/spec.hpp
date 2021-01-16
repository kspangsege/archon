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

#ifndef ARCHON_X_CLI_X_SPEC_HPP
#define ARCHON_X_CLI_X_SPEC_HPP

/// \file


#include <memory>
#include <utility>
#include <stdexcept>
#include <initializer_list>
#include <string_view>
#include <vector>
#include <ostream>

#include <archon/base/features.h>
#include <archon/base/assert.hpp>
#include <archon/base/type_traits.hpp>
#include <archon/base/buffer.hpp>
#include <archon/base/char_mapper.hpp>
#include <archon/base/string_formatter.hpp>
#include <archon/base/quote.hpp>
#include <archon/cli/option_attributes.hpp>
#include <archon/cli/option_actions.hpp>
#include <archon/cli/command_line.hpp>
#include <archon/cli/detail/option_action.hpp>
#include <archon/cli/detail/spec.hpp>
#include <archon/cli/detail/pattern_position_nfa.hpp>
#include <archon/cli/detail/spec_parser.hpp>
#include <archon/cli/detail/value_parser.hpp>
#include <archon/cli/detail/pattern_matcher.hpp>


namespace archon::cli {


struct SpecConfig {
    /// FIXME: Explain            
    bool allow_cross_pattern_ambiguity = false;

    /// FIXME: Explain            
    bool allow_internal_pattern_ambiguity = false;            
};


/// \brief 
///
/// 
///
/// If `-f` or `--foo` takes an optional argument (opt_arg_maybe()), then an     
/// argument, `<val>`, **must** be specified as `-f<val>` or
/// `--foo=<val>`. If, instead, `-f` or `--foo` takes a mandatory argument
/// (opt_arg_always()), then the argument can also be specified as `-f <val>`         
/// or `--foo <val>`.
///
/// In `-f -b`, `-b` is interpreted as an argument to `-f`, if `-f` takes a
/// mandatory argument (opt_arg_always()), otherwise `-b` is interpreted as a    
/// new option. Similarly for `-f --bar`, `--foo -b`, and `--foo --bar`.
///
/// If `-f` takes an argument (opt_arg_always() or opt_arg_maybe()), then    
/// `-fbg` means pass argument `bg` to `-f`. Otherwise, it is equivalent to
/// `-f -bg`. FIXME: Mention special rule for `-f-`.        
///
/// If `-f` requires an argument, then an empty argument can be
/// specified as `-f ""`.
///
/// It is not possible to specify an empty argument for a short form option
/// taking an optional argument.
///
/// Single `-` followed by nothing, or by a space is interpreted as a
/// positional argument.
///
template<class C, class T = std::char_traits<C>> class BasicSpec {
public:
    using char_type        = C;
    using traits_type      = T;
    using string_view_type = std::basic_string_view<C, T>;
    using ostream_type     = std::basic_ostream<C, T>;

    BasicSpec(const std::locale& = std::locale());

    /// \{
    ///
    /// \brief Add command-line pattern.
    ///
    /// FIXME: By default, that is, if no patterns are added explicitely, \ref
    /// parse() acts as if one empty pattern was added with no associated
    /// action.
    ///
    /// The function passed as pattern action must have return type `void` or
    /// `int`. If it has return type `void`, then \ref parse() will return
    /// `EXIT_SUCCESS` as its exit status when that pattern gets
    /// executed. Otherwise, the value returned by the pattern action function
    /// will be returned as exit status by \ref parse().
    ///
    /// FIXME: Explain: Unparenthesized disjunctions are not allowed. This is
    /// because show_help() needs to be able to construct an unambiguous
    /// synopsis by taking the pattern string, exactly as it is specified, then
    /// prepend the program name (`argv[0]`) followed by a single space. This
    /// gives the application straightforward control over the exact appearance
    /// of the synopsis as it will be displayed by show_help().
    ///
    /// FIXME: Explain implicit option specifications and the the associated rules                  
    ///
    /// \sa \ref pat().      
    ///
    void add_pattern(const char* pattern, const char* descr);
    template<class A>
    void add_pattern(const char* pattern, const char* descr, A&& action);
    void add_pattern(string_view_type pattern, string_view_type descr);
    template<class A>
    void add_pattern(string_view_type pattern, string_view_type descr, A&& action);
    /// \}

    /// \{
    ///
    /// \brief Add command-line option.
    ///
    /// FIXME: Describe valid forms:                                          
    ///   Short: `-x` where `x` is a single character other than `-`.
    ///   Long:  `--xxx` where `xxx` is a sequence of zero or more characters.
    ///
    /// FIXME: Explain what forms are allowed in \a forms.
    ///
    /// FIXME: Explain what forms \a arg can take.
    ///
    /// FIXME: Explain variable substitution scheme for `descr`:
    /// `N` -> arg name (only valid with opt_arg_always() and opt_arg_maybe()).              
    /// `V` -> original value of associated variable (only valid with \ref raise_flag() and \ref assign()).
    /// `A` -> value assigned by default (default argument) (only valid with \ref raise_flag() and \ref assign()).
    ///
    /// FIXME: Explain what forms actions can take.
    ///
    /// FIXME: Explain support for `std::optional<>` in option argument value types. If a variable of type `std::optional<T>` is passed to \ref assign() or a function taking an argument of type `std::optional<T>` is passed as the option action, then any option argument specified on the command line will be passed as the type `T`. In those cases, if the option is specified without argument, the value assigned by \ref assign() or passed to the specified function will be `std::optional<T>()`.
    ///
    /// \sa \ref opt().      
    ///
    void add_option(const char* forms, const char* arg, int attr, const char* descr);
    template<class A>
    void add_option(const char* forms, const char* arg, int attr, const char* descr, A&& action);
    void add_option(string_view_type forms, string_view_type arg, int attr,
                    string_view_type descr);
    template<class A>
    void add_option(string_view_type forms, string_view_type arg, int attr,
                    string_view_type descr, A&& action);
    /// \}

    /// \brief           
    ///
    /// FIXME: Explain return values: Returns `EXIT_SUCCESS` if a short circuit
    /// option was encountered. Otherwise, returns `EXIT_FAILURE` if parsing
    /// fails (should be subject to configuration). Otherwise returns whatever
    /// is returned by the pattern action function (delegating or not).
    ///
    int parse(const CommandLine&) const;

    /// \brief           
    ///
    /// Sets \a exit_status to the value returned by process(const
    /// CommandLine&), then returns true if, and only if that exit status was
    /// `EXIT_SUCCESS`.
    ///
    bool parse(const CommandLine&, int& exit_status) const;

    /// \brief Print out help text.
    ///
    /// This function prints out text that describes the command-line interface.
    ///
    /// Patterns will be shown in the order that they are specified (using
    /// \ref add_pattern()).
    ///
    /// Options will be shown in the order that they are specified (using \ref
    /// add_option()).
    ///
    void show_help(const CommandLine&, ostream_type&, long width) const;

private:
    using OptionForm = typename detail::Spec<C, T>::OptionForm;
    using ArgSpec    = typename detail::Spec<C, T>::ArgSpec;

    using pattern_action_type    = detail::PatternAction<C, T>;
    using option_action_type     = detail::OptionAction<C, T>;
    using string_chunk_type      = base::Buffer<char_type>;
    using option_form_chunk_type = base::Buffer<OptionForm>;

    const std::locale m_locale;
    base::BasicStringWidener<C, T> m_widener;
    std::vector<string_chunk_type> m_string_chunks;
    std::vector<option_form_chunk_type> m_option_form_chunks;
    detail::SpecParser<C, T> m_spec_parser;
    detail::Spec<C, T> m_spec_rep;
    detail::PatternPositionNfa m_pattern_position_nfa;

    void do_add_pattern(const char* pattern, const char* descr,
                        std::unique_ptr<pattern_action_type>);
    void do_add_pattern(string_view_type pattern, string_view_type descr,
                        std::unique_ptr<pattern_action_type>);
    void do_add_pattern_2(string_view_type pattern, string_view_type descr,
                          std::unique_ptr<pattern_action_type>);
    void do_add_option(const char* forms, const char* arg, int attr, const char* descr,
                       std::unique_ptr<option_action_type>);
    void do_add_option(string_view_type forms, string_view_type arg, int attr,
                       string_view_type descr, std::unique_ptr<option_action_type>);
    void do_add_option_2(string_view_type forms, string_view_type arg, int attr,
                         string_view_type descr, std::unique_ptr<option_action_type>);

    void intern(std::initializer_list<string_view_type*>);
};


using Spec     = BasicSpec<char>;
using WideSpec = BasicSpec<wchar_t>;




/// \{
///
/// \brief Add command-line pattern.
///
/// These functions have the same effect as the corresponding `add_pattern()`
/// functions in `BasicSpec` (\ref BasicSpec::add_pattern()). The advantage of
/// these functions over those in `BasicSpec` is that they increase the amount
/// of space available per line of arguments when code is formatted in the style
/// shown here:
///
/// \code{.cpp}
///
///    archon::cli::Spec spec;
///    pat("copy <source path> <target path>", spec,
///        "Copy the file at the specified source path to specified target part.",
///        [&](...) {                                                                              
///            // ...
///        });
///
/// \endcode
///
template<class C, class T>
void pat(const char* pattern, BasicSpec<C, T>&, const char* descr);
template<class C, class T, class A>
void pat(const char* pattern, BasicSpec<C, T>&, const char* descr, A&& action);
template<class C, class T>
void pat(typename base::Wrap<std::basic_string_view<C, T>>::type pattern, BasicSpec<C, T>&,
         typename base::Wrap<std::basic_string_view<C, T>>::type descr);
template<class C, class T, class A>
void pat(typename base::Wrap<std::basic_string_view<C, T>>::type pattern, BasicSpec<C, T>&,
         typename base::Wrap<std::basic_string_view<C, T>>::type descr, A&& action);
/// \}


/// \{
///
/// \brief Add command-line option.
///
/// These functions have the same effect as the corresponding `add_otion()`
/// functions in `BasicSpec` (\ref BasicSpec::add_option()). The advantage of
/// these functions over those in `BasicSpec` is that they increase the amount
/// of space available per line of arguments when code is formatted in the style
/// shown here:
///
/// \code{.cpp}
///
///    archon::cli::Spec spec;
///    opt({"-w", "--width"}, "<num>", archon::cli::no_attributes, spec,
///        "Format text to a line length of @N (default is @V).",
///        archon::cli::assign(width));
///
/// \endcode
///
template<class C, class T>
void opt(const char* forms, const char* arg, int attr, BasicSpec<C, T>&, const char* descr);
template<class C, class T, class A>
void opt(const char* forms, const char* arg, int attr, BasicSpec<C, T>&, const char* descr,
         A&& action);
template<class C, class T>
void opt(typename base::Wrap<std::basic_string_view<C, T>>::type forms,
         typename base::Wrap<std::basic_string_view<C, T>>::type arg, int attr, BasicSpec<C, T>&,
         typename base::Wrap<std::basic_string_view<C, T>>::type descr);
template<class C, class T, class A>
void opt(typename base::Wrap<std::basic_string_view<C, T>>::type forms,
         typename base::Wrap<std::basic_string_view<C, T>>::type arg, int attr, BasicSpec<C, T>&,
         typename base::Wrap<std::basic_string_view<C, T>>::type descr, A&& action);
/// \}








// Implementation


template<class C, class T> inline BasicSpec<C, T>::BasicSpec(const std::locale& locale) :
    m_locale(locale),
    m_widener(locale), // Throws
    m_spec_parser(locale), // Throws
    m_spec_rep(locale)
{
}


template<class C, class T>
void BasicSpec<C, T>::add_pattern(const char* pattern, const char* descr)
{
    do_add_pattern(pattern, descr, nullptr); // Throws
}


template<class C, class T> template<class A>
void BasicSpec<C, T>::add_pattern(const char* pattern, const char* descr, A&& action)
{
    std::unique_ptr<pattern_action_type> action_2 =
        detail::make_pattern_action<C, T>(std::forward<A>(action)); // Throws
    do_add_pattern(pattern, descr, std::move(action_2)); // Throws
}


template<class C, class T>
void BasicSpec<C, T>::add_pattern(string_view_type pattern, string_view_type descr)
{
    do_add_pattern(pattern, descr, nullptr); // Throws
}


template<class C, class T> template<class A>
void BasicSpec<C, T>::add_pattern(string_view_type pattern, string_view_type descr, A&& action)
{
    std::unique_ptr<pattern_action_type> action_2 =
        detail::make_pattern_action<C, T>(std::forward<A>(action)); // Throws
    do_add_pattern(pattern, descr, std::move(action_2)); // Throws
}


template<class C, class T>
void BasicSpec<C, T>::add_option(const char* forms, const char* arg, int attr, const char* descr)
{
    do_add_option(forms, arg, attr, descr, nullptr); // Throws
}


template<class C, class T> template<class A>
void BasicSpec<C, T>::add_option(const char* forms, const char* arg, int attr, const char* descr,
                                 A&& action)
{
    std::unique_ptr<option_action_type> action_2 =
        detail::make_option_action<C, T>(std::forward<A>(action)); // Throws
    do_add_option(forms, arg, attr, descr, std::move(action_2)); // Throws
}


template<class C, class T>
void BasicSpec<C, T>::add_option(string_view_type forms, string_view_type arg, int attr,
                                 string_view_type descr)
{
    do_add_option(forms, arg, attr, descr, nullptr); // Throws
}


template<class C, class T> template<class A>
void BasicSpec<C, T>::add_option(string_view_type forms, string_view_type arg, int attr,
                                 string_view_type descr, A&& action)
{
    std::unique_ptr<option_action_type> action_2 =
        detail::make_option_action<C, T>(std::forward<A>(action)); // Throws
    do_add_option(forms, arg, attr, descr, std::move(action_2)); // Throws
}


template<class C, class T> int BasicSpec<C, T>::parse(const CommandLine& command_line) const
{
    using Pattern = typename detail::Spec<C, T>::Pattern;
    const Pattern* pattern = nullptr;
    const char* const* args = command_line.m_args_begin;
    std::size_t arg_index = 0;
    std::size_t num_args = std::size_t(command_line.m_args_end - command_line.m_args_begin);
    const CommandLine::Parent* parent = command_line.m_parent;
    CommandLine::PendingErrors errors;
    CommandLine::PendingErrors& errors_2 = (parent ? parent->errors : errors);
    {
        detail::ValueParser<C, T> value_parser(m_locale); // Throws
        detail::PatternMatcher pattern_matcher(m_spec_rep, m_pattern_position_nfa); // Throws

        const char* arg_1  = nullptr;
        const char* arg_2  = nullptr;
        std::string_view opt_lead, opt_name, opt_val;
        std::string opt_form_buf, opt_context_buf;
        base::StringFormatter formatter(m_locale);
        auto build_opt_form = [&] {
            opt_form_buf.clear();
            opt_form_buf.append(opt_lead.begin(), opt_lead.end()); // Throws
            opt_form_buf.append(opt_name.begin(), opt_name.end()); // Throws
            using sv = std::string_view;
            sv str = formatter.format("%s", base::smart_quoted(sv(opt_form_buf))); // Throws
            opt_form_buf.clear();
            opt_form_buf.append(str); // Throws
            return std::string_view(opt_form_buf);
        };
        auto build_opt_context = [&] {
            std::string_view str;
            if (!arg_2) {
                str = formatter.format("%s", base::smart_quoted(arg_1)); // Throws
            }
            else {
                str = formatter.format("`%s %s`", base::smart_quoted(arg_1),
                                       base::smart_quoted(arg_2)); // Throws
            }
            opt_context_buf.clear();
            opt_context_buf.append(str); // Throws
            return std::string_view(opt_context_buf);
        };

        bool has_pattern_match_error = false;
        auto error = [&](const char* string, const auto&... params) {
            format_2(errors_2.out, string, params...); // Throws
            errors_2.ends.push_back(errors_2.out.view().size()); // Throws
        };

        bool no_more_options = false;
        for (;;) {
            if (ARCHON_LIKELY(!pattern_matcher.is_match(pattern))) {
                if (ARCHON_UNLIKELY(arg_index == num_args)) {
                    if (ARCHON_LIKELY(!has_pattern_match_error))
                        error("Too few command-line arguments"); // Throws
                    goto error;
                }
            }
            else {
                if (ARCHON_UNLIKELY(pattern && pattern->action->is_deleg))
                    goto deleg;
                if (ARCHON_UNLIKELY(arg_index == num_args)) {
                    if (ARCHON_LIKELY(errors_2.ends.empty()))
                        goto enact;
                    goto error;
                }
            }

            bool opt_in_pattern;
            std::size_t opt_ndx;
            using Option = typename detail::Spec<C, T>::Option;
            const Option* opt = nullptr;
            arg_1 = args[arg_index];
            arg_2 = nullptr;
            const char* subarg = nullptr;
            if (no_more_options || arg_1[0] != '-')
                goto not_an_option;
            if (arg_1[1] != '-') {
                if (arg_1[1] != '\0') {
                    opt_lead = { arg_1, 1 };
                    subarg = opt_lead.end();
                    goto short_form_option;
                }
                goto not_an_option;
            }
            opt_lead = { arg_1, 2 };
            subarg = opt_lead.end();
            goto long_form_option;

          not_an_option:
            if (ARCHON_LIKELY(!has_pattern_match_error)) {
                std::size_t keyword_index = m_spec_rep.find_keyword(arg_1);
                if (ARCHON_UNLIKELY(keyword_index != std::size_t(-1))) {
                    if (ARCHON_UNLIKELY(pattern_matcher.consume_keyword(keyword_index)))
                        goto next_arg;
                    // If the DFA has no transition on that keyword, do not
                    // interpret this argument as a keyword.
                }
/*
                auto j = m_keyword_map.find(arg_1);
                if (ARCHON_UNLIKELY(j != m_keyword_map.end())) {
                    std::size_t keyword_index = j->second;
                    if (ARCHON_UNLIKELY(pattern_recog.consume_keyword(keyword_index)))
                        goto next_arg;
                    // If the DFA has no transition on that keyword, do not
                    // interpret this argument as a keyword.
                }
*/
                if (ARCHON_UNLIKELY(!pattern_matcher.consume_value(arg_1)))
                    goto rejected_pattern_arg;
            }
            goto next_arg;

          short_form_option:
            opt_name = { subarg, 1 };
            subarg = opt_name.end();
            {
                std::size_t proto_index = m_spec_rep.find_proto_option({ false, arg_1 });
                if (ARCHON_LIKELY(proto_index != std::size_t(-1))) {
                    unpack_option(proto_index, opt_in_pattern, opt_ndx, opt);
                    if (!opt->arg.allow) {
                        if (ARCHON_LIKELY(*subarg != '-'))
                            goto option_without_arg;
                        opt_val = { subarg };
                        goto no_option_arg_allowed;
                    }
                    if (*subarg != '\0')
                        goto option_with_arg;
                    if (!opt->arg.need)
                        goto option_without_arg;
                    bool last_arg = (arg_index == num_args - 1);
                    if (ARCHON_LIKELY(!last_arg)) {
                        arg_2 = args[++arg_index];
                        subarg = arg_2;
                        goto option_with_arg;
                    }
                    goto missing_option_arg;
                }
/*
                auto j = m_short_forms.find(opt_name[0]);
                if (ARCHON_LIKELY(j != m_short_forms.end())) {
                    opt_ndx = j->second;
                    opt = &m_options[opt_ndx];
                }
*/
            }
            goto unknown_option;

          long_form_option:
            opt_name = { subarg, std::strcspn(subarg, "=") };
            subarg = opt_name.end();
            {
                std::size_t proto_index = m_spec_rep.find_proto_option({ true, arg_1 });     
                if (ARCHON_LIKELY(proto_index != std::size_t(-1))) {
                    unpack_option(proto_index, opt_in_pattern, opt_ndx, opt);
                auto j = m_long_forms.find(opt_name);
                if (ARCHON_LIKELY(j != m_long_forms.end())) {
                    opt_ndx = j->second;
                    opt = &m_options[opt_ndx];
                    if (*subarg == '\0') {
                        if (!opt->arg.need)
                            goto option_without_arg;
                        bool last_arg = (arg_index == num_args - 1);
                        if (ARCHON_LIKELY(!last_arg)) {
                            arg_2 = args[++arg_index];
                            subarg = arg_2;
                            goto option_with_arg;
                        }
                        goto missing_option_arg;
                    }
                    ARCHON_ASSERT(*subarg == '=');
                    ++subarg;
                    if (ARCHON_LIKELY(opt->arg.allow))
                        goto option_with_arg;
                    opt_val = { subarg };
                    goto no_option_arg_allowed;
                }
            }
            goto unknown_option;

          maybe_short_form_continuation:
            if (!subarg || *subarg == '\0')
                goto next_arg;
            goto short_form_option;

          option_without_arg:
            if (opt->action)
                opt->action->enact_without_arg(); // Throws
            if (ARCHON_UNLIKELY((opt->attr & short_circuit) != 0))
                return EXIT_SUCCESS;
            if (ARCHON_UNLIKELY((opt->attr & end_of_options) != 0))
                no_more_options = true;
            if (opt->in_pattern && !has_pattern_match_error) {
                if (ARCHON_UNLIKELY(!pattern_matcher.consume_option(opt_ndx)))
                    goto rejected_pattern_arg;
            }
            goto maybe_short_form_continuation;

          option_with_arg:
            ARCHON_ASSERT(opt->action);
            ARCHON_ASSERT(!opt->in_pattern);
            opt_val = { subarg };
            if (ARCHON_LIKELY(opt->action->enact_with_arg(opt_val, value_parser))) // Throws
                goto next_arg;
            goto bad_option_arg;

          unknown_option:
            if (opt_name.begin() == opt_lead.end()) {
                if (*subarg == '\0') {
                    error("Unknown command-line option %s", build_opt_form()); // Throws
                    goto next_arg;
                }
                error("Unknown command-line option %s in %s", build_opt_form(),
                      build_opt_context()); // Throws
                goto next_arg;
            }
            error("Unknown command-line option %s in multi-option compound %s",
                  build_opt_form(), build_opt_context()); // Throws
            goto next_arg;

          missing_option_arg:
            if (opt_name.begin() == opt_lead.end() && *subarg == '\0') {
                error("Missing argument for command-line option %s", build_opt_form()); // Throws
                goto next_arg;
            }
            error("Missing argument for command-line option %s in %s", build_opt_form(),
                  build_opt_context()); // Throws
            goto next_arg;

          bad_option_arg:
            error("Bad argument %s for command-line option %s in %s", quoted(opt_val),
                  build_opt_form(), build_opt_context()); // Throws
            goto next_arg;

          no_option_arg_allowed:
            error("No argument (not even %s) allowed for command-line option %s in %s",
                  quoted(opt_val), build_opt_form(), build_opt_context()); // Throws
            goto next_arg;

          rejected_pattern_arg:
            if (pattern_matcher.can_consume_anything_more()) {
                error("Unexpected command-line argument"); // Throws
            }
            else {
                error("Too many command-line arguments"); // Throws
            }
            has_pattern_match_error = true;
            goto maybe_short_form_continuation;

          next_arg:
            ++arg_index;
        }
    }

  deleg:
    {
        ARCHON_ASSERT(pattern);
        std::cerr << "DELEG: " << pattern->spec << "\n";
        const char* const* args_begin = args + arg_index;
        const char* const* args_end   = args + num_args;
        CommandLine::Parent parent_2{errors_2, command_line, pattern->spec};
        CommandLine command_line{args_begin, args_end, parent_2};
        return pattern->action->deleg(command_line); // Throws
    }

  enact:
    {
        if (ARCHON_LIKELY(pattern)) {
            std::cerr << "ENACT: " << pattern->spec << "\n";
            // FIXME: Pass value arguments from command line as function arguments                                                                                                     
            return pattern->action->enact(); // Throws
        }
    }
    return EXIT_SUCCESS;

  error:
    {
        // Display pending error messages
        const char* data = errors_2.message_buffer.data();
        std::size_t begin = 0;
        for (std::size_t end : errors_2.ends) {
            std::string_view message(data + begin, end - begin);
            std::cerr << "ERROR: " << message << "\n"; // FIXME: Report the errors through an abstract sink which can be configured through CommandLine::Config                   
            begin = end;
        }
    }
*/
    return EXIT_FAILURE; // FIXME: Allow for alternative return values (ProcessConfig)                  
}


template<class C, class T>
inline bool BasicSpec<C, T>::parse(const CommandLine& command_line, int& exit_status) const
{
    exit_status = parse(command_line); // Throws
    return (exit_status == EXIT_SUCCESS);
}


template<class C, class T>
void BasicSpec<C, T>::show_help(const CommandLine&, ostream_type& out, long width) const
{
    static_cast<void>(width);             
    out << "*CLICK*\n";   
}


template<class C, class T>
inline void BasicSpec<C, T>::do_add_pattern(const char* pattern, const char* descr,
                                            std::unique_ptr<pattern_action_type> action)
{
    string_view_type pattern_2, descr_2;
    typename base::BasicStringWidener<C, T>::Entry entries[] = {
        { pattern, &pattern_2 },
        { descr,   &descr_2   }
    };
    m_widener.widen(entries); // Throws
    do_add_pattern(pattern_2, descr_2, std::move(action)); // Throws
}


template<class C, class T>
void BasicSpec<C, T>::do_add_pattern(string_view_type pattern, string_view_type descr,
                                     std::unique_ptr<pattern_action_type> action)
{
    string_view_type pattern_2 = pattern;
    string_view_type descr_2   = descr;
    intern({ &pattern_2, &descr_2 }); // Throws
    do_add_pattern_2(pattern_2, descr_2, std::move(action)); // Throws
}


template<class C, class T>
inline void BasicSpec<C, T>::do_add_pattern_2(string_view_type pattern, string_view_type descr,
                                              std::unique_ptr<pattern_action_type> action)
{
    std::size_t ndx = m_spec_rep.get_num_patterns();
    m_spec_parser.parse_pattern(pattern, m_spec_rep, m_pattern_position_nfa, ndx,
                                action && action->is_deleg); // Throws
    m_spec_rep.add_pattern(pattern, descr, std::move(action)); // Throws
}


template<class C, class T>
inline void BasicSpec<C, T>::do_add_option(const char* forms, const char* arg, int attr,
                                           const char* descr,
                                           std::unique_ptr<option_action_type> action)
{
    string_view_type forms_2, arg_2, descr_2;
    typename base::BasicStringWidener<C, T>::Entry entries[] = {
        { forms, &forms_2 },
        { arg,   &arg_2   },
        { descr, &descr_2 }
    };
    m_widener.widen(entries); // Throws
    do_add_option(forms_2, arg_2, attr, descr_2, std::move(action)); // Throws
}


template<class C, class T>
void BasicSpec<C, T>::do_add_option(string_view_type forms, string_view_type arg, int attr,
                                    string_view_type descr,
                                    std::unique_ptr<option_action_type> action)
{
    string_view_type forms_2 = forms;
    string_view_type arg_2   = arg;
    string_view_type descr_2 = descr;
    intern({ &forms_2, &arg_2, &descr_2 }); // Throws
    do_add_option_2(forms_2, arg_2, attr, descr_2, std::move(action)); // Throws
}


template<class C, class T>
void BasicSpec<C, T>::do_add_option_2(string_view_type forms, string_view_type arg, int attr,
                                      string_view_type descr,
                                      std::unique_ptr<option_action_type> action)
{
    base::ArraySeededBuffer<OptionForm, 4> buffer;
    std::size_t ndx = m_spec_rep.get_num_options();
    base::Span<OptionForm> forms_2 =
        m_spec_parser.parse_option_forms(ndx, forms, buffer); // Throws
    base::Buffer<OptionForm> chunk(forms_2); // Throws
    m_option_form_chunks.push_back(std::move(chunk)); // Throws
    ArgSpec arg_2 = m_spec_parser.parse_option_arg(ndx, arg); // Throws
    m_spec_rep.add_option(forms_2, arg_2, attr, descr, std::move(action)); // Throws
}


template<class C, class T>
void BasicSpec<C, T>::intern(std::initializer_list<string_view_type*> strings)
{
    std::size_t total_size = 0;
    for (const string_view_type* string : strings)
        base::int_add(total_size, string->size()); // Throws
    base::Buffer<char_type> chunk(total_size); // Throws
    char_type* data = chunk.data();
    for (string_view_type* string : strings) {
        std::size_t size = string->size();
        char_type* data_2 = std::copy_n(string->data(), size, data);
        *string = { data, size };
        data = data_2;
    }
    m_string_chunks.push_back(std::move(chunk)); // Throws
}


template<class C, class T> void pat(const char* pattern, BasicSpec<C, T>& spec, const char* descr)
{
    spec.add_pattern(pattern, descr); // Throws
}


template<class C, class T, class A>
void pat(const char* pattern, BasicSpec<C, T>& spec, const char* descr, A&& action)
{
    spec.add_pattern(pattern, descr, std::forward<A>(action)); // Throws
}


template<class C, class T>
void pat(typename base::Wrap<std::basic_string_view<C, T>>::type pattern, BasicSpec<C, T>& spec,
         typename base::Wrap<std::basic_string_view<C, T>>::type descr)
{
    spec.add_pattern(pattern, descr); // Throws
}


template<class C, class T, class A>
void pat(typename base::Wrap<std::basic_string_view<C, T>>::type pattern, BasicSpec<C, T>& spec,
         typename base::Wrap<std::basic_string_view<C, T>>::type descr, A&& action)
{
    spec.add_pattern(pattern, descr, std::forward<A>(action)); // Throws
}


template<class C, class T>
void opt(const char* forms, const char* arg, int attr, BasicSpec<C, T>& spec, const char* descr)
{
    spec.add_option(forms, arg, attr, descr); // Throws
}


template<class C, class T, class A>
void opt(const char* forms, const char* arg, int attr, BasicSpec<C, T>& spec, const char* descr,
         A&& action)
{
    spec.add_option(forms, arg, attr, descr, std::forward<A>(action)); // Throws
}


template<class C, class T>
void opt(typename base::Wrap<std::basic_string_view<C, T>>::type forms,
         typename base::Wrap<std::basic_string_view<C, T>>::type arg, int attr,
         BasicSpec<C, T>& spec, typename base::Wrap<std::basic_string_view<C, T>>::type descr)
{
    spec.add_option(forms, arg, attr, descr); // Throws
}


template<class C, class T, class A>
void opt(typename base::Wrap<std::basic_string_view<C, T>>::type forms,
         typename base::Wrap<std::basic_string_view<C, T>>::type arg, int attr,
         BasicSpec<C, T>& spec, typename base::Wrap<std::basic_string_view<C, T>>::type descr,
         A&& action)
{
    spec.add_option(forms, arg, attr, descr, std::forward<A>(action)); // Throws
}


} // namespace archon::cli

#endif // ARCHON_X_CLI_X_SPEC_HPP
