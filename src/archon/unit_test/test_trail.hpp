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

#ifndef ARCHON_X_UNIT_TEST_X_TEST_TRAIL_HPP
#define ARCHON_X_UNIT_TEST_X_TEST_TRAIL_HPP

/// \file


#include <string>

#include <archon/base/prefix_logger.hpp>
#include <archon/unit_test/test_context.hpp>


namespace archon::unit_test {


/// \brief Build breadcrumb trail within unit test.
///
/// By using this macro, you can build a breadcrumb trail within a unit test and
/// have that breadcrumb trail be revealed as part of the message, that is
/// generated when a check fails. This assumes that \ref Reporter::fail() of the
/// configured reporter uses the passed logger to report its message (the
/// default reporter does this).
///
/// More specifically, this macro derives a new test context from the specified
/// parent test context. Since this is effectively a new variable named
/// `test_context`, any subsequent checks will automatically bind to it, rather
/// than to the parent context (see the example below). Any number of nested
/// sub-test contexts can be introduced this way.
///
/// The specified trail segment (\p trail_segment) must be something that can be
/// explicitely converted to `std::string`.
///
/// Messages logged by the unit test through \ref TestContext::logger of the
/// derived test context will also show the built up breadcrumb trail.
///
/// This macro is a short-hand for defining a new variable with name
/// `test_context`, and type \ref SubtestContext, constructed from the specified
/// arguments.
///
/// Example:
///
/// \code{.cpp}
///
///   ARCHON_TEST(Foo)
///   {
///       std::array<char, 16> seed_memory;
///       archon::base::ValueFormatter formatter(seed_memory);
///       auto subtest = [&, &parent_test_context = test_context](int i) {
///           ARCHON_TEST_TRAIL(parent_test_context, formatter.format(i));
///           ARCHON_CHECK(foo(i));
///       };
///       subtest(7);
///       subtest(9);
///       subtest(13);
///   }
///
/// \endcode
///
#define ARCHON_TEST_TRAIL(parent_test_context, trail_segment)   \
    X_ARCHON_TEST_TRAIL(parent_test_context, trail_segment)



/// \brief Sub-test context for building breadcrumb trails.
///
/// See \ref ARCHON_TEST_TRAIL().
///
class SubtestContext final :
        public TestContext {
public:
    SubtestContext(TestContext& parent_test_context, std::string&& trail_segment) noexcept;

private:
    class PrefixLogger final : private base::Logger::Prefix, public base::Logger {
    public:
        PrefixLogger(base::Logger& base_logger, const SubtestContext&) noexcept;

        // Overriding functions from base::Logger::Prefix
        void format_prefix(std::ostream&) const override final;

    private:
        const base::Logger::Prefix& m_parent_prefix;
        const SubtestContext& m_subtest_context;
    };

    const std::string m_trail_segment;
    PrefixLogger m_report_logger;
    PrefixLogger m_inner_logger;
};








// Implementation


#define X_ARCHON_TEST_TRAIL(parent_test_context, trail_segment)         \
    archon::unit_test::SubtestContext test_context(parent_test_context, std::string(trail_segment))


inline SubtestContext::SubtestContext(TestContext& parent_test_context,
                                      std::string&& trail_segment) noexcept :
    TestContext(get_thread_context_impl(parent_test_context), parent_test_context.test_details,
                parent_test_context.mapped_file_path, parent_test_context.test_index,
                parent_test_context.recurrence_index, m_report_logger, m_inner_logger),
    m_trail_segment(std::move(trail_segment)),
    m_report_logger(get_report_logger(parent_test_context), *this),
    m_inner_logger(parent_test_context.logger, *this)
{
}


inline SubtestContext::PrefixLogger::PrefixLogger(base::Logger& base_logger, const SubtestContext& subtest_context) noexcept :
    base::Logger(*this, base_logger.get_channel(), base_logger.get_map()),
    m_parent_prefix(base_logger.get_prefix()),
    m_subtest_context(subtest_context)
{
}


} // namespace archon::unit_test

#endif // ARCHON_X_UNIT_TEST_X_TEST_TRAIL_HPP
