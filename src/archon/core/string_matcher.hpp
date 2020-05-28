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

#ifndef ARCHON_X_CORE_X_STRING_MATCHER_HPP
#define ARCHON_X_CORE_X_STRING_MATCHER_HPP

/// \file


#include <cstddef>
#include <type_traits>
#include <limits>
#include <utility>
#include <algorithm>
#include <string_view>
#include <stdexcept>
#include <string>
#include <vector>
#include <set>
#include <map>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/memory.hpp>
#include <archon/core/circular_buffer.hpp>
#include <archon/core/range_map.hpp>
#include <archon/core/frozen_sets.hpp>
#include <archon/core/char_mapper.hpp>
#include <archon/core/format_as.hpp>
#include <archon/core/format_enc.hpp>


namespace archon::core {


/// \brief String matcher base class.
///
/// This is the base class shared by all instantiations of the \ref core::BasicStringMatcher
/// class template.
///
class StringMatcherBase {
public:
    enum class PatternType;
};



/// \brief Available pattern syntaxes.
///
/// These are the available syntaxes in which specified patterns can be expressed.
///
enum class StringMatcherBase::PatternType {
    /// \brief Wildcard pattern syntax.
    ///
    /// With this pattern syntax, all characters stand for themselves, except star (`*`)
    /// which is the wildcard, and which stands for an arbitrary sequence of characters, and
    /// this includes the empty sequence. A pattern may contain any number of wildcards,
    /// including zero.
    ///
    wildcard
};




template<class C, class T = std::char_traits<C>> class BasicStringMatcher
    : public StringMatcherBase {
public:
    class Builder;

    using string_view_type = std::basic_string_view<C, T>;

    BasicStringMatcher() noexcept = default;

    BasicStringMatcher(PatternType, const char* pattern, const std::locale& = {});
    BasicStringMatcher(PatternType, string_view_type pattern, const std::locale& = {});

    bool match(string_view_type string) const noexcept;
    bool match(string_view_type string, std::size_t& pattern_ident) const noexcept;

    bool is_degenerate() const noexcept;

private:
    struct Pattern;
    struct State;
    struct Transition;

    core::Slab<Pattern> m_patterns;
    core::Slab<State> m_states;
    core::Slab<Transition> m_transitions;
};


using StringMatcher     = BasicStringMatcher<char>;
using WideStringMatcher = BasicStringMatcher<wchar_t>;




template<class C, class T> class BasicStringMatcher<C, T>::Builder
    : private BasicCharMapper<C, T>
    , private BasicCharMapper<C, T>::WidenBuffer {
public:
    struct Config;

    Builder();
    explicit Builder(Config);

    void add_pattern(PatternType, const char* pattern, std::size_t ident = 0);
    void add_pattern(PatternType, string_view_type pattern, std::size_t ident = 0);

    auto build() -> BasicStringMatcher;

    ///
    /// By passing a preexisting matcher object, you allow for memory already owned by that
    /// matcher object to be reused.
    ///
    void build(BasicStringMatcher&);

    void clear() noexcept;

    bool empty() const noexcept;

private:
    struct Token {
        enum class Type { segment, wildcard, end_of_input };
        Type type;
        string_view_type lexeme;
    };

    struct Node;
    struct Symbol;
    struct PositionSlot;
    struct Result;

    using PositionSet = typename core::FrozenSets<std::size_t>::Ident;

    // Patterns are integrated into the builder by expanding an "NFA over positions"
    // (m_positions, m_followpos_sets, m_start_positions).
    //
    // Each position entry corresponds to a position of an input symbol in a some pattern
    // (regular expression).
    //
    // The positions of this specialized NFA correspond to states of a regular NFA, and
    // given a particular position, P, associated with input symbol, S; a particular entry,
    // Q, in followpos of P corresponds to an edge from P to Q on S.
    //
    // This specialized type of NFA does not have any epsilon edges, and all edges
    // originating from a particular position carry the same input symbol, which is the
    // input symbol that the origin position is associated with.

    const bool m_allow_interpattern_ambiguity;
    std::vector<Pattern> m_patterns;
    const C m_asterisk;
    const C* m_curr;
    const C* m_end;
    Token m_next_token;
    std::vector<Node> m_nodes;
    std::vector<PositionSlot> m_positions;
    std::vector<std::set<std::size_t>> m_followpos_sets;
    std::set<std::size_t> m_start_positions;
    std::vector<State> m_states;
    std::vector<Transition> m_transitions;
    core::CircularBuffer<PositionSet> m_unchecked_position_sets;
    core::FrozenSets<std::size_t> m_position_sets_1;
    core::FrozenSets<std::size_t> m_position_sets_2;

    void do_add_pattern(PatternType, string_view_type pattern, std::size_t pattern_index);
    bool reset_parser(string_view_type string) noexcept;
    auto parse() -> std::size_t;
    auto parse_concatenation() -> std::size_t;
    auto parse_element() -> std::size_t;
    void extract_next_token() noexcept;
    auto add_node(std::size_t node_index, std::size_t pattern_index) -> Result;
    auto register_position(Symbol symbol, std::size_t pattern_index) -> std::size_t;
    void register_followpos(std::size_t pos_1, std::size_t pos_2);
    void register_followpos(PositionSet set, std::size_t pos);
    void register_followpos(PositionSet set_1, PositionSet set_2);
    void build_dfa();
    template<class... P> void error(const char* message, const P&... params);
};




template<class C, class T>
struct BasicStringMatcher<C, T>::Builder::Config {
    /// \brief Locale to be used by builder.
    ///
    /// This is the locale to be used by the builder.
    ///
    std::locale locale;

    /// \brief Allow for some strings to match multiple patterns.
    ///
    /// If set to `true`, ambiguity between patterns is allowed. Whenever a string matches
    /// more than one pattern, the match is reported for the pattern that was added first
    /// (\ref Builder::add_pattern()).
    ///
    bool allow_interpattern_ambiguity = false;
};








// Implementation


template<class C, class T>
struct BasicStringMatcher<C, T>::Pattern {
    std::size_t pattern_ident;
};


template<class C, class T>
struct BasicStringMatcher<C, T>::State {
    std::size_t pattern_index; // std::size_t(-1) if not final
    std::size_t transitions_begin, transitions_end;
};


template<class C, class T>
struct BasicStringMatcher<C, T>::Transition {
    C range_first, range_last;
    std::size_t target_state_index;
};


template<class C, class T>
struct BasicStringMatcher<C, T>::Builder::Node {
    enum class Type { segment, wildcard, concatenation };
    Type type;
    std::size_t left = 0, right = 0;
    string_view_type lexeme;
};


template<class C, class T>
struct BasicStringMatcher<C, T>::Builder::Symbol {
    enum class Type : char {
        character,
        wildcard
    };
    struct Value {
        C ch;
    };
    Type type;
    Value value;
    static auto character(C value) noexcept -> Symbol
    {
        return { Type::character, { value } };
    }
    static auto wildcard() noexcept -> Symbol
    {
        return { Type::wildcard, {} };
    }
};


// If `single_followpos` is false, then `followpos` refers to a set of "follow positions" as
// an index into `Builder::m_followpos_sets`.
//
// If `single_followpos` is true, then `followpos` refers directly to a single follow
// position as an index into `Builder::m_positions`.
//
// In any case, if the set of "follow positions" is empty after completion of the "NFA over
// positions", then, and only then is this a final position.
//
template<class C, class T>
struct BasicStringMatcher<C, T>::Builder::PositionSlot {
    std::size_t pattern_index;
    bool single_followpos;
    typename Symbol::Type symbol_type;
    typename Symbol::Value symbol_value;
    std::size_t followpos;
};


template<class C, class T>
struct BasicStringMatcher<C, T>::Builder::Result {
    bool nullable;
    PositionSet firstpos, lastpos;
};


template<class C, class T>
inline BasicStringMatcher<C, T>::BasicStringMatcher(PatternType type, const char* pattern, const std::locale& locale)
{
    typename Builder::Config config;
    config.locale = locale;
    Builder builder(std::move(config)); // Throws
    builder.add_pattern(type, pattern); // Throws
    builder.build(*this); // Throws
}


template<class C, class T>
inline BasicStringMatcher<C, T>::BasicStringMatcher(PatternType type, string_view_type pattern,
                                                    const std::locale& locale)
{
    typename Builder::Config config;
    config.locale = locale;
    Builder builder(std::move(config)); // Throws
    builder.add_pattern(type, pattern); // Throws
    builder.build(*this); // Throws
}


template<class C, class T>
inline bool BasicStringMatcher<C, T>::match(string_view_type string) const noexcept
{
    std::size_t pattern_ident; // Dummy
    return match(string, pattern_ident);
}


template<class C, class T>
bool BasicStringMatcher<C, T>::match(string_view_type string, std::size_t& pattern_ident) const noexcept
{
    bool degenerate = m_patterns.empty();
    if (ARCHON_LIKELY(!degenerate)) {
        ARCHON_ASSERT(m_states.size() > 0);
        const State* states = m_states.data();
        const Transition* transitions = m_transitions.data();
        const State* state = states + 0;
        for (C ch : string) {
            const Transition* begin = transitions + state->transitions_begin;
            const Transition* end   = transitions + state->transitions_end;
            auto cmp = [](const Transition& transition, C ch) {
                return transition.range_last < ch;
            };
            auto i = std::lower_bound(begin, end, ch, cmp);
            if (ARCHON_LIKELY(i != end && ch >= i->range_first)) {
                state = states + i->target_state_index;
                continue;
            }
            return false;
        }
        if (ARCHON_LIKELY(state->pattern_index != std::size_t(-1))) {
            const Pattern& pattern = m_patterns[state->pattern_index];
            pattern_ident = pattern.pattern_ident;
            return true;
        }
    }
    return false;
}


template<class C, class T>
inline bool BasicStringMatcher<C, T>::is_degenerate() const noexcept
{
    return m_patterns.empty();
}


template<class C, class T>
inline BasicStringMatcher<C, T>::Builder::Builder()
    : Builder({}) // Throws
{
}


template<class C, class T>
BasicStringMatcher<C, T>::Builder::Builder(Config config)
    : BasicCharMapper<C, T>(config.locale) // Throws
    , m_allow_interpattern_ambiguity(config.allow_interpattern_ambiguity)
    , m_asterisk(this->widen('*')) // Throws
{
    // First set must always be empty
    m_followpos_sets.resize(1); // Throws
}


template<class C, class T>
inline void BasicStringMatcher<C, T>::Builder::add_pattern(PatternType type, const char* pattern, std::size_t ident)
{
    string_view_type pattern_2 = this->widen(pattern, *this); // Throws
    add_pattern(type, pattern_2, ident); // throws
}


template<class C, class T>
inline void BasicStringMatcher<C, T>::Builder::add_pattern(PatternType type, string_view_type pattern,
                                                           std::size_t ident)
{
    std::size_t pattern_index = m_patterns.size();
    m_patterns.push_back({ ident }); // Throws
    do_add_pattern(type, pattern, pattern_index); // Throws
}


template<class C, class T>
inline auto BasicStringMatcher<C, T>::Builder::build() -> BasicStringMatcher
{
    BasicStringMatcher matcher;
    build(matcher); // Throws
    return matcher;
}


template<class C, class T>
void BasicStringMatcher<C, T>::Builder::build(BasicStringMatcher& matcher)
{
    // Construct DFA from "positions NFA"
    build_dfa(); // throws

    // Copy DFA into matcher object
    core::Slab patterns { core::Span(m_patterns) }; // Throws
    core::Slab states { core::Span(m_states) }; // Throws
    core::Slab transitions { core::Span(m_transitions) }; // Throws
    matcher.m_patterns = std::move(patterns);
    matcher.m_states = std::move(states);
    matcher.m_transitions = std::move(transitions);
}


template<class C, class T>
void BasicStringMatcher<C, T>::Builder::clear() noexcept
{
    m_patterns.clear();
    m_positions.clear();
    m_followpos_sets.resize(1); // First set must always be empty
    m_start_positions.clear();
}


template<class C, class T>
inline bool BasicStringMatcher<C, T>::Builder::empty() const noexcept
{
    return m_patterns.empty();
}


template<class C, class T>
void BasicStringMatcher<C, T>::Builder::do_add_pattern(PatternType, string_view_type pattern,
                                                       std::size_t pattern_index)
{
    Symbol term_symbol = {}; // The value has no meaning in this case (final state)
    if (reset_parser(pattern)) {
        std::size_t node_index = parse(); // Throws
        m_position_sets_1.clear();
        Result result = add_node(node_index, pattern_index); // Throws
        PositionSet firstpos = result.firstpos;
        PositionSet lastpos  = result.lastpos;
        for (std::size_t start_pos : m_position_sets_1[firstpos])
            m_start_positions.insert(start_pos); // Throws
        std::size_t term_pos = register_position(term_symbol, pattern_index); // Throws
        if (result.nullable)
            m_start_positions.insert(term_pos); // Throws
        register_followpos(lastpos, term_pos); // Throws
    }
    else {
        std::size_t term_pos = register_position(term_symbol, pattern_index); // Throws
        m_start_positions.insert(term_pos); // Throws
    }
}


template<class C, class T>
bool BasicStringMatcher<C, T>::Builder::reset_parser(string_view_type string) noexcept
{
    m_curr = string.data();
    m_end = m_curr + string.size();
    m_nodes.clear();
    extract_next_token();
    return (m_next_token.type != Token::Type::end_of_input);
}


template<class C, class T>
inline auto BasicStringMatcher<C, T>::Builder::parse() -> std::size_t
{
    std::size_t node_index = parse_concatenation(); // Throws
    ARCHON_ASSERT(m_next_token.type == Token::Type::end_of_input);
    return node_index;
}


template<class C, class T>
auto BasicStringMatcher<C, T>::Builder::parse_concatenation() -> std::size_t
{
    std::size_t node_index = parse_element(); // Throws
    for (;;) {
        bool is_concat = (m_next_token.type != Token::Type::end_of_input);
        if (!is_concat)
            break;
        Node node;
        node.type = Node::Type::concatenation;
        node.left = node_index;
        node.right = parse_element(); // Throws
        node_index = m_nodes.size();
        m_nodes.push_back(node); // Throws
    }
    return node_index;
}


template<class C, class T>
auto BasicStringMatcher<C, T>::Builder::parse_element() -> std::size_t
{
    typename Node::Type node_type = Node::Type::segment;
    switch (m_next_token.type) {
        case Token::Type::segment:
            goto token_node;
        case Token::Type::wildcard:
            node_type = Node::Type::wildcard;
            goto token_node;
        case Token::Type::end_of_input:
            break;
    }
    ARCHON_ASSERT(false);

  token_node:
    Node node;
    node.type = node_type;
    node.lexeme = m_next_token.lexeme;
    extract_next_token(); // Throws
    std::size_t node_index = m_nodes.size();
    m_nodes.push_back(node); // Throws
    return node_index;
}


template<class C, class T>
void BasicStringMatcher<C, T>::Builder::extract_next_token() noexcept
{
    const C* i = m_curr;
    Token token;
    if (i == m_end) {
        token.type = Token::Type::end_of_input;
        goto good;
    }
    if (*i == m_asterisk) {
        ++i;
        token.type = Token::Type::wildcard;
        goto good;
    }
    do ++i;
    while (i != m_end && *i != m_asterisk);
    token.type = Token::Type::segment;
  good:
    token.lexeme = {m_curr, std::size_t(i - m_curr)};
    m_curr = i;
    m_next_token = token;
}


template<class C, class T>
auto BasicStringMatcher<C, T>::Builder::add_node(std::size_t node_index, std::size_t pattern_index) -> Result
{
    const Node& node = m_nodes[node_index];
    switch (node.type) {
        case Node::Type::segment:
            goto segment;
        case Node::Type::wildcard:
            goto wildcard;
        case Node::Type::concatenation:
            goto concatenation;
    }
    ARCHON_ASSERT(false);
    return {};

  segment:
    {
        ARCHON_ASSERT(!node.lexeme.empty());
        std::size_t pos = register_position(Symbol::character(node.lexeme[0]), pattern_index); // Throws
        Result result;
        result.nullable = false;
        result.firstpos = m_position_sets_1.freeze(pos); // Throws
        std::size_t i = 1, n = node.lexeme.size();
        if (i != n) {
            do {
                std::size_t pos_2 = register_position(Symbol::character(node.lexeme[i]), pattern_index); // Throws
                register_followpos(pos, pos_2); // Throws
                pos = pos_2;
                ++i;
            }
            while (i != n);
            result.lastpos = m_position_sets_1.freeze(pos); // Throws
        }
        else {
            result.lastpos = result.firstpos;
        }
        return result;
    }

  wildcard:
    {
        std::size_t pos = register_position(Symbol::wildcard(), pattern_index); // Throws
        register_followpos(pos, pos); // Throws
        Result result;
        result.nullable = true;
        result.firstpos = m_position_sets_1.freeze(pos); // Throws
        result.lastpos  = result.firstpos;
        return result;
    }

  concatenation:
    {
        Result result_1 = add_node(node.left,  pattern_index); // Throws
        Result result_2 = add_node(node.right, pattern_index); // Throws
        register_followpos(result_1.lastpos, result_2.firstpos); // Throws
        Result result;
        result.nullable = (result_1.nullable && result_2.nullable);
        if (!result_1.nullable) {
            result.firstpos = result_1.firstpos;
        }
        else {
            result.firstpos = m_position_sets_1.unite(result_1.firstpos, result_2.firstpos); // Throws
        }
        if (!result_2.nullable) {
            result.lastpos = result_2.lastpos;
        }
        else {
            result.lastpos = m_position_sets_1.unite(result_1.lastpos, result_2.lastpos); // Throws
        }
        return result;
    }
}


template<class C, class T>
auto BasicStringMatcher<C, T>::Builder::register_position(Symbol symbol, std::size_t pattern_index) -> std::size_t
{
    std::size_t pos = m_positions.size();
    m_positions.push_back({}); // Throws
    PositionSlot& slot = m_positions.back();
    slot.pattern_index = pattern_index;
    slot.symbol_type   = symbol.type;
    slot.symbol_value  = symbol.value;
    return pos;
}


template<class C, class T>
inline void BasicStringMatcher<C, T>::Builder::register_followpos(std::size_t pos_1, std::size_t pos_2)
{
    PositionSlot& slot = m_positions[pos_1];
    if (!slot.single_followpos) {
        // `slot.followpos` is index into m_followpos_sets;
        if (slot.followpos == 0) {
            slot.single_followpos = true;
            slot.followpos = pos_2;
            return;
        }
        std::set<std::size_t>& set = m_followpos_sets[slot.followpos];
        set.insert(pos_2); // Throws
        return;
    }
    // `slot.followpos` is index into m_positions;
    std::size_t index = m_followpos_sets.size();
    m_followpos_sets.push_back({ slot.followpos, pos_2 }); // Throws
    slot.single_followpos = false;
    slot.followpos = index;
}


template<class C, class T>
void BasicStringMatcher<C, T>::Builder::register_followpos(PositionSet set, std::size_t pos)
{
    for (std::size_t pos_1 : m_position_sets_1[set]) {
        std::size_t pos_2 = pos;
        register_followpos(pos_1, pos_2); // Throws
    }
}


template<class C, class T>
void BasicStringMatcher<C, T>::Builder::register_followpos(PositionSet set_1, PositionSet set_2)
{
    for (std::size_t pos_1 : m_position_sets_1[set_1]) {
        for (std::size_t pos_2 : m_position_sets_1[set_2])
            register_followpos(pos_1, pos_2); // Throws
    }
}


template<class C, class T>
void BasicStringMatcher<C, T>::Builder::build_dfa()
{
    m_states.clear();
    m_transitions.clear();
    m_unchecked_position_sets.clear();
    m_position_sets_2.clear();
    auto compare = [&](PositionSet a, PositionSet b) noexcept {
        core::Span<const std::size_t> a_2 = m_position_sets_2[a];
        core::Span<const std::size_t> b_2 = m_position_sets_2[b];
        return std::lexicographical_compare(a_2.begin(), a_2.end(), b_2.begin(), b_2.end());
    };
    std::map<PositionSet, std::size_t, decltype(compare)> state_map(compare); // Throws
    auto ensure_state = [&](const PositionSet& position_set_1) {
        core::Span<const std::size_t> span = m_position_sets_1[position_set_1];
        PositionSet position_set_2 = m_position_sets_2.freeze_ordered(span.begin(), span.end()); // Throws
        std::size_t state_index = state_map.size();
        auto p = state_map.emplace(position_set_2, state_index); // Throws
        auto i = p.first;
        bool was_inserted = p.second;
        if (!was_inserted) {
            m_position_sets_2.discard_from(position_set_2);
            state_index = i->second;
        }
        else {
            m_unchecked_position_sets.push_back(position_set_2); // Throws
        }
        return state_index;
    };
    {
        m_position_sets_1.clear();
        PositionSet position_set_1 = m_position_sets_1.freeze(m_start_positions); // Throws
        ensure_state(position_set_1); // Throws
    }
    do {
        m_position_sets_1.clear();
        RangeMap<C, PositionSet> target_sets;
        PositionSet position_set_2 = m_unchecked_position_sets.front();
        std::size_t pattern_index = std::size_t(-1);
        for (std::size_t pos : m_position_sets_2[position_set_2]) {
            const PositionSlot& slot = m_positions[pos];
            bool is_final_pos = (!slot.single_followpos && slot.followpos == 0);
            if (!is_final_pos) {
                auto func = [&](PositionSet& position_set_1) {
                    if (slot.single_followpos) {
                        position_set_1 = m_position_sets_1.unite(position_set_1, slot.followpos); // Throws
                        return;
                    }
                    const std::set<std::size_t>& set = m_followpos_sets[slot.followpos];
                    position_set_1 = m_position_sets_1.unite(position_set_1, set); // Throws
                };
                switch (slot.symbol_type) {
                    case Symbol::Type::character:
                        target_sets.update({ slot.symbol_value.ch, slot.symbol_value.ch }, func); // Throws
                        break;
                    case Symbol::Type::wildcard:
                        using lim = std::numeric_limits<C>;
                        target_sets.update({ lim::min(), lim::max() }, func); // Throws
                        break;
                }
                continue;
            }
            if (pattern_index == std::size_t(-1)) {
                pattern_index = slot.pattern_index;
                continue;
            }
            ARCHON_ASSERT(pattern_index < slot.pattern_index);
            if (m_allow_interpattern_ambiguity) {
                // Resolve ambiguity by choosing the pattern that was specified first.
                continue;
            }
            error("Ambiguity between %s and %s pattern",
                  core::as_ordinal(1 + pattern_index),
                  core::as_ordinal(1 + slot.pattern_index)); // Throws
        }
        auto equal = [&](const PositionSet& a, const PositionSet& b) noexcept {
            core::Span<const std::size_t> a_2 = m_position_sets_1[a];
            core::Span<const std::size_t> b_2 = m_position_sets_1[b];
            return std::equal(a_2.begin(), a_2.end(), b_2.begin(), b_2.end());
        };
        target_sets.defrag(equal); // Throws
        std::size_t transitions_begin = m_transitions.size();
        for (auto entry : target_sets) {
            auto range = entry.range();
            const PositionSet& position_set = entry.value();
            ARCHON_ASSERT(!position_set.empty());
            std::size_t target_state_index = ensure_state(position_set); // Throws
            m_transitions.push_back({ range.first, range.last, target_state_index }); // Throws
        }
        std::size_t transitions_end = m_transitions.size();
        m_states.push_back({ pattern_index, transitions_begin, transitions_end }); // Throws
        m_unchecked_position_sets.pop_front();
    }
    while (!m_unchecked_position_sets.empty());
}


template<class C, class T>
template<class... P> void BasicStringMatcher<C, T>::Builder::error(const char* message, const P&... params)
{
    throw std::runtime_error(core::format_enc<C>(message, params...));
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_STRING_MATCHER_HPP
