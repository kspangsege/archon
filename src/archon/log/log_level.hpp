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

#ifndef ARCHON_X_LOG_X_LOG_LEVEL_HPP
#define ARCHON_X_LOG_X_LOG_LEVEL_HPP

/// \file


#include <archon/core/enum.hpp>


namespace archon::log {


/// \brief Available log levels.
///
/// This is the list of available log levels. When passed to \ref log::BasicLogger::log(),
/// one should think of it as a verbosity level, or inverse criticality level. When returned
/// from \ref log::BasicLogger::get_log_level_limit(), it should be thought of as a limit on
/// verbosity (threshold on criticality).
///
/// A specialization of \ref core::EnumTraits is provided, making stream input and output
/// available.
///
enum class LogLevel {
    /// Do not log anything. This level should only be used as a limit, never as a level
    /// passed to \ref log::BasicLogger::log().
    off,

    /// Be silent except when there is a fatal error.
    fatal,

    /// Be silent except when there is an error.
    error,

    /// Be silent except when there is an error or a warning.
    warn,

    /// Reveal information about what is going on, but in a minimalistic fashion to avoid
    /// general overhead from logging, and to ease the burden on readers.
    info,

    /// Like `info`, but prioritize completeness over minimalism.
    detail,

    /// Reveal information that can aid debugging, no longer paying attention to efficiency.
    debug,

    /// A version of `debug` that allows for a very high volume of output.
    trace,

    /// Log everything. This level should only be used as a limit, never as a level passed
    /// to \ref log::BasicLogger::log().
    all
};


/// \brief Check log level value validity.
///
/// Returns true if, and only if the specifed integer value corresponds to one of the named
/// values in LogLevel.
///
bool is_valid_log_level(int value);








// Implementation


} // namespace archon::log

namespace archon::core {

template<> struct EnumTraits<log::LogLevel> {
    static constexpr bool is_specialized = true;
    struct Spec {
        static constexpr core::EnumAssoc map[] = {
            { int(log::LogLevel::off),    "off"    },
            { int(log::LogLevel::fatal),  "fatal"  },
            { int(log::LogLevel::error),  "error"  },
            { int(log::LogLevel::warn),   "warn"   },
            { int(log::LogLevel::info),   "info"   },
            { int(log::LogLevel::detail), "detail" },
            { int(log::LogLevel::debug),  "debug"  },
            { int(log::LogLevel::trace),  "trace"  },
            { int(log::LogLevel::all),    "all"    }
        };
    };
    static constexpr bool ignore_case = false;
};

} // namespace archon::core

#endif // ARCHON_X_LOG_X_LOG_LEVEL_HPP
