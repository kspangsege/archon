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

#ifndef ARCHON_UTIL_RECT_PACKER_HPP
#define ARCHON_UTIL_RECT_PACKER_HPP

#include <cstddef>
#include <vector>


namespace archon {
namespace util {

/// Pack a number of small rectangles inside a larger rectangle.
///
/// The larger rectangle must have a fixed width, but its height can either be
/// fixed or unbounded.
///
/// \sa http://www.blackpawn.com/texts/lightmaps/default.html,
/// http://www.gamedev.net/community/forums/topic.asp?topic_id=392413,
/// http://en.wikipedia.org/wiki/Bin_packing_problem.
///
/// FIXME: Talk about the effect of presorting vs not presorting.
class RectanglePacker {
public:
    /// \param height Pass a negative value to get an unbounded height.
    RectanglePacker(int width, int height = -1, int spacing = 0);

    /// \return False if there is not enough space left for a rectangle of the
    /// specified size.
    bool insert(int w, int h, int& x, int& y);

    /// If the height is unbounded, this method returns the actually used
    /// height, otherwise it simply returns the height.
    int get_height() const noexcept;

    float get_coverage() const noexcept;

    void reset(int width, int height = -1, int spacing = 0);

private:
    using NodeIndexType = std::size_t;
    static constexpr NodeIndexType nil() { return NodeIndexType(-1); }

    struct Node {
        int x, y;
        int width, height;

        // Index of root of 'right' branch, or `nil()` if this is a leaf
        // node. If the this is not a leaf node, the root of the 'under' branch
        // is at index `branches_ndx+1`.
        NodeIndexType branches_ndx = nil();

        Node(int x, int y, int w, int h):
            x{x},
            y{y},
            width{w},
            height{h}
        {
        }
    };

    const int m_spacing;
    std::vector<Node> m_nodes;

    // Returns the index of the node within the specified branch that represents
    // the inserted box, or `nil()` if the box did not fit anywhere in that
    // branch.
    NodeIndexType do_insert(NodeIndexType, int w, int h);

    // Calculate the amount of free space in the specified branch.
    long get_free_space(const Node&, bool ignore_lowest) const noexcept;
};



// Implementation

inline bool RectanglePacker::insert(int w, int h, int& x, int& y)
{
    NodeIndexType root_ndx = 0;
    NodeIndexType node_ndx = do_insert(root_ndx, w + m_spacing, h + m_spacing); // Throws
    if (node_ndx == nil())
        return false;
    const Node& node = m_nodes[node_ndx];
    x = node.x + m_spacing;
    y = node.y + m_spacing;
    return true;
}

inline float RectanglePacker::get_coverage() const noexcept
{
    const Node& root = m_nodes.front();
    long area = get_height() * long(root.width);
    return double(area - get_free_space(root, root.height < 0)) / area;
}

inline long RectanglePacker::get_free_space(const Node& node, bool ignore_lowest) const noexcept
{
    bool is_leaf = (node.branches_ndx == nil());
    if (is_leaf)
        return (ignore_lowest ? 0 : node.height * long(node.width));
    const Node& right = m_nodes[node.branches_ndx + 0];
    const Node& under = m_nodes[node.branches_ndx + 1];
    return get_free_space(right, false) + get_free_space(under, ignore_lowest);
}

} // namespace util
} // namespace archon

#endif // ARCHON_UTIL_RECT_PACKER_HPP
