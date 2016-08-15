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
 * Testing the nondeterministic finite automata (NFA) implementation.
 */

#include <string>
#include <iostream>

#include <archon/parser/cfg.hpp>

using namespace std;
using namespace archon::core;
using namespace archon::parser;

struct MyActor: Cfg::Actor
{
  int getNumberOfMethods() const
  {
    return 0;
  }

  int getMethodArity(int) const
  {
    return 1;
  }

  string getMethodName(int) const
  {
    return "hest";
  }
};

int main() throw()
{
  MyActor a;
  Cfg g(&a);

  int t_a = g.defineTerminal("A");
  int t_b = g.defineTerminal("B");

  int n_a = g.defineNonterminal("a");
  int n_b = g.defineNonterminal("b");

  g.addProd(n_a, Cfg::nont(n_b), Cfg::term(t_a));
  g.addProd(n_a, Cfg::nont(n_b), Cfg::term(t_b));
  g.addProd(n_b, Cfg::term(t_a), Cfg::copy(1), Cfg::nont(n_a), Cfg::term(t_b));

  g.eliminateMidRuleActions();

  cout << g.print() << endl;

  return 0;
}
