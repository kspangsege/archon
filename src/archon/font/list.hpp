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

#ifndef ARCHON_FONT_LIST_HPP
#define ARCHON_FONT_LIST_HPP

#include <string>

#include <archon/core/unique_ptr.hpp>
#include <archon/font/loader.hpp>


namespace archon
{
  namespace Font
  {
    /**
     * This class manages a list of font faces, and allows you to
     * retreive a font face by family name.
     *
     * New list instances are aquired by calling
     * new_font_list(FontLoader::Ptr,std::string,double,double) or one
     * of the other functions of the same name.
     *
     * A \c FontList instance is always associated with a \c FontLoader
     * instance. This is the loader that was used to create the \c
     * FontList instance.
     *
     * Faces are identified by their index within the list, and a
     * negative index always indicates the default face of this
     * list. The fact that a list can never be empty, guarantees that
     * there is always at least a default font available.
     *
     * The methods of this class are not thread-safe. It is safe,
     * however, for multiple threads to use this class simultaneously,
     * as long as they access different instances, and no two
     * instances are associated with the same \c FontLoader instance. That
     * is, you also need one loader instance per thread.
     */
    struct FontList
    {
      typedef Core::SharedPtr<FontList> Ptr;
      typedef Ptr const &Arg;
      typedef Core::SharedPtr<FontList const> ConstPtr;
      typedef ConstPtr const &ConstArg;


      /**
       * Used by find_default_size() and find_face() to report the
       * result of searching for a matching fixed rendering size.
       */
      struct SizeInfo
      {
        /**
         * Set to -1 if the queried font face was scalable and the
         * wanted rendering size did not match any of the available
         * fixed sizes.
         */
        int fixed_size_index;

        /**
         * Set to true if, and only if fixed_size_index was -1,
         * or the located fixed size matched the wanted size exactly.
         */
        bool exact;
      };


      /**
       * Search for a matching rendering size offered by the default
       * font face. This size selection scheme mimics that used by
       * FontFace::set_approx_size().
       *
       * \sa SizeInfo
       *
       * It shall be guaranteed that this method does not trigger a
       * scan through the font path for further font files. On the
       * other hand, this method will not find a matching fixed size,
       * if that fixed size is not offered by the default face. This
       * is true even when such a fixed size is available after the
       * font path has been scanned.
       */
      virtual void find_default_size(double width, double height, SizeInfo &info) const = 0;


      enum FindType
      {
        find_Exact,      ///< Require an exact match.
        find_BestSize,   ///< Allow an unexact size. Family name and style must match.
        find_BestStyle,  ///< Allow an unexact size and style. Family name must match.
        find_BestFace    ///< Allow any font.
      };


      /**
       * Find a font face with the specified family name and
       * attributes.
       *
       * \param find_type Specifies to which extent a face must match
       * the specified criteria. In cases where there is no perfect
       * match, but multiple faces match sufficiently, this method
       * will choose the one that fits your criteria best.
       *
       * \param family The family name of the desired font,
       * e.g. 'Times New Roman'.
       *
       * \param bold Set to true if, and only if you want a font face
       * that is bold.
       *
       * \param italic Set to true if, and only if you want a font face
       * that is italic/oblique.
       *
       * \param width, height In some cases a list will contain
       * multiple font faces with the same name and same style. The
       * only thing that sets them apart, is that they provide
       * different fixed rendering sizes. In such cases this method
       * will select the one that provides the fixed size that most
       * closely matches the specified size.
       *
       * \param size_info If not null, and the return value is not -1,
       * information about the best matching fixed rendering size (if
       * any) is stored in the referenced object. If the return value
       * is -1, then the referenced object is not modified. This size
       * selection scheme mimics that used by
       * FontFace::set_approx_size().
       *
       * \return The index of the desired font face, or -1 if no such
       * font face could be found.
       *
       * \note If the located font face is scalable, then this method
       * does not guarantee that a fixed size is found if that fixed
       * size is available only after the font path has been scanned.
       */
      virtual int find_face(FindType find_type, std::string family, bool bold, bool italic,
                            double width = 12, double height = 12,
                            SizeInfo *size_info = 0) const = 0;


      /**
       * Load the specified font face. The returned \c FontFace object
       * is primarily usefull for rendering individual glyphs of the
       * font face.
       *
       * \param face_index The index within this list of the font face
       * to be loaded. Passing a negative index will cause the default
       * face to be loaded.
       *
       * \return The loaded font face.
       *
       * \throw BadFontFileException If the file containing the
       * specified font could not be successfully processed.
       */
      virtual Core::UniquePtr<FontFace> load_face(int face_index = -1) const = 0;


      /**
       * Get the number of font faces in this list.
       *
       * Because the first font face in any list, is the default font
       * face of the associated loader, this number will always be at
       * least 1.
       *
       * \return The number of font faces.
       */
      virtual int get_num_faces() const = 0;


      /**
       * Get various descriptive details about the specified font
       * face.
       *
       * \param face_index The index within this list of the font face
       * to query. Passing a negative index will load information
       * about the default face.
       *
       * \return A reference to an object containing the requested
       * information.
       */
      virtual FontLoader::FaceInfo const &get_face_info(int face_index = -1) const = 0;


      /**
       * Get the number of distinct font families that are available
       * in this list.
       */
      virtual int get_num_families() const = 0;


      /**
       * Get the name of the specified font family. Use
       * get_num_families() to find out how many different families
       * there are.
       */
      virtual std::string get_family_name(int family_index) const = 0;


      /**
       * Add the specified font face from the specified font file to
       * this list. Optionally, add all the font face in the file.
       *
       * If you want to add all the files contained in some directory,
       * consider using scan_dir().
       *
       * \param font_file_path The file system path of the font file
       * that contains the font face to be added.
       *
       * \param face_index The index of the desired font face within
       * the specified font file. If you specify a negative value, all
       * the faces contained in the file will be added. Use \c
       * FontLoader::check_file to see how many faces the file
       * contains.
       *
       * \throw BadFontFileException If the specified font file could
       * not be successfully processed.
       *
       * \sa FontLoader::check_file
       */
      virtual void add_face(std::string font_file_path, int face_index = -1) = 0;


      /**
       * Add all font faces of all font files that can be found in the
       * specified directory.
       *
       * \param dir_path The file system path of the directory to
       * scan.
       *
       * \param recursive If true, this method will also recursively
       * scan each subdirectory of the specified directory.
       */
      virtual void scan_dir(std::string dir_path, bool recursive = true) = 0;


      /**
       * Call scan_dir() for each colon separated directory mentioned
       * in <tt>dir_paths</tt>. It is not an error if mentioned
       * directories do not exists.
       */
      void scan_dirs(std::string dir_paths, bool recursive = true);


      /**
       * The initial rendering size that this list applies to any face
       * after loading it.
       */
      virtual void get_init_size(double &width, double &height) const = 0;


      /**
       * Get the index of the default face.
       */
      virtual int get_default_face() const = 0;


      virtual ~FontList() {}
    };



    /**
     * Create a new font face list that initally contains only the
     * specified face from the specified font file. This face becomes
     * the default face of the list.
     *
     * \param width, height Any scalable font face loaded from the
     * returned list will have its initial rendering size set as
     * closely as possible to this size. The actual selection scheme
     * is identical to that of FontFace::set_approx_size().
     *
     * \return The new font face list.
     */
    FontList::Ptr new_font_list(FontLoader::Ptr loader, std::string font_file, int face_index,
                                double width, double height);


    /**
     * Create a new font face list that initally contains all the
     * faces from all the font files that can be found by traversing
     * the specified font search path.
     *
     * The search path is not scanned immediately. If the application
     * only ever queries the list for the default face, then the
     * scanning probably never happens. As soon as a reference is made
     * to a face that is not the default face, scanning will happen.
     *
     * \param font_search_path A colon separated list of directories
     * holding font files. Each mentioned directory will be searched
     * recursively. If no directories are mentioned (the default) then
     * only the default fallback font will be available.
     *
     * \param width, height Any scalable font face loaded from the
     * returned list will have its initial rendering size set as
     * closely as possible to this size. The actual selection scheme
     * is identical to that of FontFace::set_approx_size().
     */
    FontList::Ptr new_font_list(FontLoader::Ptr loader, std::string font_search_path = "",
                                double width = 12, double height = 12);


    /**
     * Same as
     * new_font_list(FontLoader::Ptr,std::string,double,double), but
     * this method allows you to specify what the default font of the
     * list should be.
     *
     * Sometimes there are multiple seperate font faces with the same
     * family name, boldness and italicity, but they provide different
     * fixed rendering sizes. In such cases, this method shall attempt
     * to locate the face with the fixed size that most closely
     * matches the specified size.
     */
    FontList::Ptr new_font_list(FontLoader::Ptr loader, std::string font_search_path,
                                FontList::FindType find_type, std::string family,
                                bool bold, bool italic, double width, double height);
  }
}

#endif // ARCHON_FONT_LIST_HPP
