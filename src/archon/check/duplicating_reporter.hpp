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

#ifndef ARCHON_X_CHECK_X_DUPLICATING_REPORTER_HPP
#define ARCHON_X_CHECK_X_DUPLICATING_REPORTER_HPP

/// \file


#include <initializer_list>

#include <archon/core/span.hpp>
#include <archon/core/array_seeded_buffer.hpp>
#include <archon/check/reporter.hpp>


namespace archon::check {


/// \brief Drive multiple reporters concurrently.
///
/// Use a duplicating reporter to drive multiple reporters concurrently.
///
class DuplicatingReporter
    : public check::Reporter {
public:
    explicit DuplicatingReporter(check::Reporter& subreporter_1, check::Reporter& subreporter_2);
    explicit DuplicatingReporter(std::initializer_list<check::Reporter*> subreporters);
    explicit DuplicatingReporter(core::Span<check::Reporter* const> subreporters);

    void root_begin(const check::RootContext&) override;
    void thread_begin(const check::ThreadContext&) override;
    void begin(const check::TestContext&, log::Logger&) override;
    void fail(const check::FailContext&, std::string_view, log::Logger&) override;
    void end(const check::TestContext&, double, log::Logger&) override;
    void thread_end(const check::ThreadContext&) override;
    void root_end(const check::RootContext&, const check::Summary&) override;

protected:
    core::ArraySeededBuffer<check::Reporter*, 3> m_subreporters;
};








// Implementation


inline DuplicatingReporter::DuplicatingReporter(check::Reporter& subreporter_1, check::Reporter& subreporter_2)
    : DuplicatingReporter({ &subreporter_1, &subreporter_2 }) // Throws
{
}


inline DuplicatingReporter::DuplicatingReporter(std::initializer_list<check::Reporter*> subreporters)
    : DuplicatingReporter(core::Span(subreporters.begin(), subreporters.end())) // Throws
{
}


inline DuplicatingReporter::DuplicatingReporter(core::Span<check::Reporter* const> subreporters)
    : m_subreporters(core::BufferDataTag(), subreporters) // Throws
{
}


} // namespace archon::check

#endif // ARCHON_X_CHECK_X_DUPLICATING_REPORTER_HPP
