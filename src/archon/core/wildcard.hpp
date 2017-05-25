/*
 * This file is part of the Archon library framework.
 *
 * Copyright (C) 2012  Kristian Spangsege <kristian.spangsege@gmail.com>
 *
 * The Archon library framework is free software: You can redistribute
 * it and/or modify it under the terms of the GNU Lesser General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * The Archon library framework is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the Archon library framework.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef ARCHON_CORE_WILDCARD_HPP
#define ARCHON_CORE_WILDCARD_HPP

#include <cstddef>
#include <cstring>
#include <string>
#include <vector>


namespace archon {
namespace core {


class WildcardPattern {
public:
    explicit WildcardPattern(const std::string& text);

    bool match(const char* begin, const char* end) const noexcept;

    bool match(const char* c_str) const noexcept;

private:
    std::string m_text;

    struct Card {
        size_t m_offset, m_size;
        Card(size_t begin, size_t end) noexcept;
    };

    // Must contain at least one card. The first, and the last card may be empty
    // strings. All other cards must be non-empty. If there is exactly one card,
    // the pattern matches a string if, and only if the string is equal to the
    // card. Otherwise, the first card must be a prefix of the string, and the
    // last card must be a suffix.
    std::vector<Card> m_cards;
};




// Implementation

inline bool WildcardPattern::match(const char* c_str) const noexcept
{
    const char* begin = c_str;
    const char* end = begin + std::strlen(c_str);
    return match(begin, end);
}

inline WildcardPattern::card::card(std::size_t begin, std::size_t end) noexcept
{
    m_offset = begin;
    m_size = end - begin;
}


} // namespace core
} // namespace archon

#endif // ARCHON_CORE_WILDCARD_HPP
