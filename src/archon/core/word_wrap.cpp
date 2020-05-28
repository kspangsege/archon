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
#include <limits>
#include <stdexcept>
#include <vector>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/word_wrap.hpp>


using namespace archon;
namespace word_wrap = core::word_wrap;
using word_wrap::KnuthWrapper;


void word_wrap::greedy(core::Span<const word_wrap::Word> words, std::size_t trailing_space_size,
                       core::Span<const word_wrap::Geometry> geometry, std::vector<std::size_t>& breakpoints,
                       std::size_t first_geometry_index)
{
    breakpoints.clear();

    std::size_t num_words = words.size();
    if (ARCHON_UNLIKELY(num_words < 2))
        return;

    std::size_t geom_index = first_geometry_index;
    std::size_t word_index = 0;
    std::size_t line_size, cursor_pos;

  first:
    // Place first word on current output line
    {
        ARCHON_ASSERT(geom_index < geometry.size());
        line_size = geometry[geom_index].line_size;
        const word_wrap::Word& word = words[word_index];
        cursor_pos = word.word_size;
        bool is_first_word = (word_index == 0);
        if (ARCHON_UNLIKELY(is_first_word)) {
            if (ARCHON_UNLIKELY(!core::try_int_add(cursor_pos, word.space_size)))
                cursor_pos = std::numeric_limits<std::size_t>::max();
        }
    }

  next:
    // If it fits, place another word on current output line
    ++word_index;
    if (ARCHON_LIKELY(word_index < num_words)) {
        if (ARCHON_LIKELY(line_size >= cursor_pos)) {
            std::size_t rest = std::size_t(line_size - cursor_pos);
            const word_wrap::Word& word = words[word_index];
            if (ARCHON_LIKELY(rest >= word.word_size)) {
                rest -= word.word_size;
                if (ARCHON_LIKELY(rest >= word.space_size)) {
                    rest -= word.space_size;
                    bool is_last_word = (word_index == std::size_t(num_words - 1));
                    if (ARCHON_LIKELY(!is_last_word)) {
                        cursor_pos = std::size_t(line_size - rest);
                        goto next;
                    }
                    else if (ARCHON_LIKELY(rest >= trailing_space_size)) {
                        rest -= trailing_space_size;
                        cursor_pos = std::size_t(line_size - rest);
                        goto next;
                    }
                }
            }
        }

        // Break onto new output line
        breakpoints.push_back(word_index); // Throws
        ARCHON_ASSERT(geom_index < geometry.size());
        geom_index = geometry[geom_index].next_geometry_index;
        goto first;
    }
}


void KnuthWrapper::wrap(core::Span<const word_wrap::Word> words, std::size_t trailing_space_size,
                        core::Span<const word_wrap::Geometry> geometry, std::vector<std::size_t>& breakpoints,
                        std::size_t first_geometry_index)
{
    // To "move right" means to "place the current word on the current output line, then
    // move to the next word".

    // To "move down" means to "break onto a new output line".

    breakpoints.clear();

    std::size_t num_words = words.size();
    if (ARCHON_UNLIKELY(num_words < 2))
        return;

    m_word_slots.resize(num_words); // Throws
    for (std::size_t i = 0; i < num_words; ++i) {
        const word_wrap::Word& word = words[i];
        std::size_t word_size = word.word_size;
        bool is_first_word = (i == 0);
        if (ARCHON_UNLIKELY(is_first_word)) {
            if (ARCHON_UNLIKELY(!core::try_int_add(word_size, word.space_size)))
                word_size = std::numeric_limits<std::size_t>::max();
        }
        bool is_last_word = (i == std::size_t(num_words - 1));
        if (ARCHON_UNLIKELY(is_last_word)) {
            if (ARCHON_UNLIKELY(!core::try_int_add(word_size, trailing_space_size)))
                word_size = std::numeric_limits<std::size_t>::max();
        }
        m_word_slots[i].word_size = word_size;
    }

    // Unpack sufficient prefix of geometry
    std::size_t num_geometries;
    {
        std::size_t max_num_lines = num_words;
        m_line_slots.resize(max_num_lines); // Throws
        std::size_t geom_index = first_geometry_index;
        std::size_t max_geom_index = 0;
        std::size_t line_index = 0;
        for (;;) {
            LineSlot& line_slot = m_line_slots[line_index];
            ARCHON_ASSERT(geom_index < geometry.size());
            const word_wrap::Geometry& g = geometry[geom_index];
            line_slot.size = g.line_size;
            line_slot.geom_index = geom_index;
            if (ARCHON_UNLIKELY(++line_index == max_num_lines))
                break;
            geom_index = g.next_geometry_index;
            if (ARCHON_UNLIKELY(geom_index > max_geom_index))
                max_geom_index = geom_index;
        }
        num_geometries = std::size_t(max_geom_index + 1);
    }

    std::size_t cache_size = num_words;
    core::int_mul(cache_size, num_geometries); // Throws
    m_cache.clear();
    m_cache.resize(cache_size); // Throws
    CacheSlot* cache = m_cache.data();

    badness_type max = std::numeric_limits<badness_type>::max();
    badness_type bound = max;

    m_results = { Result::indefinite(max) };
    m_breakpoints.clear();

    std::size_t unused_breakpoint_index = std::size_t(-1);
    auto alloc_breakpoint = [&](std::size_t word_index, std::size_t next_breakpoint_index) {
        std::size_t index;
        if (ARCHON_LIKELY(unused_breakpoint_index == std::size_t(-1))) {
            index = m_breakpoints.size();
            m_breakpoints.push_back({ word_index, next_breakpoint_index }); // Throws
        }
        else {
            index = unused_breakpoint_index;
            Breakpoint& breakpoint = m_breakpoints[index];
            unused_breakpoint_index = breakpoint.next_breakpoint_index;
            breakpoint = { word_index, next_breakpoint_index };
        }
        return index;
    };
    auto recycle_breakpoint = [&](const Result& result) {
        if (ARCHON_LIKELY(result.is_indefinite()))
            return;
        std::size_t index = result.breakpoint_index;
        if (ARCHON_LIKELY(index != std::size_t(-1))) {
            m_breakpoints[index].next_breakpoint_index = unused_breakpoint_index;
            unused_breakpoint_index = index;
        }
    };

    std::size_t word_index   = 0;
    std::size_t line_index   = 0;
    std::size_t result_index = 0;
    std::size_t line_size, cursor_pos;

    m_line_slots[0].word_index = 0;

  move_right_1:
    // Place first word on current line
    {
        const Result& result = m_results[result_index];
        ARCHON_ASSERT(result.badness == max);
        ARCHON_ASSERT(result.is_indefinite());
        line_size = m_line_slots[line_index].size;
        WordSlot& word_slot = m_word_slots[word_index];
        word_slot.cursor_pos = 0;
        cursor_pos = word_slot.word_size;
    }

  move_right_2:
    ++word_index;
    if (ARCHON_UNLIKELY(word_index == num_words)) {
        // At leaf
        m_results[result_index] = Result::definite(0);
        goto move_up;
    }

    // Try place another word on current line
    {
        WordSlot& word_slot = m_word_slots[word_index];
        std::size_t word_size = word_slot.word_size;
        if (ARCHON_LIKELY(line_size >= cursor_pos)) {
            std::size_t rest = std::size_t(line_size - cursor_pos);
            if (ARCHON_LIKELY(rest >= word_size)) {
                rest -= word_size;
                std::size_t space_size = words[word_index].space_size;
                if (ARCHON_LIKELY(rest >= space_size)) {
                    rest -= space_size;
                    word_slot.cursor_pos = cursor_pos;
                    cursor_pos = std::size_t(line_size - rest);
                    goto move_right_2;
                }
            }
        }
    }
    // Could not move right

  move_down:
    // Try break onto new line
    {
        badness_type badness = 0;
        if (ARCHON_LIKELY(cursor_pos < line_size)) {
            std::size_t excess = std::size_t(line_size - cursor_pos);
            bool overflow = (!core::try_int_cast(excess, badness) ||
                             !core::try_int_mul(badness, badness));
            if (ARCHON_UNLIKELY(overflow))
                goto move_up;
        }

        if (ARCHON_UNLIKELY(badness > bound)) {
            Result& result = m_results[result_index];
            if (ARCHON_UNLIKELY(badness < result.badness)) {
                recycle_breakpoint(result);
                result = Result::indefinite(badness);
            }
            goto move_up;
        }
        LineSlot& line_slot = m_line_slots[line_index];
        line_slot.badness = badness;
        line_slot.result_index = result_index;
        ++line_index;
        m_line_slots[line_index].word_index = word_index;
        ARCHON_ASSERT(bound >= badness);
        bound -= badness;
        std::size_t geom_index = m_line_slots[line_index].geom_index;
        std::size_t cache_index = std::size_t(geom_index * num_words + word_index);
        ARCHON_ASSERT(cache_index < m_cache.size());
        CacheSlot& cache_slot = cache[cache_index];
        if (ARCHON_LIKELY(cache_slot.result_ident != 0)) {
            result_index = std::size_t(cache_slot.result_ident - 1);
            Result& result = m_results[result_index];
            ARCHON_ASSERT(result.badness != 0 || !result.is_indefinite());
            bool need_refresh = (result.is_indefinite() && result.badness <= bound);
            if (ARCHON_LIKELY(!need_refresh))
                goto move_up;
            result.badness = max;
        }
        else {
            result_index = m_results.size();
            m_results.push_back(Result::indefinite(max)); // Throws
            cache_slot.result_ident = std::size_t(result_index + 1);
        }
    }
    goto move_right_1;

  move_left:
    {
        ARCHON_ASSERT(word_index > 0);
        --word_index;
        std::size_t first_word_index = m_line_slots[line_index].word_index;
        if (ARCHON_LIKELY(word_index > first_word_index)) {
            ARCHON_ASSERT(word_index > 0);
            cursor_pos = m_word_slots[word_index].cursor_pos;
            goto move_down;
        }
    }

  move_up:
    // Return to previous line
    {
        if (ARCHON_UNLIKELY(line_index == 0))
            goto done;
        std::size_t first_word_index = m_line_slots[line_index].word_index;
        --line_index;
        const LineSlot& line_slot = m_line_slots[line_index];
        word_index = first_word_index;
        line_size  = line_slot.size;
        Result& result_from_below = m_results[result_index];
        result_index = line_slot.result_index;
        Result& result = m_results[result_index];
        badness_type badness = line_slot.badness;
        bound += badness;
        bool overflow = !core::try_int_add(badness, result_from_below.badness);
        bool adopt_result_from_below = (!overflow && (badness < result.badness));
        if (ARCHON_UNLIKELY(adopt_result_from_below)) {
            recycle_breakpoint(result);
            if (ARCHON_LIKELY(!result_from_below.is_indefinite())) {
                std::size_t breakpoint_index =
                    alloc_breakpoint(word_index, result_from_below.breakpoint_index); // Throws
                result = Result::definite(badness, breakpoint_index);
                if (ARCHON_UNLIKELY(badness <= bound)) {
                    if (ARCHON_UNLIKELY(badness == 0))
                        goto move_up;
                    bound = badness_type(badness - 1);
                }
            }
            else {
                result = Result::indefinite(badness);
                ARCHON_ASSERT(badness > bound);
            }
        }
    }
    goto move_left;

  done:
    // Gather breakpoints
    const Result& result = m_results[result_index];
    // FIXME: Consider using core::MulPrecInt to completely eliminate the risk of badness
    // overflow.                    
    if (ARCHON_UNLIKELY(result.is_indefinite()))
        throw std::overflow_error("Badness");
    std::size_t breakpoint_index = result.breakpoint_index;
    while (breakpoint_index != std::size_t(-1)) {
        const Breakpoint& breakpoint = m_breakpoints[breakpoint_index];
        breakpoints.push_back(breakpoint.word_index); // Throws
        breakpoint_index = breakpoint.next_breakpoint_index;
    }
    m_badness = result.badness;
}
