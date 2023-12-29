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

#ifndef ARCHON_X_CLI_X_IMPL_X_PATTERN_FUNC_CHECKER_HPP
#define ARCHON_X_CLI_X_IMPL_X_PATTERN_FUNC_CHECKER_HPP


#include <cstddef>
#include <type_traits>
#include <utility>
#include <array>
#include <tuple>
#include <optional>
#include <variant>
#include <vector>

#include <archon/core/features.h>
#include <archon/core/type_list.hpp>
#include <archon/core/type_traits.hpp>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/cli/impl/pattern_structure.hpp>


namespace archon::cli::impl {


template<class C, class T, class U> class PatternFuncParamChecker;
template<class C, class T, class U> class PatternFuncTupleChecker;



template<class C, class T> class PatternFuncChecker {
public:
    using pattern_structure_type = impl::PatternStructure<C, T>;

    using Elem = typename pattern_structure_type::Elem;
    using Seq  = typename pattern_structure_type::Seq;
    using Alt  = typename pattern_structure_type::Alt;

    PatternFuncChecker(const pattern_structure_type&) noexcept;

    template<class F> bool check(std::size_t seq_index) const noexcept;

private:
    class ParamFunc;
    class BranchFunc;

    const pattern_structure_type& m_pattern_structure;

    template<class U> bool check_tuple(const Seq&) const noexcept;
    template<class U> bool check_opt_param(const Elem&) const noexcept;
    template<class U> bool check_rep_param(const Elem&) const noexcept;
    template<class... U> bool check_alt_param(const Elem&) const noexcept;

    template<class, class, class> friend class impl::PatternFuncParamChecker;
    template<class, class, class> friend class impl::PatternFuncTupleChecker;
};








// Implementation


// ============================ PatternFuncParamChecker ============================


template<class C, class T, class U> class PatternFuncParamChecker {
public:
    using pattern_structure_type    = impl::PatternStructure<C, T>;
    using pattern_func_checker_type = impl::PatternFuncChecker<C, T>;

    using Elem = typename pattern_structure_type::Elem;
    using Seq  = typename pattern_structure_type::Seq;

    static bool check(const pattern_func_checker_type& func_checker, const Elem& elem) noexcept
    {
        if (ARCHON_LIKELY(elem.type == Elem::Type::sym))
            return true;
        ARCHON_ASSERT(elem.is_param);
        if constexpr (std::is_same_v<U, bool>) {
            return (elem.type == Elem::Type::opt && elem.collapsible);
        }
        else if constexpr (std::is_same_v<U, std::size_t>) {
            if (ARCHON_LIKELY(elem.type == Elem::Type::rep || elem.type == Elem::Type::alt))
                return elem.collapsible;
            if (ARCHON_LIKELY(elem.type == Elem::Type::opt)) {
                std::size_t seq_index = elem.index;
                ARCHON_ASSERT(seq_index < func_checker.m_pattern_structure.seqs.size());
                const Seq& seq = func_checker.m_pattern_structure.seqs[seq_index];
                if (ARCHON_LIKELY(seq.num_params == 1)) {
                    for (std::size_t i = 0; i < seq.num_elems; ++i) {
                        std::size_t elem_index = seq.elems_offset + i;
                        ARCHON_ASSERT(elem_index < func_checker.m_pattern_structure.elems.size());
                        const Elem& elem_2 = func_checker.m_pattern_structure.elems[elem_index];
                        if (ARCHON_LIKELY(elem_2.is_param))
                            return (elem_2.type == Elem::Type::rep && elem_2.collapsible);
                    }
                }
            }
            return false;
        }
        else {
            return false;
        }
    }
};


template<class C, class T, class U> class PatternFuncParamChecker<C, T, std::optional<U>> {
public:
    using pattern_structure_type    = impl::PatternStructure<C, T>;
    using pattern_func_checker_type = impl::PatternFuncChecker<C, T>;

    using Elem = typename pattern_structure_type::Elem;

    static bool check(const pattern_func_checker_type& func_checker, const Elem& elem) noexcept
    {
        return func_checker.template check_opt_param<U>(elem);
    }
};


template<class C, class T, class U> class PatternFuncParamChecker<C, T, std::vector<U>> {
public:
    using pattern_structure_type    = impl::PatternStructure<C, T>;
    using pattern_func_checker_type = impl::PatternFuncChecker<C, T>;

    using Elem = typename pattern_structure_type::Elem;

    static bool check(const pattern_func_checker_type& func_checker, const Elem& elem) noexcept
    {
        return func_checker.template check_rep_param<U>(elem);
    }
};


template<class C, class T, class... U> class PatternFuncParamChecker<C, T, std::variant<U...>> {
public:
    using pattern_structure_type    = impl::PatternStructure<C, T>;
    using pattern_func_checker_type = impl::PatternFuncChecker<C, T>;

    using Elem = typename pattern_structure_type::Elem;

    static bool check(const pattern_func_checker_type& func_checker, const Elem& elem) noexcept
    {
        return func_checker.template check_alt_param<U...>(elem);
    }
};



// ============================ PatternFuncTupleChecker ============================


template<class C, class T, class U> class PatternFuncTupleChecker {
public:
    using pattern_structure_type    = impl::PatternStructure<C, T>;
    using pattern_func_checker_type = impl::PatternFuncChecker<C, T>;

    using Seq = typename pattern_structure_type::Seq;

    static bool check(const pattern_func_checker_type& func_checker, const Seq& seq) noexcept
    {
        return func_checker.template check_tuple<std::tuple<U>>(seq);
    }
};


template<class C, class T, class... U> class PatternFuncTupleChecker<C, T, std::tuple<U...>> {
public:
    using pattern_structure_type    = impl::PatternStructure<C, T>;
    using pattern_func_checker_type = impl::PatternFuncChecker<C, T>;

    using Seq = typename pattern_structure_type::Seq;

    static bool check(const pattern_func_checker_type& func_checker, const Seq& seq) noexcept
    {
        return func_checker.template check_tuple<std::tuple<U...>>(seq);
    }
};


template<class C, class T, class U, class V> class PatternFuncTupleChecker<C, T, std::pair<U, V>> {
public:
    using pattern_structure_type    = impl::PatternStructure<C, T>;
    using pattern_func_checker_type = impl::PatternFuncChecker<C, T>;

    using Seq = typename pattern_structure_type::Seq;

    static bool check(const pattern_func_checker_type& func_checker, const Seq& seq) noexcept
    {
        return func_checker.template check_tuple<std::pair<U, V>>(seq);
    }
};


template<class C, class T, class U, std::size_t N>
class PatternFuncTupleChecker<C, T, std::array<U, N>> {
public:
    using pattern_structure_type    = impl::PatternStructure<C, T>;
    using pattern_func_checker_type = impl::PatternFuncChecker<C, T>;

    using Seq = typename pattern_structure_type::Seq;

    static bool check(const pattern_func_checker_type& func_checker, const Seq& seq) noexcept
    {
        return func_checker.template check_tuple<std::array<U, N>>(seq);
    }
};


template<class C, class T> class PatternFuncTupleChecker<C, T, std::monostate> {
public:
    using pattern_structure_type    = impl::PatternStructure<C, T>;
    using pattern_func_checker_type = impl::PatternFuncChecker<C, T>;

    using Seq = typename pattern_structure_type::Seq;

    static bool check(const pattern_func_checker_type& func_checker, const Seq& seq) noexcept
    {
        return func_checker.template check_tuple<std::tuple<>>(seq);
    }
};



// ============================ PatternFuncChecker ============================


template<class C, class T>
class PatternFuncChecker<C, T>::ParamFunc {
public:
    template<class U, std::size_t I>
    static bool exec(const PatternFuncChecker& checker, core::Span<const Elem* const> param_elems) noexcept
    {
        ARCHON_ASSERT(I < param_elems.size());
        return impl::PatternFuncParamChecker<C, T, U>::check(checker, *param_elems[I]);
    }
};


template<class C, class T>
class PatternFuncChecker<C, T>::BranchFunc {
public:
    template<class U, std::size_t I>
    static bool exec(const PatternFuncChecker& checker, std::size_t seqs_offset) noexcept
    {
        std::size_t seq_index = std::size_t(seqs_offset + I);
        ARCHON_ASSERT(seq_index < checker.m_pattern_structure.seqs.size());
        const Seq& seq = checker.m_pattern_structure.seqs[seq_index];
        return impl::PatternFuncTupleChecker<C, T, U>::check(checker, seq);
    }
};


template<class C, class T>
inline PatternFuncChecker<C, T>::PatternFuncChecker(const pattern_structure_type& pattern_structure) noexcept
    : m_pattern_structure(pattern_structure)
{
}


template<class C, class T>
template<class F> inline bool PatternFuncChecker<C, T>::check(std::size_t seq_index) const noexcept
{
    using tuple_type = core::TupleOfDecayedFuncParams<F>;
    ARCHON_ASSERT(seq_index < m_pattern_structure.seqs.size());
    const Seq& seq = m_pattern_structure.seqs[seq_index];
    return check_tuple<tuple_type>(seq);
}


template<class C, class T>
template<class U> bool PatternFuncChecker<C, T>::check_tuple(const Seq& seq) const noexcept
{
    using param_types = core::TypeListFromTuple<U>;
    const std::size_t num_params = core::type_count<param_types>;
    if (ARCHON_LIKELY(seq.num_params == num_params)) {
        std::array<const Elem*, num_params> param_elems = {};
        std::size_t param_index = 0;
        for (std::size_t i = 0; i < seq.num_elems; ++i) {
            std::size_t elem_index = seq.elems_offset + i;
            ARCHON_ASSERT(elem_index < m_pattern_structure.elems.size());
            const Elem& elem = m_pattern_structure.elems[elem_index];
            if (ARCHON_LIKELY(elem.is_param)) {
                ARCHON_ASSERT(param_index < num_params);
                param_elems[param_index] = &elem;
                ++param_index;
            }
        }
        ARCHON_ASSERT(param_index == num_params);
        return core::for_each_type_alt_a<param_types, ParamFunc>(*this, param_elems);
    }
    return false;
}


template<class C, class T>
template<class U> inline bool PatternFuncChecker<C, T>::check_opt_param(const Elem& elem) const noexcept
{
    if (ARCHON_LIKELY(elem.type == Elem::Type::opt)) {
        std::size_t seq_index = elem.index;
        ARCHON_ASSERT(seq_index < m_pattern_structure.seqs.size());
        const Seq& seq = m_pattern_structure.seqs[seq_index];
        return impl::PatternFuncTupleChecker<C, T, U>::check(*this, seq);
    }
    return false;
}


template<class C, class T>
template<class U> inline bool PatternFuncChecker<C, T>::check_rep_param(const Elem& elem) const noexcept
{
    if (ARCHON_LIKELY(elem.type == Elem::Type::rep)) {
        std::size_t seq_index = elem.index;
        ARCHON_ASSERT(seq_index < m_pattern_structure.seqs.size());
        const Seq& seq = m_pattern_structure.seqs[seq_index];
        return impl::PatternFuncTupleChecker<C, T, U>::check(*this, seq);
    }
    else if (ARCHON_LIKELY(elem.type == Elem::Type::opt)) {
        std::size_t seq_index = elem.index;
        ARCHON_ASSERT(seq_index < m_pattern_structure.seqs.size());
        const Seq& seq = m_pattern_structure.seqs[seq_index];
        if (ARCHON_LIKELY(seq.num_params == 1)) {
            for (std::size_t i = 0; i < seq.num_elems; ++i) {
                std::size_t elem_index = seq.elems_offset + i;
                ARCHON_ASSERT(elem_index < m_pattern_structure.elems.size());
                const Elem& elem_2 = m_pattern_structure.elems[elem_index];
                if (ARCHON_LIKELY(elem_2.is_param)) {
                    if (ARCHON_LIKELY(elem_2.type == Elem::Type::rep)) {
                        std::size_t seq_index_2 = elem_2.index;
                        ARCHON_ASSERT(seq_index_2 < m_pattern_structure.seqs.size());
                        const Seq& seq_2 = m_pattern_structure.seqs[seq_index_2];
                        return impl::PatternFuncTupleChecker<C, T, U>::check(*this, seq_2);
                    }
                    break;
                }
            }
        }
    }
    return false;
}


template<class C, class T>
template<class... U> inline bool PatternFuncChecker<C, T>::check_alt_param(const Elem& elem) const noexcept
{
    if (ARCHON_LIKELY(elem.type == Elem::Type::alt)) {
        std::size_t alt_index = elem.index;
        ARCHON_ASSERT(alt_index < m_pattern_structure.alts.size());
        const Alt& alt = m_pattern_structure.alts[alt_index];
        const std::size_t num_branches = sizeof... (U);
        if (ARCHON_LIKELY(alt.num_seqs == num_branches)) {
            using branch_types = core::TypeList<U...>;
            return core::for_each_type_alt_a<branch_types, BranchFunc>(*this, alt.seqs_offset);
        }
    }
    return false;
}


} // namespace archon::cli::impl

#endif // ARCHON_X_CLI_X_IMPL_X_PATTERN_FUNC_CHECKER_HPP
