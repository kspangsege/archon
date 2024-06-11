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

#ifndef ARCHON_X_CHECK_X_TEST_RUNNER_HPP
#define ARCHON_X_CHECK_X_TEST_RUNNER_HPP

/// \file


#include <utility>
#include <memory>
#include <iostream>

#include <archon/log/logger.hpp>
#include <archon/check/test_config.hpp>


namespace archon::check {


/// \brief Provide for execution of test cases.
///
/// This class provides for the execution of test cases. The list of test cases is specified
/// via \ref check::TestConfig::test_list.
///
/// For a particular test runner, \ref run() should never be executed by two threads in an
/// overlapping fashion, as that would lead to possible clobbering of files. On the other
/// hand, two test runners can safely execute the same test list, provided that they are
/// configured to not clobber each other's files (see \ref
/// check::TestConfig::test_file_base_dir and \ref check::TestConfig::log_file_base_dir).
///
/// The most convenient way to define individual test cases, is to use \ref ARCHON_TEST(),
/// or another macro in that family.
///
/// \sa \ref check::run()
///
class TestRunner {
public:
    /// \{
    ///
    /// \brief Construct test runner from specified configuration.
    ///
    /// The specified locale will be use internally in the testing framework, and will be
    /// exposed to test cases via \ref check::TestContext::locale.
    ///
    /// If no locale is specified, a copy of the global locale will be used.
    ///
    TestRunner(check::TestConfig = {});
    TestRunner(const std::locale&, check::TestConfig = {});
    /// \}

    /// \brief Execute selected test cases.
    ///
    /// This function executes the test cases that are enabled, and match the configured
    /// filter (\ref check::TestConfig::filter).
    ///
    /// When the default configuration is used, all the tests in the list returned by \ref
    /// check::TestList::get_default_list() will be executed.
    ///
    /// \return `true` if all tests succeed, else `false`.
    ///
    bool run() const;

    auto get_logger() const noexcept -> log::Logger&;

    auto get_config() const noexcept -> const check::TestConfig&;

private:
    const std::locale m_locale;
    const check::TestConfig m_config;
    const std::unique_ptr<log::Logger> m_logger_owner;
    log::Logger& m_logger;

    static auto make_logger(const std::locale&, const check::TestConfig&) -> std::unique_ptr<log::Logger>;
};








// Implementation


inline TestRunner::TestRunner(check::TestConfig config)
    : TestRunner({}, std::move(config)) // Throws
{
}


inline auto TestRunner::get_logger() const noexcept -> log::Logger&
{
    std::cerr << "TestRunner::get_logger(): " << static_cast<const void*>(this) << ", " <<  static_cast<void*>(&m_logger) << "\n";    
    return m_logger;
}


inline auto TestRunner::get_config() const noexcept -> const check::TestConfig&
{
    return m_config;
}


} // namespace archon::check

#endif // ARCHON_X_CHECK_X_TEST_RUNNER_HPP
