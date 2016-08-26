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
 *
 * Testing the kd-tree.
 */

#include <iostream>

#include <archon/core/memory.hpp>
#include <archon/core/random.hpp>
#include <archon/core/time.hpp>
#include <archon/core/iterator.hpp>
#include <archon/math/vec_ops.hpp>
#include <archon/util/kd_tree.hpp>


using namespace archon::core;
using namespace archon::math;
using namespace archon::util;


namespace {

template<class T> void test(int num_points, int num_components, int num_searches)
{
    std::cout << "test_dyn: Find "<<num_searches<<" points in cloud of "<<num_points<<" points "
        "each with "<<num_components<<" components" << std::endl;

    KdTreeSet<T> kd{num_components};

    std::cout << "Generating random points" << std::endl;
    std::size_t m = num_points * size_t(num_components);
    std::size_t n = num_searches * size_t(num_components);
    std::unique_ptr<T[]> buffer = std::make_unique<T[]>(m+n);
    Random r;
    for (std::size_t i = 0; i < m+n; ++i)
        buffer[i] = r.get_uniform();

    const T* points = buffer.get();
    const T* needles = points + m;

    std::cout << "Balancing kd-tree" << std::endl;
    RowIter<const T*> begin{points, num_components};
    kd.add(begin, begin + num_points);

    std::unique_ptr<T[]> first_results = std::make_unique<T[]>(num_searches);
    std::unique_ptr<T[]> second_results = std::make_unique<T[]>(num_searches);

    std::cout << "Searching... " << std::flush;
    Time start = Time::now();
    for (int i = 0; i < num_searches; ++i) {
        const T* needle = needles + i*size_t(num_components);
        const T* vec = kd.find_nearest(needle);
        first_results[i] = vec_sq_dist(needle, needle + num_components, vec);
    }
    Time time_first = Time::now() - start;
    std::cout << time_first.get_as_millis()<<"ms" << std::endl;


    std::cout << "Brute force check... " << std::flush;
    start = Time::now();
    for (int i = 0; i < num_searches; ++i) {
        const T* needle = needles + i*size_t(num_components);
        T min = vec_sq_dist(needle, needle + num_components, points);
        for (int j = 1; j < num_points; ++j) {
            T d = vec_sq_dist(needle, needle + num_components,
                              points + j*size_t(num_components));
            if (d < min)
                min = d;
        }
        second_results[i] = min;
    }
    Time time_second = Time::now() - start;
    std::cout << time_second.get_as_millis()<<"ms" << std::endl;

    std::cout << "Speedup: " << (time_second.get_as_micros()/
                                 double(time_first.get_as_micros())) << std::endl;


    int failures = 0;
    for (int i = 0; i < num_searches; ++i) {
        if (first_results[i] != second_results[i]) {
            ++failures;
            if (failures <= 10)
                std::cout << "Failure "<<failures<<": "
                    ""<<first_results[i]<<" != "<<second_results[i] << std::endl;
        }
    }
    if (failures) {
        std::cout << "FAILURES: "<<failures<<"/"<<num_searches << std::endl;
    }
    else {
        std::cout << "SUCCESS!!!" << std::endl;
    }
}

} // unnamed namespace


int main()
{
    test<double>(250000, 5, 5000);
    test<float>(256, 3, 5000000);
}
