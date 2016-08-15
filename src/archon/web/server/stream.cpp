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

#include <archon/core/sys.hpp>
#include <archon/core/file.hpp>
#include <archon/web/server/stream.hpp>


using namespace std;
using namespace archon::core;


namespace archon
{
  namespace web
  {
    namespace Server
    {
      void FileInputStream::open(string p)
      {
        fildes = File::open(p);
        must_close = true;
        closed = false;
      }

      void FileInputStream::open(int fd, bool close)
      {
        fildes = fd;
        must_close = close;
        closed = false;
      }

      FileInputStream::size_type FileInputStream::read(char_type *b, size_type n)
        throw(ReadException, InterruptException)
      {
        if(closed) throw ReadException("Reading from closed stream");
        return Sys::read(fildes, b, n);
      }

      void FileInputStream::close() throw()
      {
        if(closed) return;
        if(must_close) Sys::close(fildes);
        closed = true;
      }
    }
  }
}
