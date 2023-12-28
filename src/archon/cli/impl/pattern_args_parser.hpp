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

#ifndef ARCHON_X_CLI_X_IMPL_X_PATTERN_ARGS_PARSER_HPP
#define ARCHON_X_CLI_X_IMPL_X_PATTERN_ARGS_PARSER_HPP

/// \file


#include <cstddef>
#include <type_traits>
#include <utility>
#include <tuple>
#include <array>
#include <string_view>
#include <optional>
#include <variant>
#include <vector>
#include <stack>

#include <archon/core/features.h>
#include <archon/core/type.hpp>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/utility.hpp>
#include <archon/core/quote.hpp>
#include <archon/cli/impl/value_parser.hpp>
#include <archon/cli/proc_error.hpp>
#include <archon/cli/impl/error_accum.hpp>
#include <archon/cli/impl/pattern_structure.hpp>


namespace archon::cli::impl {


template<class C, class T> struct PatternArgsParser {
public:
    using string_view_type       = std::basic_string_view<C, T>;
    using value_parser_type      = impl::ValueParser<C, T>;
    using error_accum_type       = impl::ErrorAccum<C, T>;
    using pattern_structure_type = impl::PatternStructure<C, T>;

    using Sym  = typename pattern_structure_type::Sym;
    using Elem = typename pattern_structure_type::Elem;
    using Seq  = typename pattern_structure_type::Seq;
    using Alt  = typename pattern_structure_type::Alt;

    struct MatchPos;

    struct Desc;

    static void generate_descs(const pattern_structure_type&, std::size_t pattern_index,
                               core::Span<const MatchPos> match_positions, std::vector<Desc>&);

    PatternArgsParser(const pattern_structure_type&, core::Span<const string_view_type> args,
                      core::Span<const Desc> descs, std::size_t show_arg_max_size) noexcept;

    template<class U> bool parse(U& elems, value_parser_type&, error_accum_type&) const;

private:
    class State;

    const pattern_structure_type& m_pattern_structure;
    core::Span<const string_view_type> m_args;
    core::Span<const Desc> m_descs;
    std::size_t m_show_arg_max_size;
};


template<class C, class T> struct PatternArgsParser<C, T>::MatchPos {
    std::size_t pos;
    std::size_t arg_index;
};


template<class C, class T> struct PatternArgsParser<C, T>::Desc {
    const Elem* elem;

    // If `elem->type` is `Elem::Type::sym`, `value` is the index of the matched
    // command-line argument within `argv` (as passed to `main()`). If `elem->type` is
    // `Elem::Type::opt`, `value` is 1 if, and only if the optionality construct was matched
    // against a non-empty list of command-line arguments. If `elem->type` is
    // `Elem::Type::rep`, `value` is the number of times the sub-pattern was matched. If
    // `elem->type` is `Elem::Type::alt`, `value` is the index of the branch in the
    // alternatives construct.
    std::size_t value;
};








// Implementation


template<class C, class T>
void PatternArgsParser<C, T>::generate_descs(const pattern_structure_type& pattern_structure,
                                             std::size_t elem_seq_index, core::Span<const MatchPos> match_positions,
                                             std::vector<Desc>& descs)
{
    descs.clear();

    ARCHON_ASSERT(elem_seq_index < pattern_structure.seqs.size());
    const Seq& root = pattern_structure.seqs[elem_seq_index];

    struct Cursor {
        const Seq* seq;
        std::size_t elem_index;
    };

    struct Frame {
        Cursor cursor;
        std::size_t begin_pos;
        std::size_t desc_index;
    };

    Frame frame = { { &root, 0 }, 0, 0 };
    std::stack<Frame> stack;

    std::size_t num_match_positions = match_positions.size();
    std::size_t match_pos_index = 0;
    MatchPos match_pos;

    auto pos_in_range = [&](const Elem& elem) noexcept {
        return (match_pos.pos >= frame.begin_pos && match_pos.pos < elem.end_pos);
    };

    auto next_elem = [&](const Elem& elem) noexcept {
        ++frame.cursor.elem_index;
        frame.begin_pos = elem.end_pos;
    };

    auto enter_subseq = [&](const Elem& elem) {
        ARCHON_ASSERT(elem.type == Elem::Type::opt || elem.type == Elem::Type::rep);
        stack.push(std::move(frame)); // Throws
        std::size_t seq_index = elem.index;
        ARCHON_ASSERT(seq_index < pattern_structure.seqs.size());
        const Seq& seq = pattern_structure.seqs[seq_index];
        frame.cursor = { &seq, 0 };
    };

    goto start;

  next_match_pos:
    ++match_pos_index;

  start:
    ARCHON_ASSERT(match_pos_index < num_match_positions);
    match_pos = match_positions[match_pos_index];

  again:
    if (ARCHON_LIKELY(frame.cursor.elem_index < frame.cursor.seq->num_elems)) {
        std::size_t elem_index = std::size_t(frame.cursor.seq->elems_offset + frame.cursor.elem_index);
        ARCHON_ASSERT(elem_index < pattern_structure.elems.size());
        const Elem& elem = pattern_structure.elems[elem_index];
        switch (elem.type) {
            case Elem::Type::sym: {
                ARCHON_ASSERT(pos_in_range(elem));
                if (elem.is_param) {
                    std::size_t arg_index = match_pos.arg_index;
                    std::size_t value = arg_index;
                    descs.push_back({ &elem, value }); // Throws
                }
                next_elem(elem);
                goto next_match_pos;
            }
            case Elem::Type::opt: {
                if (ARCHON_LIKELY(!pos_in_range(elem))) {
                    std::size_t value = 0; // Absent option
                    descs.push_back({ &elem, value }); // Throws
                    next_elem(elem);
                    goto again;
                }
                std::size_t value = 1; // Present option
                descs.push_back({ &elem, value }); // Throws
                enter_subseq(elem); // Throws
                goto again;
            }
            case Elem::Type::rep: {
                ARCHON_ASSERT(pos_in_range(elem));
                frame.desc_index = descs.size();
                std::size_t value = 1; // First occurrence in repetition
                descs.push_back({ &elem, value }); // Throws
                enter_subseq(elem); // Throws
                goto again;
            }
            case Elem::Type::alt: {
                std::size_t branch_index = 0;
                std::size_t alt_index = elem.index;
                ARCHON_ASSERT(alt_index < pattern_structure.alts.size());
                const Alt& alt = pattern_structure.alts[alt_index];
                if (ARCHON_LIKELY(!pos_in_range(elem))) {
                    ARCHON_ASSERT(alt.nullable_seq_index < alt.num_seqs);
                    branch_index = alt.nullable_seq_index;
                    goto alt_finish;
                }
                {
                    std::size_t n = alt.num_seqs;
                    for (std::size_t i = 0; i < n; ++i) {
                        std::size_t seq_index = std::size_t(alt.seqs_offset + i);
                        ARCHON_ASSERT(seq_index < pattern_structure.seqs.size());
                        const Seq& seq = pattern_structure.seqs[seq_index];
                        if (ARCHON_LIKELY(match_pos.pos >= seq.end_pos))
                            continue;
                        branch_index = i;
                        goto alt_finish;
                    }
                }
                ARCHON_ASSERT_UNREACHABLE();
              alt_finish:
                std::size_t value = branch_index;
                descs.push_back({ &elem, value }); // Throws
                stack.push(std::move(frame)); // Throws
                ARCHON_ASSERT(branch_index < alt.num_seqs);
                std::size_t seq_index = std::size_t(alt.seqs_offset + branch_index);
                ARCHON_ASSERT(seq_index < pattern_structure.seqs.size());
                const Seq& seq = pattern_structure.seqs[seq_index];
                frame.cursor = { &seq, 0 };
                goto again;
            }
        }
        ARCHON_ASSERT_UNREACHABLE();
    }

    if (ARCHON_LIKELY(!stack.empty())) {
        frame = std::move(stack.top());
        stack.pop();
        std::size_t elem_index = std::size_t(frame.cursor.seq->elems_offset + frame.cursor.elem_index);
        ARCHON_ASSERT(elem_index < pattern_structure.elems.size());
        const Elem& elem = pattern_structure.elems[elem_index];
        switch (elem.type) {
            case Elem::Type::sym:
                break;
            case Elem::Type::rep:
                if (ARCHON_LIKELY(pos_in_range(elem))) {
                    ++descs[frame.desc_index].value; // Next occurrence in repetition
                    enter_subseq(elem); // Throws
                    goto again;
                }
                [[fallthrough]];
            case Elem::Type::opt:
            case Elem::Type::alt:
                next_elem(elem);
                goto again;
        }
        ARCHON_ASSERT_UNREACHABLE();
    }

    ++match_pos_index;
    ARCHON_ASSERT(match_pos_index == num_match_positions);
}


template<class C, class T>
inline PatternArgsParser<C, T>::PatternArgsParser(const pattern_structure_type& pattern_structure,
                                                  core::Span<const string_view_type> args,
                                                  core::Span<const Desc> descs, std::size_t show_arg_max_size) noexcept
    : m_pattern_structure(pattern_structure)
    , m_args(args)
    , m_descs(descs)
    , m_show_arg_max_size(show_arg_max_size)
{
}


template<class C, class T>
class PatternArgsParser<C, T>::State {
public:
    State(const PatternArgsParser& parser, value_parser_type& value_parser, error_accum_type& error_accum) noexcept
        : m_parser(parser)
        , m_value_parser(value_parser)
        , m_error_accum(error_accum)
        , m_desc(parser.m_descs.data())
        , m_desc_end(m_desc + parser.m_descs.size())
    {
    }

    template<class U> bool parse(U& elems)
    {
        bool complete = parse_pattern(elems); // Throws
        ARCHON_ASSERT(!complete || m_desc == m_desc_end);
        ARCHON_ASSERT(complete || m_error);
        return !m_error;
    }

private:
    const PatternArgsParser& m_parser;
    value_parser_type& m_value_parser;
    error_accum_type& m_error_accum;
    bool m_error = false;
    const Desc* m_desc;
    const Desc* m_desc_end;

    template<class... U> bool parse_pattern(std::tuple<U...>& elems)
    {
        return core::for_each_tuple_elem_a(elems, [&](auto& elem) {
            return parse_elem(elem); // Throws
        }); // Throws
    }

    template<class U, class V> bool parse_pattern(std::pair<U, V>& elems)
    {
        return core::for_each_tuple_elem_a(elems, [&](auto& elem) {
            return parse_elem(elem); // Throws
        }); // Throws
    }

    template<class U, std::size_t N> bool parse_pattern(std::array<U, N>& elems)
    {
        return core::for_each_tuple_elem_a(elems, [&](auto& elem) {
            return parse_elem(elem); // Throws
        }); // Throws
    }

    bool parse_pattern(std::monostate&)
    {
        return true;
    }

    template<class U> bool parse_pattern(U& elem)
    {
        return parse_elem(elem); // Throws
    }

    template<class U> bool parse_elem(std::optional<U>& opt)
    {
        Desc desc = next();
        ARCHON_ASSERT(desc.elem->type == Elem::Type::opt);
        ARCHON_ASSERT(desc.value < 2);
        bool present = (desc.value > 0);
        if (ARCHON_LIKELY(!present))
            return true;
        return parse_pattern(opt.emplace()); // Throws
    }

    template<class U> bool parse_elem(std::vector<U>& vec)
    {
        Desc desc = next();
        if (ARCHON_UNLIKELY(desc.elem->type == Elem::Type::opt)) {
            bool present = (desc.value > 0);
            if (ARCHON_LIKELY(!present))
                return true;
            desc = next();
        }
        ARCHON_ASSERT(desc.elem->type == Elem::Type::rep);
        std::size_t num_repetitions = desc.value;
        vec.resize(num_repetitions); // Throws
        for (std::size_t i = 0; i < num_repetitions; ++i) {
            if (ARCHON_LIKELY(parse_pattern(vec[i]))) // Throws
                continue;
            return false;
        }
        return true;
    }

    template<class... U> struct ParseVariant {
        template<std::size_t I> static bool exec(State& state, std::variant<U...>& var)
        {
            return state.parse_pattern(var.template emplace<I>()); // Throws
        }
    };

    template<class... U> bool parse_elem(std::variant<U...>& var)
    {
        Desc desc = next();
        ARCHON_ASSERT(desc.elem->type == Elem::Type::alt);
        std::size_t branch_index = desc.value;
        constexpr std::size_t num_branches = sizeof... (U);
        ARCHON_ASSERT(branch_index < num_branches);
        return core::dispatch<ParseVariant<U...>, num_branches>(branch_index, *this, var); // Throws
    }

    template<class U> bool parse_elem(U& elem)
    {
        Desc desc = next();
        if constexpr (std::is_same_v<U, bool>) {
            if (ARCHON_LIKELY(desc.elem->type != Elem::Type::opt))
                goto regular;
            ARCHON_ASSERT(desc.elem->collapsible);
            bool present = (desc.value > 0);
            elem = present;
            return true;
        }
        else if constexpr (std::is_same_v<U, std::size_t>) {
            if (ARCHON_LIKELY(desc.elem->type != Elem::Type::opt)) {
                if (ARCHON_LIKELY(desc.elem->type != Elem::Type::rep))
                    goto regular;
            }
            else {
                bool present = (desc.value > 0);
                if (ARCHON_LIKELY(!present)) {
                    elem = 0;
                    return true;
                }
                desc = next();
                ARCHON_ASSERT(desc.elem->type == Elem::Type::rep);
            }
            ARCHON_ASSERT(desc.elem->collapsible);
            std::size_t num_repetitions = desc.value;
            elem = num_repetitions;
            return true;
        }
        goto regular;
      regular:
        ARCHON_ASSERT(desc.elem->type == Elem::Type::sym);
        std::size_t arg_index = desc.value;
        ARCHON_ASSERT(arg_index < m_parser.m_args.size());
        string_view_type arg = m_parser.m_args[arg_index];
        if (ARCHON_LIKELY(m_value_parser.parse(arg, elem))) // Throws
            return true;
        // Parse error
        m_error = true;
        std::size_t sym_index = desc.elem->index;
        ARCHON_ASSERT(sym_index < m_parser.m_pattern_structure.syms.size());
        const Sym& sym = m_parser.m_pattern_structure.syms[sym_index];
        m_error_accum.add_error(arg_index, cli::ProcError::bad_pattern_arg, "Bad command-line argument %s for %s",
                                core::quoted(arg, m_parser.m_show_arg_max_size), sym.lexeme); // Throws
        return true;
    }

    Desc next() noexcept
    {
        ARCHON_ASSERT(m_desc < m_desc_end);
        Desc desc = *m_desc;
        ++m_desc;
        return desc;
    }
};


// This function is marked "noinline" in order to attempt to minimize the stack frame of the
// caller, i.e., impl::FuncExecPatternAction::invoke(), and thereby minimize the total size
// of the stack at the point from where the pattern function is invoked.
template<class C, class T>
template<class U>
ARCHON_NOINLINE bool PatternArgsParser<C, T>::parse(U& elems, value_parser_type& value_parser,
                                                    error_accum_type& error_accum) const
{
    State state(*this, value_parser, error_accum);
    return state.template parse<U>(elems); // Throws
}


} // namespace archon::cli::impl

#endif // ARCHON_X_CLI_X_IMPL_X_PATTERN_ARGS_PARSER_HPP
