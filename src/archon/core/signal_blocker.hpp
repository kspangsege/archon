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

#ifndef ARCHON_X_CORE_X_SIGNAL_BLOCKER_HPP
#define ARCHON_X_CORE_X_SIGNAL_BLOCKER_HPP

/// \file


#include <csignal>

#include <archon/core/features.h>


namespace archon::core {


/// \brief Block all system signals.
///
/// While instantiated, this class will block all system signals from being delivered to the
/// instantiating thread.
///
/// On platforms that support POSIX signals, the constructor will set the signal mask such
/// that all signals are blocked from being delivered to the calling thread, and the
/// destructor will restore the signal mask to its original value.
///
/// This scheme assumes that it is always the same thread that constructs and destroys a
/// particular instance of SignalBlocker, and that, for a particular thread, two
/// SignalBlocker objects never overlap in time, and the signal mask is never modified by
/// other means while a SignalBlocker object exists.
///
class SignalBlocker {
public:
    SignalBlocker() noexcept;
    ~SignalBlocker() noexcept;

private:
#if !ARCHON_WINDOWS
    ::sigset_t m_orig_mask;
#endif
};








// Implementation


inline SignalBlocker::SignalBlocker() noexcept
{
#if !ARCHON_WINDOWS
    ::sigset_t mask;
    sigfillset(&mask);
    int ret = ::pthread_sigmask(SIG_BLOCK, &mask, &m_orig_mask);
    ARCHON_ASSERT(ret == 0);
#endif
}


inline SignalBlocker::~SignalBlocker() noexcept
{
#if !ARCHON_WINDOWS
    int ret = ::pthread_sigmask(SIG_SETMASK, &m_orig_mask, nullptr);
    ARCHON_ASSERT(ret == 0);
#endif
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_SIGNAL_BLOCKER_HPP
