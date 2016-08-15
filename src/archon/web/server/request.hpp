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

#ifndef ARCHON_WEB_SERVER_REQUEST_HPP
#define ARCHON_WEB_SERVER_REQUEST_HPP

#include <archon/web/server/header.hpp>


namespace archon
{
  namespace web
  {
    namespace Server
    {
      enum Method
      {
        method_OPTIONS,
        method_GET,
        method_HEAD,
        method_POST,
        method_PUT,
        method_DELETE,
        method_TRACE,
        method_CONNECT
      };

      enum Protocol
      {
        protocol_HTTP_1_0,
        protocol_HTTP_1_1
      };


      struct Request
      {
        virtual Method get_method() const = 0;

        virtual std::string get_uri() const = 0;

        virtual Protocol get_protocol() const = 0;

        virtual int get_num_headers() const = 0;

        virtual Header get_header_name(int i) const = 0;

        virtual std::string get_header_value(int i) const = 0;

        virtual std::string find_header(Header h) const = 0;

        virtual ~Request() {}
      };


      namespace _Impl
      {
        struct MethodSpec   { static core::EnumAssoc map[]; };
        struct ProtocolSpec { static core::EnumAssoc map[]; };
      }

      typedef core::Enum<Method,   _Impl::MethodSpec>   MethodEnum;
      typedef core::Enum<Protocol, _Impl::ProtocolSpec> ProtocolEnum;
    }
  }
}

#endif // ARCHON_WEB_SERVER_REQUEST_HPP
