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

/// \file
///
/// \author Kristian Spangsege

#ifndef ARCHON_WEB_SERVER_SERVER_HPP
#define ARCHON_WEB_SERVER_SERVER_HPP

#include <memory>
#include <string>

#include <archon/web/server/resolver.hpp>


namespace archon {
namespace web {
namespace server {

class Server;

std::unique_ptr<Server> make_default_server(int port);


/// When this server runs, it will occasionally write to a socket that is closed
/// by the client. This causes the system to send a SIGPIPE signal to this
/// process, and the default behavior when such a signal is received, is to
/// instantly terminate the process. This is inconvenient for a web server, and
/// therfore the application is advised to disable this behavior, for example as
/// follows:
///
/// <pre>
///
///   #include <archon/core/sys.hpp>
///   core::sys::signal::ignore_signal(SIGPIPE);
///
/// </pre>
class Server {
public:
    /// The ownership of the Resolver remains with the caller.
    virtual void register_context(std::string path, Resolver*) = 0;

    /// Not thread-safe.
    virtual void serve() = 0;

    virtual ~Server() {}
};

} // namespace server
} // namespace web
} // namespace archon

#endif // ARCHON_WEB_SERVER_SERVER_HPP
