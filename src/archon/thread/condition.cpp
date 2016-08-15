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

#include <sys/select.h>
#include <unistd.h>

#include <cerrno>
#include <ctime>
#include <cstdlib>
#include <csignal>
#include <cstring>
#include <stdexcept>
#include <iostream>

#include <archon/core/assert.hpp>
#include <archon/core/sys.hpp>
#include <archon/thread/condition.hpp>
#include <archon/thread/thread.hpp>


using namespace std;
using namespace archon::core;
using namespace archon::Thread;


namespace
{
  inline void write_fdset(set<int> const &s, fd_set &fds, int &max_fd)
  {
    FD_ZERO(&fds);
    set<int>::const_iterator const e = s.end();
    for(set<int>::const_iterator i=s.begin(); i!=e; ++i)
    {
      if(*i < 0 || FD_SETSIZE <= *i) throw invalid_argument("File descriptor out of range");
      FD_SET(*i, &fds);
      if(max_fd < *i) max_fd = *i;
    }
  }


  inline void read_fdset(fd_set const &fds, set<int> const &in, set<int> &out)
  {
    set<int>::const_iterator const e = in.end();
    for(set<int>::const_iterator i=in.begin(); i!=e; ++i) if(FD_ISSET(*i, &fds)) out.insert(*i);
  }


  template<class T> struct LockedTempSetInserter
  {
    LockedTempSetInserter(Mutex const &m, set<T> &c, T e): mutex(m), cont(c), elem(e)
    {
      Mutex::Lock l(mutex);
      cont.insert(elem);
    }

    ~LockedTempSetInserter() throw()
    {
      Mutex::Lock l(mutex);
      cont.erase(elem);
    }

  private:
    Mutex const &mutex;
    set<T> &cont;
    T const elem;
  };


  void clear_pipe(int pipe_read) throw()
  {
    size_t const buf_size = 16;
    char buf[buf_size];
    for(;;)
    {
      ssize_t const m = read(pipe_read, buf, buf_size);
      if(0 < m)
      {
        if(m < ssize_t(buf_size)) return;
      }
      else if(m < 0)
      {
        int const errnum = errno;
        if(errnum == EAGAIN) return;
        if(errnum != EINTR)
          throw runtime_error("Read from pipe failed: "+Sys::error(errnum));
      }
      else throw runtime_error("Write end of pipe was unexpectedly closed");
    }
  }
}


namespace archon
{
  namespace Thread
  {
    void SelectSpec::open_pipe()
    {
      if(pipe(pipe_fds) < 0)
      {
        int const errnum = errno;
        throw runtime_error("Failed to create pipe: "+Sys::error(errnum));
      }
      Sys::nonblock(pipe_fds[0]); // Read end of pipe
      Sys::nonblock(pipe_fds[1]); // Write end of pipe
      prepped = true;
    }

    void SelectSpec::close_pipe()
    {
      close(pipe_fds[0]);
      close(pipe_fds[1]);
    }




    Condition::SimpleCond::~SimpleCond()
    {
        int ret = pthread_cond_destroy(&pthread_cond);
        ARCHON_ASSERT(ret == 0);
    }


    bool Condition::SimpleCond::wait(pthread_mutex_t &m, Time const &timeout)
    {
      int e;
      if(timeout)
      {
        timespec ts;
        ts.tv_sec  = timeout.get_as_seconds();
        ts.tv_nsec = timeout.get_nanos_part();
        e = pthread_cond_timedwait(&pthread_cond, &m, &ts);
        if(e == ETIMEDOUT) return true;
      }
      else e = pthread_cond_wait(&pthread_cond, &m);
      if(e) throw runtime_error("Attempt to wait on condition failed");
      return false;
    }




    struct Condition::ConditionPublisher
    {
      ConditionPublisher(Thread *t, Condition *c): thread(t)
      {
        Mutex::Lock l(thread->mutex);
        if(thread->interrupted)
        {
          thread->interrupted = false;
          throw InterruptException();
        }
        thread->current_wait_cond = c;
      }

      ~ConditionPublisher() throw()
      {
        if(thread->current_wait_cond)
        {
          Mutex::Lock l(thread->mutex);
          retract_helper();
        }
      }

      void retract()
      {
        Mutex::Lock l(thread->mutex);
        retract_helper();
        if(thread->interrupted)
        {
          thread->interrupted = false;
          throw InterruptException();
        }
      }

    private:
      void retract_helper() throw()
      {
        while(thread->interrupting) thread->interrupting_cond.wait(thread->mutex.mutex);
        thread->current_wait_cond = 0;
      }

      Thread *const thread;
    };


    bool Condition::wait(Time timeout)
    {
      Thread::Ref const self = Thread::self();
      ConditionPublisher p(self.get(), this); // May throw InterruptException
      bool const timed_out = simple_cond.wait(mutex.mutex, timeout);
      p.retract(); // May throw InterruptException
      return timed_out;
    }


    struct Condition::MutexUnlocker
    {
      MutexUnlocker(Mutex &m): mutex(m)
      {
        mutex.unlock();
      }

      ~MutexUnlocker() throw()
      {
        mutex.lock(false);
      }

    private:
      Mutex &mutex;
    };


    bool Condition::select(SelectSpec &s, Time timeout)
    {
      s.read_out.clear();
      s.write_out.clear();
      s.except_out.clear();
      s.prep(); // Create pipe if not done already

      bool const r = !s.read_in.empty(), w = !s.write_in.empty(), e = !s.except_in.empty();
      int const pipe_read = s.pipe_fds[0];
      int max_fd = pipe_read;
      fd_set rfds, wfds, efds;
      if(r) write_fdset(s.read_in,   rfds, max_fd);
      if(w) write_fdset(s.write_in,  wfds, max_fd);
      if(e) write_fdset(s.except_in, efds, max_fd);
      FD_SET(pipe_read, &rfds);

      int n;
      try
      {
        Thread::Ref const self = Thread::self();
        LockedTempSetInserter<int> t(pipes_mutex, pipes, s.pipe_fds[1]);
        ConditionPublisher p(self.get(), this); // May throw InterruptException
        {
          MutexUnlocker u(mutex);
          for(;;)
          {
            timeval time;
            bool has_time = false;
            if(timeout)
            {
              timeout -= Time::now();
              if(Time(0) < timeout)
              {
                time.tv_sec  = timeout.get_as_seconds();
                time.tv_usec = (timeout.get_nanos_part() + 999) / 1000;
                has_time = true;
              }
            }
            n = ::select(max_fd+1, r ? &rfds : 0, w ? &wfds : 0, e ? &efds : 0,
                         has_time ? &time : 0);
            if(0 <= n) break;
            int errnum = errno;
            if(errnum != EINTR)
              throw runtime_error("'select' failed: "+Sys::error(errnum));
          }
        }

        p.retract(); // May throw InterruptException
      }
      catch(InterruptException &)
      {
        clear_pipe(pipe_read);
        throw;
      }

      if(n)
      {
        if(r) read_fdset(rfds, s.read_in,   s.read_out);
        if(w) read_fdset(wfds, s.write_in,  s.write_out);
        if(e) read_fdset(efds, s.except_in, s.except_out);
        if(FD_ISSET(pipe_read, &rfds)) clear_pipe(pipe_read);
        return false;
      }

      return true;
    }


    void Condition::pipes_notify()
    {
      static char const zero = 0;
      set<int>::const_iterator const e = pipes.end();
      for(set<int>::const_iterator i=pipes.begin(); i!=e; ++i)
      {
        for(;;)
        {
          if(write(*i, &zero, 1) != -1) break;
          int errnum = errno;
          if(errnum == EINTR) continue;
          throw runtime_error("Write on pipe failed: "+Sys::error(errnum));
        }
      }
    }
  }
}
