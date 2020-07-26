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


/// \file

#ifndef ARCHON__UNIT_TEST__NOINST__TEST_LEVEL_REPORT_LOGGER_HPP
#define ARCHON__UNIT_TEST__NOINST__TEST_LEVEL_REPORT_LOGGER_HPP

#include <string_view>

#include <archon/base/logger.hpp>
#include <archon/unit_test/test_context.hpp>


namespace archon::unit_test::detail {


class TestLevelReportLogger final :
        private base::Logger::Prefix,
        public base::Logger {
public:
    TestContext* test_context;
    std::string_view file_path;
    long line_number;

    TestLevelReportLogger(base::Logger& base_logger) noexcept;

private:
    const base::Logger::Prefix& m_parent_prefix;

    // Overriding functions from Prefix
    void format_prefix(ostream_type&) const override final;
};


} // namespace archon::unit_test::detail

#endif // ARCHON__UNIT_TEST__NOINST__TEST_LEVEL_REPORT_LOGGER_HPP
