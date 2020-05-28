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
#include <cmath>
#include <string_view>
#include <vector>
#include <random>
#include <ios>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/buffer_contents.hpp>
#include <archon/core/seed_memory_output_stream.hpp>
#include <archon/core/as_list.hpp>
#include <archon/core/random.hpp>
#include <archon/core/word_wrap.hpp>
#include <archon/log.hpp>
#include <archon/check.hpp>
#include <archon/core/test/word_wrap_knuth_alt.hpp>


using namespace archon;
namespace word_wrap = core::word_wrap;
namespace test = core::test;


namespace {


class Sampler {
public:
    Sampler(std::mt19937_64& random) noexcept
        : m_random(random)
    {
    }

    auto sample_words(std::size_t num_words, std::size_t max_word_size) -> std::vector<word_wrap::Word>
    {
        std::size_t space_size = 0;
        std::vector<word_wrap::Word> result;
        for (std::size_t i = 0; i < num_words; ++i) {
            std::size_t word_size = sample_word_size(max_word_size); // Throws
            result.push_back({ space_size, word_size }); // Throws
            space_size = 1;
        }
        return result;
    }

private:
    std::mt19937_64& m_random;

    auto geo(double p) -> std::size_t
    {
        return std::size_t(std::floor(std::log(core::rand_float<double>(m_random)) / std::log(1 - p))); // Throws
    }

    auto sample_word_size(std::size_t max_word_size) -> std::size_t
    {
        int n = 10;
        double p = 0.75;
        for (;;) {
            std::size_t size = 1;
            for (int i = 0; i < n; ++i)
                size += geo(p); // Throws
            if (size <= max_word_size)
                return size;
        }
    }
};


auto render(core::Span<const word_wrap::Word> words, std::size_t trailing_space_size,
            core::Span<const std::size_t> breakpoints, core::Buffer<char>& buffer) -> std::string_view
{
    core::BufferContents contents(buffer);
    if (!words.empty()) {
        auto line = [&](std::size_t begin, std::size_t end) {
            for (std::size_t i = begin; i < end; ++i) {
                word_wrap::Word word = words[i];
                if (i > begin || i == 0)
                    contents.append(word.space_size, ' ');
                contents.append(word.word_size, 'x');
            }
        };
        std::size_t prev_word_index = 0;
        for (std::size_t breakpoint : breakpoints) {
            line(prev_word_index, breakpoint);
            contents.append(1, '\n');
            prev_word_index = breakpoint;
        }
        line(prev_word_index, words.size());
    }
    contents.append(trailing_space_size, ' ');
    contents.append(1, '\n');
    return { contents.data(), contents.size() };
}


void parse(std::string_view text, std::vector<word_wrap::Word>& words, std::size_t& trailing_space_size,
           std::vector<std::size_t>& breakpoints)
{
    std::size_t i = 0;
    std::size_t n = text.size();
    for (;;) {
        // Scan across space
        std::size_t mark = i;
        for (;;) {
            if (ARCHON_UNLIKELY(i == n)) {
                trailing_space_size = std::size_t(i - mark);
                return;
            }
            char ch = text[i];
            if (ARCHON_LIKELY(ch != ' ')) {
                if (ARCHON_LIKELY(ch != '\n'))
                    break;
                breakpoints.push_back(words.size()); // Throws
            }
            ++i;
        }
        std::size_t space_size = std::size_t(i - mark);

        // Scan across word
        mark = i;
        for (;;) {
            ++i;
            if (ARCHON_UNLIKELY(i == n))
                break;
            char ch = text[i];
            if (ARCHON_UNLIKELY(ch == ' ' || ch == '\n'))
                break;
        }
        std::size_t word_size = std::size_t(i - mark);
        words.push_back({ space_size, word_size }); // Throws
    }
}


class KnuthImplComparator {
public:
    KnuthImplComparator(log::Logger& logger)
        : m_logger(logger)
        , m_out() // Throws
    {
        m_out.exceptions(std::ios_base::badbit | std::ios_base::failbit); // Throws
        m_out.imbue(m_logger.get_locale()); // Throws
    }

    bool compare(core::Span<const word_wrap::Word> words, std::size_t trailing_space_size,
                 core::Span<const word_wrap::Geometry> geometry)
    {
        m_wrapper.wrap(words, trailing_space_size, geometry, m_breakpoints_1); // Throws
        using badness_type = word_wrap::KnuthWrapper::badness_type;
        badness_type badness_1 = m_wrapper.get_badness();
        badness_type badness_2 = test::word_wrap_knuth_alt(words, trailing_space_size, geometry,
                                                           m_breakpoints_2); // Throws
        bool same = (m_breakpoints_1 == m_breakpoints_2 && badness_1 == badness_2);
        if (ARCHON_LIKELY(same))
            return true;
        m_out << "Mismatch detected\n"; // Throws
        m_out << "========================== breakpoints_1 ==========================\n"; // Throws
        m_out << "Badness: " << badness_1 << "\n"; // Throws
        m_out << render(words, trailing_space_size, m_breakpoints_1, m_buffer); // Throws
        m_out << "========================== breakpoints_2 ==========================\n"; // Throws
        m_out << "Badness: " << badness_2 << "\n"; // Throws
        m_out << render(words, trailing_space_size, m_breakpoints_2, m_buffer); // Throws
        m_out << "==================================================================="; // Throws
        m_logger.error("%s", m_out.view()); // Throws
        return false;
    }

private:
    log::Logger& m_logger;
    core::SeedMemoryOutputStream m_out;
    core::Buffer<char> m_buffer;
    word_wrap::KnuthWrapper m_wrapper;
    std::vector<std::size_t> m_breakpoints_1, m_breakpoints_2;
};


} // unnamed namespace


ARCHON_TEST(Core_WordWrap_GreedyCase1)
{
    std::string_view text =
        "xxxxxxxxxxxxx x\n"
        "xxxxxxx xxxx x\n"
        "xxx x x x\n"
        "xxxxxx xxx\n"
        "xxxxxxx\n"
        "xxxxxxxxxxxxxxx";
    std::vector<word_wrap::Word> words;
    std::size_t trailing_space_size = 0;
    std::vector<std::size_t> breakpoints_1, breakpoints_2;
    parse(text, words, trailing_space_size, breakpoints_1);
    std::size_t format_width = 15;
    word_wrap::Geometry geometry[] = {
        { format_width, 0 },
    };
    word_wrap::greedy(words, trailing_space_size, geometry, breakpoints_2);
    // FIXME: Should instead use ARCHON_CHECK_EQUAL_SEQ here and in many of the tests below                                                             
    if (ARCHON_UNLIKELY(!ARCHON_CHECK(breakpoints_1 == breakpoints_2))) {
        log("1: %s", core::as_list(breakpoints_1));
        log("2: %s", core::as_list(breakpoints_2));
    }
}


ARCHON_TEST(Core_WordWrap_KnuthCase1)
{
    std::string_view text =
        "xxxxxxxxxxxxx\n"
        "x xxxxxxx\n"
        "xxxx x xxx x\n"
        "x x xxxxxx\n"
        "xxx xxxxxxx\n"
        "xxxxxxxxxxxxxxx";
    std::vector<word_wrap::Word> words;
    std::size_t trailing_space_size = 0;
    std::vector<std::size_t> breakpoints_1, breakpoints_2, breakpoints_3;
    parse(text, words, trailing_space_size, breakpoints_1);
    std::size_t format_width = 15;
    word_wrap::Geometry geometry[] = {
        { format_width, 0 },
    };
    word_wrap::KnuthWrapper wrapper;
    wrapper.wrap(words, trailing_space_size, geometry, breakpoints_2);
    if (ARCHON_UNLIKELY(!ARCHON_CHECK(breakpoints_1 == breakpoints_2))) {
        log("1: %s", core::as_list(breakpoints_1));
        log("2: %s", core::as_list(breakpoints_2));
    }
    test::word_wrap_knuth_alt(words, trailing_space_size, geometry, breakpoints_3);
    if (ARCHON_UNLIKELY(!ARCHON_CHECK(breakpoints_1 == breakpoints_3))) {
        log("1: %s", core::as_list(breakpoints_1));
        log("2: %s", core::as_list(breakpoints_3));
    }
}


ARCHON_TEST(Core_WordWrap_CompareKnuthWithAltImpl_Case1)
{
    std::string_view text =
        "xxxxxxxxxxxxx\n"
        "x xxxxxxx\n"
        "xxxx x xxx x\n"
        "x x xxxxxx\n"
        "xxx xxxxxxx\n"
        "xxxxxxxxxxxxxxx";
    std::vector<word_wrap::Word> words;
    std::size_t trailing_space_size = 0;
    std::vector<std::size_t> breakpoints; // Dummy
    parse(text, words, trailing_space_size, breakpoints);
    std::size_t format_width = 15;
    word_wrap::Geometry geometry[] = {
        { format_width, 0 },
    };
    KnuthImplComparator comparator(test_context.logger);
    ARCHON_CHECK(comparator.compare(words, trailing_space_size, geometry));
}


ARCHON_TEST(Core_WordWrap_GreedyCase2)
{
    std::string_view text =
        "otorhinolaryngological\n"
        "immunoelectrophoretically\n"
        "psychophysicotherapeutics\n"
        "thyroparathyroidectomized\n"
        "pneumoencephalographically\n"
        "radioimmunoelectrophoresis\n"
        "psychoneuroendocrinological\n"
        "hepaticocholangiogastrostomy\n"
        "spectrophotofluorometrically\n"
        "pseudopseudohypoparathyroidism";
    std::vector<word_wrap::Word> words;
    std::size_t trailing_space_size = 0;
    std::vector<std::size_t> breakpoints_1, breakpoints_2;
    parse(text, words, trailing_space_size, breakpoints_1);
    std::size_t format_width = 20;
    word_wrap::Geometry geometry[] = {
        { format_width, 0 },
    };
    word_wrap::greedy(words, trailing_space_size, geometry, breakpoints_2);
    if (ARCHON_UNLIKELY(!ARCHON_CHECK(breakpoints_1 == breakpoints_2))) {
        log("1: %s", core::as_list(breakpoints_1));
        log("2: %s", core::as_list(breakpoints_2));
    }
}


ARCHON_TEST(Core_WordWrap_KnuthCase2)
{
    std::string_view text =
        "otorhinolaryngological\n"
        "immunoelectrophoretically\n"
        "psychophysicotherapeutics\n"
        "thyroparathyroidectomized\n"
        "pneumoencephalographically\n"
        "radioimmunoelectrophoresis\n"
        "psychoneuroendocrinological\n"
        "hepaticocholangiogastrostomy\n"
        "spectrophotofluorometrically\n"
        "pseudopseudohypoparathyroidism";
    std::vector<word_wrap::Word> words;
    std::size_t trailing_space_size = 0;
    std::vector<std::size_t> breakpoints_1, breakpoints_2, breakpoints_3;
    parse(text, words, trailing_space_size, breakpoints_1);
    std::size_t format_width = 20;
    word_wrap::Geometry geometry[] = {
        { format_width, 0 },
    };
    word_wrap::KnuthWrapper wrapper;
    wrapper.wrap(words, trailing_space_size, geometry, breakpoints_2);
    if (ARCHON_UNLIKELY(!ARCHON_CHECK(breakpoints_1 == breakpoints_2))) {
        log("1: %s", core::as_list(breakpoints_1));
        log("2: %s", core::as_list(breakpoints_2));
    }
    test::word_wrap_knuth_alt(words, trailing_space_size, geometry, breakpoints_3);
    if (ARCHON_UNLIKELY(!ARCHON_CHECK(breakpoints_1 == breakpoints_3))) {
        log("1: %s", core::as_list(breakpoints_1));
        log("2: %s", core::as_list(breakpoints_3));
    }
}


ARCHON_TEST(Core_WordWrap_GreedyCase3)
{
    std::string_view text =
        "         x\n"
        "x\n"
        "x         ";
    std::vector<word_wrap::Word> words;
    std::size_t trailing_space_size = 0;
    std::vector<std::size_t> breakpoints_1, breakpoints_2;
    parse(text, words, trailing_space_size, breakpoints_1);
    std::size_t format_width = 10;
    word_wrap::Geometry geometry[] = {
        { format_width, 0 },
    };
    word_wrap::greedy(words, trailing_space_size, geometry, breakpoints_2);
    if (ARCHON_UNLIKELY(!ARCHON_CHECK(breakpoints_1 == breakpoints_2))) {
        log("1: %s", core::as_list(breakpoints_1));
        log("2: %s", core::as_list(breakpoints_2));
    }
}


ARCHON_TEST(Core_WordWrap_KnuthCase3)
{
    std::string_view text =
        "         x\n"
        "x\n"
        "x         ";
    std::vector<word_wrap::Word> words;
    std::size_t trailing_space_size = 0;
    std::vector<std::size_t> breakpoints_1, breakpoints_2, breakpoints_3;
    parse(text, words, trailing_space_size, breakpoints_1);
    std::size_t format_width = 10;
    word_wrap::Geometry geometry[] = {
        { format_width, 0 },
    };
    word_wrap::KnuthWrapper wrapper;
    wrapper.wrap(words, trailing_space_size, geometry, breakpoints_2);
    if (ARCHON_UNLIKELY(!ARCHON_CHECK(breakpoints_1 == breakpoints_2))) {
        log("1: %s", core::as_list(breakpoints_1));
        log("2: %s", core::as_list(breakpoints_2));
    }
    test::word_wrap_knuth_alt(words, trailing_space_size, geometry, breakpoints_3);
    if (ARCHON_UNLIKELY(!ARCHON_CHECK(breakpoints_1 == breakpoints_3))) {
        log("1: %s", core::as_list(breakpoints_1));
        log("2: %s", core::as_list(breakpoints_3));
    }
}


ARCHON_TEST(Core_WordWrap_CompareKnuthWithAltImpl_Fuzzer)
{
    std::size_t format_width = 26;
    std::size_t num_words = 300;
    std::size_t max_word_size = format_width;
    std::mt19937_64 random(test_context.seed_seq());
    Sampler sampler(random);
    KnuthImplComparator comparator(test_context.logger);
    long n = 100;
    for (long i = 0; i < n; ++i) {
        std::vector<word_wrap::Word> words = sampler.sample_words(num_words, max_word_size);
        word_wrap::Geometry geometry[] = {
            { format_width + 0, 1 },
            { format_width + 3, 2 },
            { format_width + 7, 3 },
            { format_width + 5, 0 },
        };
        std::size_t trailing_space_size = 0;
        bool success = ARCHON_CHECK(comparator.compare(words, trailing_space_size, geometry));
        if (ARCHON_UNLIKELY(!success)) {
            log("Format width:    %s", format_width);
            log("Number of words: %s", num_words);
            break;
        }
    }
}


ARCHON_TEST_IF(Core_WordWrap_KnuthSpeedTest, false)
{
    // Fast random seed: AvaKQWZqcixE1TIUjOkAf15Eu0KAtX4er-GrmB1GlfLOK9ZwIE3KNSWrIpGbiShstj6
    // Slow random seed: GEVVtWa7vxz4GPsHwMymTJJfVbKJMtHx4-IbsZGTerca37WXSqHweyiJ8cZoNGkD1xx
    //
    // For 100'000 invocations with 800 words and format width being 80:
    //   SLOW SEED, FAST IMPL: 9.97s
    //   SLOW SEED, SLOW IMPL:   78s
    //   FAST SEED, FAST IMPL: 1.07s
    //   FAST SEED, SLOW IMPL: 7.33s
    //
    // Conclusion: Good nonrecursive implementation of Kuth is about 7 times faster than the
    // reference recursive implementation of Knuth

    const bool alt = false;
    std::size_t format_width = 80;
    std::size_t num_words = 800;
    std::size_t max_word_size = format_width;
    std::mt19937_64 random(test_context.seed_seq());
    Sampler sampler(random);
    std::vector<word_wrap::Word> words = sampler.sample_words(num_words, max_word_size);
    std::size_t trailing_space_size = 0;
    word_wrap::Geometry geometry[] = {
        { format_width, 0 },
    };
    word_wrap::KnuthWrapper wrapper;
    std::vector<std::size_t> breakpoints;
    for (long long i = 0; i < 100000; ++i) {
        if (alt) {
            test::word_wrap_knuth_alt(words, trailing_space_size, geometry, breakpoints);
        }
        else {
            wrapper.wrap(words, trailing_space_size, geometry, breakpoints);
        }
    }
    core::Buffer<char> buffer;
    log("%s", render(words, trailing_space_size, breakpoints, buffer));
}
