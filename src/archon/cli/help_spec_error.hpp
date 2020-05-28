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

#ifndef ARCHON_X_CLI_X_HELP_SPEC_ERROR_HPP
#define ARCHON_X_CLI_X_HELP_SPEC_ERROR_HPP

/// \file


namespace archon::cli {


/// \brief Command-line interface help specification errors.
///
/// These are the possible ways in which the specification of the description of a
/// command-line interface can be invalid.
///
/// If an invalid help specification is passed to the processor constructor, and if \ref
/// cli::BasicProcessor::show_help() is called, a \ref cli::BadHelpSpec exception will be
/// thrown carrying one of the error codes below.
///
enum class HelpSpecError {
    /// A syntax error was reported by the string template parser (\ref
    /// core::BasicStringTemplate) while parsing an option description (see \ref
    /// cli::BasicSpec::add_option()). See \ref core::BasicStringTemplate::Parser::Error.
    bad_syntax,

    /// A bad parameter reference error was reported by the string template parser (\ref
    /// core::BasicStringTemplate) while parsing an option description (see \ref
    /// cli::BasicSpec::add_option()). See \ref core::BasicStringTemplate::Parser::Error.
    bad_param_ref,

    /// A parameter occurs in a context where it is not allowed. For example, `@A` in the
    /// description of an option that does not take an argument. See \ref
    /// cli::BasicSpec::add_option() for the list of parameters.
    illegal_combination,
};


} // namespace archon::cli

#endif // ARCHON_X_CLI_X_HELP_SPEC_ERROR_HPP
