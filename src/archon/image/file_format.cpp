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

#include <vector>
#include <map>

#include <archon/platform.hpp> // Never include in other header files
#include <archon/image/file_format.hpp>


using namespace std;
using namespace Archon::Core;
using namespace Archon::Imaging;


// Declarations of available defaul file format getters
namespace Archon
{
  namespace Imaging
  {
    FileFormat::ConstRef get_default_gif_file_format();
#ifdef ARCHON_HAVE_LIBPNG
    FileFormat::ConstRef get_default_png_file_format();
#endif
#ifdef ARCHON_HAVE_LIBJPEG
    FileFormat::ConstRef get_default_jpeg_file_format();
#endif
  }
}


namespace
{
  struct RegistryImpl: FileFormat::Registry
  {
    int get_num_formats() const
    {
      return formats.size();
    }

    FileFormat::ConstRef get_format(int index) const
    {
      return formats.at(index);
    }
 
    FileFormat::ConstRef lookup(std::string name) const
    {
      map<string, FileFormat::ConstRef>::const_iterator i = format_map.find(name);
      if(i == format_map.end()) return CntRefNullTag();
      return i->second;
    }

    void register_format(FileFormat::ConstRefArg format)
    {
      string name = format->get_name();
      formats.push_back(format);
      if(lookup(name)) throw invalid_argument("A format is already registered as '"+name+"'");
      format_map[name] = format;
    }

    vector<FileFormat::ConstRef> formats;
    map<string, FileFormat::ConstRef> format_map;
  };

  struct DefaultRegistry: RegistryImpl
  {
    DefaultRegistry()
    {
      register_format(get_default_gif_file_format());
#ifdef ARCHON_HAVE_LIBPNG
      register_format(get_default_png_file_format());
#endif
#ifdef ARCHON_HAVE_LIBJPEG
      register_format(get_default_jpeg_file_format());
#endif
    }
  };
}


namespace Archon
{
  namespace Imaging
  {
    FileFormat::Registry::ConstRef FileFormat::Registry::get_default_registry()
    {
      static ConstRef r(new DefaultRegistry());
      return r;
    }

    FileFormat::Registry::Ref
    FileFormat::Registry::new_registry(Registry::ConstRefArg base)
    {
      Ref r(new RegistryImpl());
      int n = base->get_num_formats();
      for(int i=0; i<n; ++i) r->register_format(base->get_format(i));
      return r;
    }
  }
}
