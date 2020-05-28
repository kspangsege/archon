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


#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/stream_output.hpp>

#include <archon/check/xml_reporter.hpp>


using namespace archon;


namespace {


struct AsXmlEscaped {
    std::string_view string;
};


inline auto as_xml_escaped(std::string_view string) noexcept -> AsXmlEscaped
{
    return { string };
}


auto operator<<(std::ostream& out, const AsXmlEscaped& pod) -> std::ostream&
{
    std::array<char, 64> seed_memory;
    return core::ostream_sentry(out, [&](core::StreamOutputHelper& helper) {
        for (char ch : pod.string) {
            switch (ch) {
                case '&':
                    helper.write("&amp;"); // Throws
                    continue;
                case '<':
                    helper.write("&lt;"); // Throws
                    continue;
                case '>':
                    helper.write("&gt;"); // Throws
                    continue;
                case '\'':
                    helper.write("&apos;"); // Throws
                    continue;
                case '"':
                    helper.write("&quot;"); // Throws
                    continue;
            }
            helper.put(ch); // Throws
        }
    }, core::Span(seed_memory)); // Throws
}


} // unnamed namespace


using check::XmlReporter;


void XmlReporter::begin(const check::TestContext& context, log::Logger&)
{
    auto key = key_type(context.test_index, context.repetition_no);
    m_tests.emplace(key, Test()); // Throws
}


void XmlReporter::fail(const check::FailContext& context, std::string_view message, log::Logger&)
{
    Failure f;
    f.file_path = context.mapped_file_path;
    f.line_number = context.location.line_number;
    f.message = std::string(message); // Throws
    const check::TestContext& test_context = context.test_context;
    auto key = key_type(test_context.test_index, test_context.repetition_no);
    auto i = m_tests.find(key);
    ARCHON_ASSERT(i != m_tests.end());
    i->second.failures.push_back(f); // Throws
}


void XmlReporter::end(const check::TestContext& context, double elapsed_seconds, log::Logger&)
{
    auto key = key_type(context.test_index, context.repetition_no);
    auto i = m_tests.find(key);
    ARCHON_ASSERT(i != m_tests.end());
    i->second.elapsed_seconds = elapsed_seconds;
}


void XmlReporter::root_end(const check::RootContext& context, const check::Summary& summary)
{
    m_out << "<?xml version=\"1.0\"?>\n"
        "<unittest-results "
        "tests=\"" << summary.num_test_executions << "\" "
        "failedtests=\"" << summary.num_failed_test_executions << "\" "
        "checks=\"" << summary.num_checks << "\" "
        "failures=\"" << summary.num_failed_checks << "\" "
        "time=\"" << summary.elapsed_seconds << "\">\n"; // Throws

    for (const auto& p : m_tests) {
        auto key = p.first;
        const Test& t = p.second;
        std::size_t test_index = key.first;
        int repetition_no = key.second;
        const check::TestDetails& test_details = context.get_test_details(test_index);
        std::string_view test_name = test_details.name;
        if (context.num_repetitions > 1)
            test_name = m_formatter.format("%s#%s", test_name, repetition_no); // Throws

        m_out << "  <test suite=\"" << as_xml_escaped(m_suite_name) << "\" "
            "name=\"" << as_xml_escaped(test_name) << "\" "
            "time=\"" << t.elapsed_seconds << "\""; // Throws
        if (t.failures.empty()) {
            m_out << "/>\n"; // Throws
            continue;
        }
        m_out << ">\n"; // Throws

        for (auto& i_2 : t.failures) {
            m_out << "    <failure message=\"" << i_2.file_path << "(" << i_2.line_number << ") : "
                "" << as_xml_escaped(i_2.message) << "\"/>\n"; // Throws
        }
        m_out << "  </test>\n"; // Throws
    }
    m_out << "</unittest-results>\n"; // Throws
}
