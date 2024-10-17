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

#ifndef ARCHON_X_CLI_X_PROCESSOR_HPP
#define ARCHON_X_CLI_X_PROCESSOR_HPP

/// \file


#include <cstddef>
#include <utility>
#include <memory>
#include <initializer_list>
#include <string_view>
#include <vector>
#include <locale>
#include <ostream>

#include <archon/core/type_traits.hpp>
#include <archon/core/span.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/seed_memory_output_stream.hpp>
#include <archon/core/char_mapper.hpp>
#include <archon/core/string_formatter.hpp>
#include <archon/core/quote.hpp>
#include <archon/core/format.hpp>
#include <archon/core/text_codec.hpp>
#include <archon/core/text_file_stream.hpp>
#include <archon/cli/option_attributes.hpp>
#include <archon/cli/command_line.hpp>
#include <archon/cli/impl/value_parser.hpp>
#include <archon/cli/impl/pattern_args_parser.hpp>
#include <archon/cli/impl/pattern_action.hpp>
#include <archon/cli/impl/option_action.hpp>
#include <archon/cli/impl/pattern_structure.hpp>
#include <archon/cli/impl/spec.hpp>
#include <archon/cli/impl/spec_parser.hpp>
#include <archon/cli/impl/processor.hpp>
#include <archon/cli/impl/help_formatter.hpp>


namespace archon::cli {


struct ProcessorConfig {
    /// FIXME: Explain            
    bool allow_cross_pattern_ambiguity = false;

    /// FIXME: Explain            
    bool allow_internal_pattern_ambiguity = false;            
};


template<class C, class T = std::char_traits<C>> class BasicProcessor {
public:
    using char_type   = C;
    using traits_type = T;

    using string_view_type  = std::basic_string_view<C, T>;
    using ostream_type      = std::basic_ostream<C, T>;
    using command_line_type = cli::BasicCommandLine<C, T>;

    using Config = cli::BasicConfig<C, T>;

    /// \{
    ///
    /// \brief 
    ///
    /// 
    ///
    BasicProcessor(int argc, const char* const argv[], Config = {});
    BasicProcessor(int argc, const char* const argv[], const std::locale&, Config = {});
    /// \}

    BasicProcessor(const command_line_type&, ProcessorConfig = {});

    /// \{
    ///
    /// \brief Add command-line pattern.
    ///
    /// FIXME: By default, that is, if no patterns are added explicitly, \ref process() acts
    /// as if one empty pattern was added with no associated action.
    ///
    /// The function passed as pattern action must have return type `void` or `int`. If it
    /// has return type `void`, then \ref process() will return `EXIT_SUCCESS` as its exit
    /// status when that pattern gets executed. Otherwise, the value returned by the pattern
    /// action function will be returned as exit status by \ref process().
    ///
    /// If no action function is passed, the effect is ad if the passed function was `[]()
    /// {}`.
    ///
    /// FIXME: Explain: Unparenthesized disjunctions are not allowed. This is because
    /// show_help() needs to be able to construct an unambiguous synopsis by taking the
    /// pattern string, exactly as it is specified, then prepend the program name
    /// (`argv[0]`) followed by a single space. This gives the application straightforward
    /// control over the exact appearance of the synopsis as it will be displayed by
    /// show_help().
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
    /// FIXME: Explain what forms \a arg can take. Empty string means, no argument accepted; `<foo>` means mandatory argument named `foo`; `[<foo>]` means optional argument named `foo`. Space allowed around `[` and `]`?   
    ///
    /// FIXME: Explain variable substitution scheme for `descr` when formatted by show_help():
    /// `A` -> arg lexeme (`<foo>`) (only valid with opt_arg_always() and opt_arg_maybe()).              
    /// `V` -> original value of associated variable (only valid with \ref raise_flag() and \ref assign()).
    /// `W` -> value assigned/passed by default (default argument) (only valid with \ref raise_flag(), \ref assign(), and \ref exec()). Hmm, only some versions of exec()!?!?!?    
    /// `Q` -> same as `V`, but quoted as if by \ref core::quoted() with `max_size` set to 32.
    /// `R` -> same as `W`, but quoted as if by \ref core::quoted() with `max_size` set to 32.
    ///
    /// FIXME: Explain what forms actions can take.
    ///
    /// FIXME: Explain support for `std::optional<>` in option argument value types. If a variable of type `std::optional<T>` is passed to \ref assign() or a function taking an argument of type `std::optional<T>` is passed as the option action, then any option argument specified on the command line will be parsed as the type `T`. In those cases, if the option is specified without argument, the value assigned by \ref assign() or passed to the specified function will be `std::optional<T>()`.
    ///
    /// FIXME: Currently no support for `std::optional<>` in help formatter. What should `@V` expand to, if there is no value?
    ///
    /// FIXME: Explain: If `char_type` is not `char`, it is still possible use a variable of
    /// type `std::string` or `std::string_view` as assignment target (cli::assign()), or a
    /// function with a parameter of type `std::string` or `std::string_view` as execution
    /// target (cli::exec()). In such cases, the produced sting values will be encoded as if
    /// by \ref base::StringCodec and according to the selected locale.
    ///
    /// FIXME: Explain: If the assignment target is a variable of string view type
    /// (`std::string_view` or `string_view_type`), then the memory referenced by the
    /// produced string view is owned by a string holder (\ref cli::BasicStringHolder). If
    /// no string holder is specified by the application (\ref cli::Config::string_holder),
    /// then the effectiive string holder is owned by the processor, and its lifetime ends
    /// when the processor dies. The application can extend the lifetime of the memory
    /// referenced by string views by creating its own string holder, and specifying it
    /// through \ref cli::Config::string_holder.
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
    ///  
    ///
    void add_help_option(long format_width = 80);

    /// \brief Process command line.
    ///
    /// This function processes the command line that was associated with this processor at
    /// construction time (\ref BasicProcessor()).
    ///
    /// It returns `false` when the primary function of the invoked program is supposed to
    /// be carried out, and `true` (meaning, early out) when it should be skipped, either
    /// because there was an error while processing the command line, or because a
    /// short-circuit option has been acted upon already (think `--help`). A value is
    /// assigned to \p exit_status when, and only when this function returns `true`.
    ///
    /// This function is useful when there are no explicitly specified patterns (\ref
    /// add_pattern()). In this case, the caller needs to know whether the primary function
    /// of the invoked program should be carried out.
    ///
    /// If at least one pattern is explicitly specified, this function will always return
    /// `true`, and it will therefore make more sense to call \ref process() in this case.
    ///
    /// Example of intended use:
    ///
    /// \code{.cpp}
    ///
    ///    int main(int argc, char* argv[])
    ///    {
    ///        archon::cli::Processor proc(argc, argv);
    ///        // Specify options, but no patterns
    ///        // ...
    ///
    ///        int exit_status = 0;
    ///        if (proc.process(exit_status))
    ///          return exit_status;
    ///
    ///        // Carry out the primary function of the invoked program
    ///        // ...
    ///    }
    ///
    /// \endcode
    ///
    /// \param exit_status Left untouched if `process()` returns `false`. Otherwise, set to
    /// `EXIT_FAILURE` if an error occurs during processing of the command line, to
    /// `EXIT_SUCCESS` if a short-circuit option was acted upon, and to whatever was
    /// returned by the function if a pattern function was invoked.
    ///
    /// \return `true` if any action, that was supposed to be taken, has already been taken,
    /// either because a pattern was acted upon, a short-circuit option was acted upon, or
    /// an error occurred. `false` otherwise. For a processor with no explicitly specified
    /// patterns, this function sets \p exit_status and returns `true` if an error occurs
    /// during processing of the command line, or if a short-circuit option has been acted
    /// upon. Otherwise it returns `false`. For a processor with at least one explicitly
    /// specified patterm, this function always returns `true`.
    ///
    /// \sa \ref process()
    ///
    /// FIXME: Explain that this version of process() will fail if variables of string view
    /// type have been registered as targets of assignment actions or if functions
    /// registered as option or pattern actions have parameters of string view type. This is
    /// because a string holder is needed in order to have process() propduce values of
    /// string view type.            
    ///
    bool process(int& exit_status) const;

    /// \brief Process command line.
    ///
    /// This function processes the command line that was associated with this processor at
    /// construction time (\ref BasicProcessor()).
    ///
    /// FIXME: Explain return values: Returns `EXIT_SUCCESS` if a short circuit option was
    /// encountered. Otherwise, returns `EXIT_FAILURE` if parsing fails (should be subject
    /// to configuration). Otherwise returns whatever is returned by the pattern action
    /// function (delegating or not).
    ///
    /// FIXME: Explain that with this version of process(), produced values of string view
    /// type must not be accessed after return from process() because the internally created
    /// string holder will have been destroyed.             
    ///
    int process() const;

    /// \brief Print out help text.
    ///
    /// This function prints out text that describes the command-line interface.
    ///
    /// Patterns will be shown in the order that they are specified (using \ref
    /// add_pattern()).
    ///
    /// Options will be shown in the order that they are specified (using \ref
    /// add_option()).
    ///
    /// The specified stream must use a locale that is compatible with the locale of the
    /// command-line processor. The important thing is that the character encodings agree
    /// (`std::codecvt` facet).
    ///
    /// This function does not flush the stream before returning.
    ///
    void show_help(ostream_type&, long width = 80) const;

private:
    using logger_type         = log::BasicLogger<C, T>;
    using pattern_action_type = impl::PatternAction<C, T>;
    using option_action_type  = impl::OptionAction<C, T>;
    using spec_type           = impl::Spec<C, T>;

    using OptionForm  = typename spec_type::OptionForm;
    using ArgSpec     = typename spec_type::ArgSpec;
    using Pattern     = typename spec_type::Pattern;

    using string_chunk_type      = core::Buffer<char_type>;
    using option_form_chunk_type = core::Buffer<OptionForm>;

    std::unique_ptr<command_line_type> m_command_line_owner;
    const command_line_type& m_command_line;
    const ProcessorConfig m_config;
    const std::locale m_locale;
    logger_type& m_logger;
    core::BasicStringWidener<C, T> m_widener;
    std::vector<string_chunk_type> m_string_chunks;
    std::vector<option_form_chunk_type> m_option_form_chunks;
    impl::SpecParser<C, T> m_spec_parser;
    impl::PatternStructure m_pattern_structure;
    spec_type m_spec;

    BasicProcessor(std::unique_ptr<command_line_type> command_line_owner,
                   const command_line_type* command_line, ProcessorConfig);

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


using Processor     = BasicProcessor<char>;
using WideProcessor = BasicProcessor<wchar_t>;




/// \{
///
/// \brief Add command-line pattern.
///
/// These functions have the same effect as the corresponding `add_pattern()` functions in
/// `BasicProcessor` (\ref BasicProcessor::add_pattern()). The advantage of these functions
/// over those in `BasicProcessor`, is that they increase the amount of space available per
/// line of arguments when code is formatted in the style shown here:
///
/// \code{.cpp}
///
///    archon::cli::Processor proc(...);
///    pat("copy <source path> <target path>", proc,
///        "Copy the file at the specified source path to specified target part.",
///        [&](...) {                                                                              
///            // ...
///        });
///
/// \endcode
///
template<class C, class T>
void pat(const char* pattern, BasicProcessor<C, T>&, const char* descr);
template<class C, class T, class A>
void pat(const char* pattern, BasicProcessor<C, T>&, const char* descr, A&& action);
template<class C, class T>
void pat(typename core::Wrap<std::basic_string_view<C, T>>::type pattern, BasicProcessor<C, T>&,
         typename core::Wrap<std::basic_string_view<C, T>>::type descr);
template<class C, class T, class A>
void pat(typename core::Wrap<std::basic_string_view<C, T>>::type pattern, BasicProcessor<C, T>&,
         typename core::Wrap<std::basic_string_view<C, T>>::type descr, A&& action);
/// \}



/// \{
///
/// \brief Add command-line option.
///
/// These functions have the same effect as the corresponding `add_otion()` functions in
/// `BasicProcessor` (\ref BasicProcessor::add_option()). The advantage of these functions
/// over those in `BasicProcessor`, is that they increase the amount of space available per
/// line of arguments when code is formatted in the style shown here:
///
/// \code{.cpp}
///
///    archon::cli::Processor proc(...);
///    opt({"-w", "--width"}, "<num>", archon::cli::no_attributes, proc,
///        "Format text to a line length of @N (default is @V).",
///        archon::cli::assign(width));
///
/// \endcode
///
template<class C, class T>
void opt(const char* forms, const char* arg, int attr, BasicProcessor<C, T>&, const char* descr);
template<class C, class T, class A>
void opt(const char* forms, const char* arg, int attr, BasicProcessor<C, T>&, const char* descr,
         A&& action);
template<class C, class T>
void opt(typename core::Wrap<std::basic_string_view<C, T>>::type forms,
         typename core::Wrap<std::basic_string_view<C, T>>::type arg, int attr,
         BasicProcessor<C, T>&, typename core::Wrap<std::basic_string_view<C, T>>::type descr);
template<class C, class T, class A>
void opt(typename core::Wrap<std::basic_string_view<C, T>>::type forms,
         typename core::Wrap<std::basic_string_view<C, T>>::type arg, int attr,
         BasicProcessor<C, T>&, typename core::Wrap<std::basic_string_view<C, T>>::type descr,
         A&& action);
/// \}








// Implementation


template<class C, class T>
inline BasicProcessor<C, T>::BasicProcessor(int argc, const char* const argv[], Config config)
    : BasicProcessor(std::make_unique<command_line_type>(argc, argv, std::move(config)),
                     nullptr, {}) // Throws
{
}


template<class C, class T>
inline BasicProcessor<C, T>::BasicProcessor(int argc, const char* const argv[],
                                            const std::locale& locale, Config config)
    : BasicProcessor(std::make_unique<command_line_type>(argc, argv, locale, std::move(config)),
                     nullptr, {}) // Throws
{
}


template<class C, class T>
inline BasicProcessor<C, T>::BasicProcessor(const command_line_type& command_line,
                                            ProcessorConfig config)
    : BasicProcessor(nullptr, &command_line, std::move(config)) // Throws
{
}


template<class C, class T>
void BasicProcessor<C, T>::add_pattern(const char* pattern, const char* descr)
{
    do_add_pattern(pattern, descr, nullptr); // Throws
}


template<class C, class T>
template<class A>
void BasicProcessor<C, T>::add_pattern(const char* pattern, const char* descr, A&& action)
{
    std::unique_ptr<pattern_action_type> action_2 =
        impl::make_pattern_action<C, T>(std::forward<A>(action)); // Throws
    do_add_pattern(pattern, descr, std::move(action_2)); // Throws
}


template<class C, class T>
void BasicProcessor<C, T>::add_pattern(string_view_type pattern, string_view_type descr)
{
    do_add_pattern(pattern, descr, nullptr); // Throws
}


template<class C, class T>
template<class A>
void BasicProcessor<C, T>::add_pattern(string_view_type pattern, string_view_type descr,
                                       A&& action)
{
    std::unique_ptr<pattern_action_type> action_2 =
        impl::make_pattern_action<C, T>(std::forward<A>(action)); // Throws
    do_add_pattern(pattern, descr, std::move(action_2)); // Throws
}


template<class C, class T>
void BasicProcessor<C, T>::add_option(const char* forms, const char* arg, int attr,
                                      const char* descr)
{
    do_add_option(forms, arg, attr, descr, nullptr); // Throws
}


template<class C, class T>
template<class A>
void BasicProcessor<C, T>::add_option(const char* forms, const char* arg, int attr,
                                      const char* descr, A&& action)
{
    std::unique_ptr<option_action_type> action_2 =
        impl::make_option_action<C, T>(std::forward<A>(action)); // Throws
    do_add_option(forms, arg, attr, descr, std::move(action_2)); // Throws
}


template<class C, class T>
void BasicProcessor<C, T>::add_option(string_view_type forms, string_view_type arg, int attr,
                                      string_view_type descr)
{
    do_add_option(forms, arg, attr, descr, nullptr); // Throws
}


template<class C, class T>
template<class A>
void BasicProcessor<C, T>::add_option(string_view_type forms, string_view_type arg, int attr,
                                      string_view_type descr, A&& action)
{
    std::unique_ptr<option_action_type> action_2 =
        impl::make_option_action<C, T>(std::forward<A>(action)); // Throws
    do_add_option(forms, arg, attr, descr, std::move(action_2)); // Throws
}


template<class C, class T>
inline void BasicProcessor<C, T>::add_help_option(long format_width)
{
    auto func = [this, format_width] {
        core::BasicTextFileStream<C, T> out(&core::File::get_stdout()); // Throws
        out.exceptions(std::ios_base::badbit | std::ios_base::failbit); // Throws
        out.imbue(m_locale); // Throws
        show_help(out, format_width); // Throws
        out.flush(); // Throws
    };
    add_option("-h, --help", "", cli::short_circuit,
               "Show command synopsis and the list of available options.",
               std::move(func)); // Throws
}


template<class C, class T>
inline bool BasicProcessor<C, T>::process(int& exit_status) const
{
    using RootState = typename command_line_type::RootState;
    const RootState& root_state = m_command_line.m_root_state;
    impl::ValueParser<C, T> value_parser(root_state.string_holder, m_locale); // Throws
    bool deleg = false;
    const Pattern* pattern = nullptr;
    std::vector<std::size_t> positions;
    impl::DelegCapsule<C, T> deleg_capsule;
    {
        impl::PatternMatcherConfig pattern_matcher_config;
        pattern_matcher_config.allow_cross_pattern_ambiguity =
            m_config.allow_cross_pattern_ambiguity;
        pattern_matcher_config.allow_internal_pattern_ambiguity =
            m_config.allow_internal_pattern_ambiguity;
        impl::Processor<C, T> processor(m_command_line, m_spec, value_parser, m_pattern_structure,
                                        std::move(pattern_matcher_config)); // Throws
        bool shorted = false;
        impl::ProcessError process_error = {};
        bool success = processor.process(shorted, deleg, pattern, positions,
                                         deleg_capsule, process_error); // Throws
        if (ARCHON_UNLIKELY(!success)) {
            switch (process_error) {
                case impl::ProcessError::bad_option:
                    exit_status = root_state.bad_option_exit_status;
                    return true;
                case impl::ProcessError::bad_option_arg:
                    exit_status = root_state.parse_error_exit_status;
                    return true;
                case impl::ProcessError::no_pattern_match:
                    exit_status = root_state.match_error_exit_status;
                    return true;
            }
            ARCHON_ASSERT_UNREACHABLE();
        }
        if (shorted) {
            exit_status = EXIT_SUCCESS;
            return true;
        }
    }
    if (ARCHON_LIKELY(!deleg)) {
        if (pattern) {
            if (ARCHON_LIKELY(pattern->action)) {
                core::Span<const string_view_type> args = root_state.args;
                ARCHON_ASSERT(m_command_line.m_args_offset <= args.size());
                auto args_2 = args.subspan(m_command_line.m_args_offset);
                using pattern_args_parser_type = impl::PatternArgsParser<C, T>;
                pattern_args_parser_type pattern_args_parser(m_pattern_structure,
                                                             pattern->elem_seq_index, args_2,
                                                             positions); // Throws
                bool success = pattern->action->invoke(pattern_args_parser, value_parser,
                                                       exit_status); // Throws
                if (ARCHON_LIKELY(success))
                    return true;
                exit_status = root_state.parse_error_exit_status;
                return true;
            }
            exit_status = EXIT_SUCCESS;
            return true;
        }
        return false;
    }

    ARCHON_ASSERT(pattern);
    ARCHON_ASSERT(pattern->action);
    typename command_line_type::Parent parent {
        deleg_capsule,
        m_command_line,
        pattern->spec
    };
    command_line_type command_line(parent);
    exit_status = pattern->action->deleg(command_line); // Throws
    return true;
}


template<class C, class T>
inline int BasicProcessor<C, T>::process() const
{
    int exit_status = EXIT_SUCCESS;
    process(exit_status); // Throws
    return exit_status;
}


template<class C, class T>
void BasicProcessor<C, T>::show_help(ostream_type& out, long width) const
{
    impl::HelpFormatter<C, T> help_formatter(m_spec, out, width, m_locale);
    help_formatter.format(); // Throws
}


template<class C, class T>
inline BasicProcessor<C, T>::BasicProcessor(std::unique_ptr<command_line_type> command_line_owner,
                                            const command_line_type* command_line,
                                            ProcessorConfig config)
    : m_command_line_owner(std::move(command_line_owner))
    , m_command_line(command_line ? *command_line : *m_command_line_owner)
    , m_config(std::move(config))
    , m_locale(m_command_line.m_root_state.locale)
    , m_logger(m_command_line.m_root_state.logger)
    , m_widener(m_locale) // Throws
    , m_spec_parser(m_locale) // Throws
    , m_spec(m_locale)
{
}


template<class C, class T>
inline void BasicProcessor<C, T>::do_add_pattern(const char* pattern, const char* descr,
                                                 std::unique_ptr<pattern_action_type> action)
{
    string_view_type pattern_2, descr_2;
    typename core::BasicStringWidener<C, T>::Entry entries[] = {
        { pattern, &pattern_2 },
        { descr,   &descr_2   }
    };
    m_widener.widen(entries); // Throws
    do_add_pattern(pattern_2, descr_2, std::move(action)); // Throws
}


template<class C, class T>
void BasicProcessor<C, T>::do_add_pattern(string_view_type pattern, string_view_type descr,
                                          std::unique_ptr<pattern_action_type> action)
{
    string_view_type pattern_2 = pattern;
    string_view_type descr_2   = descr;
    intern({ &pattern_2, &descr_2 }); // Throws
    do_add_pattern_2(pattern_2, descr_2, std::move(action)); // Throws
}


template<class C, class T>
void BasicProcessor<C, T>::do_add_pattern_2(string_view_type pattern, string_view_type descr,
                                                   std::unique_ptr<pattern_action_type> action)
{
    using Snapshot = impl::PatternStructure::Snapshot;
    Snapshot snapshot = m_pattern_structure.snapshot();
    try {
        std::size_t ndx = m_spec.get_num_patterns();
        bool is_deleg = (action && action->is_deleg);
        std::size_t elem_seq_index =
            m_spec_parser.parse_pattern(pattern, m_pattern_structure, m_spec, ndx,
                                        is_deleg); // Throws
        impl::PatternFuncChecker pattern_func_checker(m_pattern_structure);
        bool good;
        if (action) {
            if (is_deleg) {
                // Delegating patterns with value slots are rejected during parsing of the
                // pattern.
                good = true;
            }
            else {
                good = action->check(pattern_func_checker, elem_seq_index);
            }
        }
        else {
            good = pattern_func_checker.check<void()>(elem_seq_index);
        }
        if (!good) {
            // FIXME: Move the pattern function match check to impl::Spec                                    
            throw std::runtime_error("Mismatch between pattern and pattern function");                       
        }
        m_spec.add_pattern(pattern, descr, elem_seq_index, std::move(action)); // Throws
    }
    catch (...) {
        m_pattern_structure.revert(snapshot);
        throw;
    }
}


template<class C, class T>
inline void BasicProcessor<C, T>::do_add_option(const char* forms, const char* arg, int attr,
                                                const char* descr,
                                                std::unique_ptr<option_action_type> action)
{
    string_view_type forms_2, arg_2, descr_2;
    typename core::BasicStringWidener<C, T>::Entry entries[] = {
        { forms, &forms_2 },
        { arg,   &arg_2   },
        { descr, &descr_2 }
    };
    m_widener.widen(entries); // Throws
    do_add_option(forms_2, arg_2, attr, descr_2, std::move(action)); // Throws
}


template<class C, class T>
void BasicProcessor<C, T>::do_add_option(string_view_type forms, string_view_type arg, int attr,
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
void BasicProcessor<C, T>::do_add_option_2(string_view_type forms, string_view_type arg, int attr,
                                           string_view_type descr,
                                           std::unique_ptr<option_action_type> action)
{
    core::ArraySeededBuffer<OptionForm, 4> buffer;
    std::size_t ndx = m_spec.get_num_options();
    core::Span<OptionForm> forms_2 =
        m_spec_parser.parse_option_forms(ndx, forms, buffer); // Throws
    core::Buffer<OptionForm> chunk(forms_2); // Throws
    core::Span<OptionForm> forms_3 = chunk;
    m_option_form_chunks.push_back(std::move(chunk)); // Throws
    ArgSpec arg_2 = m_spec_parser.parse_option_arg(ndx, arg); // Throws
    m_spec.add_option(forms_3, arg_2, attr, descr, std::move(action)); // Throws
}


template<class C, class T>
void BasicProcessor<C, T>::intern(std::initializer_list<string_view_type*> strings)
{
    std::size_t total_size = 0;
    for (const string_view_type* string : strings)
        core::int_add(total_size, string->size()); // Throws
    core::Buffer<char_type> chunk(total_size); // Throws
    char_type* data = chunk.data();
    for (string_view_type* string : strings) {
        std::size_t size = string->size();
        char_type* data_2 = std::copy_n(string->data(), size, data);
        *string = { data, size };
        data = data_2;
    }
    m_string_chunks.push_back(std::move(chunk)); // Throws
}


template<class C, class T>
void pat(const char* pattern, BasicProcessor<C, T>& proc, const char* descr)
{
    proc.add_pattern(pattern, descr); // Throws
}


template<class C, class T, class A>
void pat(const char* pattern, BasicProcessor<C, T>& proc, const char* descr, A&& action)
{
    proc.add_pattern(pattern, descr, std::forward<A>(action)); // Throws
}


template<class C, class T>
void pat(typename core::Wrap<std::basic_string_view<C, T>>::type pattern,
         BasicProcessor<C, T>& proc, typename core::Wrap<std::basic_string_view<C, T>>::type descr)
{
    proc.add_pattern(pattern, descr); // Throws
}


template<class C, class T, class A>
void pat(typename core::Wrap<std::basic_string_view<C, T>>::type pattern,
         BasicProcessor<C, T>& proc, typename core::Wrap<std::basic_string_view<C, T>>::type descr,
         A&& action)
{
    proc.add_pattern(pattern, descr, std::forward<A>(action)); // Throws
}


template<class C, class T>
void opt(const char* forms, const char* arg, int attr, BasicProcessor<C, T>& proc,
         const char* descr)
{
    proc.add_option(forms, arg, attr, descr); // Throws
}


template<class C, class T, class A>
void opt(const char* forms, const char* arg, int attr, BasicProcessor<C, T>& proc,
         const char* descr, A&& action)
{
    proc.add_option(forms, arg, attr, descr, std::forward<A>(action)); // Throws
}


template<class C, class T>
void opt(typename core::Wrap<std::basic_string_view<C, T>>::type forms,
         typename core::Wrap<std::basic_string_view<C, T>>::type arg, int attr,
         BasicProcessor<C, T>& proc, typename core::Wrap<std::basic_string_view<C, T>>::type descr)
{
    proc.add_option(forms, arg, attr, descr); // Throws
}


template<class C, class T, class A>
void opt(typename core::Wrap<std::basic_string_view<C, T>>::type forms,
         typename core::Wrap<std::basic_string_view<C, T>>::type arg, int attr,
         BasicProcessor<C, T>& proc, typename core::Wrap<std::basic_string_view<C, T>>::type descr,
         A&& action)
{
    proc.add_option(forms, arg, attr, descr, std::forward<A>(action)); // Throws
}


} // namespace archon::cli

#endif // ARCHON_X_CLI_X_PROCESSOR_HPP
