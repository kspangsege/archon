// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
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


#include <archon/check/random_seed.hpp>


using namespace archon;
using RandomSeed = check::RandomSeed;


RandomSeed::RandomSeed(core::Span<const value_type> values)
{
    std::size_t num_blocks = std::size_t(values.size() / 6);
    std::size_t size = std::size_t(num_blocks * 6);
    m_slab.recreate(size, {}); // Throws
    std::copy_n(values.data(), size, m_slab.data());
}


auto RandomSeed::random(std::size_t num_blocks) -> RandomSeed
{
    std::size_t n = num_blocks;
    core::int_mul(n, 6); // Throws
    RandomSeed seed;
    seed.m_slab.recreate(n, {}); // Throws
    core::seed_nondeterministically_a(n, [&](const core::SeedSeq& seed_seq) {
        auto begin = seed.m_slab.data();
        auto end   = begin + n;
        seed_seq.generate(begin, end); // Throws
    }); // Throws
    return seed;
}
