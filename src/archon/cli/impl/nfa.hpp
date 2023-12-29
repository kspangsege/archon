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

#ifndef ARCHON_X_CLI_X_IMPL_X_NFA_HPP
#define ARCHON_X_CLI_X_IMPL_X_NFA_HPP


#include <cstddef>
#include <vector>
#include <set>

#include <archon/core/assert.hpp>
#include <archon/cli/impl/pattern_symbol.hpp>


namespace archon::cli::impl {


// NFA over pattern positions.
//
// Each position entry corresponds to a position of an option, a keyword, or a value slot in
// a some pattern (regular expression).
//
// The positions of this specialised NFA correspond to states of a regular NFA, and given a
// particular position, P, associated with input symbol, S; a particular entry, Q, in
// followpos of P corresponds to an edge from P to Q on S.
//
// This specialised type of NFA does not have any epsilon edges, and all edges originating
// from a particular position carry the same input symbol, which is the input symbol that
// the origin position is associated with.
//
class Nfa {
public:
    struct Position;

    // FIXME: Consider finding a way to not use std::set here, i.e., use a more memory
    // compact representation of position sets instead. Consider this in the context of how
    // the NFA is actually built.                           
    using PositionSet = std::set<std::size_t>;

    auto create_position(std::size_t pattern_index, std::size_t pattern_internal_pos,
                         impl::PatternSymbol symbol) -> std::size_t;
    void register_startpos(std::size_t pos);
    void register_followpos(std::size_t pos_1, std::size_t pos_2);

    auto get_start_positions() const noexcept -> const PositionSet&;
    auto get_position(std::size_t pos) const noexcept -> const Position&;

private:
    std::vector<Position> m_positions;
    PositionSet m_start_positions;
};


// This is a final position (state) when, and only when `followpos` is empty. In a final
// position, the value of `symbol` has no meaning.
//
struct Nfa::Position {
    // Index within spec (impl::Spec) of originating pattern.
    std::size_t pattern_index;

    // Position within pattern. First symbol in pattern is position zero.
    std::size_t pattern_internal_pos;

    // Pattern symbol associated with the edges originating from this position.
    impl::PatternSymbol symbol;

    // Set of target positions to which an edge leads from this position.
    PositionSet followpos;
};








// Implementation


inline auto Nfa::create_position(std::size_t pattern_index, std::size_t pattern_internal_pos,
                                 impl::PatternSymbol symbol) -> std::size_t
{
    std::size_t pos = m_positions.size();
    m_positions.push_back({ pattern_index, pattern_internal_pos, symbol, {} }); // Throws
    return pos;
}


inline void Nfa::register_startpos(std::size_t pos)
{
    m_start_positions.insert(pos); // Throws
}


inline void Nfa::register_followpos(std::size_t pos_1, std::size_t pos_2)
{
    ARCHON_ASSERT(pos_1 < m_positions.size());
    Position& position = m_positions[pos_1];
    position.followpos.insert(pos_2); // Throws
}


inline auto Nfa::get_start_positions() const noexcept -> const PositionSet&
{
    return m_start_positions;
}


inline auto Nfa::get_position(std::size_t pos) const noexcept -> const Position&
{
    return m_positions[pos];
}


} // namespace archon::cli::impl

#endif // ARCHON_X_CLI_X_IMPL_X_NFA_HPP
