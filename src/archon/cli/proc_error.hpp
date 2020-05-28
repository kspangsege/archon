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

#ifndef ARCHON_X_CLI_X_PROC_ERROR_HPP
#define ARCHON_X_CLI_X_PROC_ERROR_HPP

/// \file


namespace archon::cli {


/// \brief Errors that can occur during command line processing.
///
/// These are the types of errors that can occur while a command line is processed (see \ref
/// cli::BasicProcessor). If the command line processor is configured with an error handler
/// (\ref cli::BasicErrorHandler), the application has insight into the types of errors that
/// are generated during command line processing.
///
enum class ProcError {
    /// Unknown option, missing option argument, or argument given for option that takes no
    /// argument.
    bad_option,

    /// Missing command line argument, extraneous command line arguments, or wrong type of
    /// command line argument (e.g., an option where a value is required).
    no_pattern_match,

    /// The specified option argument could not be parsed as a value of the type required by
    /// the option action, e.g., by the type of the function parameter if the option action
    /// is a function.
    bad_option_arg,

    /// A command-line argument could not be parsed as a value of the type required by the
    /// corresponding value slot of the matching pattern. Note that the types required by
    /// value slots are determined by the pattern action, e.g., by the types of the function
    /// parameters if the pattern action is a function.
    bad_pattern_arg
};


} // namespace archon::cli

#endif // ARCHON_X_CLI_X_PROC_ERROR_HPP
