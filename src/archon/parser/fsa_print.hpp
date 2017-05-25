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

/// \file
///
/// \author Kristian Spangsege

#ifndef ARCHON_PARSER_FSA_PRINT_HPP
#define ARCHON_PARSER_FSA_PRINT_HPP

#include <cwchar>
#include <vector>
#include <map>
#include <locale>

#include <archon/core/codec.hpp>
#include <archon/core/text.hpp>
#include <archon/core/text_table.hpp>

#include <archon/parser/fsa.hpp>

namespace archon {
namespace parser {

template<typename Ch, typename Tok = short unsigned, typename Tr = FsaTraits<Ch, Tok>>
class BasicFsaPrinter {
public:
    using FsaType    = BasicFsa<Ch, Tok, Tr>;
    using CharType   = typename FsaType::CharType;
    using StringType = typename FsaType::StringType;
    using TokenId    = typename FsaType::TokenId;

    using SymbolPrinter = core::Text::PrinterBase<CharType, CharType>;

    StringType print(FsaType const &) const;

    BasicFsaPrinter(std::size_t width = 0, const SymbolPrinter& = s_default_symbol_printer,
                    const std::locale& = std::locale::classic());

private:
    std::size_t m_width;
    const SymbolPrinter& m_sym_printer;
    std::locale m_locale;

    class DefaultSymbolPrinter: public SymbolPrinter {
    public:
        StringType print(const CharType& c) const { return StringType(1,c); }
    };

    static DefaultSymbolPrinter s_default_symbol_printer;
};


using FsaPrinter     = BasicFsaPrinter<char>;
using WideFsaPrinter = BasicFsaPrinter<wchar_t>;



// Implementation

template<typename Ch, typename Tok, typename Tr>
typename BasicFsaPrinter<Ch, Tok, Tr>::StringType
BasicFsaPrinter<Ch, Tok, Tr>::print(const FsaType& fsa) const
{
    // Widen some fixed strings
    core::BasicLocaleCharMapper<CharType> mapper(m_locale);
    StringType ping        = mapper.widen("'");
    StringType dash_ping   = mapper.widen("-'");
    StringType comma_space = mapper.widen(", ");
    StringType arrow       = mapper.widen(" -> ");
    StringType arrow_2     = mapper.widen("-> ");
    StringType bol         = mapper.widen("BOL");
    StringType eol         = mapper.widen("EOL");
    StringType bow         = mapper.widen("BOW");
    StringType eow         = mapper.widen("EOW");

    core::Text::BasicTable<CharType> table;
    table.get_col(0).set_width(1);
    table.get_col(1).set_width(1);
    table.get_col(2).set_width(1);
    table.get_col(3).set_width(8);
    table.get_cell(0,0).set_text(mapper.widen("State"));
    table.get_cell(0,1).set_text(mapper.widen("Start index"));
    table.get_cell(0,2).set_text(mapper.widen("Token ID"));
    table.get_cell(0,3).set_text(mapper.widen("FSA transitions"));

    using StartStates = std::map<typename FsaType::StateId, typename FsaType::SizeType>;
    StartStates state_states;
    for (typename FsaType::SizeType i = 0; i < fsa.getStartStateRegistrySize(); ++i)
        state_states[fsa.getStartState(i)] = i;

    std::size_t i = 0;
    for (typename FsaType::StateSeq s = fsa.getStates(); s; ++s, ++i) {
        table.get_cell(i+1, 0).set_text(core::format_int<CharType>(s->getId(), m_locale));
        {
            auto j = state_states.find(s->getId());
            if (j != state_states.end())
                table.get_cell(i+1, 1).set_text(core::format_int<CharType>(j->second, m_locale));
        }
        if (s->getTokenId() != FsaType::TraitsType::noToken())
            table.get_cell(i+1, 2).set_text(core::format_int<CharType>(s->getTokenId(), m_locale));

        StringType cell;
        for (typename FsaType::EdgeRangeSeq j = s->getEdgeRanges(); j; ++j) {
            if (cell.size())
                cell += comma_space;
            cell += ping+m_sym_printer.print(j->getRange().first)+ping;
            if (j->getRange().first < j->getRange().second)
                cell += dash_ping+m_sym_printer.print(j->getRange().second)+ping;
            cell += arrow+core::format_int<CharType>(j->getTargetState(), m_locale);
        }

        for (typename FsaType::SentinelEdgeSeq j = s->getSentinelEdges(); j; ++j) {
            StringType t;
            switch (j->getSentinel()) {
                case FsaType::anchor_bol:
                    t = bol;
                    break;
                case FsaType::anchor_eol:
                    t = eol;
                    break;
                case FsaType::anchor_bow:
                    t = bow;
                    break;
                case FsaType::anchor_eow:
                    t = eow;
                    break;
            }
            if (cell.size())
                cell += comma_space;
            cell += t;
            cell += arrow+core::format_int<CharType>(j->getTargetState(), m_locale);
        }

        for (typename FsaType::EpsilonEdgeSeq j = s->getEpsilonEdges(); j; ++j) {
            if (cell.size())
                cell += comma_space;
            cell += arrow_2+core::format_int<CharType>(j->getTargetState(), m_locale);
        }

        table.get_cell(i+1, 3).set_text(cell);
    }

    return table.print(m_width, 3, true, m_locale);
}


template<typename Ch, typename Tok, typename Tr>
BasicFsaPrinter<Ch, Tok, Tr>::BasicFsaPrinter(std::size_t width, const SymbolPrinter& sym_printer,
                                              const std::locale& locale):
    m_width{width},
    m_sym_printer{sym_printer},
    m_locale(locale)
{
}


template<typename Ch, typename Tok, typename Tr>
typename BasicFsaPrinter<Ch, Tok, Tr>::DefaultSymbolPrinter
BasicFsaPrinter<Ch, Tok, Tr>::s_default_symbol_printer;

} // namespace parser
} // namespace archon

#endif // ARCHON_PARSER_FSA_PRINT_HPP
