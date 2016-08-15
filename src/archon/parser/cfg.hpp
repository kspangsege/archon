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

#ifndef ARCHON_PARSER_CFG_HPP
#define ARCHON_PARSER_CFG_HPP

#include <string>
#include <utility>
#include <vector>
#include <map>
#include <set>
#include <list>

#include <archon/core/text.hpp>

namespace archon
{
  namespace Parser
  {
    /**
     * Context free grammars
     *
     * The start symbol is always the left hand side of the first rule.
     *
     * Still to be done:
     *  Conversion to Chomsky Normal Form
     *   http://www.wikipedia.com/wiki/Chomsky+Normal+Form
     *   http://muldoon.cipic.ucdavis.edu/~jchen007/UCD/ECS120/Notes/LectureNotes11.pdf
     */
    struct Cfg
    {
      struct Rule;
      struct FirstSets;
      struct FollowSets;

      struct Item
      {
	int rule;
	int production;
	int position;
	Item(int rule, int production, int position):
	  rule(rule), production(production), position(position) {}
      };

      struct Symbol
      {
	enum Type
	{
	  nil = 0,
	  terminal,
	  nonterminal,
	  action
	};

	Type getType() const { return type; }
	int getIndex() const { return index; }
	std::vector<int> const &getArgs() const { return args; }

      private:
	friend class Cfg;
	friend class FirstSets;
	friend class FollowSets;

	Type type;
	int index; // -1 is 'null', -2 is 'copy' and -3 is 'concat'
	std::vector<int> args; // For actions only

	Symbol(): type(nil) {}
	Symbol(Type type, int index): type(type), index(index) {}
	Symbol(int index, std::vector<int> const &args):
	  type(action), index(index), args(args) {}
      };

      struct Production
      {
	int getNumberOfSymbols() const { return symbols.size(); }
	Symbol const &getSymbol(int i) const { return symbols[i]; }

      private:
	friend class Cfg;
	friend class Rule;
	friend class FirstSets;
	friend class FollowSets;

	Production() {}
	Production(std::vector<Symbol> const &symbols): symbols(symbols) {}

	std::vector<Symbol> symbols;
      };

      struct Rule
      {
	int getNumberOfProductions() const { return productions.size(); }
	Production const &getProduction(int i) const { return productions[i]; }

      private:
	friend class Cfg;
	friend class FirstSets;
	friend class FollowSets;

	std::string name;
	std::vector<Production> productions;

	Rule(std::string const &name): name(name) {}

	void addProduction(std::vector<Symbol> const &symbols)
	{
	  productions.push_back(Production(symbols));
	}
      };

      struct Actor
      {
	/**
	 * Get the number of user-defined methods
	 */
	virtual int getNumberOfMethods() const = 0;

	/**
	 * @param methodIndex -3 for 'concat', -2 for 'copy' and -1 for 'null'
	 * otherwise user-defined
	 */
	virtual int getMethodArity(int methodIndex) const = 0;

	/**
	 * @param methodIndex -3 for 'concat', -2 for 'copy' and -1 for 'null'
	 * otherwise user-defined
	 */
	virtual std::string getMethodName(int methodIndex) const = 0;

	virtual ~Actor() {}
      };


      Cfg(Actor const *a=0): actor(a) {}

      int getNumberOfTerminals() const { return terminals.size(); }
      int getNumberOfRules() const { return rules.size(); }
      Rule const &getRule(int i) const { return rules[i]; }

      int defineTerminal(std::string const &name);
      int defineNonterminal(std::string const &name);

      static Symbol const &nil();

      void addProd(int nontermIndex, std::vector<Symbol> const &);
      void addProd(int nontermIndex, Symbol=nil(), Symbol=nil(),
		   Symbol=nil(), Symbol=nil(), Symbol=nil(),
		   Symbol=nil(), Symbol=nil(), Symbol=nil());

      /**
       * Make a terminal symbol from a terminal index
       */
      static Symbol term(int terminalIndex)
      {
	return Symbol(Symbol::terminal, terminalIndex);
      }

      /**
       * Make a nonterminal symbol from a nonterminal index
       */
      static Symbol nont(int nonterminalIndex)
      {
	return Symbol(Symbol::nonterminal, nonterminalIndex);
      }

      /**
       * Make an action symbol from a method index and argument
       * references. An argument reference of zero refers to the symbol
       * imediately preceeding this action. A value of 1 refers to the
       * symbol before that one and so on. A value of -1 indicates that
       * a null argument should be passed to the method. A value of -2
       * is skipped and does not count as an argument.
       */
      static Symbol act(int methodIndex,
			int arg1 = -2, int arg2 = -2, int arg3 = -2,
			int arg4 = -2, int arg5 = -2, int arg6 = -2,
			int arg7 = -2);

      /**
       * Special action that return the null reference
       */
      static Symbol null() { return act(-1); }

      /**
       * Special action that copies attributes
       */
      static Symbol copy(int arg) { return act(-2, arg); }

      /**
       * Special action that concattenates strings
       */
      static Symbol concat(int arg1, int arg2) { return act(-3, arg1, arg2); }

      struct FirstSets
      {
	FirstSets(Cfg const &);

	/**
	 * Add the first set of the symbols after the position in the
	 * item to the argument set
	 * @return true if the symbols after the item position can derive epsilon
	 */
	bool includeFirstSet(Item const &, std::set<int> &) const;

	std::string print(int width = 0) const;

      private:
	friend struct FollowSets;

	Cfg const &grammar;
	std::vector<std::set<int> > terminals; // One entry per nonterminal
	std::vector<bool> nullable; // One entry per nonterminal
      };

      struct FollowSets
      {
	FollowSets(FirstSets const &);
	std::set<int> const &get(int i) const { return terminals[i]; }
	std::string print(int width = 0) const;

      private:
	Cfg const &grammar;
	std::vector<std::set<int> > terminals; // -1 represents EOI
      };

      /**
       * \param index A negative value will be interpreted as the
       * imaginary EOI terminal.
       */
      std::string printTerminal(int index) const;

      std::string printNonterminal(int index) const;
      std::string printProduction(int rule, int production) const;
      std::string printItem(Item const &) const;
      std::string print(int width = 0) const;

      void introduceNewStartSymbol();

      /**
       * CURRENTLY DOES NOT REWRITE ACTIONS PROPERLY!!!!!
       *
       * Convert the grammar into an equivalent epsilon-free grammar.
       *
       * We say that a grammar is epsilon-free if either it has no
       * epsilon productions or there is exactly one epsilon production
       * for the start symbol and then the start symbol does not appear
       * on the right side of any production.
       *
       * The new grammar will accept exactly the same language and the
       * sequence of semantic actions that are to be evaluated for any
       * given derivation will not be changed.
       *
       * This operation may introduce a new start symbol.
       *
       * This operation may leave the grammar with non-terminals that
       * cannot produce anything, so you might want to run the
       * 'eliminateDeadNonTerminals' method afterwards.
       *
       * This operation also may leave the grammar with duplicate
       * productions, so you also might want to run the
       * 'eliminateDuplicateProductions' method afterwards.
       *
       *
       * Ambiguous nullability:
       *
       * This operation will fail if any non-terminal has ambiguous
       * nullability, that is if there among the ways to derive null
       * from the non-terminal are some that result in different
       * sequences of actions to be performed. An example would be:
       *
       *  A -> B | C
       *  B -> {action1} | b
       *  C -> {action2} | c
       *
       * Note that both B nad C have epsilon productions but with
       * different actions associated to them.
       */
      void eliminateEpsilonProductions();

      /**
       * CURRENTLY DOES NOT REWRITE ACTIONS PROPERLY!!!!!
       *
       * Convert the grammar into an equivalent cycle-free grammar.
       *
       * A grammar is cycle free if it has no cycles at all. A grammar has
       * a cycle if for some non-terminal A there is a possible derivation
       * A =>+ A. That is, a derivation from A to itself in one or more
       * steps.
       *
       * Elimination procedure:
       *
       * To simplify things we will start this operation by deriving an
       * epsilon-free grammar. From such a garmmar it is reasonably simple
       * to find and eliminate cycles. In the search for cycles we only
       * need to considder productions that have no terminal symbols and
       * have exactly one non-terminal on the right side.
       *
       * A cycle in an unambiguous epsilon-free grammar always has the
       * form:
       *
       *  A0 -> A1
       *  A1 -> A2
       *  .
       *  .
       *  .
       *  An-1 -> An
       *  An -> A0
       *
       * That is, the right sides consist of nothing but one non-terminal.
       *
       * One could imagine a cycle where one or more of the involved
       * productions had semantic actions in them, but in that case the
       * grammar would be ambiguous. If any such cycle exists in the
       * grammar this method will report failure.
       *
       * To eliminate the cycle shown above from an epsilon-free grammar
       * we need only to choose an Ai among the cyclic non-terminals then
       * remove the production Ai-1 -> Ai or An -> A0 if i = 0 and then
       * fix all productions that are not part of the cycle by replacing
       * any occurance of Aj by Ai where 0 <= j <= n and j != i.
       *
       * What about productions like A2 -> A1 or A2 -> B and B -> A3?
       *
       * A cycle may never contain a non-terminal twice.
       */
      void eliminateCycles();

      /**
       * Remove non-terminals that produce nothing and non-terminals
       * that are not reachable from the starting symbol.
       */

      // void eliminateDeadNonTerminals();

      /**
       * Rewrite
       *
       *  A -> B C f(1) D E g(2, 5) F G h(3, 8)
       *
       * to
       *
       *  A -> B C M D E N F G h(3, 8)
       *  M -> f(-1)
       *  N -> g(-3, 0)
       */
      void eliminateMidRuleActions();

      ~Cfg();

    private:
      void updateRuleIndices(std::vector<int> const &);
      std::string chooseUniqueName(std::string, int);
      void findNullableNonTerminals(std::vector<bool> &, std::vector<std::vector<int> > &);
      void addNullableCombinations(unsigned, bool,
				   Production const &,
				   std::vector<bool> const &,
				   std::vector<std::vector<int> > const &,
				   std::vector<Symbol> &,
				   std::vector<Production> &);
      int eliminateCyclesVisit(int, std::vector<int> &, std::list<std::pair<int, int> > &);
      std::string printProductionRightSide(Production const &, int mark=-1) const;

      std::vector<std::string> terminals;
      std::vector<Rule> rules;

      std::map<std::string, int> terminalMap;    // Maps terminal names to terminal indices.
      std::map<std::string, int> nonterminalMap; // Maps non-terminal names to rule indices.

      Actor const *actor; // Defines the known methods and knows how to call them. Is optional.

      core::Text::ValuePrinter valPrinter;
    };
  }
}

#endif // ARCHON_PARSER_CFG_HPP
