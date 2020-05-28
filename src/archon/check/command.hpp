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

#ifndef ARCHON_X_CHECK_X_COMMAND_HPP
#define ARCHON_X_CHECK_X_COMMAND_HPP

/// \file


#include <string_view>
#include <locale>

#include <archon/core/span.hpp>
#include <archon/core/build_environment.hpp>


namespace archon::check {


/// \brief Command-line tool for running a test suite.
///
/// This function provides a convenient way of creating a command-line tool for running a
/// particular test suite. Here is an example of its intended use:
///
/// \code{.cpp}
///
///   constexpr std::string_view test_order[] = {
///       "Foo_*",
///       "Bar_*",
///   };
///
///   int main(int argc, char* argv[])
///   {
///       archon::core::BuildEnvironment::Params build_env_params;
///       build_env_params.file_path = __FILE__;
///       build_env_params.bin_path  = "test"; // Relative to build reflection of source root
///       build_env_params.src_path  = "test.cpp"; // Relative to source root
///       std::locale locale;
///       return check::command("Foo 1.0", argc, argv, build_env_params, test_order, locale);
///   }
///
/// \endcode
///
/// This functions executes the tests in the default test list (\ref
/// check::TestList::get_default_list()), which are all generally those tests that are
/// defined through use of test macros such as (\ref ARCHON_TEST) inside those compilation
/// checks that are linked with the caller of this function. Here, "linked" refers to the
/// linking process that follows the compilation process in order to produce an executable
/// program.
///
/// Beyond providing a number of useful command line options, this function sets up a
/// pattern based test order (\ref check::PatternBasedTestOrder) to control the order of
/// test executions. The specified test order entries (\p test order) are passed directly on
/// to the constructor of the pattern based test order object.
///
/// The returned value is suitable for being returned by `main()` (`EXIT_SUCCESS` on
/// success, `EXIT_FAILURE` on failure).
///
/// The specified locale (\p loc) is used for all locale sensitive aspects of the operation
/// of this function. In particular, it is passed to the test runner constructor (\ref
/// check::TestRunner) which makes it available to the individual test cases through \ref
/// check::TestContext::locale.
///
int command(std::string_view label, int argc, char* argv[], const core::BuildEnvironment::Params&,
            core::Span<const std::string_view> test_order, const std::locale& loc);


} // namespace archon::check

#endif // ARCHON_X_CHECK_X_COMMAND_HPP
