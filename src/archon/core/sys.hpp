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

#ifndef ARCHON_CORE_SYS_HPP
#define ARCHON_CORE_SYS_HPP

#include <cstddef>
#include <csignal>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <list>
#include <set>

#include <archon/core/time.hpp>
#include <archon/core/blocking.hpp>
#include <archon/core/stream.hpp>


namespace Archon
{
  namespace Core
  {
    /**
     * Provides convenient, portable, and thread safe wrappers for several system features.
     */
    namespace Sys
    {
      /**
       * An instance of this class represents a mutual exclusion in a
       * multi-threaded environment. It is intended to be used when
       * accessing system library functions that are non guaranteed to
       * be thread-safe. It is also usefull for protecting access to
       * 3rd party libraries which could potentially use some of the
       * unsafe system library functions.
       *
       * The environment manipulation functions (getenv(), putenv())
       * use this lock, since the corresponding native system
       * functions are generally not thread-safe.
       */
      struct GlobalLock
      {
        GlobalLock();
        ~GlobalLock();
      };


      /**
       * A thread safe version of strerror() from <ctring>.
       */
      std::string error(int errnum);


      /**
       * A thread-safe version of standard getenv() from <cstdlib>.
       *
       * This function together with setenv() and unsetenv()
       * provides a thread-safe way of accessing the environment by
       * wrapping the standard functions inside a mutex. Note,
       * however, that the thread-safty relies on the fact that all
       * environment access goes through these two functions. This may
       * be hard to ensure if 3rd party libraries are involved. See \c
       * GlobalLock for more details and a possible solution.
       *
       * \param name The name of the environment variable whose value
       * is needed.
       *
       * \return The value of the specified environment variable, or
       * the empty string if the environment does not contain a
       * variable with the specified name.
       *
       * \todo FIXME: On some systems \c getenv and friends may be
       * guaranteed to be thread safe, so it would be good to detect
       * some of those, and skip the locking there.
       */
      std::string getenv(std::string name);

      /**
       * A thread-safe version of setenv() from <cstdlib>.
       *
       * Please see comments in the documentation of getenv().
       *
       * \param name, value The name and value of the environment
       * variable to set.
       */
      void setenv(std::string name, std::string value);

      /**
       * A thread-safe version of unsetenv() from <cstdlib>.
       *
       * Please see comments in the documentation of getenv().
       *
       * \param name The name of the environment variable to be
       * removed from the environment.
       */
      void unsetenv(std::string name);


      /**
       * Get the character encoding specified by the locale of the
       * current environment. If it could not be detected, the empty
       * string will be returned.
       *
       * This function is thread-safe, but the thread-safty relies on
       * the GlobalLock.
       */
      std::string get_env_locale_charenc();


      /**
       * Read from the specified file descriptor.
       */
      std::size_t read(int fd, char *b, std::size_t n) throw(ReadException, InterruptException);

      /**
       * Write to the specified file descriptor.
       */
      std::size_t write(int fd, char const *b, std::size_t n) throw(WriteException, InterruptException);

      /**
       * Close the specified file descriptor.
       */
      void close(int fd) throw(WriteException, InterruptException);


      /**
       * Configure the specified file descriptor for non-blocking I/O.
       */
      void nonblock(int fd);


      /**
       * Convert the calling process into a daemon. This includes the
       * following steps:
       *
       *  - Forking and terminating the parent.
       *  - Become session leader (setsid).
       *  - Change working directory to "/" so we don't keep unintentional resources.
       *  - Reset 'umask'.
       *  - Closing all open file descriptors.
       */
      void daemon_init();


      /**
       * Get the name of the local host as it is known on the network.
       */
      std::string get_hostname();



      /**
       * Functions for working with system signals.
       */
      namespace Signal
      {
        /**
         * Representation of a signal handler.
         */
        struct Handler
        {
          Handler() {}
          Handler(void (*)(int));
          struct sigaction act;
        };

        /**
         * Set a new signal handler for the specified signal. The old
         * handler is returned.
         */
        Handler set_handler(int signal, Handler handler);

        /**
         * Revert to the default behavior for the specified signal. The old
         * handler is returned.
         */
        Handler reset_handler(int signal);

        /**
         * Set a null handler for the specified signal, such that no
         * action is taken upon reception of that signal. The old
         * handler is returned.
         */
        Handler ignore_signal(int signal);

        /**
         * Create an object of this type to block a set of signals
         * during the lifetime of the object. The original signal mask
         * will be automatically restored when the object is
         * destroyed.
         */
        struct Block
        {
          Block(std::set<int> const &signals);
          ~Block();
        private:
          sigset_t original_sigset;
        };

        /**
         * Block the specified set of signals.
         */
        void block(std::set<int> const &signals);

        /**
         * Unblock the specified set of signals.
         */
        void unblock(std::set<int> const &signals);
      }


      /**
       * A flexible utility for constructing shell-like pipelines.
       * That is, sets of commands running in parallel and connected
       * with pipes.
       */
      struct Pipeline
      {
        struct Command
        {
          Command &addArg(std::string arg) { args.push_back(arg); return *this; }
        private:
          friend struct Pipeline;
          Command(std::string cmd): cmd(cmd) {}
          std::string cmd;
          std::vector<std::string> args;
        };

        Command &addCommand(std::string cmd) { cmds.push_back(Command(cmd)); return cmds.back(); }

        enum InputMode  { in_devnull, in_stdin };
        enum OutputMode { out_discard, out_stdout, out_file, out_grab };
        enum ErrorMode  { err_discard, err_stderr };

        void setInputMode(InputMode m)   { inputMode  = m; }
        void setOutputMode(OutputMode m) { outputMode = m; }
        void setErrorMode(ErrorMode m)   { errorMode  = m; }

        void setOutputFile(std::string f) { outputFile = f; }

        std::string run();

        Pipeline(): inputMode(in_stdin), outputMode(out_stdout), errorMode(err_stderr) {}

      private:
        std::list<Command> cmds;
        InputMode  inputMode;
        OutputMode outputMode;
        ErrorMode  errorMode;
        std::string outputFile;
      };
    }
  }
}

#endif // ARCHON_CORE_SYS_HPP
