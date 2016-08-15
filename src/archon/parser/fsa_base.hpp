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

#ifndef ARCHON_PARSER_FSA_BASE_HPP
#define ARCHON_PARSER_FSA_BASE_HPP

#include <limits>
#include <string>

#include <archon/core/refcnt.hpp>
#include <archon/core/stream.hpp>

namespace archon
{
  namespace parser
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
     * An abstract interface for an arbitrary finite state
     * machine/automaton (FSA/FSM).
     *
     * This definition allows the following extensions compared to the
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
     * string. To be able to report a deterministic token ID on match
     * in such a case we shall always assume that the token ID with
     * the highest numerical value takes precedence.
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
     * states will in general represent two different sets of accepted
     * strings.
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
     * explicitly, however we we shall always assume their presence.
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
     * since it will simply be ignored. this leads to the following
     * simple rule for the DFA simulator:
     *
     * At any particular state determine whether that state has any
     * sentinel edges leaving it. If not, continue as normal. that is
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
     *
     * \sa Nfa
     *
     * \sa Dfa
     */
    template<typename Ch, typename Tok = short unsigned, typename Tr = FsaTraits<Ch, Tok> >
    struct FsaBase: core::CntRefObjectBase
    {
      typedef Tr                              TraitsType;
      typedef typename TraitsType::CharType   CharType;
      typedef typename TraitsType::StringType StringType;
      typedef typename TraitsType::StreamType StreamType;
      typedef typename TraitsType::SizeType   SizeType;
      typedef typename TraitsType::TokenId    TokenId;
      typedef typename TraitsType::StateId    StateId;

      typedef core::CntRef<FsaBase>           Ref;
      typedef core::CntRef<FsaBase const>     ConstRef;
      typedef Ref const                      &RefArg;
      typedef ConstRef const                 &ConstRefArg;

      enum Sentinel
      {
        anchor_bol,
        anchor_eol,
        anchor_bow,
        anchor_eow
      };

      /**
       * A matcher object linked to the state machine through which it
       * was created.
       *
       * The purpose of the matcher is to keep track of the input
       * state between repeated match operations.
       *
       * The matcher assumes that the specified input is a complete
       * entity. It will consider a certain set of anchor conditions
       * to be fulfilled initially based on this assumption. For
       * example, the "beginning of line" anchor condition will always
       * be satisfied at the start of input.
       */
      struct Matcher: core::CntRefObjectBase, core::CntRefDefs<Matcher>
      {
        /**
         * Match the longest possible prefix of the remaining
         * input. This method may be called repeatedly to tokenize the
         * input.
         *
         * \param startState The state ID of the start state from
         * which the machine simulation must start. The default value
         * of \c TraitsType::noState() means the fist available start
         * state. That is, the start state one would get by passing
         * zero to the method \c getStartState(). Assuming still that
         * the default value is passed, then if this automaton has no
         * start state, the match shall fail by returning \c
         * TraitsType::noToken().
         *
         * \return The numerically highest token ID among all the
         * accepting states that are reachable from the specified
         * start state on the matched input. Of course, for a
         * deterministic automaton there will always be at most one
         * accepting state to consider. If no prefix could be matched
         * \c TraitsType::noToken() will be returned and the input
         * position will be left unchanged.
         */
        virtual TokenId match(StateId startState = TraitsType::noState()) const = 0;

        virtual ~Matcher() {}
      };

      /**
       * Construct a matcher object feeded from the specified input
       * stream.
       *
       * It is the intention that the constuction and initialization
       * of the matcher should be a lean operation and that the
       * matching operation should be carried out in a direct
       * manner. In particular implementations are not supposed to run
       * any kind of optimization or transformation of the state
       * machine.
       *
       * \param input The input stream to feed to the matcher.
       *
       * \return A new matcher object
       */
      virtual core::CntRef<Matcher> getMatcher(core::CntRef<StreamType> const &input) const = 0;

      /**
       * Simulate this state machine on the specified input string.
       *
       * \param input The input string to use in the simulation.
       *
       * \return The numerically highest token ID among all the
       * accepting states that are reachable from the specified start
       * state on the specified input. If the input string is rejected
       * (ie. no accepting states are reachable) then
       * \c TraitsType::noToken() will be returned.
       *
       * \sa Matcher::match
       */
      virtual TokenId match(StringType const &input, StateId startState = TraitsType::noState()) const = 0;

      /**
       * Get the number of states in this state machine.
       *
       * This includes both start states and accepting states.
       *
       * \return The number of states in this state machine.
       */
      virtual SizeType getNumberOfStates() const = 0;

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
      virtual SizeType getStartStateRegistrySize() const = 0;

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
      virtual StateId getStartState(SizeType index) const = 0;

      /**
       * Test whether or not this state machine is empty (has no
       * states).
       *
       * \return True iff this state machine has at least one state.
       */
      virtual bool empty() const { return getNumberOfStates(); }

      virtual ~FsaBase() {}
    };
  }
}

#endif // ARCHON_PARSER_FSA_BASE_HPP
