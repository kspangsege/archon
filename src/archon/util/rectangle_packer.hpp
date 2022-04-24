// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2022 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef ARCHON_X_UTIL_X_RECTANGLE_PACKER_HPP
#define ARCHON_X_UTIL_X_RECTANGLE_PACKER_HPP

/// \file


#include <cstddef>
#include <cstdint>
#include <vector>
#include <stack>


#include <archon/core/mul_prec_int.hpp>


namespace archon::util {


/// \brief Find close-to-optimal packing of rectangles inside larger rectangle.
///
/// This class offers a bin packing mechanism for the case where the bin and the items to be
/// packed are rectangles. It is an implementation of the first-fit decreasing algorithm.
///
/// \tparam T The type of the components (X and Y) of the rectangle sizes and positions.
///
template<class T = int> class RectanglePacker {
public:
    using comp_type = T;

    static constexpr comp_type max_comp = core::int_max<comp_type>();

    RectanglePacker(comp_type spacing = 0, comp_type margin = 0) noexcept;

    void reset() noexcept;

    void add_rect(comp_type width, comp_type height);

    auto suggest_bin_width() const noexcept -> comp_type;

    /// \brief Pack the added rectangles.
    ///
    /// This function packs the added rectangles into one or more bins of the specified
    /// size.
    ///
    /// The packing operation fails if more than \p max_bins would be needed in order to fit
    /// all the added rectangles. In this case, this function returns `false`. Otherwise it
    /// returns `true`.
    ///
    bool pack(comp_type bin_width, comp_type bin_height = max_comp, std::size_t max_bins = 1);

    auto get_num_bins() const noexcept -> std::size_t;

    auto get_utilized_width(std::size_t bin_index = 0) const noexcept  -> comp_type;
    auto get_utilized_height(std::size_t bin_index = 0) const noexcept -> comp_type;

    void get_rect_pos(std::size_t rect_index, comp_type& x, comp_type& y) const noexcept;
    void get_rect_pos(std::size_t rect_index, std::size_t& bin_index, comp_type& x, comp_type& y) const noexcept;

private:
    struct Rect {
        comp_type w, h;
        comp_type x, y;
        std::size_t bin_index;
    };

    struct Bin {
        std::size_t root_node;
        comp_type max_x, max_y;
    };

    struct Node {
        bool occupied;
        comp_type x, y;
        comp_type w, h;
        std::size_t right_node, down_node;
    };

    // FIXME: Avoid using core::MulPrecInt when a sufficiently wide built-in integer type exists.
    // FIXME: Use `comp_type` if `comp_type` is a floating point type.
    using area_type = core::MulPrecInt<std::make_unsigned_t<comp_type>, 2, false>;

    comp_type m_spacing;
    comp_type m_margin;

    std::vector<Rect> m_rects;
    std::vector<std::size_t> m_order;
    std::vector<Bin> m_bins;
    std::vector<Node> m_nodes;
    std::vector<std::size_t> m_stack;

    bool do_pack(comp_type bin_width, comp_type bin_height, std::size_t max_bins);
};








// Implementation


template<class T>
inline RectanglePacker<T>::RectanglePacker(comp_type spacing, comp_type margin) noexcept
    : m_spacing(spacing)
    , m_margin(margin)
{
}


template<class T>
inline void RectanglePacker<T>::reset() noexcept
{
    m_rects.clear();
    m_bins.clear();
    m_nodes.clear();
}


template<class T>
inline void RectanglePacker<T>::add_rect(comp_type width, comp_type height)
{
    ARCHON_ASSERT(!core::is_negative(width));
    ARCHON_ASSERT(!core::is_negative(height));
    comp_type x = 0, y = 0;
    std::size_t bin_index = 0;
    m_rects.push_back({ width, height, x, y, bin_index }); // Throws
}


template<class T>
auto RectanglePacker<T>::suggest_bin_width() const noexcept -> comp_type
{
    comp_type min_width = 0;
    for (const Rect& rect: m_rects) {
        if (rect.w > min_width)
            min_width = rect.w;
    }

    comp_type width;
    using part_type = std::uintmax_t;
    {
        part_type area = 0;
        for (const Rect& rect: m_rects) {
            part_type w = part_type(rect.w);
            part_type h = part_type(rect.h);
            if (ARCHON_LIKELY(core::try_int_add(w, m_spacing) &&
                              core::try_int_add(w, m_spacing))) {
                part_type a = w;
                if (ARCHON_LIKELY(core::try_int_mul(a, h) && core::try_int_add(area, a)))
                    continue;
            }
            goto fallback;
        }
        width = std::max(comp_type(core::int_sqrt(area) - m_spacing), min_width);
    }

  add_margins:
    {
        comp_type max = core::int_max<comp_type>();
        for (int i = 0; i < 2; ++i) {
            if (ARCHON_LIKELY(m_margin <= max - width)) {
                width += m_margin;
                continue;
            }
            return max;
        }
    }
    return width;

  fallback:
    {
        // Need to be able to hold area of a rectangle whose sides lengths are
        // max_image_wisth + spacing and max_image_height + spacing.
        constexpr int bits = 2 * (core::num_value_bits<comp_type>() + 1);
        constexpr int num_parts = bits / core::int_width<part_type>();
        ARCHON_ASSERT(num_parts > 1); // Otherwise the attempt above would have succeeded
        constexpr bool is_signed = false;
        using mul_prec_type = core::MulPrecInt<part_type, num_parts, is_signed>;
        mul_prec_type spacing = mul_prec_type(m_spacing);
        mul_prec_type area = mul_prec_type(0);
        for (const Rect& rect: m_rects) {
            mul_prec_type a = ((mul_prec_type(rect.w) + spacing) *
                               (mul_prec_type(rect.h) + spacing));
            if (ARCHON_LIKELY(a <= mul_prec_type::max() - area)) {
                area += a;
                continue;
            }
            return max_comp;
        }
        width = std::max(comp_type(core::int_sqrt(area) - mul_prec_type(m_spacing)), min_width);
    }
    goto add_margins;
}


template<class T>
bool RectanglePacker<T>::pack(comp_type bin_width, comp_type bin_height, std::size_t max_bins)
{
    try {
        return do_pack(bin_width, bin_height, max_bins); // Throws
    }
    catch (...) {
        m_bins.clear();
        m_nodes.clear();
        throw;
    }
}


template<class T>
inline std::size_t RectanglePacker<T>::get_num_bins() const noexcept
{
    return m_bins.size();
}


template<class T>
inline auto RectanglePacker<T>::get_utilized_width(std::size_t bin_index) const noexcept -> comp_type
{
    ARCHON_ASSERT(bin_index < m_bins.size());
    const Bin& bin = m_bins[bin_index];
    return bin.max_x + m_margin;
}


template<class T>
inline auto RectanglePacker<T>::get_utilized_height(std::size_t bin_index) const noexcept -> comp_type
{
    ARCHON_ASSERT(bin_index < m_bins.size());
    const Bin& bin = m_bins[bin_index];
    return bin.max_y + m_margin;
}


template<class T>
inline void RectanglePacker<T>::get_rect_pos(std::size_t rect_index, comp_type& x, comp_type& y) const noexcept
{
    std::size_t bin_index = 0; // Dummy
    return get_rect_pos(rect_index, bin_index, x, y);
}


template<class T>
void RectanglePacker<T>::get_rect_pos(std::size_t rect_index, std::size_t& bin_index,
                                      comp_type& x, comp_type& y) const noexcept
{
    ARCHON_ASSERT(rect_index < m_rects.size());
    const Rect& rect = m_rects[rect_index];
    bin_index = rect.bin_index;
    x = rect.x;
    y = rect.y;
}


template<class T>
bool RectanglePacker<T>::do_pack(comp_type bin_width, comp_type bin_height, std::size_t max_bins)
{
    if (ARCHON_UNLIKELY(m_margin > bin_width  || m_margin > bin_width  - m_margin ||
                        m_margin > bin_height || m_margin > bin_height - m_margin))
        return false;
    comp_type root_x = m_margin;
    comp_type root_y = m_margin;
    comp_type root_w = bin_width  - 2 * m_margin;
    comp_type root_h = bin_height - 2 * m_margin;

    // Order by descending height
    std::size_t n = m_rects.size();
    m_order.resize(n); // Throws
    for (std::size_t i = 0; i < n; ++i)
        m_order[i] = i;
    std::stable_sort(m_order.begin(), m_order.end(), [&](std::size_t i, std::size_t j) {
        return (m_rects[i].h > m_rects[j].h ||
                (m_rects[i].h == m_rects[j].h && m_rects[i].w > m_rects[j].w));
    });

    // Place rectangles
    ARCHON_ASSERT(m_nodes.empty());
    ARCHON_ASSERT(m_bins.empty());
    auto add_node = [&](comp_type x, comp_type y, comp_type w, comp_type h) {
        bool occupied = false;
        std::size_t right_node = std::size_t(-1);
        std::size_t down_node = std::size_t(-1);
        m_nodes.push_back({ occupied, x, y, w, h, right_node, down_node }); // Throws
    };
    auto insert_rect = [&](Rect& rect) {
        comp_type w = rect.w;
        comp_type h = rect.h;
        std::size_t bin_index = 0;
        for (;;) {
            if (ARCHON_UNLIKELY(bin_index == m_bins.size())) {
                if (ARCHON_UNLIKELY(bin_index == max_bins || w > root_w || h > root_h))
                    return false;
                std::size_t root_node = m_nodes.size();
                add_node(root_x, root_y, root_w, root_h); // Throws
                comp_type max_x = root_x;
                comp_type max_y = root_y;
                m_bins.push_back({ root_node, max_x, max_y }); // Throws
            }
            Bin& bin = m_bins[bin_index];
            m_stack.clear();
            std::stack stack(m_stack);
            std::size_t node_index = bin.root_node;
            for (;;) {
                Node* node = &m_nodes[node_index];
                if (!node->occupied) {
                    bool fits = (w <= node->w && h <= node->h);
                    if (ARCHON_LIKELY(!fits))
                        goto back;
                    node->occupied = true;
                    if (ARCHON_UNLIKELY(node->x + w > bin.max_x))
                        bin.max_x = node->x + w;
                    if (ARCHON_UNLIKELY(node->y + h > bin.max_y))
                        bin.max_y = node->y + h;
                    comp_type w_1, h_1; // Right
                    comp_type w_2, h_2; // Down
                    w_1 = node->w - w;
                    h_2 = node->h - h;
                    if (h_2 >= w_1) {
                        // Split into upper and lower parts, then split upper part into left
                        // and right parts.
                        h_1 = h;
                        w_2 = node->w;
                    }
                    else {
                        // Split into left and right parts, then split left part into upper
                        // and lower parts.
                        h_1 = node->h;
                        w_2 = w;
                    }
                    if (w_1 >= m_spacing) {
                        w_1 -= m_spacing;
                        node->right_node = m_nodes.size();
                        comp_type x = node->x + w + m_spacing;
                        comp_type y = node->y;
                        add_node(x, y, w_1, h_1); // Throws
                        node = &m_nodes[node_index];
                    }
                    if (h_2 >= m_spacing) {
                        h_2 -= m_spacing;
                        node->down_node = m_nodes.size();
                        comp_type x = node->x;
                        comp_type y = node->y + h + m_spacing;
                        add_node(x, y, w_2, h_2); // Throws
                        node = &m_nodes[node_index];
                    }
                    rect.bin_index = bin_index;
                    rect.x = node->x;
                    rect.y = node->y;
                    return true;
                }
                if (node->down_node != std::size_t(-1))
                    stack.push(node->down_node); // Throws
                if (node->right_node != std::size_t(-1)) {
                    node_index = node->right_node;
                    continue;
                }
              back:
                if (!stack.empty()) {
                    node_index = stack.top();
                    stack.pop();
                    continue;
                }
                ++bin_index;
                break; // Next bin
            }
        }
    };
    for (std::size_t i : m_order) {
        Rect& rect = m_rects[i];
        if (ARCHON_LIKELY(insert_rect(rect)))
            continue;
        return false;
    }
    return true;
}


} // namespace archon::util

#endif // ARCHON_X_UTIL_X_RECTANGLE_PACKER_HPP
