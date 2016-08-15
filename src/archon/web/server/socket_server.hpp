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

#ifndef ARCHON_WEB_SERVER_SOCKET_SERVER_HPP
#define ARCHON_WEB_SERVER_SOCKET_SERVER_HPP

#include <vector>

#include <archon/core/memory.hpp>
#include <archon/core/unique_ptr.hpp>
#include <archon/thread/condition.hpp>
#include <archon/web/server/stream.hpp>


namespace archon
{
  namespace Web
  {
    namespace Server
    {
      struct SocketServer
      {
        struct Connection;

        virtual Core::UniquePtr<Connection> new_connection() = 0;
        virtual void destroy_connection(Core::UniquePtr<Connection>) {}

        SocketServer(int p): port(p) {}

        virtual ~SocketServer() {}

        void serve();

      private:
        enum StreamState { state_Ready, state_Probe, state_Wait, state_Closed };
        struct Input;
        struct Output;
        struct Stream;

        int const port;
        typedef Core::DeletingMap<int, Connection> Connections;
        Connections connections;
        std::vector<int> pending_close;
        Thread::SelectSpec select_spec;
      };


      struct SocketServer::Input: InputStreamNew
      {
        virtual void close_input() = 0;
        void close() throw() { close_input(); }
      };


      struct SocketServer::Output: OutputStreamNew
      {
        virtual void close_output() = 0;
        void flush() throw() {}
        void close() throw() { close_output(); }
      };


      struct SocketServer::Stream: Input, Output
      {
        std::size_t read(char *buffer, std::size_t n)
          throw(Core::ReadException, Core::InterruptException);

        std::size_t write(char const *buffer, std::size_t n)
          throw(Core::WriteException, Core::InterruptException);

        void close_input();
        void close_output();

        void resume_read();
        void resume_write();

        void reset(Thread::SelectSpec *, Connection *, int fd);
        bool process();
        void try_read();
        void try_write();

        Thread::SelectSpec *select_spec;
        Connection *conn;
        int fildes;
        StreamState read_state, write_state;
        bool swap_order;
        bool async_resumption_enabled;
      };


      struct SocketServer::Connection
      {
        /**
         * Called by the server when reading from the input stream can
         * proceed.
         *
         * Reading can proceed when all of the following are true,
         * data is immediately available on the input stream for
         * reading, reading is not suspended, and the input stream is
         * not closed.
         *
         * Immediately before this method is called, reading becomes
         * suspended. It can then only be brought back from the
         * suspended state, by reading at least one byte from the
         * input stream, or by closing the input stream, or by calling
         * resume_read().
         */
        virtual void handle_read()  throw(Core::ReadException) = 0;

        /**
         * Called by the server when writing to the output stream can
         * proceed.
         *
         * Writing can proceed when all of the following are true, the
         * output stream is ready the immediately receive written
         * data, writing is not suspended, and the output stream is
         * not closed.
         *
         * Immediately before this method is called, writing becomes
         * suspended. It can then only be brought back from the
         * suspended state, by writing at least one byte to the output
         * stream, or by closing the output stream, or by calling
         * resume_write().
         */
        virtual void handle_write() throw(Core::WriteException) = 0;

        virtual ~Connection() {}


        InputStreamNew  &get_input_stream()  throw() { return stream; }
        OutputStreamNew &get_output_stream() throw() { return stream; }

        void resume_read()  throw() { stream.resume_read();  }
        void resume_write() throw() { stream.resume_write(); }


      private:
        friend class SocketServer;
        Stream stream;
      };






      // Implementation:

      inline void SocketServer::Stream::resume_read()
      {
        if(read_state == state_Wait)
        {
          select_spec->read_in.insert(fildes);
          read_state = state_Ready;
        }
        else if(read_state == state_Probe) read_state = state_Ready;
      }

      inline void SocketServer::Stream::resume_write()
      {
        if(write_state == state_Wait)
        {
          select_spec->write_in.insert(fildes);
          write_state = state_Ready;
        }
        else if(write_state == state_Probe) write_state = state_Ready;
      }
    }
  }
}

#endif // ARCHON_WEB_SERVER_SOCKET_SERVER_HPP
