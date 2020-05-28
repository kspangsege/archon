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
#include <functional>
#include <memory>
#include <utility>
#include <vector>
#include <map>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/word_wrap.hpp>
#include <archon/core/test/word_wrap_knuth_alt.hpp>


using namespace archon;
namespace word_wrap = core::word_wrap;
namespace test = core::test;


auto test::word_wrap_knuth_alt(core::Span<const word_wrap::Word> words, std::size_t trailing_space_size,
                               core::Span<const word_wrap::Geometry> geometry,
                               std::vector<std::size_t>& breakpoints) -> word_wrap::KnuthWrapper::badness_type
{
    using badness_type = word_wrap::KnuthWrapper::badness_type;

    struct Result {
        badness_type badness;
        std::vector<std::size_t> breakpoints;
    };

    std::size_t num_words = words.size();

    std::function<std::shared_ptr<Result>(std::size_t word_index, std::size_t cursor_pos, std::size_t geom_index,
                                          std::size_t width, badness_type badness_bound)> add_word;

    auto do_add_first_word = [&](std::size_t word_index, std::size_t geom_index, badness_type badness_bound) {
        ARCHON_ASSERT(word_index < num_words);
        const word_wrap::Word& word = words[word_index];
        std::size_t cursor_pos = word.word_size;
        bool is_first_word = (word_index == 0);
        if (ARCHON_UNLIKELY(is_first_word))
            cursor_pos += word.space_size;
        ARCHON_ASSERT(geom_index < geometry.size());
        std::size_t width = geometry[geom_index].line_size;
        return add_word(word_index + 1, cursor_pos, geom_index, width, badness_bound); // Throws
    };

    using Key = std::pair<std::size_t, std::size_t>;
    struct Slot {
        std::shared_ptr<Result> result;
        badness_type badness_bound = 0;
    };
    std::map<Key, Slot> map;

    auto add_first_word = [&](std::size_t word_index, std::size_t geom_index,
                              badness_type badness_bound) -> std::shared_ptr<Result> {
        auto p = map.insert({ { word_index, geom_index }, {} }); // Throws
        Slot& slot = p.first->second;
        bool was_inserted = p.second;
        if (!was_inserted) {
            if (slot.result) {
                if (slot.result->badness < badness_bound)
                    return slot.result;
                return nullptr;
            }
            if (slot.badness_bound >= badness_bound)
                return nullptr;
        }
        auto result = do_add_first_word(word_index, geom_index, badness_bound); // Throws
        if (result) {
            slot.result = result;
        }
        else {
            slot.badness_bound = badness_bound;
        }
        return result;
    };

    auto break_line = [&](std::size_t word_index, std::size_t cursor_pos, std::size_t geom_index, std::size_t width,
                          badness_type badness_bound) -> std::shared_ptr<Result> {
        ARCHON_ASSERT(word_index < num_words);
        ARCHON_ASSERT(cursor_pos > 0);
        badness_type local_badness = 0;
        if (ARCHON_LIKELY(cursor_pos <= width)) {
            auto rest = badness_type(width - cursor_pos);
            local_badness = badness_type(rest * rest);
        }
        if (local_badness < badness_bound) {
            std::size_t geom_index_2 = geometry[geom_index].next_geometry_index;
            badness_type badness_bound_2 = badness_type(badness_bound - local_badness);
            auto result = add_first_word(word_index, geom_index_2, badness_bound_2); // Throws
            if (result) {
                auto badness = badness_type(local_badness + result->badness);
                auto breakpoints = result->breakpoints;
                breakpoints.insert(breakpoints.begin(), word_index); // Throws
                Result result_2 = { badness, std::move(breakpoints) };
                return std::make_shared<Result>(std::move(result_2)); // Throws
            }
        }
        return nullptr;
    };

    add_word = [&](std::size_t word_index, std::size_t cursor_pos, std::size_t geom_index, std::size_t width,
                   badness_type badness_bound) {
        ARCHON_ASSERT(cursor_pos > 0);
        ARCHON_ASSERT(badness_bound > 0);
        if (word_index < num_words) {
            const word_wrap::Word& word = words[word_index];
            std::size_t cursor_pos_2 = std::size_t(cursor_pos + word.space_size + word.word_size);
            bool is_last_word = (word_index == std::size_t(num_words - 1));
            if (ARCHON_UNLIKELY(is_last_word))
                cursor_pos_2 += trailing_space_size;
            if (cursor_pos_2 <= width) {
                auto result_1 = add_word(std::size_t(word_index + 1), cursor_pos_2, geom_index, width,
                                         badness_bound); // Throws
                if (result_1) {
                    badness_type badness_bound_2 = result_1->badness;
                    auto result_2 = break_line(word_index, cursor_pos, geom_index, width, badness_bound_2); // Throws
                    if (result_2)
                        return result_2;
                    return result_1;
                }
            }
            return break_line(word_index, cursor_pos, geom_index, width, badness_bound); // Throws
        }
        return std::make_shared<Result>(Result { 0, {} }); // Throws
    };

    if (num_words < 2) {
        breakpoints = {};
        return 0;
    }

    std::size_t word_index = 0;
    std::size_t geom_index = 0;
    badness_type badness_bound = std::numeric_limits<badness_type>::max();
    auto result = add_first_word(word_index, geom_index, badness_bound); // Throws
    breakpoints = result->breakpoints;
    return result->badness;
}
