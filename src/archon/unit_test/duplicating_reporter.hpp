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

#ifndef ARCHON_X_UNIT_TEST_X_DUPLICATING_REPORTER_HPP
#define ARCHON_X_UNIT_TEST_X_DUPLICATING_REPORTER_HPP

/// \file


#include <initializer_list>

#include <archon/base/span.hpp>
#include <archon/base/seed_memory_buffer.hpp>
#include <archon/unit_test/reporter.hpp>


namespace archon::unit_test {


/// \brief Drive multiple reporters concurrently.
///
/// Use a duplicating reporter to drive multiple reporters concurrently.
///
class DuplicatingReporter :
        public Reporter {
public:
    explicit DuplicatingReporter(Reporter& subreporter_1, Reporter& subreporter_2);
    explicit DuplicatingReporter(std::initializer_list<Reporter*> subreporters);
    explicit DuplicatingReporter(base::Span<Reporter* const> subreporters);

    void root_begin(const RootContext&) override;
    void thread_begin(const ThreadContext&) override;
    void begin(const TestContext&, base::Logger&) override;
    void fail(const FailContext&, std::string_view, base::Logger&) override;
    void end(const TestContext&, double, base::Logger&) override;
    void thread_end(const ThreadContext&) override;
    void root_end(const RootContext&, const Summary&) override;

protected:
    base::ArraySeededBuffer<Reporter*, 3> m_subreporters;
};








// Implementation


inline DuplicatingReporter::DuplicatingReporter(Reporter& subreporter_1, Reporter& subreporter_2) :
    DuplicatingReporter({ &subreporter_1, &subreporter_2 }) // Throws
{
}


inline DuplicatingReporter::DuplicatingReporter(std::initializer_list<Reporter*> subreporters) :
    DuplicatingReporter(base::Span(subreporters.begin(), subreporters.end())) // Throws
{
}


inline DuplicatingReporter::DuplicatingReporter(base::Span<Reporter* const> subreporters) :
    m_subreporters(subreporters) // Throws
{
}


} // namespace archon::unit_test

#endif // ARCHON_X_UNIT_TEST_X_DUPLICATING_REPORTER_HPP
