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

#ifndef ARCHON_PARSER_NFA_PRINT_HPP
#define ARCHON_PARSER_NFA_PRINT_HPP

#include <cwchar>
#include <vector>
#include <map>
#include <locale>

#include <archon/core/codec.hpp>
#include <archon/core/text.hpp>

#include <archon/parser/nfa.hpp>

namespace Archon
{
  namespace Parser
  {
    template<typename Ch, typename Tok = short unsigned, typename Tr = FsaTraits<Ch, Tok> >
    struct BasicNfaPrinter
    {
      typedef BasicNfa<Ch, Tok, Tr>        NfaType;
      typedef typename NfaType::CharType   CharType;
      typedef typename NfaType::StringType StringType;
      typedef typename NfaType::TokenId    TokenId;

      typedef Core::Text::PrinterBase<CharType, CharType> SymbolPrinter;

      StringType print(typename NfaType::ConstRefArg) const;

      BasicNfaPrinter(size_t width = 0, SymbolPrinter const & = defaultSymbolPrinter, std::locale const & = std::locale(""));

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


    typedef BasicNfaPrinter<char>    NfaPrinter;
    typedef BasicNfaPrinter<wchar_t> WideNfaPrinter;




    // Template implementations:


    template<typename Ch, typename Tok, typename Tr>
    typename BasicNfaPrinter<Ch, Tok, Tr>::StringType BasicNfaPrinter<Ch, Tok, Tr>::print(typename NfaType::ConstRefArg nfa) const
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

      std::vector<double> columnWidthFractions;
      columnWidthFractions.push_back(0.1);
      columnWidthFractions.push_back(0.1);
      columnWidthFractions.push_back(0.1);
      columnWidthFractions.push_back(0.8);
      Core::Text::Table<CharType> table(nfa->getNumberOfStates()+1, columnWidthFractions);
      table(0, 0) = mapper.widen("State");
      table(0, 1) = mapper.widen("Start index");
      table(0, 2) = mapper.widen("Token ID");
      table(0, 3) = mapper.widen("NFA transitions");

      typedef std::map<typename NfaType::StateId, typename NfaType::SizeType> StartStates;
      StartStates stateStates;
      for(typename NfaType::SizeType i = 0; i < nfa->getStartStateRegistrySize(); ++i)
        stateStates[nfa->getStartState(i)] = i;

      size_t i = 0;
      for(typename NfaType::StateSeq s = nfa->getStates(); s; ++s, ++i)
      {
	table(i+1, 0) = valPrinter.print(s->getId());
        {
          typename StartStates::iterator j = stateStates.find(s->getId());
          if(j != stateStates.end()) table(i+1, 1) = valPrinter.print(j->second);
        }
	if(s->getTokenId() != NfaType::TraitsType::noToken()) table(i+1, 2) = valPrinter.print(s->getTokenId());

	for(typename NfaType::EdgeRangeSeq j = s->getEdgeRanges(); j; ++j)
	{
	  if(table(i+1, 3).size()) table(i+1, 3) += commaSpace;
	  table(i+1, 3) += ping+symPrinter.print(j->getRange().first)+ping;
	  if(j->getRange().first < j->getRange().second)
	    table(i+1, 3) += dashPing+symPrinter.print(j->getRange().second)+ping;
	  table(i+1, 3) += arrow+valPrinter.print(j->getTargetState());
	}

	for(typename NfaType::SentinelEdgeSeq j = s->getSentinelEdges(); j; ++j)
	{
          StringType t;
          switch(j->getSentinel())
          {
          case NfaType::anchor_bol: t = bol; break;
          case NfaType::anchor_eol: t = eol; break;
          case NfaType::anchor_bow: t = bow; break;
          case NfaType::anchor_eow: t = eow; break;
          }
	  if(table(i+1, 3).size()) table(i+1, 3) += commaSpace;
	  table(i+1, 3) += t;
	  table(i+1, 3) += arrow+valPrinter.print(j->getTargetState());
	}

	for(typename NfaType::EpsilonEdgeSeq j = s->getEpsilonEdges(); j; ++j)
	{
	  if(table(i+1, 3).size()) table(i+1, 3) += commaSpace;
	  table(i+1, 3) += arrow2+valPrinter.print(j->getTargetState());
	}
      }

      return table.print(width, 3, true, loc);
    }


    template<typename Ch, typename Tok, typename Tr>
    BasicNfaPrinter<Ch, Tok, Tr>::BasicNfaPrinter(size_t w, SymbolPrinter const &s, std::locale const &l):
      width(w), symPrinter(s), loc(l)
    {
    }


    template<typename Ch, typename Tok, typename Tr>
    typename BasicNfaPrinter<Ch, Tok, Tr>::DefaultSymbolPrinter BasicNfaPrinter<Ch, Tok, Tr>::defaultSymbolPrinter;
  }
}

#endif // ARCHON_PARSER_NFA_PRINT_HPP
