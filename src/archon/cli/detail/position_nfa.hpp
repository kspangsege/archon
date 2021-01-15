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

#ifndef ARCHON_X_CLI_X_DETAIL_X_POSITION_NFA_HPP
#define ARCHON_X_CLI_X_DETAIL_X_POSITION_NFA_HPP

/// \file


#include <cstddef>
#include <vector>
#include <set>

#include <archon/cli/detail/pattern_symbol.hpp>


namespace archon::cli::detail {


class PositionNfa {
public:
    std::size_t create_position(std::size_t pattern_index, PatternSymbol symbol);
    void register_startpos(std::size_t pos);
    void register_followpos(std::size_t pos_1, std::size_t pos_2);

private:
    struct Position;

    using PositionSet = std::set<std::size_t>;

    std::vector<Position> m_positions;
    PositionSet m_start_positions;
};


// This is a final position (state) when, and only when `followpos` is empty. In
// a final position, the value of `symbol` has no meaning.
struct PositionNfa::Position {
    std::size_t pattern_index;
    PatternSymbol symbol;
    PositionSet followpos;
};








// Implementation


inline std::size_t PositionNfa::create_position(std::size_t pattern_index, PatternSymbol symbol)
{
    std::size_t pos = m_positions.size();
    m_positions.push_back({ pattern_index, symbol, {} }); // Throws
    return pos;
}


inline void PositionNfa::register_startpos(std::size_t pos)
{
    m_start_positions.insert(pos); // Throws
}


void PositionNfa::register_followpos(std::size_t pos_1, std::size_t pos_2)
{
    Position& pos = m_positions[pos_1];
    pos.followpos.insert(pos_2); // Throws
}


} // namespace archon::cli::detail

#endif // ARCHON_X_CLI_X_DETAIL_X_POSITION_NFA_HPP
