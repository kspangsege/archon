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


#include <cstddef>
#include <limits>
#include <algorithm>
#include <array>
#include <optional>
#include <vector>
#include <random>

#include <archon/core/features.h>
#include <archon/core/inexact_compare.hpp>
#include <archon/core/random.hpp>
#include <archon/check.hpp>
#include <archon/math/vector.hpp>
#include <archon/util/kdtree.hpp>


using namespace archon;


ARCHON_TEST(Util_Kdtree_Nonempty)
{
    std::array<std::size_t, 0> indexes;

    bool get_comp_called = false;
    auto get_comp = [&](std::size_t, int) -> double {
        get_comp_called = true;
        return {};
    };

    double components[] = { 0, 0, 0 };
    std::optional<double> max_dist = {};
    std::size_t index = 0;
    double dist = 0;
    ARCHON_CHECK_NOT(util::kdtree_find(3, std::begin(indexes), std::end(indexes), get_comp, components, max_dist,
                                       index, dist));
}


ARCHON_TEST(Util_Kdtree_Basics)
{
    std::array<double, 3> points[] = {
        { 1, 0, 0 },
        { 0, 1, 0 },
        { 0, 0, 1 },
    };

    std::size_t indexes[std::size(points)];
    for (std::size_t i = 0; i < std::size(indexes); ++i)
        indexes[i] = i;

    auto get_comp = [&](std::size_t index, int i) {
        return points[index][i];
    };

    util::kdtree_sort(3, std::begin(indexes), std::end(indexes), get_comp);

    std::size_t index = 0;
    double dist = 0;
    auto find = [&](double x, double y, double z) {
        double components[] = { x, y, z };
        std::optional<double> max_dist = {};
        return util::kdtree_find(3, std::begin(indexes), std::end(indexes), get_comp, components, max_dist,
                                 index, dist);
    };

    double eps = std::numeric_limits<double>::epsilon() * 5;

    if (ARCHON_LIKELY(ARCHON_CHECK(find(1, 0, 0)))) {
        ARCHON_CHECK_EQUAL(index, 0);
        ARCHON_CHECK_APPROXIMATELY_EQUAL(dist, 0, eps);
    }
    if (ARCHON_LIKELY(ARCHON_CHECK(find(0, 2, 0)))) {
        ARCHON_CHECK_EQUAL(index, 1);
        ARCHON_CHECK_APPROXIMATELY_EQUAL(dist, 1, eps);
    }
    if (ARCHON_LIKELY(ARCHON_CHECK(find(0, 0, 3)))) {
        ARCHON_CHECK_EQUAL(index, 2);
        ARCHON_CHECK_APPROXIMATELY_EQUAL(dist, 2, eps);
    }

    auto find_2 = [&](double x, double y, double z, double max_dist) {
        double components[] = { x, y, z };
        return util::kdtree_find(3, std::begin(indexes), std::end(indexes), get_comp, components, max_dist,
                                 index, dist);
    };

    if (ARCHON_LIKELY(ARCHON_CHECK(find_2(1, 0, 0, 1)))) {
        ARCHON_CHECK_EQUAL(index, 0);
        ARCHON_CHECK_APPROXIMATELY_EQUAL(dist, 0, eps);
    }

    if (ARCHON_LIKELY(ARCHON_CHECK(find_2(0.5, 0, 0, 1)))) {
        ARCHON_CHECK_EQUAL(index, 0);
        ARCHON_CHECK_APPROXIMATELY_EQUAL(dist, 0.5, eps);
    }

    ARCHON_CHECK_NOT(find_2(1, 1, 1, 1));
}


ARCHON_TEST(Util_Kdtree_Randomized)
{
    std::mt19937_64 random(test_context.seed_seq());

    constexpr std::size_t num_points = 16;
    std::array<math::Vector3, num_points> points;

    for (std::size_t i = 0; i < num_points; ++i) {
        math::Vector3 point;
        for (std::size_t i = 0; i < 3; ++i)
            point[i] = core::rand_float<double>(random);
        points[i] = point;
    }

    std::size_t indexes[num_points];
    for (std::size_t i = 0; i < num_points; ++i)
        indexes[i] = i;

    auto get_comp = [&](std::size_t index, int i) {
        return points[index][i];
    };

    util::kdtree_sort(3, std::begin(indexes), std::end(indexes), get_comp);

    std::vector<std::size_t> candidates;
    constexpr int num_lookups = 128;
    for (int i = 0; i < num_lookups; ++i) {
        math::Vector3 point;
        for (std::size_t i = 0; i < 3; ++i)
            point[i] = core::rand_float<double>(random);

        std::optional<double> max_dist = {};
        std::size_t index = 0;
        double dist = 0;
        bool found = util::kdtree_find(3, std::begin(indexes), std::end(indexes), get_comp, point.components().data(),
                                       max_dist, index, dist);
        ARCHON_CHECK(found);

        candidates.clear();
        double eps = std::numeric_limits<double>::epsilon() * 10;
        for (std::size_t j = 0; j < num_points; ++j) {
            double dist_2 = math::len(point - points[j]);
            ARCHON_CHECK_NOT_DEFINITELY_LESS(dist_2, dist, eps);
            if (!core::definitely_greater(dist_2, dist, eps))
                candidates.push_back(j);
        }
        auto j = std::find(candidates.begin(), candidates.end(), index);
        ARCHON_CHECK(j != candidates.end());
    }
}
