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

#include <iostream>
#include <algorithm>
#include <archon/util/logger.hpp>
#include <archon/util/text.hpp>

#include <archon/util/job.hpp>


using namespace archon::util;
using namespace archon::thread;


namespace
{
  struct JobQueueImpl: JobQueue
  {
    JobQueueImpl(int maxThreads = 8, Time maxIdleTime = Time(10.0));

    /**
     * Waits for all jobs to complete.
     */
    ~Queue();

    void add(Ref<Job>);

    /**
     * Send a termination request to this job if it is has started
     * running. Otherwise just remove it from the queue.
     */
    void cancel(Ref<Job>);

    /**
     * Wait for all jobs to complete.
     */
    void wait();

  private:
    const int maxThreads;
    const Time maxIdleTime;

    Mutex mutex;
    Condition newJob;     // Signalled when a new job is added to the queue of pending jobs.
    Condition threadQuit; // Signalled each time a thread quits.
    Condition allDone;    // Signalled when all threads are idle and there is no pending jobs.

    unsigned long nextThreadId;
    list<Ref<Job> > pendingJobs;
    list<unsigned long> idleThreads;
    int activeThreads;
    bool shutdown;

    void worker();
    static void workerEntry(Queue *);
  };
}


namespace archon
{
  namespace utilities
  {
    using namespace std;

    static inline void log(string m)
    {
      Logger::get()->log(m);
    }

    void Job::Queue::worker()
    {
      unsigned long id;

      {
        Mutex::Lock l(mutex);
        id = nextThreadId++;
      }

      //log((string)"Job::Queue::worker(" + Text::toString(id) + "): started");
    
      for(;;)
      {
        Mutex::Lock l(mutex);

        if(pendingJobs.empty())
        {
          --activeThreads;
          idleThreads.push_back(id);

          if(activeThreads == 0) allDone.notifyAll();

          Time wakeup = Time::now();
          wakeup += maxIdleTime;

          for(;;)
          {
            //log((string)"Job::Queue::worker(" + Text::toString(id) + "): going to sleep");

            const bool timedOut = newJob.timedWait(wakeup);

            if(shutdown)
            {
              /*
               * Note that the contents of 'idleThreads' is undefined after
               * 'shutdown' is set to true. The only thing that can be relied on
               * is that the size of the list equals the number of threads that
               * still have not ended their duty.
               */
              idleThreads.pop_back();
              //log((string)"Job::Queue::worker(" + Text::toString(id) + "): terminated");
              threadQuit.notifyAll();
              return;
            }

            if(timedOut)
            {
              idleThreads.erase(find(idleThreads.begin(), idleThreads.end(), id));
              if(!pendingJobs.empty()) break;
              //log((string)"Job::Queue::worker(" + Text::toString(id) + "): timed out");
              threadQuit.notifyAll();
              return;
            }

            if(pendingJobs.empty() || idleThreads.back() != id) continue;
            idleThreads.pop_back();
            break;
          }

          ++activeThreads;
        }

        Ref<Job> job = pendingJobs.front();
        pendingJobs.pop_front();

        job->thread = Thread::self().get();

        l.release();

        //log((string)"Job::Queue::worker(" + Text::toString(id) + "): running new job");
        job->main();

        {
          Mutex::Lock l(mutex);
          job->thread = 0;
        }

        Thread::selfResurrect();
      }
    }

    Job::Queue::Queue(int maxThreads, Time maxIdleTime):
      maxThreads(maxThreads), maxIdleTime(maxIdleTime),
      newJob(mutex), threadQuit(mutex), allDone(mutex),
      nextThreadId(0), activeThreads(0), shutdown(false)
    {
    }

    Job::Queue::~Queue()
    {{ // The extra scope is needed to work around gcc3.2 bug #8287
      Mutex::Lock l(mutex);

      // Wait for all thread to become idle
      while(!pendingJobs.empty() || activeThreads) allDone.wait();

      // Shutdown all threads
      shutdown = true;
      newJob.notifyAll();

      /*
       * Note that the contents of 'idleThreads' is undefined after
       * 'shutdown' is set to true. The only thing that can be relied on
       * is that the size of the list equals the number of threads that
       * still have not ended their duty.
       */
      while(!idleThreads.empty()) threadQuit.wait();
    }}

    void Job::Queue::workerEntry(Job::Queue *q)
    {
      q->worker();
    }

    void Job::Queue::add(Ref<Job> j)
    {
      Mutex::Lock l(mutex);
      if(shutdown) ARCHON_THROW1(StateException, "Shutting down");
      pendingJobs.push_back(j);
      if(idleThreads.empty() && activeThreads < maxThreads)
      {
        ++activeThreads;
        Thread::run(workerEntry, this);
      }
      newJob.notifyAll();
    }

    /**
     * \todo Terminating the thread causes everything to crash
     */
    void Job::Queue::cancel(Ref<Job> j)
    {
      Mutex::Lock l(mutex);
      if(j->thread)
      {
        Ref<Thread> t(j->thread, RefSafeIncTag());
        l.release();
        if(!t) return;
        t->terminate();
      }
      else pendingJobs.remove(j);
    }

    void Job::Queue::wait()
    {
      Mutex::Lock l(mutex);
      while(!pendingJobs.empty() || activeThreads > 0) allDone.wait();
    }
  }
}
