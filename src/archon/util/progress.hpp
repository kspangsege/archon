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

#ifndef ARCHON_UTIL_PROGRESS_HPP
#define ARCHON_UTIL_PROGRESS_HPP

#include<string>
#include<iostream>

#include <archon/core/time.hpp>


namespace archon
{
  namespace Util
  {
    struct ProgressTracker
    {
      /**
       * \param frac The estimated fraction of work completed so far.
       *
       * \paran seconds_left The estimated number of seconds remaining
       * until completion.
       */
      virtual void progress(double frac, double seconds_left) = 0;

      virtual ~ProgressTracker() {}
    };



    /**
     * This class provides a textual progress bar which can be
     * rendered on a text terminal.
     */
    struct ProgressBar: ProgressTracker
    {
      /**
       * \param width The number of characters that should be used to
       * render the progress bar as a line of text. A negative value
       * corresponds to the width of the terminal, or 80 if there is
       * no terminal.
       */
      ProgressBar(int width = -1, std::string prefix = "",
                  std::ostream &out = std::cout);

      ~ProgressBar();

      void progress(double, double);

    private:
      std::string const prefix;
      std::ostream &out;
      int width;
      Core::Time start_time;
    };
  }
}

#endif // ARCHON_UTIL_PROGRESS_HPP
