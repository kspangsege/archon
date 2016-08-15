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

#include <archon/core/file.hpp>
#include <archon/core/char_enc.hpp>
#include <archon/util/stream.hpp>
#include <archon/image/imageio.hpp>


using namespace std;
using namespace archon::core;
using namespace archon::Util;

namespace archon
{
  namespace Imaging
  {
    namespace ImageIO
    {
      BufferedImage::Ref load(InputStream &in,
                              string source_name, string format_name,
                              Logger *logger,
                              FileFormat::ProgressTracker *tracker,
                              FileFormat::Registry::ConstRefArg r)
      {
        FileFormat::Registry::ConstRef registry = r;
        if(!registry) registry = FileFormat::Registry::get_default_registry();

        RewindableStream rewindable(in);

        // Primary auto-detection
        if(format_name.empty())
          for(int i=0; i<registry->get_num_formats(); ++i)
          {
            FileFormat::ConstRef f = registry->get_format(i);
            bool s = f->check_signature(rewindable);
            rewindable.rewind();
            if(!s) continue;
            format_name = f->get_name();
            break;
          }
        rewindable.release();

        // Secondary auto-detection
        if(format_name.empty())
        {
          string suffix = ascii_tolower(File::suffix_of(source_name));
          if(!suffix.empty())
            for(int i=0; i<registry->get_num_formats(); ++i)
            {
              FileFormat::ConstRef f = registry->get_format(i);
              if(!f->check_suffix(suffix)) continue;
              format_name = f->get_name();
              break;
            }
        }

        if(format_name.empty())
          throw UnresolvableFormatException("Image format could not be detected from the initial "
                                            "data nor from the file name: \""+source_name+"\"");

        for(int i=0; i<registry->get_num_formats(); ++i)
        {
          FileFormat::ConstRef f = registry->get_format(i);
          if(format_name != f->get_name()) continue;
          BufferedImage::Ref j = f->load(rewindable, logger, tracker);
          return j;
        }

        throw UnknownFormatException("Unrecognized format specifier: \""+format_name+"\"");
      }


      BufferedImage::Ref load(string file_path, string format_name, Logger *logger,
                              FileFormat::ProgressTracker *tracker,
                              FileFormat::Registry::ConstRefArg r)
      {
        return load(*make_file_input_stream(file_path),
                    file_path, format_name, logger, tracker, r);
      }



      void save(Image::ConstRefArg image, OutputStream &out,
		string target_name, string format_name,
		Logger *logger, FileFormat::ProgressTracker *tracker,
		FileFormat::Registry::ConstRefArg r)
      {
        FileFormat::Registry::ConstRef registry = r;
        if(!registry) registry = FileFormat::Registry::get_default_registry();

        if(format_name.empty())
        {
          // Determine format by suffix
          string suffix = ascii_tolower(File::suffix_of(target_name));
          if(!suffix.empty())
            for(int i=0; i<registry->get_num_formats(); ++i)
            {
              FileFormat::ConstRef f = registry->get_format(i);
              if(!f->check_suffix(suffix)) continue;
              format_name = f->get_name();
              break;
            }
        }

        if(format_name.empty())
          throw UnresolvableFormatException("Image format could not be "
                                            "detected from the file name"
                                            ": \"" + target_name + "\"");

        for(int i=0; i<registry->get_num_formats(); ++i)
        {
          FileFormat::ConstRef f = registry->get_format(i);
          if(format_name != f->get_name()) continue;
          f->save(image, out, logger, tracker);
          out.flush();
          return;
        }

        throw UnknownFormatException("Unrecognized format "
                                     "specifier: \""+format_name+"\"");
      }


      void save(Image::ConstRefArg image,
                string file_path, string format_name,
		Logger *logger, FileFormat::ProgressTracker *tracker,
		FileFormat::Registry::ConstRefArg r)
      {
        save(image, *make_file_output_stream(file_path),
             file_path, format_name, logger, tracker, r);
      }
    }
  }
}
