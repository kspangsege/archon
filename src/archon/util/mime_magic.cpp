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

#include <archon/platform.hpp> // Never include in other header files
#include <archon/core/text.hpp>
#include <archon/core/sys.hpp>
#include <archon/core/mutex.hpp>
#include <archon/util/mime_magic.hpp>

#ifdef ARCHON_HAVE_LIBMAGIC

#include <cerrno>
#include <stdexcept>

#include <magic.h>

#else // ! ARCHON_HAVE_LIBMAGIC

#include <utility>
#include <map>

#include <archon/core/char_enc.hpp>

#endif // ARCHON_HAVE_LIBMAGIC


using namespace std;
using namespace archon::core;
using namespace archon::Util;


namespace
{
#ifdef ARCHON_HAVE_LIBMAGIC

  struct Checker
  {
    Checker()
    {
      Sys::GlobalLock l;
      cookie = magic_open(MAGIC_MIME_TYPE | MAGIC_MIME_ENCODING | MAGIC_SYMLINK | MAGIC_ERROR);
      if(!cookie)
      {
        int const e = errno;
        throw runtime_error("'magic_open' failed: "+Sys::error(e));
      }
      int const r = magic_load(cookie, 0);
      if(r < 0) throw runtime_error("'magic_load' failed: "+string(magic_error(cookie)));
    }

    ~Checker()
    {
      Sys::GlobalLock l;
      magic_close(cookie);
    }

    string check(string p) const throw(File::AccessException)
    {
      string type;
      {
        Sys::GlobalLock l;
        const char *const r = magic_file(cookie, p.c_str());
        if(!r)
        {
          int const e = magic_errno(cookie);
          if(e != 0) File::throw_file_access_exception(e, "'magic_file' failed");
          throw runtime_error("'magic_file' failed: "+string(magic_error(cookie)));
        }
        type = r;
      }
      if(Text::is_prefix<char>("text/", type)) return type;
      string const pre = Text::get_prefix<char>(";", type);
      return pre.empty() ? type : pre;
    }

    magic_t cookie;
  };

#else // ! ARCHON_HAVE_LIBMAGIC

  struct Checker
  {
    Checker():
      fallback("application/octet-stream"),
      charenc_part("; charset="+ascii_tolower(Sys::get_env_locale_charenc()))
    {
      add("txt",  "text/plain");
      add("xml",  "text/xml");
      add("xsl",  "text/xml"); // XML stylesheet language
      add("xsd",  "text/xml"); // XML schema definition
      add("htm",  "text/html");
      add("html", "text/html");
      add("js",   "text/javascript");
      add("css",  "text/css");

      add("wrl",  "model/vrml");
      add("vrml", "model/vrml");

      add("png",  "image/png");
      add("gif",  "image/gif");
      add("jpg",  "image/jpeg");
      add("jpeg", "image/jpeg");
      add("tif",  "image/tiff");
      add("tiff", "image/tiff");

      add("oga",  "audio/ogg");
      add("ogg",  "audio/ogg");
      add("spx",  "audio/ogg");
      add("mpga", "audio/mpeg");
      add("mp1",  "audio/mpeg");
      add("mp2",  "audio/mpeg");
      add("mp3",  "audio/mpeg");

      add("ogv",  "video/ogg");
      add("mp4",  "video/mp4");
      add("mpg4", "video/mp4");
      add("mpeg", "video/mpeg");
      add("mpg",  "video/mpeg");
      add("mpe",  "video/mpeg");

      add("pdf",  "application/pdf");
    }

    void add(string ext, string mime_type)
    {
      types.insert(make_pair(ext, mime_type));
    }

    string check(string p) const throw(File::AccessException)
    {
      string const s = File::suffix_of(p);
      if(s.empty()) return fallback;
      Types::const_iterator const i = types.find(ascii_tolower(s));
      if(i == types.end()) return fallback;
      string const t = i->second;
      return Text::is_prefix<char>("text/", t) ? t+charenc_part : t;
    }

    string const fallback;
    string const charenc_part;
    typedef map<string, string> Types;
    Types types;
  };

#endif // ARCHON_HAVE_LIBMAGIC


  struct Manager
  {
    Manager(): use_count(0) {}

    void acquire()
    {
      Mutex::Lock l(mutex);
      if(use_count == 0) checker.reset(new Checker());
      ++use_count;
    }

    void release()
    {
      Mutex::Lock l(mutex);
      if(--use_count == 0) checker.reset();
    }

    Mutex mutex;
    UniquePtr<Checker> checker;
    size_t use_count;
  };


  Manager &get_manager()
  {
    static Manager m;
    return m;
  }


  struct MagicProxy: MimeMagician
  {
    MagicProxy()  { get_manager().acquire(); }
    ~MagicProxy() { get_manager().release(); }

    string check(string p) const throw(File::AccessException)
    {
      return get_manager().checker->check(p);
    }
  };
}


namespace archon
{
  namespace Util
  {
    UniquePtr<MimeMagician> new_mime_magician()
    {
      UniquePtr<MimeMagician> m(new MagicProxy());
      return m;
    }
  }
}
