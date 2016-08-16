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

#ifndef ARCHON_UTIL_MIME_MAGIC_HPP
#define ARCHON_UTIL_MIME_MAGIC_HPP

#include <string>

#include <archon/core/unique_ptr.hpp>
#include <archon/core/file.hpp>


namespace archon
{
  namespace util
  {
    /**
     * A utility for determining the MIME type of files in the file
     * system using 'MIME magic'.
     *
     * Thread-safty: This class is thread-safe, and so is each
     * instance. See also \ref ThreadSafety.
     */
    struct MimeMagician
    {
      /**
       * Determine the MIME type of the specified file. If the type
       * has major type "text", the returned string will contain
       * information about the character encoding too, for example
       * 'text/plain; charset=iso-8859-1'.
       *
       * \return The MIME type of the specified file.
       */
      virtual std::string check(std::string filesys_path) const
        throw(core::file::AccessException) = 0;

      virtual ~MimeMagician() {}
    };



    /**
     * Create a new MIME magic instance.
     *
     * This function is thread-safe.
     */
    core::UniquePtr<MimeMagician> new_mime_magician();
  }
}

#endif // ARCHON_UTIL_MIME_MAGIC_HPP
