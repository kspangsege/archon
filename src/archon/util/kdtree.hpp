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

#ifndef ARCHON_X_UTIL_X_KDTREE_HPP
#define ARCHON_X_UTIL_X_KDTREE_HPP

/// \file


#include <cmath>
#include <iterator>
#include <utility>
#include <algorithm>
#include <array>
#include <optional>

#include <archon/core/features.h>
#include <archon/core/type.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/math.hpp>


namespace archon::util {


/// \brief Produce balanced k-d tree.
///
/// This function produces a balanced k-d tree from the specified sequence of points. It
/// does this by reordering the points according to a particular, but unspecified scheme for
/// representing a k-d tree.
///
/// The number of dimensions in the tree is specified by \p k.
///
/// The scheme by which points are specified is highly customizable. The specified range may
/// contain the actual points, or it may contain something that refers to the actual points.
///
/// In any case, when this function needs the (`i` + 1)'th component of a point `p`, it
/// calls `get_comp(p, i)`. Here, `p` is one of the objects in the specified range (which
/// may, or may not be the point itself). The type of the object passed as first argument is
/// `std::iterator_traits<I>::value_type`, and the type of the object passed as second
/// argument is `int`.
///
/// Once the points have been sorted, the resulting k-d tree can be used with \ref
/// util::kdtree_find().
///
/// \sa \ref util::kdtree_find()
///
template<class I, class G> void kdtree_sort(int k, I begin, I end, G&& get_comp);


/// \brief Search for closest point in k-d tree.
///
/// If the specified range (\p begin -> \p end) is a sequence of points sorted by \ref
/// util::kdtree_sort(), `kdtree_find()` will search for the point that is closest to the
/// specified point (\p components). If a maximum distance is specified (\p max_dist), only
/// points closer than that will be considered.
///
/// The first \p k coordinate components in the array pointed to be \p components is taken
/// as specifying the point with respect to which the search is to be done. Note that \p k
/// is the number of dimensions in the k-d tree.
///
/// Parameters \p k and \p get_comp must be the same as (or be equivalent to) those passed
/// to \ref util::kdtree_sort().
///
/// If a point is found, this function returns `true` after setting \p point to that point
/// and \p dist to the distance to that point. Otherwise, this function returns `false` and
/// leaves \p point and \p dist unchanged.
///
/// If the specified range is empty, the value of \p k is immaterial.
///
/// Due to limited numeric accuracy in distance computations, if there are points that are
/// as close, or almost as close as the closest point, this function may not return the
/// point expected by the caller.
///
/// \sa \ref util::kdtree_sort()
///
template<class I, class G, class T, class P>
bool kdtree_find(int k, I begin, I end, G&& get_comp, const T* components,
                 std::optional<core::Type<T>> max_dist, P& point, T& dist);








// Implementation


namespace impl {


template<class I, class G> class Kdtree {
public:
    using iter_type = I;
    using get_comp_type = G;

    using size_type = typename std::iterator_traits<iter_type>::difference_type;
    using point_type = typename std::iterator_traits<iter_type>::value_type;

    static constexpr int max_levels = core::num_value_bits<size_type>();

    int k;
    iter_type begin, end;
    G get_comp;

    void sort() const;
    template<class T> bool find(const T* components, std::optional<T> max_dist, point_type& point, T& dist) const;

private:
    static auto midpoint(size_type a, size_type b) noexcept -> size_type
    {
        return size_type(a + (b - a - 1) / 2);
    }

    auto next_dim(int dim) const noexcept
    {
        return (dim + 1) % k;
    }
};


template<class I, class G>
void Kdtree<I, G>::sort() const
{
    // Using "non-recursive" implementation for efficiency and in order to have predictable
    // stack footprint (determinable at compile time).

    size_type size = size_type(end - begin);
    if (ARCHON_LIKELY(size != 0)) {
        ARCHON_ASSERT(size > 0);
        struct Range {
            int parents_dim;
            // INVARIANT: begin < end
            size_type begin, end;
        };
        std::array<Range, max_levels> pending_ranges;
        int num_pending_ranges = 0;
        int dim = 0; // Current splitting dimension
        size_type a = 0, b = size;

        auto sort = [&](const point_type& a, const point_type& b) {
            return get_comp(a, dim) < get_comp(b, dim); // Throws
        };

      again:
        ARCHON_ASSERT(a < b);
        size_type c = midpoint(a, b);
        std::nth_element(begin + a, begin + c, begin + b, sort); // Throws

        // Submit right sub-range as pending if nonempty
        size_type d = size_type(c + 1);
        if (d < b) {
            ARCHON_ASSERT(num_pending_ranges < max_levels);
            pending_ranges[num_pending_ranges] = { dim, d, b };
            ++num_pending_ranges;
        }

        // Enter into left sub-range if nonempty
        if (a < c) {
            dim = next_dim(dim);
            b = c;
            goto again;
        }

        if (ARCHON_LIKELY(num_pending_ranges > 0)) {
            --num_pending_ranges;
            Range range = pending_ranges[num_pending_ranges];
            dim = next_dim(range.parents_dim);
            a = range.begin;
            b = range.end;
            goto again;
        }
    }
}


template<class I, class G>
template<class T> bool Kdtree<I, G>::find(const T* components, std::optional<T> max_dist, point_type& point,
                                          T& dist) const
{
    using value_type = T;

    // Using "non-recursive" implementation for efficiency and in order to have predictable
    // stack footprint (determinable at compile time).

    auto size = end - begin;
    if (ARCHON_LIKELY(size != 0)) {
        ARCHON_ASSERT(size > 0);
        struct Transrange {
            int parents_dim;
            size_type parents_midpoint;
            // INVARIANT: begin < end
            size_type begin, end;
        };
        std::array<Transrange, max_levels> pending_transranges;
        int num_pending_transranges = 0;
        value_type sqdist = 0;
        bool have_sqdist = false;
        if (max_dist.has_value()) {
            sqdist = core::square(max_dist.value());
            have_sqdist = true;
        }
        size_type index = size_type(-1);
        int dim = 0; // Current splitting dimension
        size_type a = 0, b = size;

      enter:
        ARCHON_ASSERT(a < b);
        size_type c = midpoint(a, b);

        // Deal with point in current node
        value_type sqdist_2 = 0;
        for (int i = 0; i < k; ++i)
            sqdist_2 += core::square(get_comp(begin[c], i) - components[i]); // Throws
        if (ARCHON_UNLIKELY(!have_sqdist || sqdist_2 < sqdist)) {
            sqdist = sqdist_2;
            have_sqdist = true;
            index = c;
        }

        size_type d = size_type(c + 1);
        if (components[dim] <= get_comp(begin[c], dim)) { // Throws
            // Query point lies below splitting plane

            // Submit right sub-range as pending if nonempty
            if (d < b) {
                ARCHON_ASSERT(num_pending_transranges < max_levels);
                pending_transranges[num_pending_transranges] = { dim, c, d, b };
                ++num_pending_transranges;
            }

            // Enter into left sub-range if nonempty
            if (a < c) {
                dim = next_dim(dim);
                b = c;
                goto enter;
            }
        }
        else {
            // Query point lies above splitting plane

            // Submit left sub-range as pending if nonempty
            if (a < c) {
                ARCHON_ASSERT(num_pending_transranges < max_levels);
                pending_transranges[num_pending_transranges] = { dim, c, a, c };
                ++num_pending_transranges;
            }

            // Enter into right sub-range if nonempty
            if (d < b) {
                dim = next_dim(dim);
                a = d;
                goto enter;
            }
        }


      leave:
        if (ARCHON_LIKELY(num_pending_transranges > 0)) {
            --num_pending_transranges;
            Transrange transrange = pending_transranges[num_pending_transranges];
            // If the separation plane is further away from the query point than the
            // currently closest point, no point on the other side of the plane can be a
            // candidate for closest point.
            dim = transrange.parents_dim;
            c = transrange.parents_midpoint;
            value_type sqdist_3 = core::square(get_comp(begin[c], dim) - components[dim]); // Throws
            ARCHON_ASSERT(have_sqdist);
            if (ARCHON_LIKELY(sqdist_3 >= sqdist))
                goto leave;
            dim = next_dim(dim);
            a = transrange.begin;
            b = transrange.end;
            goto enter;
        }

        // If a point was found, return it, and the distance to it
        if (index != size_type(-1)) {
            point = begin[index];
            dist = std::sqrt(sqdist);
            return true;
        }
    }

    return false; // No points found
}


} // namespace impl


template<class I, class G> inline void kdtree_sort(int k, I begin, I end, G&& get_comp)
{
    impl::Kdtree<I, G> tree = { k, begin, end, std::forward<G>(get_comp) }; // Throws
    tree.sort(); // Throws
}


template<class I, class G, class T, class P>
inline bool kdtree_find(int k, I begin, I end, G&& get_comp, const T* components,
                        std::optional<core::Type<T>> max_dist, P& point, T& dist)
{
    impl::Kdtree<I, G> tree = { k, begin, end, std::forward<G>(get_comp) }; // Throws
    return tree.template find<T>(components, std::move(max_dist), point, dist); // Throws
}


} // namespace archon::util

#endif // ARCHON_X_UTIL_X_KDTREE_HPP
