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


#include <random>

#include <archon/core/random.hpp>
#include <archon/check.hpp>


using namespace archon;


ARCHON_TEST(Core_Random_SeedPrngNondeterministically)
{
    // Just check that the template function can be instantiated, and that it can be
    // executed without throwing and exception.

    std::mt19937_64 random;
    core::seed_prng_nondeterministically(random);
}


ARCHON_TEST(Core_Random_SeedSeq)
{
    std::mt19937_64 random(test_context.seed_seq());
    const std::size_t num_words = 20;
    std::array<core::SeedSeq::result_type, num_words> buffer;
    std::array<std::seed_seq::result_type, num_words> gen_buf_1;
    std::array<core::SeedSeq::result_type, num_words> gen_buf_2;
    for (int i = 0; i < 500; ++i) {
        for (std::size_t j = 0; j < buffer.size(); ++j)
            buffer[j] = core::SeedSeq::result_type(random());

        std::seed_seq seq_1(buffer.begin(), buffer.end());
        ARCHON_ASSERT(seq_1.size() == num_words);
        seq_1.generate(gen_buf_1.begin(), gen_buf_1.end());

        core::SeedSeq seq_2(buffer.begin(), buffer.end());
        ARCHON_CHECK_EQUAL(seq_2.size(), num_words);
        seq_2.generate(gen_buf_2.begin(), gen_buf_2.end());
        ARCHON_CHECK_EQUAL_SEQ(gen_buf_2, gen_buf_1);

        core::SeedSeq seq_3 = core::SeedSeq::no_copy(buffer);
        ARCHON_CHECK_EQUAL(seq_3.size(), num_words);
        seq_3.generate(gen_buf_2.begin(), gen_buf_2.end());
        ARCHON_CHECK_EQUAL_SEQ(gen_buf_2, gen_buf_1);

        const core::Span<const core::SeedSeq::result_type> parts[] = {
            core::Span(buffer).first(num_words / 2),
            core::Span(buffer).subspan(num_words / 2)
        };
        core::SeedSeq seq_4 = core::SeedSeq::no_copy_a(parts);
        ARCHON_CHECK_EQUAL(seq_4.size(), num_words);
        seq_4.generate(gen_buf_2.begin(), gen_buf_2.end());
        ARCHON_CHECK_EQUAL_SEQ(gen_buf_2, gen_buf_1);
    }
}
