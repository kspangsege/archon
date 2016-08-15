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
 *
 * Testing the socket server.
 */

#include <iostream>

#include <archon/core/options.hpp>
#include <archon/util/socket_server.hpp>


using namespace std;
using namespace archon::Core;
using namespace archon::Web::Server;


namespace
{
  namespace Pipe
  {
    struct First: SocketServer::Connection
    {
    };

    struct Server: SocketServer
    {
      UniquePtr<Connection> open_connection()
      {
        UniquePtr<Connection> conn;
        if(!first)
        {
          first = new First();
          conn.reset(first);
        }
        else
        {
          conn.reset(new Second(first));
        }
        return conn;
      }

      First *first;
    };
  }
}


int main(int argc, char const *argv[]) throw()
{
  int opt_port = 8008;

  CommandlineOptions o;
  o.add_help("Test Application for the Archon web server");
  o.check_num_args();

  o.add_param("p", "port", opt_port, "Select the port number to bind to");

  if(int stop = o.process(argc, argv)) return stop == 2 ? 0 : 1;

  Sys::Signal::ignore_signal(SIGPIPE); // Required by the socket server

  Server server;

  server.bind(opt_port);

  return 0;
}
