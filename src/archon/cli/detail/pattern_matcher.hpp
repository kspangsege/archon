// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef ARCHON_X_CLI_X_DETAIL_X_PATTERN_MATCHER_HPP
#define ARCHON_X_CLI_X_DETAIL_X_PATTERN_MATCHER_HPP

/// \file


#include <locale>

#include <archon/base/features.h>
#include <archon/base/format.hpp>
#include <archon/base/format_enc.hpp>
#include <archon/base/format_as.hpp>
#include <archon/cli/exception.hpp>
#include <archon/cli/detail/spec.hpp>
#include <archon/cli/detail/pattern_position_nfa.hpp>


namespace archon::cli::detail {


template<class C, class T> class PatternMatcher {
public:
    struct Config;

    using char_type   = C;
    using traits_type = T;
    using spec_type   = Spec<C, T>;

    using Pattern     = typename spec_type::Pattern;
    using Position    = typename PatternPositionNfa::Position;
    using PositionSet = typename PatternPositionNfa::PositionSet;

    PatternMatcher(const std::locale&, const spec_type&, const PatternPositionNfa&, const Config&);

    bool consume_keyword(std::size_t keyword_index);
    bool consume_option(std::size_t option_index);
    bool consume_value(const char* value);
    bool can_consume_anything_more() const noexcept;
    bool is_match(const Pattern*&) noexcept;

private:
    using FrozenPositionSetElems = std::vector<std::size_t>;

    // Refers to a range in m_frozen_position_set_elems
    using FrozenPositionSet = std::pair<std::size_t, std::size_t>;

    struct DfaState {
        FrozenPositionSet positions;
        std::optional<std::size_t> finalpos;         
        std::map<PatternSymbol, std::size_t> transitions;
    };

    struct HistoryEntry {
        std::size_t dfa_state_index;
        PatternSymbol symbol;
        const char* value;
    };

    const spec_type& m_spec;
    const PatternPositionNfa& m_nfa;

    FrozenPositionSetElems m_frozen_position_set_elems;
    std::vector<DfaState> m_dfa_states;
    std::size_t m_current_dfa_state_index = 0;
    std::vector<HistoryEntry> m_history;

    bool is_degenerate() const noexcept;
    bool consume(PatternSymbol, const char* value);
};


template<class C, class T> struct PatternMatcher<C, T>::Config {
    bool allow_cross_pattern_ambiguity    = false;
    bool allow_internal_pattern_ambiguity = false;
};








// Implementation


template<class C, class T>
PatternMatcher<C, T>::PatternMatcher(const std::locale& locale, const spec_type& spec,
                                     const PatternPositionNfa& nfa, const Config& config) :
    m_spec(spec),
    m_nfa(nfa)
{
    if (spec.get_num_patterns() == 0)
        return; // No patterns -> degenerate case

    // Construct DFA from position NFA (NFA over regular expression positions)
    auto error = [&](const char* message, const auto&... params) {
        std::string message_2 =
            base::format_enc<char_type>(locale, "Error in pattern specifications: %s",
                                        base::formatted(message, params...)); // Throws
        throw BadCommandLineInterfaceSpec(std::move(message_2));
    };
    class FrozenPositionSetCompare {
    public:
        FrozenPositionSetCompare(const FrozenPositionSetElems& elems) noexcept :
            m_elems(elems)
        {
        }
        bool operator()(FrozenPositionSet a, FrozenPositionSet b) const noexcept
        {
            auto a_begin = m_elems.begin() + a.first;
            auto a_end   = m_elems.begin() + a.second;
            auto b_begin = m_elems.begin() + b.first;
            auto b_end   = m_elems.begin() + b.second;
            return std::lexicographical_compare(a_begin, a_end, b_begin, b_end);
        }
    private:
        const FrozenPositionSetElems& m_elems;
    };
    FrozenPositionSetCompare compare(m_frozen_position_set_elems);
    // Values of `dfa_state_map` are indexes into `m_dfa_states`
    std::map<FrozenPositionSet, std::size_t, FrozenPositionSetCompare> dfa_state_map(compare);
    auto ensure_dfa_state = [&](const PositionSet& position_set) {
        std::size_t frozen_begin = m_frozen_position_set_elems.size();
        for (std::size_t pos : position_set)
            m_frozen_position_set_elems.push_back(pos); // Throws
        std::size_t frozen_end = m_frozen_position_set_elems.size();
        FrozenPositionSet frozen_position_set { frozen_begin, frozen_end };
        std::size_t dfa_state_index = m_dfa_states.size();
        auto p = dfa_state_map.emplace(frozen_position_set, dfa_state_index); // Throws
        auto i = p.first;
        bool was_inserted = p.second;
        if (!was_inserted) {
            m_frozen_position_set_elems.resize(frozen_begin);
            dfa_state_index = i->second;
        }
        else {
            std::optional<std::size_t> finalpos;      
            for (std::size_t i = frozen_begin; i < frozen_end; ++i) {
                std::size_t pos = m_frozen_position_set_elems[i];
                const Position& position = nfa.get_position(pos);
                bool position_is_final = position.followpos.empty();
                if (!position_is_final)
                    continue;
                if (!finalpos.has_value()) {
                    finalpos = pos;
                    continue;
                }
                const Position& prior_position = nfa.get_position(finalpos.value());
                std::size_t prior_pattern_index = prior_position.pattern_index;
                ARCHON_ASSERT(prior_pattern_index < position.pattern_index);
                if (config.allow_cross_pattern_ambiguity) {
                    // Resolve ambiguity by choosing the pattern that was specified
                    // first.
                    continue;
                }
                // FIXME: where is internal pattern ambiguity detected (and what is it?)                                                           
                error("Ambiguity between %s and %s pattern",
                      base::as_ordinal(1 + prior_pattern_index),
                      base::as_ordinal(1 + position.pattern_index)); // Throws
            }
            m_dfa_states.push_back({ frozen_position_set, finalpos, {} }); // Throws
        }
        return dfa_state_index;
    };
    ensure_dfa_state(nfa.get_start_positions()); // Throws
    std::size_t num_checked_dfa_states = 0;
    do {
        std::map<PatternSymbol, PositionSet> target_sets;
        {
            DfaState& dfa_state = m_dfa_states[num_checked_dfa_states];
            std::size_t begin = dfa_state.nfa_states.first;
            std::size_t end   = dfa_state.nfa_states.second;
            for (std::size_t i = begin; i < end; ++i) {
                std::size_t pos = m_frozen_position_set_elems[i];
                const Position& position = nfa.get_position(pos);
                if (!position.followpos.empty()) {
                    PatternSymbol symbol = position.symbol;
                    PositionSet& position_set = target_sets[symbol]; // Throws
                    for (std::size_t pos_2 : position.followpos) {
                        position_set.insert(pos_2); // Throws
                        // FIXME: Expand backtracking DFA                                                                                      
                    }
                }
            }
        }
        for (const auto& entry : target_sets) {
            PatternSymbol symbol = entry.first;
            const PositionSet& position_set = entry.second;
            ARCHON_ASSERT(!position_set.empty());
            std::size_t target_dfa_state_index = ensure_dfa_state(position_set); // Throws
            DfaState& dfa_state = m_dfa_states[num_checked_dfa_states];
            if (ARCHON_UNLIKELY(dfa_state.transitions.empty() && dfa_state.finalpos.has_value())) {
                std::size_t pos = dfa_state.finalpos.value();
                const Position& position = nfa.get_position(pos);
                const Pattern& pattern = spec.get_pattern(position.pattern_index);
                if (pattern.action->is_deleg) {
                    if (position_set.count(pos) != 0) {
                        error("A delegating pattern (%s pattern) is not allowed to match "
                              "anything that is a prefix of something else matched by that "
                              "pattern", base::as_ordinal(1 + position.pattern_index)); // Throws
                    }
                    std::size_t pos_2 = *position_set.begin();
                    const Position& position_2 = nfa.get_position(pos_2);
                    error("A delegating pattern (%s pattern) is not allowed to match anything "
                          "that is a prefix of something matched by any other pattern "
                          "(%s pattern)", base::as_ordinal(1 + position.pattern_index),
                          base::as_ordinal(1 + position_2.pattern_index)); // Throws
                }
            }
            dfa_state.transitions[symbol] = target_dfa_state_index; // Throws
        }
        ++num_checked_dfa_states;
    }
    while (num_checked_dfa_states < m_dfa_states.size());
}


} // namespace archon::cli::detail

#endif // ARCHON_X_CLI_X_DETAIL_X_PATTERN_MATCHER_HPP
