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

#ifndef ARCHON_FONT_UTIL_HPP
#define ARCHON_FONT_UTIL_HPP

#include <string>
#include <ostream>

#include <archon/core/unique_ptr.hpp>
#include <archon/core/series.hpp>
#include <archon/core/config.hpp>
#include <archon/font/list.hpp>


namespace Archon
{
  namespace Font
  {
    /**
     * Print out a descriptive table with an entry for each font
     * face in the specified list.
     *
     * \paran l The list of font faces to print.
     *
     * \param out The target stream.
     *
     * \param enable_ansi_term_attr Set to false if the target
     * stream is not an ANSI terminal, or if you do not want the
     * output to be colored.
     */
    void print_font_list(FontList::ConstArg l, std::ostream &out, bool enable_ansi_term_attr = true);



    struct ListConfig
    {
      /**
       * The nominal glyph size (width, height) in number of pixels
       * (may be fractional). If either component is less than or
       * equal to zero, the default size will be used.
       *
       * The default is (0,0).
       */
      Core::Series<2, double> size;

      /**
       * Set to true if you only want the list of available fonts to
       * be printed. No font face will be loaded.
       *
       * It is false by default.
       */
      bool list;

      /**
       * A colon separated list of directories holding font
       * files. Each mentioned directory will be searched recursively.
       *
       * The default is "/usr/share/fonts".
       */
      std::string path;

      /**
       * The file system path of the font file to load. When not the
       * empty string, the first font face in the specified file will
       * be loaded, and <tt>family</tt>, <tt>bold</tt>, and
       * <tt>italic</tt> will be ignored.
       *
       * It is the empty string by default.
       */
      std::string file;

      void populate(Core::ConfigBuilder &cfg);

      ListConfig();
    };


    struct FontConfig: ListConfig
    {
      /**
       * The family name of the font face to load. If left empty, the
       * name of the default font face will be used.
       *
       * It is empty by default.
       */
      std::string family;

      /**
       * If set to true, the loaded font face will be bold.
       *
       * It is false by default.
       */
      bool bold;

      /**
       * If set to true, the loaded font face will be italic/oblique.
       *
       * It is false by default.
       */
      bool italic;

      void populate(Core::ConfigBuilder &cfg);

      FontConfig();
    };


    /**
     * Load a font face according to the specified configuration.
     *
     * If the desired font could not be found, an appropriate message
     * is displayed on STDOUT/STDERR and NULL is returned.
     *
     * \param resource_dir The directory holding the font loader
     * resources.
     */
    Core::UniquePtr<FontFace> load_font(std::string resource_dir, FontConfig const &cfg);


    /**
     * Make a font list whose default font is selected according to
     * the specified configuration.
     *
     * If the desired default font could not be found, an appropriate
     * message is displayed on STDOUT/STDERR and NULL is returned.
     *
     * \param resource_dir The directory holding the font loader
     * resources.
     */
    FontList::Ptr make_font_list(std::string resource_dir, FontConfig const &cfg);
  }
}

#endif // ARCHON_FONT_UTIL_HPP
