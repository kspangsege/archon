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

#ifndef ARCHON_RENDER_LOAD_TEXTURE_HPP
#define ARCHON_RENDER_LOAD_TEXTURE_HPP

#include <archon/image/image.hpp>


namespace Archon
{
  namespace Render
  {
    /**
     * Load the specified image into the currently bound OpenGL
     * texture object, and configure it for non-mipmap operation.
     *
     * \param no_interp Turn off linear interpolation between texture
     * pixels. In this case the nearest pixel is used.
     */
    void load_texture(std::string image_path, bool no_interp = false);


    /**
     * Load the specified image into the currently bound OpenGL
     * texture object, and configure it for non-mipmap operation.
     *
     * \param with_border Set to true if the edge texels of the
     * specified image should make up a border of the texture, rather
     * than be a part of the texture. A texture border can optionally
     * be used to supply texels when querying for the color near the
     * edge of the texture. In particular, when a linear filter is
     * used to average over the 'nearest' 2x2 block of texels, and
     * coordinate clamping is set to <tt>GL_CLAMP_TO_BORDER</tt>, then
     * the the border is used to supply texels for the part of the
     * block that falls outside the main texture. When the border is
     * not contained in the image (\c with_border is false) then the
     * border has a fixed but per-texture configurable color.
     *
     * \param no_interp Turn off linear interpolation between texture
     * pixels. In this case the nearest pixel is used.
     */
    void load_texture(Imaging::Image::ConstRefArg, bool with_border = false,
                      bool no_interp = false);


    /**
     * Generate all the mipmap levels of the specified image, and load
     * them into the currently bound OpenGL texture object, and
     * configure it for mipmap operation.
     */
    void load_mipmap(Imaging::Image::ConstRefArg);


    /**
     * Load one or more mipmap levels into the currently bound
     * OpenGL texture object, and configure it for mipmap operation.
     *
     * A level of zero indicates the unreduced image, whose width
     * and size must both be powers of two. A level of one indicates
     * the first reduction of the original image, which is exactly
     * half as big in each direction. A special case is when the
     * width of the original is 1, while the height is greater than
     * 1, in which case the width of the first reduction level
     * remains to be 1, while the height is halved. Interchange the
     * meanings of width and height, and the story is the same. The
     * maximum level, corresponding to 1x1 is \c log2(max(w,h))
     * where \c w and \c h are the width and height of the unreduced
     * image at level 0.
     *
     * \param level The reduction level of the specified image.
     *
     * \param first The first reduction level to be loaded. If a
     * negative value is specified the effective value will be the
     * same as the value of <tt>level</tt>.
     *
     * \param last The last reduction levels to be loaded. If a
     * negative value is specified the effective value will be the
     * same as the effective value of <tt>first</tt>.
     *
     * \param min_avail The lowest available reduction level
     * (largest image size) for the bound texture after the new
     * levels have been added. If a negative value is specified, it
     * will be taken to be equal to the effective value of
     * <tt>first</tt>.
     *
     * \param min_avail The highest available reduction level
     * (smallest image size) for the bound texture after the new
     * levels have been added. If a negative value is specified, it
     * will be taken to be equal to the effective value of
     * <tt>last</tt>.
     */
    void load_mipmap_levels(Imaging::Image::ConstRefArg, int level,
                            int first = -1, int last = -1,
                            int min_avail = -1, int max_avail = -1);






    // Implementation:

    inline void load_texture(std::string image_path, bool no_interp)
    {
      load_texture(Imaging::Image::load(image_path), no_interp);
    }
  }
}

#endif // ARCHON_RENDER_LOAD_TEXTURE_HPP
