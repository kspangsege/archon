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

#include <archon/util/rect_packer.hpp>


namespace archon
{
  namespace util
  {
    RectanglePacker::RectanglePacker(int width, int height, int spacing):
      spacing(spacing), root(0, 0, width - spacing, height < 0 ? -1 : height - spacing) {}



    RectanglePacker::Node const *RectanglePacker::Node::insert(int w, int h)
    {
      if(right)
      {
        Node const *const n = right->insert(w,h);
        return n ? n : under->insert(w,h);
      }

      int w1, h1; // Right
      int w2, h2; // Under
      w1 = width - w;
      if(w1 < 0) return 0;
      if(height < 0)
      {
        // When the height is unbounded, the cut must always be
        // horizontal
        h1 = h;
        w2 = width;
        h2 = -1;
      }
      else
      {
        h2 = height - h;
        if(h2 < 0) return 0;
/*
        if(h2 < w1)
        {
          h1 = height;
          w2 = w;
        }
        else
*/
        {
          h1 = h;
          w2 = width;
        }
      }
      // FIXME: Would benefit much from a chunk allocator for Node
      right.reset(new Node(x+w, y, w1, h1));
      under.reset(new Node(x, y+h, w2, h2));
      return this;
    }



    int RectanglePacker::get_height()
    {
      Node *n = &root;
      if(0 <= n->height) return n->height + spacing; // Bounded height
      for(;;)
      {
        Node *const u = n->under.get();
        if(!u) return n->y + spacing;
        n = u;
      }
    }
  }
}
