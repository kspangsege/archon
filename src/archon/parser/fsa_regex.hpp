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

#ifndef ARCHON_PARSER_FSA_REGEX_HPP
#define ARCHON_PARSER_FSA_REGEX_HPP

#include <stdexcept>
#include <limits>
#include <vector>

#include <archon/util/range_map.hpp>

#include <archon/parser/regex.hpp>
#include <archon/parser/fsa.hpp>

namespace archon
{
  namespace Parser
  {
    /**
     * Add a new start state to the specified automaton through which
     * it will accept the language accepted by the specified regular
     * expression.
     *
     * \param a The automaton whose language is to be extended.
     *
     * \param r A regular expression.
     *
     * \param t The token ID to associate with the new language.
     *
     * \return The start state registry index of the new language.
     */
    template<typename Ch, typename Tok, typename Tr>
    typename BasicFsa<Ch, Tok, Tr>::SizeType
    addRegex(BasicFsa<Ch, Tok, Tr> &a, typename BasicRegex<Ch>::ExpArg r,
             typename BasicFsa<Ch, Tok, Tr>::TokenId t = BasicFsa<Ch, Tok, Tr>::TraitsType::defaultToken());

    /**
     * Construct an NFA fragment inside the specified NFA that
     * recognizes precisely the same language as the specified regular
     * expression denotes.
     *
     * \param a The automaton inside which the fragment should be
     * created.
     *
     * \param r The regular expression from which the NFA fragment
     * should be built.
     *
     * \return A pair (start, stop) of state IDs which are the assumed
     * start and stop states of the resulting fragment.
     */
    template<typename Ch, typename Tok, typename Tr>
    typename BasicFsa<Ch, Tok, Tr>::StatePair
    regexFragment(BasicFsa<Ch, Tok, Tr> &a, typename BasicRegex<Ch>::ExpArg r);

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
    template<typename Ch, typename Tok, typename Tr>
    typename BasicFsa<Ch, Tok, Tr>::StatePair
    repeatFragment(BasicFsa<Ch, Tok, Tr> &a, typename BasicRegex<Ch>::ExpArg r,
                   size_t min = 0, size_t max = 0);




    // Template implementations:


    template<typename Ch, typename Tok, typename Tr>
    typename BasicFsa<Ch, Tok, Tr>::SizeType
    addRegex(BasicFsa<Ch, Tok, Tr> &a, typename BasicRegex<Ch>::ExpArg r,
             typename BasicFsa<Ch, Tok, Tr>::TokenId t)
    {
      typedef typename BasicFsa<Ch, Tok, Tr>::StatePair StatePair;
      StatePair f = regexFragment(a, r);
      a.setTokenId(f.second, t);
      return a.registerStartState(f.first);
    }


    template<typename Ch, typename Tok, typename Tr>
    typename BasicFsa<Ch, Tok, Tr>::StatePair
    regexFragment(BasicFsa<Ch, Tok, Tr> &a, typename BasicRegex<Ch>::ExpArg r)
    {
      typedef Ch                          CharType;
      typedef BasicRegex<CharType>        RegexType;
      typedef BasicFsa<CharType, Tok, Tr> FsaType;

      if(typename RegexType::Alt const *e = dynamic_cast<typename RegexType::Alt const *>(r.get()))
      {
        typename FsaType::StatePair f = regexFragment(a, e->e1);
        return a.alternFragments(f, regexFragment(a, e->e2));
      }

      if(typename RegexType::Jux const *e = dynamic_cast<typename RegexType::Jux const *>(r.get()))
      {
        typename FsaType::StatePair f = regexFragment(a, e->e1);
        return a.concatFragments(f, regexFragment(a, e->e2));
      }

      if(typename RegexType::Rep const *e = dynamic_cast<typename RegexType::Rep const *>(r.get()))
        return repeatFragment(a, e->e, e->min, e->max);

      if(typename RegexType::Str const *e = dynamic_cast<typename RegexType::Str const *>(r.get()))
        return a.stringFragment(e->s);

      if(typename RegexType::Bra const *e = dynamic_cast<typename RegexType::Bra const *>(r.get()))
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

        std::vector<typename FsaType::CharRange> ranges;
        for(typename RangeMap::RangeSeq i=rangeMap.get_ranges(); i; ++i)
          if(i->get_value()) ranges.push_back(typename FsaType::CharRange(i->get_first(), i->get_last()));

	return a.rangesFragment(ranges.begin(), ranges.end());
      }

      if(dynamic_cast<typename RegexType::Bol const *>(r.get())) return a.sentinelFragment(FsaType::anchor_bol);
      if(dynamic_cast<typename RegexType::Eol const *>(r.get())) return a.sentinelFragment(FsaType::anchor_eol);
      if(dynamic_cast<typename RegexType::Bow const *>(r.get())) return a.sentinelFragment(FsaType::anchor_bow);
      if(dynamic_cast<typename RegexType::Eow const *>(r.get())) return a.sentinelFragment(FsaType::anchor_eow);

      throw std::invalid_argument("Unsupported Regex node");
    }


    template<typename Ch, typename Tok, typename Tr>
    typename BasicFsa<Ch, Tok, Tr>::StatePair
    repeatFragment(BasicFsa<Ch, Tok, Tr> &a, typename BasicRegex<Ch>::ExpArg r,
                   size_t min, size_t max)
    {
      typedef BasicFsa<Ch, Tok, Tr> FsaType;

      if(max && max < min) throw std::invalid_argument("Bad repetition range");

      typename FsaType::StatePair f = regexFragment(a, r);

      if(max == 0)
      {
        if(min == 0)  return a.optionalFragment(a.repeatFragment(f)); // Kleene closure
        if(min == 1u) return a.repeatFragment(f); // Positive closure
        return a.concatFragments(f, repeatFragment(a, r, min-1u, 0));
      }

      if(max == 1u)
      {
        if(min == 0) return a.optionalFragment(f);
        return f;
      }

      return min == 0 ?
        a.optionalFragment(a.concatFragments(f, repeatFragment(a, r, 0, max-1u))) :
        a.concatFragments(f, repeatFragment(a, r, min-1u, max-1u));
    }
  }
}

#endif // ARCHON_PARSER_FSA_REGEX_HPP
