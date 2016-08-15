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

#ifndef ARCHON_PARSER_NFA_OPS_HPP
#define ARCHON_PARSER_NFA_OPS_HPP

#include <cwchar>
#include <stdexcept>
#include <limits>
#include <vector>

#include <archon/util/range_map.hpp>

#include <archon/parser/nfa.hpp>
#include <archon/parser/regex.hpp>

namespace archon
{
  namespace Parser
  {
    template<typename Ch, typename Tok = short unsigned, typename FsaTr = FsaTraits<Ch, Tok> >
    struct BasicNfaFromRegex
    {
      typedef Ch                             CharType;
      typedef BasicNfa<CharType, Tok, FsaTr> NfaType;
      typedef BasicRegex<CharType>           RegexType;

      /**
       * Construct an NFA with one start state that recognizes exactly
       * the same language as the specified regular expression denotes.
       *
       * \param regex The regular expression from which the NFA should
       * be built.
       *
       * \param tokenId The token ID that should be reported on a match.
       *
       * \return An NFA that reconizes the same language as the
       * specified regular expression.
       */
      static typename NfaType::Ref construct(typename RegexType::ExpArg regex, typename NfaType::TokenId tokenId = NfaType::TraitsType::defaultToken());
      
      /**
       * Construct an NFA fragment inside the specified NFA that
       * recognizes precisely the same language as the specified regular
       * expression denotes.
       *
       * If you are interested in a complete NFA rather than a fragment
       * then use \c constructNfaFromRegex instead. This function is
       * usefull when building NFAs for lexical analysers where multiple
       * regular expressions should be associates with different token
       * IDs.
       *
       * \param nfa The NFA inside which the fragment should be created.
       *
       * \param regex The regular expression from which the NFA should
       * be built.
       *
       * \return A pair (start, stop) of state IDs which are the assumed
       * start and stop states of the resulting fragment.
       */
      static typename NfaType::StatePair constructFragment(typename NfaType::RefArg nfa, typename RegexType::ExpArg regex);

      /**
       * Repeat the specified regular expression such that if \c{max
       * != 0} the resulting fragment recognizes precisely the \c{U{
       * L^n | n in [min, max] }} where \c L is the language denoted
       * by \c regex and L^n is the concatenation of n instances of
       * L. If \c{max == 0} the above applies assuming max is positive
       * infinity, however the recognized language can also be
       * described as precisely the concatenation of \c L^min and \c
       * L* where \c L* is the kleene closure of \c L.
       *
       * \param min The minimum number of repetitions.
       *
       * \param max The maximum number of repetitions. Zero indicates
       * that there is no upper bound to the number of
       * repetitions. When \c max is not zero it must be greater than
       * or equal to \c min.
       *
       * \return A pair (start, stop) of state IDs which are the
       * assumed start and stop states of the resulting fragment.
       */
      static typename NfaType::StatePair repeatFragment(typename NfaType::RefArg nfa, typename RegexType::ExpArg regex, size_t min = 0, size_t max = 0);
    };


    typedef BasicNfaFromRegex<char>    NfaFromRegex;
    typedef BasicNfaFromRegex<wchar_t> WideNfaFromRegex;




    // Template implementations:


    template<typename Ch, typename Tok, typename FsaTr>
    typename BasicNfaFromRegex<Ch, Tok, FsaTr>::NfaType::Ref BasicNfaFromRegex<Ch, Tok, FsaTr>::construct(typename RegexType::ExpArg regex, typename NfaType::TokenId tokenId)
    {
      typename NfaType::Ref nfa(new NfaType());
      typename NfaType::StatePair f = constructFragment(nfa, regex);
      nfa->registerStartState(f.first);
      nfa->setTokenId(f.second, tokenId);
      return nfa;
    }


    template<typename Ch, typename Tok, typename FsaTr>
    typename BasicNfaFromRegex<Ch, Tok, FsaTr>::NfaType::StatePair BasicNfaFromRegex<Ch, Tok, FsaTr>::constructFragment(typename NfaType::RefArg nfa, typename RegexType::ExpArg regex)
    {
      if(typename RegexType::Alt const *e = dynamic_cast<typename RegexType::Alt const *>(regex.get()))
      {
        typename NfaType::StatePair f = constructFragment(nfa, e->e1);
        return nfa->alternFragments(f, constructFragment(nfa, e->e2));
      }

      if(typename RegexType::Jux const *e = dynamic_cast<typename RegexType::Jux const *>(regex.get()))
      {
        typename NfaType::StatePair f = constructFragment(nfa, e->e1);
        return nfa->concatFragments(f, constructFragment(nfa, e->e2));
      }

      if(typename RegexType::Rep const *e = dynamic_cast<typename RegexType::Rep const *>(regex.get()))
        return repeatFragment(nfa, e->e, e->min, e->max);

      if(typename RegexType::Str const *e = dynamic_cast<typename RegexType::Str const *>(regex.get()))
        return nfa->stringFragment(e->s);

      if(typename RegexType::Bra const *e = dynamic_cast<typename RegexType::Bra const *>(regex.get()))
      {
	if(!e->classes.empty()) throw std::invalid_argument("Named classes are not supported yet");

        typedef util::RangeMap<CharType, bool> RangeMap;
	RangeMap rangeMap;
        bool value = true;
	if(e->invert)
	{
	  rangeMap.assign(std::numeric_limits<CharType>::min(), std::numeric_limits<CharType>::max(), true);
          value = false;
        }
        for(typename std::vector<typename RegexType::CharRange>::const_iterator i=e->ranges.begin(); i!=e->ranges.end(); ++i)
          rangeMap.assign(i->first, i->second, value);

        std::vector<typename NfaType::CharRange> ranges;
        for(typename RangeMap::RangeSeq i=rangeMap.getRanges(); i; ++i)
          if(i->getValue()) ranges.push_back(typename NfaType::CharRange(i->getFirst(), i->getLast()));

	return nfa->rangesFragment(ranges.begin(), ranges.end());
      }

      if(dynamic_cast<typename RegexType::Bol const *>(regex.get())) return nfa->sentinelFragment(NfaType::anchor_bol);
      if(dynamic_cast<typename RegexType::Eol const *>(regex.get())) return nfa->sentinelFragment(NfaType::anchor_eol);
      if(dynamic_cast<typename RegexType::Bow const *>(regex.get())) return nfa->sentinelFragment(NfaType::anchor_bow);
      if(dynamic_cast<typename RegexType::Eow const *>(regex.get())) return nfa->sentinelFragment(NfaType::anchor_eow);

      throw std::invalid_argument("Unsupported Regex node");
    }


    template<typename Ch, typename Tok, typename FsaTr>
    typename BasicNfaFromRegex<Ch, Tok, FsaTr>::NfaType::StatePair BasicNfaFromRegex<Ch, Tok, FsaTr>::repeatFragment(typename NfaType::RefArg nfa, typename RegexType::ExpArg regex, size_t min, size_t max)
    {
      if(max && max < min) throw std::invalid_argument("Bad repetition range");

      typename NfaType::StatePair f = constructFragment(nfa, regex);

      if(max == 0)
      {
        if(min == 0)  return nfa->optionalFragment(nfa->repeatFragment(f)); // Kleene closure
        if(min == 1u) return nfa->repeatFragment(f); // Positive closure
        return nfa->concatFragments(f, repeatFragment(nfa, regex, min-1u, 0));
      }

      if(max == 1u)
      {
        if(min == 0) return nfa->optionalFragment(f);
        return f;
      }

      return min == 0 ?
        nfa->optionalFragment(nfa->concatFragments(f, repeatFragment(nfa, regex, 0, max-1u))) :
        nfa->concatFragments(f, repeatFragment(nfa, regex, min-1u, max-1u));
    }
  }
}

#endif // ARCHON_PARSER_NFA_OPS_HPP
