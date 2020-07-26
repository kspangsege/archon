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


#include <archon/unit_test/duplicating_reporter.hpp>


using namespace archon;
using namespace archon::unit_test;


void DuplicatingReporter::root_begin(const RootContext& context)
{
    for (Reporter* r : m_subreporters)
        r->root_begin(context); // Throws
}


void DuplicatingReporter::thread_begin(const ThreadContext& context)
{
    for (Reporter* r : m_subreporters)
        r->thread_begin(context); // Throws
}


void DuplicatingReporter::begin(const TestContext& context, base::Logger& logger)
{
    for (Reporter* r : m_subreporters)
        r->begin(context, logger); // throws
}


void DuplicatingReporter::fail(const FailContext& context, std::string_view message,
                               base::Logger& logger)
{
    for (Reporter* r : m_subreporters)
        r->fail(context, message, logger); // Throws
}


void DuplicatingReporter::end(const TestContext& context, double elapsed_seconds,
                              base::Logger& logger)
{
    for (Reporter* r : m_subreporters)
        r->end(context, elapsed_seconds, logger); // Throws
}


void DuplicatingReporter::thread_end(const ThreadContext& context)
{
    for (Reporter* r : m_subreporters)
        r->thread_end(context); // Throws
}


void DuplicatingReporter::root_end(const RootContext& context, const Summary& summary)
{
    for (Reporter* r : m_subreporters)
        r->root_end(context, summary); // Throws
}
