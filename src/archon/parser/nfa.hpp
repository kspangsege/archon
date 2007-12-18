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

#ifndef ARCHON_PARSER_NFA_HPP
#define ARCHON_PARSER_NFA_HPP

#include <cwchar>
#include <stdexcept>
#include <utility>
#include <vector>
#include <stack>

#include <archon/core/refcnt.hpp>
#include <archon/core/proxy_iter.hpp>
#include <archon/core/iseq.hpp>

#include <archon/parser/nfa_base.hpp>

namespace Archon
{
  namespace Parser
  {
    /**
     * A generic implementation of the abstract NFA API. It uses
     * tables to encode the transition function and allows it to be
     * modified in various ways.
     *
     * \sa NfaBase
     */
    template<typename Ch, typename Tok = short unsigned, typename Tr = FsaTraits<Ch, Tok> >
    struct BasicNfa: NfaBase<Ch, Tok, Tr>
    {
      typedef NfaBase<Ch, Tok, Tr> _Super;
      typedef typename _Super::TraitsType  TraitsType;
      typedef typename _Super::CharType    CharType;
      typedef typename _Super::StringType  StringType;
      typedef typename _Super::StreamType  StreamType;
      typedef typename _Super::SizeType    SizeType;
      typedef typename _Super::Sentinel    Sentinel;
      typedef typename _Super::TokenId     TokenId;
      typedef typename _Super::StateId     StateId;
      typedef typename _Super::StateSet    StateSet;
      typedef typename _Super::EdgeMap     EdgeMap;
      typedef typename _Super::Matcher     Matcher;
      typedef std::pair<StateId, StateId>  StatePair;

      typedef Core::CntRef<BasicNfa>       Ref;
      typedef Core::CntRef<BasicNfa const> ConstRef;
      typedef Ref const                   &RefArg;
      typedef ConstRef const              &ConstRefArg;

      /**
       * Specifies a range of symbols. The range begins with \c
       * CharRange.first and ends with \c CharRange.second and
       * includes both.
       */
      typedef std::pair<CharType, CharType> CharRange;

      /**
       * Make an empty NFA. That is, a NFA with no states at
       * all. Since such an NFA has no starts states, it accepts
       * nothing.
       */
      BasicNfa() {}

      /**
       * Add a new state to the NFA.
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
       * Add an epsilon edge between two states. An epsilon edge is an
       * edge that may be followed without consuming any input symbol.
       *
       * \param origin The ID of the state from which the new edge
       * should originate.
       *
       * \param target The ID of the state to which the new edge
       * should lead.
       */
      void addEpsilonEdge(StateId origin, StateId target);


      /**
       * Remove all states and clear the start state registry.
       */
      void clear()
      {
	startStates.clear();
	states.clear();
      }


      /**
       * Combine the two specified NFA fragments such that the
       * resulting fragment recognizes precisely the union of the
       * languages recognized by the two specified fragments.
       *
       * \return A pair (start, stop) of state IDs which are the
       * assumed start and stop states of the resulting fragment.
       *
       * \note The specified fragments will become part of the
       * returned fragment.
       */
      StatePair alternFragments(StatePair f1, StatePair f2);

      /**
       * Combine the two specified NFA fragments such that the
       * resulting fragment recognizes precisely the concattenation of
       * the languages recognized by the two specified fragments.
       *
       * \return A pair (start, stop) of state IDs which are the
       * assumed start and stop states of the resulting fragment.
       *
       * \note The specified fragments will become part of the
       * returned fragment.
       */
      StatePair concatFragments(StatePair f1, StatePair f2);

      /**
       * Construct the positive closure of the specified fragment. The
       * language recognized by the resulting fragment is precisely \c
       * L+ where \c L is the language recognized by the specified
       * fragment, and \c L+ is the positive closure of \c L which is
       * equal to \c{U{ L^n | 1<=n }}.
       *
       * \return A pair (start, stop) of state IDs which are the
       * assumed start and stop states of the resulting fragment.
       *
       * \note The specified fragment will become part of the returned
       * fragment.
       */
      StatePair repeatFragment(StatePair f);

      /**
       * Construct a fragment that recognizes the empty string and
       * anything \c f does. Formally this is \c{L U {e}} where L is
       * the language recognized by the specified fragment.
       *
       * \return A pair (start, stop) of state IDs which are the
       * assumed start and stop states of the resulting fragment.
       *
       * \note The specified fragment will become part of the returned
       * fragment.
       */
      StatePair optionalFragment(StatePair f);

      /**
       * Construct an NFA fragment that recognizes the language
       * consisting of the specified string and nothing else. This
       * string may be empty.
       *
       * \param s The string that the fragment should recognize.
       *
       * \return A pair (start, stop) of state IDs which are the
       * assumed start and stop states of the resulting fragment.
       */
      StatePair stringFragment(StringType s);

      /**
       * Construct an NFA fragment whose recognized language is
       * precisely the strings of length one composed of a symbol that
       * falls in one of the specified ranges. If the specified range
       * sequence is empty, the language recognized by the resulting
       * fragment will be empty.
       *
       * \param begin An iterator that points to the first symbol
       * range. The iterator must resolve to an object of type
       * \c CharRange.
       *
       * \param end An iterator that denotes the end of the symbol
       * range sequence.
       *
       * \return A pair (start, stop) of state IDs which are the
       * assumed start and stop states of the resulting fragment.
       */
      template<typename Iter> StatePair rangesFragment(Iter begin, Iter end);

      /**
       * Construct an NFA fragment whose recognized language is
       * precisely the empty string, but only when the input state is
       * in a state that satisfies the condition assiciated with the
       * specified sentinel symbol.
       *
       * \param s Any of the predefined sentinel symbols.
       *
       * \return A pair (start, stop) of state IDs which are the
       * assumed start and stop states of the resulting fragment.
       */
      StatePair sentinelFragment(Sentinel s);


      typename Matcher::Ref getMatcher(typename StreamType::RefArg) const;
      TokenId match(StringType const &, StateId) const;

      SizeType getNumberOfStates() const { return states.size(); }
      SizeType getStartStateRegistrySize() const { return startStates.size(); }
      StateId getStartState(SizeType index) const { return startStates[index]; }

      bool closedAdd(StateId, StateSet &) const;
      void initEdgeMap(StateSet const &, EdgeMap &) const;
      TokenId chooseTokenId(StateSet const &) const;

    private:
      struct EdgeRangeRep
      {
	CharRange range;
	StateId targetState;
	EdgeRangeRep(CharRange range, StateId targetState):
	  range(range), targetState(targetState) {}
      };

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

        /**
         * We do not care about order or redundancy here. A vector was
         * chosen since it allows fast addition of new edges.
         */
	std::vector<EdgeRangeRep> edgeRanges;

        /**
         * We do not care about order or redundancy here. A vector was
         * chosen since it allows fast addition of new edges.
         */
	std::vector<SentinelEdgeRep> sentinelEdges;

        /**
         * We do not care about order or redundancy here. A vector was
         * chosen since it allows fast addition of new edges.
         */
	std::vector<StateId> epsilonEdges;

	StateRep(TokenId tokenId = TraitsType::noToken()): tokenId(tokenId) {}
      };

      std::vector<StateRep> states;
      std::vector<StateId> startStates; // State indices

      typedef typename std::vector<EdgeRangeRep>::const_iterator    EdgeRangeRepIter;
      typedef typename std::vector<SentinelEdgeRep>::const_iterator SentinelEdgeRepIter;
      typedef typename std::vector<StateId>::const_iterator         EpsilonEdgeRepIter;
      typedef typename std::vector<StateRep>::const_iterator        StateRepIter;

      struct StateAdder;

    public:
      struct EdgeRangeProxy: Core::ProxyBase<EdgeRangeProxy, EdgeRangeRepIter>
      {
        CharRange getRange()       const { return this->i->range; }
        StateId   getTargetState() const { return this->i->targetState; }

        EdgeRangeProxy(EdgeRangeRepIter i):
          Core::ProxyBase<EdgeRangeProxy, EdgeRangeRepIter>(i) {}
      };

      struct SentinelEdgeProxy: Core::ProxyBase<SentinelEdgeProxy, SentinelEdgeRepIter>
      {
        Sentinel getSentinel()    const { return this->i->sentinel; }
        StateId  getTargetState() const { return this->i->targetState; }

        SentinelEdgeProxy(SentinelEdgeRepIter i):
          Core::ProxyBase<SentinelEdgeProxy, SentinelEdgeRepIter>(i) {}
      };

      struct EpsilonEdgeProxy: Core::ProxyBase<EpsilonEdgeProxy, EpsilonEdgeRepIter>
      {
        StateId getTargetState() const { return *this->i; }

        EpsilonEdgeProxy(EpsilonEdgeRepIter i):
          Core::ProxyBase<EpsilonEdgeProxy, EpsilonEdgeRepIter>(i) {}
      };

      typedef Core::IterSeq<Core::ProxyIter<EdgeRangeProxy> >    EdgeRangeSeq;
      typedef Core::IterSeq<Core::ProxyIter<SentinelEdgeProxy> > SentinelEdgeSeq;
      typedef Core::IterSeq<Core::ProxyIter<EpsilonEdgeProxy> >  EpsilonEdgeSeq;

      struct StateProxy: Core::ProxyBase<StateProxy, StateRepIter>
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

        EpsilonEdgeSeq getEpsilonEdges() const
        {
          return EpsilonEdgeSeq(EpsilonEdgeProxy(this->i->epsilonEdges.begin()),
                                EpsilonEdgeProxy(this->i->epsilonEdges.end()));
        }

        void next()
        {
          Core::ProxyBase<StateProxy, StateRepIter>::next();
          ++s;
        }

        StateProxy(StateRepIter i, StateId s):
          Core::ProxyBase<StateProxy, StateRepIter>(i), s(s) {}

      private:
        StateId s;
      };

      typedef Core::ProxyIter<StateProxy> StateIter;
      typedef Core::IterSeq<StateIter>    StateSeq;

      StateIter getState(StateId s) const
      {
        return StateIter(StateProxy(states.begin()+s, s));
      }

      StateSeq getStates() const
      {
        return StateSeq(getState(0), getState(states.size()));
      }
    };


    typedef BasicNfa<char>    Nfa;
    typedef BasicNfa<wchar_t> WideNfa;




    // Template implementations:


    template<typename Ch, typename Tok, typename Tr>
    typename BasicNfa<Ch, Tok, Tr>::StateId BasicNfa<Ch, Tok, Tr>::addState(TokenId tokenId)
    {
      states.push_back(StateRep(tokenId));
      return states.size()-1;
    }


    template<typename Ch, typename Tok, typename Tr>
    typename BasicNfa<Ch, Tok, Tr>::SizeType BasicNfa<Ch, Tok, Tr>::registerStartState(StateId s)
    {
      startStates.push_back(s);
      return startStates.size()-1;
    }


    template<typename Ch, typename Tok, typename Tr>
    void BasicNfa<Ch, Tok, Tr>::addEdgeRange(StateId origin, StateId target, CharRange range)
    {
      if(range.second < range.first) throw std::invalid_argument("Illegal range");
      states[origin].edgeRanges.push_back(EdgeRangeRep(range, target));
    }


    template<typename Ch, typename Tok, typename Tr>
    void BasicNfa<Ch, Tok, Tr>::addSentinelEdge(StateId origin, StateId target, Sentinel sentinel)
    {
      states[origin].sentinelEdges.push_back(SentinelEdgeRep(sentinel, target));
    }


    template<typename Ch, typename Tok, typename Tr>
    void BasicNfa<Ch, Tok, Tr>::addEpsilonEdge(StateId origin, StateId target)
    {
      states[origin].epsilonEdges.push_back(target);
    }


    template<typename Ch, typename Tok, typename Tr>
    typename BasicNfa<Ch, Tok, Tr>::StatePair BasicNfa<Ch, Tok, Tr>::alternFragments(StatePair f1, StatePair f2)
    {
      StateId t = addState(), u = addState();
      addEpsilonEdge(t, f1.first);
      addEpsilonEdge(t, f2.first);
      addEpsilonEdge(f1.second, u);
      addEpsilonEdge(f2.second, u);
      return StatePair(t, u);
    }


    template<typename Ch, typename Tok, typename Tr>
    typename BasicNfa<Ch, Tok, Tr>::StatePair BasicNfa<Ch, Tok, Tr>::concatFragments(StatePair f1, StatePair f2)
    {
      addEpsilonEdge(f1.second, f2.first);
      return StatePair(f1.first, f2.second);
    }


    template<typename Ch, typename Tok, typename Tr>
    typename BasicNfa<Ch, Tok, Tr>::StatePair BasicNfa<Ch, Tok, Tr>::repeatFragment(StatePair f)
    {
      addEpsilonEdge(f.second, f.first);
      return f;
    }


    template<typename Ch, typename Tok, typename Tr>
    typename BasicNfa<Ch, Tok, Tr>::StatePair BasicNfa<Ch, Tok, Tr>::optionalFragment(StatePair f)
    {
      StateId t = addState(), u = addState();
      addEpsilonEdge(t, f.first);
      addEpsilonEdge(f.second, u);
      addEpsilonEdge(t, u);
      return StatePair(t, u);
    }


    template<typename Ch, typename Tok, typename Tr>
    typename BasicNfa<Ch, Tok, Tr>::StatePair BasicNfa<Ch, Tok, Tr>::stringFragment(StringType s)
    {
      StateId t = addState(), u = t;
      for(typename StringType::size_type i=0; i<s.size(); ++i)
      {
        StateId v = addState();
        addEdge(u, v, s[i]);
        u = v;
      }
      return StatePair(t, u);
    }


    template<typename Ch, typename Tok, typename Tr> template<typename Iter>
    typename BasicNfa<Ch, Tok, Tr>::StatePair BasicNfa<Ch, Tok, Tr>::rangesFragment(Iter begin, Iter end)
    {
      StateId t = addState(), u = addState();
      for(Iter i = begin; i!=end; ++i) addEdgeRange(t, u, CharRange(i->first, i->second));
      return StatePair(t, u);
    }


    template<typename Ch, typename Tok, typename Tr>
    typename BasicNfa<Ch, Tok, Tr>::StatePair BasicNfa<Ch, Tok, Tr>::sentinelFragment(Sentinel s)
    {
      StateId t = addState(), u = addState();
      addSentinelEdge(t, u, s);
      return StatePair(t, u);
    }


    template<typename Ch, typename Tok, typename Tr>
    typename BasicNfa<Ch, Tok, Tr>::Matcher::Ref BasicNfa<Ch, Tok, Tr>::getMatcher(typename StreamType::RefArg) const
    {
      throw std::runtime_error("Not implemented");
    }


    template<typename Ch, typename Tok, typename Tr>
    typename BasicNfa<Ch, Tok, Tr>::TokenId BasicNfa<Ch, Tok, Tr>::match(StringType const &, StateId) const
    {
      throw std::runtime_error("Not implemented");
    }


    template<typename Ch, typename Tok, typename Tr>
    bool BasicNfa<Ch, Tok, Tr>::closedAdd(StateId state, StateSet &stateSet) const
    {
      if(!stateSet.insert(state).second) return false;
      std::stack<StateId> uncheckedStates;
      uncheckedStates.push(state);
      while(!uncheckedStates.empty())
      {
	StateRep const &s = states[uncheckedStates.top()];
	uncheckedStates.pop();
	for(typename std::vector<StateId>::const_iterator i=s.epsilonEdges.begin(); i!=s.epsilonEdges.end(); ++i)
	  if(stateSet.insert(*i).second) uncheckedStates.push(*i);
      }
      return true;
    }


    template<typename Ch, typename Tok, typename Tr>
    typename BasicNfa<Ch, Tok, Tr>::TokenId BasicNfa<Ch, Tok, Tr>::chooseTokenId(StateSet const &stateSet) const
    {
      TokenId rule = TraitsType::noToken();
      for(typename StateSet::iterator i=stateSet.begin(); i!=stateSet.end(); ++i)
      {
	TokenId r = states[*i].tokenId;
	if(r != TraitsType::noToken() && (rule == TraitsType::noToken() || rule < r)) rule = r;
      }
      return rule;
    }


    template<typename Ch, typename Tok, typename Tr>
    struct BasicNfa<Ch, Tok, Tr>::StateAdder
    {
      bool operator()(StateSet &s) { return nfa->closedAdd(state, s); }
      StateAdder(BasicNfa const *n, StateId s): nfa(n), state(s) {}
      BasicNfa const *nfa;
      StateId state;
    };

    template<typename Ch, typename Tok, typename Tr>
    void BasicNfa<Ch, Tok, Tr>::initEdgeMap(StateSet const &s, EdgeMap &m) const
    {
      for(typename StateSet::iterator i=s.begin(); i!=s.end(); ++i)
      {
	StateRep const &s = states[*i];
	for(typename std::vector<EdgeRangeRep>::const_iterator j=s.edgeRanges.begin(); j!=s.edgeRanges.end(); ++j)
	  m.ranges.update(j->range.first, j->range.second, StateAdder(this, j->targetState));
	for(typename std::vector<SentinelEdgeRep>::const_iterator j=s.sentinelEdges.begin(); j!=s.sentinelEdges.end(); ++j)
	  closedAdd(j->targetState, m.sentinels[j->sentinel]);
      }
    }
  }
}

#endif // ARCHON_PARSER_NFA_HPP
