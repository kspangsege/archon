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

#ifndef ARCHON_X_CLI_X_IMPL_X_PATTERN_STRUCTURE_HPP
#define ARCHON_X_CLI_X_IMPL_X_PATTERN_STRUCTURE_HPP


#include <cstddef>
#include <string_view>
#include <vector>

#include <archon/cli/impl/pattern_symbol.hpp>


namespace archon::cli::impl {


// A record of the structure of a number of patterns.
template<class C, class T> class PatternStructure {
public:
    using string_view_type = std::basic_string_view<C, T>;

    struct Sym;
    struct Elem;
    struct Seq;
    struct Alt;

    std::vector<Sym> syms;
    std::vector<Elem> elems;
    std::vector<Seq> seqs;
    std::vector<Alt> alts;
};


template<class C, class T> struct PatternStructure<C, T>::Sym {
    impl::PatternSymbol sym;
    string_view_type lexeme;
};


// An element of a pattern, or of a sub-pattern.
template<class C, class T> struct PatternStructure<C, T>::Elem {
    enum class Type {
        sym, // Pattern symbol (keywork, option, or value slot)
        opt, // Optionality construct
        rep, // Repetition construct
        alt  // Alternatives construct
    };

    Type type;

    // `is_param` is `true` if `type` is not `sym`, or if `type` is `sym` and the referenced
    // symbol is a value slot. See also Seq::num_params.
    bool is_param;

    // If `type` is `sym`, `collapsible` is `false`. If `type` is `opt` or `rep`,
    // `collapsible` is `true` when, and only when `Seq::num_params` is zero in the
    // referenced element sequence. If `type` is `alt`, `collapsible` is `true` when, and
    // only when `Seq::num_params` is zero in all the branches of the referenced
    // alternatives construct.
    bool collapsible;

    // If `type` is `sym`, `index` is index into `syms`. If `type` is `opt` or `rep`,
    // `index` is index into `seqs`. If `type` is `alt`, `index` is index into `alts`.
    std::size_t index;

    // One beyond position of last symbol (keyword, option, or value slot) within this
    // pattern element. This is a pattern-internal position. Symbol positions are numbered
    // according to the order of the symbols in the string representation of the
    // pattern. The position of the first symbol in the pattern is taken to be zero.
    std::size_t end_pos;
};


// A sequence of pattern elements. It is either the sequence of top-level pattern elements
// of a pattern, i.e., a *root sequence*; or it is the sequence of elements of a
// sub-pattern. For example, in `[-x <foo>]`, `-x <foo>` is a sub-pattern, and `-x` and
// `<foo>` are elements of that sub-pattern. The root sequence has only one element, which
// is the entire optionality construct.
template<class C, class T> struct PatternStructure<C, T>::Seq {
    // Number of pattern elements in this sequence. It may be zero, but only if this is the
    // root sequence of a pattern.
    std::size_t num_elems;

    // For a non-empty sequence of size N, this is the index of the entry in `elems` that
    // correspond to the first element of the sequence, and the subsequent N - 1 enetries in
    // `elems` correspond to the remaining elements of the sequence. For an empty sequence,
    // the value of `elems_offset` is immaterial.
    std::size_t elems_offset;

    // The number of elements in this sequence where Elem::is_param is true.
    //
    // For a root sequence, this is the number of elements that correspond to parameters of
    // the pattern function, and it must therefore match the number of parameters of the
    // pattern function.
    //
    // For a sub-sequence, it is the number of elements that correspond to elements of the
    // corresponding tuple, or tuple-like type argument in the type of relevant parameter of
    // the pattern function.
    //
    // For example, with a pattern `[-x <foo> <bar>]` and a pattern function whose type is
    // `int(std::optional<std::pair<int, int>>)`, `num_params` is 1 for the root sequence,
    // which equals the number of parameters in the pattern function. And `num_params` is 2
    // for the sub-sequence corresponding to the operand of the optionality operator, which
    // equals the number of elements in `std::pair<int, int>`.
    std::size_t num_params;

    // One beyond position of last symbol within this element sequence (see Elem::end_pos),
    // or zero if the sequence is empty (only the root sequence can be empty).
    std::size_t end_pos;

    // True if, and only if all the elements are nullable.
    bool nullable;
};


// An alternatives construct.
template<class C, class T> struct PatternStructure<C, T>::Alt {
    // Number of branches. Never zero.
    std::size_t num_seqs;

    // For an alternatives construct with N branches, this is the index of the entry in
    // `seqs` that correspond to the first branch, and the subsequent N - 1 enetries in
    // `seqs` correspond to the remaining branches of the alternatives construct.
    std::size_t seqs_offset;

    // Index within alternatives construct of first nullable branch (Seq::nullable), or
    // equal to `num_seqs` if no branch is nullable.
    std::size_t nullable_seq_index;
};


} // namespace archon::cli::impl

#endif // ARCHON_X_CLI_X_IMPL_X_PATTERN_STRUCTURE_HPP
