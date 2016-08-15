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

#ifndef ARCHON_UTIL_FILESYS_LISTEN_HPP
#define ARCHON_UTIL_FILESYS_LISTEN_HPP

#include <string>

#include <archon/core/unique_ptr.hpp>
#include <archon/core/time.hpp>


namespace archon
{
  namespace Util
  {
    /**
     * Listen for creation of new directory entries. Any such event
     * can be waited for with the 'wait' method. The occurance of
     * such events will be recorded by this class even when no
     * process is waiting (calling the 'wait' method). Use the
     * 'clear' method if you want to forget about past events.
     */
    struct FileSystemListener
    {
      /**
       * On some systems it may not be possible to listen for file
       * system events. In such cases this function must return false,
       * and the wait() method must behave as a simple timed wait
       * operation.
       */
      static bool is_supported();

      /**
       * Wait for the first of the following events to occur: The
       * occurance of a file system event, the reaching of 'timeout'.
       *
       * \param timeout Waiting will not continue beyond this point in
       * time. If zero, the timeout event will never occur. If timeout
       * is less than 'now', but not zero, this call will simply poll
       * for a previously occured event.
       *
       * \return True if the timeout was reached.
       *
       * \throw core::InterruptException If the waiting
       * thread is interrupted.
       */
      virtual bool wait(core::Time timeout = 0) = 0;

      /**
       * Forget about past events.
       */
      virtual void clear() = 0;

      static core::UniquePtr<FileSystemListener> new_listener(std::string path);

      virtual ~FileSystemListener() {}
    };
  }
}

#endif // ARCHON_UTIL_FILESYS_LISTEN_HPP
