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

#include <stack>
#include <set>
#include <variant>

#include <archon/core/type_list.hpp>
#include <archon/core/utility.hpp>
#include <archon/core/string.hpp>
#include <archon/core/integer_formatter.hpp>
#include <archon/core/format_as.hpp>
#include <archon/core/enum.hpp>
#include <archon/core/value_parser.hpp>
#include <archon/check.hpp>


using namespace archon;


namespace {


class Symbol {
public:
    enum Type { value, option };
    Type type;
    std::string_view which;

    bool operator!=(Symbol s) const noexcept
    {
        return (type != s.type || which != s.which);
    }

    bool operator<(Symbol s) const noexcept
    {
        return (type < s.type || (type == s.type && which < s.which));
    }
};

std::ostream& operator<<(std::ostream& out, Symbol symbol)
{
    switch (symbol.type) {
        case Symbol::Type::value:
            out << "<>";
            break;
        case Symbol::Type::option:
            out << symbol.which;
            break;
    }
    return out;
}



class Nfa {
public:
    using PositionSet = std::set<std::size_t>;

    struct Position {
        std::size_t pattern_index;
        std::size_t pattern_internal_pos;
        Symbol symbol;
        PositionSet follow_pos;
    };

    std::vector<Position> positions;
    PositionSet start_positions;

    std::size_t create_position(std::size_t pattern_index, std::size_t pattern_internal_pos,
                                Symbol symbol)
    {
        std::size_t pos = positions.size();
        positions.push_back({ pattern_index, pattern_internal_pos, symbol, {} }); // Throws
        return pos;
    }

    void register_start_pos(std::size_t pos)
    {
        start_positions.insert(pos); // Throws
    }

    void register_follow_pos(std::size_t a, std::size_t b)
    {
        ARCHON_ASSERT(a < positions.size());
        Position& position = positions[a];
        position.follow_pos.insert(b); // Throws
    }

    void register_follow_pos(PositionSet a, std::size_t b)
    {
        for (std::size_t pos_1 : a) {
            std::size_t pos_2 = b;
            register_follow_pos(pos_1, pos_2); // Throws
        }
    }

    void register_follow_pos(PositionSet a, PositionSet b)
    {
        for (std::size_t pos_1 : a) {
            for (std::size_t pos_2 : b)
                register_follow_pos(pos_1, pos_2); // Throws
        }
    }

    void dump(log::Logger& logger)
    {
        logger.info("Start positions: %s", core::as_sbr_list(start_positions, [](auto v) {
            return core::as_dec_int(v);
        }));
        for (std::size_t i = 0; i < positions.size(); ++i) {
            const auto& position = positions[i];
            logger.info("Position %s:", core::as_dec_int(i));
            logger.info("    Pattern %s", core::as_dec_int(position.pattern_index + 1));
            if (position.follow_pos.empty())
                logger.info("    Is final");
            for (std::size_t pos : position.follow_pos)
                logger.info("    %s -> %s", position.symbol, core::as_dec_int(pos));
        }
    }
};



class PatternStructure {
public:
    struct Elem {
        enum class Type { sym, opt, rep, alt };
        Type type;

        // `is_param` is `true` if `type` is not `sym`, or if `type` is `sym` and the
        // referenced symbol is a value slot.
        bool is_param;

        // If `type` is `sym`, `collapsible` is `false`. If `type` is `opt` or `rep`,
        // `collapsible` is `true` when, and only when `Seq::num_params` is zero in the
        // referenced element sequence. If `type` is `alt`, `collapsible` is `true` when,
        // and only when `Seq::num_params` is zero in all the branches of the referenced
        // alternatives construct.
        bool collapsible;

        // If `type` is `sym`, `index` is index into `syms`. If `type` is `opt` or `rep`,
        // `index` is index into `seqs`. If `type` is `alt`, `index` is index into `alts`.
        std::size_t index;

        // One beyond position of last symbol (keyword, option, or value slot) within this
        // pattern element. This is a pattern-internal position. Symbol positions are
        // numbered according to the order of the symbols in the string form of the
        // pattern. The position of the first symbol in the pattern is taken to be zero.
        std::size_t end_pos;
    };

    struct Seq {
        std::size_t num_elems;
        std::size_t elems_offset;
        std::size_t num_params;

        // One beyond position of last symbol within this element sequence (see
        // Elem::end_pos), or zero if the sequence is empty (only the root sequence can be
        // empty).
        std::size_t end_pos;

        bool nullable;
    };

    struct Alt {
        std::size_t num_seqs;
        std::size_t seqs_offset;

        // Index of first nullable branch, or equal to `num_seqs` if no branch is nullable.
        std::size_t nullable_seq_index;
    };

    std::vector<Symbol> syms;
    std::vector<Elem> elems;
    std::vector<Seq> seqs;
    std::vector<Alt> alts;

    class Snapshot;

    Snapshot snapshot() const noexcept;
    void revert(Snapshot) noexcept;

    void dump(std::size_t seq_index, log::Logger& logger) const
    {
        dump_seq(seq_index, logger, 0);
    }

private:
    static auto indent(int level)
    {
        return core::with_width("", 2 * level);
    }

    void dump_seq(std::size_t seq_index, log::Logger& logger, int level) const
    {
        const Seq& seq = seqs.at(seq_index);
        logger.info("%sSeq (num_params=%s, nullable=%s):", indent(level), seq.num_params,
                    seq.nullable);
        for (std::size_t i = 0; i < seq.num_elems; ++i)
            dump_elem(seq.elems_offset + i, logger, level + 1);
    }

    void dump_elem(std::size_t elem_index, log::Logger& logger, int level) const
    {
        const Elem& elem = elems.at(elem_index);
        switch (elem.type) {
            case Elem::Type::sym:
                logger.info("%sSym %s (is_param=%s, collapsible=%s, end_pos=%s):", indent(level),
                            syms.at(elem.index), elem.is_param, elem.collapsible, elem.end_pos);
                break;
            case Elem::Type::opt:
                logger.info("%sOpt (is_param=%s, collapsible=%s, end_pos=%s):", indent(level),
                            elem.is_param, elem.collapsible, elem.end_pos);
                dump_seq(elem.index, logger, level + 1);
                break;
            case Elem::Type::rep:
                logger.info("%sRep (is_param=%s, collapsible=%s, end_pos=%s):", indent(level),
                            elem.is_param, elem.collapsible, elem.end_pos);
                dump_seq(elem.index, logger, level + 1);
                break;
            case Elem::Type::alt: {
                std::size_t alt_index = elem.index;
                const Alt& alt = alts.at(alt_index);
                logger.info("%sAlt (is_param=%s, collapsible=%s, end_pos=%s, "
                            "nullable_seq_index=%s):", indent(level), elem.is_param,
                            elem.collapsible, elem.end_pos, alt.nullable_seq_index);
                for (std::size_t i = 0; i < alt.num_seqs; ++i)
                    dump_seq(alt.seqs_offset + i, logger, level + 1);
            }
        }
    }
};


class PatternStructure::Snapshot {
private:
    std::size_t m_syms_watermark;
    std::size_t m_elems_watermark;
    std::size_t m_seqs_watermark;
    std::size_t m_alts_watermark;

    friend class PatternStructure;
};


inline auto PatternStructure::snapshot() const noexcept -> Snapshot
{
    Snapshot snapshot;
    snapshot.m_syms_watermark  = syms.size();
    snapshot.m_elems_watermark = elems.size();
    snapshot.m_seqs_watermark  = seqs.size();
    snapshot.m_alts_watermark  = alts.size();
    return snapshot;
}


void PatternStructure::revert(Snapshot snapshot) noexcept
{
    auto resize = [](auto& vec, std::size_t size) noexcept {
        ARCHON_ASSERT(vec.size() >= size);
        vec.resize(size);
    };
    resize(syms, snapshot.m_syms_watermark);
    resize(elems, snapshot.m_elems_watermark);
    resize(seqs, snapshot.m_seqs_watermark);
    resize(alts, snapshot.m_alts_watermark);
}



} // unnamed namespace

namespace archon::core {

template<> struct EnumTraits<PatternStructure::Elem::Type> {
    static constexpr bool is_specialized = true;
    struct Spec {
        static constexpr EnumAssoc map[] = {
            { int(PatternStructure::Elem::Type::sym), "sym" },
            { int(PatternStructure::Elem::Type::opt), "opt" },
            { int(PatternStructure::Elem::Type::rep), "rep" },
            { int(PatternStructure::Elem::Type::alt), "alt" },
        };
    };
    static constexpr bool ignore_case = false;
};

} // namespace archon::core

namespace {



template<class T> std::set<T> set_union(const std::set<T>& a, const std::set<T>& b)
{
    std::set<T> c = a;
    c.insert(b.begin(), b.end());
    return c;
}



class PatternFuncChecker {
public:
    using Elem = PatternStructure::Elem;
    using Seq  = PatternStructure::Seq;
    using Alt  = PatternStructure::Alt;

    PatternFuncChecker(const PatternStructure&) noexcept;

    template<class F> bool check(std::size_t seq_index) const noexcept;

private:
    class ParamFunc;
    class BranchFunc;

    template<class T> class CheckParam;
    template<class T> class CheckTuple;

    const PatternStructure& m_pattern_structure;

    template<class T> bool check_tuple(const Seq&) const noexcept;
    template<class T> bool check_opt_param(const Elem&) const noexcept;
    template<class T> bool check_rep_param(const Elem&) const noexcept;
    template<class... T> bool check_alt_param(const Elem&) const noexcept;
};


class PatternFuncChecker::ParamFunc {
public:
    template<class T, std::size_t I>
    static bool exec(const PatternFuncChecker& checker,
                     core::Span<const Elem* const> param_elems) noexcept
    {
        ARCHON_ASSERT(I < param_elems.size());
        return CheckParam<T>::check(checker, *param_elems[I]);
    }
};


class PatternFuncChecker::BranchFunc {
public:
    template<class T, std::size_t I>
    static bool exec(const PatternFuncChecker& checker, std::size_t seqs_offset) noexcept
    {
        std::size_t seq_index = std::size_t(seqs_offset + I);
        ARCHON_ASSERT(seq_index < checker.m_pattern_structure.seqs.size());
        const Seq& seq = checker.m_pattern_structure.seqs[seq_index];
        return CheckTuple<T>::check(checker, seq);
    }
};


template<class T> class PatternFuncChecker::CheckParam {
public:
    static bool check(const PatternFuncChecker& checker, const Elem& elem) noexcept
    {
        if (ARCHON_LIKELY(elem.type == Elem::Type::sym))
            return true;
        ARCHON_ASSERT(elem.is_param);
        if constexpr (std::is_same_v<T, bool>) {
            return (elem.type == Elem::Type::opt && elem.collapsible);
        }
        else if constexpr (std::is_same_v<T, std::size_t>) {
            if (ARCHON_LIKELY(elem.type == Elem::Type::rep || elem.type == Elem::Type::alt))
                return elem.collapsible;
            if (ARCHON_LIKELY(elem.type == Elem::Type::opt)) {
                std::size_t seq_index = elem.index;
                ARCHON_ASSERT(seq_index < checker.m_pattern_structure.seqs.size());
                const Seq& seq = checker.m_pattern_structure.seqs[seq_index];
                if (ARCHON_LIKELY(seq.num_params == 1)) {
                    for (std::size_t i = 0; i < seq.num_elems; ++i) {
                        std::size_t elem_index = seq.elems_offset + i;
                        ARCHON_ASSERT(elem_index < checker.m_pattern_structure.elems.size());
                        const Elem& elem_2 = checker.m_pattern_structure.elems[elem_index];
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

template<class T> class PatternFuncChecker::CheckParam<std::optional<T>> {
public:
    static bool check(const PatternFuncChecker& checker, const Elem& elem) noexcept
    {
        return checker.check_opt_param<T>(elem);
    }
};

template<class T> class PatternFuncChecker::CheckParam<std::vector<T>> {
public:
    static bool check(const PatternFuncChecker& checker, const Elem& elem) noexcept
    {
        return checker.check_rep_param<T>(elem);
    }
};

template<class... T> class PatternFuncChecker::CheckParam<std::variant<T...>> {
public:
    static bool check(const PatternFuncChecker& checker, const Elem& elem) noexcept
    {
        return checker.check_alt_param<T...>(elem);
    }
};


template<class T> class PatternFuncChecker::CheckTuple {
public:
    static bool check(const PatternFuncChecker& checker, const Seq& seq) noexcept
    {
        return checker.check_tuple<std::tuple<T>>(seq);
    }
};

template<class... T> class PatternFuncChecker::CheckTuple<std::tuple<T...>> {
public:
    static bool check(const PatternFuncChecker& checker, const Seq& seq) noexcept
    {
        return checker.check_tuple<std::tuple<T...>>(seq);
    }
};

template<class T, class U> class PatternFuncChecker::CheckTuple<std::pair<T, U>> {
public:
    static bool check(const PatternFuncChecker& checker, const Seq& seq) noexcept
    {
        return checker.check_tuple<std::pair<T, U>>(seq);
    }
};

template<class T, std::size_t N> class PatternFuncChecker::CheckTuple<std::array<T, N>> {
public:
    static bool check(const PatternFuncChecker& checker, const Seq& seq) noexcept
    {
        return checker.check_tuple<std::array<T, N>>(seq);
    }
};

template<> class PatternFuncChecker::CheckTuple<std::monostate> {
public:
    static bool check(const PatternFuncChecker& checker, const Seq& seq) noexcept
    {
        return checker.check_tuple<std::tuple<>>(seq);
    }
};


inline PatternFuncChecker::PatternFuncChecker(const PatternStructure& pattern_structure) noexcept
    : m_pattern_structure(pattern_structure)
{
}


template<class F> inline bool PatternFuncChecker::check(std::size_t seq_index) const noexcept
{
    using tuple_type = core::TupleOfFuncArgs<F>;
    ARCHON_ASSERT(seq_index < m_pattern_structure.seqs.size());
    const Seq& seq = m_pattern_structure.seqs[seq_index];
    return check_tuple<tuple_type>(seq);
}


template<class T> bool PatternFuncChecker::check_tuple(const Seq& seq) const noexcept
{
    using param_types = core::TypeListFromTuple<T>;
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


template<class T> inline bool PatternFuncChecker::check_opt_param(const Elem& elem) const noexcept
{
    if (ARCHON_LIKELY(elem.type == Elem::Type::opt)) {
        std::size_t seq_index = elem.index;
        ARCHON_ASSERT(seq_index < m_pattern_structure.seqs.size());
        const Seq& seq = m_pattern_structure.seqs[seq_index];
        return CheckTuple<T>::check(*this, seq);
    }
    return false;
}


template<class T> inline bool PatternFuncChecker::check_rep_param(const Elem& elem) const noexcept
{
    if (ARCHON_LIKELY(elem.type == Elem::Type::rep)) {
        std::size_t seq_index = elem.index;
        ARCHON_ASSERT(seq_index < m_pattern_structure.seqs.size());
        const Seq& seq = m_pattern_structure.seqs[seq_index];
        return CheckTuple<T>::check(*this, seq);
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
                        return CheckTuple<T>::check(*this, seq_2);
                    }
                    break;
                }
            }
        }
    }
    return false;
}


template<class... T> inline bool PatternFuncChecker::check_alt_param(const Elem& elem) const noexcept
{
    if (ARCHON_LIKELY(elem.type == Elem::Type::alt)) {
        std::size_t alt_index = elem.index;
        ARCHON_ASSERT(alt_index < m_pattern_structure.alts.size());
        const Alt& alt = m_pattern_structure.alts[alt_index];
        const std::size_t num_branches = sizeof... (T);
        if (ARCHON_LIKELY(alt.num_seqs == num_branches)) {
            using branch_types = core::TypeList<T...>;
            return core::for_each_type_alt_a<branch_types, BranchFunc>(*this, alt.seqs_offset);
        }
    }
    return false;
}



class PatternAction {
public:
    using Elem = PatternStructure::Elem;

    struct Desc {
        Elem::Type type;
        bool collapsible;
        std::size_t value;
    };

    virtual void check(std::size_t seq_index, const PatternStructure&) const = 0;
    virtual int invoke(core::Span<const Desc>, core::Span<const std::string> args) const = 0;
    virtual ~PatternAction() = default;
};

template<class F> class FuncExecPatternAction
    : public PatternAction {
public:
    using func_type = F;

    FuncExecPatternAction(std::function<func_type>);

    void check(std::size_t seq_index, const PatternStructure&) const override final;
    int invoke(core::Span<const Desc>, core::Span<const std::string> args) const override final;

private:
    class Parser;

    const std::function<func_type> m_func;
};


template<class F> class FuncExecPatternAction<F>::Parser {
public:
    Parser(core::Span<const Desc> descs, core::Span<const std::string> args) noexcept
        : m_desc(descs.data())
        , m_desc_end(descs.data() + descs.size())
        , m_args(args.data())
        , m_num_args(args.size())
    {
    }

    template<class T> bool parse(T& elems)
    {
        return parse_pattern(elems); // Throws
    }

private:
    const Desc* m_desc;
    const Desc* m_desc_end;
    const std::string* m_args;
    std::size_t m_num_args;
    core::ValueParser m_value_parser;

    template<class... T> bool parse_pattern(std::tuple<T...>& elems)
    {
        return core::for_each_tuple_elem_a(elems, [&](auto& elem) {
            return parse_elem(elem); // Throws
        }); // Throws
    }

    template<class T, class U> bool parse_pattern(std::pair<T, U>& elems)
    {
        return core::for_each_tuple_elem_a(elems, [&](auto& elem) {
            return parse_elem(elem); // Throws
        }); // Throws
    }

    template<class T, std::size_t N> bool parse_pattern(std::array<T, N>& elems)
    {
        return core::for_each_tuple_elem_a(elems, [&](auto& elem) {
            return parse_elem(elem); // Throws
        }); // Throws
    }

    bool parse_pattern(std::monostate&)
    {
        return true;
    }

    template<class T> bool parse_pattern(T& elem)
    {
        return parse_elem(elem); // Throws
    }

    template<class T> bool parse_elem(std::optional<T>& opt)
    {
        Desc desc = next();
        ARCHON_ASSERT(desc.type == Elem::Type::opt);
        ARCHON_ASSERT(desc.value < 2);
        bool present = (desc.value > 0);
        if (ARCHON_LIKELY(!present))
            return true;
        return parse_pattern(*opt); // Throws
    }

    template<class T> bool parse_elem(std::vector<T>& vec)
    {
        Desc desc = next();
        if (ARCHON_UNLIKELY(desc.type == Elem::Type::opt)) {
            bool present = (desc.value > 0);
            if (ARCHON_LIKELY(!present))
                return true;
            desc = next();
        }
        ARCHON_ASSERT(desc.type == Elem::Type::rep);
        std::size_t num_repetitions = desc.value;
        vec.resize(num_repetitions); // Throws
        for (std::size_t i = 0; i < num_repetitions; ++i) {
            if (ARCHON_LIKELY(parse_pattern(vec[i]))) // Throws
                continue;
            return false;
        }
        return true;
    }

    template<class... T> struct ParseVariant {
        template<std::size_t I> static bool exec(Parser& parser, std::variant<T...>& var)
        {
            return parser.parse_pattern(var.template emplace<I>()); // Throws
        }
    };

    template<class... T> bool parse_elem(std::variant<T...>& var)
    {
        Desc desc = next();
        ARCHON_ASSERT(desc.type == Elem::Type::alt);
        std::size_t branch_index = desc.value;
        constexpr std::size_t num_branches = sizeof... (T);
        ARCHON_ASSERT(branch_index < num_branches);
        return core::dispatch<ParseVariant<T...>, num_branches>(branch_index, *this, var); // Throws
    }

    template<class T> bool parse_elem(T& elem)
    {
        Desc desc = next();
        if constexpr (std::is_same_v<T, bool>) {
            if (ARCHON_LIKELY(desc.type != Elem::Type::opt))
                goto regular;
            ARCHON_ASSERT(desc.collapsible);
            bool present = (desc.value > 0);
            elem = present;
            return true;
        }
        else if constexpr (std::is_same_v<T, std::size_t>) {
            if (ARCHON_LIKELY(desc.type != Elem::Type::opt)) {
                if (ARCHON_LIKELY(desc.type != Elem::Type::rep))
                    goto regular;
            }
            else {
                bool present = (desc.value > 0);
                if (ARCHON_LIKELY(!present)) {
                    elem = 0;
                    return true;
                }
                desc = next();
                ARCHON_ASSERT(desc.type == Elem::Type::rep);
            }
            ARCHON_ASSERT(desc.collapsible);
            std::size_t num_repetitions = desc.value;
            elem = num_repetitions;
            return true;
        }

      regular:
        ARCHON_ASSERT(desc.type == Elem::Type::sym);
        std::size_t arg_index = desc.value;
        ARCHON_ASSERT(arg_index < m_num_args);
        std::string_view arg = m_args[arg_index];
        return parse_arg(arg, elem); // Throws
    }

    template<class T> bool parse_arg(std::string_view string, T& elem)
    {
        return m_value_parser.parse(string, elem); // Throws
    }

    Desc next() noexcept
    {
        ARCHON_ASSERT(m_desc < m_desc_end);
        Desc desc = *m_desc;
        ++m_desc;
        return desc;
    }
};


template<class F> inline FuncExecPatternAction<F>::FuncExecPatternAction(std::function<func_type> func)
    : m_func(std::move(func))
{
}


template<class F> void FuncExecPatternAction<F>::check(std::size_t seq_index, const PatternStructure& pattern_structure) const
{
    PatternFuncChecker checker(pattern_structure);
    if (ARCHON_LIKELY(checker.check<func_type>(seq_index)))
        return;
    throw std::runtime_error("Mismatch between pattern and pattern function");
}


template<class F> int FuncExecPatternAction<F>::invoke(core::Span<const Desc> descs, core::Span<const std::string> args) const
{
    using args_tuple_type = core::TupleOfFuncArgs<func_type>;
    args_tuple_type args_2;
    Parser parser(descs, args);
    if (ARCHON_LIKELY(parser.parse(args_2))) // Throws
        return std::apply(m_func, std::move(args_2)); // Throws
    return 1;    
}



class Expr {
public:
    virtual ~Expr() = default;
};


class Sym
    : public Expr {
public:
    Sym(std::string_view lexeme)
        : m_lexeme(std::move(lexeme))
    {
        if (m_lexeme.starts_with("-"sv)) {
            m_type = Symbol::Type::option;
        }
        else if (m_lexeme.starts_with("<"sv)) {
            m_type = Symbol::Type::value;
        }
        else {
            throw std::runtime_error("Bad symbol");
        }
    }

    Symbol::Type get_type() const noexcept
    {
        return m_type;
    }

    std::string_view get_lexeme() const noexcept
    {
        return m_lexeme;
    }

private:
    Symbol::Type m_type;
    std::string_view m_lexeme;
};


class Opt
    : public Expr {
public:
    Opt(std::unique_ptr<Expr> expr)
        : m_expr(std::move(expr))
    {
    }

    const Expr& get_expr() const noexcept
    {
        return *m_expr;
    }

private:
    std::unique_ptr<Expr> m_expr;
};


class Rep
    : public Expr {
public:
    Rep(std::unique_ptr<Expr> expr)
        : m_expr(std::move(expr))
    {
    }

    const Expr& get_expr() const noexcept
    {
        return *m_expr;
    }

private:
    std::unique_ptr<Expr> m_expr;
};


class Cat
    : public Expr {
public:
    Cat(std::unique_ptr<Expr> left, std::unique_ptr<Expr> right)
        : m_left(std::move(left))
        , m_right(std::move(right))
    {
    }

    const Expr& get_left() const noexcept
    {
        return *m_left;
    }

    const Expr& get_right() const noexcept
    {
        return *m_right;
    }

private:
    std::unique_ptr<Expr> m_left, m_right;
};


class Alt
    : public Expr {
public:
    Alt(std::unique_ptr<Expr> left, std::unique_ptr<Expr> right)
        : m_left(std::move(left))
        , m_right(std::move(right))
    {
    }

    const Expr& get_left() const noexcept
    {
        return *m_left;
    }

    const Expr& get_right() const noexcept
    {
        return *m_right;
    }

private:
    std::unique_ptr<Expr> m_left, m_right;
};



class Dfa {
public:
    struct State {
        Nfa::PositionSet positions;
        std::optional<std::size_t> final_pos;
        std::map<Symbol, std::size_t> transitions;
    };

    struct Edge {
        std::size_t prior_state_index;
        Symbol symbol;
    };

    std::vector<State> states;

    void init(const Nfa& nfa, log::Logger&)
    {
        bool allow_cross_pattern_ambiguity = false;
        bool allow_internal_pattern_ambiguity = true; // false;

        if (nfa.start_positions.empty())
            return;

        std::map<Nfa::PositionSet, std::size_t> state_map;
        auto ensure_state = [&](const Nfa::PositionSet& position_set) {
            std::size_t state_index = states.size();
            auto p = state_map.emplace(position_set, state_index);
            auto i = p.first;
            bool was_inserted = p.second;
            if (!was_inserted) {
                state_index = i->second;
            }
            else {
                std::optional<std::size_t> final_pos;
                for (std::size_t pos : position_set) {
                    const Nfa::Position& position = nfa.positions.at(pos);
                    bool position_is_final = position.follow_pos.empty();
                    if (!position_is_final)
                        continue;
                    if (!final_pos.has_value()) {
                        final_pos = pos;
                        continue;
                    }
                    const Nfa::Position& prior_position = nfa.positions.at(final_pos.value());
                    std::size_t prior_pattern_index = prior_position.pattern_index;
                    ARCHON_ASSERT(prior_pattern_index < position.pattern_index);
                    if (allow_cross_pattern_ambiguity) {
                        // Resolve ambiguity by choosing the pattern that was
                        // specified first.
                        continue;
                    }
                    throw std::runtime_error("------------------------------------------->>>>> Cross pattern ambiguity");
                }
                states.push_back({ position_set, final_pos, {} });
            }
            return state_index;
        };
        ensure_state(nfa.start_positions);
        std::size_t num_checked_states = 0;
        do {
            std::map<Symbol, Nfa::PositionSet> target_sets;
            {
                State& state = states.at(num_checked_states);
                for (std::size_t pos : state.positions) {
                    const Nfa::Position& position = nfa.positions.at(pos);
                    if (!position.follow_pos.empty()) {
                        Symbol symbol = position.symbol;
                        Nfa::PositionSet& position_set = target_sets[symbol];
                        for (std::size_t pos_2 : position.follow_pos) {
                            auto p = position_set.insert(pos_2);
                            bool was_inserted = p.second;
                            if (was_inserted || allow_internal_pattern_ambiguity)
                                continue;
                            throw std::runtime_error("------------------------------------------->>>>> Internal pattern ambiguity");
                        }
                    }
                }
            }
            for (const auto& entry : target_sets) {
                Symbol symbol = entry.first;
                const Nfa::PositionSet& position_set = entry.second;
                ARCHON_ASSERT(!position_set.empty());
                std::size_t target_state_index = ensure_state(position_set);
                State& state = states.at(num_checked_states);
                state.transitions[symbol] = target_state_index;
            }
            ++num_checked_states;
        }
        while (num_checked_states < states.size());
    }

    bool consume(Symbol symbol, std::size_t& state_index) const
    {
        if (!states.empty()) {
            const State& state = states.at(state_index);
            auto i = state.transitions.find(symbol);
            if (i != state.transitions.end()) {
                state_index = i->second;
                return true;
            }
        }
        return false;
    }

    void dump(const Nfa& nfa, log::Logger& logger)
    {
        for (std::size_t i = 0; i < states.size(); ++i) {
            const auto& state = states[i];
            logger.info("State %s:", core::as_dec_int(i));
            logger.info("    Positions: %s", core::as_sbr_list(state.positions, [](auto v) {
                return core::as_dec_int(v);
            }));
            if (state.final_pos.has_value()) {
                logger.info("    Final pos: %s (pattern %s)",
                            core::as_dec_int(*state.final_pos),
                            core::as_dec_int(1 + nfa.positions.at(*state.final_pos).pattern_index));
            }
            for (const auto& entry : state.transitions)
                logger.info("    %s -> %s", entry.first, core::as_dec_int(entry.second));
        }
    }
};



class Spec {
public:
    struct Pattern {
        std::size_t seq_index;
        std::unique_ptr<PatternAction> action;
    };
    std::vector<Pattern> patterns;
};



class PatternMatcher {
public:
    using Pattern = Spec::Pattern;

    struct HistoryEntry {
        Dfa::Edge edge;
    };

    PatternMatcher(const Spec& spec, const PatternStructure& pattern_structure, log::Logger& logger)
    {
        std::size_t n = spec.patterns.size();
        for (std::size_t i = 0; i < n; ++i)
            add_pattern_to_nfa(i, spec, pattern_structure); // Throws
        m_nfa.dump(logger);
        m_dfa.init(m_nfa, logger);
        m_dfa.dump(m_nfa, logger);
    }

    bool consume(Symbol symbol)
    {
        std::size_t orig_state_index = m_state_index;
        if (m_dfa.consume(symbol, m_state_index)) {
            m_history.push_back({ orig_state_index, symbol });
            return true;
        }
        return false;
    }

    bool is_match(std::size_t& pattern_index, std::vector<std::size_t>& positions, log::Logger& logger)
    {
        const Dfa::State& state = m_dfa.states.at(m_state_index);
        if (!state.final_pos.has_value())
            return false;
        logger.info("History: %s", core::as_sbr_list(m_history, [](const auto& entry) {
            return core::formatted("(%s, %s)", entry.edge.prior_state_index, entry.edge.symbol);
        }));
        logger.info("Final state: %s", core::as_dec_int(m_state_index));
        pattern_index = m_nfa.positions.at(*state.final_pos).pattern_index;
        backtrack(positions, logger);
        return true;
    }

private:
    Nfa m_nfa;
    Dfa m_dfa;
    std::size_t m_state_index = 0;
    std::vector<HistoryEntry> m_history;

    void add_pattern_to_nfa(std::size_t pattern_index, const Spec& spec, const PatternStructure& pattern_structure)
    {
        using PositionSet = Nfa::PositionSet;
        using Seq  = PatternStructure::Seq;
        using Elem = PatternStructure::Elem;
        using Alt  = PatternStructure::Alt;

        struct Result {
            PositionSet first_pos, last_pos;
            bool nullable = false;
        };

        struct Frame {
            struct SeqSlot {
                const Seq* seq;
                std::size_t elem_index;
            };
            struct AltSlot {
                const Alt* alt;
                std::size_t seq_index;
            };
            bool is_alt;
            union {
                SeqSlot seq;
                AltSlot alt;
            };
            Result result;
        };
        Frame frame = {};
        std::stack<Frame, core::Vector<Frame, 7>> stack;

        auto init_seq = [&](std::size_t seq_index) noexcept {
            ARCHON_ASSERT(seq_index < pattern_structure.seqs.size());
            frame.is_alt = false;
            frame.seq = { &pattern_structure.seqs[seq_index], 0 };
            frame.result = {};
            frame.result.nullable = true;
        };

        auto init_alt = [&](std::size_t alt_index) noexcept {
            ARCHON_ASSERT(alt_index < pattern_structure.alts.size());
            frame.is_alt = true;
            frame.alt = { &pattern_structure.alts[alt_index], 0 };
            frame.result = {};
        };

        auto integrate_seq_subresult = [&](Result&& subresult) {
            ARCHON_ASSERT(!frame.is_alt);
            Result& result_1 = frame.result;
            Result& result_2 = subresult;
            m_nfa.register_follow_pos(result_1.last_pos, result_2.first_pos); // Throws
            Result result;
            if (!result_1.nullable) {
                result.first_pos = std::move(result_1.first_pos);
            }
            else {
                result.first_pos = set_union(std::move(result_1.first_pos), std::move(result_2.first_pos)); // Throws
            }
            if (!result_2.nullable) {
                result.last_pos = std::move(result_2.last_pos);
            }
            else {
                result.last_pos = set_union(std::move(result_1.last_pos), std::move(result_2.last_pos)); // Throws
            }
            result.nullable = (result_1.nullable && result_2.nullable);
            frame.result = std::move(result);
        };

        auto integrate_alt_subresult = [&](Result&& subresult) {
            ARCHON_ASSERT(frame.is_alt);
            Result& result_1 = frame.result;
            Result& result_2 = subresult;
            Result result;
            result.first_pos = set_union(std::move(result_1.first_pos), std::move(result_2.first_pos)); // Throws
            result.last_pos  = set_union(std::move(result_1.last_pos), std::move(result_2.last_pos)); // Throws
            result.nullable = (result_1.nullable || result_2.nullable);
            frame.result = std::move(result);
        };

        const Pattern& pattern = spec.patterns[pattern_index];
        init_seq(pattern.seq_index);
        goto seq_begin;

      seq_continue:
        ARCHON_ASSERT(!frame.is_alt);
        ARCHON_ASSERT(frame.seq.elem_index < frame.seq.seq->num_elems);
        ++frame.seq.elem_index;

      seq_begin:
        ARCHON_ASSERT(!frame.is_alt);
        if (frame.seq.elem_index < frame.seq.seq->num_elems) {
            std::size_t elem_index = frame.seq.seq->elems_offset + frame.seq.elem_index;
            ARCHON_ASSERT(elem_index < pattern_structure.elems.size());
            const Elem& elem = pattern_structure.elems[elem_index];
            switch (elem.type) {
                case Elem::Type::sym: {
                    ARCHON_ASSERT(elem.end_pos > 0);
                    std::size_t pattern_internal_pos = std::size_t(elem.end_pos - 1);
                    std::size_t sym_index = elem.index;
                    ARCHON_ASSERT(sym_index < pattern_structure.syms.size());
                    Symbol symbol = pattern_structure.syms[sym_index];
                    std::size_t pos = m_nfa.create_position(pattern_index, pattern_internal_pos,
                                                            symbol); // Throws
                    Result subresult;
                    subresult.first_pos = { pos };
                    subresult.last_pos  = { pos };
                    integrate_seq_subresult(std::move(subresult)); // Throws
                    goto seq_continue;
                }
                case Elem::Type::opt:
                case Elem::Type::rep: {
                    stack.push(std::move(frame)); // Throws
                    init_seq(elem.index);
                    goto seq_begin;
                }
                case Elem::Type::alt: {
                    stack.push(std::move(frame)); // Throws
                    init_alt(elem.index);
                    goto alt_begin;
                }
            }
            ARCHON_ASSERT_UNREACHABLE();
        }

        // seq end
        ARCHON_ASSERT(frame.result.nullable == frame.seq.seq->nullable);
        if (!stack.empty()) {
            Result subresult = std::move(frame.result);
            frame = std::move(stack.top());
            stack.pop();
            if (ARCHON_LIKELY(!frame.is_alt)) {
                std::size_t elem_index = frame.seq.seq->elems_offset + frame.seq.elem_index;
                ARCHON_ASSERT(elem_index < pattern_structure.elems.size());
                const Elem& elem = pattern_structure.elems[elem_index];
                switch (elem.type) {
                    case Elem::Type::sym:
                        break;
                    case Elem::Type::opt:
                        subresult.nullable = true;
                        integrate_seq_subresult(std::move(subresult)); // Throws
                        goto seq_continue;
                    case Elem::Type::rep:
                        m_nfa.register_follow_pos(subresult.last_pos, subresult.first_pos); // Throws
                        integrate_seq_subresult(std::move(subresult)); // Throws
                        goto seq_continue;
                    case Elem::Type::alt:
                        break;
                }
                ARCHON_ASSERT_UNREACHABLE();
            }
            else {
                integrate_alt_subresult(std::move(subresult)); // Throws
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
            stack.push(std::move(frame)); // Throws
            init_seq(seq_index);
            goto seq_begin;
        }

        // alt end
        ARCHON_ASSERT(!stack.empty());
        {
            Result subresult = std::move(frame.result);
            frame = std::move(stack.top());
            stack.pop();
            ARCHON_ASSERT(!frame.is_alt);
            std::size_t elem_index = frame.seq.seq->elems_offset + frame.seq.elem_index;
            ARCHON_ASSERT(elem_index < pattern_structure.elems.size());
            const Elem& elem = pattern_structure.elems[elem_index];
            ARCHON_ASSERT(elem.type == Elem::Type::alt);
            integrate_seq_subresult(std::move(subresult)); // Throws
            goto seq_continue;
        }

      finalize:
        for (std::size_t pos : frame.result.first_pos)
            m_nfa.register_start_pos(pos); // Throws
        std::size_t pattern_internal_pos = frame.seq.seq->end_pos;
        Symbol dummy_symbol = {};
        std::size_t term_pos = m_nfa.create_position(pattern_index, pattern_internal_pos,
                                                     dummy_symbol); // Throws
        if (frame.result.nullable)
            m_nfa.register_start_pos(term_pos); // Throws
        m_nfa.register_follow_pos(frame.result.last_pos, term_pos); // Throws
    }

    void backtrack(std::vector<std::size_t>& positions, log::Logger& logger) const
    {
        std::size_t state_index = m_state_index;
        const Dfa::State& final_state = m_dfa.states.at(state_index);
        ARCHON_ASSERT(final_state.final_pos.has_value());
        std::size_t pos = *final_state.final_pos;
        logger.info("Matched pattern: %s",
                    core::as_dec_int(1 + m_nfa.positions[pos].pattern_index));
        logger.info("Final position: %s", core::as_dec_int(pos));
        std::size_t i = m_history.size();
        core::int_add(i, 1); // Throws
        positions.resize(i);
        --i;
      again:
        positions[i] = m_nfa.positions[pos].pattern_internal_pos;
        if (ARCHON_LIKELY(i > 0)) {
            --i;
            const HistoryEntry& entry = m_history[i];
            std::size_t state_index_2 = entry.edge.prior_state_index;
            const Dfa::State& prior_state = m_dfa.states.at(state_index_2);
            std::optional<std::size_t> prior_pos;
            for (std::size_t pos_2 : prior_state.positions) {
                const Nfa::Position& position = m_nfa.positions.at(pos_2);
                if (position.symbol != entry.edge.symbol)
                    continue;
                if (position.follow_pos.count(pos) == 0)
                    continue;
                // In case of internal pattern ambiguity, we resolve it by choosing the
                // "left-most path" through the pattern.
                prior_pos = pos_2;
                break;
            }
            // Because for a given DFA edge, a position, P, is in the target DFA state
            // precisely when there is an edge in the NFA from a position in the origin DFA
            // state to P.
            ARCHON_ASSERT(prior_pos.has_value());
            state_index = state_index_2;
            pos = *prior_pos;
            logger.info("Backtrack to position %s at state %s", core::as_dec_int(pos),
                        core::as_dec_int(state_index));
            goto again;
        }
        ARCHON_ASSERT(state_index == 0);
    }
};



std::size_t register_pattern_struct(const Expr* expr, PatternStructure& pattern_structure,
                                    log::Logger& logger)
{
    struct Result {
        // `nullable` is `true` if, and only if the empty list of command-line arguments
        // matches the sub-pattern represented by this result.
        bool nullable;

        // `has_repeatable_match` is `true` if, and only if there exists a nonempty list of
        // command-line arguments such that it, and all repetitions of it match the
        // sub-pattern represented by this result.
        //
        //                                                                                                                                                                                                                                      
        // DESCRIPTION ABOVE AND NAME BELOW IS INCORRECT ---> Use this instead:
        //   Name: `has_repeating_branch`
        //   For element: Has repeating branch if it is a repetition construct, if it is an optionality construct whose sub-pattern has a repeating branch, or of it is an alternatives construct with a branch that has a repeating branch.
        //   For a sequence when adding an new element: New sequence has repeating branch if old sequence has repeating branch and new element is nullable, or if old sequence is nullable and new element has repeating branch.
        //   For alternatives construct when adding new branch: New alternatives construct has repeating branch if old alternatives construct has repeating branch, or if new branch has repeating branch.
        //                                                                                                                                                                                                                                      
        //
        bool has_repeatable_match;
    };
    Result result;

    using Elem = PatternStructure::Elem;
    core::Vector<Elem, 16> elems;

    using Seq = PatternStructure::Seq;
    core::Vector<Seq, 8> branches;

    core::Vector<std::size_t, 4> branches_offsets;

    struct Frame {
        const Expr* node;
        std::size_t elems_offset;
        std::optional<Result> left_result;
    };
    std::stack<Frame, core::Vector<Frame, 7>> stack;

    const Expr* node = expr;
    std::size_t next_pos = 0;

    auto count_params = [&](std::size_t elems_offset) noexcept {
        std::size_t n = 0;
        std::size_t num_elems = std::size_t(elems.size() - elems_offset);
        for (std::size_t i = 0; i < num_elems; ++i) {
            const Elem& elem = elems[elems_offset + i];
            if (elem.is_param)
                ++n;
        }
        return n;
    };

    auto find_nullable_branch = [&](std::size_t branches_offset) noexcept {
        std::size_t num_branches = std::size_t(branches.size() - branches_offset);
        for (std::size_t i = 0; i < num_branches; ++i) {
            const Seq& seq = branches[branches_offset + i];
            if (seq.nullable)
                return i;
        }
        return num_branches;
    };

    auto has_branch_with_params = [&](std::size_t branches_offset) noexcept {
        std::size_t num_branches = std::size_t(branches.size() - branches_offset);
        for (std::size_t i = 0; i < num_branches; ++i) {
            const Seq& seq = branches[branches_offset + i];
            if (seq.num_params > 0)
                return true;
        }
        return false;
    };

    auto register_seq = [&](std::size_t elems_offset, std::size_t num_params, bool nullable) {
        std::size_t num_elems = std::size_t(elems.size() - elems_offset);
        std::size_t elems_offset_2 = pattern_structure.elems.size();
        pattern_structure.elems.insert(pattern_structure.elems.end(),
                                       elems.begin() + elems_offset, elems.end()); // Throws
        elems.resize(elems_offset);
        std::size_t seq_index = pattern_structure.seqs.size();
        pattern_structure.seqs.push_back({ num_elems, elems_offset_2, num_params, next_pos, nullable }); // Throws
        return seq_index;
    };

    auto add_branch = [&](std::size_t elems_offset, std::size_t num_params, bool nullable) {
        std::size_t num_elems = std::size_t(elems.size() - elems_offset);
        std::size_t elems_offset_2 = pattern_structure.elems.size();
        pattern_structure.elems.insert(pattern_structure.elems.end(),
                                       elems.begin() + elems_offset, elems.end()); // Throws
        elems.resize(elems_offset);
        branches.push_back({ num_elems, elems_offset_2, num_params, next_pos, nullable }); // Throws
    };

    auto register_alt = [&](std::size_t branches_offset, std::size_t nullable_seq_index) {
        std::size_t num_seqs = std::size_t(branches.size() - branches_offset);
        std::size_t seqs_offset = pattern_structure.seqs.size();
        pattern_structure.seqs.insert(pattern_structure.seqs.end(),
                                      branches.begin() + branches_offset, branches.end()); // Throws
        branches.resize(branches_offset);
        std::size_t alt_index = pattern_structure.alts.size();
        pattern_structure.alts.push_back({ num_seqs, seqs_offset, nullable_seq_index }); // Throws
        return alt_index;
    };

    if (node)
        goto enter;
    result.nullable = true;
    goto finish;

  enter:
    if (const Sym* sym = dynamic_cast<const Sym*>(node)) {
        logger.info("Process Sym: %s", sym->get_lexeme());
        Symbol symbol = { Symbol::Type::option, sym->get_lexeme() };
        bool is_value_slot = false;
        switch (sym->get_type()) {
            case Symbol::Type::value:
                symbol = { Symbol::Type::value, {} };
                is_value_slot = true;
                break;
            case Symbol::Type::option:
                break;
        }
        bool is_param = is_value_slot;
        bool collapsible = false;
        std::size_t sym_index = pattern_structure.syms.size();
        pattern_structure.syms.push_back(symbol); // Throws
        core::int_add(next_pos, 1); // Throws
        elems.push_back({ Elem::Type::sym, is_param, collapsible, sym_index, next_pos }); // Throws
        result = {};
        goto leave;
    }
    if (const Opt* opt = dynamic_cast<const Opt*>(node)) {
        std::size_t elems_offset = elems.size(); // Throws
        stack.push({ node, elems_offset, {} }); // Throws
        node = &opt->get_expr();
        goto enter;
    }
    if (const Rep* rep = dynamic_cast<const Rep*>(node)) {
        std::size_t elems_offset = elems.size(); // Throws
        stack.push({ node, elems_offset, {} }); // Throws
        node = &rep->get_expr();
        goto enter;
    }
    if (const Cat* cat = dynamic_cast<const Cat*>(node)) {
        std::size_t elems_offset = 0; // The value has no meaning in this case
        stack.push({ node, elems_offset, {} }); // Throws
        node = &cat->get_left();
        goto enter;
    }
    if (const Alt* alt = dynamic_cast<const Alt*>(node)) {
        if (stack.empty() || !dynamic_cast<const Alt*>(stack.top().node))
            branches_offsets.push_back(branches.size()); // Throws
        std::size_t elems_offset = elems.size(); // Throws
        stack.push({ node, elems_offset, {} }); // Throws
        node = &alt->get_left();
        goto enter;
    }
    ARCHON_STEADY_ASSERT_UNREACHABLE();

  leave:
    if (!stack.empty()) {
        Frame top = std::move(stack.top());
        stack.pop();
        if (dynamic_cast<const Opt*>(top.node)) {
            logger.info("Process Opt");    
            // Internal pattern ambiguity if sub-pattern is already nullable. If this had
            // been allowed, then, in a case like `[[-a <foo>]]` with empty input, it would
            // not be clear whether the std::optional object associated with the outer-most
            // optionality construct should have a value.
            if (!result.nullable) {
                std::size_t num_params = count_params(top.elems_offset);
                bool is_param = true;
                bool collapsible = (num_params == 0);
                std::size_t seq_index = register_seq(top.elems_offset, num_params, result.nullable); // Throws
                elems.push_back({ Elem::Type::opt, is_param, collapsible, seq_index, next_pos }); // Throws
                result.nullable = true;
                goto leave;
            }
            throw std::runtime_error("Internal pattern ambiguity: "
                                     "Optionality construct with nullable sub-pattern");
        }
        if (dynamic_cast<const Rep*>(top.node)) {
            logger.info("Process Rep");    
            // Internal pattern ambiguity if sub-pattern is nullable. If this had been
            // allowed, then, in a case like `[-a <foo>]...` with empty input, it would not
            // be clear how many elements should be in the std::vector object associated
            // with the repetition construct.
            if (!result.nullable) {
                // Internal pattern ambiguity if there is some sequence of symbols for which
                // the sub-pattern already admits arbitrary repetition. If this had been
                // allowed, then, in a case like `((-a <foo>)...)...` with input matching
                // `-a <foo> -a <foo>`, it would not be clear whether the std::vector
                // objects associated with the outer-most and inner-most repetition
                // constructs should have one and two elements respectively, or whether it
                // should be the other way around (2 elements in the outer-most std::vector
                // object).
                if (!result.has_repeatable_match) {
                    std::size_t num_params = count_params(top.elems_offset);
                    bool is_param = true;
                    bool collapsible = (num_params == 0);
                    std::size_t seq_index = register_seq(top.elems_offset, num_params, result.nullable); // Throws
                    elems.push_back({ Elem::Type::rep, is_param, collapsible, seq_index, next_pos }); // Throws
                    result.has_repeatable_match = true;
                    goto leave;
                }
                throw std::runtime_error("Internal pattern ambiguity: "
                                         "Repetition construct with sub-pattern having repeatable "
                                         "matches");
            }
            throw std::runtime_error("Internal pattern ambiguity: "
                                     "Repetition construct with nullable sub-pattern");
        }
        if (const Cat* cat = dynamic_cast<const Cat*>(top.node)) {
            if (!top.left_result.has_value()) {
                std::size_t elems_offset = 0; // The value has no meaning in this case
                stack.push({ top.node, elems_offset, { std::move(result) } }); // Throws
                node = &cat->get_right();
                goto enter;
            }
            logger.info("Process Cat");    
            Result result_1 = std::move(*top.left_result);
            Result result_2 = std::move(result);
            result = {};
            result.nullable = (result_1.nullable && result_2.nullable);
            result.has_repeatable_match =
                ((result_1.has_repeatable_match && result_2.nullable) ||
                 (result_1.nullable && result_2.has_repeatable_match));
            goto leave;
        }
        if (const Alt* alt = dynamic_cast<const Alt*>(top.node)) {
            if (!top.left_result.has_value()) {
                if (!dynamic_cast<const Alt*>(&alt->get_left())) {
                    std::size_t num_params = count_params(top.elems_offset);
                    add_branch(top.elems_offset, num_params, result.nullable); // Throws
                }
                std::size_t elems_offset = elems.size(); // Throws
                stack.push({ top.node, elems_offset, { std::move(result) } }); // Throws
                node = &alt->get_right();
                goto enter;
            }
            if (!dynamic_cast<const Alt*>(&alt->get_right())) {
                std::size_t num_params = count_params(top.elems_offset);
                add_branch(top.elems_offset, num_params, result.nullable); // Throws
            }
            logger.info("Process Alt");    
            Result result_1 = std::move(*top.left_result);
            Result result_2 = std::move(result);
            // Alternatives constructs must not have nullable branches. This avoids a case
            // like `<foo> | [<bar>]` where it would be difficult to figure out that the
            // variant object should be instantiated with its second alternative when there
            // is no input. This would be difficult because of the nature of how the command
            // line is parsed.
            //
            // This rule also avoids a particular kind of internal pattern
            // ambiguity. Without this rule, in a case like `[-a <foo>] | [-b <bar>]` with
            // empty input, it would not be clear which alternative should be expressed in
            // the associated std::variant object.
            //
            // FIXME: Update text above to match reality (up to one branch is now allowed to be nullable)                                                            
            //
            if (!result_1.nullable || !result_2.nullable) {
                if (stack.empty() || !dynamic_cast<const Alt*>(stack.top().node)) {
                    bool is_param = true;
                    ARCHON_ASSERT(!branches_offsets.empty());
                    std::size_t branches_offset = branches_offsets.back();
                    branches_offsets.pop_back();
                    bool collapsible = !has_branch_with_params(branches_offset);
                    std::size_t nullable_seq_index = find_nullable_branch(branches_offset);
                    std::size_t alt_index = register_alt(branches_offset, nullable_seq_index); // Throws
                    elems.push_back({ Elem::Type::alt, is_param, collapsible, alt_index, next_pos }); // Throws
                }
                result = {};
                result.nullable = (result_1.nullable || result_2.nullable);
                result.has_repeatable_match = (result_1.has_repeatable_match ||
                                               result_2.has_repeatable_match);
                goto leave;
            }
            throw std::runtime_error("Invalid pattern: Alternatives construct with multiple nullable branches");
        }
        ARCHON_STEADY_ASSERT_UNREACHABLE();
    }

  finish:
    std::size_t num_params = count_params(0);
    std::size_t seq_index = register_seq(0, num_params, result.nullable); // Throws
    return seq_index;
}



class Processor {
public:
    template<class F> void add_pattern(std::unique_ptr<Expr> expr, F&& func, log::Logger& logger)
    {
        PatternStructure::Snapshot snapshot = m_pattern_structure.snapshot();
        try {
            std::size_t seq_index = register_pattern_struct(expr.get(), m_pattern_structure, logger); // Throws
            logger.info("Pattern structure:");  
            m_pattern_structure.dump(seq_index, logger);    
            using func_type = core::FuncDecay<F>;
            using pattern_action_type = FuncExecPatternAction<func_type>;
            std::unique_ptr<pattern_action_type> pattern_action = std::make_unique<pattern_action_type>(std::forward<F>(func));
            pattern_action->check(seq_index, m_pattern_structure); // Throws
            m_spec.patterns.push_back({ seq_index, std::move(pattern_action) }); // Throws
        }
        catch (...) {
            m_pattern_structure.revert(snapshot);
            throw;
        }
    }

    int process(std::vector<std::string> args, log::Logger& logger)
    {
        std::size_t pattern_index = 0;
        std::vector<std::size_t> positions;
        {
            PatternMatcher pattern_matcher(m_spec, m_pattern_structure, logger);
            for (const auto& arg : args) {
                Symbol symbol = { Symbol::Type::value, "" };
                if (!arg.empty() && arg[0] == '-')
                    symbol = { Symbol::Type::option, arg };
                if (!pattern_matcher.consume(symbol))
                    throw std::runtime_error("No transition");
            }
            if (!pattern_matcher.is_match(pattern_index, positions, logger))
                throw std::runtime_error("Incomplete");
        }
        logger.info("Matched pattern: %s", core::as_dec_int(1 + pattern_index));
        logger.info("Positions: %s", core::as_sbr_list(positions, [](auto v) {
            return core::as_dec_int(v);
        }));
        ARCHON_ASSERT(pattern_index < m_spec.patterns.size());
        const Pattern& pattern = m_spec.patterns[pattern_index];
        using Desc = PatternAction::Desc;
        std::vector<Desc> descs;

        using Seq  = PatternStructure::Seq;
        using Elem = PatternStructure::Elem;
        using Alt  = PatternStructure::Alt;

        ARCHON_ASSERT(pattern.seq_index < m_pattern_structure.seqs.size());
        const Seq& root = m_pattern_structure.seqs[pattern.seq_index];

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
        std::stack<Frame, core::Vector<Frame, 7>> stack;

        std::size_t num_positions = positions.size();
        ARCHON_ASSERT(num_positions > 0 && num_positions - 1 == args.size());

        std::size_t pos_index = 0;
        std::size_t pos;

        auto pos_in_range = [&](const Elem& elem) noexcept {
            return (pos >= frame.begin_pos && pos < elem.end_pos);
        };

        auto next_elem = [&](const Elem& elem) noexcept {
            ++frame.cursor.elem_index;
            frame.begin_pos = elem.end_pos;
        };

        auto enter_subseq = [&](const Elem& elem) {
            ARCHON_ASSERT(elem.type == Elem::Type::opt || elem.type == Elem::Type::rep);
            logger.info("Enter into subseq of %s at index %s", elem.type, frame.cursor.elem_index);    
            stack.push(std::move(frame)); // Throws
            std::size_t seq_index = elem.index;
            ARCHON_ASSERT(seq_index < m_pattern_structure.seqs.size());
            const Seq& seq = m_pattern_structure.seqs[seq_index];
            frame.cursor = { &seq, 0 };
        };

        goto start;

      next_pos:
        ++pos_index;

      start:
        ARCHON_ASSERT(pos_index < num_positions);
        pos = positions[pos_index];
        logger.info("Next pos: %s", pos);    

      again:
        if (ARCHON_LIKELY(frame.cursor.elem_index < frame.cursor.seq->num_elems)) {
            std::size_t elem_index = std::size_t(frame.cursor.seq->elems_offset +
                                                 frame.cursor.elem_index);
            ARCHON_ASSERT(elem_index < m_pattern_structure.elems.size());
            const Elem& elem = m_pattern_structure.elems[elem_index];
            logger.info("Process %s", elem.type);    
            switch (elem.type) {
                case Elem::Type::sym: {
                    ARCHON_ASSERT(pos_in_range(elem));
                    std::size_t arg_index = pos_index;
                    ARCHON_ASSERT(arg_index < args.size());
                    std::size_t value = arg_index;
                    descs.push_back({ elem.type, elem.collapsible, value }); // Throws
                    next_elem(elem);
                    goto next_pos;
                }
                case Elem::Type::opt: {
                    if (ARCHON_LIKELY(!pos_in_range(elem))) {
                        std::size_t value = 0; // Absent option
                        descs.push_back({ elem.type, elem.collapsible, value }); // Throws
                        next_elem(elem);
                        goto again;
                    }
                    std::size_t value = 1; // Present option
                    descs.push_back({ elem.type, elem.collapsible, value }); // Throws
                    enter_subseq(elem); // Throws
                    goto again;
                }
                case Elem::Type::rep: {
                    ARCHON_ASSERT(pos_in_range(elem));
                    frame.desc_index = descs.size();
                    std::size_t value = 1; // First occurrence in repetition
                    descs.push_back({ elem.type, elem.collapsible, value }); // Throws
                    enter_subseq(elem); // Throws
                    goto again;
                }
                case Elem::Type::alt: {
                    std::size_t branch_index = 0;
                    std::size_t alt_index = elem.index;
                    ARCHON_ASSERT(alt_index < m_pattern_structure.alts.size());
                    const Alt& alt = m_pattern_structure.alts[alt_index];
                    if (ARCHON_LIKELY(!pos_in_range(elem))) {
                        ARCHON_ASSERT(alt.nullable_seq_index < alt.num_seqs);
                        branch_index = alt.nullable_seq_index;
                        goto alt_finish;
                    }
                    {
                        std::size_t n = alt.num_seqs;
                        for (std::size_t i = 0; i < n; ++i) {
                            std::size_t seq_index = std::size_t(alt.seqs_offset + i);
                            ARCHON_ASSERT(seq_index < m_pattern_structure.seqs.size());
                            const Seq& seq = m_pattern_structure.seqs[seq_index];
                            if (ARCHON_LIKELY(pos >= seq.end_pos))
                                continue;
                            branch_index = i;
                            goto alt_finish;
                        }
                    }
                    ARCHON_ASSERT_UNREACHABLE();
                  alt_finish:
                    logger.info("Enter into branch %s of %s at index %s", branch_index, elem.type, frame.cursor.elem_index);    
                    std::size_t value = branch_index;
                    descs.push_back({ elem.type, elem.collapsible, value }); // Throws
                    stack.push(std::move(frame)); // Throws
                    ARCHON_ASSERT(branch_index < alt.num_seqs);
                    std::size_t seq_index = std::size_t(alt.seqs_offset + branch_index);
                    ARCHON_ASSERT(seq_index < m_pattern_structure.seqs.size());
                    const Seq& seq = m_pattern_structure.seqs[seq_index];
                    frame.cursor = { &seq, 0 };
                    goto again;
                }
            }
            ARCHON_ASSERT_UNREACHABLE();
        }

        if (ARCHON_LIKELY(!stack.empty())) {
            frame = std::move(stack.top());
            stack.pop();
            std::size_t elem_index = std::size_t(frame.cursor.seq->elems_offset +
                                                 frame.cursor.elem_index);
            ARCHON_ASSERT(elem_index < m_pattern_structure.elems.size());
            const Elem& elem = m_pattern_structure.elems[elem_index];
            if (elem.type != Elem::Type::alt) {
                logger.info("Exit out of subseq of %s at index %s", elem.type, frame.cursor.elem_index);    
            }
            else {
                logger.info("Exit out of branch of %s at index %s", elem.type, frame.cursor.elem_index);    
            }
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

        ++pos_index;
        ARCHON_ASSERT(pos_index == num_positions);

        for (Desc desc : descs) {
            logger.info("Desc: (type=%s, collapsible=%s, value=%s)", desc.type, desc.collapsible,
                        desc.value);    
        }

        return pattern.action->invoke(descs, args); // Throws
    }

private:
    using Pattern = Spec::Pattern;

    PatternStructure m_pattern_structure;
    Spec m_spec;
};


} // unnamed namespace


ARCHON_TEST(Cli_Foo)
{
    Processor proc;

/*
    proc.add_pattern(std::make_unique<Cat>(std::make_unique<Sym>("<a>"), std::make_unique<Sym>("<b>")), [&](int a, int b) {
        log("CLICK (%s, %s)", a, b);
        return EXIT_SUCCESS;
    }, test_context.logger);
*/

/*
    proc.add_pattern(std::make_unique<Cat>(std::make_unique<Rep>(std::make_unique<Sym>("<a>")), std::make_unique<Rep>(std::make_unique<Sym>("<b>"))), [&](std::vector<int>, std::vector<int>) {
        log("CLICK");
        return EXIT_SUCCESS;
    }, test_context.logger);
*/

/*
    proc.add_pattern(std::make_unique<Alt>(std::make_unique<Sym>("<a>"), std::make_unique<Sym>("<b>")), [&](std::variant<int, float>) {
        log("CLICK");
        return EXIT_SUCCESS;
    }, test_context.logger);
*/

/*
    proc.add_pattern(std::make_unique<Alt>(std::make_unique<Sym>("-a"), std::make_unique<Sym>("-a")), [&]() {
        log("CLICK");
        return EXIT_SUCCESS;
    }, test_context.logger);
*/

/*
    proc.add_pattern(std::make_unique<Cat>(std::make_unique<Sym>("-a"), std::make_unique<Sym>("<a>")), [&](int i) {
        log("A<%s>", i);
        return EXIT_SUCCESS;
    }, test_context.logger);
    proc.add_pattern(std::make_unique<Cat>(std::make_unique<Sym>("-b"), std::make_unique<Sym>("<b>")), [&](int i) {
        log("B<%s>", i);
        return EXIT_SUCCESS;
    }, test_context.logger);
*/

/*
    proc.add_pattern(std::make_unique<Alt>(std::make_unique<Cat>(std::make_unique<Sym>("<a1>"), std::make_unique<Cat>(std::make_unique<Sym>("-a"), std::make_unique<Sym>("<a2>"))),
                                           std::make_unique<Cat>(std::make_unique<Sym>("<b1>"), std::make_unique<Cat>(std::make_unique<Sym>("-b"), std::make_unique<Sym>("<b2>")))), [&](std::variant<std::pair<int, int>, std::pair<int, int>>) {
        log("CLICK");
        return EXIT_SUCCESS;
    }, test_context.logger);
*/

/*
    proc.add_pattern(std::make_unique<Cat>(std::make_unique<Sym>("<a>"), std::make_unique<Rep>(std::make_unique<Sym>("<b>"))), [&](int, std::vector<int>) {
        log("CLICK");
        return EXIT_SUCCESS;
    }, test_context.logger);
*/

/*
    proc.add_pattern(std::make_unique<Rep>(std::make_unique<Opt>(std::make_unique<Sym>("<a>"))), [&]() {
        log("CLICK");
        return EXIT_SUCCESS;
    }, test_context.logger);
*/

/*
    proc.add_pattern(std::make_unique<Rep>(std::make_unique<Sym>("<a>")), [&](std::vector<int> vec) {
        log("CLICK %s", core::as_sbr_list(vec));
        return EXIT_SUCCESS;
    }, test_context.logger);
*/

/*
    proc.add_pattern(std::make_unique<Opt>(std::make_unique<Sym>("<b>")), [&](std::optional<int>) {
        log("CLICK");
        return EXIT_SUCCESS;
    }, test_context.logger);
*/

/*
    proc.add_pattern(std::make_unique<Sym>("<a>"), [&](int) {
        log("CLICK");
        return EXIT_SUCCESS;
    }, test_context.logger);
*/

/*
    proc.add_pattern(std::make_unique<Cat>(std::make_unique<Opt>(std::make_unique<Sym>("-a")),
                                           std::make_unique<Opt>(std::make_unique<Sym>("-b"))), [&]() {
        log("CLICK");
        return EXIT_SUCCESS;
    }, test_context.logger);
*/

/*
    proc.add_pattern(std::make_unique<Rep>(std::make_unique<Opt>(std::make_unique<Sym>("<a>"))), [&]() {
        log("CLICK");
        return EXIT_SUCCESS;
    }, test_context.logger);
*/

/*
    // Test case (collapsed param): (<foo> [-x])...
    proc.add_pattern(std::make_unique<Rep>(std::make_unique<Cat>(std::make_unique<Sym>("<foo>"), std::make_unique<Opt>(std::make_unique<Sym>("-x")))), [&](std::vector<std::pair<int, bool>>) {
        log("CLICK");
        return EXIT_SUCCESS;
    }, test_context.logger);
*/

/*
    // Test case (collapsed param): (<foo> -x...)...
    proc.add_pattern(std::make_unique<Rep>(std::make_unique<Cat>(std::make_unique<Sym>("<foo>"), std::make_unique<Rep>(std::make_unique<Sym>("-x")))), [&](std::vector<std::pair<int, std::size_t>>) {
        log("CLICK");
        return EXIT_SUCCESS;
    }, test_context.logger);
*/

/*
    // Test case (collapsed param): (<foo> [-a...])...
    proc.add_pattern(std::make_unique<Rep>(std::make_unique<Cat>(std::make_unique<Sym>("<foo>"), std::make_unique<Opt>(std::make_unique<Rep>(std::make_unique<Sym>("-x"))))), [&](std::vector<std::pair<int, std::size_t>>) {
        log("CLICK");
        return EXIT_SUCCESS;
    }, test_context.logger);
*/

/*
    // Test case (collapsed param): (<foo> (-a | -b | -c))...
    proc.add_pattern(std::make_unique<Rep>(std::make_unique<Cat>(std::make_unique<Sym>("<foo>"), std::make_unique<Alt>(std::make_unique<Alt>(std::make_unique<Sym>("-a"), std::make_unique<Sym>("-b")), std::make_unique<Sym>("-c")))), [&](std::vector<std::pair<int, std::size_t>>) {
        log("CLICK");
        return EXIT_SUCCESS;
    }, test_context.logger);
*/

/*
    // Test case (collapsed param): (<foo> [-a | -b | -c])...
    proc.add_pattern(std::make_unique<Rep>(std::make_unique<Cat>(std::make_unique<Sym>("<foo>"), std::make_unique<Opt>(std::make_unique<Alt>(std::make_unique<Alt>(std::make_unique<Sym>("-a"), std::make_unique<Sym>("-b")), std::make_unique<Sym>("-c"))))), [&](std::vector<std::pair<int, std::optional<std::size_t>>>) {
        log("CLICK");
        return EXIT_SUCCESS;
    }, test_context.logger);
*/

/*
    // Test case (collapsed param): (<foo> (-a | -b | -c)...)...
    proc.add_pattern(std::make_unique<Rep>(std::make_unique<Cat>(std::make_unique<Sym>("<foo>"), std::make_unique<Rep>(std::make_unique<Alt>(std::make_unique<Alt>(std::make_unique<Sym>("-a"), std::make_unique<Sym>("-b")), std::make_unique<Sym>("-c"))))), [&](std::vector<std::pair<int, std::vector<std::size_t>>>) {
        log("CLICK");
        return EXIT_SUCCESS;
    }, test_context.logger);
*/

/*
    // Test case: ([<a>] -x [<b>])...
    proc.add_pattern(std::make_unique<Rep>(std::make_unique<Cat>(std::make_unique<Cat>(std::make_unique<Opt>(std::make_unique<Sym>("<a>")), std::make_unique<Sym>("-x")), std::make_unique<Opt>(std::make_unique<Sym>("<b>")))), [&](std::vector<std::pair<std::optional<int>, std::optional<int>>>) {
        log("CLICK");
        return EXIT_SUCCESS;
    }, test_context.logger);
*/

    // Test case: -a | [-b]
    {
        std::size_t n = 0;
        Processor proc;
        proc.add_pattern(std::make_unique<Alt>(std::make_unique<Sym>("-a"), std::make_unique<Opt>(std::make_unique<Sym>("-b"))), [&](std::variant<std::monostate, bool> var) {
            ++n;
            const bool* ptr = std::get_if<1>(&var);
            ARCHON_CHECK(ptr && !*ptr);
            return EXIT_SUCCESS;
        }, test_context.logger);
        int exit_status = proc.process({}, test_context.logger);
        ARCHON_CHECK_EQUAL(exit_status, EXIT_SUCCESS);
        ARCHON_CHECK_EQUAL(n, 1);
    }
    {
        std::size_t n = 0;
        Processor proc;
        proc.add_pattern(std::make_unique<Alt>(std::make_unique<Sym>("-a"), std::make_unique<Opt>(std::make_unique<Sym>("-b"))), [&](std::variant<std::monostate, bool> var) {
            ++n;
            const std::monostate* ptr = std::get_if<0>(&var);
            ARCHON_CHECK(ptr);
            return EXIT_SUCCESS;
        }, test_context.logger);
        int exit_status = proc.process({ "-a" }, test_context.logger);
        ARCHON_CHECK_EQUAL(exit_status, EXIT_SUCCESS);
        ARCHON_CHECK_EQUAL(n, 1);
    }
    {
        std::size_t n = 0;
        Processor proc;
        proc.add_pattern(std::make_unique<Alt>(std::make_unique<Sym>("-a"), std::make_unique<Opt>(std::make_unique<Sym>("-b"))), [&](std::variant<std::monostate, bool> var) {
            ++n;
            const bool* ptr = std::get_if<1>(&var);
            ARCHON_CHECK(ptr && *ptr);
            return EXIT_SUCCESS;
        }, test_context.logger);
        int exit_status = proc.process({ "-b" }, test_context.logger);
        ARCHON_CHECK_EQUAL(exit_status, EXIT_SUCCESS);
        ARCHON_CHECK_EQUAL(n, 1);
    }

    // Test case: -x...
    {
        std::size_t n = 0;
        Processor proc;
        proc.add_pattern(std::make_unique<Rep>(std::make_unique<Sym>("-x")), [&](std::size_t m) {
            ++n;
            ARCHON_CHECK_EQUAL(m, 3);
            return EXIT_SUCCESS;
        }, test_context.logger);
        int exit_status = proc.process({ "-x", "-x", "-x" }, test_context.logger);
        ARCHON_CHECK_EQUAL(exit_status, EXIT_SUCCESS);
        ARCHON_CHECK_EQUAL(n, 1);
    }
    {
        std::size_t n = 0;
        Processor proc;
        proc.add_pattern(std::make_unique<Rep>(std::make_unique<Sym>("-x")), [&](std::vector<std::monostate> v) {
            ++n;
            ARCHON_CHECK_EQUAL(v.size(), 3);
            return EXIT_SUCCESS;
        }, test_context.logger);
        int exit_status = proc.process({ "-x", "-x", "-x" }, test_context.logger);
        ARCHON_CHECK_EQUAL(exit_status, EXIT_SUCCESS);
        ARCHON_CHECK_EQUAL(n, 1);
    }
}
