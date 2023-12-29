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

#ifndef ARCHON_X_CLI_X_IMPL_X_PATTERN_MATCHER_HPP
#define ARCHON_X_CLI_X_IMPL_X_PATTERN_MATCHER_HPP


#include <cstddef>
#include <algorithm>
#include <utility>
#include <optional>
#include <string_view>
#include <string>
#include <vector>
#include <map>
#include <locale>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/format.hpp>
#include <archon/core/format_as.hpp>
#include <archon/core/format_enc.hpp>
#include <archon/cli/spec_error.hpp>
#include <archon/cli/exception.hpp>
#include <archon/cli/impl/pattern_structure.hpp>
#include <archon/cli/impl/pattern_args_parser.hpp>
#include <archon/cli/impl/spec.hpp>
#include <archon/cli/impl/nfa.hpp>
#include <archon/cli/impl/nfa_builder.hpp>


namespace archon::cli::impl {


struct PatternMatcherConfig {
    bool allow_cross_pattern_ambiguity               = false;
    bool allow_pattern_internal_positional_ambiguity = false;
};


template<class C, class T> class PatternMatcher {
public:
    using char_type   = C;
    using traits_type = T;

    struct Config;

    using string_view_type         = std::basic_string_view<C, T>;
    using pattern_structure_type   = impl::PatternStructure<C, T>;
    using pattern_args_parser_type = impl::PatternArgsParser<C, T>;
    using spec_type                = impl::Spec<C, T>;

    using Pattern     = typename spec_type::Pattern;
    using MatchPos    = typename pattern_args_parser_type::MatchPos;
    using Position    = typename Nfa::Position;
    using PositionSet = typename Nfa::PositionSet;

    PatternMatcher(const std::locale&, const spec_type&, const pattern_structure_type&, std::size_t num_args,
                   const PatternMatcherConfig&);

    bool consume_keyword(std::size_t keyword_index, std::size_t arg_index);
    bool consume_option(std::size_t proto_index, std::size_t arg_index);
    bool consume_value(std::size_t arg_index);
    bool can_delegate() const noexcept;
    bool can_consume() const noexcept;

    bool is_match(const Pattern*&) const noexcept;

    // Determine the pattern-internal positions with respect to the matched pattern
    // corresponding to the matching command-line arguments. On success, `match_positions`
    // will have one entry for each command-line argument that was matched with a pattern
    // symbol plus a final entry representing the end-position for the matched pattern. In
    // the final entry, `pos` will be equal to the number of symbols in the matched pattern,
    // and `arg_index` will be equal to the number of command-line arguments as it was
    // passed to the pattern matcher constructor.
    void backtrack(std::vector<MatchPos>& match_positions) const;

private:
    using FrozenPositionSetElems = std::vector<std::size_t>;

    // Refers to a range in m_frozen_position_set_elems
    using FrozenPositionSet = std::pair<std::size_t, std::size_t>;

    struct State {
        FrozenPositionSet positions;
        std::optional<std::size_t> finalpos;
        std::map<PatternSymbol, std::size_t> transitions;
    };

    struct HistoryEntry {
        std::size_t prior_state_index;
        PatternSymbol symbol;
        std::size_t arg_index;
    };

    const spec_type& m_spec;
    const std::size_t m_num_args;

    impl::Nfa m_nfa;
    FrozenPositionSetElems m_frozen_position_set_elems;
    std::vector<State> m_states;
    std::size_t m_state_index = 0;
    std::vector<HistoryEntry> m_history;

    bool consume(PatternSymbol, std::size_t arg_index);
};








// Implementation


template<class C, class T>
PatternMatcher<C, T>::PatternMatcher(const std::locale& locale, const spec_type& spec,
                                     const pattern_structure_type& pattern_structure, std::size_t num_args,
                                     const PatternMatcherConfig& config)
    : m_spec(spec)
    , m_num_args(num_args)
{
    ARCHON_ASSERT(m_spec.get_num_patterns() > 0);

    // Construct "NFA over positions" from patterns
    {
        impl::NfaBuilder builder(m_nfa, pattern_structure);
        std::size_t n = m_spec.get_num_patterns();
        for (std::size_t i = 0; i < n; ++i) {
            const Pattern& pattern = m_spec.get_pattern(i);
            builder.add_pattern(pattern.elem_seq_index, i); // Throws
        }
    }

    // Construct DFA from "NFA over positions"
    auto error = [&](cli::SpecError code, const char* message, const auto&... params) {
        std::string message_2 = core::format_enc<char_type>(locale, "Error in pattern specifications: %s",
                                                            core::formatted(message, params...)); // Throws
        throw cli::BadSpec(code, std::move(message_2));
    };
    class FrozenPositionSetCompare {
    public:
        FrozenPositionSetCompare(const FrozenPositionSetElems& elems) noexcept
            : m_elems(elems)
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
    // Values of `state_map` are indexes into `m_states`
    std::map<FrozenPositionSet, std::size_t, FrozenPositionSetCompare> state_map(compare);
    auto ensure_state = [&](const PositionSet& position_set) {
        std::size_t frozen_begin = m_frozen_position_set_elems.size();
        for (std::size_t pos : position_set)
            m_frozen_position_set_elems.push_back(pos); // Throws
        std::size_t frozen_end = m_frozen_position_set_elems.size();
        FrozenPositionSet frozen_position_set { frozen_begin, frozen_end };
        std::size_t state_index = m_states.size();
        auto p = state_map.emplace(frozen_position_set, state_index); // Throws
        auto i = p.first;
        bool was_inserted = p.second;
        if (!was_inserted) {
            m_frozen_position_set_elems.resize(frozen_begin);
            state_index = i->second;
        }
        else {
            std::optional<std::size_t> finalpos;
            for (std::size_t i = frozen_begin; i < frozen_end; ++i) {
                std::size_t pos = m_frozen_position_set_elems[i];
                const Position& position = m_nfa.get_position(pos);
                bool position_is_final = position.followpos.empty();
                if (!position_is_final)
                    continue;
                if (!finalpos.has_value()) {
                    finalpos = pos;
                    continue;
                }
                const Position& prior_position = m_nfa.get_position(finalpos.value());
                std::size_t prior_pattern_index = prior_position.pattern_index;
                ARCHON_ASSERT(prior_pattern_index < position.pattern_index);
                if (config.allow_cross_pattern_ambiguity) {
                    // Resolve ambiguity by choosing the pattern that was specified first.
                    continue;
                }
                error(cli::SpecError::cross_pattern_ambiguity,
                      "Ambiguity between %s and %s pattern",
                      core::as_ordinal(1 + prior_pattern_index),
                      core::as_ordinal(1 + position.pattern_index)); // Throws
            }
            m_states.push_back({ frozen_position_set, finalpos, {} }); // Throws
        }
        return state_index;
    };
    ensure_state(m_nfa.get_start_positions()); // Throws
    std::size_t num_finalized_states = 0;
    do {
        // Finalize the next unfinalized DFA state by filling in its outgoing
        // transitions. The next unfinalized state is always the one at index
        // `num_finalized_states`.
        std::map<PatternSymbol, PositionSet> target_sets;
        {
            State& state = m_states[num_finalized_states];
            ARCHON_ASSERT(state.transitions.empty()); // Not yet finalized
            std::size_t begin = state.positions.first;
            std::size_t end   = state.positions.second;
            for (std::size_t i = begin; i < end; ++i) {
                std::size_t pos = m_frozen_position_set_elems[i];
                const Position& position = m_nfa.get_position(pos);
                if (!position.followpos.empty()) {
                    PatternSymbol symbol = position.symbol;
                    PositionSet& position_set = target_sets[symbol]; // Throws
                    for (std::size_t pos_2 : position.followpos) {
                        auto p = position_set.insert(pos_2);
                        bool was_inserted = p.second;
                        if (was_inserted || config.allow_pattern_internal_positional_ambiguity)
                            continue;
                        //
                        // FIXME: Explain why we have detected positional ambiguity if we
                        // have reached this point.                
                        //
                        // Find origin position for conflicting transition
                        for (std::size_t j = begin; j < i; ++j) {
                            std::size_t pos_3 = m_frozen_position_set_elems[j];
                            const Position& position_2 = m_nfa.get_position(pos_3);
                            bool found = (position_2.symbol == symbol && position_2.followpos.count(pos_2) != 0);
                            if (!found)
                                continue;
                            error(cli::SpecError::positional_ambiguity,
                                  "Positional ambiguity in %s pattern (between %s and %s symbol)",
                                  core::as_ordinal(1 + position.pattern_index),
                                  core::as_ordinal(1 + pos_3),
                                  core::as_ordinal(1 + pos)); // Throws
                        }
                        ARCHON_ASSERT_UNREACHABLE();
                    }
                }
            }
        }
        for (const auto& entry : target_sets) {
            PatternSymbol symbol = entry.first;
            const PositionSet& position_set = entry.second;
            ARCHON_ASSERT(!position_set.empty());
            // New states introduced here will not be finalized immediately, which means
            // that they will not have their outgoing transitions filled in until later.
            std::size_t target_state_index = ensure_state(position_set); // Throws
            // The state whose finalization is in progress
            State& state = m_states[num_finalized_states];
            bool first_transition = state.transitions.empty();
            if (ARCHON_UNLIKELY(first_transition && state.finalpos.has_value())) {
                // This is a final state, and we are about to add its first outgoing
                // transition. This is illegal for a delegating pattern, as it would cause
                // ambiguity in terms of where on the command line to hand over the
                // interpretation of arguments to the subordinate command-line processor, or
                // even whether to hand over at all, because further arguments may lead to a
                // match against a different pattern.
                std::size_t pos = state.finalpos.value();
                const Position& position = m_nfa.get_position(pos);
                const Pattern& pattern = m_spec.get_pattern(position.pattern_index);
                if (pattern.action.is_deleg) {
                    // This is a final state for a delegating pattern.
                    error(cli::SpecError::prefix_deleg_pattern,
                          "Delegating pattern (%s pattern) has match that is a proper prefix of "
                          "another match from same or different pattern.",
                          core::as_ordinal(1 + position.pattern_index)); // Throws
                }
            }
            state.transitions[symbol] = target_state_index; // Throws
        }
        ++num_finalized_states;
    }
    while (num_finalized_states < m_states.size());
}


template<class C, class T>
inline bool PatternMatcher<C, T>::consume_keyword(std::size_t keyword_index, std::size_t arg_index)
{
    return consume(PatternSymbol::keyword(keyword_index), arg_index); // Throws
}


template<class C, class T>
inline bool PatternMatcher<C, T>::consume_option(std::size_t proto_index, std::size_t arg_index)
{
    return consume(PatternSymbol::proto_option(proto_index), arg_index); // Throws
}


template<class C, class T>
inline bool PatternMatcher<C, T>::consume_value(std::size_t arg_index)
{
    return consume(PatternSymbol::value_slot(), arg_index); // Throws
}


template<class C, class T>
bool PatternMatcher<C, T>::can_delegate() const noexcept
{
    ARCHON_ASSERT(m_state_index < m_states.size());
    const State& state = m_states[m_state_index];
    std::size_t begin = state.positions.first;
    std::size_t end   = state.positions.second;
    for (std::size_t i = begin; i < end; ++i) {
        std::size_t pos = m_frozen_position_set_elems[i];
        const Position& position = m_nfa.get_position(pos);
        const Pattern& pattern = m_spec.get_pattern(position.pattern_index);
        if (pattern.action.is_deleg)
            return true;
    }
    return false;
}


template<class C, class T>
inline bool PatternMatcher<C, T>::can_consume() const noexcept
{
    ARCHON_ASSERT(m_state_index < m_states.size());
    const State& state = m_states[m_state_index];
    return !state.transitions.empty();
}


template<class C, class T>
bool PatternMatcher<C, T>::is_match(const Pattern*& pattern) const noexcept
{
    ARCHON_ASSERT(m_state_index < m_states.size());
    const State& state = m_states[m_state_index];
    if (ARCHON_LIKELY(!state.finalpos.has_value()))
        return false;
    const Position& position = m_nfa.get_position(state.finalpos.value());
    pattern = &m_spec.get_pattern(position.pattern_index);
    return true;
}


template<class C, class T>
void PatternMatcher<C, T>::backtrack(std::vector<MatchPos>& match_positions) const
{
    match_positions.clear();
    std::size_t state_index = m_state_index;
    ARCHON_ASSERT(state_index < m_states.size());
    const State& final_state = m_states[state_index];
    ARCHON_ASSERT(final_state.finalpos.has_value());
    std::size_t pos = *final_state.finalpos;
    std::size_t arg_index = m_num_args;
    std::size_t i = m_history.size();
    core::int_add(i, 1); // Throws
    match_positions.resize(i); // Throws
    --i;
  again:
    match_positions[i] = { m_nfa.get_position(pos).pattern_internal_pos, arg_index };
    if (ARCHON_LIKELY(i > 0)) {
        --i;
        const HistoryEntry& entry = m_history[i];
        std::size_t state_index_2 = entry.prior_state_index;
        ARCHON_ASSERT(state_index_2 < m_states.size());
        const State& prior_state = m_states[state_index_2];
        std::optional<std::size_t> prior_pos;
        std::size_t begin = prior_state.positions.first;
        std::size_t end   = prior_state.positions.second;
        for (std::size_t j = begin; j < end; ++j) {
            std::size_t pos_2 = m_frozen_position_set_elems[j];
            const Position& position = m_nfa.get_position(pos_2);
            if (position.symbol != entry.symbol)
                continue;
            if (position.followpos.count(pos) == 0)
                continue;
            // In case of pattern-internal positional ambiguity, we resolve it by choosing
            // the "left-most path" through the pattern.                        
            prior_pos = pos_2;
            break;
        }
        // The following assertion is sound, because for a given DFA edge, a position, P, is
        // in the target DFA state precisely when there is an edge in the NFA from a
        // position in the origin DFA state to P.
        ARCHON_ASSERT(prior_pos.has_value());
        state_index = state_index_2;
        pos = *prior_pos;
        arg_index = entry.arg_index;
        goto again;
    }
    ARCHON_ASSERT(state_index == 0);
}


template<class C, class T>
bool PatternMatcher<C, T>::consume(PatternSymbol symbol, std::size_t arg_index)
{
    ARCHON_ASSERT(arg_index < m_num_args);
    ARCHON_ASSERT(m_state_index < m_states.size());
    const State& state = m_states[m_state_index];
    auto i = state.transitions.find(symbol);
    if (ARCHON_LIKELY(i != state.transitions.end())) {
        std::size_t new_state_index = i->second;
        m_history.push_back({ m_state_index, symbol, arg_index });
        m_state_index = new_state_index;
        return true;
    }
    return false;
}


} // namespace archon::cli::impl

#endif // ARCHON_X_CLI_X_IMPL_X_PATTERN_MATCHER_HPP
