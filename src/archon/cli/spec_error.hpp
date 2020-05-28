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

#ifndef ARCHON_X_CLI_X_SPEC_ERROR_HPP
#define ARCHON_X_CLI_X_SPEC_ERROR_HPP

/// \file


namespace archon::cli {


/// \brief Command-line interface specification errors.
///
/// These are the possible ways in which a command-line interface specification (\ref
/// cli::BasicSpec) can be invalid.
///
/// If an invalid specification is passed to the processor constructor (\ref
/// cli::BasicProcessor), a \ref cli::BadSpec exception will be thrown carrying one of the
/// error codes below.
///
enum class SpecError {
    /// Syntax error in pattern string. A pattern specification carried a pattern string
    /// that could not be parsed due to a syntax error.
    bad_pattern_syntax,

    /// Syntax error in option forms. The option forms string in an option specification
    /// could not be parsed due to a syntax error.
    bad_option_forms_syntax,

    /// Syntax error in option argument. The option argument string in an option
    /// specification could not be parsed due to a syntax error.
    bad_option_arg_syntax,

    /// Invalid pattern attributes. A pattern specification carried invalid pattern
    /// attributes. Only the attributes in \ref cli::PatternAttributes may be specified.
    bad_pattern_attr,

    /// Invalid option attributes. An option specification carried invalid option
    /// attributes. Only the attributes in \ref cli::OptionAttributes may be specified.
    bad_option_attr,

    /// Illegal option reference in pattern. A pattern specification referred to an option
    /// of a type that is not allowed in a pattern.
    bad_option_ref,

    /// Argument not allowed for option. An argument was specified for an option of a type
    /// that is not allowed to have an argument.
    option_arg_not_allowed,

    /// Mismatch between pattern and pattern function. The type of a function specified for
    /// a pattern did not match the specified pattern string.
    pattern_func_mismatch,

    /// Mismatch between option argument specification and type of specified option
    /// function.
    option_func_mismatch,

    /// Reuse of option form. A particular option form was used in multiple option
    /// specifications.
    option_form_reuse,

    /// A pattern specification has structural ambiguity. See \ref
    /// cli::Spec::allow_pattern_internal_positional_ambiguity() for an explanation of what
    /// this means.
    structural_ambiguity,

    /// A pattern specification has positional ambiguity. See \ref
    /// cli::Spec::allow_pattern_internal_positional_ambiguity() for an explanation of what
    /// this means, and for a way to allow for this kind of ambiguity.
    positional_ambiguity,

    /// The specification exhibits cross-pattern ambiguity. See \ref
    /// cli::Spec::allow_cross_pattern_ambiguity() for an explanation of what this means,
    /// and for a way to allow for this kind of ambiguity.
    cross_pattern_ambiguity,

    /// Delegating pattern has match that is prefix of other match. A delegating pattern
    /// matches a command line that is a proper prefix of another command line that matches
    /// the same, or a different pattern.
    prefix_deleg_pattern,
};


} // namespace archon::cli

#endif // ARCHON_X_CLI_X_SPEC_ERROR_HPP
