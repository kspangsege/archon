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

#include <archon/web/server/request.hpp>


using namespace Archon::Core;


namespace Archon
{
  namespace Web
  {
    namespace Server
    {
      namespace _Impl
      {
        EnumAssoc MethodSpec::map[] =
        {
          { method_OPTIONS, "OPTIONS" },
          { method_GET,     "GET"     },
          { method_HEAD,    "HEAD"    },
          { method_POST,    "POST"    },
          { method_PUT,     "PUT"     },
          { method_DELETE,  "DELETE"  },
          { method_TRACE,   "TRACE"   },
          { method_CONNECT, "CONNECT" },
          { 0, 0 }
        };


        EnumAssoc ProtocolSpec::map[] =
        {
          { protocol_HTTP_1_0, "HTTP/1.0" },
          { protocol_HTTP_1_1, "HTTP/1.1" },
          { 0, 0 }
        };
      }
    }
  }
}
