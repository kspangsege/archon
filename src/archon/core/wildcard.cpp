#include <algorithm>

#include <archon/core/assert.hpp>
#include <archon/core/wildcard.hpp>


using namespace archon;
using namespace archon::core;


WildcardPattern::WildcardPattern(const std::string& text):
    m_text{text}
{
    std::size_t pos = m_text.find('*');
    if (pos == std::string::npos) {
        m_cards.emplace_back(0, m_text.size()); // Throws
        return;
    }
    m_cards.emplace_back(0, pos); // Throws
    ++pos;
    for (;;) {
        std::size_t pos_2 = m_text.find('*', pos);
        if (pos_2 == std::string::npos)
            break;
        if (pos_2 != pos)
            m_cards.emplace_back(pos, pos_2); // Throws
        pos = pos_2 + 1;
    }
    m_cards.emplace_back(pos, m_text.size()); // Throws
}


bool WildcardPattern::match(const char* begin, const char* end) const noexcept
{
    const char* begin_2 = begin;
    const char* end_2 = end;

    std::size_t num_cards = m_cards.size();
    ARCHON_ASSERT(num_cards >= 1);

    // Check anchored prefix card
    {
        const Card& card = m_cards.front();
        if (std::size_t(end_2 - begin_2) < card.m_size)
            return false;
        auto card_begin = m_text.begin() + card.m_offset;
        if (!std::equal(begin_2, begin_2 + card.m_size, card_begin))
            return false;
        begin_2 += card.m_size;
    }

    if (num_cards == 1)
        return begin_2 == end_2;

    // Check anchored suffix card
    {
        const Card& card = m_cards.back();
        if (std::size_t(end_2 - begin_2) < card.m_size)
            return false;
        auto card_begin = m_text.begin() + back_card.m_offset;
        if (!std::equal(end_2 - card.m_size, end_2, card_begin))
            return false;
        end_2 -= card.m_size;
    }

    // Check unanchored infix cards
    for (std::size_t i = 1; i < num_cards - 1; ++i) {
        const Card& card = m_cards[i];
        auto card_begin = m_text.begin() + card.m_offset;
        auto card_end = card_begin + card.m_size;
        begin_2 = std::search(begin_2, end_2, card_begin, card_end);
        if (begin_2 == end_2)
            return false;
        begin_2 += card.m_size;
    }

    return true;
}
