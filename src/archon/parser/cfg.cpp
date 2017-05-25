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




#include <iostream>




#include <algorithm>

#include <archon/core/string.hpp>
#include <archon/core/char_enc.hpp>
#include <archon/core/text_table.hpp>
#include <archon/parser/cfg.hpp>

using namespace archon::core;
using namespace archon::parser;

namespace {

template<class T> void set_include(const std::set<T>& right, std::set<T>& left)
{
    auto i = left.cbegin();
    auto j = right.begin();
    auto end = right.end();
    while (j != end) {
        if (i == left.end() || *j < *i) {
            left.insert(i, *j++);
        }
        else if(*j == *i) {
            ++j;
        }
        else {
            ++i;
        }
    }
}

} // unnamed namespace


void Cfg::update_rule_indices(const std::vector<int>& old_to_new_map)
{
    for (Rule& rule: m_rules) {
        for (Production& prod: rule.m_productions) {
            for(Symbol& sym: prod.m_symbols) {
                if (sym.m_type == Symbol::nonterminal)
                    sym.m_index = old_to_new_map[sym.m_index];
            }
        }
    }
}


std::string Cfg::choose_unique_name(std::string stem, int enumerator)
{
    if (enumerator < 0) {
        while (m_nonterminal_map.find(stem) != m_nonterminal_map.end())
            stem += "'";
        return stem;
    }

    for (;;) {
        std::string n = stem + core::format_int(enumerator, m_locale);
        if (m_nonterminal_map.find(n) == m_nonterminal_map.end())
            return n;
        ++enumerator;
    }
}


void Cfg::find_nullable_nonterminals(std::vector<bool>& nullable,
                                     std::vector<std::vector<int>>& null_actions)
{
    nullable.resize(m_rules.size());
    null_actions.resize(m_rules.size());
    bool again;
    do {
        again = false;
        for (unsigned i = 0; i < m_rules.size(); ++i) {
            if (nullable[i])
                continue;
            Rule& rule = m_rules[i];
            for (Production& prod: rule.m_productions) {
                bool skip = false;
                std::vector<int> a;
                for (Symbol& sym: prod.m_symbols) {
                    // Skip production if the current symbol is not nullable
                    skip = sym.m_type == Symbol::terminal ||
                        (sym.m_type == Symbol::nonterminal && !nullable[sym.m_index]);
                    if (skip)
                        break;
                    if(sym.m_type == Symbol::action) {
                        a.push_back(sym.m_index);
                    }
                    else {
                        const std::vector<int>& v = null_actions[sym.m_index];
                        a.insert(a.end(), v.begin(), v.end());
                    }
                }
                if (skip)
                    continue;
                // Detect ambiguous nullability
                if (nullable[i] && null_actions[i] != a)
                    throw std::invalid_argument("Ambiguous nullability for "
                                                "nonterminal '"+rule.m_name+"'");
                nullable[i] = true;
                null_actions[i] = a;
                again = true;
            }
        }
    }
    while (again);
}


void Cfg::add_nullable_combinations(unsigned i, bool epsilon,
                                    const Production& production,
                                    const std::vector<bool>& nullable,
                                    const std::vector<std::vector<int>>& null_actions,
                                    std::vector<Symbol>& prefix,
                                    std::vector<Production>& new_productions)
{
    while (i < production.m_symbols.size()) {
        const Symbol& sym = production.m_symbols[i];
        ++i;
        if (sym.m_type == Symbol::nonterminal && nullable[sym.m_index]) {
            std::vector<Symbol> p = prefix;
            for (int a: null_actions[sym.m_index])
                p.push_back(act(a));
            add_nullable_combinations(i, epsilon, production, nullable,
                                      null_actions, p, new_productions);
        }
        prefix.push_back(sym);
        if (sym.m_type != Symbol::action)
            epsilon = false;
    }
    if (!epsilon)
        new_productions.emplace_back(prefix);
}


int Cfg::eliminate_cycles_visit(int rule_index, std::vector<int>& visited_rules,
                                std::list<std::pair<int, int> >& cycle)
{
    if (visited_rules[rule_index] == 1)
        return rule_index;
    if (visited_rules[rule_index] == 2)
        return -1;

    visited_rules[rule_index] = 1;

    Rule &rule = m_rules[rule_index];

    for (unsigned i = 0; i < rule.m_productions.size(); ++i) {
        Production &prod = rule.m_productions[i];

        int target_nonterminal = -1;

        for (Symbol& sym: prod.m_symbols) {
            if (sym.m_type == Symbol::nonterminal) {
                if (target_nonterminal >= 0) {
                    target_nonterminal = -1;
                    break;
                }
                target_nonterminal = sym.m_index;
            }
            else if (sym.m_type == Symbol::terminal) {
                target_nonterminal = -1;
                break;
            }
        }

        if (target_nonterminal < 0)
            continue;

        int s = eliminate_cycles_visit(target_nonterminal, visited_rules, cycle);
        if (s == -2)
            return -2;
        if (s == -1)
            continue;
        if (prod.m_symbols.size() > 1)
            throw std::invalid_argument("Ambiguous count for cycle production '"+rule.m_name +
                                        " -> "+print_production_right_side(prod)+"'");
        cycle.insert(cycle.begin(), std::make_pair(rule_index, i));
        return (s == rule_index ? -2 : s);
    }

    visited_rules[rule_index] = 2;
    return -1;
}


std::string Cfg::print_production_right_side(const Production& prod, int m) const
{
    std::string r;
    for (unsigned i = 0; i < prod.m_symbols.size(); ++i) {
        if (int(i) == m) {
            r += "\u00B7";
        }
        else if (i != 0) {
            r += " ";
        }
        const Symbol& sym = prod.m_symbols[i];
        switch (sym.m_type) {
            case Symbol::terminal:
                r += print_terminal(sym.m_index);
                break;
            case Symbol::nonterminal:
                r += print_nonterminal(sym.m_index);
                break;
            case Symbol::action:
                r += ascii_tolower(m_actor->get_method_name(sym.m_index)) + "(";
                for (unsigned j = 0; j < sym.m_args.size(); ++j) {
                    if (j != 0)
                        r += ", ";
                    r += (sym.m_args[j] < 0 ? "_" :
                          core::format_int(int(i) - sym.m_args[j], m_locale));
                }
                r += ")";
                break;
            case Symbol::nil:
                break;
        }
    }
    if (m == int(prod.m_symbols.size()))
        r += "\u00B7";
    return (r.empty() ? "<epsilon>" : r);
}


int Cfg::define_terminal(const std::string& name)
{
    auto p = m_terminal_map.insert(std::make_pair(name, m_terminals.size()));
    if (!p.second)
        throw std::invalid_argument("Redefinition of terminal '"+name+"'");
    m_terminals.push_back(name);
    return m_terminals.size() - 1;
}


int Cfg::define_nonterminal(const std::string& name)
{
    auto p = m_nonterminal_map.insert(std::make_pair(name, m_rules.size()));
    if (!p.second)
        throw std::invalid_argument("Redefinition of non-terminal '"+name+"'");
    m_rules.emplace_back(name);
    return m_rules.size() - 1;
}


void Cfg::add_prod(int nonterm_index, const std::vector<Symbol>& symbols)
{
    for (const Symbol& sym: symbols) {
        switch (sym.m_type) {
            case Symbol::terminal:
                if (sym.m_index < 0 || sym.m_index >= int(m_terminals.size()))
                    throw std::invalid_argument("Illegal terminal index");
                break;
            case Symbol::nonterminal:
                if (sym.m_index < 0 || sym.m_index >= int(m_rules.size()))
                    throw std::invalid_argument("Illegal nonterminal index");
                break;
            case Symbol::action:
                if (!m_actor)
                    throw std::invalid_argument("Can't have actions without an actor");
                if (sym.m_index < -3 || sym.m_index >= m_actor->get_num_methods())
                    throw std::invalid_argument("Illegal method index");
                if (int(sym.m_args.size()) != m_actor->get_method_arity(sym.m_index))
                    throw std::invalid_argument("Wrong number of arguments to '" +
                                                m_actor->get_method_name(sym.m_index)+"'");
                break;
            case Symbol::nil:
                break;
        }
    }
    m_rules[nonterm_index].add_production(symbols);
}


void Cfg::add_prod(int nonterm_index, Symbol s_1, Symbol s_2, Symbol s_3, Symbol s_4,
                   Symbol s_5, Symbol s_6, Symbol s_7, Symbol s_8)
{
    std::vector<Symbol> symbols;
    if (s_1.m_type)
        symbols.push_back(s_1);
    if (s_2.m_type)
        symbols.push_back(s_2);
    if (s_3.m_type)
        symbols.push_back(s_3);
    if (s_4.m_type)
        symbols.push_back(s_4);
    if (s_5.m_type)
        symbols.push_back(s_5);
    if (s_6.m_type)
        symbols.push_back(s_6);
    if (s_7.m_type)
        symbols.push_back(s_7);
    if (s_8.m_type)
        symbols.push_back(s_8);
    add_prod(nonterm_index, symbols);
}


Cfg::Symbol Cfg::act(int method_index, int arg_1, int arg_2, int arg_3,
                     int arg_4, int arg_5, int arg_6, int arg_7)
{
    std::vector<int> args;
    if (arg_1 > -2)
        args.push_back(arg_1);
    if (arg_2 > -2)
        args.push_back(arg_2);
    if (arg_3 > -2)
        args.push_back(arg_3);
    if (arg_4 > -2)
        args.push_back(arg_4);
    if (arg_5 > -2)
        args.push_back(arg_5);
    if (arg_6 > -2)
        args.push_back(arg_6);
    if (arg_7 > -2)
        args.push_back(arg_7);
    return Symbol{method_index, args};
}


Cfg::FirstSets::FirstSets(const Cfg& g):
    m_grammar{g}
{
    m_terminals.resize(g.m_rules.size());
    m_nullable.resize(g.m_rules.size());
    bool again;
    do {
        again = false;
        for (unsigned i = 0; i < g.m_rules.size(); ++i) {
            const Rule& r = g.m_rules[i];
            std::set<int> &t = m_terminals[i];
            for (unsigned j = 0; j < r.m_productions.size(); ++j) {
                unsigned n = t.size();
                Item item{int(i), int(j), 0};
                if (include_first_set(item, t) && !m_nullable[i]) {
                    again = m_nullable[i] = true;
                }
                else if (t.size() > n) {
                    again = true;
                }
            }
        }
    }
    while (again);
}


bool Cfg::FirstSets::include_first_set(const Item& item, std::set<int>& t) const
{
    const Production& prod = m_grammar.m_rules[item.rule].m_productions[item.production];
    unsigned i;
    for (i = item.position; i < prod.m_symbols.size(); ++i) {
        const Symbol& sym = prod.m_symbols[i];
        if (sym.m_type == Symbol::terminal) {
            t.insert(sym.m_index);
            break;
        }
        if (sym.m_type == Symbol::nonterminal) {
            set_include(m_terminals[sym.m_index], t);
            if (!m_nullable[sym.m_index])
                break;
        }
    }
    return (i == prod.m_symbols.size());
}


std::string Cfg::print_terminal(int i) const
{
    if (i < 0)
        return "<eoi>";
    return ascii_toupper(m_terminals[i]);
}


std::string Cfg::print_nonterminal(int i) const
{
    return ascii_tolower(m_rules[i].m_name);
}


std::string Cfg::print_production(int i, int j) const
{
    return print_nonterminal(i) + " -> " + print_production_right_side(m_rules[i].m_productions[j]);
}


std::string Cfg::print_item(const Item& i) const
{
    return print_nonterminal(i.rule) + " -> " +
        print_production_right_side(m_rules[i.rule].m_productions[i.production], i.position);
}


std::string Cfg::FirstSets::print(int width) const
{
    Text::Table table;
    table.get_col(0).set_width(1);
    table.get_col(1).set_width(4);
    table.get_cell(0,0).set_text("Nonterminal");
    table.get_cell(0,1).set_text("First set");
    for (unsigned i = 0; i < m_terminals.size(); ++i) {
        const std::set<int>& t = m_terminals[i];
        table.get_cell(i+1, 0).set_text(m_grammar.print_nonterminal(i));
        std::string s;
        for (auto j = t.begin(); j != t.end(); ++j) {
            if (j != t.begin())
                s += " ";
            s += m_grammar.print_terminal(*j);
        }
        if (m_nullable[i]) {
            if (s.size())
                s += " ";
            s += "<epsilon>";
        }
        table.get_cell(i+1, 1).set_text(s);
    }

    return table.print(width, 2, true);
}


Cfg::FollowSets::FollowSets(const FirstSets& f):
    m_grammar(f.m_grammar)
{
    m_terminals.resize(m_grammar.m_rules.size());
    m_terminals[0].insert(-1);
    bool again;
    do {
        again = false;
        for (unsigned i = 0; i < m_grammar.m_rules.size(); ++i) {
            const Rule& rule = m_grammar.m_rules[i];
            for (unsigned j = 0; j < rule.m_productions.size(); ++j) {
                const Production& prod = rule.m_productions[j];
                for (unsigned k = 0; k < prod.m_symbols.size(); ++k) {
                    const Symbol& sym = prod.m_symbols[k];
                    if (sym.m_type != Symbol::nonterminal)
                        continue;
                    std::set<int>& t = m_terminals[sym.m_index];
                    unsigned n = t.size();
                    Item item{int(i), int(j), int(k+1)};
                    if (f.include_first_set(item, t))
                        set_include(m_terminals[i], t);
                    if (t.size() > n)
                        again = true;
                }
            }
        }
    }
    while (again);
}


std::string Cfg::FollowSets::print(int width) const
{
    Text::Table table;
    table.get_col(0).set_width(1);
    table.get_col(1).set_width(4);
    table.get_cell(0,0).set_text("Nonterminal");
    table.get_cell(0,1).set_text("Follow set");
    for (unsigned i = 0; i < m_terminals.size(); ++i) {
        const std::set<int>& t = m_terminals[i];
        table.get_cell(i+1, 0).set_text(m_grammar.print_nonterminal(i));
        std::string s;
        for (int term: t) {
            if (term < 0)
                continue;
            if (!s.empty())
                s += " ";
            s += m_grammar.print_terminal(term);
        }
        if (!t.empty() && *t.begin() < 0) {
            if (t.size() > 1)
                s += " ";
            s += "<eoi>";
        }
        table.get_cell(i+1, 1).set_text(s);
    }

    return table.print(width, 2, true);
}


std::string Cfg::print(int width) const
{
    Text::Table table;
    table.get_col(0).set_width(10);
    table.get_col(1).set_width( 1);
    table.get_col(2).set_width(39);

    int l = 0;
    for (unsigned i = 0; i < m_rules.size(); ++i) {
        if (i > 0) {
            table.get_cell(l,0).set_text(" ");
            ++l;
        }
        table.get_cell(l,0).set_text(print_nonterminal(i));
        table.get_cell(l,1).set_text("=");
        const Rule& rule = m_rules[i];
        for (unsigned j = 0; j < rule.m_productions.size(); ++j) {
            if (j > 0)
                table.get_cell(l,1).set_text("|");
            table.get_cell(l,2).set_text(print_production_right_side(rule.m_productions[j]));
            ++l;
        }
        if (!rule.m_productions.size())
            ++l;
    }

    return table.print(width, 1);
}


void Cfg::introduce_new_start_symbol()
{
    if (!m_rules.size())
        throw std::invalid_argument("Original grammar must have "
                                    "at least one nonterminal");
    std::vector<int> m(m_rules.size());
    for (unsigned l = 0; l < m_rules.size(); ++l)
        m[l] = l+1;
    update_rule_indices(m);
    m_rules.insert(m_rules.begin(), Rule{choose_unique_name(m_rules[0].m_name, -1)});
    add_prod(0, nont(1));
}


void Cfg::eliminate_epsilon_productions()
{
    std::vector<bool> nullable;
    std::vector<std::vector<int> > null_actions;
    find_nullable_nonterminals(nullable, null_actions);

    // If the start symbol is nullable it may not occur on the right hand side
    // of any production
    if (nullable[0]) {
        for (Rule& rule: m_rules) {
            for (Production& prod: rule.m_productions) {
                for (Symbol& sym: prod.m_symbols) {
                    if (sym.m_type == Symbol::nonterminal && sym.m_index == 0) {
                        // Make a brand new start symbol
                        nullable.insert(nullable.begin(), true);
                        null_actions.insert(null_actions.begin(), null_actions[0]);
                        introduce_new_start_symbol();
                        goto rewrite;
                    }
                }
            }
        }
    }

    // Rewrite each rule
  rewrite:
    for (Rule &rule: m_rules) {
        std::vector<Production> new_productions;
        for (Production& prod: rule.m_productions) {
            std::vector<Symbol> prefix;
            add_nullable_combinations(0, true, prod, nullable, null_actions, prefix,
                                      new_productions);
        }
        rule.m_productions = new_productions;
    }

    // Is the start symbol nullable
    if (nullable[0]) {
        std::vector<Symbol> epsilon;
        for (int a: null_actions[0])
            epsilon.push_back(act(a));
        add_prod(0, epsilon);
    }
}


void Cfg::eliminate_cycles()
{
    Cfg g = *this;
    g.eliminate_epsilon_productions();
    int cycles = 0;
    for (;;) {
        // Find a cycle
        std::list<std::pair<int, int>> cycle;
        // 0=unvisited, 1=in current path, 2=visited but no longer in current path
        std::vector<int> visited_rules(g.m_rules.size());
        for (int i = 0; i < int(g.m_rules.size()); ++i) {
            if (g.eliminate_cycles_visit(i, visited_rules, cycle) == -2)
                break;
        }
        if (!cycle.size())
            break;
        ++cycles;

        std::cerr << "Found cycle: " << g.m_rules[cycle.front().first].m_name;
        for (auto& p: cycle)
            std::cerr << " -> " << g.print_production_right_side(g.m_rules[p.first].m_productions[p.second]);
        std::cerr << "\n";

        std::set<std::pair<int, int>> cycle_productions;
        std::set<int> cycle_nonterminals;
        for (auto& p: cycle) {
            cycle_productions.insert(p);
            cycle_nonterminals.insert(p.first);
        }

        // Rewrite each rule to eliminate the cycle
        for (int i = 0; i < int(g.m_rules.size()); ++i) {
            Rule& rule = g.m_rules[i];
            std::vector<Production> new_productions;
            for (int j = 0; j < int(rule.m_productions.size()); ++j) {
                Production& prod = rule.m_productions[j];
                auto q = std::make_pair(i,j);
                if (cycle_productions.find(q) != cycle_productions.end()) {
                    if (q != cycle.back())
                        new_productions.push_back(prod);
                }
                else {
                    Production n;
                    for (Symbol& sym: prod.m_symbols) {
                        if (   (sym.m_type == Symbol::nonterminal &&
                                cycle_nonterminals.find(sym.m_index) != cycle_nonterminals.end())) {
                            n.m_symbols.push_back(nont(cycle.front().first));
                        }
                        else {
                            n.m_symbols.push_back(sym);
                        }
                    }
                    new_productions.push_back(n);
                }
            }

            rule.m_productions = new_productions;
        }
    }

    if (cycles != 0)
        *this = g;
}


void Cfg::eliminate_midrule_actions()
{
    unsigned n = m_rules.size();
    for (unsigned i = 0; i < n; ++i) {
        Rule* r = &m_rules[i];
        for (unsigned j = 0; j < r->m_productions.size(); ++j) {
            Production* p = &r->m_productions[j];
            for (unsigned k = 0; k+1 < p->m_symbols.size(); ++k) {
                Symbol* s = &p->m_symbols[k];
                if (s->m_type != Symbol::action)
                    continue;
                int a = define_nonterminal(choose_unique_name("action", 1));
                // rules vector may be relocated so we need to aquire a new
                // pointers.
                r = &m_rules[i];
                p = &r->m_productions[j];
                s = &p->m_symbols[k];
                add_prod(a, *s);
                s->m_type = Symbol::nonterminal;
                s->m_index = a;
                s->m_args.clear();
            }
        }
    }
}
