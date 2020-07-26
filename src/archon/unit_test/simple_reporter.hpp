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

#ifndef ARCHON__UNIT_TEST__SIMPLE_REPORTER_HPP
#define ARCHON__UNIT_TEST__SIMPLE_REPORTER_HPP

#include <archon/unit_test/reporter.hpp>


namespace archon::unit_test {


/// \brief Simple reporter.
///
/// This is a simple, minimalistic reporter implementation. Besides logging
/// failures as they occur, it logs the number of testing threads initially,
/// logs a faily detailed summery at the end of the testing process, and
/// optionally logs a message each time a unit test execution begins (when
/// `true` is passed to constrcutor for \p report_progress).
///
/// This class can be used as a base class for more advanced reporters.
///
class SimpleReporter :
        public Reporter {
public:
    explicit SimpleReporter(bool report_progress = false) noexcept;

    void root_begin(const RootContext&) override;
    void thread_begin(const ThreadContext&) override;
    void begin(const TestContext&, base::Logger&) override;
    void fail(const FailContext&, std::string_view, base::Logger&) override;
    void thread_end(const ThreadContext&) override;
    void root_end(const RootContext&, const Summary&) override;

protected:
    const bool m_report_progress;
};








// Implementation


inline SimpleReporter::SimpleReporter(bool report_progress) noexcept :
    m_report_progress(report_progress)
{
}


} // namespace archon::unit_test

#endif // ARCHON__UNIT_TEST__SIMPLE_REPORTER_HPP
