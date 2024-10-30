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


#include <cstring>
#include <limits>
#include <algorithm>
#include <memory>
#include <string_view>
#include <exception>

#include <archon/check.hpp>
#include <archon/check/run.hpp>
#include <archon/check/wildcard_filter.hpp>
#include <archon/check/simple_reporter.hpp>


using namespace archon;


namespace {


check::TestList zero_tests_list, zero_checks_list;
check::TestList one_check_success_list, one_check_failure_list;
check::TestList one_test_success_list, one_test_failure_list;
check::TestList few_tests_success_list, few_tests_failure_list;
check::TestList mixed_list;
check::TestList success_list, failure_list;


class FooException {};


class BarException final
    : public std::exception {
public:
    auto what() const noexcept -> const char* override
    {
        return "bar";
    }
};


void throw_foo()
{
    throw FooException();
}


void throw_bar()
{
    throw BarException();
}


void throw_nothing()
{
}


class SummaryRecorder final
    : public check::Reporter {
public:
    SummaryRecorder(check::Summary& summary) noexcept
        : m_summary(summary)
    {
    }
    void root_end(const check::RootContext&, const check::Summary& summary) override
    {
        m_summary = summary;
    }
private:
    check::Summary& m_summary;
};


void check_summary(check::TestContext& test_context, const check::TestList& test_list, int num_selected_tests,
                   int num_failed_test_executions, int num_excluded_tests, int num_checks, int num_failed_checks)
{
    check::Summary summary;
    {
        ARCHON_TEST_DIR(dir);
        SummaryRecorder reporter(summary);
        check::TestConfig config;
        config.num_threads = 1;
        config.test_list = &test_list;
        config.logger = &test_context.logger;
        config.reporter = &reporter;
        config.test_file_base_dir = dir;
        run(std::move(config));
    }
    ARCHON_CHECK_EQUAL(summary.num_selected_tests, num_selected_tests);
    ARCHON_CHECK_EQUAL(summary.num_failed_test_executions, num_failed_test_executions);
    ARCHON_CHECK_EQUAL(summary.num_excluded_tests, num_excluded_tests);
    ARCHON_CHECK_EQUAL(summary.num_checks, num_checks);
    ARCHON_CHECK_EQUAL(summary.num_failed_checks, num_failed_checks);
}


void check_filtered_summary(check::TestContext& test_context, const check::TestList& test_list,
                            std::string_view filter_str, int num_selected_tests, int num_failed_test_executions,
                            int num_excluded_tests, int num_checks, int num_failed_checks)
{
    check::Summary summary;
    {
        ARCHON_TEST_DIR(dir);
        check::WildcardFilter filter(filter_str);
        SummaryRecorder reporter(summary);
        check::TestConfig config;
        config.num_threads = 1;
        config.test_list = &test_list;
        config.filter = &filter;
        config.logger = &test_context.logger;
        config.reporter = &reporter;
        config.test_file_base_dir = dir;
        run(std::move(config));
    }
    ARCHON_CHECK_EQUAL(summary.num_selected_tests, num_selected_tests);
    ARCHON_CHECK_EQUAL(summary.num_failed_test_executions, num_failed_test_executions);
    ARCHON_CHECK_EQUAL(summary.num_excluded_tests, num_excluded_tests);
    ARCHON_CHECK_EQUAL(summary.num_checks, num_checks);
    ARCHON_CHECK_EQUAL(summary.num_failed_checks, num_failed_checks);
}


} // unnamed namespace




ARCHON_TEST_EX(zero_checks_list, ZeroChecks, true, true)
{
}


ARCHON_TEST_EX(one_check_success_list, OneCheckSuccess, true, true)
{
    ARCHON_CHECK(true);
}


ARCHON_TEST_EX(one_check_failure_list, OneCheckFailure, true, true)
{
    ARCHON_CHECK(false);
}


ARCHON_TEST_EX(one_test_success_list, OneTestSuccess, true, true)
{
    ARCHON_CHECK_EQUAL(0, 0);
    ARCHON_CHECK_NOT_EQUAL(0, 1);
    ARCHON_CHECK(true); // <--- Success
    ARCHON_CHECK_LESS(0, 1);
    ARCHON_CHECK_GREATER(1, 0);
}


ARCHON_TEST_EX(one_test_failure_list, OneTestFailure, true, true)
{
    ARCHON_CHECK_EQUAL(0, 0);
    ARCHON_CHECK_NOT_EQUAL(0, 1);
    ARCHON_CHECK(false); // <--- Failure
    ARCHON_CHECK_LESS(0, 1);
    ARCHON_CHECK_GREATER(1, 0);
}


ARCHON_TEST_EX(few_tests_success_list, FewTestsSuccess_1, true, true)
{
    ARCHON_CHECK_EQUAL(0, 0);
    ARCHON_CHECK_NOT_EQUAL(0, 1);
    ARCHON_CHECK_LESS(0, 1);
    ARCHON_CHECK_GREATER(1, 0);
}


ARCHON_TEST_EX(few_tests_success_list, FewTestsSuccess_2, true, true)
{
    ARCHON_CHECK_EQUAL(0, 0);
    ARCHON_CHECK_NOT_EQUAL(0, 1);
    ARCHON_CHECK(true); // <--- Success
    ARCHON_CHECK_LESS(0, 1);
    ARCHON_CHECK_GREATER(1, 0);
}


ARCHON_TEST_EX(few_tests_success_list, FewTestsSuccess_3, true, true)
{
    ARCHON_CHECK_EQUAL(0, 0);
    ARCHON_CHECK_NOT_EQUAL(0, 1);
    ARCHON_CHECK_LESS(0, 1);
    ARCHON_CHECK_GREATER(1, 0);
}


ARCHON_TEST_EX(few_tests_failure_list, FewTestsFailure_1, true, true)
{
    ARCHON_CHECK_EQUAL(0, 0);
    ARCHON_CHECK_NOT_EQUAL(0, 1);
    ARCHON_CHECK_LESS(0, 1);
    ARCHON_CHECK_GREATER(1, 0);
}


ARCHON_TEST_EX(few_tests_failure_list, FewTestsFailure_2, true, true)
{
    ARCHON_CHECK_EQUAL(0, 0);
    ARCHON_CHECK_NOT_EQUAL(0, 1);
    ARCHON_CHECK(false); // <--- Failure
    ARCHON_CHECK_LESS(0, 1);
    ARCHON_CHECK_GREATER(1, 0);
}


ARCHON_TEST_EX(few_tests_failure_list, FewTestsFailure_3, true, true)
{
    ARCHON_CHECK_EQUAL(0, 0);
    ARCHON_CHECK_NOT_EQUAL(0, 1);
    ARCHON_CHECK_LESS(0, 1);
    ARCHON_CHECK_GREATER(1, 0);
}


ARCHON_TEST_EX(mixed_list, Mixed_1_X, true, true)
{
    ARCHON_CHECK_EQUAL(0, 0);
    ARCHON_CHECK_NOT_EQUAL(0, 1);
    ARCHON_CHECK_LESS(0, 1);
    ARCHON_CHECK_GREATER(1, 0);
}


ARCHON_TEST_EX(mixed_list, Mixed_2_Y, true, true)
{
    ARCHON_CHECK_EQUAL(0, 0);
    ARCHON_CHECK_EQUAL(0, 1); // <--- Failure
    ARCHON_CHECK_LESS(0, 1);
    ARCHON_CHECK_GREATER(1, 0);
}


ARCHON_TEST_EX(mixed_list, Mixed_3_X, true, true)
{
}


ARCHON_TEST_EX(mixed_list, Mixed_4_Y, true, true)
{
    ARCHON_CHECK_NOT_EQUAL(0, 0);     // <--- Failure
    ARCHON_CHECK_EQUAL(0, 1);         // <--- Failure
    ARCHON_CHECK_GREATER_EQUAL(0, 1); // <--- Failure
}


ARCHON_TEST_EX(mixed_list, Mixed_5_X, true, true)
{
    ARCHON_CHECK_NOT_EQUAL(0, 0); // <--- Failure
    ARCHON_CHECK_NOT_EQUAL(0, 1);
    ARCHON_CHECK_GREATER_EQUAL(0, 1); // <--- Failure
    ARCHON_CHECK_GREATER(1, 0);
}


ARCHON_TEST_EX(mixed_list, Mixed_6_Y, true, true)
{
}


ARCHON_TEST_EX(mixed_list, Mixed_7_Y, true, true)
{
    ARCHON_CHECK_EQUAL(0, 0);
    ARCHON_CHECK_NOT_EQUAL(0, 1);
    ARCHON_CHECK_LESS(0, 1);
    ARCHON_CHECK_GREATER(1, 0);
}


// Test #1, accum checks = 0 + 13 = 13
ARCHON_TEST_EX(success_list, Success_Bool, true, true)
{
    ARCHON_CHECK(true);
    ARCHON_CHECK_EQUAL(false, false);
    ARCHON_CHECK_EQUAL(true, true);
    ARCHON_CHECK_NOT_EQUAL(false, true);
    ARCHON_CHECK_NOT_EQUAL(true, false);
    ARCHON_CHECK_LESS(false, true);
    ARCHON_CHECK_GREATER(true, false);
    ARCHON_CHECK_LESS_EQUAL(false, false);
    ARCHON_CHECK_LESS_EQUAL(false, true);
    ARCHON_CHECK_LESS_EQUAL(true, true);
    ARCHON_CHECK_GREATER_EQUAL(false, false);
    ARCHON_CHECK_GREATER_EQUAL(true, false);
    ARCHON_CHECK_GREATER_EQUAL(true, true);
}


// Test #1, accum checks = 0 + 13 = 13
ARCHON_TEST_EX(failure_list, Failure_Bool, true, true)
{
    ARCHON_CHECK(false);
    ARCHON_CHECK_EQUAL(false, true);
    ARCHON_CHECK_EQUAL(true, false);
    ARCHON_CHECK_NOT_EQUAL(false, false);
    ARCHON_CHECK_NOT_EQUAL(true, true);
    ARCHON_CHECK_LESS(false, false);
    ARCHON_CHECK_LESS(true, false);
    ARCHON_CHECK_LESS(true, true);
    ARCHON_CHECK_GREATER(false, false);
    ARCHON_CHECK_GREATER(false, true);
    ARCHON_CHECK_GREATER(true, true);
    ARCHON_CHECK_LESS_EQUAL(true, false);
    ARCHON_CHECK_GREATER_EQUAL(false, true);
}


// Test #2, accum checks = 13 + 12 = 25
ARCHON_TEST_EX(success_list, Success_Int, true, true)
{
    ARCHON_CHECK_EQUAL(1, 1);
    ARCHON_CHECK_EQUAL(2, 2);
    ARCHON_CHECK_NOT_EQUAL(1, 2);
    ARCHON_CHECK_NOT_EQUAL(2, 1);
    ARCHON_CHECK_LESS(1, 2);
    ARCHON_CHECK_GREATER(2, 1);
    ARCHON_CHECK_LESS_EQUAL(1, 1);
    ARCHON_CHECK_LESS_EQUAL(1, 2);
    ARCHON_CHECK_LESS_EQUAL(2, 2);
    ARCHON_CHECK_GREATER_EQUAL(1, 1);
    ARCHON_CHECK_GREATER_EQUAL(2, 1);
    ARCHON_CHECK_GREATER_EQUAL(2, 2);
}


// Test #2, accum checks = 13 + 12 = 25
ARCHON_TEST_EX(failure_list, Failure_Int, true, true)
{
    ARCHON_CHECK_EQUAL(1, 2);
    ARCHON_CHECK_EQUAL(2, 1);
    ARCHON_CHECK_NOT_EQUAL(1, 1);
    ARCHON_CHECK_NOT_EQUAL(2, 2);
    ARCHON_CHECK_LESS(1, 1);
    ARCHON_CHECK_LESS(2, 1);
    ARCHON_CHECK_LESS(2, 2);
    ARCHON_CHECK_GREATER(1, 1);
    ARCHON_CHECK_GREATER(1, 2);
    ARCHON_CHECK_GREATER(2, 2);
    ARCHON_CHECK_LESS_EQUAL(2, 1);
    ARCHON_CHECK_GREATER_EQUAL(1, 2);
}


// Test #3, accum checks = 25 + 32 = 57
ARCHON_TEST_EX(success_list, Success_Float, true, true)
{
    ARCHON_CHECK_EQUAL(3.1, 3.1);
    ARCHON_CHECK_EQUAL(3.2, 3.2);
    ARCHON_CHECK_NOT_EQUAL(3.1, 3.2);
    ARCHON_CHECK_NOT_EQUAL(3.2, 3.1);
    ARCHON_CHECK_LESS(3.1, 3.2);
    ARCHON_CHECK_GREATER(3.2, 3.1);
    ARCHON_CHECK_LESS_EQUAL(3.1, 3.1);
    ARCHON_CHECK_LESS_EQUAL(3.1, 3.2);
    ARCHON_CHECK_LESS_EQUAL(3.2, 3.2);
    ARCHON_CHECK_GREATER_EQUAL(3.1, 3.1);
    ARCHON_CHECK_GREATER_EQUAL(3.2, 3.1);
    ARCHON_CHECK_GREATER_EQUAL(3.2, 3.2);

    double eps = 0.5;
    ARCHON_CHECK_APPROXIMATELY_EQUAL(+0.00, +0.00, eps); // Max error = 0.0
    ARCHON_CHECK_APPROXIMATELY_EQUAL(+1.00, +1.00, eps); // Max error = 0.5
    ARCHON_CHECK_APPROXIMATELY_EQUAL(+0.51, +1.00, eps); // Max error = 0.5
    ARCHON_CHECK_APPROXIMATELY_EQUAL(-1.00, -1.00, eps); // Max error = 0.5
    ARCHON_CHECK_APPROXIMATELY_EQUAL(-1.00, -0.51, eps); // Max error = 0.5

    ARCHON_CHECK_ESSENTIALLY_EQUAL(+0.00, +0.00, eps); // Max error = 0.0
    ARCHON_CHECK_ESSENTIALLY_EQUAL(+1.00, +1.00, eps); // Max error = 0.5
    ARCHON_CHECK_ESSENTIALLY_EQUAL(+1.00, +1.49, eps); // Max error = 0.5
    ARCHON_CHECK_ESSENTIALLY_EQUAL(-1.00, -1.00, eps); // Max error = 0.5
    ARCHON_CHECK_ESSENTIALLY_EQUAL(-1.49, -1.00, eps); // Max error = 0.5

    ARCHON_CHECK_DEFINITELY_LESS(-1.00, +1.00, eps); // Min error = 0.5
    ARCHON_CHECK_DEFINITELY_LESS(+0.00, +1.00, eps); // Min error = 0.5
    ARCHON_CHECK_DEFINITELY_LESS(+0.49, +1.00, eps); // Min error = 0.5
    ARCHON_CHECK_DEFINITELY_LESS(-1.00, -0.00, eps); // Min error = 0.5
    ARCHON_CHECK_DEFINITELY_LESS(-1.00, -0.49, eps); // Min error = 0.5

    ARCHON_CHECK_DEFINITELY_GREATER(+1.00, -1.00, eps); // Min error = 0.5
    ARCHON_CHECK_DEFINITELY_GREATER(+1.00, +0.00, eps); // Min error = 0.5
    ARCHON_CHECK_DEFINITELY_GREATER(+1.00, +0.49, eps); // Min error = 0.5
    ARCHON_CHECK_DEFINITELY_GREATER(-0.00, -1.00, eps); // Min error = 0.5
    ARCHON_CHECK_DEFINITELY_GREATER(-0.49, -1.00, eps); // Min error = 0.5
}


// Test #3, accum checks = 25 + 52 = 77
ARCHON_TEST_EX(failure_list, Failure_Float, true, true)
{
    ARCHON_CHECK_EQUAL(3.1, 3.2);
    ARCHON_CHECK_EQUAL(3.2, 3.1);
    ARCHON_CHECK_NOT_EQUAL(3.1, 3.1);
    ARCHON_CHECK_NOT_EQUAL(3.2, 3.2);
    ARCHON_CHECK_LESS(3.1, 3.1);
    ARCHON_CHECK_LESS(3.2, 3.1);
    ARCHON_CHECK_LESS(3.2, 3.2);
    ARCHON_CHECK_GREATER(3.1, 3.1);
    ARCHON_CHECK_GREATER(3.1, 3.2);
    ARCHON_CHECK_GREATER(3.2, 3.2);
    ARCHON_CHECK_LESS_EQUAL(3.2, 3.1);
    ARCHON_CHECK_GREATER_EQUAL(3.1, 3.2);

    double eps = 0.5;
    ARCHON_CHECK_APPROXIMATELY_EQUAL(-1.00, +1.00, eps); // Max error = 0.5
    ARCHON_CHECK_APPROXIMATELY_EQUAL(+0.00, +1.00, eps); // Max error = 0.5
    ARCHON_CHECK_APPROXIMATELY_EQUAL(+0.49, +1.00, eps); // Max error = 0.5
    ARCHON_CHECK_APPROXIMATELY_EQUAL(-1.00, -0.00, eps); // Max error = 0.5
    ARCHON_CHECK_APPROXIMATELY_EQUAL(-1.00, -0.49, eps); // Max error = 0.5
    ARCHON_CHECK_APPROXIMATELY_EQUAL(+1.00, -1.00, eps); // Max error = 0.5
    ARCHON_CHECK_APPROXIMATELY_EQUAL(+1.00, +0.00, eps); // Max error = 0.5
    ARCHON_CHECK_APPROXIMATELY_EQUAL(+1.00, +0.49, eps); // Max error = 0.5
    ARCHON_CHECK_APPROXIMATELY_EQUAL(-0.00, -1.00, eps); // Max error = 0.5
    ARCHON_CHECK_APPROXIMATELY_EQUAL(-0.49, -1.00, eps); // Max error = 0.5

    ARCHON_CHECK_ESSENTIALLY_EQUAL(-1.00, +1.00, eps); // Max error = 0.5
    ARCHON_CHECK_ESSENTIALLY_EQUAL(+0.00, +1.00, eps); // Max error = 0.0
    ARCHON_CHECK_ESSENTIALLY_EQUAL(+1.00, +1.51, eps); // Max error = 0.5
    ARCHON_CHECK_ESSENTIALLY_EQUAL(-1.00, -0.00, eps); // Max error = 0.0
    ARCHON_CHECK_ESSENTIALLY_EQUAL(-1.51, -1.00, eps); // Max error = 0.5
    ARCHON_CHECK_ESSENTIALLY_EQUAL(+1.00, -1.00, eps); // Max error = 0.5
    ARCHON_CHECK_ESSENTIALLY_EQUAL(+1.00, +0.00, eps); // Max error = 0.0
    ARCHON_CHECK_ESSENTIALLY_EQUAL(+1.51, +1.00, eps); // Max error = 0.5
    ARCHON_CHECK_ESSENTIALLY_EQUAL(-0.00, -1.00, eps); // Max error = 0.0
    ARCHON_CHECK_ESSENTIALLY_EQUAL(-1.00, -1.51, eps); // Max error = 0.5

    ARCHON_CHECK_DEFINITELY_LESS(+0.00, +0.00, eps); // Min error = 0.0
    ARCHON_CHECK_DEFINITELY_LESS(+1.00, +1.00, eps); // Min error = 0.5
    ARCHON_CHECK_DEFINITELY_LESS(+0.51, +1.00, eps); // Min error = 0.5
    ARCHON_CHECK_DEFINITELY_LESS(-1.00, -1.00, eps); // Min error = 0.5
    ARCHON_CHECK_DEFINITELY_LESS(-1.00, -0.51, eps); // Min error = 0.5
    ARCHON_CHECK_DEFINITELY_LESS(+1.00, -1.00, eps); // Min error = 0.5
    ARCHON_CHECK_DEFINITELY_LESS(+1.00, +0.00, eps); // Min error = 0.5
    ARCHON_CHECK_DEFINITELY_LESS(+1.00, +0.49, eps); // Min error = 0.5
    ARCHON_CHECK_DEFINITELY_LESS(-0.00, -1.00, eps); // Min error = 0.5
    ARCHON_CHECK_DEFINITELY_LESS(-0.49, -1.00, eps); // Min error = 0.5

    ARCHON_CHECK_DEFINITELY_GREATER(+0.00, +0.00, eps); // Min error = 0.0
    ARCHON_CHECK_DEFINITELY_GREATER(+1.00, +1.00, eps); // Min error = 0.5
    ARCHON_CHECK_DEFINITELY_GREATER(+0.51, +1.00, eps); // Min error = 0.5
    ARCHON_CHECK_DEFINITELY_GREATER(-1.00, -1.00, eps); // Min error = 0.5
    ARCHON_CHECK_DEFINITELY_GREATER(-1.00, -0.51, eps); // Min error = 0.5
    ARCHON_CHECK_DEFINITELY_GREATER(-1.00, +1.00, eps); // Min error = 0.5
    ARCHON_CHECK_DEFINITELY_GREATER(+0.00, +1.00, eps); // Min error = 0.5
    ARCHON_CHECK_DEFINITELY_GREATER(+0.49, +1.00, eps); // Min error = 0.5
    ARCHON_CHECK_DEFINITELY_GREATER(-1.00, -0.00, eps); // Min error = 0.5
    ARCHON_CHECK_DEFINITELY_GREATER(-1.00, -0.49, eps); // Min error = 0.5
}


// Test #4, accum checks = 57 + 12 = 69
ARCHON_TEST_EX(success_list, Success_String, true, true)
{
    std::string_view s_1 = "";
    const char* s_2 = "x";
    ARCHON_CHECK_EQUAL(s_1, s_1);
    ARCHON_CHECK_EQUAL(s_2, s_2);
    ARCHON_CHECK_NOT_EQUAL(s_1, s_2);
    ARCHON_CHECK_NOT_EQUAL(s_2, s_1);
    ARCHON_CHECK_LESS(s_1, s_2);
    ARCHON_CHECK_GREATER(s_2, s_1);
    ARCHON_CHECK_LESS_EQUAL(s_1, s_1);
    ARCHON_CHECK_LESS_EQUAL(s_1, s_2);
    ARCHON_CHECK_LESS_EQUAL(s_2, s_2);
    ARCHON_CHECK_GREATER_EQUAL(s_1, s_1);
    ARCHON_CHECK_GREATER_EQUAL(s_2, s_1);
    ARCHON_CHECK_GREATER_EQUAL(s_2, s_2);
}


// Test #4, accum checks = 77 + 12 = 89
ARCHON_TEST_EX(failure_list, Failure_String, true, true)
{
    std::string_view s_1 = "";
    const char* s_2 = "x";
    ARCHON_CHECK_EQUAL(s_1, s_2);
    ARCHON_CHECK_EQUAL(s_2, s_1);
    ARCHON_CHECK_NOT_EQUAL(s_1, s_1);
    ARCHON_CHECK_NOT_EQUAL(s_2, s_2);
    ARCHON_CHECK_LESS(s_1, s_1);
    ARCHON_CHECK_LESS(s_2, s_1);
    ARCHON_CHECK_LESS(s_2, s_2);
    ARCHON_CHECK_GREATER(s_1, s_1);
    ARCHON_CHECK_GREATER(s_1, s_2);
    ARCHON_CHECK_GREATER(s_2, s_2);
    ARCHON_CHECK_LESS_EQUAL(s_2, s_1);
    ARCHON_CHECK_GREATER_EQUAL(s_1, s_2);
}


// Test #5, accum checks = 69 + 12 = 81
ARCHON_TEST_EX(success_list, Success_Pointer, true, true)
{
    const char ch[2] = { 1, 0 };
    const char* p_1 = &ch[0];
    const char* p_2 = &ch[1];
    ARCHON_CHECK_EQUAL(p_1, p_1);
    ARCHON_CHECK_EQUAL(p_2, p_2);
    ARCHON_CHECK_NOT_EQUAL(p_1, p_2);
    ARCHON_CHECK_NOT_EQUAL(p_2, p_1);
    ARCHON_CHECK_LESS(p_1, p_2);
    ARCHON_CHECK_GREATER(p_2, p_1);
    ARCHON_CHECK_LESS_EQUAL(p_1, p_1);
    ARCHON_CHECK_LESS_EQUAL(p_1, p_2);
    ARCHON_CHECK_LESS_EQUAL(p_2, p_2);
    ARCHON_CHECK_GREATER_EQUAL(p_1, p_1);
    ARCHON_CHECK_GREATER_EQUAL(p_2, p_1);
    ARCHON_CHECK_GREATER_EQUAL(p_2, p_2);
}


// Test #5, accum checks = 89 + 12 = 101
ARCHON_TEST_EX(failure_list, Failure_Pointer, true, true)
{
    const char ch[2] = { 1, 0 };
    const char* p_1 = &ch[0];
    const char* p_2 = &ch[1];
    ARCHON_CHECK_EQUAL(p_1, p_2);
    ARCHON_CHECK_EQUAL(p_2, p_1);
    ARCHON_CHECK_NOT_EQUAL(p_1, p_1);
    ARCHON_CHECK_NOT_EQUAL(p_2, p_2);
    ARCHON_CHECK_LESS(p_1, p_1);
    ARCHON_CHECK_LESS(p_2, p_1);
    ARCHON_CHECK_LESS(p_2, p_2);
    ARCHON_CHECK_GREATER(p_1, p_1);
    ARCHON_CHECK_GREATER(p_1, p_2);
    ARCHON_CHECK_GREATER(p_2, p_2);
    ARCHON_CHECK_LESS_EQUAL(p_2, p_1);
    ARCHON_CHECK_GREATER_EQUAL(p_1, p_2);
}


// Test #6, accum checks = 81 + 2 = 83
ARCHON_TEST_EX(success_list, Success_Exception, true, true)
{
    ARCHON_CHECK_THROW(throw_foo(), FooException);
    ARCHON_CHECK_THROW(throw_bar(), BarException);
}


// Test #6, accum checks = 101 + 2 = 103
ARCHON_TEST_EX(failure_list, Failure_Exception, true, true)
{
    ARCHON_CHECK_THROW(throw_nothing(), FooException);
    ARCHON_CHECK_THROW(throw_nothing(), BarException);
}




ARCHON_TEST(Check_Basic)
{
    ARCHON_TEST_DIR(dir);
    auto run = [&](const check::TestList& test_list) {
        check::TestConfig config;
        config.num_threads = 1;
        config.test_list = &test_list;
        config.logger = &test_context.logger;
        config.test_file_base_dir = dir;
        return check::run(std::move(config));
    };
    ARCHON_CHECK(run(zero_tests_list));
    ARCHON_CHECK(run(zero_checks_list));
    ARCHON_CHECK(run(one_check_success_list));
    ARCHON_CHECK_NOT(run(one_check_failure_list));
    ARCHON_CHECK(run(one_test_success_list));
    ARCHON_CHECK_NOT(run(one_test_failure_list));
    ARCHON_CHECK(run(few_tests_success_list));
    ARCHON_CHECK_NOT(run(few_tests_failure_list));
    ARCHON_CHECK_NOT(run(mixed_list));
    ARCHON_CHECK(run(success_list));
    ARCHON_CHECK_NOT(run(failure_list));

    check_summary(test_context, zero_tests_list, 0, 0, 0, 0, 0);
    check_summary(test_context, zero_checks_list, 1, 0, 0, 0, 0);
    check_summary(test_context, one_check_success_list, 1, 0, 0, 1, 0);
    check_summary(test_context, one_check_failure_list, 1, 1, 0, 1, 1);
    check_summary(test_context, one_test_success_list, 1, 0, 0, 5, 0);
    check_summary(test_context, one_test_failure_list, 1, 1, 0, 5, 1);
    check_summary(test_context, few_tests_success_list, 3, 0, 0, 13, 0);
    check_summary(test_context, few_tests_failure_list, 3, 1, 0, 13, 1);
    check_summary(test_context, mixed_list, 7, 3, 0, 19, 6);
    check_summary(test_context, success_list, 6, 0, 0, 83, 0);
    check_summary(test_context, failure_list, 6, 6, 0, 103, 103);

    check_filtered_summary(test_context, mixed_list, "- *", 0, 0, 7, 0, 0);
    check_filtered_summary(test_context, mixed_list, "* - *", 0, 0, 7, 0, 0);
    check_filtered_summary(test_context, mixed_list, "", 7, 3, 0, 19, 6);
    check_filtered_summary(test_context, mixed_list, "*", 7, 3, 0, 19, 6);
    check_filtered_summary(test_context, mixed_list, "* -", 7, 3, 0, 19, 6);
    check_filtered_summary(test_context, mixed_list, "-", 7, 3, 0, 19, 6);
    check_filtered_summary(test_context, mixed_list, "Mixed_*", 7, 3, 0, 19, 6);
    check_filtered_summary(test_context, mixed_list, "Mixed_* -", 7, 3, 0, 19, 6);
    check_filtered_summary(test_context, mixed_list, "Mixed_1_X", 1, 0, 6, 4, 0);
    check_filtered_summary(test_context, mixed_list, "Mixed_2_Y", 1, 1, 6, 4, 1);
    check_filtered_summary(test_context, mixed_list, "Mixed_3_X", 1, 0, 6, 0, 0);
    check_filtered_summary(test_context, mixed_list, "Mixed_4_Y", 1, 1, 6, 3, 3);
    check_filtered_summary(test_context, mixed_list, "Mixed_5_X", 1, 1, 6, 4, 2);
    check_filtered_summary(test_context, mixed_list, "Mixed_6_Y", 1, 0, 6, 0, 0);
    check_filtered_summary(test_context, mixed_list, "Mixed_7_Y", 1, 0, 6, 4, 0);
    check_filtered_summary(test_context, mixed_list, "Mixed_*_X", 3, 1, 4, 8, 2);
    check_filtered_summary(test_context, mixed_list, "Mixed_*_Y", 4, 2, 3, 11, 4);
    check_filtered_summary(test_context, mixed_list, "* - Mixed_*_X", 4, 2, 3, 11, 4);
    check_filtered_summary(test_context, mixed_list, "* - Mixed_*_Y", 3, 1, 4, 8, 2);
    check_filtered_summary(test_context, mixed_list, "Mixed_1_X Mixed_3_X Mixed_5_X", 3, 1, 4, 8, 2);
    check_filtered_summary(test_context, mixed_list, "* - Mixed_1_X Mixed_3_X Mixed_5_X", 4, 2, 3, 11, 4);
}


ARCHON_TEST(Check_CrossTypeCompare)
{
    ARCHON_CHECK_EQUAL(static_cast<signed char>(1), static_cast<unsigned char>(1));
    ARCHON_CHECK_EQUAL(static_cast<signed char>(1), static_cast<unsigned short>(1));
    ARCHON_CHECK_EQUAL(static_cast<signed char>(1), static_cast<unsigned int>(1));
    ARCHON_CHECK_EQUAL(static_cast<signed char>(1), static_cast<unsigned long>(1));
    ARCHON_CHECK_EQUAL(static_cast<short>(1), static_cast<unsigned short>(1));
    ARCHON_CHECK_EQUAL(static_cast<short>(1), static_cast<unsigned int>(1));
    ARCHON_CHECK_EQUAL(static_cast<short>(1), static_cast<unsigned long>(1));
    ARCHON_CHECK_EQUAL(static_cast<int>(1), static_cast<unsigned int>(1));
    ARCHON_CHECK_EQUAL(static_cast<int>(1), static_cast<unsigned long>(1));
    ARCHON_CHECK_EQUAL(static_cast<long>(1), static_cast<unsigned long>(1));

    ARCHON_CHECK_NOT_EQUAL(static_cast<signed char>(-1), static_cast<unsigned char>(-1));
    ARCHON_CHECK_NOT_EQUAL(static_cast<signed char>(-1), static_cast<unsigned short>(-1));
    ARCHON_CHECK_NOT_EQUAL(static_cast<signed char>(-1), static_cast<unsigned int>(-1));
    ARCHON_CHECK_NOT_EQUAL(static_cast<signed char>(-1), static_cast<unsigned long>(-1));
    ARCHON_CHECK_NOT_EQUAL(static_cast<short>(-1), static_cast<unsigned short>(-1));
    ARCHON_CHECK_NOT_EQUAL(static_cast<short>(-1), static_cast<unsigned int>(-1));
    ARCHON_CHECK_NOT_EQUAL(static_cast<short>(-1), static_cast<unsigned long>(-1));
    ARCHON_CHECK_NOT_EQUAL(static_cast<int>(-1), static_cast<unsigned int>(-1));
    ARCHON_CHECK_NOT_EQUAL(static_cast<int>(-1), static_cast<unsigned long>(-1));
    ARCHON_CHECK_NOT_EQUAL(static_cast<long>(-1), static_cast<unsigned long>(-1));

    ARCHON_CHECK_LESS(static_cast<signed char>(-1), static_cast<unsigned char>(-1));
    ARCHON_CHECK_LESS(static_cast<signed char>(-1), static_cast<unsigned short>(-1));
    ARCHON_CHECK_LESS(static_cast<signed char>(-1), static_cast<unsigned int>(-1));
    ARCHON_CHECK_LESS(static_cast<signed char>(-1), static_cast<unsigned long>(-1));
    ARCHON_CHECK_LESS(static_cast<short>(-1), static_cast<unsigned short>(-1));
    ARCHON_CHECK_LESS(static_cast<short>(-1), static_cast<unsigned int>(-1));
    ARCHON_CHECK_LESS(static_cast<short>(-1), static_cast<unsigned long>(-1));
    ARCHON_CHECK_LESS(static_cast<int>(-1), static_cast<unsigned int>(-1));
    ARCHON_CHECK_LESS(static_cast<int>(-1), static_cast<unsigned long>(-1));
    ARCHON_CHECK_LESS(static_cast<long>(-1), static_cast<unsigned long>(-1));
}


ARCHON_TEST(Check_SpecialCond_Basics)
{
    ARCHON_TEST_DIR(dir);
    auto check = [&](auto&& func) {
        check::TestList list;
        list.add("TEST", __FILE__, __LINE__, std::move(func));
        bool report_progress = true;
        check::SimpleReporter reporter(report_progress);
        check::TestConfig config;
        config.num_threads = 1;
        config.test_list = &list;
        config.logger = &test_context.logger;
        config.reporter = &reporter;
        config.test_file_base_dir = dir;
        return check::run(std::move(config));
    };

    struct Unordered {
        constexpr bool operator==(Unordered) const noexcept
        {
            return false;
        }
        constexpr bool operator==(int) const noexcept
        {
            return false;
        }
        constexpr auto operator<=>(Unordered) const noexcept
        {
            return std::partial_ordering::unordered;
        }
        constexpr auto operator<=>(int) const noexcept
        {
            return std::partial_ordering::unordered;
        }
    };

    constexpr bool have_nan = std::numeric_limits<double>::has_quiet_NaN;
    constexpr double nan = std::numeric_limits<double>::quiet_NaN();

    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_EQUAL(2, 2);
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_EQUAL(2, 3);
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_EQUAL(2, Unordered());
    }));

    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_NOT_EQUAL(2, 3);
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_NOT_EQUAL(2, 2);
    }));
    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_NOT_EQUAL(2, Unordered());
    }));

    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_LESS(2, 3);
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_LESS(2, 2);
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_LESS(2, Unordered());
    }));

    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_LESS_EQUAL(2, 2);
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_LESS_EQUAL(3, 2);
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_LESS_EQUAL(2, Unordered());
    }));

    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_NOT_LESS(2, 2);
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_NOT_LESS(2, 3);
    }));
    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_NOT_LESS(2, Unordered());
    }));

    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_NOT_LESS_EQUAL(3, 2);
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_NOT_LESS_EQUAL(2, 2);
    }));
    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_NOT_LESS_EQUAL(2, Unordered());
    }));

    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_GREATER(3, 2);
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_GREATER(2, 2);
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_GREATER(2, Unordered());
    }));

    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_GREATER_EQUAL(2, 2);
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_GREATER_EQUAL(2, 3);
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_GREATER_EQUAL(2, Unordered());
    }));

    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_NOT_GREATER(2, 2);
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_NOT_GREATER(3, 2);
    }));
    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_NOT_GREATER(2, Unordered());
    }));

    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_NOT_GREATER_EQUAL(2, 3);
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_NOT_GREATER_EQUAL(2, 2);
    }));
    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_NOT_GREATER_EQUAL(2, Unordered());
    }));

    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_DIST_LESS(2, 3, 2);
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_DIST_LESS(2, 4, 2);
    }));
    if constexpr (have_nan) {
        ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
            ARCHON_CHECK_DIST_LESS(2.0, 2.0, nan);
        }));
    }

    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_DIST_LESS_EQUAL(2, 4, 2);
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_DIST_LESS_EQUAL(2, 5, 2);
    }));
    if constexpr (have_nan) {
        ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
            ARCHON_CHECK_DIST_LESS_EQUAL(2.0, 2.0, nan);
        }));
    }

    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_DIST_NOT_LESS(2, 4, 2);
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_DIST_NOT_LESS(2, 3, 2);
    }));
    if constexpr (have_nan) {
        ARCHON_CHECK(check([&](check::TestContext& test_context) {
            ARCHON_CHECK_DIST_NOT_LESS(2.0, 2.0, nan);
        }));
    }

    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_DIST_NOT_LESS_EQUAL(2, 5, 2);
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_DIST_NOT_LESS_EQUAL(2, 4, 2);
    }));
    if constexpr (have_nan) {
        ARCHON_CHECK(check([&](check::TestContext& test_context) {
            ARCHON_CHECK_DIST_NOT_LESS_EQUAL(2.0, 2.0, nan);
        }));
    }

    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_DIST_GREATER(2, 5, 2);
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_DIST_GREATER(2, 4, 2);
    }));
    if constexpr (have_nan) {
        ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
            ARCHON_CHECK_DIST_GREATER(2.0, 2.0, nan);
        }));
    }

    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_DIST_GREATER_EQUAL(2, 4, 2);
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_DIST_GREATER_EQUAL(2, 3, 2);
    }));
    if constexpr (have_nan) {
        ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
            ARCHON_CHECK_DIST_GREATER_EQUAL(2.0, 2.0, nan);
        }));
    }

    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_DIST_NOT_GREATER(2, 4, 2);
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_DIST_NOT_GREATER(2, 5, 2);
    }));
    if constexpr (have_nan) {
        ARCHON_CHECK(check([&](check::TestContext& test_context) {
            ARCHON_CHECK_DIST_NOT_GREATER(2.0, 2.0, nan);
        }));
    }

    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_DIST_NOT_GREATER_EQUAL(2, 3, 2);
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_DIST_NOT_GREATER_EQUAL(2, 4, 2);
    }));
    if constexpr (have_nan) {
        ARCHON_CHECK(check([&](check::TestContext& test_context) {
            ARCHON_CHECK_DIST_NOT_GREATER_EQUAL(2.0, 2.0, nan);
        }));
    }

    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_BETWEEN(1, 2, 3);
    }));
    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_BETWEEN(2, 2, 3);
    }));
    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_BETWEEN(3, 2, 3);
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_BETWEEN(4, 2, 3);
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_BETWEEN(Unordered(), 2, 3);
    }));

    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_NOT_BETWEEN(1, 2, 3);
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_NOT_BETWEEN(2, 2, 3);
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_NOT_BETWEEN(3, 2, 3);
    }));
    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_NOT_BETWEEN(4, 2, 3);
    }));
    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_NOT_BETWEEN(Unordered(), 2, 3);
    }));

    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_APPROXIMATELY_EQUAL(1, 1, 0);
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_APPROXIMATELY_EQUAL(1, 2, 0);
    }));
    if constexpr (have_nan) {
        ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
            ARCHON_CHECK_APPROXIMATELY_EQUAL(1, nan, 0);
        }));
    }

    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_ESSENTIALLY_EQUAL(1, 1, 0);
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_ESSENTIALLY_EQUAL(1, 2, 0);
    }));
    if constexpr (have_nan) {
        ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
            ARCHON_CHECK_ESSENTIALLY_EQUAL(1, nan, 0);
        }));
    }

    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_NOT_APPROXIMATELY_EQUAL(1, 2, 0);
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_NOT_APPROXIMATELY_EQUAL(1, 1, 0);
    }));
    if constexpr (have_nan) {
        ARCHON_CHECK(check([&](check::TestContext& test_context) {
            ARCHON_CHECK_NOT_APPROXIMATELY_EQUAL(1, nan, 0);
        }));
    }

    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_NOT_ESSENTIALLY_EQUAL(1, 2, 0);
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_NOT_ESSENTIALLY_EQUAL(1, 1, 0);
    }));
    if constexpr (have_nan) {
        ARCHON_CHECK(check([&](check::TestContext& test_context) {
            ARCHON_CHECK_NOT_ESSENTIALLY_EQUAL(1, nan, 0);
        }));
    }

    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_DEFINITELY_LESS(1, 2, 0);
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_DEFINITELY_LESS(1, 1, 0);
    }));
    if constexpr (have_nan) {
        ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
            ARCHON_CHECK_DEFINITELY_LESS(1, nan, 0);
        }));
    }

    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_DEFINITELY_GREATER(2, 1, 0);
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_DEFINITELY_GREATER(1, 1, 0);
    }));
    if constexpr (have_nan) {
        ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
            ARCHON_CHECK_DEFINITELY_GREATER(1, nan, 0);
        }));
    }

    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_NOT_DEFINITELY_LESS(1, 1, 0);
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_NOT_DEFINITELY_LESS(1, 2, 0);
    }));
    if constexpr (have_nan) {
        ARCHON_CHECK(check([&](check::TestContext& test_context) {
            ARCHON_CHECK_NOT_DEFINITELY_LESS(1, nan, 0);
        }));
    }

    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_NOT_DEFINITELY_GREATER(1, 1, 0);
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_NOT_DEFINITELY_GREATER(2, 1, 0);
    }));
    if constexpr (have_nan) {
        ARCHON_CHECK(check([&](check::TestContext& test_context) {
            ARCHON_CHECK_NOT_DEFINITELY_GREATER(1, nan, 0);
        }));
    }

    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_IN(1, 1, 2);
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_IN(1, 2, 3);
    }));
    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_IN(1, 1, Unordered());
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_IN(1, 2, Unordered());
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_IN(Unordered(), 1, 2);
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_IN(Unordered(), 1, Unordered());
    }));

    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_NOT_IN(1, 1, 2);
    }));
    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_NOT_IN(1, 2, 3);
    }));
    ARCHON_CHECK_NOT(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_NOT_IN(1, 1, Unordered());
    }));
    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_NOT_IN(1, 2, Unordered());
    }));
    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_NOT_IN(Unordered(), 1, 2);
    }));
    ARCHON_CHECK(check([&](check::TestContext& test_context) {
        ARCHON_CHECK_NOT_IN(Unordered(), 1, Unordered());
    }));
}


ARCHON_TEST(Check_SpecialCond_ExactlyOneEvaluationOfEachCheckArgument)
{
    ARCHON_TEST_DIR(dir);
    auto run = [&](auto&& func) {
        check::TestList list;
        list.add("TEST", __FILE__, __LINE__, std::move(func));
        bool report_progress = true;
        check::SimpleReporter reporter(report_progress);
        check::TestConfig config;
        config.num_threads = 1;
        config.test_list = &list;
        config.logger = &test_context.logger;
        config.reporter = &reporter;
        config.test_file_base_dir = dir;
        bool success = check::run(std::move(config));
        ARCHON_ASSERT(success);
    };

    auto post_incr = [](auto& var) {
        auto val = var;
        var += 1;
        return val;
    };

    {
        int a = 0, b = 0;
        run([&](check::TestContext& test_context) {
            ARCHON_CHECK_EQUAL(post_incr(a), post_incr(b));
        });
        ARCHON_CHECK_EQUAL(a, 1);
        ARCHON_CHECK_EQUAL(b, 1);
    } {
        int a = 0, b = 1;
        run([&](check::TestContext& test_context) {
            ARCHON_CHECK_NOT_EQUAL(post_incr(a), post_incr(b));
        });
        ARCHON_CHECK_EQUAL(a, 1);
        ARCHON_CHECK_EQUAL(b, 2);
    } {
        int a = 0, b = 1;
        run([&](check::TestContext& test_context) {
            ARCHON_CHECK_LESS(post_incr(a), post_incr(b));
        });
        ARCHON_CHECK_EQUAL(a, 1);
        ARCHON_CHECK_EQUAL(b, 2);
    } {
        int a = 0, b = 0;
        run([&](check::TestContext& test_context) {
            ARCHON_CHECK_LESS_EQUAL(post_incr(a), post_incr(b));
        });
        ARCHON_CHECK_EQUAL(a, 1);
        ARCHON_CHECK_EQUAL(b, 1);
    } {
        int a = 0, b = 0;
        run([&](check::TestContext& test_context) {
            ARCHON_CHECK_NOT_LESS(post_incr(a), post_incr(b));
        });
        ARCHON_CHECK_EQUAL(a, 1);
        ARCHON_CHECK_EQUAL(b, 1);
    } {
        int a = 1, b = 0;
        run([&](check::TestContext& test_context) {
            ARCHON_CHECK_NOT_LESS_EQUAL(post_incr(a), post_incr(b));
        });
        ARCHON_CHECK_EQUAL(a, 2);
        ARCHON_CHECK_EQUAL(b, 1);
    } {
        int a = 1, b = 0;
        run([&](check::TestContext& test_context) {
            ARCHON_CHECK_GREATER(post_incr(a), post_incr(b));
        });
        ARCHON_CHECK_EQUAL(a, 2);
        ARCHON_CHECK_EQUAL(b, 1);
    } {
        int a = 0, b = 0;
        run([&](check::TestContext& test_context) {
            ARCHON_CHECK_GREATER_EQUAL(post_incr(a), post_incr(b));
        });
        ARCHON_CHECK_EQUAL(a, 1);
        ARCHON_CHECK_EQUAL(b, 1);
    } {
        int a = 0, b = 0;
        run([&](check::TestContext& test_context) {
            ARCHON_CHECK_NOT_GREATER(post_incr(a), post_incr(b));
        });
        ARCHON_CHECK_EQUAL(a, 1);
        ARCHON_CHECK_EQUAL(b, 1);
    } {
        int a = 0, b = 1;
        run([&](check::TestContext& test_context) {
            ARCHON_CHECK_NOT_GREATER_EQUAL(post_incr(a), post_incr(b));
        });
        ARCHON_CHECK_EQUAL(a, 1);
        ARCHON_CHECK_EQUAL(b, 2);
    } {
        int a = 0, b = 0, dist = 1;
        run([&](check::TestContext& test_context) {
            ARCHON_CHECK_DIST_LESS(post_incr(a), post_incr(b), post_incr(dist));
        });
        ARCHON_CHECK_EQUAL(a, 1);
        ARCHON_CHECK_EQUAL(b, 1);
        ARCHON_CHECK_EQUAL(dist, 2);
    } {
        int a = 0, b = 0, dist = 0;
        run([&](check::TestContext& test_context) {
            ARCHON_CHECK_DIST_LESS_EQUAL(post_incr(a), post_incr(b), post_incr(dist));
        });
        ARCHON_CHECK_EQUAL(a, 1);
        ARCHON_CHECK_EQUAL(b, 1);
        ARCHON_CHECK_EQUAL(dist, 1);
    } {
        int a = 0, b = 0, dist = 0;
        run([&](check::TestContext& test_context) {
            ARCHON_CHECK_DIST_NOT_LESS(post_incr(a), post_incr(b), post_incr(dist));
        });
        ARCHON_CHECK_EQUAL(a, 1);
        ARCHON_CHECK_EQUAL(b, 1);
        ARCHON_CHECK_EQUAL(dist, 1);
    } {
        int a = 0, b = 1, dist = 0;
        run([&](check::TestContext& test_context) {
            ARCHON_CHECK_DIST_NOT_LESS_EQUAL(post_incr(a), post_incr(b), post_incr(dist));
        });
        ARCHON_CHECK_EQUAL(a, 1);
        ARCHON_CHECK_EQUAL(b, 2);
        ARCHON_CHECK_EQUAL(dist, 1);
    } {
        int a = 0, b = 1, dist = 0;
        run([&](check::TestContext& test_context) {
            ARCHON_CHECK_DIST_GREATER(post_incr(a), post_incr(b), post_incr(dist));
        });
        ARCHON_CHECK_EQUAL(a, 1);
        ARCHON_CHECK_EQUAL(b, 2);
        ARCHON_CHECK_EQUAL(dist, 1);
    } {
        int a = 0, b = 0, dist = 0;
        run([&](check::TestContext& test_context) {
            ARCHON_CHECK_DIST_GREATER_EQUAL(post_incr(a), post_incr(b), post_incr(dist));
        });
        ARCHON_CHECK_EQUAL(a, 1);
        ARCHON_CHECK_EQUAL(b, 1);
        ARCHON_CHECK_EQUAL(dist, 1);
    } {
        int a = 0, b = 0, dist = 0;
        run([&](check::TestContext& test_context) {
            ARCHON_CHECK_DIST_NOT_GREATER(post_incr(a), post_incr(b), post_incr(dist));
        });
        ARCHON_CHECK_EQUAL(a, 1);
        ARCHON_CHECK_EQUAL(b, 1);
        ARCHON_CHECK_EQUAL(dist, 1);
    } {
        int a = 0, b = 0, dist = 1;
        run([&](check::TestContext& test_context) {
            ARCHON_CHECK_DIST_NOT_GREATER_EQUAL(post_incr(a), post_incr(b), post_incr(dist));
        });
        ARCHON_CHECK_EQUAL(a, 1);
        ARCHON_CHECK_EQUAL(b, 1);
        ARCHON_CHECK_EQUAL(dist, 2);
    } {
        int x = 0, min = 0, max = 0;
        run([&](check::TestContext& test_context) {
            ARCHON_CHECK_BETWEEN(post_incr(x), post_incr(min), post_incr(max));
        });
        ARCHON_CHECK_EQUAL(x, 1);
        ARCHON_CHECK_EQUAL(min, 1);
        ARCHON_CHECK_EQUAL(max, 1);
    } {
        int x = 1, min = 0, max = 0;
        run([&](check::TestContext& test_context) {
            ARCHON_CHECK_NOT_BETWEEN(post_incr(x), post_incr(min), post_incr(max));
        });
        ARCHON_CHECK_EQUAL(x, 2);
        ARCHON_CHECK_EQUAL(min, 1);
        ARCHON_CHECK_EQUAL(max, 1);
    } {
        double a = 1, b = 1, eps = 0;
        run([&](check::TestContext& test_context) {
            ARCHON_CHECK_APPROXIMATELY_EQUAL(post_incr(a), post_incr(b), post_incr(eps));
        });
        ARCHON_CHECK_EQUAL(a, 2);
        ARCHON_CHECK_EQUAL(b, 2);
        ARCHON_CHECK_EQUAL(eps, 1);
    } {
        double a = 1, b = 1, eps = 0;
        run([&](check::TestContext& test_context) {
            ARCHON_CHECK_ESSENTIALLY_EQUAL(post_incr(a), post_incr(b), post_incr(eps));
        });
        ARCHON_CHECK_EQUAL(a, 2);
        ARCHON_CHECK_EQUAL(b, 2);
        ARCHON_CHECK_EQUAL(eps, 1);
    } {
        double a = 0, b = 1, eps = 0;
        run([&](check::TestContext& test_context) {
            ARCHON_CHECK_NOT_APPROXIMATELY_EQUAL(post_incr(a), post_incr(b), post_incr(eps));
        });
        ARCHON_CHECK_EQUAL(a, 1);
        ARCHON_CHECK_EQUAL(b, 2);
        ARCHON_CHECK_EQUAL(eps, 1);
    } {
        double a = 0, b = 1, eps = 0;
        run([&](check::TestContext& test_context) {
            ARCHON_CHECK_NOT_ESSENTIALLY_EQUAL(post_incr(a), post_incr(b), post_incr(eps));
        });
        ARCHON_CHECK_EQUAL(a, 1);
        ARCHON_CHECK_EQUAL(b, 2);
        ARCHON_CHECK_EQUAL(eps, 1);
    } {
        double a = 0, b = 1, eps = 0;
        run([&](check::TestContext& test_context) {
            ARCHON_CHECK_DEFINITELY_LESS(post_incr(a), post_incr(b), post_incr(eps));
        });
        ARCHON_CHECK_EQUAL(a, 1);
        ARCHON_CHECK_EQUAL(b, 2);
        ARCHON_CHECK_EQUAL(eps, 1);
    } {
        double a = 1, b = 0, eps = 0;
        run([&](check::TestContext& test_context) {
            ARCHON_CHECK_DEFINITELY_GREATER(post_incr(a), post_incr(b), post_incr(eps));
        });
        ARCHON_CHECK_EQUAL(a, 2);
        ARCHON_CHECK_EQUAL(b, 1);
        ARCHON_CHECK_EQUAL(eps, 1);
    } {
        double a = 1, b = 1, eps = 0;
        run([&](check::TestContext& test_context) {
            ARCHON_CHECK_NOT_DEFINITELY_LESS(post_incr(a), post_incr(b), post_incr(eps));
        });
        ARCHON_CHECK_EQUAL(a, 2);
        ARCHON_CHECK_EQUAL(b, 2);
        ARCHON_CHECK_EQUAL(eps, 1);
    } {
        double a = 1, b = 1, eps = 0;
        run([&](check::TestContext& test_context) {
            ARCHON_CHECK_NOT_DEFINITELY_GREATER(post_incr(a), post_incr(b), post_incr(eps));
        });
        ARCHON_CHECK_EQUAL(a, 2);
        ARCHON_CHECK_EQUAL(b, 2);
        ARCHON_CHECK_EQUAL(eps, 1);
    } {
        int x = 1, a = 1, b = 2;
        run([&](check::TestContext& test_context) {
            ARCHON_CHECK_IN(post_incr(x), post_incr(a), post_incr(b));
        });
        ARCHON_CHECK_EQUAL(x, 2);
        ARCHON_CHECK_EQUAL(a, 2);
        ARCHON_CHECK_EQUAL(b, 3);
    } {
        int x = 1, a = 2, b = 3;
        run([&](check::TestContext& test_context) {
            ARCHON_CHECK_NOT_IN(post_incr(x), post_incr(a), post_incr(b));
        });
        ARCHON_CHECK_EQUAL(x, 2);
        ARCHON_CHECK_EQUAL(a, 3);
        ARCHON_CHECK_EQUAL(b, 4);
    }
}
