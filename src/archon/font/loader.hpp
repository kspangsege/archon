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

#ifndef ARCHON_FONT_LOADER_HPP
#define ARCHON_FONT_LOADER_HPP

#include <stdexcept>
#include <utility>
#include <vector>
#include <string>

#include <archon/core/shared_ptr.hpp>
#include <archon/font/face.hpp>


namespace archon
{
  namespace Font
  {
    /**
     * Thrown if a font file could not be successfully processed.
     */
    struct BadFontFileException;


    /**
     * This class represents a means of accessing font faces within
     * font files.
     *
     * New loader instances are aquired by calling
     * <tt>Font::new_font_loader</tt>.
     *
     * Multiple threads can safly use the font loader as long as each
     * thread uses a separate loader instance. Indeed, if two threads
     * need to use the \c FontFace class simultaneously, they must not
     * only use separate instances, but and those instances must be
     * retreived from separate loader instances.
     *
     * \sa FontFace
     * \sa FontList
     */
    struct FontLoader
    {
      typedef core::SharedPtr<FontLoader> Ptr;
      typedef Ptr const &Arg;


      /**
       * Load the default font face.
       *
       * \param width, height The returned font face will have its
       * default rendering size set as close as possible to this
       * size. The actual selection scheme is identical to that of
       * FontFace::set_approx_size().
       *
       * \return The loaded font face.
       */
      virtual core::UniquePtr<FontFace> load_default_face(double width = 12,
                                                          double height = 12) const = 0;


      /**
       * Load the specified font face from the specified font file.
       *
       * \param font_file The file system path of the font file
       * that contains the font face to be loaded.
       *
       * \param face_index The index of the desired font face within
       * the specified font file.
       *
       * \param width, height The returned font face will have its
       * default rendering size set as close as possible to this
       * size. The actual selection scheme is identical to that of
       * FontFace::set_approx_size().
       *
       * \return The loaded font face.
       *
       * \throw BadFontFileException If the specified font file could
       * not be successfully processed.
       */
      virtual core::UniquePtr<FontFace> load_face(std::string font_file, int face_index = 0,
                                                  double width = 12, double height = 12) const = 0;


      /**
       * Identifying information about a particular font face.
       */
      struct FaceInfo
      {
        std::string family;
        bool bold, italic, monospace, scalable;
        typedef std::pair<double, double> FixedSize; ///< (width, height)
        typedef std::vector<FixedSize> FixedSizes;
        FixedSizes fixed_sizes;
      };


      /**
       * Load identifying information about the specified font face
       * from the specified font file.
       *
       * It is the intention that it is more efficient to call this
       * method than to call load_face() and then extract the
       * information manually.
       *
       * \param font_file The file system path of the font file
       * that contains the font face to be queried.
       *
       * \param face_index The index of the desired font face within
       * the specified font file.
       *
       * \param info The loaded font face information will be stored
       * in this object.
       *
       * \throw BadFontFileException If the specified font file could
       * not be successfully processed.
       */
      virtual void load_face_info(std::string font_file, int face_index, FaceInfo &info) const = 0;


      /**
       * Check the specified file to see if it is a font file, and if
       * so, how many font faces it contains.
       *
       * \param font_file_path The file system path of the font file
       * to check.
       *
       * \return Zero if the specified file is not a recognized font
       * file, otherwise the number of individual font faces contained
       * in the file.
       */
      virtual int check_file(std::string font_file_path) const = 0;


      virtual std::string get_default_font_file() const = 0;


      virtual int get_default_face_index() const = 0;


      virtual ~FontLoader() {}
    };



    /**
     * Create a new loader instance.
     *
     * \param resource_dir The directory holding the font loader
     * resources.
     *
     * \note This function is thread-safe.
     */
    FontLoader::Ptr new_font_loader(std::string resource_dir);








    // Implementation:

    struct BadFontFileException: std::runtime_error
    {
      BadFontFileException(std::string msg): std::runtime_error(msg) {}
    };
  }
}

#endif // ARCHON_FONT_LOADER_HPP
