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

#ifndef ARCHON_X_CORE_X_THREAD_GUARD_HPP
#define ARCHON_X_CORE_X_THREAD_GUARD_HPP

/// \file


#include <exception>
#include <utility>
#include <string>

#include <archon/core/demangle.hpp>
#include <archon/core/signal_blocker.hpp>
#include <archon/core/thread.hpp>


namespace archon::core {


/// \brief Execute a function using a "managed thread".
///
/// A thread guard object manages a thread of execution (an instance of `std::thread`). The
/// constructor launches the thread and makes it execute a specified function. The
/// destructor waits for execution to complete, either sucessfully, or through the throwing
/// of an exception.
///
/// If the executed function throws an exception, that exception is remembered and can be
/// rethrown by having the parent thread call \ref join_and_rethrow().
///
class ThreadGuard {
public:
    class ThreadName;
    struct Config;

    /// \{
    ///
    /// \brief Execute function by managed thread.
    ///
    /// These constructors launch a new thread, and make it thread execute the specified
    /// function. The life of the thread guard object is guaranteed to not end before the
    /// execution of the function completes. This guarantee is achived by having the
    /// destructor wait for the thread to exit.
    ///
    template<class F> explicit ThreadGuard(F&& func);
    template<class F> ThreadGuard(F&& func, Config);
    /// \}

    /// \brief Wait for execution to complete.
    ///
    /// Wait for the managed thread to exit, either successfully, or through the throwing of
    /// an exception. If the thread has already exited, \ref join() returns immediately.
    ///
    /// Calling \ref join() after a successful invocation of \ref join() has no effect
    /// (idempotency).
    ///
    /// If the thread exited by the throwing of an exception, that exception is remembered,
    /// and can still be rethrown by a subsequent invocation of \ref join_and_rethrow().
    ///
    void join();

    /// \brief Wait, then rethrow exception thrown in thread.
    ///
    /// Wait for the managed thread to exit, then rethrow any exception that was thrown by
    /// the executed function. Waiting occurs as if by an invocation of \ref join().
    ///
    /// Calling \ref join_and_rethrow() after having called \ref join() will still work,
    /// i.e., it will still rethrow any exception that was thrown by the executed function.
    ///
    /// \ref join_and_rethrow() may be called multiple times, but at most one of those
    /// invocations will lead to the rethrowing of an exception that was thrown by the
    /// executed thread (the first invocation that does not fail while waiting for the
    /// thread to exit).
    ///
    void join_and_rethrow();

    /// \brief Construct degenerate thread guard.
    ///
    /// A default constructed thread guard object is in a degenerate state. In this state,
    /// \ref join() and \ref join_and_rethrow() have no effect.
    ///
    ThreadGuard() noexcept = default;

    /// \brief Destroy thread guard after waiting for execution to complete.
    ///
    /// The destruction of a thread guard object involves waiting for the managed thread to
    /// exit. This happens as if by an invocation of \ref join(). If the joining operation
    /// fails the program will be terminated, i.e., in cases where \ref join() would have
    /// thrown. Fortunately, on most platforms, the joining operation will not fail in
    /// practice.
    ///
    ~ThreadGuard() noexcept;

    /// \{
    ///
    /// \brief Thread guard obejcts are movable.
    ///
    /// A moved-from thread guard object is in the same degenerate state as a default
    /// constructed object.
    ///
    /// Assigning to a thread guard object involves waiting for the thread managed by the
    /// assigned-to object to exit. This happens as if by an invocation of \ref join(). If
    /// the joining operation fails the program will be terminated, i.e., in cases where
    /// \ref join() would have thrown. Fortunately, on most platforms, the joining operation
    /// will not fail in practice.
    ///
    ThreadGuard(ThreadGuard&&) noexcept = default;
    auto operator=(ThreadGuard&&) noexcept -> ThreadGuard&;
    /// \}

private:
    std::thread m_thread;
    std::exception_ptr m_exception;
};



class ThreadGuard::ThreadName {
public:
    explicit ThreadName(std::string, const std::locale& = {}) noexcept;

private:
    std::string m_name;
    std::locale m_locale;

    friend class ThreadGuard;
};



/// \brief Thread creation configuration parameters.
///
/// Configuration parameters controlling the creation of the new thread.
///
struct ThreadGuard::Config {
    /// \ref Name of new thread.
    ///
    /// The name to be assigned to the new thread. The name is assigned as if by \ref
    /// core::set_thread_name().
    ///
    /// If no name is specified, the new thread inherits the name of the parent.
    ///
    std::optional<ThreadName> thread_name;

    /// \brief Block signals from new thread.
    ///
    /// Block delivery of POSIX system signals to the new thread. The blocking of signals is
    /// done as if by \ref SignalBlocker. To ensure that the signals are blocked from the
    /// beginning of the life of the new thread, the signals are blocked in the parent
    /// thread while the new thread is created, which causes the signal blocking to be
    /// inherited by the new thread.
    ///
    bool block_signals = false;
};








// Implementation


template<class F> inline ThreadGuard::ThreadGuard(F&& func)
    : ThreadGuard(std::move(func), {}) // Throws
{
}


template<class F> inline ThreadGuard::ThreadGuard(F&& func, Config config)
{
    auto run = [this, thread_name = std::move(config.thread_name),
                func = std::move(func)]() noexcept {
        try {
            if (thread_name.has_value())
                core::set_thread_name(thread_name->m_name, thread_name->m_locale); // Throws
            func(); // Throws
        }
        catch (...) {
            m_exception = std::current_exception();
        }
    };
    if (config.block_signals) {
        SignalBlocker sb;
        m_thread = std::thread(std::move(run)); // Throws
    }
    else {
        m_thread = std::thread(std::move(run)); // Throws
    }
}


inline void ThreadGuard::join()
{
    if (m_thread.joinable())
        m_thread.join(); // Throws
}


inline void ThreadGuard::join_and_rethrow()
{
    join(); // Throws
    if (m_exception) {
        std::exception_ptr exception = std::move(m_exception);
        std::rethrow_exception(exception); // Throws
    }
}


inline ThreadGuard::~ThreadGuard() noexcept
{
    join(); // Throws
}


inline auto ThreadGuard::operator=(ThreadGuard&& other) noexcept -> ThreadGuard&
{
    join(); // Throws
    m_thread    = std::move(other.m_thread);
    m_exception = std::move(other.m_exception);
    return *this;
}


inline ThreadGuard::ThreadName::ThreadName(std::string name, const std::locale& locale) noexcept
    : m_name(std::move(name))
    , m_locale(locale)
{
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_THREAD_GUARD_HPP
