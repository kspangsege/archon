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

// ISO/ANSI C++
#include <cerrno>
#include <stdexcept>
#include <iostream>

// POSIX
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <archon/core/sys.hpp>
#include <archon/thread/thread.hpp>
#include <archon/web/server/socket_server.hpp>


using namespace std;
using namespace archon::Core;
using namespace archon::Thread;


namespace
{
  /**
   * \param s Socket file descriptor
   *
   * \param p Port number.
   */
  void bind(int s, int p)
  {
    struct sockaddr_in a;
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_ANY);
    a.sin_port = htons(p);
    int const r = ::bind(s, reinterpret_cast<sockaddr *>(&a), sizeof(a));
    if(r == -1)
    {
      int const e = errno;
      throw runtime_error("'bind' failed: "+Sys::error(e));
    }
  }


  /**
   * Open a socket on the specified port, and make it non-blocking and
   * listening.
   *
   * \param port Port number.
   */
  int listen(int port)
  {
    int const s = socket(PF_INET, SOCK_STREAM, 0);
    if(s == -1)
    {
      int const e = errno;
      throw runtime_error("'socket' failed: "+Sys::error(e));
    }
    Sys::nonblock(s);
    bind(s, port);
    int const r = ::listen(s, 10);
    if(r == -1)
    {
      int const e = errno;
      throw runtime_error("'listen' failed: "+Sys::error(e));
    }
    return s;
  }
}



namespace archon
{
  namespace Web
  {
    namespace Server
    {
      void SocketServer::serve()
      {
        int const socket = listen(port);
        select_spec.read_in.insert(socket);
        cerr << "Listening on port: " << port << endl;

        for(;;)
        {
          ::Thread::select(select_spec);

          // Check for new conections
          if(select_spec.read_out.find(socket) != select_spec.read_out.end())
          {
            for(;;)
            {
              int const fd = accept(socket, 0, 0);
              if(fd == -1)
              {
                int const e = errno;
                if(e == EAGAIN) break;
                throw runtime_error("Failed to accept socket connection: "+Sys::error(e));
              }
              Sys::nonblock(fd);
              UniquePtr<Connection> conn(new_connection().release());
              conn->stream.reset(&select_spec, conn.get(), fd);
              cerr << "Open connection " << fd << endl;
              connections.set_at(fd, conn);
              select_spec.read_in.insert(fd);
              select_spec.write_in.insert(fd);
            }
          }

          Connections::iterator i = connections.begin();
          while(i != connections.end())
          {
            Connection *const conn = i->second;
            if(conn->stream.process()) ++i;
            else
            {
              int const fd = i->first;
              cerr << "Close connection " << fd << endl;
              UniquePtr<Connection> c(conn);
              i->second = 0;
              connections.erase(i++);
              Sys::close(fd);
              destroy_connection(c);
            }
          }
        }
      }


      size_t SocketServer::Stream::read(char *b, size_t n)
        throw(ReadException, InterruptException)
      {
        if(read_state == state_Closed || n == 0) return 0;
        size_t const m = Sys::read(fildes, b, n);
        if(read_state == state_Wait) select_spec->read_in.insert(fildes);
        read_state = state_Ready;
        return m;
      }


      size_t SocketServer::Stream::write(char const *b, size_t n)
        throw(WriteException, InterruptException)
      {
        if(write_state == state_Closed || n == 0) return 0;
        size_t const m = Sys::write(fildes, b, n);
        if(write_state == state_Wait) select_spec->write_in.insert(fildes);
        write_state = state_Ready;
        return m;
      }


      void SocketServer::Stream::close_input()
      {
        if(read_state == state_Probe ||
           read_state == state_Ready) select_spec->read_in.erase(fildes);
        read_state = state_Closed;
      }


      void SocketServer::Stream::close_output()
      {
        if(write_state == state_Probe ||
           write_state == state_Ready) select_spec->write_in.erase(fildes);
        write_state = state_Closed;
      }


      void SocketServer::Stream::reset(Thread::SelectSpec *s, Connection *c, int fd)
      {
        select_spec = s;
        conn = c;
        fildes = fd;
        read_state = write_state = state_Ready;
        swap_order = false;
        async_resumption_enabled = false;
      }


      bool SocketServer::Stream::process()
      {
        bool const can_read  = select_spec->read_out.find(fildes)  != select_spec->read_out.end();
        bool const can_write = select_spec->write_out.find(fildes) != select_spec->write_out.end();

        try
        {
          if(can_read)
          {
            if(can_write)
            {
              if(swap_order)
              {
                try_write();
                bool const wait = write_state == state_Wait;
                try_read();
                if(wait && write_state != state_Wait) { swap_order = false; cerr << "RESTORING READ/WRITE ORDER" << endl; }
              }
              else
              {
                try_read();
                bool const wait = read_state == state_Wait;
                try_write();
                if(wait && read_state != state_Wait) { swap_order = true; cerr << "REVERSING READ/WRITE ORDER" << endl; }
              }
            }
            else try_read();
          }
          else if(can_write) try_write();
        }
        catch(IOException &e)
        {
          cerr << "I/O error: "<<e.what() << endl;
          select_spec->read_in.erase(fildes);
          select_spec->write_in.erase(fildes);
          return false;
        }

        bool const close = write_state == state_Closed && read_state == state_Closed;
        if(!close && !async_resumption_enabled &&
           read_state != state_Ready && write_state != state_Ready)
          throw runtime_error("Connection implementation malfunction: Dead-locked");
        return !close;
      }


      void SocketServer::Stream::try_read()
      {
        read_state = state_Probe;
        conn->handle_read();
        if(read_state == state_Probe)
        {
          read_state = state_Wait;
          select_spec->read_in.erase(fildes);
        }
      }


      void SocketServer::Stream::try_write()
      {
        write_state = state_Probe;
        conn->handle_write();
        if(write_state == state_Probe)
        {
          write_state = state_Wait;
          select_spec->write_in.erase(fildes);
        }
      }
    }
  }
}
