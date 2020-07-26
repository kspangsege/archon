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

#ifndef ARCHON_X_UNIT_TEST_X_RUN_HPP
#define ARCHON_X_UNIT_TEST_X_RUN_HPP

/// \file


#include <archon/unit_test/test_config.hpp>


namespace archon::unit_test {


/// \brief Execute unit tests.
///
/// This function executes a number of unit tests. The list of unit tests to
/// consider for execution is specified via \ref TestConfig::test_list. The
/// tests that are enabled and match the specified filter (\ref
/// TestConfig::filter) will be executed. When the default configuration is
/// used, then all the tests in the list returned by \ref
/// TestList::get_default_list() will be executed.
///
/// \return `true` if all tests succeed, else `false`.
///
bool run(TestConfig = {});


} // namespace archon::unit_test

#endif // ARCHON_X_UNIT_TEST_X_RUN_HPP
