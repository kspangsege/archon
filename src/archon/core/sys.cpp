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

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

#include <cstring>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <stdexcept>
#include <climits>
#include <locale>
#include <algorithm>
#include <string>
#include <sstream>
#include <iostream>

#include <archon/core/functions.hpp>
#include <archon/core/mutex.hpp>
#include <archon/core/sys.hpp>


using namespace std;
using namespace archon::Core;


namespace
{
  Mutex global_system_mutex;
  Mutex::Lock global_system_lock;


  // Returns true if successfull. Unspecified entries are set to
  // the empty string. On failure, no arguments are modified.
  bool parse_posix_locale(char const *begin, char const *end,
                          string &language, string &territory,
                          string &charenc, string &modifier)
  {
    // We can recognize only this format:
    //
    //   language[_territory][.codeset][@modifier]
    //
    // where 'language' is an ISO 639-1 language code, and
    // 'territory' is an ISO 3166-1 alpha-2 country code.
    locale const loc = locale::classic(); // US-ASCII
    ctype<char> const &ctype_facet = use_facet<ctype<char> >(loc);
    string l,t,c,m;
    {
      char const *const p = ctype_facet.scan_not(ctype_base::lower, begin, end);
      size_t const n = p - begin;
      if(n != 2) return false;
      l.assign(begin, n);
      begin = p;
    }
    if(begin < end && *begin == '_')
    {
      char const *const p = ctype_facet.scan_not(ctype_base::upper, ++begin, end);
      size_t const n = p - begin;
      if(n != 2) return false;
      t.assign(begin, n);
      begin = p;
    }
    if(begin < end && *begin == '.')
    {
      char const *const p = find(++begin, end, '@');
      size_t const n = p - begin;
      if(n < 1) return false;
      c.assign(begin, n);
      begin = p;
    }
    if(begin < end && *begin == '@')
    {
      char const *const p = end;
      size_t const n = p - begin;
      if(n < 1) return false;
      m.assign(begin, n);
      begin = p;
    }
    if(begin != end) return false;
    language  = l;
    territory = t;
    charenc   = c;
    modifier  = m;
    return true;
  }
}


namespace archon
{
  namespace Core
  {
    namespace Sys
    {
      GlobalLock::GlobalLock()
      {
        global_system_lock.acquire(global_system_mutex);
      }

      GlobalLock::~GlobalLock()
      {
        global_system_lock.release();
      }


      string error(int errnum)
      {
	char buf[1024];
#if _GNU_SOURCE // GNU-specific version
	errno = 0;
	char *m = strerror_r(errnum, buf, sizeof(buf));
	if(!m || errno) return "Unknown error";
	return m;
#else // POSIX version
	if(strerror_r(errnum, buf, sizeof(buf)) < 0) return "Unknown error";
	return buf;
#endif
      }


      string getenv(string name)
      {
        GlobalLock l;
        char *const p = ::getenv(name.c_str());
        return p ? string(p) : string();
      }

      void setenv(string name, string value)
      {
        GlobalLock l;
        if(::setenv(name.c_str(), value.c_str(), 1)) throw runtime_error("'setenv' failed");
      }

      void unsetenv(string name)
      {
        GlobalLock l;
        if(::unsetenv(name.c_str())) throw runtime_error("'unsetenv' failed");
      }


      string get_env_locale_charenc()
      {
        string loc;
        {
          GlobalLock l;
          char *p = ::getenv("LC_ALL");
          if(!p)
          {
            p = ::getenv("LC_CTYPE");
            if(!p)
            {
              p = ::getenv("LANG");
              if(!p) return "US-ASCII";
            }
          }
          loc.assign(p);
        }
        string language, territory, charenc, modifier;
        char const *data = loc.data();
        return parse_posix_locale(data, data+loc.size(), language,
                                  territory, charenc, modifier) ? charenc : string();
      }


      size_t read(int fd, char *b, size_t n) throw(ReadException, InterruptException)
      {
        if(int_less_than(SSIZE_MAX, n)) n = SSIZE_MAX;
        ssize_t const m = ::read(fd, b, n);
        if(m < 0)
        {
          int const e = errno;
           if(e == EINTR || e == EAGAIN) throw InterruptException();
         throw ReadException(Sys::error(e));
        }
        return m;
      }

      size_t write(int fd, char const *b, size_t n) throw(WriteException, InterruptException)
      {
        if(int_less_than(SSIZE_MAX, n)) n = SSIZE_MAX;
        ssize_t const m = ::write(fd, b, n);
        if(m < 0)
        {
          int const e = errno;
          if(e == EINTR || e == EAGAIN) throw InterruptException();
          throw WriteException(Sys::error(e));
        }
        if(static_cast<size_t>(m) != n) throw WriteException(Sys::error(EFBIG));
        return m;
      }

      void close(int fd) throw(WriteException, InterruptException)
      {
        int const r = ::close(fd);
	if(r == 0) return;
        int const e = errno;
        if(e == EINTR || e == EAGAIN) throw InterruptException();
        throw WriteException(Sys::error(e));
      }


      void nonblock(int fd)
      {
	int f = fcntl(fd, F_GETFL, 0);
	if(f == -1)
        {
          int const e = errno;
          throw runtime_error("'fcntl' failed: "+error(e));
        }
	f |= O_NONBLOCK;
	f = fcntl(fd, F_SETFL, f);
	if(f == -1)
        {
          int const e = errno;
          throw runtime_error("'fcntl' failed: "+error(e));
        }
      }


      void daemon_init()
      {
	errno = 0;
	int m = sysconf(_SC_OPEN_MAX);
	if(m < 0)
	{
	  if(errno)
          {
            int const e = errno;
            throw runtime_error("'sysconf(_SC_OPEN_MAX)' failed: "+error(e));
          }
	  throw runtime_error("'sysconf(_SC_OPEN_MAX)' failed: It's indeterminate");
	}
	int p = fork();
	if(p < 0)
        {
          int const e = errno;
          throw runtime_error("'fork' failed: "+error(e));
        }
	if(p) _exit(0);
	setsid();
	int ret = chdir("/");
        ARCHON_ASSERT(ret != -1);
	umask(S_IWGRP|S_IWOTH);
	for(int i=0; i<m; ++i) ::close(i);
      }


      string get_hostname()
      {
	char b[1024];
	if(gethostname(b, sizeof(b)-1)<0)
        {
          int const e = errno;
          throw runtime_error("'gethostname' failed: "+error(e));
        }
    	b[sizeof(b)-1] = 0;
	return b;
      }


      namespace Signal
      {
	Handler::Handler(void (*h)(int))
	{
	  sigemptyset(&act.sa_mask);
	  act.sa_flags = 0;
	  act.sa_handler = h;
	}

	Handler set_handler(int signal, Handler h)
	{
	  Handler old;
	  sigaction(signal,  &h.act, &old.act);
	  return old;
	}

	Handler reset_handler(int signal)
        {
          return set_handler(signal, SIG_DFL);
        }

	Handler ignore_signal(int signal)
        {
          return set_handler(signal, SIG_IGN);
        }

	Block::Block(std::set<int> const &signals)
	{
	  sigset_t sigset;
	  sigemptyset(&sigset);
	  for(set<int>::iterator i=signals.begin(); i!=signals.end();++i) sigaddset(&sigset, *i);
	  pthread_sigmask(SIG_BLOCK, &sigset, &original_sigset);
	}

	Block::~Block()
	{
	  pthread_sigmask(SIG_SETMASK, &original_sigset, 0);
	}

	void block(std::set<int> const &signals)
	{
	  sigset_t sigset;
	  sigemptyset(&sigset);
	  for(set<int>::iterator i=signals.begin(); i!=signals.end();++i) sigaddset(&sigset, *i);
	  pthread_sigmask(SIG_BLOCK, &sigset, 0);
	}

	void unblock(std::set<int> const &signals)
	{
	  sigset_t sigset;
	  sigemptyset(&sigset);
	  for(set<int>::iterator i=signals.begin(); i!=signals.end();++i) sigaddset(&sigset, *i);
	  pthread_sigmask(SIG_UNBLOCK, &sigset, 0);
	}
      }

      string Pipeline::run()
      {
	if(cmds.empty()) return "";

	// Start up processes in reverse order
	int devnull = -1, child_read = -1, child_write = -1, parent_read = -1, parent_write = -1;
	if(outputMode == out_discard || errorMode == err_discard)
	{
	  devnull = open("/dev/null", O_WRONLY);
	  if(devnull < 0)
          {
            int const e = errno;
            throw runtime_error("'open' for writing failed on '/dev/null': "+error(e));
          }
	}
	switch(outputMode)
	{
	case out_discard:
	  child_write = dup(devnull);
	  if(child_write < 0)
          {
            int const e = errno;
            throw runtime_error("'dup' failed: "+error(e));
          }
	  break;
	case out_stdout:
	  child_write = dup(STDOUT_FILENO);
	  if(child_write < 0)
          {
            int const e = errno;
            throw runtime_error("'dup' failed: "+error(e));
          }
	  break;
	case out_file:
	  {
	    child_write = open(outputFile.c_str(), O_WRONLY|O_CREAT|O_TRUNC,
                               S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	    if(child_write < 0)
            {
              int const e = errno;
              throw runtime_error("'open' for writing failed on '"+outputFile+"': "+error(e));
            }
	  }
	  break;
	case out_grab:
	  {
	    int p[2];
	    if(pipe(p) < 0)
            {
              int const e = errno;
              throw runtime_error("'pipe' failed: "+error(e));
            }
	    parent_read = p[0];
	    child_write = p[1];
	  }
	  break;
	}

	vector<int> pids;
	pids.resize(cmds.size());
	vector<int>::size_type j = cmds.size()-1u;
	for(list<Command>::reverse_iterator i = cmds.rbegin(); i!=cmds.rend(); ++i, --j)
	{
	  if(j)
	  {
	    int p[2];
	    if(pipe(p) < 0)
            {
              int const e = errno;
              throw runtime_error("'pipe' failed: "+error(e));
            }
	    child_read = p[0];
	    parent_write = p[1];
	  }
	  else
	  {
	    if(inputMode == in_devnull)
	    {
	      child_read = open("/dev/null", O_RDONLY);
	      if(child_read < 0)
              {
                int const e = errno;
                throw runtime_error("'open' for reading failed on '/dev/null': "+error(e));
              }
	    }
	    else
	    {
	      child_read = dup(STDIN_FILENO);
	      if(child_read < 0)
              {
                int const e = errno;
                throw runtime_error("'dup' failed: "+error(e));
              }
	    }
	  }
	  int pid = fork();
	  if(pid < 0)
          {
            int const e = errno;
            throw runtime_error("'fork' failed: "+error(e));
          }
	  if(!pid)
	  {
	    if(j) ::close(parent_write);
	    if(outputMode == out_grab) ::close(parent_read);

	    // Input
	    if(dup2(child_read, STDIN_FILENO) < 0)
	    {
	      int const e = errno;
	      cerr << "'dup2' failed: "<<error(e)<<endl;
	      _exit(1);
	    }
	    ::close(child_read);

	    // Output
	    if(dup2(child_write, STDOUT_FILENO) < 0)
	    {
	      int const e = errno;
	      cerr << "'dup2' failed: "<<error(e)<<endl;
	      _exit(1);
	    }
	    ::close(child_write);

	    // Error
	    if(errorMode==err_discard && dup2(devnull, STDERR_FILENO) < 0)
	    {
	      int const e = errno;
	      cerr << "'dup2' failed: "<<error(e)<<endl;
	      _exit(1);
	    }
	    if(-1 < devnull) ::close(devnull);

	    char **argv = new char *[i->args.size()+2u];
	    argv[0] = const_cast<char *>(i->cmd.c_str());
	    for(vector<string>::size_type k=0; k<i->args.size(); ++k) argv[k+1u] = const_cast<char *>(i->args[k].c_str());
	    argv[i->args.size()+1u] = 0;
	    execvp(argv[0], argv);
	    int const e = errno;
	    cerr << "Failed to execute '"<<i->cmd<<"': "<<error(e)<<endl;
	    _exit(1);
	  }

	  pids[j] = pid;
	  ::close(child_read);
	  ::close(child_write);
	  if(j) child_write = parent_write;
	}
	if(-1 < devnull) ::close(devnull);

	ostringstream result;

	// Read pipeline output if we must:
	if(outputMode == out_grab)
	{
	  char buffer[4096];
	  for(;;)
	  {
	    ssize_t n = read(parent_read, buffer, sizeof(buffer));
	    if(!n) break;
	    result.write(buffer, n);
	  }
	  ::close(parent_read);
	}

	// Wait for all childs to terminate:
	string error;
	j = cmds.size()-1u;
	for(list<Command>::reverse_iterator i = cmds.rbegin(); i!=cmds.rend(); ++i, --j)
	{
	  int pid = pids[j], status;
	  for(;;)
	  {
	    if(-1 < waitpid(pid, &status, 0)) break;
	    if(errno != EINTR)
            {
              int const e = errno;
              throw runtime_error("'waitpid' failed: "+Sys::error(e));
            }
	  }
	  if(!status || !error.empty()) continue;
	  ostringstream o;
	  o << "Execution of '"<<i->cmd<<"' ";
	  if(WIFSIGNALED(status))
	  {
	    o << "was terminated by signal "<<WTERMSIG(status);
	    if(WCOREDUMP(status)) o << " (core dumped)";
	  }
	  else  o << "failed";
	  if(WIFEXITED(status)) o << " (exit status: "<<WEXITSTATUS(status)<<")";
	  error = o.str();
	}

	if(!error.empty()) throw runtime_error(error);

	return result.str();
      }
    }
  }
}
