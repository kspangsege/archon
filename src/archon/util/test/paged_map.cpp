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
///
/// Testing the paged map.
///
/// Status in its form as of Sep 16 2009:
///
/// <pre>
///
///               Time      Memory
///   --------------------------------
///   PagedMap    0.5s      140MB
///   std::map    3.1s      770MB
///
/// </pre>

#include <cmath>
#include <iostream>

#include <archon/core/random.hpp>
#include <archon/util/paged_map.hpp>


using namespace archon::core;
using namespace archon::util;


int main()
{
    int range = std::numeric_limits<int>::max();

    int num_chunks = 256;
    int typical_chunk_size = 256;

    double lambda = 16;
    double cutoff = lambda + 4.5 * sqrt(lambda);

    Random random;
    Random::UniformDistrib uniform(&random);
    std::unique_ptr<Random::Distribution> poisson = Random::get_poisson_distrib(lambda);

    for (int ii = 0; ii < 256; ++ii) {
        PagedMap<int, int>* map = new PagedMap<int, int>(typical_chunk_size);
        // std::map<int, int>* map = new std::map<int, int>;

        for (int i = 0; i < num_chunks; ++i) {
            double v;
            do {
                v = poisson->get();
            }
            while (cutoff <= v);
            int size = 16 * v;
            int pos = floor(uniform() * (range-size+1));
            for (int j = 0; j < size; ++j)
                (*map)[pos+j] = pos;
        }
    }

/*
    std::cerr << "Done\n";
    pause();
*/
}
