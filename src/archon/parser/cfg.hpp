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

#ifndef ARCHON_PARSER_CFG_HPP
#define ARCHON_PARSER_CFG_HPP

#include <string>
#include <utility>
#include <vector>
#include <map>
#include <set>
#include <list>

namespace archon {
namespace parser {

/// Context free grammars
///
/// The start symbol is always the left hand side of the first rule.
///
/// Still to be done:
///   - Conversion to Chomsky Normal Form
///       http://www.wikipedia.com/wiki/Chomsky+Normal+Form
///       http://muldoon.cipic.ucdavis.edu/~jchen007/UCD/ECS120/Notes/LectureNotes11.pdf
class Cfg {
public:
    class Rule;
    class FirstSets;
    class FollowSets;

    struct Item {
        int rule;
        int production;
        int position;
    };

    class Symbol {
    public:
        enum Type {
            nil = 0,
            terminal,
            nonterminal,
            action
        };

        Type get_type() const { return m_type; }
        int get_index() const { return m_index; }
        const std::vector<int>& get_args() const { return m_args; }

    private:
        Type m_type;
        int m_index; // -1 is 'null', -2 is 'copy' and -3 is 'concat'
        std::vector<int> m_args; // For actions only

        Symbol() noexcept: m_type(nil) {}
        Symbol(Type type, int index): m_type(type), m_index(index) {}
        Symbol(int index, const std::vector<int>& args):
            m_type(action), m_index(index), m_args(args) {}

        friend class Cfg;
        friend class FirstSets;
        friend class FollowSets;
    };

    class Production {
    public:
        Production(const std::vector<Symbol>& symbols): m_symbols(symbols) {}

        int get_num_symbols() const { return m_symbols.size(); }
        const Symbol& get_symbol(int i) const { return m_symbols[i]; }

    private:
        Production() {}

        std::vector<Symbol> m_symbols;

        friend class Cfg;
        friend class Rule;
        friend class FirstSets;
        friend class FollowSets;
    };

    class Rule {
    public:
        Rule(const std::string& name): m_name(name) {}

        int get_num_productions() const { return m_productions.size(); }
        const Production& get_production(int i) const { return m_productions[i]; }

    private:
        std::string m_name;
        std::vector<Production> m_productions;

        void add_production(const std::vector<Symbol>& symbols)
        {
            m_productions.emplace_back(symbols);
        }

        friend class Cfg;
        friend class FirstSets;
        friend class FollowSets;
    };

    class Actor {
    public:
        /// Get the number of user-defined methods.
        virtual int get_num_methods() const = 0;

        /// \param method_index -3 for 'concat', -2 for 'copy' and -1 for 'null'
        /// otherwise user-defined.
        virtual int get_method_arity(int method_index) const = 0;

        /// \param method_index -3 for 'concat', -2 for 'copy' and -1 for 'null'
        /// otherwise user-defined.
        virtual std::string get_method_name(int method_index) const = 0;

        virtual ~Actor() {}
    };


    Cfg(const Actor* = nullptr, std::locale = std::locale::classic());

    int get_num_terminals() const { return m_terminals.size(); }
    int get_num_rules() const { return m_rules.size(); }
    const Rule& get_rule(int i) const { return m_rules[i]; }

    int define_terminal(const std::string& name);
    int define_nonterminal(const std::string& name);

    static Symbol nil() noexcept { return {}; }

    void add_prod(int nonterm_index, const std::vector<Symbol>&);
    void add_prod(int nonterm_index, Symbol=nil(), Symbol=nil(),
                 Symbol=nil(), Symbol=nil(), Symbol=nil(),
                 Symbol=nil(), Symbol=nil(), Symbol=nil());

    /// Make a terminal symbol from a terminal index.
    static Symbol term(int terminal_index)
    {
        return Symbol{Symbol::terminal, terminal_index};
    }

    /// Make a nonterminal symbol from a nonterminal index.
    static Symbol nont(int nonterminal_index)
    {
        return Symbol{Symbol::nonterminal, nonterminal_index};
    }

    /// Make an action symbol from a method index and argument references. An
    /// argument reference of zero refers to the symbol immediately preceding
    /// this action. A value of 1 refers to the symbol before that one and so
    /// on. A value of -1 indicates that a null argument should be passed to the
    /// method. A value of -2 is skipped and does not count as an argument.
    static Symbol act(int method_index,
                      int arg_1 = -2, int arg_2 = -2, int arg_3 = -2,
                      int arg_4 = -2, int arg_5 = -2, int arg_6 = -2,
                      int arg_7 = -2);

    /// Special action that return the null reference.
    static Symbol null() { return act(-1); }

    /// Special action that copies attributes.
    static Symbol copy(int arg) { return act(-2, arg); }

    /// Special action that concattenates strings.
    static Symbol concat(int arg1, int arg2) { return act(-3, arg1, arg2); }

    class FirstSets {
    public:
        FirstSets(const Cfg&);

        /// Add the first set of the symbols after the position in the item to
        /// the argument set.
        ///
        /// \return true if the symbols after the item position can derive
        /// epsilon.
        bool include_first_set(const Item&, std::set<int>&) const;

        std::string print(int width = 0) const;

    private:
        const Cfg& m_grammar;
        std::vector<std::set<int> > m_terminals; // One entry per nonterminal
        std::vector<bool> m_nullable; // One entry per nonterminal

        friend class FollowSets;
    };

    class FollowSets {
    public:
        FollowSets(const FirstSets&);
        const std::set<int>& get(int i) const { return m_terminals[i]; }
        std::string print(int width = 0) const;

    private:
        const Cfg& m_grammar;
        std::vector<std::set<int>> m_terminals; // -1 represents EOI
    };

    /// \param index A negative value will be interpreted as the imaginary EOI
    /// terminal.
    std::string print_terminal(int index) const;

    std::string print_nonterminal(int index) const;
    std::string print_production(int rule, int production) const;
    std::string print_item(const Item&) const;
    std::string print(int width = 0) const;

    void introduce_new_start_symbol();

    /// CURRENTLY DOES NOT REWRITE ACTIONS PROPERLY!!!!!
    ///
    /// Convert the grammar into an equivalent epsilon-free grammar.
    ///
    /// We say that a grammar is epsilon-free if either it has no epsilon
    /// productions or there is exactly one epsilon production for the start
    /// symbol and then the start symbol does not appear on the right side of
    /// any production.
    ///
    /// The new grammar will accept exactly the same language and the sequence
    /// of semantic actions that are to be evaluated for any given derivation
    /// will not be changed.
    ///
    /// This operation may introduce a new start symbol.
    ///
    /// This operation may leave the grammar with non-terminals that cannot
    /// produce anything, so you might want to run the
    /// 'eliminateDeadNonTerminals' method afterwards.
    ///
    /// This operation also may leave the grammar with duplicate productions, so
    /// you also might want to run the 'eliminateDuplicateProductions' method
    /// afterwards.
    ///
    ///
    /// Ambiguous nullability:
    ///
    /// This operation will fail if any non-terminal has ambiguous nullability,
    /// that is if there among the ways to derive null from the non-terminal are
    /// some that result in different sequences of actions to be performed. An
    /// example would be:
    ///
    ///  A -> B | C
    ///  B -> {action1} | b
    ///  C -> {action2} | c
    ///
    /// Note that both B nad C have epsilon productions but with different
    /// actions associated to them.
    void eliminate_epsilon_productions();

    /// CURRENTLY DOES NOT REWRITE ACTIONS PROPERLY!!!!!
    ///
    /// Convert the grammar into an equivalent cycle-free grammar.
    ///
    /// A grammar is cycle free if it has no cycles at all. A grammar has a
    /// cycle if for some non-terminal A there is a possible derivation
    /// A =>+ A. That is, a derivation from A to itself in one or more steps.
    ///
    /// Elimination procedure:
    ///
    /// To simplify things we will start this operation by deriving an
    /// epsilon-free grammar. From such a garmmar it is reasonably simple to
    /// find and eliminate cycles. In the search for cycles we only need to
    /// considder productions that have no terminal symbols and have exactly one
    /// non-terminal on the right side.
    ///
    /// A cycle in an unambiguous epsilon-free grammar always has the form:
    ///
    ///  A0 -> A1
    ///  A1 -> A2
    ///  .
    ///  .
    ///  .
    ///  An-1 -> An
    ///  An -> A0
    ///
    /// That is, the right sides consist of nothing but one non-terminal.
    ///
    /// One could imagine a cycle where one or more of the involved productions
    /// had semantic actions in them, but in that case the grammar would be
    /// ambiguous. If any such cycle exists in the grammar this method will
    /// report failure.
    ///
    /// To eliminate the cycle shown above from an epsilon-free grammar we need
    /// only to choose an Ai among the cyclic non-terminals then remove the
    /// production Ai-1 -> Ai or An -> A0 if i = 0 and then fix all productions
    /// that are not part of the cycle by replacing any occurance of Aj by Ai
    /// where 0 <= j <= n and j != i.
    ///
    /// What about productions like A2 -> A1 or A2 -> B and B -> A3?
    ///
    /// A cycle may never contain a non-terminal twice.
    void eliminate_cycles();

    /// Remove non-terminals that produce nothing and non-terminals that are not
    /// reachable from the starting symbol.

    // void eliminateDeadNonTerminals();

    /// Rewrite
    ///
    ///  A -> B C f(1) D E g(2, 5) F G h(3, 8)
    ///
    /// to
    ///
    ///  A -> B C M D E N F G h(3, 8)
    ///  M -> f(-1)
    ///  N -> g(-3, 0)
    void eliminate_midrule_actions();

private:
    void update_rule_indices(const std::vector<int>&);
    std::string choose_unique_name(std::string, int);
    void find_nullable_nonterminals(std::vector<bool>&, std::vector<std::vector<int>>&);
    void add_nullable_combinations(unsigned, bool,
                                   const Production&,
                                   const std::vector<bool>&,
                                   const std::vector<std::vector<int>>&,
                                   std::vector<Symbol>&,
                                   std::vector<Production>&);
    int eliminate_cycles_visit(int, std::vector<int>&, std::list<std::pair<int, int>>&);
    std::string print_production_right_side(const Production&, int mark = -1) const;

    std::vector<std::string> m_terminals;
    std::vector<Rule> m_rules;

    std::map<std::string, int> m_terminal_map;    // Maps terminal names to terminal indices.
    std::map<std::string, int> m_nonterminal_map; // Maps nonterminal names to rule indices.

    const Actor* m_actor; // Defines the known methods and knows how to call them. Is optional.

    std::locale m_locale; // Needed for integer formatting
};



// Implementation:

inline Cfg::Cfg(const Actor* actor, std::locale locale):
    m_actor{actor},
    m_locale{std::move(locale)}
{
}

} // namespace parser
} // namespace archon

#endif // ARCHON_PARSER_CFG_HPP
