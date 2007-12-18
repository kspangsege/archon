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

#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <stdexcept>
#include <deque>

// POSIX:
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <archon/core/sys.hpp>
#include <archon/core/file.hpp>
#include <archon/core/dir_scan.hpp>
#include <archon/core/text.hpp>


using namespace std;


namespace
{
  size_t max_cwd_buffer_size = 32767-1024;
}


namespace Archon
{
  namespace Core
  {
    namespace File
    {
      int open(string p) throw(AccessException)
      {
        int const fd = ::open(p.c_str(), O_RDONLY);
        if (fd < 0) {
          int const e = errno;
          File::throw_file_access_exception(e, "Could not open \""+p+"\" for reading: "+
                                            Sys::error(e));
        }
        return fd;
      }


      int creat(string p) throw(AccessException)
      {
        int const fd = ::creat(p.c_str(), S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
        if (fd < 0) {
          int const e = errno;
          File::throw_file_access_exception(e, "Could not open \""+p+"\" for writing: "+
                                            Sys::error(e));
        }
        return fd;
      }


      string get_cwd()
      {
	size_t buffer_size = 1024;
	char *buffer;
	for(;;) {
	  buffer = new char[buffer_size];
	  if (getcwd(buffer, buffer_size)) break;
	  if (errno != ERANGE) {
            int errnum = errno;
	    delete[] buffer;
	    throw runtime_error("Utilities::File::get_cwd: 'getcwd' failed: " +
                                Sys::error(errnum));
	  }

	  delete[] buffer;
	  buffer_size += 1024;
	  if (buffer_size > max_cwd_buffer_size)
	    throw runtime_error("System::File::get_cwd: Path is too long");
	}
	string result = buffer;
	delete[] buffer;
        if (result.empty() || result[result.size()-1] != '/') result += "/";
	return result;
      }


      string get_home_dir()
      {
	string s = Sys::getenv("HOME");
	if (s.empty()) throw runtime_error("Could not determine home directory of "
                                          "the logged in user");
        if (s.empty() || s[s.size()-1] != '/') s += "/";
	return s;
      }


      string get_temp_dir()
      {
	return "/tmp/";
      }


      void make_dir(string path)
      {
	int error = mkdir(path.c_str(), S_IRWXU|S_IRWXG|S_IRWXO);
	if (error < 0) {
          int errnum = errno;
	  throw runtime_error("'mkdir "+path+"' failed: "+Sys::error(errnum));
        }
      }


      void scan_dir(string dir_path, vector<string> &result, bool include_special)
      {
        UniquePtr<DirScanner> s(DirScanner::new_dir_scanner(dir_path, include_special).release());
        for (;;) {
          string n = s->next_entry();
          if (n.empty()) break;
          result.push_back(n);
        }
      }


      Stat::Stat(string path, bool follow_sym_links)
      {
	struct stat s;
	if(follow_sym_links ? stat(path.c_str(), &s) : lstat(path.c_str(), &s))
        {
          int const e = errno;
          throw_file_access_exception(e, "'stat' failed");
        }

	type =
	  S_ISREG(s.st_mode)  ? type_Regular :
	  S_ISDIR(s.st_mode)  ? type_Directory :
	  S_ISCHR(s.st_mode)  ? type_CharDev :
	  S_ISBLK(s.st_mode)  ? type_BlockDev :
	  S_ISFIFO(s.st_mode) ? type_Fifo :
	  S_ISLNK(s.st_mode)  ? type_SymLink :
	  S_ISSOCK(s.st_mode) ? type_Socket : type_Other;

        size = s.st_size;
      }

      Stat::Stat(int fildes)
      {
	struct stat s;
	if(fstat(fildes, &s))
        {
          int const e = errno;
	  throw runtime_error("Utilities::File::Stat::Stat: 'stat "+
                              Text::print(fildes)+"' failed: "+Sys::error(e));
        }

	type =
	  S_ISREG(s.st_mode)  ? type_Regular :
	  S_ISDIR(s.st_mode)  ? type_Directory :
	  S_ISCHR(s.st_mode)  ? type_CharDev :
	  S_ISBLK(s.st_mode)  ? type_BlockDev :
	  S_ISFIFO(s.st_mode) ? type_Fifo :
	  S_ISLNK(s.st_mode)  ? type_SymLink :
	  S_ISSOCK(s.st_mode) ? type_Socket : type_Other;
      }

      bool exists(string p)
      {
	try
	{
	  Stat s(p);
	  return true;
	}
	catch(exception &)
	{
	  return false;
	}
      }

      bool is_regular(string p)
      {
	try
	{
	  Stat s(p);
	  return s.get_type() == Stat::type_Regular;
	}
	catch(exception &)
	{
	  return false;
	}
      }

      bool is_dir(string p)
      {
	try
	{
	  Stat s(p);
	  return s.get_type() == Stat::type_Directory;
	}
	catch(exception &)
	{
	  return false;
	}
      }

      bool is_sym_link(string p)
      {
	try
	{
	  Stat s(p);
	  return s.get_type() == Stat::type_SymLink;
	}
	catch(exception &)
	{
	  return false;
	}
      }

      string name_of(string path)
      {
	string::size_type p = path.rfind('/');
	return p == string::npos ? path : string(path, p+1);
      }

      string dir_of(string path)
      {
	string::size_type p = path.rfind('/');
	return p == string::npos ? string() : string(path, 0, p+1);
      }

      string suffix_of(string path)
      {
	string name = name_of(path);
	string::size_type p = name.rfind('.');
	return p == string::npos ? "" : string(name, p+1);
      }

      string stem_of(string path)
      {
	string name = name_of(path);
	string::size_type p = name.rfind('.');
	return p == string::npos ? name : string(name, 0, p);
      }


      string resolve_path(string path, string base)
      {
        if (!path.empty() && path[0] == '/') return path; // Absolute
        string const base_name = name_of(base);
        if (base_name == "." || base_name == "..") base += "/";
        else if(!base_name.empty()) base = dir_of(base);
        return base + path;
      }


      string canonicalize_path(string path, bool reduce_double_slash)
      {
        if (path.empty()) return path;
        deque<string> segments1;
        Text::split(path, "/", back_inserter(segments1));
        // This effectively adds a trailing slash to paths that end in '.'
        // or in '..'. We do this because such paths really refer to a
        // directory.
        if (segments1.back() == "." || segments1.back() == "..") segments1.push_back("");
        // At this point we always have at least two segments.
        bool const abs = segments1.front().empty();
        bool const dir = segments1.back().empty();
        if (abs) segments1.pop_front();
        if (dir) segments1.pop_back();
        deque<string> segments2;
        int underflow = 0;
        while (!segments1.empty()) {
          string const seg = segments1.front();
          segments1.pop_front();
          if (seg.empty() && reduce_double_slash) continue;
          if (seg == ".") continue;
          if (seg == "..") {
            if(segments2.empty()) ++underflow;
            else segments2.pop_back();
            continue;
          }
          segments2.push_back(seg);
        }

        while (0 < underflow) {
          segments2.push_front("..");
          --underflow;
        }

        // At this point, segments2 can be empty only if 'dir' is true. In
        // this case, the effective path is equivalent to './'.
        if (segments2.empty()) return "";

        if (dir) segments2.push_back("");
        else if (segments2.back().empty()) segments2.push_back(".");

        if (abs) segments2.push_front("");
        else if (segments2.front().empty()) segments2.push_front(".");

        return Text::join(segments2.begin(), segments2.end(), "/");
      }


      void throw_file_access_exception(int errnum, string m)
      {
	switch(errnum)
	{
	case ENOENT:  // No such file or directory
	case ENOTDIR: // Not a directory
	  throw File::NotFoundException(m);

	case EACCES:  // Permission denied
	  throw File::PermissionException(m);

          // Temporary lack of resources
	case EMFILE:  // Too many open files
	case ENFILE:  // File table overflow
	case ENOMEM:  // Out of memory

          // Internal errors
	default:
	  throw runtime_error(m+" ("+Sys::error(errnum)+")");
        }
      }
    }
  }
}
