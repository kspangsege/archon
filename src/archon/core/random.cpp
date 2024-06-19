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


#include <cstddef>
#include <atomic>
#include <chrono>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/random.hpp>


#if ARCHON_WINDOWS
#  if !defined NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#else
#  include <unistd.h>
#endif


using namespace archon;


using core::impl::NondeterministicRandomSeeder;


void NondeterministicRandomSeeder::get_extra_entropy(extra_entropy_type& extra_entropy)
{
    ARCHON_ASSERT(extra_entropy.size() == 3);

    using clock = std::chrono::high_resolution_clock;
    using uint = unsigned int;
    extra_entropy[0] = uint(clock::now().time_since_epoch().count());

#if ARCHON_WINDOWS
    extra_entropy[1] = uint(GetCurrentProcessId());
#else
    extra_entropy[1] = uint(getpid());
#endif

    static std::atomic<unsigned int> counter { 0 };
    extra_entropy[2] = ++counter;
}


using core::SeedSeq;


auto SeedSeq::no_copy_a(core::Span<const span_type> seq_seq) -> SeedSeq
{
    // Check that sizes can be summed without overflow
    std::size_t size = 0;
    for (auto seq : seq_seq)
        core::int_add(size, seq.size()); // Throws

    SeedSeq seq_2;
    seq_2.m_seq_seq = seq_seq;
    return seq_2;
}


auto SeedSeq::size() const noexcept -> std::size_t
{
    std::size_t size = 0;
    for (auto seq : m_seq_seq) {
        bool success = core::try_int_add(size, seq.size());
        ARCHON_ASSERT(success); // Ensured at construction time
    }
    return size;
}
