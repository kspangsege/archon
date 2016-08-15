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

#ifndef ARCHON_CORE_MUTEX_HPP
#define ARCHON_CORE_MUTEX_HPP

#include <pthread.h>
#include <cerrno>
#include <stdexcept>

#include <archon/core/assert.hpp>

namespace archon
{
  namespace Thread
  {
    struct Thread;
    struct Condition;
  }


  namespace Core
  {
    /**
     * Thrown by the \c Mutex::Lock constructor and the \c
     * Mutex::Lock::acquire method if the \c non_block flag was true
     * and the mutex was already locked.
     */
    struct AlreadyLockedException;


    /**
     * A mutex for thread synchronization.
     *
     * \sa Mutex::Lock
     */
    struct Mutex
    {
      Mutex();
      ~Mutex();

      /**
       * A mutex lock holder.
       *
       * This class is used to lock and unlock a mutex.
       * In the simplest situation it is used like this:
       *
       * Mutex m;
       *
       * void f()
       * {
       *   ...
       *
       *   {
       *     Mutex::Lock l(m)
       *
       *     // Critical stuff
       *   }
       *
       *   ...
       * }
       *
       * That is, The costruction of Lock, when a mutex is passed as
       * argument, locks that mutex. Also, the implicit destruction at
       * the end of its scope unlocks the mutex.
       *
       * You may also construct a Lock without passing a mutex. This
       * feature together with the availability of the 'acquire' and
       * 'release' methods allows you to do locking that does not
       * strictly follow your scopes while still ensuring that the the
       * lock is release when you leave the scope of the Lock
       * variable eg. when an execption escapes that scope.
       *
       * Note: The individual Lock variables are not thread-safe. That
       * is, only one thread must access a specific Lock variable at
       * any time (this includes the implicit destruction).
       *
       * \sa acquire
       * \sa release
       */
      struct Lock
      {
	/**
	 * Create a mutex lock holder that does not hold a lock
	 * initally. A lock may later be acquired using the \c acquire
	 * method.
	 *
	 * \sa acquire
	 */
	Lock() throw(): mutex(0) {}

	/**
	 * Create a mutex lock holder and acquire a lock on the
	 * specified mutex.
         *
         * \param m The mutex to lock.
         *
         * \param non_block If true, do not wait for the specified
         * mutex to be released, if it is already locked. Instead
         * throw <tt>AlreadyLockedException</tt>.
         *
         * \throw AlreadyLockedException If \c non_block was true and
         * the specified mutex was already locked.
	 */
	Lock(Mutex const &m, bool non_block = false);

	/**
	 * Acquire a lock on the specified mutex. If another lock was
	 * already held it is first released.
         *
         * \param m The mutex to lock.
         *
         * \param non_block If true, do not wait for the specified
         * mutex to be released, if it is already locked. Instead
         * throw <tt>AlreadyLockedException</tt>.
         *
         * \throw AlreadyLockedException If \c non_block was true and
         * the specified mutex was already locked. In this case, a
         * previously held lock, will still be released.
	 */
	void acquire(Mutex const &m, bool non_block = false);

	/**
	 * Release the currently held mutex lock. If no lock was held,
	 * nothing is done.
	 */
	void release() throw();

        bool is_acquired() const throw() { return mutex; }

	~Lock() throw();

      private:

	Mutex const *mutex;

	Lock(Lock const &); // Hide
	Lock &operator=(Lock const &); // Hide
      };

    private:
      friend struct Thread::Condition;
      friend struct Thread::Thread;

      mutable pthread_mutex_t mutex;

      Mutex(Mutex const &); // Hide
      Mutex &operator=(Mutex const &); // Hide

      /*
       * Note that the constness of the 'lock' and the 'unlock'
       * methods, strictly speaking, is nonsense, but it is faked to
       * allow you to implement methods that are both const and thread
       * safe without requiring you to const_cast 'this' every time.
       */
      void lock(bool non_block) const;
      void unlock() const throw();
    };





    // Definitions:

    struct AlreadyLockedException: std::runtime_error
    {
      AlreadyLockedException():
        std::runtime_error("Mutex was already locked") {}
    };


    inline Mutex::Mutex()
    {
      int const e = pthread_mutex_init(&mutex, 0);
      if(e != 0)
        throw std::runtime_error("Mutex initialization failed");
    }

    inline Mutex::~Mutex()
    {
      int ret = pthread_mutex_destroy(&mutex);
      ARCHON_ASSERT(ret == 0);
    }

    inline void Mutex::lock(bool non_block) const
    {
      int const e =
        non_block ? pthread_mutex_trylock(&mutex) :
        pthread_mutex_lock(&mutex);
      if(e == 0) return;
      if(e == EBUSY && non_block)
        throw AlreadyLockedException();
      if(e == EDEADLK)
        throw std::runtime_error("Attempt to lock a locked mutex");
      if(e == EINVAL)
        throw std::runtime_error("Attempt to lock uninitialized mutex");
      throw std::runtime_error("Attempt to lock mutex failed");
    }

    inline void Mutex::unlock() const throw()
    {
      int const e =
        pthread_mutex_unlock(&mutex);
      if(e == 0) return;
      if(e == EPERM)
        throw std::runtime_error("Attempt to unlock mutex that is locked "
                                 "by another thread");
      if(e == EINVAL)
        throw std::runtime_error("Attempt to unlock uninitialized mutex");
      throw std::runtime_error("Attempt to unlock mutex failed");
    }


    inline Mutex::Lock::Lock(Mutex const &m, bool non_block): mutex(&m)
    {
      m.lock(non_block);
    }

    inline Mutex::Lock::~Lock() throw()
    {
      release();
    }

    inline void Mutex::Lock::acquire(Mutex const &m, bool non_block)
    {
      release();
      m.lock(non_block);
      mutex = &m;
    }

    inline void Mutex::Lock::release() throw()
    {
      if(!mutex) return;
      mutex->unlock();
      mutex = 0;
    }
  }
}

#endif // ARCHON_CORE_MUTEX_HPP
