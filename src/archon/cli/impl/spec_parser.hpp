// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef ARCHON_X_CLI_X_IMPL_X_SPEC_PARSER_HPP
#define ARCHON_X_CLI_X_IMPL_X_SPEC_PARSER_HPP


#include <cstddef>
#include <algorithm>
#include <utility>
#include <string_view>
#include <string>
#include <vector>
#include <stack>
#include <locale>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/index_range.hpp>
#include <archon/core/buffer_contents.hpp>
#include <archon/core/array_seeded_buffer.hpp>
#include <archon/core/char_mapper.hpp>
#include <archon/core/format.hpp>
#include <archon/core/format_as.hpp>
#include <archon/core/format_enc.hpp>
#include <archon/core/quote.hpp>
#include <archon/cli/spec_error.hpp>
#include <archon/cli/exception.hpp>
#include <archon/cli/impl/pattern_symbol.hpp>
#include <archon/cli/impl/pattern_structure.hpp>
#include <archon/cli/impl/spec.hpp>


namespace archon::cli::impl {


template<class C, class T> class SpecParser {
public:
    using char_type   = C;
    using traits_type = T;

    using string_view_type       = std::basic_string_view<C, T>;
    using char_mapper_type       = core::BasicCharMapper<C, T>;
    using pattern_structure_type = impl::PatternStructure<C, T>;
    using spec_type              = impl::Spec<C, T>;

    using OptionForm = typename spec_type::OptionForm;
    using ArgSpec    = typename spec_type::ArgSpec;

    SpecParser(const std::locale&, const char_mapper_type&);

    auto parse_pattern(string_view_type pattern, pattern_structure_type&, spec_type&, std::size_t pattern_index,
                       bool is_deleg) -> std::size_t;
    auto parse_option_forms(string_view_type forms, spec_type&, std::size_t option_index) -> core::IndexRange;
    auto parse_option_arg(string_view_type arg, std::size_t option_index) -> ArgSpec;

private:
    class PatternStructureRecorder;

    struct ErrorQualifier {
        std::size_t index;
        const char* entity;
        const char* component;
        SpecError code;
    };

    struct Token {
        enum class Type { keyword, short_option, long_option, value_slot,
                          left_sq_bracket, right_sq_bracket,
                          left_parenthesis, right_parenthesis,
                          ellipsis, vertical_bar, comma, end_of_input };
        Type type;
        string_view_type lexeme;
        bool preceded_by_space = false;
    };

    struct Node {
        enum class Type { keyword, short_option, long_option, value_slot, optionality,
                          repetition, juxtaposition, alternatives, sequence };
        Type type;
        std::size_t left = 0, right = 0;
        string_view_type lexeme;
    };

    struct Result {
        std::size_t node_index;
        int precedence;
    };

    const std::locale m_locale;
    const std::ctype<char_type>& m_ctype;
    const char_mapper_type& m_char_mapper;
    const char_type m_space, m_dash, m_dot, m_greater_than;
    ErrorQualifier m_error_qualifier;
    const char_type* m_curr;
    const char_type* m_end;
    Token m_next_token;
    std::vector<Node> m_nodes;
    PatternStructureRecorder m_pattern_structure_recorder;
    bool m_has_value_slot;

    bool reset(ErrorQualifier error_qualifier, string_view_type spec);
    auto parse() -> Result;
    auto parse_sequence() -> Result;
    auto parse_alternatives() -> Result;
    auto parse_juxtaposition() -> Result;
    auto parse_repetition() -> Result;
    auto parse_element() -> Result;
    void extract_next_token();

    template<class... P> [[noreturn]] void parse_error(const char* message, const P&... params) const;
    template<class... P> [[noreturn]] void error(const char* message, const P&... params) const;
    template<class... P> auto format_except(const char* message, const P&... params) const -> std::string;

    bool get_option_forms(std::size_t node_index, core::BufferContents<OptionForm>&) const;
    bool get_option_arg_spec(std::size_t node_index, bool optional, ArgSpec&) const noexcept;
};








// Implementation


template<class C, class T>
class SpecParser<C, T>::PatternStructureRecorder {
public:
    auto record(std::size_t node_index, std::size_t pattern_index, const SpecParser&, pattern_structure_type&,
                spec_type&) -> std::size_t;

private:
    using Elem = typename pattern_structure_type::Elem;
    using Seq  = typename pattern_structure_type::Seq;
    using Alt  = typename pattern_structure_type::Alt;

    class Level;

    struct Frame {
        typename Node::Type parent_node_type = Node::Type::juxtaposition;
        const Node* node = nullptr;
        bool right = false;
    };

    std::vector<Level> m_levels;
    std::vector<Seq> m_staged_seqs;
    std::vector<Elem> m_staged_elems;
    std::vector<Frame> m_stack;
};


template<class C, class T>
class SpecParser<C, T>::PatternStructureRecorder::Level {
public:
    struct Seq {
        std::size_t staged_elems_offset = 0;
        std::size_t num_params = 0;
        bool nullable = true;
        bool repeating = false;
    };

    struct Alt {
        std::size_t staged_seqs_offset = 0;
        std::size_t nullable_seq_index = 0;
        bool nullable = false;
        bool repeating = false;
        bool has_seq_with_params = false;
        bool multiple_nullable_seqs = false;
    };

    union {
        Seq seq;
        Alt alt;
    };

    bool is_alt;

    Level() noexcept
    {
        init_seq(0);
    }

    void init_seq(std::size_t staged_elems_offset) noexcept
    {
        seq = {};
        seq.staged_elems_offset = staged_elems_offset;
        is_alt = false;
    }

    void init_alt(std::size_t staged_seqs_offset) noexcept
    {
        alt = {};
        alt.staged_seqs_offset = staged_seqs_offset;
        is_alt = true;
    }
};


template<class C, class T>
inline SpecParser<C, T>::SpecParser(const std::locale& locale, const char_mapper_type& char_mapper)
    : m_locale(locale)
    , m_ctype(std::use_facet<std::ctype<char_type>>(m_locale)) // Throws
    , m_char_mapper(char_mapper)
    , m_space(char_mapper.widen(' ')) // Throws
    , m_dash(char_mapper.widen('-')) // Throws
    , m_dot(char_mapper.widen('.')) // Throws
    , m_greater_than(char_mapper.widen('>')) // Throws
{
}


template<class C, class T>
auto SpecParser<C, T>::parse_pattern(string_view_type pattern, pattern_structure_type& pattern_structure,
                                     spec_type& spec, std::size_t pattern_index, bool is_deleg) -> std::size_t
{
    std::size_t node_index = std::size_t(-1);
    if (reset({ pattern_index, "pattern", "pattern", cli::SpecError::bad_pattern_syntax }, pattern)) { // Throws
        Result result = parse(); // Throws
        if (result.precedence == 0)
            parse_error("Multiple patterns not allowed"); // Throws
        if (result.precedence == 1)
            parse_error("Unparenthesized alternatives construct not allowed"); // Throws
        if (m_has_value_slot && is_deleg)
            parse_error("Value slots are not allowed in delegating patterns"); // Throws
        node_index = result.node_index;
    }

    return m_pattern_structure_recorder.record(node_index, pattern_index, *this, pattern_structure, spec); // Throws
}


template<class C, class T>
auto SpecParser<C, T>::parse_option_forms(string_view_type forms, spec_type& spec, std::size_t option_index) ->
    core::IndexRange
{
    core::ArraySeededBuffer<OptionForm, 6> buffer;
    core::BufferContents forms_2(buffer);
    if (reset({ option_index, "option", "option forms", cli::SpecError::bad_option_forms_syntax }, forms)) { // Throws
        Result result = parse(); // Throws
        // FIXME: Extract information about what went wrong below, and vary error message
        // based on it. Don't just always report `Syntax error`. Probably use "error enum
        // and switch" scheme....                  
        if (!get_option_forms(result.node_index, forms_2)) // Throws
            parse_error("Syntax error"); // Throws
    }
    return spec.add_option_forms(forms_2); // Throws
}


template<class C, class T>
auto SpecParser<C, T>::parse_option_arg(string_view_type arg, std::size_t option_index) -> ArgSpec
{
    ArgSpec arg_2;
    if (reset({ option_index, "option", "option argument", cli::SpecError::bad_option_arg_syntax }, arg)) { // Throws
        Result result = parse(); // Throws
        // FIXME: Extract information about what went wrong below, and vary error message
        // based on it. Don't just always report `Syntax error`. Probably use "error enum
        // and switch" scheme....                  
        if (!get_option_arg_spec(result.node_index, false, arg_2))
            parse_error("Syntax error"); // Throws
    }
    return arg_2;
}


template<class C, class T>
auto SpecParser<C, T>::PatternStructureRecorder::record(std::size_t node_index, std::size_t pattern_index,
                                                        const SpecParser& parser,
                                                        pattern_structure_type& pattern_structure, spec_type& spec) ->
    std::size_t
{
    m_levels.clear();
    m_staged_seqs.clear();
    m_staged_elems.clear();
    m_stack.clear();

    std::stack levels(m_levels);
    std::stack stack(m_stack);

    Level level;
    Frame frame;
    std::size_t next_pos = 0;

    auto stage_elem = [&](typename Elem::Type type, bool is_param, bool nullable, bool repeating, bool collapsible,
                          std::size_t index) {
        m_staged_elems.push_back({ type, is_param, collapsible, index, next_pos }); // Throws
        ARCHON_ASSERT(!level.is_alt);
        level.seq.num_params += (is_param ? 1 : 0);
        level.seq.repeating = ((level.seq.repeating && nullable) || (level.seq.nullable && repeating));
        level.seq.nullable = (level.seq.nullable && nullable);
    };

    auto stage_symbol = [&](impl::PatternSymbol sym, string_view_type lexeme, bool is_param) {
        typename Elem::Type type = Elem::Type::sym;
        bool nullable = false;
        bool repeating = false;
        bool collapsible = false;
        std::size_t sym_index = pattern_structure.syms.size();
        pattern_structure.syms.push_back({ sym, lexeme }); // Throws
        ++next_pos;
        stage_elem(type, is_param, nullable, repeating, collapsible, sym_index); // Throws
    };

    auto stage_option = [&](bool is_long) {
        string_view_type lexeme = frame.node->lexeme;
        std::size_t proto_index = spec.ensure_pattern_option({ is_long, lexeme }, pattern_index); // Throws
        impl::PatternSymbol sym = PatternSymbol::proto_option(proto_index);
        bool is_param = false;
        stage_symbol(sym, lexeme, is_param); // Throws
    };

    auto stage_seq = [&](std::size_t staged_elems_offset, std::size_t num_params, bool nullable) {
        std::size_t num_elems = std::size_t(m_staged_elems.size() - staged_elems_offset);
        std::size_t elems_offset = pattern_structure.elems.size();
        pattern_structure.elems.insert(pattern_structure.elems.end(), m_staged_elems.begin() + staged_elems_offset,
                                       m_staged_elems.end()); // Throws
        m_staged_elems.resize(staged_elems_offset);
        Seq seq = { num_elems, elems_offset, num_params, next_pos, nullable };
        m_staged_seqs.push_back(seq); // Throws
    };

    auto add_seq = [&](std::size_t staged_elems_offset, std::size_t num_params, bool nullable) {
        stage_seq(staged_elems_offset, num_params, nullable); // Throws
        std::size_t seq_offset = pattern_structure.seqs.size();
        pattern_structure.seqs.push_back(m_staged_seqs.back()); // Throws
        m_staged_seqs.pop_back();
        return seq_offset;
    };

    auto add_alt = [&](std::size_t staged_seqs_offset, std::size_t nullable_seq_index) {
        std::size_t num_seqs = std::size_t(m_staged_seqs.size() - staged_seqs_offset);
        std::size_t seqs_offset = pattern_structure.seqs.size();
        pattern_structure.seqs.insert(pattern_structure.seqs.end(), m_staged_seqs.begin() + staged_seqs_offset,
                                      m_staged_seqs.end()); // Throws
        m_staged_seqs.resize(staged_seqs_offset);
        std::size_t alt_index = pattern_structure.alts.size();
        Alt alt = { num_seqs, seqs_offset, nullable_seq_index };
        pattern_structure.alts.push_back(alt); // Throws
        return alt_index;
    };

    auto enter_node = [&](std::size_t node_index) {
        typename Node::Type parent_node_type = frame.node->type;
        ARCHON_ASSERT(node_index < parser.m_nodes.size());
        const Node& node = parser.m_nodes[node_index];
        stack.push(frame); // Throws
        frame = {};
        frame.parent_node_type = parent_node_type;
        frame.node = &node;
    };

    auto open_seq = [&] {
        levels.push(level); // Throws
        level.init_seq(m_staged_elems.size());
    };

    auto close_seq = [&] {
        ARCHON_ASSERT(!level.is_alt);
        ARCHON_ASSERT(!levels.empty());
        std::size_t seq_index = add_seq(level.seq.staged_elems_offset, level.seq.num_params,
                                        level.seq.nullable); // Throws
        level = levels.top();
        levels.pop();
        return seq_index;
    };

    auto close_seq_and_stage_as_branch = [&] {
        ARCHON_ASSERT(!level.is_alt);
        ARCHON_ASSERT(!levels.empty());
        typename Level::Seq seq = level.seq;
        stage_seq(seq.staged_elems_offset, seq.num_params, seq.nullable); // Throws
        level = levels.top();
        levels.pop();
        ARCHON_ASSERT(level.is_alt);
        if (ARCHON_UNLIKELY(level.alt.nullable && seq.nullable))
            level.alt.multiple_nullable_seqs = true;
        level.alt.nullable = (level.alt.nullable || seq.nullable);
        level.alt.repeating = (level.alt.repeating || seq.repeating);
        level.alt.has_seq_with_params = (level.alt.has_seq_with_params || seq.num_params > 0);
        level.alt.nullable_seq_index += (level.alt.nullable ? 0 : 1);
    };

    auto open_alt = [&] {
        levels.push(level); // Throws
        level.init_alt(m_staged_seqs.size());
    };

    auto close_alt = [&] {
        ARCHON_ASSERT(level.is_alt);
        ARCHON_ASSERT(!levels.empty());
        std::size_t alt_index = add_alt(level.alt.staged_seqs_offset, level.alt.nullable_seq_index); // Throws
        level = levels.top();
        levels.pop();
        return alt_index;
    };

    if (node_index != std::size_t(-1)) {
        ARCHON_ASSERT(node_index < parser.m_nodes.size());
        frame.node = &parser.m_nodes[node_index];
        goto node_1;
    }
    goto finish;

  node_1:
    switch (frame.node->type) {
        case Node::Type::keyword: {
            string_view_type lexeme = frame.node->lexeme;
            std::size_t keyword_index = spec.ensure_keyword(lexeme); // Throws
            impl::PatternSymbol sym = impl::PatternSymbol::keyword(keyword_index);
            bool is_param = false;
            stage_symbol(sym, lexeme, is_param); // Throws
            goto node_2;
        }
        case Node::Type::short_option: {
            bool is_long = false;
            stage_option(is_long); // Throws
            goto node_2;
        }
        case Node::Type::long_option: {
            bool is_long = true;
            stage_option(is_long); // Throws
            goto node_2;
        }
        case Node::Type::value_slot: {
            impl::PatternSymbol sym = impl::PatternSymbol::value_slot();
            string_view_type lexeme = frame.node->lexeme;
            bool is_param = true;
            stage_symbol(sym, lexeme, is_param); // Throws
            goto node_2;
        }
        case Node::Type::optionality:
        case Node::Type::repetition: {
            open_seq(); // Throws
            std::size_t node_index = frame.node->left;
            enter_node(node_index); // Throws
            goto node_1;
        }
        case Node::Type::juxtaposition: {
            std::size_t node_index = frame.node->left;
            enter_node(node_index); // Throws
            goto node_1;
        }
        case Node::Type::alternatives: {
            // If we have entered an alternatives node (which we have) from a
            // non-alternatives parent node, we need to open an alternatives construct, then
            // open an element sequence corresponding to its first branch.
            if (frame.parent_node_type != Node::Type::alternatives) {
                open_alt(); // Throws
                open_seq(); // Throws
            }
            std::size_t node_index = frame.node->left;
            enter_node(node_index); // Throws
            goto node_1;
        }
        case Node::Type::sequence:
            break;
    }
    ARCHON_ASSERT_UNREACHABLE();

  node_2:
    if (ARCHON_LIKELY(!stack.empty())) {
        frame = stack.top();
        stack.pop();
        switch (frame.node->type) {
            case Node::Type::keyword:
            case Node::Type::short_option:
            case Node::Type::long_option:
            case Node::Type::value_slot:
                break;
            case Node::Type::optionality: {
                ARCHON_ASSERT(!level.is_alt);
                // This optionality construct has internal ambiguity if the sub-pattern
                // (operand of optionality operator) is already nullable. If this had been
                // allowed, then, in a case like `[[-x <foo>]]` with empty input, it would
                // not be clear whether the std::optional object associated with the
                // outer-most optionality construct should have a value.
                if (ARCHON_UNLIKELY(level.seq.nullable)) {
                    parser.error("Pattern-internal structural ambiguity: "
                                 "Optionality construct with nullable sub-pattern"); // Throws
                }
                typename Elem::Type type = Elem::Type::opt;
                bool is_param = true;
                bool nullable = true;
                bool repeating = level.seq.repeating;
                bool collapsible = (level.seq.num_params == 0);
                std::size_t seq_index = close_seq(); // Throws
                stage_elem(type, is_param, nullable, repeating, collapsible, seq_index); // Throws
                goto node_2;
            }
            case Node::Type::repetition: {
                ARCHON_ASSERT(!level.is_alt);
                // This repetition construct has internal ambiguity if the sub-pattern
                // (operand of repetition operator) is nullable. If this had been allowed,
                // then, in a case like `[-x <foo>]...` with empty input, it would not be
                // clear how many elements should be in the std::vector object associated
                // with the repetition construct.
                if (ARCHON_UNLIKELY(level.seq.nullable)) {
                    parser.error("Pattern-internal structural ambiguity: "
                                 "Repetition construct with nullable sub-pattern"); // Throws
                }
                // This repetition construct has internal ambiguity if there is some
                // nonempty sequence of symbol positions such that it, and all repetitions
                // of it are in the language over symbol positions generated by the
                // sub-pattern (operand of repetition operator). If this had been allowed,
                // then, in a case like `((-x <foo>)...)...` with input matching `-x <foo>
                // -x <foo>`, it would not be clear whether the std::vector objects
                // associated with the outer-most and inner-most repetition constructs
                // should have one and two elements respectively, or whether it should be
                // the other way around (2 elements in the outer-most std::vector object).
                if (ARCHON_UNLIKELY(level.seq.repeating)) {
                    parser.error("Pattern-internal structural ambiguity: "
                                 "Repetition construct with repeating sub-pattern"); // Throws
                }
                typename Elem::Type type = Elem::Type::rep;
                bool is_param = true;
                bool nullable = false;
                bool repeating = true;
                bool collapsible = (level.seq.num_params == 0);
                std::size_t seq_index = close_seq(); // Throws
                stage_elem(type, is_param, nullable, repeating, collapsible, seq_index); // Throws
                goto node_2;
            }
            case Node::Type::juxtaposition: {
                if (frame.right)
                    goto node_2;
                frame.right = true;
                std::size_t node_index = frame.node->right;
                enter_node(node_index); // Throws
                goto node_1;
            }
            case Node::Type::alternatives: {
                if (frame.right) {
                    // If we are returning from an alternatives node (which we are) to a
                    // non-alternatives parent node, we need to close the element sequence
                    // corresponding to the last branch of the alternatives construct, then
                    // close the alternatives construct itself.
                    if (frame.parent_node_type != Node::Type::alternatives) {
                        close_seq_and_stage_as_branch(); // Throws
                        ARCHON_ASSERT(level.is_alt);
                        // This alternatives construct has internal ambiguity if multiple
                        // branches are nullable. If this had been allowed, then, in a case
                        // like `([-x <foo>] | [-y <foo>])` with empty input, it would not
                        // be clear whether the std::variant object associated with the
                        // alternatives construct should have its first or second
                        // alternative materialized.
                        if (ARCHON_UNLIKELY(level.alt.multiple_nullable_seqs)) {
                            parser.error("Pattern-internal structural ambiguity: "
                                         "Alternatives construct with multiple nullable branches"); // Throws
                        }
                        typename Elem::Type type = Elem::Type::alt;
                        bool is_param = true;
                        bool nullable = level.alt.nullable;
                        bool repeating = level.alt.repeating;
                        bool collapsible = !level.alt.has_seq_with_params;
                        std::size_t alt_index = close_alt(); // Throws
                        stage_elem(type, is_param, nullable, repeating, collapsible, alt_index); // Throws
                    }
                    goto node_2;
                }
                frame.right = true;
                // Close the element sequence corresponding to the last left-side branch of
                // the alternatives construct, and open a new one for the first right-side
                // branch.
                close_seq_and_stage_as_branch(); // Throws
                open_seq(); // Throws
                std::size_t node_index = frame.node->right;
                enter_node(node_index); // Throws
                goto node_1;
            }
            case Node::Type::sequence:
                break;
        }
        ARCHON_ASSERT_UNREACHABLE();
    }

  finish:
    ARCHON_ASSERT(!level.is_alt);
    ARCHON_ASSERT(levels.empty());
    std::size_t seq_index = add_seq(0, level.seq.num_params, level.seq.nullable); // Throws
    return seq_index;
}


template<class C, class T>
bool SpecParser<C, T>::reset(ErrorQualifier error_qualifier, string_view_type spec)
{
    m_error_qualifier = error_qualifier;
    m_curr = spec.data();
    m_end = spec.data() + spec.size();
    m_nodes.clear();
    m_has_value_slot = false;
    extract_next_token(); // Throws
    return (m_next_token.type != Token::Type::end_of_input);
}


template<class C, class T>
inline auto SpecParser<C, T>::parse() -> Result
{
    Result result = parse_sequence(); // Throws
    if (m_next_token.type != Token::Type::end_of_input)
        parse_error("Unexpected token (%s)", core::quoted_s(m_next_token.lexeme)); // Throws
    return result;
}


template<class C, class T>
auto SpecParser<C, T>::parse_sequence() -> Result
{
    Result result = parse_alternatives(); // Throws
    for (;;) {
        if (m_next_token.type != Token::Type::comma)
            break;
        extract_next_token(); // Throws
        Node node;
        node.type = Node::Type::sequence;
        node.left = result.node_index;
        node.right = parse_alternatives().node_index; // Throws
        result.node_index = m_nodes.size();
        result.precedence = 0;
        m_nodes.push_back(node); // Throws
    }
    return result;
}


template<class C, class T>
auto SpecParser<C, T>::parse_alternatives() -> Result
{
    Result result = parse_juxtaposition(); // Throws
    for (;;) {
        if (m_next_token.type != Token::Type::vertical_bar)
            break;
        extract_next_token(); // Throws
        Node node;
        node.type = Node::Type::alternatives;
        node.left = result.node_index;
        node.right = parse_juxtaposition().node_index; // Throws
        result.node_index = m_nodes.size();
        result.precedence = 1;
        m_nodes.push_back(node); // Throws
    }
    return result;
}


template<class C, class T>
auto SpecParser<C, T>::parse_juxtaposition() -> Result
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
            parse_error("Need space between juxtaposed elements"); // Throws
        Node node;
        node.type = Node::Type::juxtaposition;
        node.left = result.node_index;
        node.right = parse_repetition().node_index; // Throws
        result.node_index = m_nodes.size();
        result.precedence = 2;
        m_nodes.push_back(node); // Throws
    }
    return result;
}


template<class C, class T>
auto SpecParser<C, T>::parse_repetition() -> Result
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
        result.precedence = 3;
        m_nodes.push_back(node); // Throws
    }
    return result;
}


template<class C, class T>
auto SpecParser<C, T>::parse_element() -> Result
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
        node.type = Node::Type::optionality;
        node.left = parse_alternatives().node_index; // Throws
        if (m_next_token.type != Token::Type::right_sq_bracket)
            parse_error("Unclosed square bracket"); // Throws
        extract_next_token(); // Throws
        std::size_t node_index = m_nodes.size();
        m_nodes.push_back(node); // Throws
        return { node_index, 4 };
    }
    if (m_next_token.type == Token::Type::left_parenthesis) {
        extract_next_token(); // Throws
        std::size_t node_index = parse_alternatives().node_index; // Throws
        if (m_next_token.type != Token::Type::right_parenthesis)
            parse_error("Unclosed parenthesis"); // Throws
        extract_next_token(); // Throws
        return { node_index, 4 };
    }
    parse_error("Bad start of expression"); // Throws

  token_node:
    Node node;
    node.type = node_type;
    node.lexeme = m_next_token.lexeme;
    extract_next_token(); // Throws
    std::size_t node_index = m_nodes.size();
    m_nodes.push_back(node); // Throws
    return { node_index, 4 };
}


template<class C, class T>
void SpecParser<C, T>::extract_next_token()
{
    const char_type* i = m_curr;
    Token token;
    char ch;

  again:
    if (i == m_end) {
        token.type = Token::Type::end_of_input;
        goto good;
    }

    ch = m_char_mapper.narrow(*i, '\0');
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
                    // Long form option
                    do i = m_ctype.scan_not(std::ctype_base::alnum, i + 1, m_end);
                    while (i != m_end && *i == m_dash);
                    token.type = Token::Type::long_option;
                    goto good;
                }
                if (*i != m_space) {
                    // Short form option
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
            parse_error("Missing closing `>` in value slot specification"); // Throws

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

        case ',':
            token.type = Token::Type::comma;
            goto good;
    }

    if (ch == '-' || m_ctype.is(std::ctype_base::alnum, ch)) {
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
    parse_error("Invalid token (%s)", core::quoted_s(string_view_type(m_curr, std::size_t(i - m_curr)))); // Throws

  good:
    token.lexeme = { m_curr, std::size_t(i - m_curr) };
    m_curr = i;
    m_next_token = token;
}


template<class C, class T>
template<class... P> inline void SpecParser<C, T>::parse_error(const char* message, const P&... params) const
{
    error("Failed to parse %s specification: %s", m_error_qualifier.component,
          core::formatted(message, params...)); // Throws
}


template<class C, class T>
template<class... P> void SpecParser<C, T>::error(const char* message, const P&... params) const
{
    std::string message_2 = format_except("Error in %s %s specification: %s",
                                          core::as_ordinal(1 + m_error_qualifier.index), m_error_qualifier.entity,
                                          core::formatted(message, params...)); // Throws
    throw cli::BadSpec(m_error_qualifier.code, std::move(message_2));
}


template<class C, class T>
template<class... P>
inline auto SpecParser<C, T>::format_except(const char* message, const P&... params) const -> std::string
{
    return core::format_enc<char_type>(m_locale, message, params...); // Throws
}


template<class C, class T>
bool SpecParser<C, T>::get_option_forms(std::size_t node_index, core::BufferContents<OptionForm>& buffer) const
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
        case Node::Type::optionality:
        case Node::Type::repetition:
        case Node::Type::juxtaposition:
        case Node::Type::alternatives:
            break;
        case Node::Type::sequence:
            return (get_option_forms(node.left,  buffer) && get_option_forms(node.right, buffer)); // Throws
    }
    return false;
}


template<class C, class T>
bool SpecParser<C, T>::get_option_arg_spec(std::size_t node_index, bool optional, ArgSpec& arg) const noexcept
{
    const Node& node = m_nodes[node_index];
    switch (node.type) {
        case Node::Type::keyword:
        case Node::Type::short_option:
        case Node::Type::long_option:
            break;
        case Node::Type::value_slot:
            arg.allow   = true;
            arg.require = !optional;
            arg.lexeme = node.lexeme;
            return true;
        case Node::Type::optionality:
            if (ARCHON_LIKELY(!optional))
                return get_option_arg_spec(node.left, true, arg);
            return false;
        case Node::Type::repetition:
        case Node::Type::juxtaposition:
        case Node::Type::alternatives:
        case Node::Type::sequence:
            break;
    }
    return false;
}


} // namespace archon::cli::impl

#endif // ARCHON_X_CLI_X_IMPL_X_SPEC_PARSER_HPP
