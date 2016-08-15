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

/**
 * \file
 *
 * \author Kristian Spangsege
 */

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

namespace archon
{
  namespace Parser
  {
    template<typename Ch, typename Tok = short unsigned, typename Tr = FsaTraits<Ch, Tok> >
    struct BasicFsaPrinter
    {
      typedef BasicFsa<Ch, Tok, Tr>        FsaType;
      typedef typename FsaType::CharType   CharType;
      typedef typename FsaType::StringType StringType;
      typedef typename FsaType::TokenId    TokenId;

      typedef Core::Text::PrinterBase<CharType, CharType> SymbolPrinter;

      StringType print(FsaType const &) const;

      BasicFsaPrinter(size_t width = 0, SymbolPrinter const & = defaultSymbolPrinter, std::locale const & = std::locale(""));

    private:
      size_t width;
      SymbolPrinter const &symPrinter;
      std::locale loc;
      Core::Text::BasicValuePrinter<CharType> valPrinter;

      struct DefaultSymbolPrinter: SymbolPrinter
      {
	StringType print(CharType const &c) const { return StringType(1, c); }
      };

      static DefaultSymbolPrinter defaultSymbolPrinter;
    };


    typedef BasicFsaPrinter<char>    FsaPrinter;
    typedef BasicFsaPrinter<wchar_t> WideFsaPrinter;




    // Template implementations:


    template<typename Ch, typename Tok, typename Tr>
    typename BasicFsaPrinter<Ch, Tok, Tr>::StringType
    BasicFsaPrinter<Ch, Tok, Tr>::print(FsaType const &fsa) const
    {
      // Widen some fixed strings
      Core::BasicLocaleCharMapper<CharType> mapper(loc);
      StringType const ping       = mapper.widen("'");
      StringType const dashPing   = mapper.widen("-'");
      StringType const commaSpace = mapper.widen(", ");
      StringType const arrow      = mapper.widen(" -> ");
      StringType const arrow2     = mapper.widen("-> ");
      StringType const bol        = mapper.widen("BOL");
      StringType const eol        = mapper.widen("EOL");
      StringType const bow        = mapper.widen("BOW");
      StringType const eow        = mapper.widen("EOW");

      Core::Text::BasicTable<CharType> table;
      table.get_col(0).set_width(1);
      table.get_col(1).set_width(1);
      table.get_col(2).set_width(1);
      table.get_col(3).set_width(8);
      table.get_cell(0,0).set_text(mapper.widen("State"));
      table.get_cell(0,1).set_text(mapper.widen("Start index"));
      table.get_cell(0,2).set_text(mapper.widen("Token ID"));
      table.get_cell(0,3).set_text(mapper.widen("FSA transitions"));

      typedef std::map<typename FsaType::StateId, typename FsaType::SizeType> StartStates;
      StartStates stateStates;
      for(typename FsaType::SizeType i = 0; i < fsa.getStartStateRegistrySize(); ++i)
        stateStates[fsa.getStartState(i)] = i;

      size_t i = 0;
      for(typename FsaType::StateSeq s = fsa.getStates(); s; ++s, ++i)
      {
	table.get_cell(i+1, 0).set_text(valPrinter.print(s->getId()));
        {
          typename StartStates::iterator j = stateStates.find(s->getId());
          if(j != stateStates.end()) table.get_cell(i+1, 1).set_text(valPrinter.print(j->second));
        }
	if(s->getTokenId() != FsaType::TraitsType::noToken())
          table.get_cell(i+1, 2).set_text(valPrinter.print(s->getTokenId()));

        StringType cell;
	for(typename FsaType::EdgeRangeSeq j = s->getEdgeRanges(); j; ++j)
	{
	  if(cell.size()) cell += commaSpace;
	  cell += ping+symPrinter.print(j->getRange().first)+ping;
	  if(j->getRange().first < j->getRange().second)
	    cell += dashPing+symPrinter.print(j->getRange().second)+ping;
	  cell += arrow+valPrinter.print(j->getTargetState());
	}

	for(typename FsaType::SentinelEdgeSeq j = s->getSentinelEdges(); j; ++j)
	{
          StringType t;
          switch(j->getSentinel())
          {
          case FsaType::anchor_bol: t = bol; break;
          case FsaType::anchor_eol: t = eol; break;
          case FsaType::anchor_bow: t = bow; break;
          case FsaType::anchor_eow: t = eow; break;
          }
	  if(cell.size()) cell += commaSpace;
	  cell += t;
	  cell += arrow+valPrinter.print(j->getTargetState());
	}

	for(typename FsaType::EpsilonEdgeSeq j = s->getEpsilonEdges(); j; ++j)
	{
	  if(cell.size()) cell += commaSpace;
	  cell += arrow2+valPrinter.print(j->getTargetState());
	}

        table.get_cell(i+1, 3).set_text(cell);
      }

      return table.print(width, 3, true, loc);
    }


    template<typename Ch, typename Tok, typename Tr>
    BasicFsaPrinter<Ch, Tok, Tr>::BasicFsaPrinter(size_t w, SymbolPrinter const &s, std::locale const &l):
      width(w), symPrinter(s), loc(l)
    {
    }


    template<typename Ch, typename Tok, typename Tr>
    typename BasicFsaPrinter<Ch, Tok, Tr>::DefaultSymbolPrinter BasicFsaPrinter<Ch, Tok, Tr>::defaultSymbolPrinter;
  }
}

#endif // ARCHON_PARSER_FSA_PRINT_HPP
