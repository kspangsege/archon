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

#ifndef ARCHON_X_CLI_X_IMPL_X_NFA_BUILDER_HPP
#define ARCHON_X_CLI_X_IMPL_X_NFA_BUILDER_HPP


#include <cstddef>
#include <utility>
#include <vector>
#include <stack>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/cli/impl/pattern_symbol.hpp>
#include <archon/cli/impl/pattern_structure.hpp>
#include <archon/cli/impl/nfa.hpp>


namespace archon::cli::impl {


template<class C, class T> class NfaBuilder {
public:
    using pattern_structure_type = impl::PatternStructure<C, T>;

    NfaBuilder(impl::Nfa&, const pattern_structure_type&) noexcept;

    void add_pattern(std::size_t elem_seq_index, std::size_t pattern_index);

private:
    using Elem = typename pattern_structure_type::Elem;
    using Seq  = typename pattern_structure_type::Seq;
    using Alt  = typename pattern_structure_type::Alt;

    // Designates a range of elements in `m_position_set_elems`
    using PositionSet = std::pair<std::size_t, std::size_t>;

    struct Result;
    struct Frame;

    impl::Nfa& m_nfa;
    const pattern_structure_type& m_pattern_structure;
    std::vector<Frame> m_stack;
    std::vector<std::size_t> m_position_set_elems;

    auto make_position_set(std::size_t pos) -> PositionSet;
    auto position_set_union(PositionSet, PositionSet) -> PositionSet;
    void register_followpos(PositionSet set, std::size_t pos);
    void register_followpos(PositionSet set_1, PositionSet set_2);
};








// Implementation


template<class C, class T>
struct NfaBuilder<C, T>::Result {
    PositionSet first_pos, last_pos;
    bool nullable = false;
};


template<class C, class T>
struct NfaBuilder<C, T>::Frame {
    struct SeqA {
        const Seq* seq;
        std::size_t elem_index;
    };
    struct AltA {
        const Alt* alt;
        std::size_t seq_index;
    };
    bool is_alt;
    union {
        SeqA seq;
        AltA alt;
    };
    Result result;
};


template<class C, class T>
inline NfaBuilder<C, T>::NfaBuilder(impl::Nfa& nfa, const pattern_structure_type& pattern_structure) noexcept
    : m_nfa(nfa)
    , m_pattern_structure(pattern_structure)
{
}


template<class C, class T>
void NfaBuilder<C, T>::add_pattern(std::size_t elem_seq_index, std::size_t pattern_index)
{
    m_stack.clear();
    m_position_set_elems.clear();

    std::stack stack(m_stack);
    Frame frame = {};

    auto init_seq = [&](std::size_t seq_index) noexcept {
        ARCHON_ASSERT(seq_index < m_pattern_structure.seqs.size());
        frame.is_alt = false;
        frame.seq = { &m_pattern_structure.seqs[seq_index], 0 };
        frame.result = {};
        frame.result.nullable = true;
    };

    auto init_alt = [&](std::size_t alt_index) noexcept {
        ARCHON_ASSERT(alt_index < m_pattern_structure.alts.size());
        frame.is_alt = true;
        frame.alt = { &m_pattern_structure.alts[alt_index], 0 };
        frame.result = {};
    };

    auto integrate_seq_subresult = [&](Result subresult) {
        ARCHON_ASSERT(!frame.is_alt);
        Result result_1 = frame.result;
        Result result_2 = subresult;
        register_followpos(result_1.last_pos, result_2.first_pos); // Throws
        Result result;
        if (!result_1.nullable) {
            result.first_pos = result_1.first_pos;
        }
        else {
            result.first_pos = position_set_union(result_1.first_pos, result_2.first_pos); // Throws
        }
        if (!result_2.nullable) {
            result.last_pos = result_2.last_pos;
        }
        else {
            result.last_pos = position_set_union(result_1.last_pos, result_2.last_pos); // Throws
        }
        result.nullable = (result_1.nullable && result_2.nullable);
        frame.result = result;
    };

    auto integrate_alt_subresult = [&](Result subresult) {
        ARCHON_ASSERT(frame.is_alt);
        Result result_1 = frame.result;
        Result result_2 = subresult;
        Result result;
        result.first_pos = position_set_union(result_1.first_pos, result_2.first_pos); // Throws
        result.last_pos  = position_set_union(result_1.last_pos, result_2.last_pos); // Throws
        result.nullable = (result_1.nullable || result_2.nullable);
        frame.result = result;
    };

    init_seq(elem_seq_index);
    goto seq_begin;

  seq_continue:
    ARCHON_ASSERT(!frame.is_alt);
    ARCHON_ASSERT(frame.seq.elem_index < frame.seq.seq->num_elems);
    ++frame.seq.elem_index;

  seq_begin:
    ARCHON_ASSERT(!frame.is_alt);
    if (frame.seq.elem_index < frame.seq.seq->num_elems) {
        std::size_t elem_index = frame.seq.seq->elems_offset + frame.seq.elem_index;
        ARCHON_ASSERT(elem_index < m_pattern_structure.elems.size());
        const Elem& elem = m_pattern_structure.elems[elem_index];
        switch (elem.type) {
            case Elem::Type::sym: {
                ARCHON_ASSERT(elem.end_pos > 0);
                std::size_t pattern_internal_pos = std::size_t(elem.end_pos - 1);
                std::size_t sym_index = elem.index;
                ARCHON_ASSERT(sym_index < m_pattern_structure.syms.size());
                impl::PatternSymbol symbol = m_pattern_structure.syms[sym_index].sym;
                std::size_t pos = m_nfa.create_position(pattern_index, pattern_internal_pos, symbol); // Throws
                Result subresult;
                subresult.first_pos = make_position_set(pos); // Throws
                subresult.last_pos  = subresult.first_pos;
                integrate_seq_subresult(subresult); // Throws
                goto seq_continue;
            }
            case Elem::Type::opt:
            case Elem::Type::rep: {
                stack.push(frame); // Throws
                init_seq(elem.index);
                goto seq_begin;
            }
            case Elem::Type::alt: {
                stack.push(frame); // Throws
                init_alt(elem.index);
                goto alt_begin;
            }
        }
        ARCHON_ASSERT_UNREACHABLE();
    }

    // seq end
    ARCHON_ASSERT(frame.result.nullable == frame.seq.seq->nullable);
    if (!stack.empty()) {
        Result subresult = frame.result;
        frame = stack.top();
        stack.pop();
        if (ARCHON_LIKELY(!frame.is_alt)) {
            std::size_t elem_index = frame.seq.seq->elems_offset + frame.seq.elem_index;
            ARCHON_ASSERT(elem_index < m_pattern_structure.elems.size());
            const Elem& elem = m_pattern_structure.elems[elem_index];
            switch (elem.type) {
                case Elem::Type::sym:
                    break;
                case Elem::Type::opt:
                    subresult.nullable = true;
                    integrate_seq_subresult(subresult); // Throws
                    goto seq_continue;
                case Elem::Type::rep:
                    register_followpos(subresult.last_pos, subresult.first_pos); // Throws
                    integrate_seq_subresult(subresult); // Throws
                    goto seq_continue;
                case Elem::Type::alt:
                    break;
            }
            ARCHON_ASSERT_UNREACHABLE();
        }
        else {
            integrate_alt_subresult(subresult); // Throws
            goto alt_continue;
        }
    }
    goto finalize;

  alt_continue:
    ARCHON_ASSERT(frame.is_alt);
    ARCHON_ASSERT(frame.alt.seq_index < frame.alt.alt->num_seqs);
    ++frame.alt.seq_index;

  alt_begin:
    ARCHON_ASSERT(frame.is_alt);
    if (frame.alt.seq_index < frame.alt.alt->num_seqs) {
        std::size_t seq_index = frame.alt.alt->seqs_offset + frame.alt.seq_index;
        stack.push(frame); // Throws
        init_seq(seq_index);
        goto seq_begin;
    }

    // alt end
    ARCHON_ASSERT(!stack.empty());
    {
        Result subresult = frame.result;
        frame = stack.top();
        stack.pop();
        ARCHON_ASSERT(!frame.is_alt);
        std::size_t elem_index = frame.seq.seq->elems_offset + frame.seq.elem_index;
        ARCHON_ASSERT(elem_index < m_pattern_structure.elems.size());
        const Elem& elem = m_pattern_structure.elems[elem_index];
        ARCHON_ASSERT(elem.type == Elem::Type::alt);
        integrate_seq_subresult(subresult); // Throws
        goto seq_continue;
    }

  finalize:
    for (std::size_t i = frame.result.first_pos.first; i < frame.result.first_pos.second; ++i) {
        std::size_t pos = m_position_set_elems[i];
        m_nfa.register_startpos(pos); // Throws
    }
    std::size_t pattern_internal_pos = frame.seq.seq->end_pos;
    impl::PatternSymbol dummy_symbol = {};
    std::size_t term_pos = m_nfa.create_position(pattern_index, pattern_internal_pos, dummy_symbol); // Throws
    if (frame.result.nullable)
        m_nfa.register_startpos(term_pos); // Throws
    register_followpos(frame.result.last_pos, term_pos); // Throws
}


template<class C, class T>
inline auto NfaBuilder<C, T>::make_position_set(std::size_t pos) -> PositionSet
{
    std::size_t pos_set_begin = m_position_set_elems.size();
    m_position_set_elems.push_back(pos); // Throws
    std::size_t pos_set_end = m_position_set_elems.size();
    return { pos_set_begin, pos_set_end };
}


template<class C, class T>
auto NfaBuilder<C, T>::position_set_union(PositionSet a, PositionSet b) -> PositionSet
{
    // FIXME: Looks like all union operations unite disjoint sets. If so, the implementation
    // below is needlessly complicated.                 

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


template<class C, class T>
void NfaBuilder<C, T>::register_followpos(PositionSet set, std::size_t pos)
{
    for (std::size_t i = set.first; i < set.second; ++i) {
        std::size_t pos_1 = m_position_set_elems[i];
        std::size_t pos_2 = pos;
        m_nfa.register_followpos(pos_1, pos_2); // Throws
    }
}


template<class C, class T>
void NfaBuilder<C, T>::register_followpos(PositionSet set_1, PositionSet set_2)
{
    for (std::size_t i_1 = set_1.first; i_1 < set_1.second; ++i_1) {
        std::size_t pos_1 = m_position_set_elems[i_1];
        for (std::size_t i_2 = set_2.first; i_2 < set_2.second; ++i_2) {
            std::size_t pos_2 = m_position_set_elems[i_2];
            m_nfa.register_followpos(pos_1, pos_2); // Throws
        }
    }
}


} // namespace archon::cli::impl

#endif // ARCHON_X_CLI_X_IMPL_X_NFA_BUILDER_HPP
