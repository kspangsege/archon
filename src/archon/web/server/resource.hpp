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

#ifndef ARCHON_WEB_SERVER_RESOURCE_HPP
#define ARCHON_WEB_SERVER_RESOURCE_HPP

#include <stdexcept>
#include <string>

#include <archon/core/memory.hpp>
#include <archon/web/server/stream.hpp>
#include <archon/web/server/request.hpp>
#include <archon/web/server/response.hpp>


namespace Archon
{
  namespace Web
  {
    namespace Server
    {
      struct RequestException: std::runtime_error
      {
        RequestException(Status s, std::string msg = std::string()):
          std::runtime_error(msg), status(s) {}
        RequestException(std::string msg = std::string()):
          std::runtime_error(msg), status(status_400_Bad_Request) {}
        Status status;
      };



      struct Resource
      {
        /**
         * The default implementation does nothing.
         */
        virtual void activate(Request &req, Response &res) throw(RequestException);


        /**
         * Called by the server whenever data is available for
         * reading.
         *
         * If the resource chooses to not read at least one byte from
         * the passed stream, the server will assume that the resource
         * is not currently able to consume any more input, and
         * therefor it will not call this method again until write()
         * has written at least one byte to the output stream.
         *
         * If the resource does not want this method to be called
         * ever again, it should close the passed stream. The stream is
         * automatically closed at end-of-input.
         *
         * The default implementation will simply close the stream.
         */
        virtual void read(InputStreamNew &in) throw(ReadException) { in.close(); }

        /**
         * Called by the server when it is ready to accept further
         * writing.
         *
         * If the resource chooses to not write at least one byte to
         * the passed stream, the server will assume that the resource
         * currently has nothing more to write, and therefor it will
         * not call this method again until read() has read at least
         * one byte from the input stream.
         *
         * If the resource does not want this method to be called ever
         * again, it should close the passed stream.
         *
         * The default implementation will simply close the stream.
         */
        virtual void write(OutputStreamNew &out) throw(WriteException) { out.close(); }


        virtual ~Resource() throw() {}
      };



      struct PreparedTextResource: Resource
      {
        PreparedTextResource(std::string text): size(text.size()), data(size), pos(0)
        {
          text.copy(data.get(), size);
        }

        void write(OutputStreamNew &out) throw(WriteException)
        {
          pos += out.write(data.get()+pos, size-pos);
          if(pos == size) out.close();
        }

        size_t const size;
        Core::MemoryBuffer const data;
        size_t pos;
      };







      // Implementation:

      inline void Resource::activate(Request &, Response &) throw(RequestException) {}
    }
  }
}

#endif // ARCHON_WEB_SERVER_RESOURCE_HPP
