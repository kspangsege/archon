// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef ARCHON_X_CLI_X_DETAIL_X_SPEC_PARSER_HPP
#define ARCHON_X_CLI_X_DETAIL_X_SPEC_PARSER_HPP

/// \file


#include <cstddef>
#include <utility>
#include <string_view>
#include <string>
#include <vector>
#include <locale>

#include <archon/base/span.hpp>
#include <archon/base/seed_memory_buffer.hpp>
#include <archon/base/char_mapper.hpp>
#include <archon/base/quote.hpp>
#include <archon/base/format.hpp>
#include <archon/base/format_enc.hpp>
#include <archon/base/format_as.hpp>
#include <archon/cli/exception.hpp>
#include <archon/cli/detail/pattern_symbol.hpp>
#include <archon/cli/detail/spec.hpp>


namespace archon::cli::detail {


template<class C, class T> class SpecParser :
        private base::BasicCharMapper<C, T> {
public:
    using char_type        = C;
    using traits_type      = T;
    using string_view_type = std::basic_string_view<C, T>;
    using spec_type        = Spec<C, T>;

    using OptionForm = typename spec_type::OptionForm;
    using ArgSpec    = typename spec_type::ArgSpec;

    SpecParser(const std::locale&);

    void parse_pattern(string_view_type pattern, spec_type&, std::size_t pattern_ndx,
                       bool is_deleg);
    auto parse_option_forms(std::size_t option_index, string_view_type forms,
                            base::SeedMemoryBuffer<OptionForm>&) -> base::Span<OptionForm>;
    auto parse_option_arg(std::size_t option_index, string_view_type arg) -> ArgSpec;

private:
    struct ErrorQualifier {
        std::size_t index;
        const char* entity;
        const char* component;
    };

    struct Token {
        enum class Type { keyword, short_option, long_option, value_slot,
                          left_sq_bracket, right_sq_bracket,
                          left_parenthesis, right_parenthesis,
                          ellipsis, vertical_bar, end_of_input };
        Type type;
        string_view_type lexeme;
        bool preceded_by_space = false;
    };

    struct Node {
        enum class Type { keyword, short_option, long_option, value_slot,
                          optional, repetition, concatenation, disjunction };
        Type type;
        std::size_t left = 0, right = 0;
        string_view_type lexeme;
    };

    struct Result {
        std::size_t node_index;
        int precedence;
    };

    // Designates a range of elements in m_position_set_elems
    using PositionSet = std::pair<std::size_t, std::size_t>;

    struct NfaResult {
        bool nullable;
        PositionSet firstpos, lastpos;
    };

    const std::locale m_locale;
    const std::ctype<char_type>& m_ctype;
    const char_type m_space, m_dash, m_dot, m_greater_than;
    ErrorQualifier m_error_qualifier;
    const char_type* m_curr;
    const char_type* m_end;
    Token m_next_token;
    std::vector<Node> m_nodes;
    std::vector<std::size_t> m_position_set_elems;
    bool m_has_value_slot;

    bool reset(ErrorQualifier error_qualifier, string_view_type spec);
    Result parse();
    Result parse_disjunction();
    Result parse_concatenation();
    Result parse_repetition();
    Result parse_element();
    void extract_next_token();
    template<class... P>
    [[noreturn]] void error(const char* message, const P&... params) const;
    bool get_option_forms(std::size_t node_index,
                          base::SeedMemoryBufferContents<OptionForm>&) const;
    bool get_option_arg_spec(std::size_t node_index, bool optional, ArgSpec&) const noexcept;
    NfaResult pattern_to_nfa(std::size_t node_index, spec_type&, std::size_t pattern_index);
    void register_followpos(PositionSet, std::size_t, spec_type&) const;
    void register_followpos(PositionSet, PositionSet, spec_type&) const;
    PositionSet make_position_set(std::size_t pos);
    PositionSet position_set_union(PositionSet, PositionSet);
};








// Implementation


template<class C, class T> inline SpecParser<C, T>::SpecParser(const std::locale& locale) :
    base::BasicCharMapper<C, T>(locale),
    m_locale(locale),
    m_ctype(std::use_facet<std::ctype<char_type>>(m_locale)), // Throws
    m_space(this->widen(' ')), // Throws
    m_dash(this->widen('-')), // Throws
    m_dot(this->widen('.')), // Throws
    m_greater_than(this->widen('>')) // Throws
{
}


template<class C, class T>
void SpecParser<C, T>::parse_pattern(string_view_type pattern, spec_type& spec,
                                     std::size_t pattern_ndx, bool is_deleg)
{
    PatternSymbol symbol = {}; // The value has no meaning in this case (final state)
    string_view_type lexeme;
    if (reset({ pattern_ndx, "pattern", "pattern" }, pattern)) { // Throws
        Result result = parse(); // Throws
        if (result.precedence == 0)
            error("Unparenthesized disjunction not allowed"); // Throws
        if (m_has_value_slot && is_deleg)
            error("Value slots are not allowed in delegating patterns"); // Throws
        NfaResult result_2 = pattern_to_nfa(result.node_index, spec, pattern_ndx); // Throws
        PositionSet firstpos = result_2.firstpos;
        PositionSet lastpos  = result_2.lastpos;
        for (std::size_t i = firstpos.first; i < firstpos.second; ++i) {
            std::size_t pos = m_position_set_elems[i];
            spec.register_startpos(pos); // Throws
        }
        std::size_t term_pos = spec.create_position(pattern_ndx, symbol, lexeme); // Throws
        if (result_2.nullable)
            spec.register_startpos(term_pos); // Throws
        register_followpos(lastpos, term_pos, spec); // Throws
    }
    else {
        std::size_t term_pos = spec.create_position(pattern_ndx, symbol, lexeme); // Throws
        spec.register_startpos(term_pos); // Throws
    }
}


template<class C, class T>
auto SpecParser<C, T>::parse_option_forms(std::size_t option_index, string_view_type forms,
                                          base::SeedMemoryBuffer<OptionForm>& buffer) ->
    base::Span<OptionForm>
{
    base::SeedMemoryBufferContents<OptionForm> buffer_2(buffer);
    if (reset({ option_index, "option", "option forms" }, forms)) { // Throws
        Result result = parse(); // Throws
        if (!get_option_forms(result.node_index, buffer_2)) // Throws
            error("Syntax error"); // Throws
    }
    return base::Span(buffer_2);
}


template<class C, class T>
auto SpecParser<C, T>::parse_option_arg(std::size_t option_index, string_view_type arg) -> ArgSpec
{
    ArgSpec arg_2;
    if (reset({ option_index, "option", "option argument" }, arg)) { // Throws
        Result result = parse(); // Throws
        if (!get_option_arg_spec(result.node_index, false, arg_2))
            error("Syntax error"); // Throws
    }
    return arg_2;
}


template<class C, class T>
bool SpecParser<C, T>::reset(ErrorQualifier error_qualifier, string_view_type spec)
{
    m_error_qualifier = error_qualifier;
    m_curr = spec.begin();
    m_end = spec.end();
    m_nodes.clear();
    m_position_set_elems.clear();
    m_has_value_slot = false;
    extract_next_token(); // Throws
    return (m_next_token.type != Token::Type::end_of_input);
}


template<class C, class T> inline auto SpecParser<C, T>::parse() -> Result
{
    Result result = parse_disjunction(); // Throws
    if (m_next_token.type != Token::Type::end_of_input)
        error("Unexpected token"); // Throws
    return result;
}


template<class C, class T> auto SpecParser<C, T>::parse_disjunction() -> Result
{
    Result result = parse_concatenation(); // Throws
    for (;;) {
        if (m_next_token.type != Token::Type::vertical_bar)
            break;
        extract_next_token(); // Throws
        Node node;
        node.type = Node::Type::disjunction;
        node.left = result.node_index;
        node.right = parse_concatenation().node_index; // Throws
        result.node_index = m_nodes.size();
        result.precedence = 0;
        m_nodes.push_back(node); // Throws
    }
    return result;
}


template<class C, class T> auto SpecParser<C, T>::parse_concatenation() -> Result
{
    Result result = parse_repetition(); // Throws
    for (;;) {
        bool is_concat = (m_next_token.type == Token::Type::keyword         ||
                          m_next_token.type == Token::Type::short_option    ||
                          m_next_token.type == Token::Type::long_option     ||
                          m_next_token.type == Token::Type::value_slot      ||
                          m_next_token.type == Token::Type::left_sq_bracket ||
                          m_next_token.type == Token::Type::left_parenthesis);
        if (!is_concat)
            break;
        if (!m_next_token.preceded_by_space)
            error("Need space between elements"); // Throws
        Node node;
        node.type = Node::Type::concatenation;
        node.left = result.node_index;
        node.right = parse_repetition().node_index; // Throws
        result.node_index = m_nodes.size();
        result.precedence = 1;
        m_nodes.push_back(node); // Throws
    }
    return result;
}


template<class C, class T> auto SpecParser<C, T>::parse_repetition() -> Result
{
    Result result = parse_element(); // Throws
    for (;;) {
        if (m_next_token.type != Token::Type::ellipsis)
            break;
        extract_next_token(); // Throws
        Node node;
        node.type = Node::Type::repetition;
        node.left = result.node_index;
        result.node_index = m_nodes.size();
        result.precedence = 2;
        m_nodes.push_back(node); // Throws
    }
    return result;
}


template<class C, class T> auto SpecParser<C, T>::parse_element() -> Result
{
    typename Node::Type node_type;
    if (m_next_token.type == Token::Type::keyword) {
        node_type = Node::Type::keyword;
        goto token_node;
    }
    if (m_next_token.type == Token::Type::short_option) {
        node_type = Node::Type::short_option;
        goto token_node;
    }
    if (m_next_token.type == Token::Type::long_option) {
        node_type = Node::Type::long_option;
        goto token_node;
    }
    if (m_next_token.type == Token::Type::value_slot) {
        m_has_value_slot = true;
        node_type = Node::Type::value_slot;
        goto token_node;
    }
    if (m_next_token.type == Token::Type::left_sq_bracket) {
        extract_next_token(); // Throws
        Node node;
        node.type = Node::Type::optional;
        node.left = parse_disjunction().node_index; // Throws
        if (m_next_token.type != Token::Type::right_sq_bracket)
            error("Unclosed square bracket"); // Throws
        extract_next_token(); // Throws
        std::size_t node_index = m_nodes.size();
        m_nodes.push_back(node); // Throws
        return { node_index, 3 };
    }
    if (m_next_token.type == Token::Type::left_parenthesis) {
        extract_next_token(); // Throws
        std::size_t node_index = parse_disjunction().node_index; // Throws
        if (m_next_token.type != Token::Type::right_parenthesis)
            error("Unclosed parenthesis"); // Throws
        extract_next_token(); // Throws
        return { node_index, 3 };
    }
    error("Bad start of expression"); // Throws

  token_node:
    Node node;
    node.type = node_type;
    node.lexeme = m_next_token.lexeme;
    extract_next_token(); // Throws
    std::size_t node_index = m_nodes.size();
    m_nodes.push_back(node); // Throws
    return { node_index, 3 };
}


template<class C, class T> void SpecParser<C, T>::extract_next_token()
{
    const char_type* i = m_curr;
    Token token;
    char ch;

  again:
    if (i == m_end) {
        token.type = Token::Type::end_of_input;
        goto good;
    }

    ch = this->narrow(*i, '\0');
    ++i;
    switch (ch) {
        case ' ':
            while (i != m_end && *i == m_space)
                ++i;
            m_curr = i;
            token.preceded_by_space = true;
            goto again;

        case '-':
            if (i != m_end) {
                if (*i == m_dash) {
                    // Long option
                    do i = m_ctype.scan_not(std::ctype_base::alnum, i + 1, m_end);
                    while (i != m_end && *i == m_dash);
                    token.type = Token::Type::long_option;
                    goto good;
                }
                if (*i != m_space) {
                    // Short option
                    ++i;
                    token.type = Token::Type::short_option;
                    goto good;
                }
            }
            break;

        case '<':
            i = std::find(i, m_end, m_greater_than);
            if (i != m_end) {
                ++i;
                token.type = Token::Type::value_slot;
                goto good;
            }
            error("Missing closing `>` in value slot specification"); // Throws

        case '(':
            token.type = Token::Type::left_parenthesis;
            goto good;

        case ')':
            token.type = Token::Type::right_parenthesis;
            goto good;

        case '[':
            token.type = Token::Type::left_sq_bracket;
            goto good;

        case ']':
            token.type = Token::Type::right_sq_bracket;
            goto good;

        case '.':
            if (i != m_end && *i == m_dot) {
                ++i;
                if (i != m_end && *i == m_dot) {
                    ++i;
                    token.type = Token::Type::ellipsis;
                    goto good;
                }
            }
            goto invalid_token;

        case '|':
            token.type = Token::Type::vertical_bar;
            goto good;
    }

    if (m_ctype.is(std::ctype_base::alnum, ch)) {
        for (;;) {
            i = m_ctype.scan_not(std::ctype_base::alnum, i, m_end);
            if (ARCHON_LIKELY(i == m_end || *i != m_dash))
                break;
            ++i;
        }
        token.type = Token::Type::keyword;
        goto good;
    }

  invalid_token:
    error("Invalid token (%s)",
          base::quoted_s(string_view_type(m_curr, std::size_t(i - m_curr)))); // Throws

  good:
    token.lexeme = { m_curr, std::size_t(i - m_curr) };
    m_curr = i;
    m_next_token = token;
}


template<class C, class T> template<class... P>
inline void SpecParser<C, T>::error(const char* message, const P&... params) const
{
    std::string message_2 =
        base::format_enc<char_type>(m_locale, "Error in %s %s specification: "
                                    "Failed to parse %s specification: %s",
                                    base::as_ordinal(m_error_qualifier.index + 1),
                                    m_error_qualifier.entity, m_error_qualifier.component,
                                    base::formatted(message, params...)); // Throws
    throw BadCommandLineInterfaceSpec(std::move(message_2));
}


template<class C, class T>
bool SpecParser<C, T>::get_option_forms(std::size_t node_index,
                                        base::SeedMemoryBufferContents<OptionForm>& buffer)
    const
{
    const Node& node = m_nodes[node_index];
    switch (node.type) {
        case Node::Type::keyword:
            break;
        case Node::Type::short_option:
            buffer.push_back({ false, node.lexeme }); // Throws
            return true;
        case Node::Type::long_option:
            buffer.push_back({ true, node.lexeme }); // Throws
            return true;
        case Node::Type::value_slot:
        case Node::Type::optional:
        case Node::Type::repetition:
            break;
        case Node::Type::concatenation:
            return (get_option_forms(node.left,  buffer) &&
                    get_option_forms(node.right, buffer)); // Throws
        case Node::Type::disjunction:
            break;
    }
    return false;
}


template<class C, class T>
bool SpecParser<C, T>::get_option_arg_spec(std::size_t node_index, bool optional,
                                           ArgSpec& arg) const noexcept
{
    const Node& node = m_nodes[node_index];
    switch (node.type) {
        case Node::Type::keyword:
        case Node::Type::short_option:
        case Node::Type::long_option:
            break;
        case Node::Type::value_slot:
            arg.allow  = true;
            arg.need   = !optional;
            arg.lexeme = node.lexeme;
            return true;
        case Node::Type::optional:
            return get_option_arg_spec(node.left, true, arg);
        case Node::Type::repetition:
        case Node::Type::concatenation:
        case Node::Type::disjunction:
            break;
    }
    return false;
}


template<class C, class T>
auto SpecParser<C, T>::pattern_to_nfa(std::size_t node_index, spec_type& spec,
                                      std::size_t pattern_index) -> NfaResult
{
    const Node& node = m_nodes[node_index];
    string_view_type lexeme = node.lexeme;
    bool is_long = false;
    PatternSymbol symbol;
    switch (node.type) {
        case Node::Type::keyword: {
            std::size_t index = spec.ensure_keyword(lexeme); // Throws
            symbol = PatternSymbol::keyword(index);
            goto leaf;
        }
        case Node::Type::short_option: {
            goto option;
        }
        case Node::Type::long_option: {
            is_long = true;
            goto option;
        }
        case Node::Type::value_slot: {
            symbol = PatternSymbol::value();
            goto leaf;
        }
        case Node::Type::optional: {
            NfaResult result = pattern_to_nfa(node.left, spec, pattern_index); // Throws
            result.nullable = true;
            return result;
        }
        case Node::Type::repetition: {
            NfaResult result = pattern_to_nfa(node.left, spec, pattern_index); // Throws
            register_followpos(result.lastpos, result.firstpos, spec); // Throws
            return result;
        }
        case Node::Type::concatenation: {
            NfaResult result_1 = pattern_to_nfa(node.left, spec, pattern_index); // Throws
            NfaResult result_2 = pattern_to_nfa(node.right, spec, pattern_index); // Throws
            register_followpos(result_1.lastpos, result_2.firstpos, spec); // Throws
            NfaResult result;
            result.nullable = (result_1.nullable && result_2.nullable);
            if (!result_1.nullable) {
                result.firstpos = result_1.firstpos;
            }
            else {
                result.firstpos =
                    position_set_union(result_1.firstpos, result_2.firstpos); // Throws
            }
            if (!result_2.nullable) {
                result.lastpos = result_2.lastpos;
            }
            else {
                result.lastpos =
                    position_set_union(result_1.lastpos, result_2.lastpos); // Throws
            }
            return result;
        }
        case Node::Type::disjunction: {
            NfaResult result_1 = pattern_to_nfa(node.left, spec, pattern_index); // Throws
            NfaResult result_2 = pattern_to_nfa(node.right, spec, pattern_index); // Throws
            NfaResult result;
            result.nullable = (result_1.nullable || result_2.nullable);
            result.firstpos = position_set_union(result_1.firstpos, result_2.firstpos); // Throws
            result.lastpos  = position_set_union(result_1.lastpos, result_2.lastpos); // Throws
            return result;
        }
    }
    return {};

  option:
    {
        std::size_t index =
            spec.ensure_pattern_option({ is_long, lexeme }, pattern_index); // Throws
        symbol = PatternSymbol::proto_option(index);
    }

  leaf:
    std::size_t pos = spec.create_position(pattern_index, symbol, lexeme); // Throws
    NfaResult result;
    result.nullable = false;
    result.firstpos = make_position_set(pos); // Throws
    result.lastpos  = result.firstpos;
    return result;
}


template<class C, class T>
inline void SpecParser<C, T>::register_followpos(PositionSet a, std::size_t b,
                                                 spec_type& spec) const
{
    for (std::size_t i = a.first; i < a.second; ++i) {
        std::size_t pos_1 = m_position_set_elems[i];
        std::size_t pos_2 = b;
        spec.register_followpos(pos_1, pos_2); // Throws
    }
}


template<class C, class T>
void SpecParser<C, T>::register_followpos(PositionSet a, PositionSet b, spec_type& spec) const
{
    for (std::size_t i = a.first; i < a.second; ++i) {
        std::size_t pos_1 = m_position_set_elems[i];
        for (std::size_t j = b.first; j < b.second; ++j) {
            std::size_t pos_2 = m_position_set_elems[j];
            spec.register_followpos(pos_1, pos_2); // Throws
        }
    }
}


template<class C, class T>
inline auto SpecParser<C, T>::make_position_set(std::size_t pos) -> PositionSet
{
    std::size_t pos_set_begin = m_position_set_elems.size();
    m_position_set_elems.push_back(pos); // Throws
    std::size_t pos_set_end = m_position_set_elems.size();
    return { pos_set_begin, pos_set_end };
}


template<class C, class T>
auto SpecParser<C, T>::position_set_union(PositionSet a, PositionSet b) -> PositionSet
{
    std::size_t pos_set_begin = m_position_set_elems.size();
    std::size_t i = a.first, j = b.first;
    std::size_t pos_1, pos_2;
  begin_1:
    if (i == a.second)
        goto end_1;
    pos_1 = m_position_set_elems[i];
  begin_2:
    if (j == b.second)
        goto end_2;
    pos_2 = m_position_set_elems[j];
  begin_3:
    if (pos_1 < pos_2) {
        m_position_set_elems.push_back(pos_1); // Throws
        ++i;
        if (i == a.second)
            goto end_1;
        pos_1 = m_position_set_elems[i];
        goto begin_3;
    }
    if (pos_1 > pos_2) {
        m_position_set_elems.push_back(pos_2); // Throws
        ++j;
        goto begin_2;
    }
    m_position_set_elems.push_back(pos_1); // Throws
    ++i;
    ++j;
    goto begin_1;
  end_1:
    while (j < b.second) {
        std::size_t pos = m_position_set_elems[j];
        m_position_set_elems.push_back(pos); // Throws
        ++j;
    }
    goto end_3;
  end_2:
    while (i < a.second) {
        std::size_t pos = m_position_set_elems[i];
        m_position_set_elems.push_back(pos); // Throws
        ++i;
    }
    goto end_3;
  end_3:
    std::size_t pos_set_end = m_position_set_elems.size();
    return { pos_set_begin, pos_set_end };
}


} // namespace archon::cli::detail

#endif // ARCHON_X_CLI_X_DETAIL_X_SPEC_PARSER_HPP
