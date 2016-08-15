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

#ifndef ARCHON_UTIL_RECT_PACKER_HPP
#define ARCHON_UTIL_RECT_PACKER_HPP

#include <archon/core/unique_ptr.hpp>


namespace archon
{
  namespace Util
  {
    /**
     * Pack a number of small rectangles inside a larger rectangle.
     *
     * The larger rectangle must have a fixed with, but its height can
     * either be fixed or unbounded.
     *
     * \sa http://www.blackpawn.com/texts/lightmaps/default.html
     * \sa http://www.gamedev.net/community/forums/topic.asp?topic_id=392413
     * \sa http://en.wikipedia.org/wiki/Bin_packing_problem
     */
    struct RectanglePacker
    {
      /**
       * \param height Pass a negative value to get an unbounded
       * height.
       */
      RectanglePacker(int width, int height = -1, int spacing = 0);

      /**
       * \return False if there is not enough space left for a
       * rectangle of the specified size.
       */
      bool insert(int w, int h, int &x, int &y);

      /**
       * If the height is unbounded, this method returns the actually
       * used height, otherwise it simply returns the height.
       */
      int get_height();

      float get_coverage();

    private:
      struct Node
      {
        int x, y;
        int width, height;
        Core::UniquePtr<Node> right, under;

        // Return 0 on 'no fit'
        Node const *insert(int w, int h);

        // Calculate the amount of free space in this sub-tree.
        long free(bool ignore_lowest);

        Node(int x, int y, int w, int h): x(x), y(y), width(w), height(h) {}
      };

      int const spacing;
      Node root;
    };







    // Implementation:

    inline bool RectanglePacker::insert(int w, int h, int &x, int &y)
    {
      Node const *const n = root.insert(w + spacing, h + spacing);
      if(!n) return false;
      x = n->x + spacing;
      y = n->y + spacing;
      return true;
    }


    inline float RectanglePacker::get_coverage()
    {
      long const area = get_height() * long(root.width);
      return double(area - root.free(root.height < 0)) / area;
    }


    inline long RectanglePacker::Node::free(bool ignore_lowest)
    {
      return right ? right->free(false) + under->free(ignore_lowest) :
        ignore_lowest ? 0 : height * long(width);
    }
  }
}

#endif // ARCHON_UTIL_RECT_PACKER_HPP
