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




#include <iostream>




#include <algorithm>

#include <archon/core/char_enc.hpp>
#include <archon/core/text_table.hpp>
#include <archon/parser/cfg.hpp>

using namespace std;
using namespace archon::core;

namespace archon
{
  namespace parser
  {
    void Cfg::updateRuleIndices(vector<int> const &oldToNewMap)
    {
      for(unsigned i=0; i<rules.size(); ++i)
      {
	Rule &r = rules[i];
	for(unsigned j=0; j<r.productions.size(); ++j)
	{
	  Production &p = r.productions[j];
	  for(unsigned k=0; k<p.symbols.size(); ++k)
	  {
	    Symbol &s = p.symbols[k];
	    if(s.type == Symbol::nonterminal) s.index = oldToNewMap[s.index];
	  }
	}
      }
    }

    string Cfg::chooseUniqueName(string stem, int enumerator)
    {
      if(enumerator < 0)
      {
	while(nonterminalMap.find(stem) != nonterminalMap.end()) stem += "'";
	return stem;
      }

      for(;;)
      {
	string n = stem + valPrinter.print(enumerator);
	if(nonterminalMap.find(n) == nonterminalMap.end()) return n;
	++enumerator;
      }
    }

    void Cfg::findNullableNonTerminals(vector<bool> &nullable,
				       vector<vector<int> > &nullActions)
    {
      nullable.resize(rules.size());
      nullActions.resize(rules.size());
      bool again;
      do
      {
	again = false;
	for(unsigned i=0; i<rules.size(); ++i)
	{
	  if(nullable[i]) continue;
	  Rule &r = rules[i];
	  for(unsigned j=0; j<r.productions.size(); ++j)
	  {
	    Production &p = r.productions[j];
	    unsigned k=0;
	    vector<int> a;
	    while(k<p.symbols.size())
	    {
	      Symbol &s = p.symbols[k];
	      // Break if the current symbol is not nullable
	      if(s.type == Symbol::terminal ||
		 s.type == Symbol::nonterminal && !nullable[s.index]) break;
	      if(s.type == Symbol::action) a.push_back(s.index);
	      else
	      {
		vector<int> const &v = nullActions[s.index];
		a.insert(a.end(), v.begin(), v.end());
	      }
	      ++k;
	    }
	    if(k == p.symbols.size())
	    {
	      // Detect ambiguous nullability
	      if(nullable[i] && nullActions[i] != a)
		throw invalid_argument("Ambiguous nullability for "
                                       "non-terminal '"+r.name+"'");
	      nullable[i] = true;
	      nullActions[i] = a;
	      again = true;
	    }
	  }
	}
      }
      while(again);
    }

    void Cfg::addNullableCombinations(unsigned i, bool epsilon,
				      Production const &production,
				      vector<bool> const &nullable,
				      vector<vector<int> > const &nullActions,
				      vector<Symbol> &prefix,
				      vector<Production> &newProductions)
    {
      while(i<production.symbols.size())
      {
	Symbol const &s = production.symbols[i];
	++i;
	if(s.type == Symbol::nonterminal && nullable[s.index])
	{
	  vector<Symbol> p = prefix;
	  vector<int> const &v = nullActions[s.index];
	  for(unsigned j=0; j<v.size(); ++j)
	    p.push_back(act(v[j]));
	  addNullableCombinations(i, epsilon, production, nullable,
				  nullActions, p, newProductions);
	}
	prefix.push_back(s);
	if(s.type != Symbol::action) epsilon = false;
      }
      if(!epsilon) newProductions.push_back(Production(prefix));
    }

    int Cfg::eliminateCyclesVisit(int ruleIndex, vector<int> &visitedRules,
				  list<pair<int, int> > &cycle)
    {
      if(visitedRules[ruleIndex] == 1) return ruleIndex;
      if(visitedRules[ruleIndex] == 2) return -1;

      visitedRules[ruleIndex] = 1;

      Rule &r = rules[ruleIndex];

      for(unsigned j=0; j<r.productions.size(); ++j)
      {
	Production &p = r.productions[j];

	int targetNonTerminal = -1;

	for(unsigned k=0; k<p.symbols.size(); ++k)
	{
	  Symbol &s = p.symbols[k];
	  if(s.type == Symbol::nonterminal)
	  {
	    if(targetNonTerminal >= 0)
	    {
	      targetNonTerminal = -1;
	      break;
	    }
	    targetNonTerminal = s.index;
	  }
	  else if(s.type == Symbol::terminal)
	  {
	    targetNonTerminal = -1;
	    break;
	  }
	}

	if(targetNonTerminal < 0) continue;

	int s = eliminateCyclesVisit(targetNonTerminal, visitedRules, cycle);
	if(s == -2) return -2;
	if(s == -1) continue;
	if(p.symbols.size() > 1)
	  throw invalid_argument("Ambiguous count for cycle production '"+r.name +
                                 " -> "+printProductionRightSide(p)+"'");
	cycle.insert(cycle.begin(), make_pair(ruleIndex, j));
	return s == ruleIndex ? -2 : s;
      }

      visitedRules[ruleIndex] = 2;
      return -1;
    }

    string Cfg::printProductionRightSide(Production const &p, int m) const
    {
      string r;
      for(unsigned i=0; i<p.symbols.size(); ++i)
      {
	if(static_cast<int>(i) == m) r += "·";
	else if(i) r += " ";
	Symbol const &s = p.symbols[i];
	switch(s.type)
	{
	case Symbol::terminal:
	  r += printTerminal(s.index);
	  break;
	case Symbol::nonterminal:
	  r += printNonterminal(s.index);
	  break;
	case Symbol::action:
	  r += ascii_tolower(actor->getMethodName(s.index)) + "(";
	  for(unsigned j=0; j<s.args.size(); ++j)
	  {
	    if(j) r += ", ";
	    r += s.args[j]<0 ? "_" : valPrinter.print(static_cast<int>(i)-s.args[j]);
	  }
	  r += ")";
	  break;
	case Symbol::nil:
	  break;
	}
      }
      if(m == static_cast<int>(p.symbols.size())) r += "·";
      return r.size() ? r : "<epsilon>";
    }

    Cfg::~Cfg()
    {{ // The extra scope is needed to work around gcc3.2 bug #8287
    }}

    int Cfg::defineTerminal(string const &name)
    {
      pair<map<string, int>::iterator, bool> r =
	terminalMap.insert(make_pair(name, terminals.size()));
      if(!r.second) throw invalid_argument("Redefinition of terminal '"+name+"'");
      terminals.push_back(name);
      return terminals.size()-1;
    }

    int Cfg::defineNonterminal(string const &name)
    {
      pair<map<string, int>::iterator, bool> r =
	nonterminalMap.insert(make_pair(name, rules.size()));
      if(!r.second) throw invalid_argument("Redefinition of non-terminal '"+name+"'");
      rules.push_back(Rule(name));
      return rules.size()-1;
    }

    void Cfg::addProd(int nontermIndex, vector<Symbol> const &symbols)
    {
      for(unsigned i=0; i<symbols.size(); ++i)
      {
	Symbol const &s = symbols[i];
	switch(s.type)
	{
	case Symbol::terminal:
	  if(s.index < 0 || s.index >= static_cast<int>(terminals.size()))
	    throw invalid_argument("Illegal terminal index");
	  break;
	case Symbol::nonterminal:
	  if(s.index < 0 || s.index >= static_cast<int>(rules.size()))
	    throw invalid_argument("Illegal nonterminal index");
	  break;
	case Symbol::action:
	  if(!actor)
	    throw invalid_argument("Can't have actions without an actor");
	  if(s.index < -3 || s.index >= actor->getNumberOfMethods())
	    throw invalid_argument("Illegal method index");
	  if(static_cast<int>(s.args.size()) != actor->getMethodArity(s.index))
	    throw invalid_argument("Wrong number of arguments to '" +
                                   actor->getMethodName(s.index)+"'");
	  break;
	case Symbol::nil:
	  break;
	}
      }
      rules[nontermIndex].addProduction(symbols);
    }

    void Cfg::addProd(int nontermIndex, Symbol s1, Symbol s2, Symbol s3,
		      Symbol s4, Symbol s5, Symbol s6, Symbol s7, Symbol s8)
    {
      vector<Symbol> symbols;
      if(s1.type) symbols.push_back(s1);
      if(s2.type) symbols.push_back(s2);
      if(s3.type) symbols.push_back(s3);
      if(s4.type) symbols.push_back(s4);
      if(s5.type) symbols.push_back(s5);
      if(s6.type) symbols.push_back(s6);
      if(s7.type) symbols.push_back(s7);
      if(s8.type) symbols.push_back(s8);
      addProd(nontermIndex, symbols);
    }

    Cfg::Symbol const &Cfg::nil()
    {
      static Symbol s;
      return s;
    }

    Cfg::Symbol Cfg::act(int methodIndex, int arg1, int arg2, int arg3,
			 int arg4, int arg5, int arg6, int arg7)
    {
      vector<int> args;
      if(arg1>-2) args.push_back(arg1);
      if(arg2>-2) args.push_back(arg2);
      if(arg3>-2) args.push_back(arg3);
      if(arg4>-2) args.push_back(arg4);
      if(arg5>-2) args.push_back(arg5);
      if(arg6>-2) args.push_back(arg6);
      if(arg7>-2) args.push_back(arg7);
      return Symbol(methodIndex, args);
    }

    template<typename T>
    void set_include(set<T> const &right, set<T> &left)
    {
      typename set<T>::iterator i=left.begin();
      typename set<T>::const_iterator j=right.begin();
      while(j!=right.end())
      {
	if(i == left.end() || *j < *i) left.insert(i, *j++);
	else if(*j == *i) ++j;
	else ++i;
      }
    }

    Cfg::FirstSets::FirstSets(Cfg const &g): grammar(g)
    {
      terminals.resize(g.rules.size());
      nullable.resize(g.rules.size());
      bool again;
      do
      {
	again = false;
	for(unsigned i=0; i<g.rules.size(); ++i)
	{
	  Rule const &r = g.rules[i];
	  set<int> &t = terminals[i];
	  for(unsigned j=0; j<r.productions.size(); ++j)
	  {
	    unsigned const n = t.size();
	    Item item(i, j, 0);
	    if(includeFirstSet(item, t) && !nullable[i]) again = nullable[i] = true;
	    else if(t.size() > n) again = true;
	  }
	}
      }
      while(again);
    }

    bool Cfg::FirstSets::includeFirstSet(Item const &item, set<int> &t) const
    {
      Production const &p =
	grammar.rules[item.rule].productions[item.production];
      unsigned i;
      for(i=item.position; i<p.symbols.size(); ++i)
      {
	Symbol const &s = p.symbols[i];
      
	if(s.type == Symbol::terminal)
	{
	  t.insert(s.index);
	  break;
	}

	if(s.type == Symbol::nonterminal)
	{
	  set_include(terminals[s.index], t);
	  if(!nullable[s.index]) break;
	}
      }
      return i == p.symbols.size();
    }

    string Cfg::printTerminal(int i) const
    {
      if(i < 0) return "<eoi>";
      return ascii_toupper(terminals[i]);
    }

    string Cfg::printNonterminal(int i) const
    {
      return ascii_tolower(rules[i].name);
    }

    string Cfg::printProduction(int i, int j) const
    {
      return printNonterminal(i) + " -> " +
	printProductionRightSide(rules[i].productions[j]);
    }

    string Cfg::printItem(Item const &i) const
    {
      return printNonterminal(i.rule) + " -> " +
	printProductionRightSide(rules[i.rule].productions[i.production],
				 i.position);
    }

    string Cfg::FirstSets::print(int width) const
    {
      Text::Table table;
      table.get_col(0).set_width(1);
      table.get_col(1).set_width(4);
      table.get_cell(0,0).set_text("Nonterminal");
      table.get_cell(0,1).set_text("First set");
      for(unsigned i=0; i<terminals.size(); ++i)
      {
	set<int> const &t = terminals[i];
	table.get_cell(i+1, 0).set_text(grammar.printNonterminal(i));
        string s;
	for(set<int>::const_iterator j=t.begin(); j!=t.end(); ++j)
	{
	  if(j != t.begin()) s += " ";
	  s += grammar.printTerminal(*j);
	}
	if(nullable[i])
	{
	  if(s.size()) s += " ";
	  s += "<epsilon>";
	}
        table.get_cell(i+1, 1).set_text(s);
      }

      return table.print(width, 2, true);
    }

    Cfg::FollowSets::FollowSets(FirstSets const &f): grammar(f.grammar)
    {
      terminals.resize(grammar.rules.size());
      terminals[0].insert(-1);
      bool again;
      do
      {
	again = false;
	for(unsigned i=0; i<grammar.rules.size(); ++i)
	{
	  Rule const &r = grammar.rules[i];
	  for(unsigned j=0; j<r.productions.size(); ++j)
	  {
	    Production const &p = r.productions[j];
	    for(unsigned k=0; k<p.symbols.size(); ++k)
	    {
	      Symbol const &s = p.symbols[k];
	      if(s.type != Symbol::nonterminal) continue;
	      set<int> &t = terminals[s.index];
	      unsigned const n = t.size();
	      Item item(i, j, k+1);
	      if(f.includeFirstSet(item, t)) set_include(terminals[i], t);
	      if(t.size() > n) again = true;
	    }
	  }
	}
      }
      while(again);
    }

    string Cfg::FollowSets::print(int width) const
    {
      Text::Table table;
      table.get_col(0).set_width(1);
      table.get_col(1).set_width(4);
      table.get_cell(0,0).set_text("Nonterminal");
      table.get_cell(0,1).set_text("Follow set");
      for(unsigned i=0; i<terminals.size(); ++i)
      {
	set<int> const &t = terminals[i];
	table.get_cell(i+1, 0).set_text(grammar.printNonterminal(i));
        string s;
	for(set<int>::const_iterator j=t.begin(); j!=t.end(); ++j)
	{
	  if(*j < 0) continue;
	  if(s.size()) s += " ";
	  s += grammar.printTerminal(*j);
	}
	if(t.size() && *t.begin()<0)
	{
	  if(t.size() > 1) s += " ";
	  s += "<eoi>";
	}
        table.get_cell(i+1, 1).set_text(s);
      }

      return table.print(width, 2, true);      
    }

    string Cfg::print(int width) const
    {
      Text::Table table;
      table.get_col(0).set_width(10);
      table.get_col(1).set_width( 1);
      table.get_col(2).set_width(39);
      
      int l = 0;
      for(unsigned i=0; i<rules.size(); ++i)
      {
	if(i)
	{
	  table.get_cell(l,0).set_text(" ");
	  ++l;
	}
	table.get_cell(l,0).set_text(printNonterminal(i));
	table.get_cell(l,1).set_text("=");
	Rule const &r = rules[i];
	for(unsigned j=0; j<r.productions.size(); ++j)
	{
	  if(j) table.get_cell(l,1).set_text("|");
	  table.get_cell(l,2).set_text(printProductionRightSide(r.productions[j]));
	  ++l;
	}
	if(!r.productions.size()) ++l;
      }

      return table.print(width, 1);
    }

    void Cfg::introduceNewStartSymbol()
    {
      if(!rules.size()) throw invalid_argument("Original grammar must have "
                                               "at least one nonterminal");
      vector<int> m(rules.size());
      for(unsigned l=0; l<rules.size(); ++l) m[l] = l+1;
      updateRuleIndices(m);
      rules.insert(rules.begin(), Rule(chooseUniqueName(rules[0].name, -1)));
      addProd(0, nont(1));
    }

    void Cfg::eliminateEpsilonProductions()
    {
      vector<bool> nullable;
      vector<vector<int> > nullActions;
      findNullableNonTerminals(nullable, nullActions);

      // If the start symbol is nullable it may not occur on the right
      // hand side of any production
      if(nullable[0])
      {
	for(unsigned i=0; i<rules.size(); ++i)
	{
	  Rule &r = rules[i];
	  for(unsigned j=0; j<r.productions.size(); ++j)
	  {
	    Production &p = r.productions[j];
	    for(unsigned k=0; k<p.symbols.size(); ++k)
	    {
	      Symbol &s = p.symbols[k];
	      if(s.type == Symbol::nonterminal && s.index == 0)
	      {
		// Make a brand new start symbol
		nullable.insert(nullable.begin(), true);
		nullActions.insert(nullActions.begin(), nullActions[0]);
		introduceNewStartSymbol();
		i = rules.size();
		j = r.productions.size();
		break;
	      }
	    }
	  }
	}
      }

      // Rewrite each rule
      for(unsigned i=0; i<rules.size(); ++i)
      {
	Rule &r = rules[i];
	vector<Production> newProductions;
	for(unsigned j=0; j<r.productions.size(); ++j)
	{
	  vector<Symbol> prefix;
	  addNullableCombinations(0, true, r.productions[j], nullable,
				  nullActions, prefix, newProductions);
	}

	r.productions = newProductions;
      }

      // Is the start symbol nullable
      if(nullable[0])
      {
	vector<Symbol> epsilon;
	vector<int> const &v = nullActions[0];
	for(unsigned j=0; j<v.size(); ++j)
	  epsilon.push_back(act(v[j]));
	addProd(0, epsilon);
      }
    }

    void Cfg::eliminateCycles()
    {
      Cfg g = *this;
      g.eliminateEpsilonProductions();
      int cycles = 0;
      for(;;)
      {
	// Find a cycle
	list<pair<int, int> > cycle;
	vector<int> visitedRules(g.rules.size()); // 0=unvisited, 1=in current path, 2=visited but no longer in current path
	for(unsigned i=0; i<g.rules.size(); ++i)
	  if(g.eliminateCyclesVisit(i, visitedRules, cycle) == -2) break;
	if(!cycle.size()) break;
	++cycles;

	cerr << "Found cycle: " << g.rules[cycle.front().first].name;
	for(list<pair<int, int> >::iterator i=cycle.begin(); i!=cycle.end(); ++i)
	  cerr << " -> " << g.printProductionRightSide(g.rules[i->first].productions[i->second]);
	cerr << "\n";

	set<pair<int, int> > cycleProductions;
	set<int> cycleNonTerminals;
	for(list<pair<int, int> >::iterator i=cycle.begin(); i!=cycle.end(); ++i)
	{
	  cycleProductions.insert(*i);
	  cycleNonTerminals.insert(i->first);
	}

	// Rewrite each rule to eliminate the cycle
	for(unsigned i=0; i<g.rules.size(); ++i)
	{
	  Rule &r = g.rules[i];
	  vector<Production> newProductions;
	  for(unsigned j=0; j<r.productions.size(); ++j)
	  {
	    Production &p = r.productions[j];

	    pair<int, int> const q = make_pair(i, j);
	    if(cycleProductions.find(q) != cycleProductions.end())
	    {
	      if(q != cycle.back()) newProductions.push_back(p);
	    }
	    else
	    {
	      Production n;
	      for(unsigned k=0; k<p.symbols.size(); ++k)
	      {
		Symbol &s = p.symbols[k];
		if(s.type == Symbol::nonterminal &&
		   cycleNonTerminals.find(s.index) != cycleNonTerminals.end())
		  n.symbols.push_back(nont(cycle.front().first));
		else n.symbols.push_back(s);
	      }
	      newProductions.push_back(n);
	    }
	  }

	  r.productions = newProductions;
	}
      }

      if(cycles) *this = g;
    }

    void Cfg::eliminateMidRuleActions()
    {
      unsigned const n = rules.size();
      for(unsigned i=0; i<n; ++i)
      {
	Rule *r = &rules[i];
	for(unsigned j=0; j<r->productions.size(); ++j)
	{
	  Production *p = &r->productions[j];
	  for(unsigned k=0; k+1<p->symbols.size(); ++k)
	  {
	    Symbol *s = &p->symbols[k];
	    if(s->type != Symbol::action) continue;
	    int const a = defineNonterminal(chooseUniqueName("action", 1));
	    // rules vector may be relocated so we need to aquire a new
	    // pointers.
	    r = &rules[i];
	    p = &r->productions[j];
	    s = &p->symbols[k];
	    addProd(a, *s);
	    s->type = Symbol::nonterminal;
	    s->index = a;
	    s->args.clear();
	  }
	}
      }
    }
  }
}
