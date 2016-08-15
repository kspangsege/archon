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


#ifndef ARCHON_PARSER_DFA_HPP
#define ARCHON_PARSER_DFA_HPP

#include <vector>
#include <list>
#include <map>

#include <archon/core/text.hpp>
#include <archon/util/range_map.hpp>

#include <archon/parser/dfa_base.hpp>

namespace archon
{
  namespace Parser
  {
    /**
     * A generic implementation of the abstract DFA API. It uses
     * tables to encode the transition function and allows it to be
     * modified in various ways.
     *
     * \sa DfaBase
     */
    template<typename Ch, typename Tok = short unsigned, typename Tr = FsaTraits<Ch, Tok> >
    struct DfaBase: DfaBase<Ch, Tok, Tr>
    {
      typedef DfaBase<Ch, Tok, Tr> _Super;
      typedef typename _Super::TraitsType  TraitsType;
      typedef typename _Super::CharType    CharType;
      typedef typename _Super::StringType  StringType;
      typedef typename _Super::StreamType  StreamType;
      typedef typename _Super::SizeType    SizeType;
      typedef typename _Super::Sentinel    Sentinel;
      typedef typename _Super::TokenId     TokenId;
      typedef typename _Super::StateId     StateId;

      typedef core::CntRef<BasicDfa>       Ref;
      typedef core::CntRef<BasicDfa const> ConstRef;
      typedef Ref const                   &RefArg;
      typedef ConstRef const              &ConstRefArg;


      /**
       * Specifies a range of symbols. The range begins with \c
       * CharRange.first and ends with \c CharRange.second and
       * includes both.
       */
      typedef std::pair<CharType, CharType> CharRange;


      /**
       * Make an empty DFA. That is, a DFA with no states at
       * all. Since such an DFA has no starts states, it accepts
       * nothing.
       */
      BasicDfa() {}

      /**
       * Add a new state to the DFA.
       *
       * \param tokenId The default value will make the new state a
       * non-accepting state. Any other value will make it an
       * accepting state. If you need only one kind of accepting state
       * in your automaton, you should pass
       * TraitsType::defaultToken(). Different token IDs are usefull
       * for lexers that produce a number of distinct tokens.
       *
       * If you want to create a new start state, use this method
       * first, then pass the returned state ID to \c
       * registerStartState.
       *
       * \return The ID of the new state.
       *
       * \sa registerStartState
       */
      StateId addState(TokenId tokenId = TraitsType::noToken());

      /**
       * Change the token ID for the state with the specified ID.
       *
       * \param s The ID of the state whose token ID you want to
       * change.
       *
       * \param t The new token ID to assign to the specified state.
       */
      void setTokenId(StateId s, TokenId t) { states[s].tokenId = t; }

      /**
       * Register an existing state as a new start state.
       *
       * \param s The ID of the state to register as a new start
       * state.
       *
       * \return The start state registry index which may be used to
       * identify this start state across machine transformations.
       *
       * \sa addState
       */
      SizeType registerStartState(StateId s);

      /**
       * Add an ordinary edge between two states.
       *
       * \param origin The ID of the state from which the new edge
       * should originate.
       *
       * \param target The ID of the state to which the new edge
       * should lead.
       *
       * \param symbol The input symbol to associate with the new
       * edge.
       */
      void addEdge(StateId origin, StateId target, CharType symbol)
      {
	addEdgeRange(origin, target, CharRange(symbol, symbol));
      }

      /**
       * Add a range of ordinary edges between two states. One edge
       * wil be added for each input symbol in the specified range.
       *
       * \param origin The ID of the state from which the new edges
       * should originate.
       *
       * \param target The ID of the state to which the new edges
       * should lead.
       *
       * \param range A new edge will be created for each input symbol
       * in this range.
       */
      void addEdgeRange(StateId origin, StateId target, CharRange range);

      /**
       * Add a sentinel edge between two states. That is an edge whose
       * associated symbol is a sentinel symbol. A sentinel symbol is
       * a symbol that can be used to model regular expression
       * anchors, but can never occur in the input.
       *
       * \param origin The ID of the state from which the new edge
       * should originate.
       *
       * \param target The ID of the state to which the new edge
       * should lead.
       *
       * \param s The sentinel symbol to associate with the new edge.
       */
      void addSentinelEdge(StateId origin, StateId target, Sentinel s);

      /**
       * Remove all states and clear the start state registry.
       */
      void clear()
      {
	startStates.clear();
	states.clear();
      }


      typename Matcher::Ref getMatcher(typename StreamType::RefArg) const;
      TokenId match(StringType const &, StateId) const;

      SizeType getNumberOfStates() const { return states.size(); }
      SizeType getStartStateRegistrySize() const { return startStates.size(); }
      StateId getStartState(SizeType index) const { return startStates[index]; }

    private:
      typedef StateId GroupId;
      typedef std::vector<GroupId> Partition;
      struct PartitionGroupCompare;

      /**
       * Test whether two states are equivalent according to the
       * specified partition.
       *
       * This is the case iff for each possible input symbol and each
       * possible sentinel symbol the two states have transitions to
       * the same group in the current partition.
       *
       * The two compared states must always reside in the same group
       * of the specified partition. This restriction is needed to
       * propperly handle the implicit sentinel transistions.
       *
       * The virtual dead state must always be represented at the last
       * index in the specified partition map. This is needed to
       * propperly handle the implicit standard transistions.
       */
      bool testEquivalence(StateId, StateId, Partition const &partition) const;


      struct SentinelEdgeRep
      {
	Sentinel sentinel;
	StateId targetState;
	SentinelEdgeRep(Sentinel sentinel, StateId targetState):
	  sentinel(sentinel), targetState(targetState) {}
      };

      struct StateRep
      {
        /**
         * Must be equal to \c TraitsType::noToken() for any
         * non-accepting state.
         */
	TokenId tokenId;

 	Util::RangeMap<CharType, StateId> edgeRanges;

	std::map<Sentinel, StateId> sentinelEdges;

	StateRep(TokenId tokenId = TraitsType::noToken()): tokenId(tokenId) {}
      };

      core::std::vector<StateRep *> states;
      std::vector<StateId> startStates; // State indices

      typedef typename Util::RangeMap::RangeSeq::IterType           EdgeRangeRepIter;
      typedef typename std::map<Sentinel, StateId>::const_iterator  SentinelEdgeRepIter;
      typedef typename std::vector<StateRep *>::const_iterator      StateRepIter;

      struct StateAdder;

    public:
      struct EdgeRangeProxy: core::ProxyBase<EdgeRangeProxy, EdgeRangeRepIter>
      {
        CharRange getRange()       const { return this->i->range; }
        StateId   getTargetState() const { return this->i->targetState; }

        EdgeRangeProxy(EdgeRangeRepIter i):
          core::ProxyBase<EdgeRangeProxy, EdgeRangeRepIter>(i) {}
      };

      struct SentinelEdgeProxy: core::ProxyBase<SentinelEdgeProxy, SentinelEdgeRepIter>
      {
        Sentinel getSentinel()    const { return this->i->sentinel; }
        StateId  getTargetState() const { return this->i->targetState; }

        SentinelEdgeProxy(SentinelEdgeRepIter i):
          core::ProxyBase<SentinelEdgeProxy, SentinelEdgeRepIter>(i) {}
      };

      typedef core::IterSeq<core::ProxyIter<EdgeRangeProxy> >    EdgeRangeSeq;
      typedef core::IterSeq<core::ProxyIter<SentinelEdgeProxy> > SentinelEdgeSeq;

      struct StateProxy: core::ProxyBase<StateProxy, StateRepIter>
      {
        StateId getId()      const { return s; }
        TokenId getTokenId() const { return this->i->tokenId; }

        EdgeRangeSeq getEdgeRanges() const
        {
          return EdgeRangeSeq(EdgeRangeProxy(this->i->edgeRanges.begin()),
                              EdgeRangeProxy(this->i->edgeRanges.end()));
        }

        SentinelEdgeSeq getSentinelEdges() const
        {
          return SentinelEdgeSeq(SentinelEdgeProxy(this->i->sentinelEdges.begin()),
                                 SentinelEdgeProxy(this->i->sentinelEdges.end()));
        }

        void next()
        {
          core::ProxyBase<StateProxy, StateRepIter>::next();
          ++s;
        }

        StateProxy(StateRepIter i, StateId s):
          core::ProxyBase<StateProxy, StateRepIter>(i), s(s) {}

      private:
        StateId s;
      };

      typedef core::ProxyIter<StateProxy> StateIter;
      typedef core::IterSeq<StateIter>    StateSeq;

      StateIter getState(StateId s) const
      {
        return StateIter(StateProxy(states.begin()+s, s));
      }

      StateSeq getStates() const
      {
        return StateSeq(getState(0), getState(states.size()));
      }
    };


    typedef BasicDfa<char>    Dfa;
    typedef BasicDfa<wchar_t> WideDfa;




    // Template implementations:


    template<typename Ch, typename Tok, typename Tr>
    typename BasicDfa<Ch, Tok, Tr>::StateId BasicDfa<Ch, Tok, Tr>::addState(TokenId tokenId)
    {
      states.push_back(StateRep(tokenId));
      return states.size()-1;
    }


    template<typename Ch, typename Tok, typename Tr>
    typename BasicDfa<Ch, Tok, Tr>::SizeType BasicDfa<Ch, Tok, Tr>::registerStartState(StateId s)
    {
      startStates.push_back(s);
      return startStates.size()-1;
    }


    template<typename Ch, typename Tok, typename Tr>
    void BasicDfa<Ch, Tok, Tr>::addEdgeRange(StateId origin, StateId target, CharRange range)
    {
      states[origin].edgeRanges.assign(range.first, range.second, target);
    }


    template<typename Ch, typename Tok, typename Tr>
    void BasicDfa<Ch, Tok, Tr>::addSentinelEdge(StateId origin, StateId target, Sentinel sentinel)
    {
      states[origin].sentinelEdges[sentinel] = target;
    }


    template<typename Ch, typename Tok, typename Tr>
    typename BasicDfa<Ch, Tok, Tr>::Matcher::Ref BasicDfa<Ch, Tok, Tr>::getMatcher(typename StreamType::RefArg) const
    {
      throw std::runtime_error("Not implemented");
    }


    template<typename Ch, typename Tok, typename Tr>
    typename BasicDfa<Ch, Tok, Tr>::TokenId BasicDfa<Ch, Tok, Tr>::match(StringType const &, StateId) const
    {
      throw std::runtime_error("Not implemented");
    }


    template<typename Ch, typename Tok, typename Tr>
    struct BasicDfa<Ch, Tok, Tr>::PartitionGroupCompare: std::binary_function<StateId, StateId, bool>
    {
      bool operator()(StateId a, StateId b) const
      {
std::cerr << "PartitionGroupCompare: " <<partition[a]<<" ["<<a<<"]"<<" == "<<partition[b]<<" ["<<b<<"]"<< endl;
        return partition[a] == partition[b];
      }
      CompareSentinelEdges(Partition const &p): partition(p) {}
      Partition const &partition;
    };

    template<typename Ch, typename Tok, typename Tr>
    bool BasicDfa<Ch, Tok, Tr>::testEquivalence(StateId s1, StateId s2, Partition const &partition) const
    {
      // A missing standard transition on one state is to be
      // considered a transition to the virtual dead state which is
      // guaranteed to be positioned as the last element in the
      // partition map. So when comparing edge maps below we use the
      // last index in the partition map as default value for group
      // comparison.

      // If neither of the two states have explicit standard
      // transitions on a particular symbol, that symbol will not be
      // checked for in the comparison below, but that is ok since
      // both have implicit transitions to the virtual dead state.

      // A missing sentinel transition on one state is to be
      // considered a transition to itself. So when comparing sentinel
      // edges below we use one of the incoming state IDs as default
      // value for group comparison. It does not matter which one,
      // since both states are guaranteed to be in the same group.

      // If neither of the two states have explicit transitions on a
      // particular sentinel symbol, that symbol will not be checked
      // for in the comparison below, but that is ok since both have
      // implicit transitions to themselves, and since the two states
      // come from the same group they will have transitions to the
      // same group on that symbol.

      State const &t1 = states[s1];
      State const &t2 = states[s2];
      PartitionGroupCompare cmp(partition);
      return t1.edgeRanges.compare(t2.edgeRanges, partition.size()-1u, cmp) &&
        Util::compare_maps(t1.sentinelEdges, t2.sentinelEdges, s1, cmp);
    }


      /**
       * Construct the DFA with the minimal number of states that
       * accepts exactly the same language as the DFA passed as
       * argument.
       *
       * The new DFA will never have more start states than the
       * old one and if the old one has at least one start state then so has
       * the new. In particular, if the old DFA has one start state then the
       * new DFA gets one start state.
       *
       * The start states will always be the first states in the new
       * DFA.
       *
       * \param a The deterministic automaton to be minimized.
       */
    /**
     * The minimization algorithm works by iteratively refining a
     * partition of states. At iteration N each group in the partition
     * contains states that cannot be distinguished by strings of
     * length less than N. Since we can at most end up with one group
     * for each original state, the number of iterations is bounded by
     * the number of original states.
     *
     * In its simplest form this algorithm assumes that all states of
     * the DFA has transitions on all symbols, as is generally
     * required for a DFA. However, that is not generally the case for
     * this DFA implementation. The reason is that there is a lot of
     * time and space to be saved by dropping this requirement and
     * simply assuming that whenever a transition is missing, it is to
     * be understood as a transition to a "virtual dead state". That
     * is, a non-accepting state where all transistions lead to the
     * same dead state.
     *
     * In the implementation below, this "virtual dead state",
     * although not physically present, is always assumed to be part
     * of the first group of the partition. It is important to keep
     * this in mind when trying to understand this implementation.
     */
    template<typename Ch, typename Tok, typename Tr>
    void BasicDfa<Ch, Tok, Tr>::minimize() const
    {
      /*
       * Maps states of the source DFA to the ID of the group to which
       * they belong under the current partition.
       *
       * To prevent excessive copying and allocation we swap between
       * two partitions. One will hold the current partition while the
       * other will be used to build the new partition.
       *
       * Make room for the virtual dead state at the end of the
       * partitions.
       */
      Parition partition1(states.size()+1u), partition2(states.size()+1u);

      // Construct the initial partition with a group of non-accepting
      // states and one group of accepting states for each distinct
      // final value.
      {
        std::map<FinalValue, GroupId> m;
	// Force the group of non-accepting states to have ID = 0.
	m[TraitsType::noToken()] = 0;
	for(StateId i=0; i<states.size(); ++i)
	{
	  int const f = states[i].finalValue;
	  partition1[i] = f == nonFinal ? 0 :
	    m.insert(make_pair(f, m.size())).first->second;
	}
      }

      bool again;
      bool even = true;
      do
      {
        Partition const &partition    = even ? partition1 : partition2;
        Partition       &newPartition = even ? partition2 : partition1;
	even = !even;
	again = false;

	// Generate a mapping from group ID to the list of states in
	// that group
	typedef std::list<StateId>        States;
	typedef std::map<GroupId, States> Groups;
	Groups groups;
	// FIXME: We could as well construct these, one group at a time inside the subdivide loop
	for(StateId i=0; i<partition.size(); ++i)
	  groups[partition[i]].push_back(i);

	// Start at 1 since the first group (index 0) is always the
	// dead one (the one containing the virtual dead state)
	int newGroupId = 1;

	// Subdivide each group
	for(typename Groups::iterator i=groups.begin(); i!=groups.end(); ++i)
	{
	  States &g = i->second;

	  // Compare all physical states in group 0 with the virtual
	  // dead state which is assumed to also be in group 0
	  if(i->first == 0)
	  {
	    States::iterator j1 = g.begin();
	    while(j1!=g.end())
	    {
	      States::iterator const j2 = j1;
	      ++j1; // Must advance iterator before erasing current item
	      vector<Edge> const &e = states[*j2].edges;
	      vector<Edge>::const_iterator k=e.begin();
	      while(k!=e.end())
	      {
		if(partition[k->targetState]) break;
		++k;
	      }
	      if(k!=e.end())
	      {
		again = true;
		continue;
	      }

	      // This state stays in first group since it cannot yet
	      // be distinguished from the virtual dead state.
              newPartition[*j2] = 0;
	      g.erase(j2);
	    }
	  }

          // While there is at least one state remaining in the
          // current group, extract it and any remaining state that is
          // equivalent to it and create a new group consisting of the
          // extracted states.
	  while(!g.empty())
	  {
	    StateId s = g.front();
	    g.pop_front();

	    States::iterator j1 = g.begin();
	    while(j1!=g.end())
	    {
	      States::iterator const j2 = j1;
	      ++j1; // Must advance iterator before erasing current item
	      if(!testEquivalence(s, *j2, partition))
	      {
		again = true;
		continue;
	      }

	      newPartition[*j2] = newGroupId;
	      g.erase(j2);
	    }

	    newPartition[s] = newGroupId;
	    ++newGroupId;
	  }
	}
      }
      while(again);

      // Construct the new minimized DFA from the reachable partitions
      // of the old one.
      vector<GroupId> const &partition = even ? partition1 : partition2;
      {
	map<GroupId, StateId> newStateMap; // Maps group ID to new state ID
	stack<StateId> uncheckedStates;

        // FIXME: Assuming that we allow a DFA to have zero start
        // states, we should only add a tart state below if it is not
        // in group zero. A start state in group zero is a dead start
        // state in that it has no transition path to a final state.

	// Add a new start state for each unique start group
	for(size_t i=0; i<startStates.size(); ++i)
	{
	  StateId const s = startStates[i];
	  if(newStateMap.insert(make_pair(partition[s], target.states.size())).second)
	  {
	    target.startStates.push_back(target.states.size());
	    target.states.push_back(State(states[s].finalValue));
	    uncheckedStates.push(s);
	  }
	}

	while(uncheckedStates.size())
	{
          // Fetch the representative state for this new group
	  StateId const s = uncheckedStates.top();
	  uncheckedStates.pop();

	  StateId const n = newStateMap[partition[s]];
	  vector<Edge> const &v = states[s].edges;
	  for(vector<Edge>::const_iterator e=v.begin(); e<v.end(); ++e)
	  {
	    GroupId const g = partition[e->targetState];
	    // Dont ever jump to group 0 since at this point it
	    // contains only dead states, ie. states from which you
	    // can never reach an accepting state.
	    if(!g) continue;
	    pair<map<GroupId, StateId>::iterator, bool> r =
	      newStateMap.insert(make_pair(g, target.states.size()));
	    if(r.second)
	    {
	      target.states.push_back(State(states[e->targetState].finalValue));
	      uncheckedStates.push(e->targetState);
	    }
	    // Edge ranges on new states are added from just one
	    // original state (the representative) so we will never
	    // get into trouble with incorrect range ordering. However
	    // since the target states are partially collapsed we
	    // might need to merge adjacent ranges.
	    vector<Edge> &w = target.states[n].edges;
	    if(w.size() && w.back().range.second == e->range.first-1 &&
	       w.back().targetState == r.first->second) w.back().range.second = e->range.second;
	    else w.push_back(Edge(e->range, r.first->second));
	  }
	}
      }
    }
  }
}

#endif // ARCHON_PARSER_DFA_HPP
