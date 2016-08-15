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

#ifndef ARCHON_THREAD_CONDITION_HPP
#define ARCHON_THREAD_CONDITION_HPP

#include <set>

#include <archon/core/blocking.hpp>
#include <archon/core/mutex.hpp>
#include <archon/core/time.hpp>


namespace archon
{
  namespace Thread
  {
    struct SelectSpec;


    /**
     * A condition variable for implementing thread synchronization.
     *
     * The function of a condition variable is to put threads to sleep
     * while they wait for some condition to be met. Condition
     * variables are almost always used as part of a
     * monitor. Conceptually a monitor consist of four things; a state
     * comprised of one or more state variables, a mutex that protects
     * the state, a number of methods that operate on the state, and
     * finally one or more condition variables. A thread is said to be
     * inside the monitor if it is executing one of its methods,
     * although not if it is asleep waiting on a condition variable.
     * A key point is that at most one thread at a time can be inside
     * the monitor. A thread that attempts to enter the monitor is
     * made to wait until the monitor is free. This is also true for a
     * thread that wishes to return from a wait on a condition
     * variable.
     *
     * In C++ a monitor can be conveniently expressed as a class with
     * condition variables expressed by this \c Condition class and
     * using the companion \c Core::Mutex class for mutual
     * exclusion. Here is an example using one condition variable:
     *
     * <pre>
     *
     *   struct QueueMonitor
     *   {
     *     int get()
     *     {
     *       Core::Mutex::Lock l(mutex);
     *       while(q.empty()) non_empty.wait(); // Keep waiting until
     *                                          // the condition is satisfied.
     *       int i = q.front();
     *       q.pop();
     *       return i;
     *     }
     *
     *     void put(int i)
     *     {
     *       Core::Mutex::Lock l(mutex);
     *       q.push(i);
     *       non_empty.notify_all();            // Wake up waiting threads
     *     }
     *
     *     QueueMonitor(): non_empty(mutex) {}  // Condition is bound to mutex.
     *
     *   private:
     *     Core::Mutex mutex;
     *     std::queue<int> q;
     *     Condition non_empty;
     *   }
     *
     * </pre>
     *
     * Note that the condition variable represents a definite property
     * of the state of the monitor. In this case it represents the
     * property that the queue is not empty. Condition variables are
     * always linked to one or more state variables in this way. A
     * thread that waits on a condition variable, really waits for the
     * associated property of the state to be fulfilled.
     *
     * Note that the waiting thread tests the non-empty property after
     * it is awoken, and goes back to sleep if it still is not
     * satisfied. It may seem that this is not necessary in the
     * example above, but it is, for several reasons. First, if
     * multiple threads have called the \c get method and are waiting
     * on the condition when an element arrives, then only one of them
     * will find the queue non-empty. The rest must go back to
     * sleep. Also, had there been a third method that manipulated the
     * queue, a thread calling that method might have been able to
     * squeeze itself into the monitor before the waiting thread was
     * allowed to return. The point is that the return from a \c wait
     * method means that the condition was satisfied at some point in
     * time, but not that it remains to be satisfied at the time the
     * waiter returns to the monitor. Lastly, this implementation of
     * the condition variable will occasionally cause the wait method
     * to return spuriously, that is, in cases where no thread has
     * issued a notification.
     *
     * Note finally that the condition variable is associated with a
     * mutex. A condition variable always is, and it is essential that
     * the mutex is locked by a thread when that thread calls the \c
     * wait method. The effect of calling \c wait is to unlock the
     * mutex and go to sleep until some other thread issues a
     * notification on the condition variable. The unlocking of the
     * mutex allows other threads to enter the monitor and make
     * changes to its state.
     *
     * When a thread changes the state of the monitor in such a way
     * that a condition becomes satisfied, that thread must issue a
     * notification on the associated condition variable before it
     * leaves the monitor.
     */
    struct Condition
    {
      Condition(Core::Mutex &m): mutex(m) {}


      /**
       * Wake up at least one thread that is currently asleep waiting
       * on this condition. If no threads are waiting, this method
       * does nothing.
       *
       * While not being a safety issue, it is recommended that the
       * associated mutex is locked by the thread that calls this
       * method, since that will generally result in a more sensible
       * scheduling behavior.
       *
       * \note This method is thread-safe.
       */
      void notify_one();


      /**
       * Wake up all threads that are currently blocked waiting on
       * this condition. If no threads are blocked, this method does
       * nothing.
       *
       * While not being a safety issue, it is recommended that the
       * associated mutex is locked by the thread that calls this
       * method, since that will generally result in a more sensible
       * scheduling behavior.
       *
       * \note This method is thread-safe.
       */
      void notify_all();


      /**
       * Wait for some other thread to issue a notification on this
       * condition variable.
       *
       * VERY IMPORTANT: A condition variable has a mutex associated
       * with it, and a thread must hold a lock on that mutex when it
       * calls this method. Failure to comply with this rule will
       * result in undefined behavior.
       *
       * The mutex is automatically unlocked when the calling thread
       * is put to sleep, and it is locked again before this method
       * exits, even if it exits by throwing an exception.
       *
       * The transition to the sleeping state and the unlocking of the
       * mutex happens atomically from the point of view of other
       * threads. This is important because a condition does not
       * remember notifications, so without the atomicity guarantee a
       * notification could be missed if it occurred after the
       * unlocking of the mutex and before the waiting thread was
       * actually put to sleep.
       *
       * Note that there is generally no guarantee that the condition
       * waited for is satisfied upon return from this method. In part
       * because other threads may intervene and invalidate the
       * condition, and in part because this implementation of the
       * condition variable allows this method to return spuriously
       * (without any notification). For this reason it is not only
       * recommended, but mandatory, that you re-check the condition
       * upon return and repeat the wait until you discover that the
       * condition is satisfied. See class documentation for more on
       * this.
       *
       * The blocking of the calling thread may or may not be
       * interrupted at the reception of a UNIX system signal. You
       * must not rely on either behavior.
       *
       * If you want to have a guarantee that waiting does not
       * continue beyond a certain point in time, you may pass a \c
       * timeout argument. If the passed time is the current time, or
       * it is in the past (but not zero) then the \c wait method will
       * return immediately. If the passed time is zero (the default),
       * the wait can continue indefinitely.
       *
       * This method is equivalent to \c pthread_cond_wait from the
       * POSIX Threads API.
       *
       * \return True if waiting was aborted due to the timeout being
       * reached. Otherwise it returns false. Note, while a true
       * result implies that the timeout is reached, a false result
       * does not imply that the timeout is not reached when the
       * method returns.
       *
       * \throw Core::InterruptException If the calling thread has
       * been interrupted.
       *
       * \note The mutex must be locked at entry, and will always be
       * locked upon exit from this method, even when it throws an
       * exception.
       *
       * \note This method is thread-safe.
       *
       * \sa Time
       */
      bool wait(Core::Time timeout = 0);


      /**
       * Like \c wait except that the waiting done by this method is
       * also aborted if one of the file descriptors mentioned in the
       * \c SelectSpec argument becomes 'ready' in the same sense as
       * for the \c select system call.
       *
       * As is true for the \c select system call, there is no
       * guarantee that a read will not block even when the file
       * descriptor is marked as ready upon return. For this reason it
       * is recommended that all file descriptors used with this
       * method are configured as non-blocking.
       *
       * The \c SelectSpec object passed as argument may be used by at
       * most one thread at a time.
       *
       * \note This method is thread-safe.
       *
       * \sa SelectSpec
       * \sa Time
       * \sa Sys::nonblock
       */
      bool select(SelectSpec &s, Core::Time timeout = 0);


    private:
      friend struct Thread;

      struct SimpleCond
      {
        SimpleCond();
        ~SimpleCond();
        void wait(pthread_mutex_t &); // Indefinate
        bool wait(pthread_mutex_t &, Core::Time const &timeout); // Indefinate if timeout = 0
        void notify_one();
        void notify_all();

      private:
        pthread_cond_t pthread_cond;
      };

      SimpleCond simple_cond;
      Core::Mutex &mutex;

      Core::Mutex pipes_mutex;
      std::set<int> pipes; // Write end of pipes to wake up selects. Protected by 'pipes_mutex'

      void pipes_notify(); // Must be called with lock on 'pipes_mutex'

      struct ConditionPublisher;
      struct MutexUnlocker;

      Condition(Condition const &); // Hide
      Condition &operator=(Condition const &); // Hide
    };



    struct SelectSpec
    {
      std::set<int> read_in,  write_in,  except_in;
      std::set<int> read_out, write_out, except_out;

      SelectSpec(): prepped(false) {}
      ~SelectSpec() { if(prepped) close_pipe(); }

    private:
      SelectSpec(SelectSpec const &); // Hide
      SelectSpec &operator=(SelectSpec const &); // Hide

      friend struct Condition;

      void prep() { if(!prepped) open_pipe(); }
      void open_pipe();
      void close_pipe();

      int pipe_fds[2];
      bool prepped;
    };






    // Implementation:

    inline void Condition::notify_one()
    {
      simple_cond.notify_one();
      Core::Mutex::Lock l(pipes_mutex);
      if(!pipes.empty()) pipes_notify();
    }


    inline void Condition::notify_all()
    {
      simple_cond.notify_all();
      Core::Mutex::Lock l(pipes_mutex);
      if(!pipes.empty()) pipes_notify();
    }


    inline Condition::SimpleCond::SimpleCond()
    {
      int const e = pthread_cond_init(&pthread_cond, 0);
      if(e != 0) throw std::runtime_error("Could not initialize condition");
    }


    inline void Condition::SimpleCond::notify_one()
    {
      int const e = pthread_cond_signal(&pthread_cond);
      if(e != 0) throw std::runtime_error("Attempt to signal on condition failed");
    }


    inline void Condition::SimpleCond::notify_all()
    {
      int const e = pthread_cond_broadcast(&pthread_cond);
      if(e != 0) throw std::runtime_error("Attempt to broadcast on condition failed");
    }


    inline void Condition::SimpleCond::wait(pthread_mutex_t &m)
    {
      int const e = pthread_cond_wait(&pthread_cond, &m);
      if(e != 0) throw std::runtime_error("Attempt to wait on condition failed");
    }
  }
}

#endif // ARCHON_THREAD_CONDITION_HPP
