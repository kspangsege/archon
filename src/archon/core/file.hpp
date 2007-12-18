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

#ifndef ARCHON_CORE_FILE_HPP
#define ARCHON_CORE_FILE_HPP

#include <stdexcept>
#include <vector>
#include <string>

// POSIX:
#include <sys/types.h>


namespace Archon
{
  namespace Core
  {
    /**
     * Functions for working with the file system.
     */
    namespace File
    {
      struct AccessException: std::runtime_error
      {
        AccessException(std::string m): std::runtime_error(m) {}
      };

      struct NotFoundException;
      struct PermissionException;


      /**
       * Open the spefified file for reading.
       *
       * \return A file descriptor.
       */
      int open(std::string filesys_path) throw(AccessException);

      /**
       * Open the spefified file for writing.
       *
       * \return A file descriptor.
       */
      int creat(std::string filesys_path) throw(AccessException);


      /**
       * Get the current working directory.
       *
       * This will always be in the form of an absolute path, and
       * always with each segment delimited by slashes, and always
       * with a final slash.
       */
      std::string get_cwd();

      /**
       * Get the path to the users home directory. The returned path
       * will always have a final slash.
       */
      std::string get_home_dir();

      /**
       * Get the path to the directory that holds temporary files. The
       * returned path will always have a final slash.
       */
      std::string get_temp_dir();

      /**
       * Create the specified directory.
       */
      void make_dir(std::string dir_path);

      /**
       * Get a list of all the entries in the specified
       * directory. This includes files and subdirectories and any
       * other type of file system entry.
       *
       * \param dir_path the path of the direcory to scan.
       *
       * \param result The names of the directory entries will be
       * added to this vector. It is not cleared initially by this
       * method. The path is not included in the returned names.
       *
       * \param include_special Set to true if you want the special
       * entries "." and ".." to be included.
       *
       * \note This is just a wrapper around
       * <tt>Core::DirScanner</tt>.
       */
      void scan_dir(std::string dir_path, std::vector<std::string> &result, bool include_special = false);


      /**
       * An instance of this class represents an inquiry into the type
       * of a specific file system node.
       */
      struct Stat
      {
	enum Type
	{
	  type_Regular,
	  type_Directory,
	  type_CharDev,
	  type_BlockDev,
	  type_Fifo,
	  type_SymLink,
	  type_Socket,
	  type_Other
	};

        /**
         * Get the type of the file system node that this \c Stat
         * object represents.
         */
	Type get_type() const { return type; }

        off_t get_size() const { return size; }

        /**
         * Get information about the directory entry which the
         * specified path designates.
         */
	Stat(std::string path, bool follow_sym_links = true);

        /**
         * Get information about the specified open file descriptor.
         */
	Stat(int fildes);

      private:
	Type type;
        off_t size;
      };


      /**
       * Returns true iff the specified directory entry exists.
       */
      bool exists(std::string path);

      /**
       * Returns true iff the specified directory entry is a
       * regular file.
       */
      bool is_regular(std::string path);

      /**
       * Returns true iff the specified directory entry is a
       * directory.
       */
      bool is_dir(std::string path);

      /**
       * Returns true iff the specified directory entry is a symbolic
       * link.
       */
      bool is_sym_link(std::string path);

      /**
       * Extract the name part of the specified file system path. The
       * name part is defined as the portion of the path following the
       * last slash '/' or the whole path if it contains no slashes.
       *
       * \param path An absolute or a relative file system path. Note
       * that a simple file name is an example of a relative path.
       *
       * \return The name part of the path.
       */
      std::string name_of(std::string path);

      /**
       * Extract the directory part of the specified file system
       * path. The directory part is defined as the portion of the
       * path up to and including the last slash '/' or the empty
       * string if it contains no slashes.
       *
       * \param path An absolute or a relative file system path. Note
       * that a simple file name is an example of a relative path.
       *
       * \return The directory part of the path.
       */
      std::string dir_of(std::string path);

      /**
       * Extract the suffix (or file type) from the file name (or
       * directory name) of the specified file system path. The suffix
       * is defined as the portion of a path following the last dot of
       * the name part. If the name part does not contain a dot, the
       * suffix is the empty string.
       *
       * \param path An absolute or a relative file system path. Note
       * that a simple file name is an example of a relative path.
       *
       * \return The suffix of the name part.
       */
      std::string suffix_of(std::string path);

      /**
       * Extract the stem from the file name (or directory name) of
       * the specified file system path. The stem is defined as the
       * remainder of the name part after removing the suffix and the
       * precedding dot (see <tt>suffix_of</tt>). If there is no dot
       * in the name part, it is equal to the name part.
       *
       * \param path An absolute or a relative file system path. Note
       * that a simple file name is an example of a relative path.
       *
       * \return The stem of the name part.
       */
      std::string stem_of(std::string path);


      /**
       * Resolve the specified path relative to the specified base
       * directory.
       *
       * If the specified path is absolute, it is returned without any
       * modifications, otherwise the result is the concatenation of
       * the effective base path and the specified path.
       *
       * If the specified base path is not empty and does not have a
       * final slash, it is turned into a path with a final slash as
       * follows: If the final slash delimited segment of the path is
       * '.' or '..', a slash is added, otherwise the final segment is
       * removed.
       *
       * The specified path is understood as being absolute if, and
       * only if it constains an initial slash.
       *
       * An empty string is understood as having the same meaning as
       * the relative path './'. Empty strings may be passed for both
       * the path and the base path.
       */
      std::string resolve_path(std::string path, std::string base);


      /**
       * An empty string is understood as having the same meaning as
       * the relative path './'. This function will return the empty
       * string whenever the specified path has the same meaning as
       * './'.
       *
       * In a URI, for example, it is generally not legal to replace a
       * pair of consecutive slashes with a single slash. In such
       * cases pass false for 'reduce_double_slash'.
       */
      std::string canonicalize_path(std::string path, bool reduce_double_slash = true);


      /**
       * Throw the appropriate exception for the specified system
       * error number assuming that the error was due to an attempt to
       * access a file.
       *
       * \param errnum The value of the \c errno variable as it was
       * right after the failure.
       *
       * \param m The message to be passed to the exception.
       */
      void throw_file_access_exception(int errnum, std::string m);


      /**
       * Path could not be resolved to a file or directory. Either the
       * file itself is missing, or a directory corresponding to a
       * path segment is missing.
       */
      struct NotFoundException: AccessException
      {
        NotFoundException(std::string m): AccessException(m) {}
      };

      /**
       * File may exist but access is forbidden either on the file
       * itself or somewhere in the path.
       */
      struct PermissionException: AccessException
      {
        PermissionException(std::string m): AccessException(m) {}
      };
    }
  }
}

#endif // ARCHON_CORE_FILE_HPP
