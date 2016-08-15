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

#ifndef ARCHON_CORE_DIR_SCAN_HPP
#define ARCHON_CORE_DIR_SCAN_HPP

#include <string>

#include <archon/core/unique_ptr.hpp>
#include <archon/core/file.hpp>


namespace archon
{
  namespace Core
  {
    struct DirScanner
    {
      struct Exception;

      /**
       * Construct a new directory scanner.
       *
       * \param path The file system path of the directory to scan.
       *
       * \param include_special Set to true if you want the special
       * entries "." and ".." to be included.
       *
       * \return A new scanner object.
       *
       * \throw File::NotFoundException When the directory could not
       * be found or it was not a directory.
       *
       * \throw File::PermissionException When access to the directory
       * is forbidden.
       *
       * \note Ownership of the returned scanner object is passed to
       * the caller.
       */
      static UniquePtr<DirScanner> new_dir_scanner(std::string path, bool include_special=false);

      /**
       * Get the underlying file descriptor if possible.
       *
       * \return The file descriptor or -1 if this information is not
       * available.
       */
      virtual int get_file_descriptor() const = 0;

      /**
       * Get the name of the next directory entry.
       *
       * \return The name of the next directory entry, or the empty
       * string if all entries have been returned.
       *
       * \throw std::runtime_error If reading the directory fails.
       */
      virtual std::string next_entry() = 0;

      virtual ~DirScanner() {}
    };
  }
}

#endif // ARCHON_CORE_DIR_SCAN_HPP
