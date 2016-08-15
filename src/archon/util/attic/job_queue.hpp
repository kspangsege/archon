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

/*
 * This file is part of ArchonJS, the Archon JavaScript library.
 *
 * Copyright (C) 2008  Kristian Spangsege <kristian.spangsege@gmail.com>
 *
 * ArchonJS is free software: You can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * ArchonJS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with ArchonJS.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * \author Kristian Spangsege
 */

#ifndef ARCHON_UTIL_JOB_QUEUE_HPP
#define ARCHON_UTIL_JOB_QUEUE_HPP

#include <string>
#include <queue>
#include <list>
#include <deque>

#include <archon/util/thread.hpp>

namespace archon
{
  namespace Utilities
  {
    struct JobQueue
    {
      static Core::UniqueRef<JobQueue> new_queue(int max_threads = 4,
                                                 long max_idle_millis = 10000);


      /**
       * An chunk of work that can be added to a JobQueue.
       */
      struct Job: virtual RefObjectBase
      {
        virtual void main() = 0;

        virtual ~Job() {}
      };


      void submit_job(Ref<Job>);

      /**
       * Send a termination request to this job if it is has started
       * running. Otherwise just remove it from the queue.
       */
      void cancel(Ref<Job>);

      /**
       * Wait for all currently submitted jobs to complete.
       */
      void wait();
    };
  }
}

#endif // ARCHON_UTILITIES_JOB_HPP
