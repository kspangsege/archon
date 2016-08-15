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

#ifndef ARCHON_PARSER_FSA_HPP
#define ARCHON_PARSER_FSA_HPP

#include <cwchar>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <stack>
#include <map>
#include <set>

#include <archon/core/proxy_iter.hpp>
#include <archon/core/iseq.hpp>
#include <archon/core/stream.hpp>
#include <archon/util/range_map.hpp>

namespace archon
{
  namespace Parser
  {
    template<typename Ch, typename Tok = short unsigned> struct FsaTraits
    {
      typedef Ch                               CharType;
      typedef std::basic_string<CharType>      StringType;
      typedef core::BasicInputStream<CharType> StreamType;
      typedef size_t                           SizeType;
      typedef Tok                              TokenId;
      typedef size_t                           StateId;

      /**
       * Retrieve the default token ID. Every state has an associated
       * token ID. This function returns the ID used by default in
       * accepting states. This is usefull when an application does
       * not need to distinguish between different types of matches.
       *
       * \return The default token ID used in accepting states.
       */
      static TokenId defaultToken() { return std::numeric_limits<TokenId>::min(); }

      /**
       * Retrieve the special token ID used in non-accepting states.
       *
       * Every state has an associated token ID. Only those whose
       * token ID is not equal to the value returned by this function
       * is an accepting state.
       *
       * \return The special token ID used in non-accepting states.
       */
      static TokenId noToken() { return std::numeric_limits<TokenId>::max(); }

      /**
       * Retrieve the special state ID used to indicate the lack of a
       * state.
       *
       * \return The special token ID used in non-accepting states.
       */
      static TokenId noState() { return std::numeric_limits<StateId>::max(); }
    };

    /**
     * An implementation of an arbitrary finite state
     * machine/automaton (FSA/FSM). In general this will correspond to
     * an nondeterministic finite automaton (NFA), but it can at any
     * time be converted into a deterministic finite automaton (FSA).
     *
     * This implementation offers a number of operations for modifying
     * the state machine, such as converting it to a DFA and
     * minimizing the number of states.
     *
     * The definition allows the following extensions compared to the
     * conventional definition:
     *
     * - Each accepting state may be endowed with a unique token
     * ID. Among other things this is useful when building lexical
     * analyzers.
     *
     * - A state machine may contain any number of start states. It
     * may even have no start states at all. These start states are
     * not to be understood as equivalent, as is the case in some
     * definitions of an NFA. In general two different start states
     * correspond to two different sets of accepted strings.
     *
     * - To allow modeling regular expression anchors this automaton
     * supports the special notion of a "sentinel symbol", and a
     * "sentinel edge" is a transition on a sentinel symbol.
     *
     * For an introduction to the conventional definition of a finite
     * state machine, please see for example "Compilers: Principles,
     * Techniques and Tools" by Aho/Sethi/Ullman (the dragon book.)
     *
     *
     * <H3>Token IDs<H3>
     *
     * Each state is either implicitly or explicitly associated with a
     * token ID. Only states whose token ID is not equal to \c
     * TraitsType::noToken() is an accepting state. In a
     * non-deterministic state machine there might be multiple
     * accepting states that are reachable on a specific input
     * string. To be able to report a deterministic token ID on a
     * match in such a case we shall always assume that the token ID
     * with the highest numerical value takes precedence.
     *
     *
     * <H3>Multiple start states:</H3>
     *
     * We allow for a state machine to have any number of start states
     * (including zero.) This is usefull for example if a lexer needs
     * to be context sensitive, that is, if it sometimes need to
     * extract the next token based on a special set of patterns.
     *
     * Some NFA definitions allow multiple start states but assume
     * that they are equivalent and that we may choose any one of them
     * when checking for a match. Please note that this picture is not
     * in agreement with the difinition here. Here two different start
     * states should be unserstood basically as two different simpler
     * state machines.
     *
     * State minimization and other transforming operations present a
     * challenge when working with multiple start states, since we
     * could easily loose track of the correspondence between old and
     * new start states.
     *
     * To help us in this regard, our state machine shall support the
     * notion of a "start state registry". The start state registry
     * contains zero or more registrations. Each registration
     * associates a state ID with a start state registry index. A
     * state is a start state if there is at least one registration
     * that refers to it.
     *
     * In general the size of the registry is equal to the number of
     * start states, but due to the fact that two or more registry
     * entries can refer to the same state, the actual number of start
     * states may be less. Of course, if the registry has at least one
     * entry, then the state machine must have at least one start
     * state.
     *
     * It is the intentsion that any transforming operation on a state
     * machine shall maintain start state identities through the
     * registry. That is, the registry size must not change and the
     * logical function associated with a specific registry index must
     * remain at that index. The state IDs that each index resolves to
     * should generally be expected to change.
     *
     *
     * <H3>Sentinel symbols:</H3>
     *
     * This section explains how the concept of "sentinel symbols" is
     * used to handle regular expression anchors in state machines. It
     * also suggests how we might implement the state machines without
     * incurring too much space and time overhead.
     *
     * Since regular expression anchors do not correspond to actual
     * input symbols, it is not straight forward to handle them in an
     * NFA, and even harder to handle them in a DFA. The following is
     * an outline of one way to deal with the problem:
     *
     * We introduce the concept of a sentinel symbol, which is a
     * symbol that can never occur naturally in the input. We allocate
     * a unique sentinel symbol for each type of anchor. For example:
     * "beginning of line", "end of line", "beginning of word", "end
     * of word".
     *
     * We want to be able to handle these sentinel symbols both in an
     * NFA and in a DFA, so that we can follow the general scheme of
     * constructing an NFA from a regular expression, then a DFA from
     * the NFA, and finally a unique minimized DFA.
     *
     * We assume that in the simulation of the final DFA, sentinel
     * symbols are artificially injected into the input stream
     * whenever the corresponding anchoring condition is detected in
     * the input. For example, every time a new line begins, a
     * "beginning of line" sentinel symbol is inserted immediately
     * before the first symbol on that line, or every time there is a
     * transition from a "word" symbol to a non-word symbol in the
     * input, an "end of word" sentinel symbol is inserted between
     * those two proper symbols.
     *
     * However, in itself this is not good, because a state machine
     * corresponding to the regular expression "a " will now no longer
     * match the input "a " due to the injected "end of word" sentinel
     * symbol. To cope with this problem we shall require that every
     * state of an NFA has a transition to itself on every defined
     * sentinel symbol. We might choose to not express these edges
     * explicitly, however we shall always assume their presence.
     *
     * This way the NFA can always choose to ignore a sentinel
     * symbol. As long as the state machine contains no sentinel edges
     * between different states, this guarantees that the augmented
     * state machine accepts exactly the same language as the
     * conventional one. On the other hand, when the NFA contains a
     * sentinel edge from one state to another, then it may choose to
     * follow that edge rather than ignoring the sentinel symbol, but
     * of course only when that sentinel symbol is the next input
     * symbol.
     *
     * If two or more anchoring conditions are satisfied
     * simultaneously, we generate a sentinel for each of them in some
     * predefined order, yielding a "sentinel round". Also, to allow
     * for successive anchors such as in the regular expression "^^"
     * we must repeat a sentinel round enough times. For simplicity we
     * could say that we repeat sentinel rounds N times where N is the
     * number of states in the state machine since this is guaranteed
     * to always be enough.
     *
     * It should be noted that under this rule there is no way to
     * express that a state has no transition to itself on a
     * particular sentinel symbol. Fortunately we never need to
     * express such a condition. We should think of it this way: If a
     * certain anchor is satisfied at a certain position in the input,
     * then that anchor will remain satisfied until we consume the
     * next input symbol. Thus, it can never be a problem that an NFA
     * has a transistion to itself on a sentinel symbol.
     *
     * Note that with the requirement above there is no longer an NFA
     * that accepts every possible regular language when considering
     * the sentinels as a part of the alphabet. So although we can
     * convert any NFA with the sentinel requirement to an equivalent
     * DFA, we cannot convert every DFA to an NFA with the sentinel
     * requirement. One example is the following DFA:
     *
     * <PRE>
     *
     *           bol
     *   >(1) <------> ((2))
     *
     * </PRE>
     *
     * This one accepts an odd number of bol's. In an NFA with the
     * sentinel requirement we cannot express this, since if n bols
     * are acceptd then so is n+1.
     *
     *
     * <H3>How to implement it:</H3>
     *
     * If we did exactly as outlined above, it would lead to
     * tremendous overhead compared to the operation of a conventional
     * state machine. Fortunately there are several simplifications
     * that can be incorporated so the overhead becomes insignificant.
     *
     * First we assume that in an NFA every state has an implicit
     * transition to itself on all sentinel symbols. This way we only
     * need to add an explicit transitions on a sentinel symbol if it
     * takes us to a different state.
     *
     * It should be noted that under this rule there is no easy way to
     * express that a state has no transitions on a particular
     * sentinel symbol. Fortunately we never need to express such a
     * condition.
     *
     * In a fashion similar to the NFA, we assume that every state of
     * a DFA that has no explicit transition on a particular sentinel
     * symbol, contains an implicit transition to itself on that
     * sentinel symbol.
     *
     * When converting an NFA to a DFA we carry out the subset
     * construction. In this process we need to give special attention
     * to both implicit and explicit sentinel edges.
     *
     * Consider a subset of NFA states. If none of the states in this
     * subset has an explicit sentinel edge, then the implicit
     * sentinel edges would lead the DFA edge construction to produce
     * an edge on all sentinel symbols to a set of states identical to
     * the current subset. Since such an edge is implicit in the DFA,
     * we need not add it explicitly. What this means, it that the
     * conventional DFA edge construction will suffice for this case
     * since it produces the desired result.
     *
     * However, as soon as the current subset does contain a explicit
     * sentinel edge we must take special action. In such a case the
     * conventional DFA edge construction would produce an edge on the
     * sentinel symbol to some new state set. However, this state set
     * is not complete (in general), we must add to it all the states
     * of the current subset, since they are all possible targets in a
     * transition on that sentinel symbol due to the implicit sentinel
     * edges.
     *
     * Now, if the NFA sentinel edge was leading to a state not in the
     * current subset, then necessarily the new DFA sentinel edge will
     * lead to a subset that is different from the current subset, and
     * we must therefore add that edge explicitly.
     *
     * Consider the following NFA:
     *
     * <PRE>
     *
     *         --> (2) --
     *   bol /            \ a
     *      /              \
     *   >(1)                --> ((4))
     *      \              /
     *   bow \            / b
     *         --> (3) --
     *
     * </PRE>
     *
     * Where 1, 2, 3 and 4 are states, 1 is the start state, 4 is the
     * final state, 'a', 'b' are input symbols, 'bol' is the
     * "beginning of line" sentinel symbol and 'bow' is the "beginning
     * of word" sentinel symbol.
     *
     * The subset construction will work out as follows:
     *
     * <PRE>
     *
     *   (1):     bol -> (1,2),   bow -> (1,  3)
     *   (1,2):   bol -> (1,2),   bow -> (1,2,3), a -> (4)
     *   (1,3):   bol -> (1,2,3), bow -> (1,  3),           b -> (4)
     *   (1,2,3): bol -> (1,2,3), bow -> (1,2,3), a -> (4), b -> (4)
     *   (4):     bol -> (4),     bow -> (4)
     *
     * </PRE>
     *
     * This corresponds to the following DFA:
     *
     * <PRE>
     *
     *         --> (1,2) ------------------
     *   bol /        \                     \ a
     *      /      bow \               a,b   \
     *   >(1)            --> (1,2,3) ------------> ((4))
     *      \      bol /                     /
     *   bow \        /                     / b
     *         --> (1,3) ------------------
     *
     * </PRE>
     *
     * Note that sentinel edges from a state to itself are left out of
     * this diagram according to the rules described above.
     *
     * Now, adhering to the simulation rules also described above, let
     * us see how our DFA will react to the input "b":
     *
     * Since we are at the beginning of a line and also at the
     * beginning of a word we must inject a round of two sentinel
     * symbols. Since our NFA had 4 states, we must further repeat
     * that round 4 times yielding the following augmented input
     * stream and the corresponding DFA state transitions:
     *
     * <PRE>
     *
     *     bol   bow     bol     bow     bol     bow     bol     bow      b
     *
     *   (1) (1,2) (1,2,3) (1,2,3) (1,2,3) (1,2,3) (1,2,3) (1,2,3) (1,2,3) (4)
     *
     * </PRE>
     *
     * However, as you are probably thinking, this is totally
     * wasteful. We see that there is no need to insert a specific
     * sentinel symbol if the current state has no transition on it,
     * since it will simply be ignored. This leads to the following
     * simple rule for the DFA simulator:
     *
     * At any particular state determine whether that state has any
     * sentinel edges leaving it. If not, continue as normal. That is,
     * apply the next ordinary input symbol to the state machine.
     *
     * Otherwise determine the set of anchors whose condition is
     * currently satisfied and for which there is also a corresponding
     * sentinel edge leading from the current state.
     *
     * According to the predefined order of anchors, choose the first
     * one and apply it to the state machine. In fact we need not
     * choose the first one. Any one of them would be a valid choice,
     * but for the sake of determinism, we always choose the first one
     * that applies. Determinism is desirable since it might allow
     * further optimizations (see below for an example.)
     *
     * With these rules we would get the following sequence:
     *
     * <PRE>
     *
     *     bol   bow      b
     *
     *   (1) (1,2) (1,2,3) (4)
     *
     * </PRE>
     *
     * Note that if we had ordered 'bow' before 'bol' we would get a
     * shorter sequence. Unfortunately there is no easy way in which
     * we can choose the "right" first transition from state 1.
     *
     * Note also that the 'bol' edge from (1,3) to (1,2,3) in the DFA
     * above is a redundant edge, due to the fact that we always
     * choose 'bol' before 'bow' when both of them apply. However, it
     * is not obvious that removing such edges is worth the effort.
     *
     * Note further that repeated anchors of the same kind lead to
     * unnecessarily complex DFAs, so one might consider eliminating
     * the redundant ones in the NFA.
     *
     *
     * <H3>State minimization (unfinished documentation):</H3>
     *
     * If neither of the two states have explicit sentinel edges, then
     * they both have implicit sentinel edges to themselves, and since
     * the two states come from the same group they will be
     * transitions to the same group. Thus, if they appear equivalent
     * without regard to sentinel symbols, they are equivalent.
     *
     *
     *
     * \sa "Compilers: Principles, Techniques and Tools" by
     * Aho/Sethi/Ullman.
     *
     * \sa http://en.wikipedia.org/wiki/Finite_state_machine
     */
    template<typename Ch, typename Tok = short unsigned, typename Tr = FsaTraits<Ch, Tok> >
    struct BasicFsa
    {
      typedef Tr                              TraitsType;
      typedef typename TraitsType::CharType   CharType;
      typedef typename TraitsType::StringType StringType;
      typedef typename TraitsType::StreamType StreamType;
      typedef typename TraitsType::SizeType   SizeType;
      typedef typename TraitsType::TokenId    TokenId;
      typedef typename TraitsType::StateId    StateId;
      typedef std::set<StateId>               StateSet;
      typedef std::pair<StateId, StateId>     StatePair;

      /**
       * Specifies a range of symbols. The range begins with \c
       * CharRange.first and ends with \c CharRange.second and
       * includes both.
       */
      typedef std::pair<CharType, CharType> CharRange;

      enum Sentinel
      {
        anchor_bol,
        anchor_eol,
        anchor_bow,
        anchor_eow
      };

      /**
       * Make an empty state machine. That is, a state machine with no
       * states at all. Since such an state machine has no starts
       * states, it accepts nothing.
       */
      BasicFsa() {}

      /**
       * Get the number of states in this state machine.
       *
       * This includes both start states and accepting states.
       *
       * \return The number of states in this state machine.
       */
      SizeType getNumberOfStates() const { return states.size(); }

      /**
       * Get the number of start state registrations in this state
       * machine. This can be any number including zero. In general it
       * will be equal to the number of start states, but due to the
       * fact that two or more registry entries can refer to the same
       * state, the actual number of start states may be less. Of
       * course, if the registry has at least one entry, then the
       * state machine must have at least one start state.
       *
       * \return The number of start state registrations.
       */
      SizeType getStartStateRegistrySize() const { return startStates.size(); }

      /**
       * Get the start state corrsponding with the specified start
       * state registry index.
       *
       * \param index The start state registry index of the start
       * state whose ID you want. The largest valid index is one minus
       * the value returned by \c getStartStateRegistrySize().
       *
       * \return The state ID of the requested start state.
       *
       * \note It is completely valid for two start state indicies to
       * resolve to the same state ID. This, for example, could easily
       * be the case after state minimization where several states
       * might be collapsed into a single state.
       *
       * \note Different start states are not to be considered
       * equivalent like in some NFA definitions. A different start
       * state will in general lead to a different set of accepted
       * strings.
       *
       * \sa getStartStateRegistrySize
       */
      StateId getStartState(SizeType index) const { return startStates[index]; }

      /**
       * Test whether or not this state machine is empty (has no
       * states).
       *
       * \return True iff this state machine has at least one state.
       */
      bool empty() const { return getNumberOfStates(); }

      /**
       * Add a new state to this state machine.
       *
       * \param tokenId The default value will make the new state a
       * non-accepting state. Any other value will make it an
       * accepting state. If you need only one kind of accepting state
       * in your automaton, you should pass
       * TraitsType::defaultToken(). Different token IDs are usefull
       * for lexers that produce a number of distinct tokens.
       *
       * \return The ID of the new state.
       *
       * \note If you want to create a new start state, use this
       * method first, then pass the returned state ID to \c
       * registerStartState.
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



    private:
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
        Util::RangeMap<CharType, StateSet> ranges;

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
      bool closedAdd(StateId state, StateSet &set) const;

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
      void initEdgeMap(StateSet const &s, EdgeMap &m) const;

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
      TokenId chooseTokenId(StateSet const &s) const;


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

      struct EpsilonEdgeProxy: core::ProxyBase<EpsilonEdgeProxy, EpsilonEdgeRepIter>
      {
        StateId getTargetState() const { return *this->i; }

        EpsilonEdgeProxy(EpsilonEdgeRepIter i):
          core::ProxyBase<EpsilonEdgeProxy, EpsilonEdgeRepIter>(i) {}
      };

      typedef core::IterSeq<core::ProxyIter<EdgeRangeProxy> >    EdgeRangeSeq;
      typedef core::IterSeq<core::ProxyIter<SentinelEdgeProxy> > SentinelEdgeSeq;
      typedef core::IterSeq<core::ProxyIter<EpsilonEdgeProxy> >  EpsilonEdgeSeq;

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

        EpsilonEdgeSeq getEpsilonEdges() const
        {
          return EpsilonEdgeSeq(EpsilonEdgeProxy(this->i->epsilonEdges.begin()),
                                EpsilonEdgeProxy(this->i->epsilonEdges.end()));
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


    typedef BasicFsa<char>    Fsa;
    typedef BasicFsa<wchar_t> WideFsa;




    // Template implementations:


    template<typename Ch, typename Tok, typename Tr>
    typename BasicFsa<Ch, Tok, Tr>::StateId BasicFsa<Ch, Tok, Tr>::addState(TokenId tokenId)
    {
      states.push_back(StateRep(tokenId));
      return states.size()-1;
    }


    template<typename Ch, typename Tok, typename Tr>
    typename BasicFsa<Ch, Tok, Tr>::SizeType BasicFsa<Ch, Tok, Tr>::registerStartState(StateId s)
    {
      startStates.push_back(s);
      return startStates.size()-1;
    }


    template<typename Ch, typename Tok, typename Tr>
    void BasicFsa<Ch, Tok, Tr>::addEdgeRange(StateId origin, StateId target, CharRange range)
    {
      if(range.second < range.first) throw std::invalid_argument("Illegal range");
      states[origin].edgeRanges.push_back(EdgeRangeRep(range, target));
    }


    template<typename Ch, typename Tok, typename Tr>
    void BasicFsa<Ch, Tok, Tr>::addSentinelEdge(StateId origin, StateId target, Sentinel sentinel)
    {
      states[origin].sentinelEdges.push_back(SentinelEdgeRep(sentinel, target));
    }


    template<typename Ch, typename Tok, typename Tr>
    void BasicFsa<Ch, Tok, Tr>::addEpsilonEdge(StateId origin, StateId target)
    {
      states[origin].epsilonEdges.push_back(target);
    }


    template<typename Ch, typename Tok, typename Tr>
    typename BasicFsa<Ch, Tok, Tr>::StatePair BasicFsa<Ch, Tok, Tr>::alternFragments(StatePair f1, StatePair f2)
    {
      StateId t = addState(), u = addState();
      addEpsilonEdge(t, f1.first);
      addEpsilonEdge(t, f2.first);
      addEpsilonEdge(f1.second, u);
      addEpsilonEdge(f2.second, u);
      return StatePair(t, u);
    }


    template<typename Ch, typename Tok, typename Tr>
    typename BasicFsa<Ch, Tok, Tr>::StatePair BasicFsa<Ch, Tok, Tr>::concatFragments(StatePair f1, StatePair f2)
    {
      addEpsilonEdge(f1.second, f2.first);
      return StatePair(f1.first, f2.second);
    }


    template<typename Ch, typename Tok, typename Tr>
    typename BasicFsa<Ch, Tok, Tr>::StatePair BasicFsa<Ch, Tok, Tr>::repeatFragment(StatePair f)
    {
      addEpsilonEdge(f.second, f.first);
      return f;
    }


    template<typename Ch, typename Tok, typename Tr>
    typename BasicFsa<Ch, Tok, Tr>::StatePair BasicFsa<Ch, Tok, Tr>::optionalFragment(StatePair f)
    {
      StateId t = addState(), u = addState();
      addEpsilonEdge(t, f.first);
      addEpsilonEdge(f.second, u);
      addEpsilonEdge(t, u);
      return StatePair(t, u);
    }


    template<typename Ch, typename Tok, typename Tr>
    typename BasicFsa<Ch, Tok, Tr>::StatePair BasicFsa<Ch, Tok, Tr>::stringFragment(StringType s)
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
    typename BasicFsa<Ch, Tok, Tr>::StatePair BasicFsa<Ch, Tok, Tr>::rangesFragment(Iter begin, Iter end)
    {
      StateId t = addState(), u = addState();
      for(Iter i = begin; i!=end; ++i) addEdgeRange(t, u, CharRange(i->first, i->second));
      return StatePair(t, u);
    }


    template<typename Ch, typename Tok, typename Tr>
    typename BasicFsa<Ch, Tok, Tr>::StatePair BasicFsa<Ch, Tok, Tr>::sentinelFragment(Sentinel s)
    {
      StateId t = addState(), u = addState();
      addSentinelEdge(t, u, s);
      return StatePair(t, u);
    }


    template<typename Ch, typename Tok, typename Tr>
    bool BasicFsa<Ch, Tok, Tr>::closedAdd(StateId state, StateSet &stateSet) const
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
    typename BasicFsa<Ch, Tok, Tr>::TokenId BasicFsa<Ch, Tok, Tr>::chooseTokenId(StateSet const &stateSet) const
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
    struct BasicFsa<Ch, Tok, Tr>::StateAdder
    {
      bool operator()(StateSet &s) { return fsa->closedAdd(state, s); }
      StateAdder(BasicFsa const *n, StateId s): fsa(n), state(s) {}
      BasicFsa const *fsa;
      StateId state;
    };

    template<typename Ch, typename Tok, typename Tr>
    void BasicFsa<Ch, Tok, Tr>::initEdgeMap(StateSet const &s, EdgeMap &m) const
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

#endif // ARCHON_PARSER_FSA_HPP
