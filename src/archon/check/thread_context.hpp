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

#ifndef ARCHON_X_CHECK_X_THREAD_CONTEXT_HPP
#define ARCHON_X_CHECK_X_THREAD_CONTEXT_HPP

/// \file


#include <archon/log/logger.hpp>
#include <archon/check/root_context.hpp>


namespace archon::check {


/// \brief Provide thread-specific context to executing test cases.
///
/// This is the part of the test case execution context that is specific to a particular
/// testing thread. The root context is accessible via \ref root_context.
///
class ThreadContext {
public:
    /// \brief Root-level execution context.
    ///
    /// This is the part of the execution context that is shared by all test case executions
    /// happening on behalf of a particular execution of \ref check::run().
    ///
    const check::RootContext& root_context;

    /// \brief Index of this test thread.
    ///
    /// The index of the test thread associated with this
    /// context. `root_context.num_threads` specifies the total number of test threads.
    ///
    const int thread_index;

    /// \brief Thread-specific logger to be used by customer reporters.
    ///
    /// Do not use this logger inside your test cases. Instead use \ref TestContext::logger
    /// there.
    ///
    /// This is the thread specific logger to be used by custom reporters (\ref
    /// Reporter). See also \ref RootContext::report_logger.
    ///
    log::Logger& report_logger;

    ThreadContext(const ThreadContext&) = delete;
    auto operator=(const ThreadContext&) -> ThreadContext& = delete;

protected:
    ThreadContext(check::RootContext&, int thread_index, log::Logger&) noexcept;
};








// Implementation


inline ThreadContext::ThreadContext(check::RootContext& rc, int ti, log::Logger& rl) noexcept
    : root_context(rc)
    , thread_index(ti)
    , report_logger(rl)
{
}


} // namespace archon::check

#endif // ARCHON_X_CHECK_X_THREAD_CONTEXT_HPP
