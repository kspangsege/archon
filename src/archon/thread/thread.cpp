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

#include <errno.h>
#include <iostream>

#include <archon/thread/thread.hpp>


using namespace std;
using namespace archon::Core;


namespace archon
{
  namespace Thread
  {
    /**
     * TSD key for the thread self reference.
     */
    pthread_key_t Thread::self_key;

    /**
     * Once-only allocation of the thread self reference TSD key.
     */
    pthread_once_t Thread::self_key_once = PTHREAD_ONCE_INIT;

    /**
     * Protects active_threads and last_active_thread.
     */
    Mutex Thread::active_threads_mutex;

    /**
     * Signalled when active_threads goes from 2 to 1. Protected by
     * 'active_threads_mutex'.
     */
    Condition::SimpleCond Thread::last_active_thread;

    /**
     * Number of running threads which have either been started
     * through one of the methods of Thread or have called
     * Thread::self at least once. Protected by
     * 'active_threads_mutex'.
     */
    unsigned long Thread::active_threads = 0;


    void Thread::start(RefArg t)
    {
      if(!t) throw invalid_argument("Got null thread");
      {
	Mutex::Lock l(t->mutex);
	if(t->started) throw AlreadyStartedException();
	t->started = true;
      }

      pthread_attr_t attr;
      int error = pthread_attr_init(&attr);
      if(error) throw runtime_error("pthread_attr_init failed");
      error = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
      if(error) throw runtime_error("pthread_attr_setdetachstate failed");

      activate(t.get());
      pthread_t pthread; // Not used, and is unreliable due to the detatched state
      error =
        pthread_create(&pthread, &attr, reinterpret_cast<void *(*)(void *)>(t->entry), t.get());
      pthread_attr_destroy(&attr);

      if(!error) return;
      deactivate(t.get()); // Break the self reference.
      if(error == EAGAIN)
	throw runtime_error("Not enough system resources to create a new thread");
      throw runtime_error("Could not create a new thread");
    }


    /**
     * Must be called exactly once for each thread that is started by
     * <tt>Thread::start</tt>, and must be called just before that
     * thread starts to run. For theads that are not started this way,
     * this method will be called the first time <tt>Thread::self</tt>
     * is called, but it is still required to be called at most once.
     */
    void Thread::activate(Thread *t) throw()
    {
      {
        Mutex::Lock l(active_threads_mutex);
        ++active_threads;
      }
      // Make a reference cycle such that the thread object stays
      // alive with anyone holding a reference to it. This represents
      // the implicit reference held by the running thread. the cycle
      // will be broken forcefully when the thread terminates.
      t->self_ref.reset(t);
    }


    /**
     * Must be called exactly once for each thread where
     * <tt>Thread::activate</tt> was called.
     *
     * Must be called with self_ref set, and the calling thread must
     * hold no other reference counts to the thread object.
     *
     * It is important that the signalling on the last_thread
     * condition is the very last thing that happens for the calliing
     * thread, since this ensures that all resources held by the
     * thread are deallocated before a call to Thread::main_exit_wait
     * returns. In particular it is important that the usage count is
     * decremented before the signalling on last_thread, since if it
     * was not, and the self reference TSD key is the last reference
     * (implicit) to the usage count then the thread object might
     * never get deallocated because of main thread exit, or the
     * destructor might run after the main thread has returned from
     * the main function.
     */
    void Thread::deactivate(Thread *t)
    {
      // Break the reference cycle that keeps the thread object
      // alive. When no other thread than the calling one, holds a
      // reference to the thread object, the thread object is
      // guaranteed to be destroyed here.
      t->self_ref.reset();

      // 't' is now a dangeling pointer.

      {
        Mutex::Lock l(active_threads_mutex);
        if(--active_threads == 1) last_active_thread.notify_all();
      }
    }


    /**
     * Must be called with self_ref set.
     */
    void *Thread::entry(Thread *t) throw()
    {
      int error = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
      if(error) throw runtime_error("pthread_setcancelstate failed");
      // In case of errors 'register_self' will throw an exception and
      // return with the extra reference count removed. The immediate
      // destruction of the temporary reference object will then
      // deallocate the referenced object.
      register_self(t);
      try
      {
	t->main();
      }
      catch(InterruptException &) {}
      return 0;
    }


    /**
     * Must be called with self_ref set, and the calling thread must
     * hold no other reference counts to the thread object.
     */
    void Thread::register_self(Thread *t)
    {
      // Calls 'self_key_alloc' if this thread is the first to call
      // 'pthread_once' on 'self_key_once'.
      int error = pthread_once(&self_key_once, &self_key_alloc);
      if(error)
      {
	deactivate(t);
	throw runtime_error("pthread_once failed");
      }
      // If the following call succeeds, 'self_key_destroy' will be
      // called when this thread terminates, and will handle
      // deactivation. If the following call fails, 'self_key_destroy'
      // will till be called, but it will get a pointer to the thread
      // object, so it cannot deactivate, therefore we must do it
      // here.
      error = pthread_setspecific(self_key, reinterpret_cast<void *>(t));
      if(error)
      {
	deactivate(t);
	if(error == EINVAL)
	  throw runtime_error("Invalid TSD key in pthread_setspecific");
	throw runtime_error("pthread_setspecific failed");
      }
    }


    /**
     * Allocate the thread self reference TSD key.
     */
    void Thread::self_key_alloc() throw()
    {
      int error = pthread_key_create(&self_key, &Thread::self_key_destroy);
      if(error)
      {
	if(error == EAGAIN)
	  throw runtime_error("Too many TSD keys");
	throw runtime_error("Could not create new TSD key");
      }
    }


    /**
     * Handle destruction of self reference TSD key.
     *
     * This is also the method that is responsible for decrementing
     * the usage count of the associated thread object.
     */
    void Thread::self_key_destroy(void *v) throw()
    {
      if(Thread *t = reinterpret_cast<Thread *>(v))
      {
        {
          Mutex::Lock l(t->mutex);
          t->terminated = true;
          t->termination.notify_all();
        }
        deactivate(t);
      }
    }


    struct Thread::SelfThread: Thread
    {
      SelfThread()
      {
	started = true;
      }

      void main() {} // Dummy
    };

    Thread::Ref Thread::self()
    {
      // For any thread that is started using this thread class, the thread specific 'self key' will already have been allocated
      int error = pthread_once(&self_key_once, &self_key_alloc);
      if(error) throw runtime_error("pthread_once failed");
      Thread *t = reinterpret_cast<Thread *>(pthread_getspecific(self_key));
      if(t) return Ref(t);
      t = new SelfThread;
      activate(t);
      register_self(t);
      return Ref(t);
    }


    struct Thread::InterruptReleaser
    {
      InterruptReleaser(Thread *t) throw(): thread(t) {}

      ~InterruptReleaser() throw()
      {
        Mutex::Lock l(thread->mutex);
        thread->interrupting = false;
        thread->interrupting_cond.notify_all();
      }

    private:
      Thread *const thread;
    };


    void Thread::interrupt()
    {
      {
        Mutex::Lock l(mutex);
        if(interrupted) return;
        interrupted = true;
        if(!current_wait_cond) return;
        interrupting = true;
      }

      InterruptReleaser r(this);

      {
        Mutex::Lock l(current_wait_cond->mutex);
        current_wait_cond->notify_all();
      }
    }


    void Thread::wait()
    {
      Mutex::Lock l(mutex);
      if(!started) throw NotStartedException();
      while(!terminated) termination.wait();
    }


    void Thread::sleep(Time const &period)
    {
      Time timeout(Time::now());
      timeout += period;
      if(timeout) sleep_until(timeout);
    }


    void Thread::sleep_until(Time const &timeout)
    {
      Mutex mutex;
      Condition cond(mutex);
      Mutex::Lock lock(mutex);
      while(!cond.wait(timeout)) {}
    }


    void Thread::accept_interruption()
    {
      Ref const s = self();
      Mutex::Lock lock(s->mutex);
      if(s->interrupted)
      {
        s->interrupted = false;
        throw InterruptException();
      }
    }


    bool Thread::main_exit_wait(Time timeout)
    {
      self(); // Must make sure that the calling thread counts as active.
      Mutex::Lock l(active_threads_mutex);
      while(1 < active_threads)
        if(last_active_thread.wait(active_threads_mutex.mutex, timeout))
          return true; // Timed out

      // Get rid of the SelfThread instance that may be associated
      // with the main thread
      int error = pthread_once(&self_key_once, &self_key_alloc);
      if(!error)
      {
        if (Thread *t = reinterpret_cast<Thread *>(pthread_getspecific(self_key))) {
          error = pthread_setspecific(self_key, 0);
          if(!error) t->self_ref.reset();
        }
      }

      return false;
    }
  }
}
