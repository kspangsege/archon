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

#ifndef ARCHON_X_CLI_X_ATTRIBUTES_HPP
#define ARCHON_X_CLI_X_ATTRIBUTES_HPP

/// \file


namespace archon::cli {


/// \brief Named value for "no attributes".
///
/// As a hint to the reader about the meaning of the argument, this named value can be used
/// in place of "zero" when adding new command-line patterns or options. See \ref
/// cli::BasicSpec::add_pattern() and \ref cli::BasicSpec::add_option().
///
inline constexpr int no_attributes = 0;



/// \brief Available attributes for command-line patterns.
///
/// These are the attributes that are available when specifying new command-line
/// patterns. See \ref cli::BasicSpec::add_pattern().
///
enum PatternAttributes {
    /// Mark command-line pattern as "completing". This attribute marks the command-line
    /// pattern as *completing*, meaning that if the pattern is invoked, then there is
    /// nothing more to be done when the `process()` function returns. See \ref
    /// cli::BasicProcessor::process(), and \ref cli::process(). To be more precise, when a
    /// pattern is invoked during processing of the command line, and that pattern is not
    /// associated with a function (see \ref cli::BasicSpec::add_pattern()), or the return
    /// type of the associated function is `void`, or the function returned zero, then
    /// `process()` returns true if, and only if the pattern carries the `completing`
    /// attribute.
    completing = 1,
};



/// \brief Available attributes for command-line options.
///
/// These are the attributes that are available when specifying new command-line
/// options. See \ref cli::BasicSpec::add_option().
///
enum OptionAttributes {
    /// Make a "short circuit" command-line option. This attribute causes the receiving
    /// command-line option to become a *short circuit* option. If a short circuit option
    /// appears on the command line, parsing of the command line stops immediately with
    /// 'success' indication after the execution of the action for that option. Command line
    /// parse errors are not reported in this case, even if they occur earlier on the
    /// command line. This attribute is intended to be used with options such as `--help`
    /// and `--version`.
    short_circuit = 1,

    /// Remaining arguments are not options. If an option with this attribute appears on the
    /// command line, then none of the subsequent command line arguments will be interpreted
    /// as options, or as pattern keywords. This is useful for forcing value arguments to be
    /// interpreted as such regardless of whether they have leading dashes, and of whether
    /// they could otherwise haven been interpreted as pattern keywords. Conventionally,
    /// this role is assigned to the option `--` (long form option with empty name).
    further_args_are_values = 2,

    /// Make option unlisted. An option with this attribute will not be included the list of
    /// options shown by \ref cli::BasicProcessor::show_help().
    unlisted = 4,
};


} // namespace archon::cli

#endif // ARCHON_X_CLI_X_ATTRIBUTES_HPP
