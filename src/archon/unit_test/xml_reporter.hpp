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

#ifndef ARCHON_X_UNIT_TEST_X_XML_REPORTER_HPP
#define ARCHON_X_UNIT_TEST_X_XML_REPORTER_HPP

/// \file


#include <cstddef>
#include <utility>
#include <string_view>
#include <string>
#include <vector>
#include <map>
#include <ostream>

#include <archon/base/string_formatter.hpp>
#include <archon/unit_test/reporter.hpp>


namespace archon::unit_test {


/// \brief Produce report in XML format.
///
/// This type of reporter generates output that is compatible with the XML
/// output of UnitTest++.
///
class XmlReporter :
        public Reporter {
public:
    explicit XmlReporter(std::ostream&, std::string_view suite_name = "default");

    void begin(const TestContext&, base::Logger&) override;
    void fail(const FailContext&, std::string_view, base::Logger&) override;
    void end(const TestContext&, double, base::Logger&) override;
    void root_end(const RootContext&, const Summary&) override;

protected:
    struct Failure {
        std::string_view file_path;
        long line_number;
        std::string message;
    };

    struct Test {
        std::vector<Failure> failures;
        double elapsed_seconds = 0;
    };

    std::string_view m_suite_name;
    using key_type = std::pair<std::size_t, int>; // (test index, recurrence index)
    std::map<key_type, Test> m_tests;

    std::ostream& m_out;
    base::StringFormatter m_formatter;
};








// Implementation


inline XmlReporter::XmlReporter(std::ostream& out, std::string_view suite_name) :
    m_suite_name(suite_name),
    m_out(out),
    m_formatter(m_out.getloc()) // Throws
{
}


} // namespace archon::unit_test

#endif // ARCHON_X_UNIT_TEST_X_XML_REPORTER_HPP
