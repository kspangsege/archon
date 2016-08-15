/*
 * This file is part of the Archon library framework.
 *
 * Copyright (C) 2012  Kristian Spangsege <kristian.spangsege@gmail.com>
 *
 * The Archon library framework is free software: You can redistribute
 * it and/or modify it under the terms of the GNU Lesser General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * The Archon library framework is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the Archon library framework.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * \author Kristian Spangsege
 */

#ifndef ARCHON_CORE_LOGGER_HPP
#define ARCHON_CORE_LOGGER_HPP

#include <string>
#include <locale>

#include <archon/core/unique_ptr.hpp>


namespace archon {
namespace core {

// An abstract thread safe logger.
class Logger {
public:
    /// Log a message.
    ///
    /// This method shall always be thread safe.
    ///
    /// \param msg The log messege to output.
    virtual void log(const std::string& msg) = 0;


    /// Get the locale associated with this logger.
    virtual std::locale getloc() const = 0;


    /// Get the default logger that simply writes to STDERR.
    ///
    /// \note Ownership is not transferred to the caller.
    static Logger& get_default_logger();


    /// Construct a logger that writes messages to the specified file
    /// descriptor. To prevent output from multiple processes from
    /// getting entangled this type of logger uses advisory locking
    /// based on the \c flock system call.
    ///
    /// \param fd The file descriptor to which the log messages will
    /// be written and on which to place the advisory lock while
    /// writing.
    ///
    /// \return A new logger object with the specified semantics.
    ///
    /// \note The advisory locking scheme used here requires that all
    /// involved processes adhere to the same locking scheme. If a
    /// process ignores the lock, it is free to write to the same
    /// underlying file in an unsynchronized manner leading
    /// potentially to a garbled result.
    ///
    /// \sa Linux man page for flock(2)
    static UniquePtr<Logger> new_flock_logger(int fd, const std::locale& = std::locale());


    /// Construct a logger that writes messages to the specified
    /// file. It is semanitically equivalent to newFlockLogger(int)
    /// except that it starts out by opening the specified file.
    ///
    /// \param path the path of the log file to open.
    ///
    /// \return A new logger object with the specified semantics.
    ///
    /// \sa new_flock_logger(int)
    static UniquePtr<Logger> new_flock_logger(const std::string& path,
                                              const std::locale& = std::locale());


    /// Construct a logger that adds to each logged message a
    /// timestamp and the process ID of the caller.
    ///
    /// \param superlogger The underlying logger to wrap.
    ///
    /// \return A new logger object whose log method adds time and PID
    /// information.
    static UniquePtr<Logger> new_time_pid_logger(UniquePtr<Logger> superlogger);


    virtual ~Logger() {}
};

} // namespace core
} // namespace archon

#endif // ARCHON_CORE_LOGGER_HPP
