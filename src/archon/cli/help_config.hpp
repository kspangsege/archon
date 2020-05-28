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

#ifndef ARCHON_X_CLI_X_HELP_CONFIG_HPP
#define ARCHON_X_CLI_X_HELP_CONFIG_HPP

/// \file


#include <string_view>

#include <archon/core/var_string_ref.hpp>


namespace archon::cli {


/// \brief Configuration parameters controlling help text generation.
///
/// These parameters allow for some control over help text generation performed by \ref
/// cli::BasicProcessor::show_help().
///
template<class C, class T = std::char_traits<C>> class BasicHelpConfig {
public:
    using char_type   = C;
    using traits_type = T;

    using var_string_ref_type = core::BasicVarStringRef<C, T>;

    /// \brief Strip directory part from argv0.
    ///
    /// If set to `true`, only the filename part of `argv0` will be shown. The directory
    /// part will not. Here, `argv0` refers to the first command-line argument, which is
    /// ordinarily a copy of the name, or path of the invoked program as it was specified by
    /// the invoker (in the shell).
    ///
    /// If `argv0` is overridden (replacement text passed during construction of \ref
    /// cli::BasicCommandLine), no removal of a directory part will be attempted, and this
    /// option has no effect.
    ///
    bool argv0_strip_dir = true;

    /// \brief Strip dot-exe suffix from argv0.
    ///
    /// If set to `true`, and if `argv0` has an `.exe` suffix, that suffix will be removed
    /// if the target platform is Windows. On other platforms, this parameter has no effect.
    ///
    /// If `argv0` is overridden (replacement text passed during construction of \ref
    /// cli::BasicCommandLine), no removal of a `.exe` suffix will be attempted, and this
    /// option has no effect.
    ///
    bool argv0_strip_dot_exe = true;

    /// \brief Formatting width for generated text.
    ///
    /// This parameter specifies the width that the generated text should be formatted
    /// to. If the text is sent to a terminal, `width` should ideally be the width of that
    /// terminal. The default value is 80 because the "standard" terminal width is 80, and,
    /// traditionally, terminals were precisely 80 characters wide.
    ///
    /// See also \ref allow_adjust_width_to_terminal for a way to dynamically adapt to the
    /// actual terminal width.
    ///
    std::size_t width = 80;

    /// \{
    ///
    /// \brief Allowed range for adjusted formatting width.
    ///
    /// These parameters specify the allowed range for the choice of formatting width when
    /// the formatting width is fitted to the terminal. See \ref
    /// allow_adjust_width_to_terminal.
    ///
    /// \sa \ref width
    ///
    std::size_t min_width = 80;
    std::size_t max_width = 120;
    /// \}

    /// \brief Indentation for option description.
    ///
    /// The amount of indentation to be used for option descriptions.
    ///
    std::size_t option_descr_indent = 22;

    /// \{
    ///
    /// \brief Indentation for list of option forms.
    ///
    /// The amount of indentation to be used for the list of option forms. In general, for
    /// each option, a list of alternative option forms are displayed, and this list may
    /// extend over multiple lines. `option_form_indent_1` is the amount of indentation to
    /// use on the first of those lines, and `option_form_indent_2` is the amount of
    /// indentation to use on the remaining lines.
    ///
    std::size_t option_form_indent_1 = 2;
    std::size_t option_form_indent_2 = 4;
    /// \}

    /// \{
    ///
    /// \brief Constraints on start of option description.
    ///
    /// These parameters control when, and how the description of an option can start on the
    /// same line as the (last line of the) option forms. Generally, the start of the option
    /// description will occur on the same line as the last option form if both of the
    /// following constraints can be met:
    ///
    ///  * The separation, in number of space characters, between the last option form and
    ///    the start of the description must be greater than, or equal to
    ///    `option_descr_min_separation`.
    ///
    ///  * The description can be shifted to the right in order to satisfy the first
    ///    constraint, but the shift, in number of characters, must be less than, or equal
    ///    to `option_descr_max_displacement`.
    ///
    std::size_t option_descr_min_separation = 3;
    std::size_t option_descr_max_displacement = 0;
    /// \}

    /// \brief Size of right-side margin.
    ///
    /// The size of the right-side margin. If nonzero, this is a blank area that prevents
    /// the generated text from extending all the way to the right-side edge of the terminal
    /// window (assuming that the output is sent to a terminal, and assuming that the
    /// effective formatting with (\ref width) is equal to the width of that terminal).
    ///
    std::size_t right_side_margin = 2;

    /// \brief Allow adjustment of text width to terminal.
    ///
    /// By setting this parameter to `true`, you are allowing that \ref
    /// cli::BasicProcessor::show_help() adjusts the width of the generated text to fit the
    /// width of the terminal. For this to happen, `show_help()` must also believe that it
    /// is sending its output to a terminal, and it must know the width of that terminal.
    ///
    /// When with adjustment takes place, \ref min_width and \ref max_width take effect to
    /// constrain the allowed range.
    ///
    bool allow_adjust_width_to_terminal = false;

    /// \brief Allow use of SGR escape sequences.
    ///
    /// By setting this parameter to `true`, you are allowing the inclusion of ANSI SGR
    /// escape sequences in the output generated by \ref
    /// cli::BasicProcessor::show_help(). For this to happen, `show_help()` must also
    /// believe that it is sending its output to a terminal that supports SGR escape
    /// sequences.
    ///
    bool allow_terminal_sgr_esc_seq = false;

    /// \brief Whether argument should be shown on all option forms.
    ///
    /// If set to `true`, option arguments with be included in all alternative forms of an
    /// option, not just the last one. For example, if an option has forms `-f` and
    /// `--filter`, and argument `<string>`, then the forms will be displayed as
    /// `-f<string>, --filter=<string>` instead of `-f, --filter=<string>`.
    ///
    bool show_opt_arg_on_all_forms = false;

    /// \brief Whether text should be justified.
    ///
    /// If set to `true`, some text elements, such as option descriptions, will be justified
    /// (spaces between words will be expanded so as to obtain a flush right side).
    ///
    bool justify = false;

    /// \brief Text to be used in place of absent value.
    ///
    /// The string specified here will be used when the type of a formatted value is
    /// `std::optional<T>` and `has_value()` returns `false`. This is relevant for option
    /// description substitution parameters `@V`, `@W`, `@Q`, and `@R`. See \ref
    /// cli::BasicSpec::add_option().
    ///
    var_string_ref_type absent_value_label = "none";

    /// \brief Copy variant string references "by reference".
    ///
    /// This function makes a copy of the configuration object, although the parameters of
    /// variant string reference type will be "copied by reference" (see \ref
    /// core::BasicVarStringRef::copy_by_ref()).
    ///
    /// IMPORTANT: The copy may, or may not refer to memory owned by the original. It is
    /// therefore necessary that the original is kept alive for as long as the copy remains
    /// in use. The copy can be destroyed after destruction of the original, however.
    ///
    BasicHelpConfig copy_by_ref() const noexcept;
};


using HelpConfig     = BasicHelpConfig<char>;
using WideHelpConfig = BasicHelpConfig<wchar_t>;








// Implementation


template<class C, class T>
inline auto BasicHelpConfig<C, T>::copy_by_ref() const noexcept -> BasicHelpConfig
{
    return {
        argv0_strip_dir,
        argv0_strip_dot_exe,
        width,
        min_width,
        max_width,
        option_descr_indent,
        option_form_indent_1,
        option_form_indent_2,
        option_descr_min_separation,
        option_descr_max_displacement,
        right_side_margin,
        allow_adjust_width_to_terminal,
        allow_terminal_sgr_esc_seq,
        show_opt_arg_on_all_forms,
        justify,
        absent_value_label.copy_by_ref(),
    };
}


} // namespace archon::cli

#endif // ARCHON_X_CLI_X_HELP_CONFIG_HPP
