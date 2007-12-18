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
 * Testing the map that remembers the order of insertions.
 */

#include <utility>
#include <string>
#include <iostream>

#include <archon/util/remem_order_map.hpp>


using namespace std;
using namespace Archon::Util;


int main()
{
  typedef RememOrderMap<string, string> Map;
  typedef Map::remem_order_iterator iterator;

  Map m;
  m.insert(pair<string, string>("beta",  "test"));
  m.insert(pair<string, string>("delta", "wing"));
  m.insert(pair<string, string>("gamma", "goblin"));
  m.insert(pair<string, string>("alpha", "beat"));

  iterator begin(m.remem_order_begin()), end(m.remem_order_end());
  for(iterator i = begin; i != end; ++i) cout << i->first << " -> " << i->second << endl;

  return 0;
}

