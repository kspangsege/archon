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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <climits>
#include <limits>

#include <archon/core/atomic.hpp>
#include <archon/core/sys.hpp>
#include <archon/core/file.hpp>
#include <archon/core/memory.hpp>
#include <archon/core/stream.hpp>


using namespace std;
using namespace archon::core;

namespace
{
  struct FileInputStream: InputStream
  {
    virtual size_t read(char *b, size_t n)
    {
      return Sys::read(fildes, b, n);
    }

    FileInputStream(int fildes, bool must_close): fildes(fildes), must_close(must_close ? 1 : 0) {}

    virtual ~FileInputStream()
    {
      try
      {
        if(must_close) Sys::close(fildes);
      }
      catch(...) {}
    }

    int const fildes;
    bool const must_close;
  };



  struct FileOutputStream: OutputStream
  {
    virtual void write(char const *b, size_t n)
    {
      size_t m = 0;
      while(m < n) m += Sys::write(fildes, b + m, n - m);
    }

    void flush() {} // Noop since there is no user space buffer involved.

    FileOutputStream(int fildes, bool must_close):
      fildes(fildes), must_close(must_close ? 1 : 0) {}


    virtual ~FileOutputStream()
    {
      try
      {
        if(must_close) Sys::close(fildes);
      }
      catch(...) {}
    }

    int const fildes;
    bool const must_close;
  };
}




namespace archon
{
  namespace core
  {
    UniquePtr<InputStream> make_stdin_stream(bool close)
    {
      UniquePtr<InputStream> s(new FileInputStream(STDIN_FILENO, close));
      return s;
    }

    UniquePtr<OutputStream> make_stdout_stream(bool close)
    {
      UniquePtr<OutputStream> s(new FileOutputStream(STDOUT_FILENO, close));
      return s;
    }

    UniquePtr<OutputStream> make_stderr_stream(bool close)
    {
      UniquePtr<OutputStream> s(new FileOutputStream(STDERR_FILENO, close));
      return s;
    }


    UniquePtr<InputStream> make_file_input_stream(int fildes, bool close)
    {
      UniquePtr<InputStream> s(new FileInputStream(fildes, close));
      return s;
    }

    UniquePtr<InputStream> make_file_input_stream(string file_name)
    {
      UniquePtr<InputStream> s(new FileInputStream(File::open(file_name), true));
      return s;
    }


    UniquePtr<OutputStream> make_file_output_stream(int fildes, bool close)
    {
      UniquePtr<OutputStream> s(new FileOutputStream(fildes, close));
      return s;
    }

    UniquePtr<OutputStream> make_file_output_stream(string file_name)
    {
      UniquePtr<OutputStream> s(new FileOutputStream(File::creat(file_name), true));
      return s;
    }
  }
}
