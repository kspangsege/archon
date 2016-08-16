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
 * Reading a directory turns out to be quite a challenge when it must
 * be done safely and in a reasonably portable way. One must consider
 * both vulnerability to buffer overflow and thread safty.
 *
 * Ideally we should be able to read directly from an open file
 * descriptor, since that would provide the greatest flexibility, but
 * support for this is simply too sparse. The 'fdopendir' call
 * supported by Solaris seems to be the way we would want to go, but
 * for now we will have to stay with the ordinary 'path' argument.
 *
 * Thanks to Ben Hutchings <ben@decadentplace.org.uk> for his very
 * thorough treatment of these issues - most of the code below is
 * taken from his article "readdir_r considered harmful".
 *
 * \sa  http://womble.decadentplace.org.uk/readdir_r-advisory.html
 */

#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <climits>
#include <cstddef>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <iostream>

#include <archon/platform.hpp> // Never include in other header files
#include <archon/core/assert.hpp>
#include <archon/core/sys.hpp>
#include <archon/core/file.hpp>
#include <archon/core/dir_scan.hpp>


using namespace std;
using namespace archon::core;


namespace
{
  /**
   * Calculate the required buffer size (in bytes) for directory
   * entries read from the given directory handle.  Return -1 if this
   * this cannot be done.
   *
   * This code does not trust values of NAME_MAX that are less than
   * 255, since some systems (including at least HP-UX) incorrectly
   * define it to be a smaller value.
   *
   * If you use autoconf, include fpathconf and dirfd in your
   * AC_CHECK_FUNCS list.  Otherwise use some other method to detect
   * and use them where available.
   */
#if defined(HAVE_FPATHCONF) && defined(HAVE_DIRFD) && defined(_PC_NAME_MAX)
  size_t dirent_buf_size(DIR *dirp)
  {
    ssize_t name_max;
    name_max = fpathconf(dirfd(dirp), _PC_NAME_MAX);
    if(name_max == -1)
#if defined(NAME_MAX)
      name_max = max(NAME_MAX, 255);
#else
      return 0;
#endif
    return max(offsetof(struct dirent, d_name) + name_max + 1, sizeof(struct dirent));
  }
#elif defined(NAME_MAX)
  size_t dirent_buf_size(DIR *)
  {
    ssize_t name_max = max(NAME_MAX, 255);
    return max(offsetof(struct dirent, d_name) + name_max + 1, sizeof(struct dirent));
  }
#else
#error "Cannot determone size of 'struct dirent'"
#endif

  struct DirScannerImpl: DirScanner
  {
    int get_file_descriptor() const
    {
#if defined(HAVE_DIRFD)
      return dirfd(dirp);
#else
      return -1;
#endif
    }

    string next_entry()
    {
      if(!entry) return "";
      struct dirent *result;
    next:
      int e = readdir_r(dirp, entry, &result);
      if(e != 0) throw runtime_error("'readdir_r' failed: "+sys::error(e));
      if(result)
      {
        string const n = result->d_name;
        if(!include_special && (n == "." || n == "..")) goto next;
        return n;
      }
      delete[] reinterpret_cast<char *>(entry);
      entry = 0;
      return "";
    }

    DirScannerImpl(string p, bool include_special): include_special(include_special)
    {
      dirp = opendir(p.c_str());
      if(!dirp)
      {
        int errnum = errno;
        file::throw_file_access_exception(errnum, "'opendir' failed: "+sys::error(errnum));
      }
      size_t size = dirent_buf_size(dirp);
      if(!size) throw runtime_error("Cannot determine size of 'struct dirent'");
      entry = reinterpret_cast<struct dirent *>(new char[size]);
    }

    ~DirScannerImpl()
    {
      delete[] reinterpret_cast<char *>(entry);
      int ret = closedir(dirp);
      ARCHON_ASSERT(ret != -1);
    }

    bool const include_special;
    DIR *dirp;
    struct dirent *entry;
  };
}


namespace archon
{
  namespace core
  {
    UniquePtr<DirScanner> DirScanner::new_dir_scanner(string path, bool include_special)
    {
      UniquePtr<DirScanner> s(new DirScannerImpl(path, include_special));
      return s;
    }
  }
}
