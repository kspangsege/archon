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


#include <archon/platform.hpp>

#ifdef ARCHON_HAVE_LINUX_INOTIFY
#include <unistd.h>
#include <sys/inotify.h>
#include <cerrno>
#include <stdexcept>
#endif // ARCHON_HAVE_LINUX_INOTIFY

#include <archon/core/sys.hpp>
#include <archon/thread/thread.hpp>
#include <archon/util/filesys_listen.hpp>


using namespace std;
using namespace archon::Core;
using namespace archon::Thread;
using namespace archon::Util;


namespace
{
#ifdef ARCHON_HAVE_LINUX_INOTIFY


  struct FileSystemListenerImpl: FileSystemListener
  {
    static bool is_supported()
    {
      return true;
    }


    bool wait(Time timeout)
    {
      bool const timed_out = Thread::select(select_spec, timeout);
      clear();
      return timed_out;
    }


    void clear()
    {
      static char buffer[512];
      for(;;)
      {
	ssize_t n = read(file_des, buffer, sizeof(buffer));
	if(0<n) continue;
	if(!n) throw runtime_error("Unexpected end of 'inotify' input");
        int const errnum = errno;
	if(errnum == EAGAIN) break; // Would block
	if(errnum == EINTR) continue;
	throw runtime_error("'read' from 'inotify' failed: "+Sys::error(errnum));
      }
    }

    FileSystemListenerImpl(string p)
    {
      file_des = inotify_init();
      if(file_des < 0)
      {
        int const errnum = errno;
        throw runtime_error("'inotify_init'"+Sys::error(errnum));
      }
      Sys::nonblock(file_des);
      int w = inotify_add_watch(file_des, p.c_str(), IN_CREATE);
      if(w < 0)
      {
        int const errnum = errno;
        throw runtime_error("'inotify_add_watch' failed: "+Sys::error(errnum));
      }
      select_spec.read_in.insert(file_des);
    }

    ~FileSystemListenerImpl()
    {
      close(file_des);
    }

    int file_des;
    SelectSpec select_spec;
  };


#else // ARCHON_HAVE_LINUX_INOTIFY


  struct FileSystemListenerImpl: FileSystemListener
  {
    static bool is_supported()
    {
      return false;
    }


    bool wait(Time timeout)
    {
      Thread::sleep_until(timeout);
      return true;
    }


    void clear() {}


    FileSystemListenerImpl(string) {}
  };


#endif // ARCHON_HAVE_LINUX_INOTIFY


/*
  struct FileSystemListenerImpl: FileSystemListener
  {
    static bool is_supported()
    {
      return true;
    }


    int wait(Time timeout)
    {
      set<int> block, unblock = unblock_signals ? *unblock_signals : set<int>();
      block.insert(SIGIO);
      unblock.insert(SIGIO);
      int r = 0;
      {
	Sys::Signal::Block b(block);
	if(!event) r = Sys::select(0, 0, 0, timeout, &unblock);
	if(!event) return r; // In this case 'event' must have been
			     // false before the select and therfore r
			     // was assigned a value from select.
      }
      clear();
      return 1;
    }


    void clear()
    {
      event = false;
    }


    FileSystemListenerImpl(string p)
    {
      oldHandler = Sys::Signal::setHandler(SIGIO, sighandler);
      dir = opendir(p.c_str());
      int errnum = errno;
      if(!dir)
	throw runtime_error("'opendir(\""+p+"\")' failed: "+Sys::error(errnum));
      int fd = dirfd(dir);
      // The next two fcntl's makes sure the signal is always delivered to this thread
      if(fcntl(fd, F_SETSIG, SIGIO) < 0)
	throw runtime_error("'fcntl(F_SETSIG)' failed: "+Sys::error(errno));
      if(fcntl(fd, F_SETOWN, syscall(__NR_gettid)) < 0)
	throw runtime_error("'fcntl(F_SETOWN)' failed: "+Sys::error(errno));
      if(fcntl(fd, F_SETFL, O_ASYNC) < 0)
	throw runtime_error("'fcntl(F_SETFL)' failed: "+Sys::error(errno));
      if(fcntl(fd, F_NOTIFY, DN_MULTISHOT|DN_CREATE) < 0)
	throw runtime_error("'fcntl(F_NOTIFY)' failed: "+Sys::error(errno));
    }


    ~FileSystemListenerImpl()
    {
      closedir(dir);
      Sys::Signal::setHandler(SIGIO, oldHandler);
    }


    DIR *dir;
    Sys::Signal::Handler oldHandler;

    static void sighandler(int)
    {
      event = true;
    }

    static bool event;
  };

  bool FileSystemListenerImpl::event = false;

*/
}



namespace archon
{
  namespace Util
  {
    bool FileSystemListener::is_supported()
    {
      return FileSystemListenerImpl::is_supported();
    }


    UniquePtr<FileSystemListener> FileSystemListener::new_listener(string path)
    {
      UniquePtr<FileSystemListener> l(new FileSystemListenerImpl(path));
      return l;
    }
  }
}
