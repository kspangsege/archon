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

#include <archon/core/codec.hpp>
#include <archon/parser/dfa.hpp>

using namespace std;
using namespace Archon::Core;
using namespace Archon::Parser;

int main() throw()
{
  vector<Nfa::StateSet> stateSets;

  Regex::Exp regex = Regex::juxta(Regex::star(Regex::altern(Regex::str(L"a"),
							    Regex::str(L"b"))),
				  Regex::str(L"abb"));

  Nfa nfa(regex);

  Dfa dfa(mapToDfa(nfa, 0, &stateSets));

  Dfa minimized(minimizeDfa(dfa));

  cout << envEncode(regex->print()) << endl;

  cout << envEncode(nfa.print()) << endl;

  cout << envEncode(dfa.print()) << endl;

  cout << envEncode(minimized.print()) << endl;

  for(size_t i = 0; i < stateSets.size(); ++i)
  {
    cout << i << ": ";
    Nfa::StateSet s = stateSets[i];
    for(Nfa::StateSet::iterator j = s.begin(); j != s.end(); ++j) cout << *j << " ";
    cout << endl;
  }


  // Test handling of anchors
  cout << "Handling of anchors:" << endl;
  regex = Regex::altern(Regex::str(L"a"), Regex::juxta(Regex::altern(Regex::lineBegin(), Regex::str(L"c")), Regex::str(L"b")));
  nfa = Nfa(regex);
  stateSets.clear();
  Dfa::AnchorInfo anchorInfo(L'\xE000', 1);
  dfa = mapToDfa(nfa, &anchorInfo, &stateSets);
  //dfa = mapToDfa(nfa, 0, &stateSets);

  cout << envEncode(regex->print()) << endl;
  cout << envEncode(nfa.print()) << endl;
  cout << envEncode(dfa.print()) << endl;

  for(size_t i = 0; i < stateSets.size(); ++i)
  {
    cout << i << ": ";
    Nfa::StateSet s = stateSets[i];
    for(Nfa::StateSet::iterator j = s.begin(); j != s.end(); ++j) cout << *j << " ";
    cout << endl;
  }
  return 0;
}
