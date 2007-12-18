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

#ifndef ARCHON_UTIL_READLINE_HPP
#define ARCHON_UTIL_READLINE_HPP

#include <stdexcept>
#include <string>


namespace Archon
{
  namespace Util
  {
    /**
     * Functions for reading lines of input from the users terminal
     * optionally allowing for advanced shell-like editing.
     *
     * Advanced shell-like editing will be available when the GNU \c
     * readline library function is available.
     *
     * Thread safety is assured as long as everybody uses 'readline'
     * via this interface. If they do not, anything can happen.
     */
    namespace Readline
    {
      /**
       * Thrown when two threads attempt to call one of the functions
       * in this namespace at the same time.
       */
      struct OccupiedException;


      /**
       * Read another line of input.
       *
       * \param line The entered line will be assigned to this
       * variable.
       *
       * \return True until no more lines are available because
       * end-of-input has been reached.
       *
       * \throw OccupiedException If another thread is currently
       * executing one of the functions in this namespace.
       */
      bool read(std::string &line);


      /**
       * Set the prompt to be displayed whenever a line of input is
       * requested.
       *
       * By default, the prompt is the empty string.
       *
       * \throw OccupiedException If another thread is currently
       * executing one of the functions in this namespace.
       */
      void set_prompt(std::string text);


      /**
       * Disable file name based tab-completion while editing the
       * line.
       *
       * This function must be called prior to any calls to the read()
       * function.
       *
       * This feature may or may not be available by default.
       *
       * \throw OccupiedException If another thread is currently
       * executing one of the functions in this namespace.
       */
      void disable_file_completion();
    }







    // Definitions:

    namespace Readline
    {
      struct OccupiedException: std::runtime_error
      {
        OccupiedException(): std::runtime_error("Readline::read() called by multiple threads") {}
      };
    }
  }
}

#endif // ARCHON_UTIL_READLINE_HPP
