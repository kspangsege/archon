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

/// \file
///
/// \author Kristian Spangsege
///
/// Testing the nondeterministic finite automata (NFA) implementation.

#include <string>
#include <iostream>

#include <archon/parser/cfg.hpp>

//using namespace archon::core;
using namespace archon::parser;

class MyActor: public Cfg::Actor {
public:
    int get_num_methods() const
    {
        return 0;
    }

    int get_method_arity(int) const
    {
        return 1;
    }

    std::string get_method_name(int) const
    {
        return "hest";
    }
};

int main()
{
    MyActor a;
    Cfg g{&a};

    int t_a = g.define_terminal("A");
    int t_b = g.define_terminal("B");

    int n_a = g.define_nonterminal("a");
    int n_b = g.define_nonterminal("b");

    g.add_prod(n_a, Cfg::nont(n_b), Cfg::term(t_a));
    g.add_prod(n_a, Cfg::nont(n_b), Cfg::term(t_b));
    g.add_prod(n_b, Cfg::term(t_a), Cfg::copy(1), Cfg::nont(n_a), Cfg::term(t_b));

    g.eliminate_midrule_actions();

    std::cout << g.print() << "\n";
}
