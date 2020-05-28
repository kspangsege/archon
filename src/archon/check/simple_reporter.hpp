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

#ifndef ARCHON_X_CHECK_X_SIMPLE_REPORTER_HPP
#define ARCHON_X_CHECK_X_SIMPLE_REPORTER_HPP

/// \file


#include <archon/check/reporter.hpp>


namespace archon::check {


/// \brief Simple reporter.
///
/// This is a simple, minimalistic reporter implementation. Besides logging failures as they
/// occur, it logs the number of testing threads initially, logs a faily detailed summery at
/// the end of the testing process, and optionally logs a message each time a test case
/// execution begins (when `true` is passed to constrcutor for \p report_progress).
///
/// This class can be used as a base class for more advanced reporters.
///
class SimpleReporter
    : public check::Reporter {
public:
    explicit SimpleReporter(bool report_progress = false) noexcept;

    void root_begin(const check::RootContext&) override;
    void thread_begin(const check::ThreadContext&) override;
    void begin(const check::TestContext&, log::Logger&) override;
    void fail(const check::FailContext&, std::string_view, log::Logger&) override;
    void thread_end(const check::ThreadContext&) override;
    void root_end(const check::RootContext&, const check::Summary&) override;

protected:
    const bool m_report_progress;
};








// Implementation


inline SimpleReporter::SimpleReporter(bool report_progress) noexcept
    : m_report_progress(report_progress)
{
}


} // namespace archon::check

#endif // ARCHON_X_CHECK_X_SIMPLE_REPORTER_HPP
