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

/// \file
///
/// \author Kristian Spangsege

#include <limits>

#include <archon/util/rect_packer.hpp>


namespace archon {
namespace util {

RectanglePacker::RectanglePacker(int width, int height, int spacing):
    m_spacing{spacing}
{
    m_nodes.emplace_back(0, 0, width - spacing, (height < 0 ? -1 : height - spacing)); // Throws
}


RectanglePacker::NodeIndexType RectanglePacker::do_insert(NodeIndexType node_ndx, int w, int h)
{
    Node& node = m_nodes[node_ndx];
    bool is_leaf = (node.branches_ndx == nil());
    if (!is_leaf) {
        NodeIndexType right_ndx = node.branches_ndx + 0;
        NodeIndexType under_ndx = node.branches_ndx + 1;
        NodeIndexType node_ndx_2 = do_insert(right_ndx, w, h); // Throws
        if (node_ndx_2 != nil())
            return node_ndx_2;
        return do_insert(under_ndx, w, h); // Throws
    }

    int w_1, h_1; // Right
    int w_2, h_2; // Under
    w_1 = node.width - w;
    if (w_1 < 0)
        return nil();
    if (node.height < 0) {
        // When the height is unbounded, the cut along a horizontal line first.
        h_1 = h;
        w_2 = node.width;
        h_2 = -1;
    }
    else {
        h_2 = node.height - h;
        if (h_2 < 0)
            return nil();
        /* if (h_2 < w_1) {
            // Cut along a vertical line first, then cut the top part along a
            // horizontal line.
            h_1 = node.height;
            w_2 = w;
        }
        else */ {
            // Cut along a horizontal line first, then cut the top part along a
            // vertical line.
            h_1 = h;
            w_2 = node.width;
        }
    }
    using Nodes = decltype(m_nodes);
    static_assert(std::numeric_limits<Nodes::size_type>::max() <=
                  std::numeric_limits<NodeIndexType>::max(), "");
    NodeIndexType branches_ndx = m_nodes.size();
    try {
        int x = node.x, y = node.y;
        // Add 'right' branch
        m_nodes.emplace_back(x + w, y, w_1, h_1); // Throws
        // Add 'under' branch
        m_nodes.emplace_back(x, y + h, w_2, h_2); // Throws
    }
    catch (...) {
        // Reset the size
        while (m_nodes.size() > branches_ndx)
            m_nodes.pop_back();
        throw;
    }
    m_nodes[node_ndx].branches_ndx = branches_ndx;
    return node_ndx;
}


int RectanglePacker::get_height() const noexcept
{
    const Node* node = &m_nodes.front();
    if (node->height >= 0)
        return node->height + m_spacing; // Bounded height
    for (;;) {
        bool is_leaf = (node->branches_ndx == nil());
        if (is_leaf)
            return node->y + m_spacing;
        NodeIndexType under_ndx = node->branches_ndx + 1;
        node = &m_nodes[under_ndx];
    }
}

} // namespace util
} // namespace archon
