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
 *
 * An implementation of arbitrary nondeterministic finite state
 * automata (NFA).
 *
 * As an extension to a standard NFA, this one may have more than one
 * start state.
 *
 * This kind og automaton can be constructed from a regular expression
 * and the two will then define the exact same regular language.
 *
 * All parts are Unicode enabled.
 *
 * \sa Regex
 */

#ifndef ARCHON_PARSER_NFA_BASE_HPP
#define ARCHON_PARSER_NFA_BASE_HPP

#include <set>
#include <map>

#include <archon/core/refcnt.hpp>
#include <archon/util/range_map.hpp>

#include <archon/parser/fsa_base.hpp>

namespace archon
{
  namespace parser
  {
    /**
     * An abstract interface for an arbitrary non-deterministic finite state
     * automaton (NFA).
     *
     * The NFA adds to the general state machine API a number of
     * methods that are needed to construct a DFA from it.
     *
     * \sa FsaBase
     *
     * \sa Dfa
     */
    template<typename Ch, typename Tok = short unsigned, typename Tr = FsaTraits<Ch, Tok> >
    struct NfaBase: FsaBase<Ch, Tok, Tr>
    {
      typedef FsaBase<Ch, Tok, Tr> _Super;
      typedef typename _Super::TraitsType  TraitsType;
      typedef typename _Super::CharType    CharType;
      typedef typename _Super::StringType  StringType;
      typedef typename _Super::StreamType  StreamType;
      typedef typename _Super::SizeType    SizeType;
      typedef typename _Super::Sentinel    Sentinel;
      typedef typename _Super::TokenId     TokenId;
      typedef typename _Super::StateId     StateId;
      typedef std::set<StateId>            StateSet;

      typedef core::CntRef<NfaBase>        Ref;
      typedef core::CntRef<NfaBase const>  ConstRef;
      typedef Ref const                   &RefArg;
      typedef ConstRef const              &ConstRefArg;

      typedef std::map<Sentinel, StateSet> SentinelMap;

      struct EdgeMap
      {
        /**
         * This data structure represents all the states that are
         * reachable from some specific state set for each possible
         * input symbol. It is chosen to represent this information in a
         * compact, non-reduntant and non-ambiguous way while still
         * being relatively easy and fast to update.
         *
         * It consists of ordered non-overlaping and non-empty symbol
         * ranges each associated with a non-empty set of
         * states. Further more, it is not allowed for two symbol ranges
         * to have identical state sets if one follows immediately after
         * the other (with no intervening symbols.)
         *
         * Each entry in the top-level map associates the first symbol
         * in a range with the final symbol of that range and with the
         * set of reachable states.
         */
        util::RangeMap<CharType, StateSet> ranges;

        SentinelMap sentinels;
      };

      /**
       * Add the epsilon closure of the specified state to the
       * specified set. That is, add first the specified state then
       * add all states that are reachable from the first state
       * through epsilon edges alone.
       *
       * \param state The state whose epsilon closure determines the
       * set of states to add.
       *
       * \param set The target set of states to which zero or more
       * states will be added.
       *
       * \return True iff at least one state was added to the target
       * set.
       *
       * \note The main purpose of this method is to support NFA to
       * DFA conversion.
       */
      virtual bool closedAdd(StateId state, StateSet &set) const = 0;

      /**
       * Fill in the passed edge map such that it represents the
       * possible transitions from any ot the states in the specified
       * state set.
       *
       * For each edge on symbol S leading away from a state in the
       * specified set to state T it does the followig: Add the
       * epsilon closure of T to the state set currently associated
       * with S in the edge map. It must of course keep the edge map
       * in good shape according the definition of the \c EdgeMap
       * structure, so it may require both splitting and merging of
       * ranges in the map.
       *
       * \param s The set of states whose edges should be used to
       * update the edge map.
       *
       * \param m The edge map to be filled in. Should be empty at
       * entry.
       *
       * \note The main purpose of this method is to support NFA to
       * DFA conversion.
       */
      virtual void initEdgeMap(StateSet const &s, EdgeMap &m) const = 0;

      /**
       * Choose the numerically greatest token ID among the accepting
       * states in the specified set. If the specified set contains no
       * accepting states, then return \c TraitsType::noToken()'.
       *
       * \param s The set of states to scan for the numerically
       * greatest token ID.
       *
       * \note The main purpose of this method is to support NFA to
       * DFA conversion.
       */
      virtual TokenId chooseTokenId(StateSet const &s) const = 0;
    };
  }
}

#endif // ARCHON_PARSER_NFA_BASE_HPP
