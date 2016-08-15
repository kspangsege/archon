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

#ifndef ARCHON_WEB_SERVER_RESOLVER_HPP
#define ARCHON_WEB_SERVER_RESOLVER_HPP

#include <string>

#include <archon/core/unique_ptr.hpp>
#include <archon/web/server/resource.hpp>


namespace archon
{
  namespace web
  {
    namespace Server
    {
      struct Resolver
      {
        virtual core::UniquePtr<Resource> resolve(std::string path) throw(RequestException) = 0;

        virtual ~Resolver() {}
      };



      template<class ResourceImpl> struct SimpleResolver: Resolver
      {
        core::UniquePtr<Resource> resolve(std::string path) throw(RequestException)
        {
          core::UniquePtr<Resource> r(new ResourceImpl(path));
          return r;
        }
      };
    }
  }
}

#endif // ARCHON_WEB_SERVER_RESOLVER_HPP
