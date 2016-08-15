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

#ifndef ARCHON_PARSER_DFA_OPS_HPP
#define ARCHON_PARSER_DFA_OPS_HPP

#include <utility>
#include <map>
#include <stack>

#include <archon/core/refcnt.hpp>

#include <archon/parser/dfa.hpp>
#include <archon/parser/nfa_base.hpp>

namespace archon
{
  namespace Parser
  {
    /**
     * Construct a DFA that accepts exactly the same language as the
     * specidfied NFA for corresponding start states. The start state
     * registry of the DFA will have the same size as the start state
     * registry of the NFA, and corresponding start states are found
     * at equal indices.
     *
     * \param nfa The non-deterministic automaton that defines which
     * strings the new deterministic automaton should accept.
     *
     * \param stateSets If not 0 the final state sets resulting from
     * the subset construction algorithm will be stored in the passed
     * structure. Each entry in the map will associate a state ID of
     * the new DFA with the corresponding set of NFA states.
     *
     * The DFA contruction works by creating sets of NFA states that
     * correspond to DFA states.
     *
     * First each DFA start state is constructed from the set of NFA
     * states reachable from the corresponding NFA start state through
     * epsilon edges alone.
     *
     * Next each newly constructed state set is considered in
     * turn. The symbol edges leaving states from the current set
     * gives rise to the construction of new state sets for each
     * possible symbol. It does so, because each symbol edge adds its
     * target state to the state set associated with that symbol.
     *
     * For each symbol the associated state set is closed by adding
     * new states that are reachable from existing states in the set
     * through epsilon edges alone.
     *
     * Those of the closed state sets that are not equal to any of the
     * state sets already handled are then added to the list of newly
     * contructed state sets.
     *
     * \todo Optimize by not adding anchor edges with the same source
     * and target state.  The state set should be fetched out of
     * edgeMap. Then updated and if it is equal to the self state set
     * then delete the anchor edge other wise add the new states to
     * its state set.
     */
    Core::CntRef<Dfa> constructDfaFromNfa(Core::CntRef<NfaBase const> const &nfa,
                                          std::map<Dfa::StateId, NfaBase::StateSet> *stateSets = 0)
    {
      if(nfa.empty()) return; // Handle the special empty NFA

      // Maps NFA state sets to DFA state IDs.
      typedef std::map<NfaBase::StateSet, Dfa::StateId> StateMap;
      StateMap stateMap;

      typedef StateMap::value_type StateBond;
      std::stack<StateBond> uncheckedStates;

      // Add a DFA start state for each NFA start state.
      for(size_t i = 0; i < nfa->getStartStateRegistrySize(); ++i)
      {
	Nfa::StateSet stateSet;
	nfa->closedAdd(nfa->getStartState(i), stateSet);
        std::pair<StateMap::iterator, bool> r = stateMap.insert(std::make_pair(stateSet, Dfa::TraitsType::noState()));
	if(r.second)
	{
	  r.first->second = dfa->addState(nfa->chooseFinalValue(stateSet));
	  uncheckedStates.push(*r.first);
	}
        dfa->registerStartState(r.first->second);
      }

      while(!uncheckedStates.empty())
      {
	StateBond const &stateBond = uncheckedStates.top();
	NfaBase::EdgeMap edgeMap;
	nfa->initEdgeMap(stateBond.first, edgeMap);


-->


	// Anchor regret
	if(anchorInfo)
	{
	  // Check if there are any anchor symbols among the edges
	  // leavinge the current state set.

          // Based on range starts find the first range that potentially contains an anchor symbol
	  Nfa::EdgeMap::iterator i = edgeMap.lower_bound(anchorInfo->start);
	  if(i != edgeMap.begin() && (i == edgeMap.end() || anchorInfo->start < i->first)) --i;

	  for(size_t j=0; j<anchorInfo->numberOf; ++j)
	  {
	    CharType anchor = anchorInfo->start + j;
	    // If we stepped beyond the end of the current edge range,
	    // then step to the next one.
	    while(i != edgeMap.end() && i->second.first < anchor) ++i;
	    if(i == edgeMap.end()) break;
	    // Is the current anchor symbol within the current edge
	    // range?
	    if(i->first <= anchor)
	    {
	      for(Nfa::StateSet::iterator k=stateBond.first.begin(); k!=stateBond.first.end(); ++k)
		nfa.updateEdgeMap(Nfa::Range(anchor, anchor), *k, edgeMap);
	    }
	  }
	}

	StateId const state = stateBond.second;
	uncheckedStates.pop();

        // Create new target DFA states for eny previously seen state
        // set in the edge map
	for(Nfa::EdgeMap::iterator i=edgeMap.begin(); i!=edgeMap.end(); ++i)
	{
	  pair<StateMap::iterator, bool> r =
	    stateMap.insert(make_pair(i->second.second, nonState));
	  if(r.second)
	  {
	    r.first->second = addState(nfa.chooseFinalValue(r.first->first));
	    uncheckedStates.push(*r.first);
	  }
	  addEdgeRange(i->first, i->second.first, state, r.first->second);
	}
      }

      if(stateSets)
      {
	stateSets->clear();
	stateSets->resize(states.size());
	for(StateMap::iterator i=stateMap.begin(); i!=stateMap.end(); ++i)
	  (*stateSets)[i->second] = i->first;
      }
    }
  }
}

#endif // ARCHON_PARSER_DFA_OPS_HPP
