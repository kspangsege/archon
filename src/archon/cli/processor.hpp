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
#include <type_traits>
#include <algorithm>
#include <utility>
#include <memory>
#include <variant>
#include <string_view>
#include <string>
#include <vector>
#include <locale>
#include <ios>
#include <ostream>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/buffer_contents.hpp>
#include <archon/core/array_seeded_buffer.hpp>
#include <archon/core/vector.hpp>
#include <archon/core/char_mapper.hpp>
#include <archon/core/var_string_ref.hpp>
#include <archon/core/format_enc.hpp>
#include <archon/core/file.hpp>
#include <archon/core/text_file_stream.hpp>
#include <archon/log/logger.hpp>
#include <archon/cli/impl/value_parser.hpp>
#include <archon/cli/error_handler.hpp>
#include <archon/cli/logging_error_handler.hpp>
#include <archon/cli/config.hpp>
#include <archon/cli/attributes.hpp>
#include <archon/cli/help_config.hpp>
#include <archon/cli/impl/root_state.hpp>
#include <archon/cli/impl/option_action.hpp>
#include <archon/cli/impl/error_accum.hpp>
#include <archon/cli/impl/option_invocation.hpp>
#include <archon/cli/command_line.hpp>
#include <archon/cli/impl/pattern_structure.hpp>
#include <archon/cli/impl/pattern_args_parser.hpp>
#include <archon/cli/impl/pattern_func_checker.hpp>
#include <archon/cli/impl/pattern_action.hpp>
#include <archon/cli/impl/spec.hpp>
#include <archon/cli/spec.hpp>
#include <archon/cli/impl/processor.hpp>
#include <archon/cli/impl/help_formatter.hpp>
#include <archon/cli/impl/spec_parser.hpp>


namespace archon::cli {


/// \brief Command line processor.
///
/// FIXME: Explain: The comamnd line and spec objects passed to the constructor must remain alive for as lone as the processor remains in use (the processor can be safely destroyed after the destruction of command line and spec objects).                
///
/// FIXME: Properly explain the rules about the lifetime of the memory referenced by string views passed back to the application.                            
///
template<class C, class T = std::char_traits<C>> class BasicProcessor {
public:
    using char_type   = C;
    using traits_type = T;

    using ostream_type      = std::basic_ostream<C, T>;
    using help_config_type  = cli::BasicHelpConfig<C, T>;
    using command_line_type = cli::BasicCommandLine<C, T>;
    using spec_type         = cli::BasicSpec<C, T>;

    using Config = cli::BasicConfig<C, T>;

    BasicProcessor(int argc, const char* const argv[], const spec_type&, Config = {});
    BasicProcessor(int argc, const char* const argv[], const spec_type&, const std::locale&, Config = {});
    BasicProcessor(const command_line_type&, const spec_type&);

    /// \brief Process command line.
    ///
    /// This function processes the command line that was passed to the constructor (\ref
    /// BasicProcessor()).
    ///
    /// It returns `true` if something went wrong, or if it believes that the duties of the
    /// invoked program have been carried out. Otherwise it returns `false`.
    ///
    /// More precisely, it returns `true` if the processing of the command line failed, if a
    /// short-circuit option was encountered and therefore acted upon (think `--help`), if a
    /// pattern was invoked but returned a nonzero exit status, or if a pattern was invoked
    /// and that pattern carries the `completing` attribute (\ref
    /// cli::PatternAttributes::completing). In all other cases, it returns `false`.
    ///
    /// A value is assigned to \p exit_status when, and only when this function returns
    /// `true`.
    ///
    /// If processing of the command line failed, \p exit_status is set to `EXIT_FAILURE` if
    /// an error handler was not specified (see \ref cli::BasicConfig::error_handler). If an
    /// error handler was specified, it is set to whatever was returned by the error
    /// handler.
    ///
    /// If a short-circuit option was acted upon, \p exit_status is set to `EXIT_SUCCESS`.
    ///
    /// If a pattern was matched, and that pattern is assciated with a function, and that
    /// function returned a nonzero exit status, \p exit_status is set to that nonzero
    /// value.
    ///
    /// If a pattern was matched, and that pattern does carry the `completing` attribute,
    /// then if the pattern is associated with a function, and the return type of that
    /// function is `int`, \p exit_status is set to whater is returned by the function. If
    /// the pattern is not associated with a function, or if the return type of that
    /// function is `void`, \p exit_status is set to `EXIT_SUCCESS`.
    ///
    /// In a program with only one pattern, one can choose to carry out the duties of that
    /// program from `main()` rather than from some function that is associated with the
    /// pattern. In such a case, the intention is that the return value from `process()` is
    /// used to decide whether to quit eary, or carry out the primary duties of the
    /// program. Note that this also covers the case where there are no explicitely
    /// specified patterns, because that corresponds to having a single pattern that is
    /// empty.
    ///
    /// Here is an example of such a use case:
    ///
    /// \code{.cpp}
    ///
    ///    int main(int argc, char* argv[])
    ///    {
    ///        std::filesystem::path origin_path, target_path;
    ///        archon::cli::Spec spec;
    ///        pat("<origin path> <target path>", archon::cli::no_attributes, spec,
    ///            "Copy the file at the specified origin path to specified target path.",
    ///            std::tie(origin_path, target_path));
    ///        archon::cli::Processor proc(argc, argv, spec);
    ///        int exit_status;
    ///        if (proc.process(exit_status))
    ///            return exit_status;
    ///        std::filesystem::copy_file(origin_path, target_path);
    ///    }
    ///
    /// \endcode
    ///
    /// Alternatively, if the program has multiple patterns, and the corresponding duties
    /// are carried out in respective pattern functions, the return value from `process()`
    /// might not be relevant. Here is an example of such a use case:
    ///
    /// \code{.cpp}
    ///
    ///    int main(int argc, char* argv[])
    ///    {
    ///        archon::cli::Spec spec;
    ///        pat("copy <origin path> <target path>", archon::cli::no_attributes, spec,
    ///            "Copy the file at the specified origin path to specified target path.",
    ///            [](std::filesystem::path origin_path, std::filesystem::path target_path) {
    ///                std::filesystem::copy_file(origin_path, target_path);
    ///            });
    ///        pat("move <origin path> <target path>", archon::cli::no_attributes, spec,
    ///            "Move the file at the specified origin path to specified target path.",
    ///            [](std::filesystem::path origin_path, std::filesystem::path target_path) {
    ///                std::filesystem::rename(origin_path, target_path);
    ///            });
    ///        archon::cli::Processor proc(argc, argv, spec);
    ///        int exit_status = EXIT_SUCCESS;
    ///        proc.process(exit_status);
    ///        return exit_status;
    ///    }
    ///
    /// \endcode
    ///
    /// Note that in the second example above, we could have specified the `completing`
    /// attribute (\ref cli::completing) for the two command-line patterns, and it would
    /// have been fitting in terms of the meaning of that attribute, but it would also not
    /// have made any difference, as we don't care about the value returned by `process()`
    /// in that example.
    ///
    /// On the other hand, one could imagine a case where there is a main pattern whose
    /// duties are carried out directly from main(), and then also one or more additional
    /// patterns whose duties are acarried out by pattern functions. In such a case, it
    /// would be appropriate to use the `completing` attribute on those additional patterns.
    ///
    /// Note that in the two examples above, we explicitely instantiate a processor
    /// object. This can generally be avoided by using \ref cli::process(). Here is a
    /// version of the first example above that uses \ref cli::process() instead:
    ///
    /// \code{.cpp}
    ///
    ///    int main(int argc, char* argv[])
    ///    {
    ///        std::filesystem::path origin_path, target_path;
    ///        archon::cli::Spec spec;
    ///        pat("<origin path> <target path>", archon::cli::no_attributes, spec,
    ///            "Copy the file at the specified origin path to specified target path.",
    ///            std::tie(origin_path, target_path));
    ///        int exit_status;
    ///        if (archon::cli::process(argc, argv, spec, exit_status))
    ///            return exit_status;
    ///        std::filesystem::copy_file(origin_path, target_path);
    ///    }
    ///
    /// \endcode
    ///
    /// Note that this form has an extra advantage in that it does not leave a processor
    /// instance in scope while executing the main duties of the program.
    ///
    /// \sa \ref cli::process()
    ///
    bool process(int& exit_status) const;

    struct TerminalInfo;

    /// \{
    ///
    /// \brief Print out help text.
    ///
    /// These functions print out text that describes the command-line interface.
    ///
    /// Patterns and options will be shown in the order that they were added to the
    /// interface specification that was passed to the processor constructor.
    ///
    /// The specified output stream (\p out) must use a locale that is compatible with the
    /// locale of the command-line processor. The important thing is that the character
    /// encodings agree (`std::codecvt` facet).
    ///
    /// The overload that has an output stream argument (\p out) does not flush that stream
    /// before returning.
    ///
    /// If \p terminal_supports_sgr_esc_seq is `true` this function will assume that \p out
    /// sends its output to a terminal, and that that terminal supports ANSI SGR escape
    /// sequences. This will enabled certain optional features of the help text generation
    /// (\ref cli::HelpConfig::allow_terminal_sgr_esc_seq).
    ///
    /// If \p terminal_width is greater than, or equal to zero, this function will assume
    /// that \p out sends its output to a terminal, and that \p terminal_width is the width
    /// of that terminal. This will enabled certain optional features of the help text
    /// generation (\ref cli::HelpConfig::allow_adjust_width_to_terminal).
    ///
    /// The overload that takes a file argument (\p file) is a shorthand for constructing a
    /// text file output stream (\ref core::TextFileOutputStream) from that file, and then
    /// passing it to the overload that takes an output stream argument (\p out). In this
    /// case, if `file.is_terminal()` returns `true`, `file` is assumed to refer to a
    /// terminal that does support SGR escape sequences. Also, if
    /// `file.try_get_terminal_size()` succeeds, \p terminal_width is set to the reported
    /// width.
    ///
    /// \sa \ref cli::BasicSpec::add_help_option()
    ///
    void show_help(core::File& file, help_config_type = {}) const;
    void show_help(ostream_type& out, help_config_type = {}, bool terminal_supports_sgr_esc_seq = false,
                   int terminal_width = -1) const;
    /// \}

private:
    using string_view_type         = std::basic_string_view<C, T>;
    using option_invocation_type   = impl::OptionInvocation<C, T>;
    using value_parser_type        = impl::ValueParser<C, T>;
    using error_accum_type         = impl::ErrorAccum<C, T>;
    using pattern_structure_type   = impl::PatternStructure<C, T>;
    using pattern_args_parser_type = impl::PatternArgsParser<C, T>;
    using impl_spec_type           = impl::Spec<C, T>;
    using impl_processor_type      = impl::Processor<C, T>;
    using error_handler_type       = cli::BasicErrorHandler<C, T>;

    using Desc       = typename pattern_args_parser_type::Desc;
    using Pattern    = typename impl_spec_type::Pattern;
    using ErrorEntry = typename error_handler_type::ErrorEntry;

    class NullPatternAction;
    class NullOptionAction;
    class HelpOptionAction;

    static constexpr bool is_degenerate = std::is_same_v<string_view_type, std::string_view>;

    std::unique_ptr<command_line_type> m_command_line_owner;
    const command_line_type& m_command_line;
    const spec_type& m_spec;
    std::unique_ptr<char_type[]> m_string_data;
    pattern_structure_type m_pattern_structure;
    impl_spec_type m_impl_spec;
    NullPatternAction m_null_pattern_action;
    NullOptionAction m_null_option_action;
    core::Vector<std::unique_ptr<HelpOptionAction>, 1> m_help_option_actions;

    BasicProcessor(std::unique_ptr<command_line_type> command_line_owner, const command_line_type*, const spec_type&);

    bool do_process(value_parser_type&, error_accum_type&, std::vector<option_invocation_type>&, bool& shorted,
                    bool& deleg, const Pattern*&, std::vector<Desc>&, std::size_t& args_offset, bool& has_error) const;

    template<class... P> auto format_except(const char* message, const P&... params) const -> std::string;

    int report_errors(const error_accum_type&) const;

    static void gather_errors(const command_line_type&, const error_accum_type&, core::BufferContents<ErrorEntry>&);

    static void invoke_options(impl_processor_type&, const command_line_type&,
                               core::Span<const option_invocation_type>);
};


using Processor     = BasicProcessor<char>;
using WideProcessor = BasicProcessor<wchar_t>;








// Implementation


template<class C, class T>
class BasicProcessor<C, T>::NullPatternAction
    : public impl::PatternAction<C, T> {
public:
    using pattern_func_checker_type = impl::PatternFuncChecker<C, T>;
    using value_parser_type         = impl::ValueParser<C, T>;
    using error_accum_type          = impl::ErrorAccum<C, T>;
    using pattern_args_parser_type  = impl::PatternArgsParser<C, T>;

    NullPatternAction() noexcept
        : impl::PatternAction<C, T>(false)
    {
    }

    bool check(const pattern_func_checker_type&, std::size_t) const noexcept
    {
        return true;
    }

    bool invoke(const pattern_args_parser_type&, bool has_error, value_parser_type&, error_accum_type&, int&) const
    {
        return !has_error;
    }
};


template<class C, class T>
class BasicProcessor<C, T>::NullOptionAction
    : public impl::OptionAction<C, T> {
};


template<class C, class T>
class BasicProcessor<C, T>::HelpOptionAction
    : public impl::OptionAction<C, T> {
public:
    HelpOptionAction(const BasicProcessor& processor, help_config_type help_config)
        : m_processor(processor)
        , m_help_config(std::move(help_config)) // Throws
    {
    }

    void invoke_without_arg() const override final
    {
        m_processor.show_help(core::File::get_cout(), m_help_config.copy_by_ref()); // Throws
    }

private:
    const BasicProcessor& m_processor;
    help_config_type m_help_config;
};


template<class C, class T>
inline BasicProcessor<C, T>::BasicProcessor(int argc, const char* const argv[], const spec_type& spec, Config config)
    : BasicProcessor(std::make_unique<command_line_type>(argc, argv, std::locale(), std::move(config)), nullptr,
                     spec) // Throws
{
}


template<class C, class T>
inline BasicProcessor<C, T>::BasicProcessor(int argc, const char* const argv[], const spec_type& spec,
                                            const std::locale& locale, Config config)
    : BasicProcessor(std::make_unique<command_line_type>(argc, argv, locale, std::move(config)), nullptr,
                     spec) // Throws
{
}


template<class C, class T>
inline BasicProcessor<C, T>::BasicProcessor(const command_line_type& command_line, const spec_type& spec)
    : BasicProcessor(nullptr, &command_line, spec) // Throws
{
}


template<class C, class T>
bool BasicProcessor<C, T>::process(int& exit_status) const
{
    const impl::RootState<C, T>& root_state = m_command_line.m_root_state;
    value_parser_type value_parser(root_state.string_holder, root_state.locale); // Throws
    error_accum_type error_accum(root_state.locale); // Throws
    std::vector<option_invocation_type> option_invocations;
    bool shorted = false;
    bool deleg = false;
    std::size_t args_offset = m_command_line.m_args_offset;
    bool has_error = false;
    if (m_command_line.m_parent)
        has_error = m_command_line.m_parent->has_error;
    const Pattern* pattern = nullptr;
    std::vector<Desc> descs;

    if (ARCHON_LIKELY(do_process(value_parser, error_accum, option_invocations, shorted, deleg, pattern, descs,
                                 args_offset, has_error))) { // Throws
        if (ARCHON_LIKELY(!shorted)) {
            if (ARCHON_LIKELY(!deleg)) {
                option_invocations.clear();
                option_invocations.shrink_to_fit(); // Throws
                core::Span<const string_view_type> args = m_command_line.m_root_state.args;
                pattern_args_parser_type pattern_args_parser(m_pattern_structure, args, descs,
                                                             root_state.show_arg_max_size);
                int exit_status_2 = 0;
                bool success = pattern->action.invoke(pattern_args_parser, has_error, value_parser,
                                                      error_accum, exit_status_2); // Throws
                if (ARCHON_LIKELY(success)) {
                    bool completing = ((pattern->attr & cli::completing) != 0);
                    if (ARCHON_LIKELY(!completing))
                        return false;
                    exit_status = exit_status_2;
                    return true;
                }
                goto error;
            }

            option_invocations.shrink_to_fit(); // Throws
            typename command_line_type::Parent parent {
                m_command_line,
                option_invocations,
                error_accum,
                has_error,
                pattern->pattern,
            };
            ARCHON_ASSERT(args_offset >= m_command_line.m_args_offset);
            command_line_type command_line(parent, args_offset);
            exit_status = pattern->action.deleg(command_line); // Throws
            return true; // Complete due to delegation having occurred
        }
        exit_status = EXIT_SUCCESS;
        return true; // Complete due to short-circuit option having been acted on
    }
    ARCHON_ASSERT(has_error);

  error:
    exit_status = report_errors(error_accum); // Throws
    return true; // Complete due to error
}


template<class C, class T>
inline void BasicProcessor<C, T>::show_help(core::File& file, help_config_type config) const
{
    core::BasicTextFileStream<C, T> out(&file); // Throws
    out.exceptions(std::ios_base::badbit | std::ios_base::failbit); // Throws
    out.imbue(m_command_line.m_root_state.locale); // Throws
    core::File::TerminalInfo info;
    bool is_terminal = file.get_terminal_info(info); // Throws
    // If `file` is a terminal, we assume that it supports ANSI SGR escape sequences.
    bool terminal_supports_sgr_esc_seq = is_terminal;
    int terminal_width = -1;
    if (info.size.has_value())
        terminal_width = info.size.value().width;
    show_help(out, config.copy_by_ref(), terminal_supports_sgr_esc_seq, terminal_width); // Throws
    out.flush(); // Throws
}


template<class C, class T>
void BasicProcessor<C, T>::show_help(ostream_type& out, help_config_type help_config,
                                     bool terminal_supports_sgr_esc_seq, int terminal_width) const
{
    const impl::RootState<C, T>& root_state = m_command_line.m_root_state;
    const std::locale& locale = root_state.locale;
    ARCHON_ASSERT(root_state.args.size() > 0);
    string_view_type argv0 = root_state.args[0];
    std::basic_string<C, T> argv0_owner;
    if (root_state.argv0_override.has_value()) {
        argv0 = root_state.argv0_override.value();
    }
    else {
        if (help_config.argv0_strip_dir) {
            using string_codec_type = core::BasicStringCodec<C, T>;
            string_codec_type string_codec(locale); // Throws
            std::array<char, 128> encode_seed_memory;
            typename string_codec_type::ShortCircuitEncodeBuffer encode_buffer(encode_seed_memory);
            std::string_view argv0_2 = string_codec.encode_sc(argv0, encode_buffer); // Throws
            std::filesystem::path path = core::make_fs_path_auto(argv0_2, locale); // Throws
            std::string argv0_3 = core::path_to_string_generic(path.filename(), locale); // Throws
            std::array<char_type, 32> decode_seed_memory;
            typename string_codec_type::ShortCircuitDecodeBuffer decode_buffer(decode_seed_memory);
            argv0_owner = string_codec.decode_sc(argv0_3, decode_buffer); // Throws
            argv0 = argv0_owner;
        }
#if ARCHON_WINDOWS
        if (help_config.argv0_strip_dot_exe) {
            using char_mapper_type = core::BasicCharMapper<C, T>;
            char_mapper_type char_mapper(locale); // Throws
            constexpr std::string_view suffix = ".exe"; // Throws
            constexpr std::size_t size = suffix.size();
            char_type buffer[size];
            char_mapper.widen(suffix, buffer); // Throws
            string_view_type suffix_2 = { buffer, size }; // Throws
            if (ARCHON_LIKELY(argv0.ends_with(suffix_2)))
                argv0.remove_suffix(size);
        }
#endif
    }
    std::array<string_view_type, 3> seed_memory;
    core::Buffer<string_view_type> buffer(seed_memory);
    std::size_t offset = buffer.size();
    const typename command_line_type::Parent* parent = m_command_line.m_parent;
    while (parent) {
        buffer.prepend_a(parent->pattern, offset); // Throws
        parent = parent->command_line.m_parent;
    }
    core::Span<const string_view_type> parent_patterns = core::Span(buffer).subspan(offset);
    impl::HelpFormatter<C, T> help_formatter(argv0, parent_patterns, m_impl_spec, out, locale, std::move(help_config),
                                             terminal_supports_sgr_esc_seq, terminal_width); // Throws
    help_formatter.format(); // Throws
}


template<class C, class T>
BasicProcessor<C, T>::BasicProcessor(std::unique_ptr<command_line_type> command_line_owner,
                                     const command_line_type* command_line, const spec_type& spec)
    : m_command_line_owner(std::move(command_line_owner))
    , m_command_line(command_line ? *command_line : *m_command_line_owner)
    , m_spec(spec)
    , m_impl_spec(m_command_line.m_root_state.locale) // Throws
{
    core::BasicCharMapper<C, T> char_mapper(m_command_line.m_root_state.locale); // Throws

    using var_string_ref_type = core::BasicVarStringRef<C, T>;

    // Create buffer for storing widened strings
    std::size_t string_data_size = 0;
    if constexpr (!is_degenerate) {
        auto examine = [&](const var_string_ref_type& string) noexcept {
            std::string_view string_1;
            string_view_type string_2; // Dummy
            if (ARCHON_LIKELY(string.get(string_1, string_2)))
                core::int_add(string_data_size, string_1.size()); // Throws
        };
        for (const auto& entry : spec.m_options) {
            examine(entry.forms);
            examine(entry.arg);
            examine(entry.descr);
        }
        for (const auto& entry : spec.m_patterns) {
            examine(entry.pattern);
            examine(entry.descr);
        }
    }
    m_string_data = std::make_unique<char_type[]>(string_data_size); // Throws

    char_type* base = m_string_data.get();
    std::size_t offset = 0;
    auto get_string = [&](const var_string_ref_type& string) {
        std::string_view string_1;
        string_view_type string_2;
        bool not_wide = string.get(string_1, string_2);
        if constexpr (is_degenerate) {
            ARCHON_ASSERT(not_wide);
            return string_1;
        }
        else {
            if (ARCHON_LIKELY(not_wide)) {
                C* data = base + offset;
                std::size_t size = string_1.size();
                char_mapper.widen(string_1, data); // Throws
                string_2 = { data, size };
                ARCHON_ASSERT(size <= std::size_t(string_data_size - offset));
                offset += size;
            }
            return string_2;
        }
    };

    impl::SpecParser<C, T> spec_parser(m_command_line.m_root_state.locale, char_mapper); // Throws

    // Add options
    std::size_t num_options = spec.m_options.size();
    for (std::size_t i = 0; i < num_options; ++i) {
        std::size_t option_index = i;
        ARCHON_ASSERT(option_index == m_impl_spec.get_num_options());
        const auto& entry = spec.m_options[i];
        string_view_type forms = get_string(entry.forms); // Throws
        string_view_type arg   = get_string(entry.arg); // Throws
        string_view_type descr = get_string(entry.descr); // Throws
        using IndexRange = typename impl_spec_type::IndexRange;
        using ArgSpec    = typename impl_spec_type::ArgSpec;
        IndexRange forms_2 = spec_parser.parse_option_forms(forms, m_impl_spec, option_index); // Throws
        ArgSpec arg_2 = spec_parser.parse_option_arg(arg, option_index); // Throws
        using option_action_type = impl::OptionAction<C, T>;
        option_action_type* action = &m_null_option_action;
        if (const auto* ptr = std::get_if<std::unique_ptr<option_action_type>>(&entry.action)) {
            if (*ptr)
                action = ptr->get();
        }
        else {
            const auto* ptr_2 = std::get_if<help_config_type>(&entry.action);
            ARCHON_ASSERT(ptr_2);
            auto help_option_action = std::make_unique<HelpOptionAction>(*this, ptr_2->copy_by_ref()); // Throws
            action = help_option_action.get();
            m_help_option_actions.push_back(std::move(help_option_action)); // Throws
        }
        m_impl_spec.add_option(forms_2, arg_2, entry.attr, descr, *action); // Throws
    }

    // Add patterns
    std::size_t num_patterns = spec.m_patterns.size();
    for (std::size_t i = 0; i < num_patterns; ++i) {
        std::size_t pattern_index = i;
        ARCHON_ASSERT(pattern_index == m_impl_spec.get_num_patterns());
        const auto& entry = spec.m_patterns[i];
        string_view_type pattern = get_string(entry.pattern); // Throws
        string_view_type descr = get_string(entry.descr); // Throws
        using pattern_action_type = impl::PatternAction<C, T>;
        pattern_action_type* action = &m_null_pattern_action;
        if (entry.action)
            action = entry.action.get();
        std::size_t elem_seq_index = spec_parser.parse_pattern(pattern, m_pattern_structure, m_impl_spec,
                                                               pattern_index, action->is_deleg); // Throws
        m_impl_spec.add_pattern(pattern, entry.attr, descr, elem_seq_index, *action, m_pattern_structure); // Throws
    }

    ARCHON_ASSERT(offset == string_data_size);

    // Ensure that there is at least one pattern in the implementation spec
    if (m_impl_spec.get_num_patterns() == 0) {
        std::size_t pattern_index = 0;
        string_view_type pattern;
        int attr = 0;
        string_view_type descr;
        bool is_deleg = false;
        std::size_t elem_seq_index = spec_parser.parse_pattern(pattern, m_pattern_structure, m_impl_spec,
                                                               pattern_index, is_deleg); // Throws
        m_impl_spec.add_pattern(pattern, attr, descr, elem_seq_index, m_null_pattern_action,
                                m_pattern_structure); // Throws
    }

    m_impl_spec.shrink_to_fit(); // Throws
}


// This function is marked "noinline" in order to attempt to minimize the stack frame of the
// caller, i.e., process(), and thereby minimize the total size of the stack at the point
// from where the pattern function is invoked.
template<class C, class T>
ARCHON_NOINLINE bool BasicProcessor<C, T>::do_process(value_parser_type& value_parser, error_accum_type& error_accum,
                                                      std::vector<option_invocation_type>& option_invocations,
                                                      bool& shorted, bool& deleg, const Pattern*& pattern,
                                                      std::vector<Desc>& descs, std::size_t& args_offset,
                                                      bool& has_error) const
{
    impl_processor_type processor(m_command_line.m_root_state, m_command_line.m_args_offset, has_error,
                                  m_pattern_structure, m_impl_spec, value_parser, error_accum,
                                  m_spec.m_pattern_matcher_config); // Throws
    std::size_t deleg_args_offset = 0;
    bool success = processor.process(option_invocations, shorted, deleg, pattern, deleg_args_offset); // Throws

    // Note: `success == true` does not mean that there were no processing errors, only that
    // a pattern was matched (shorted = false) or a short-circuiting option was acted on
    // (shorted = true).

    // Options should be invoked if there was already a processing error. This is in order
    // to generate any remaining errors relating to the parsing / checking of option
    // arguments. Otherwise, options should be invoked so long as we did not act on a
    // short-circuit option and did not match a delegating pattern.
    if (ARCHON_LIKELY(!success || (!shorted && !deleg)))
        invoke_options(processor, m_command_line, option_invocations); // Throws

    has_error = processor.has_error();

    if (ARCHON_LIKELY(success)) { // Throws
        if (ARCHON_LIKELY(!shorted)) {
            if (ARCHON_LIKELY(!deleg)) {
                processor.generate_descs(pattern->elem_seq_index, descs); // Throws
            }
            else {
                args_offset = deleg_args_offset;
            }
        }
        return true;
    }
    return false;
}


template<class C, class T>
template<class... P> inline auto BasicProcessor<C, T>::format_except(const char* message, const P&... params) const ->
    std::string
{
    const std::locale& locale = m_command_line.m_root_state.locale;
    return core::format_enc<char_type>(locale, message, params...); // Throws
}


// This function is marked "noinline" in order to attempt to minimize the stack frame of the
// caller, i.e., process(), and thereby minimize the total size of the stack at the point
// from where the pattern function is invoked.
template<class C, class T>
ARCHON_NOINLINE int BasicProcessor<C, T>::report_errors(const error_accum_type& error_accum) const
{
    core::ArraySeededBuffer<ErrorEntry, 8> buffer;
    core::BufferContents errors(buffer);
    gather_errors(m_command_line, error_accum, errors); // Throws
    std::stable_sort(errors.begin(), errors.end(), [](const ErrorEntry& a, const ErrorEntry& b) {
        return a.arg_index < b.arg_index;
    });
    int exit_status = EXIT_FAILURE;
    if (m_command_line.m_root_state.error_handler) {
        m_command_line.m_root_state.error_handler->handle(errors, exit_status); // Throws
    }
    else {
        const std::locale& locale = m_command_line.m_root_state.locale;
        log::BasicFileLogger<C, T> logger(core::File::get_cerr(), locale); // Throws
        cli::BasicLoggingErrorHandler<C, T> error_handler(logger);
        error_handler.handle(errors, exit_status); // Throws
    }
    return exit_status;
}


template<class C, class T>
void BasicProcessor<C, T>::gather_errors(const command_line_type& command_line, const error_accum_type& error_accum,
                                         core::BufferContents<ErrorEntry>& errors)
{
    const typename command_line_type::Parent* parent = command_line.m_parent;
    if (ARCHON_UNLIKELY(parent))
        gather_errors(parent->command_line, parent->error_accum, errors); // Throws
    error_accum.get_errors(errors); // Throws
}


template<class C, class T>
void BasicProcessor<C, T>::invoke_options(impl_processor_type& processor, const command_line_type& command_line,
                                          core::Span<const option_invocation_type> option_invocations)
{
    const typename command_line_type::Parent* parent = command_line.m_parent;
    if (ARCHON_UNLIKELY(parent))
        invoke_options(processor, parent->command_line, parent->option_invocations); // Throws
    processor.invoke_options(option_invocations); // Throws
}


} // namespace archon::cli

#endif // ARCHON_X_CLI_X_PROCESSOR_HPP
