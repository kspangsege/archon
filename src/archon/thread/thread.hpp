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

#ifndef ARCHON_THREAD_THREAD_HPP
#define ARCHON_THREAD_THREAD_HPP

#include <stdexcept>

#include <archon/core/time.hpp>
#include <archon/core/refcnt.hpp>
#include <archon/thread/condition.hpp>

namespace archon
{
  namespace Thread
  {
    /**
     * A thread of execution (aka a light weight proccess or LWP)
     * represented as an object.
     *
     * You may think of it a thread object as an object with a will of
     * its own, or an animated object. Just as is the case for human
     * beings, going from one to two or more 'entities', will open up
     * a whole new realm of wonders and disasters. So, prepare
     * yourself.
     *
     * Whenever your program launches threads, it is strongly
     * recommended that you do not exit from the main function untill
     * all threads are known to have terminated. This can be done by
     * calling the static method \c main_exit_wait imedaitely before
     * returning from <tt>main</tt>. Please be sure to read the
     * documentation on that method carefully.
     *
     * This class supports safe cancelation/termination of
     * threads. The problem of thread cancelation usually is that it
     * is hard to guarantee proper cleanup of allocated memory and
     * other resources. This is especially true in C++ programs where
     * it is crucial that the stack is noramlly unwinded ensuring that
     * class detructors gets called. So what we really need is to
     * allow one thread to cause the throwing of an exception in the
     * context of another thread. This is exactly what this thread
     * class supports. If a thread gets an interruption request, then
     * any current or future call to any of the following methods will
     * throw a <tt>Core::InterruptException</tt>.
     *
     * <pre>
     *
     *   Condition::wait
     *   Condition::select
     *   Thread::wait
     *   Thread::sleep
     *   Thread::select
     *   Thread::accept_interruption
     *   Semaphore::down
     *
     * </pre>
     *
     * \todo Implement thread priority.
     *
     * \todo One should considder adding a certain feature to the
     * Thread class which would lead to simpler implementations in
     * cases where threads are to run as a service for as long as the
     * servicing object is referenced by outside parties such as a
     * server object. In any case it is important that a Thread object
     * is kept alive while the thread is running. This is currently
     * done by holding one extra reference count during
     * execution. This is most suitable in scenarios where a thread is
     * intended to run for a short while and then end, probably
     * because it has a certain limited job to do. Often, though, we
     * use threads in a servicing scenario where there is no intrinsic
     * condition limiting the running time. This leads, as you can
     * see, to a situation where the object will keep itself alive
     * indefinitely unless the thread is interrupted. This is most
     * often an unwanted effect. In cases like this, we would rather
     * like an automated trigger to interrupt the thread execution
     * when all outside reference are released. To simulate this, one
     * can employ a tandem object solution where one object, A, is
     * referenced by the ouside. And the Thread object, B, is
     * reference only by A. Now, when all outside references are
     * abandoned, A's destructor gets called and from here we can
     * explicitly interrupt the thread in B. So, why not add a feature
     * to Thread that supports this behaviour directly?
     */
    struct Thread: virtual Core::CntRefObjectBase, Core::CntRefDefs<Thread>
    {
      struct AlreadyStartedException: std::runtime_error
      {
	AlreadyStartedException(): std::runtime_error("") {}
      };

      struct NotStartedException: std::runtime_error
      {
	NotStartedException(): std::runtime_error("") {}
      };


      /**
       * Let a new thread invoke a method of your choise on a
       * reference counted object of your choise. An extra reference
       * count is held during the execution of the specified method,
       * and it is released afterwards.
       *
       * \note This method is thread-safe.
       *
       * \note The idea behind this method is that you have guaranteed
       * access to the data of the object passed as argument. That is,
       * the occupied memory is not deallocated while your method is
       * running. You cannot safely access anything else unles it is
       * also a dynamically allocated object.
       */
      template<typename Obj>
      static Ref run(Core::CntRef<Obj>, void (Obj::*)(), bool start = true);

      /**
       * Like Thread::run(Core::CntRef<Obj>, void (Obj::*)()) except
       * that it invokes a method taking one argument, and you get to
       * specify that extra argument when calling this method.
       *
       * \note Be sure to not use an aliased types '&' for the method
       * argument, since copying is essential to thread safty.
       */
      template<typename Obj, typename Arg>
      static Ref run(Core::CntRef<Obj>, void (Obj::*)(Arg), Arg, bool start = true);

      /**
       * Let a new thread invoke a method of your choise on a object
       * of your choise. It is your responsibility that the object is
       * not destroyed until after the thread terminates. Consider
       * using the variant that works on a reference counted objest,
       * since it provides extra safety.
       *
       * \note This method is thread-safe.
       */
      template<typename Obj>
      static Ref run(Obj *, void (Obj::*)(), bool start = true);

      /**
       * Like Thread::run(Obj *, void (Obj::*)()) except that it
       * invokes a method taking one argument, and you get to specify
       * that extra argument when calling this method. It is your
       * responsibility that the object is not destroyed until after
       * the thread terminates. Consider using the variant that works
       * on a reference counted objest, since it provides extra
       * safety.
       *
       * \note Be sure to not use an aliased types '&' for the method
       * argument, since copying is essential to thread safty.
       */
      template<typename Obj, typename Arg>
      static Ref run(Obj *, void (Obj::*)(Arg), Arg, bool start = true);

      /**
       * Let a new thread invoke a static 1-argument-function of your
       * choise.
       *
       * \note This method is thread-safe.
       *
       * \note Be sure to not use an aliased types '&' for the method
       * argument, since copying is essential to thread safty.
       *
       * \note You must not rely on global data to be acccessible. It
       * may be destroyed before this thread terminates.
       */
      template<typename T>
      static Ref run(void (*)(T), T, bool start = true);

      /**
       * Let a new thread invoke a static void-function of your
       * choise.
       *
       * \note This method is thread-safe.
       *
       * \note You must not rely on global data to be acccessible. It
       * may be destroyed before this thread terminates.
       */
      static Ref run(void (*)(), bool start = true);

      /**
       * Let a new thread invoke the virtual main method of the
       * specified Thread object. To be of any use, you must derive
       * your own class from the Thread object. An extra reference
       * count is held on the thread object during the execution of
       * the specified method, and it is released when execution ends.
       *
       * \throw AlreadyStartedException If this thread has already
       * been started.
       *
       * \note It is strongly recomended that you don not use this
       * method in the constructor specifying 'this' as argument. If
       * you do, you might get back (from the constructor) an object
       * which has already been deleted.
       *
       * \sa Core::CntRef<T>
       *
       * \note This method is thread-safe.
       *
       * \note NEVER do a Thread::start(this) in a constructor!
       *
       * \note The idea behind this method is that you have guaranteed
       * access to the data of the object passed as argument. That is,
       * the occupied memory is not deallocated while your method is
       * running. You cannot safely access anything else unles it is
       * also a dynamically allocated object.
       */
      static void start(RefArg);

      /**
       * Wait for this thread to stop.
       *
       * \throw NotStartedException If this thread was never started.
       *
       * \throw Core::InterruptException If \c force was not true
       * and some other thread has called the \c interrupt method of
       * the calling thread.
       *
       * \note This method is thread-safe. That is, any number of
       * threads may wait simultaniously.
       */
      void wait();

      /**
       * Issue an interrupt request for this thread. This causes any
       * present or furure call to
       *
       * <pre>
       *
       *   Condition::wait
       *   Condition::select
       *   Thread::wait
       *   Thread::sleep
       *   Thread::select
       *   Thread::accept_interruption
       *   Semaphore::down
       *
       * </pre>
       *
       * by this thread to imediately throw a
       * <tt>Core::InterruptException</tt>. This method will
       * normally be called by some other thread in the intent to
       * bring the execution of this thread to an end.
       *
       * Make sure the calling thread is not holding a lock on a mutex
       * that is associated with a condition on which the target
       * thread is waiting, since that would cause an immediate
       * dead-lock.
       *
       * \note This method is thread-safe.
       */
      void interrupt();

      /**
       * Return a reference to the Thread object associated with the
       * thread that calls this method. This also works for the main
       * thread and any other thread created in the context of the
       * pthead library.
       *
       * \note This method is thread-safe.
       */
      static Ref self();

      /**
       * Make the calling thread sleep for the specified amount of
       * time.
       *
       * \throw Core::InterruptException If some other thread has
       * called the \c interrupt method for this thread.
       *
       * \sa Core::Time
       *
       * \note This method is thread-safe.
       */
      static void sleep(Core::Time const &period);

      /**
       * Make the calling thread sleep until the specified point in
       * time has been reached. As a special feature, if \c timeout is
       * zero, the sleep will continue indefinitely.
       *
       * The sleeping will not be interrupted due to reception of a
       * UNIX system signal.
       *
       * \throw Core::InterruptException If some other thread has
       * called the \c interrupt method for this thread.
       *
       * \sa Core::Time
       *
       * \note This method is thread-safe.
       */
      static void sleep_until(Core::Time const &timeout);


      /**
       * Same as Condition::select except that this method does not
       * have a condition that wakes it up.
       *
       * \note This method is thread-safe.
       */
      static bool select(SelectSpec &s, Core::Time timeout = 0);


      /**
       * This method should be called by a thread if it is executing
       * lengthy computations without any blocking calls such as
       * <tt>Condition::wait</tt>. This is to ensure a fair response
       * time to an interruption request by another thread. Note: The
       * more frequent this method is called, the shorter the response
       * to an interruption request will be.
       *
       * \throw Core::InterruptException If some other thread has
       * called the \c interrupt method for this thread.
       *
       * \note This method is thread-safe.
       */
      static void accept_interruption();

      /**
       * Should be called by your programs \c main function
       * immediately before it returns. This will cause it to wait for
       * all other threads to terminate. Only the main thread may call
       * this method.
       *
       * There are two reasons why it is important to do this. First,
       * when the main function exits, all global objects will be
       * destroyed. This may or may not be a problem for you depending
       * on whether you have any global objects in your program, and
       * it is generally best to avoid that, but ocasionally it is
       * necessary, and you might also be using 3rd party libraries
       * where it is hard to tell if they are using global objects or
       * not.
       *
       * Secondly, on some systems the return from main will
       * immediately kill any threads that are still running at that
       * time. This is a particularly crude thing in a C++ context,
       * where we would expect all thread stacks to be unwound either
       * normally or by an exception propagating all the way back to
       * the point where the tread was initiated.
       *
       * Calling main_exit_wait at the end of main solves both of
       * these problems, but of course, the side effect is that you
       * can easily end up in a situation where you programs hangs at
       * exit because some thread is running and does not know that
       * main wants to exit. On the other hand this can be seen, and
       * used, as a feature. Anyway, it might be a good idea to write
       * out a message saying something like "Waiting for all threads
       * to terminate", so that you know a little bit more about why
       * your program hangs.
       *
       * Note that this method does not guard agains exit from main
       * when main throws an exception. This could be remedied by
       * wrapping main in a try block and calling this method from a
       * catch-all block, but it is probably not a good idea, because
       * you most likely want to see what exception was thrown by
       * main.
       *
       * This method will never be interrupted, even if a thread calls
       * Thread::interrupt on the main thread.
       *
       * Theads which are not created by one of the methods in this
       * Thread class and which have never called the Thread::self
       * method are unknown to this method, and will therefore not be
       * waited for.
       *
       * If you want the waiting to be terminated at a certain point
       * in time, you may pass a non-zero \c timeout argument. If the
       * passed time is the current time, or it is in the past (but
       * not zero) then the this method will return immediately. If
       * the passed time is zero (the default), the waiting will
       * continue until all threads have terminated.
       *
       * \return True if this method returned due to the timeout being
       * reached. If \c timeout was 0, this method will always return
       * false.
       */
      static bool main_exit_wait(Core::Time timeout = 0);

    protected:
      /**
       * This is the method that will be run as the new thread. You
       * should override this method.
       */
      virtual void main() = 0;

      /**
       * Does not start the thread. You must use Thread::start to do
       * that - BUT NEVER WITHIN THE COSTRUCTOR!
       */
      Thread();

    private:
      Thread(Thread const &); // Hide
      Thread &operator=(Thread const &); // Hide

      friend struct Condition;

      struct SelfThread;
      struct InterruptReleaser;

      // This represents the reference held indirectly by the running
      // thread. When the thread terminates, this reference is
      // cleared, to drop the usage count. Since it is accessed only
      // bu 'activate' and 'deactivate' no two threads can access it
      // simultaneously.
      Ref self_ref;

      Core::Mutex mutex;
      bool started; // Protected by 'mutex'.
      bool terminated; // Protected by 'mutex'.
      Condition termination; // Protected by 'mutex'.
      Condition *current_wait_cond; // The condition that this thread is currently waiting on. Pointer is protected by 'mutex'.
      bool interrupted; // This thread has a pending interruption request. Protected by 'mutex'.
      bool interrupting; // This thread is currently being interrupted. Protected by 'mutex'.
      Condition::SimpleCond interrupting_cond; // Signalled when 'interrupting' goes 'false'. Protected by 'mutex'.

      static void self_key_destroy(void *) throw();
      static void self_key_alloc() throw();
      static void register_self(Thread *);
      static void *entry(Thread *thread) throw();
      static void activate(Thread *) throw();
      static void deactivate(Thread *);

      static pthread_key_t self_key;
      static pthread_once_t self_key_once;
      static Core::Mutex active_threads_mutex;
      static Condition::SimpleCond last_active_thread;
      static unsigned long active_threads;
    };






    // Implementation:


    inline Thread::Thread(): started(false), terminated(false), termination(mutex),
                             current_wait_cond(0), interrupted(false), interrupting(false) {}


    inline bool Thread::select(SelectSpec &s, Core::Time t)
    {
      Core::Mutex m;
      Condition c(m);
      Core::Mutex::Lock l(m);
      return c.select(s,t);
    }


    template<typename Obj> struct RefCntMethodVoidRunner: Thread
    {
      Core::CntRef<Obj> obj;
      void (Obj::*meth)();
      RefCntMethodVoidRunner(Core::CntRef<Obj> o, void (Obj::*m)()): obj(o), meth(m) {}
      void main()
      {
	// The swap is used to ensure that we do not hold on the the
	// reference counted object any longer than we need to.
	Core::CntRef<Obj> o;
	o.swap(obj);
	(o.get()->*meth)();
      }
    };

    template<typename Obj>
    inline Core::CntRef<Thread> Thread::run(Core::CntRef<Obj> o, void (Obj::*m)(), bool s)
    {
      Core::CntRef<RefCntMethodVoidRunner<Obj> > r(new RefCntMethodVoidRunner<Obj>(o,m));
      if(s) start(r);
      return r;
    }



    template<typename Obj, typename Arg> struct RefCntMethodArgRunner: Thread
    {
      Core::CntRef<Obj> obj;
      void (Obj::*meth)(Arg);
      Arg arg;
      RefCntMethodArgRunner(Core::CntRef<Obj> o, void (Obj::*m)(Arg), Arg a):
        obj(o), meth(m), arg(a) {}

      void main()
      {
	// The first swap is used to ensure that we do not hold on the
	// the reference counted object any longer than we need
	// to. The second one is to ensure that when the argument owns
	// some resource (e.g. if it is a smart pointer) then we do
	// not hold on to that resource any longer that we need to.
	Core::CntRef<Obj> o;
	Arg a;
	o.swap(obj);
	std::swap(a, arg);
	(o.get()->*meth)(a);
      }
    };

    template<typename Obj, typename Arg>
    inline Core::CntRef<Thread> Thread::run(Core::CntRef<Obj> o, void (Obj::*m)(Arg), Arg a, bool s)
    {
      Core::CntRef<RefCntMethodArgRunner<Obj, Arg> > r(new RefCntMethodArgRunner<Obj, Arg>(o,m,a));
      if(s) start(r);
      return r;
    }



    template<typename Obj> struct MethodVoidRunner: Thread
    {
      Obj *obj;
      void (Obj::*meth)();
      MethodVoidRunner(Obj *o, void (Obj::*m)()): obj(o), meth(m) {}
      void main() { (obj->*meth)(); }
    };

    template<typename Obj>
    inline Core::CntRef<Thread> Thread::run(Obj *o, void (Obj::*m)(), bool s)
    {
      Core::CntRef<MethodVoidRunner<Obj> > r(new RefCntMethodVoidRunner<Obj>(o,m));
      if(s) start(r);
      return r;
    }



    template<typename Obj, typename Arg> struct MethodArgRunner: Thread
    {
      Obj *obj;
      void (Obj::*meth)(Arg);
      Arg arg;
      MethodArgRunner(Obj *o, void (Obj::*m)(Arg), Arg a): obj(o), meth(m), arg(a) {}

      void main()
      {
	// The swap is used to ensure that we do not hold on to
	// resources any longer than we need to. This is especially
	// relevant when the argument is some kind of smart pointer.
	Arg a;
	std::swap(a, arg);
	(obj->*meth)(a);
      }
    };

    template<typename Obj, typename Arg>
    inline Core::CntRef<Thread> Thread::run(Obj *o, void (Obj::*m)(Arg), Arg a, bool s)
    {
      Core::CntRef<MethodArgRunner<Obj, Arg> > r(new MethodArgRunner<Obj, Arg>(o,m,a));
      if(s) start(r);
      return r;
    }



    struct FuncVoidRunner: Thread
    {
      void (*func)();
      FuncVoidRunner(void (*f)()): func(f) {}
      void main() { (*func)(); }
    };

    inline Core::CntRef<Thread> Thread::run(void (*f)(), bool s)
    {
      Core::CntRef<FuncVoidRunner> r(new FuncVoidRunner(f));
      if(s) start(r);
      return r;
    }



    template<typename Arg> struct FuncArgRunner: Thread
    {
      void (*func)(Arg);
      Arg arg;
      FuncArgRunner(void (*f)(Arg), Arg a): func(f), arg(a) {}
      void main()
      {
	// The swap is used to ensure that we do not hold on to
	// resources any longer than we need to. This is especially
	// relevant when the argument is some kind of smart pointer.
	Arg a;
	std::swap(a, arg);
	(*func)(a);
      }
    };

    template<typename Arg>
    inline Core::CntRef<Thread> Thread::run(void (*f)(Arg), Arg a, bool s)
    {
      Core::CntRef<FuncArgRunner<Arg> > r(new FuncArgRunner<Arg>(f,a));
      if(s) start(r);
      return r;
    }
  }
}

#endif // ARCHON_THREAD_THREAD_HPP
