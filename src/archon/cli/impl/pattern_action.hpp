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

#ifndef ARCHON_X_CLI_X_IMPL_X_PATTERN_ACTION_HPP
#define ARCHON_X_CLI_X_IMPL_X_PATTERN_ACTION_HPP

/// \file


#include <cstdlib>
#include <type_traits>
#include <utility>
#include <memory>
#include <functional>
#include <tuple>
#include <string_view>

#include <archon/core/features.h>
#include <archon/core/type.hpp>
#include <archon/core/assert.hpp>
#include <archon/cli/impl/value_parser.hpp>
#include <archon/cli/impl/error_accum.hpp>
#include <archon/cli/command_line.hpp>
#include <archon/cli/impl/pattern_args_parser.hpp>
#include <archon/cli/impl/pattern_func_checker.hpp>


namespace archon::cli::impl {


template<class C, class T> class PatternAction {
public:
    using char_type   = C;
    using traits_type = T;

    using string_view_type          = std::basic_string_view<C, T>;
    using pattern_func_checker_type = impl::PatternFuncChecker<C, T>;
    using value_parser_type         = impl::ValueParser<C, T>;
    using error_accum_type          = impl::ErrorAccum<C, T>;
    using pattern_args_parser_type  = impl::PatternArgsParser<C, T>;
    using command_line_type         = cli::BasicCommandLine<C, T>;

    const bool is_deleg;

    virtual ~PatternAction() noexcept = default;

    // Returns `true` if the specified pattern structure matches the pattern function
    // represented by this pattern action. Returns 'false' otherwise.
    virtual bool check(const pattern_func_checker_type&, std::size_t elem_seq_index) const noexcept;

    // Returns `true` if parsing of arguments is successful, which requires that `has_error`
    // is `false`. In this case, the pattern function represented by this pattern action is
    // invoked. Returns 'false' otherwise. When `true` is returned, `exit_status` is set to
    // the exit status returned by the invoked function, or to `EXIT_SUCCESS` if the
    // function does not returned an exit status. When `false` is returned, `exit_status` is
    // untouched.
    virtual bool invoke(const pattern_args_parser_type&, bool has_error, value_parser_type&, error_accum_type&,
                        int& exit_status) const;

    virtual int deleg(const command_line_type&) const;

protected:
    PatternAction(bool is_deleg) noexcept;
};



template<class C, class T> auto make_pattern_action(cli::NoAction) noexcept;

template<class C, class T, class... U>
std::unique_ptr<impl::PatternAction<C, T>> make_pattern_action(std::tuple<U&...> refs);

template<class C, class T, class F> std::unique_ptr<impl::PatternAction<C, T>> make_pattern_action(F&& func);








// Implementation


// ============================ PatternAction ============================


template<class C, class T>
bool PatternAction<C, T>::check(const pattern_func_checker_type&, std::size_t) const noexcept
{
    ARCHON_ASSERT_UNREACHABLE();
    return false;
}


template<class C, class T>
inline bool PatternAction<C, T>::invoke(const pattern_args_parser_type&, bool, value_parser_type&, error_accum_type&,
                                        int&) const
{
    ARCHON_ASSERT_UNREACHABLE();
    return false;
}


template<class C, class T>
inline int PatternAction<C, T>::deleg(const command_line_type&) const
{
    ARCHON_ASSERT_UNREACHABLE();
    return 0;
}


template<class C, class T>
inline PatternAction<C, T>::PatternAction(bool d) noexcept
    : is_deleg(d)
{
}



// ============================ FuncExecPatternAction ============================


template<class C, class T, class F> class FuncExecPatternAction
    : public PatternAction<C, T> {
public:
    using func_type = F;

    using string_view_type          = std::basic_string_view<C, T>;
    using pattern_func_checker_type = impl::PatternFuncChecker<C, T>;
    using value_parser_type         = impl::ValueParser<C, T>;
    using error_accum_type          = impl::ErrorAccum<C, T>;
    using pattern_args_parser_type  = impl::PatternArgsParser<C, T>;

    FuncExecPatternAction(std::function<func_type> func)
        : PatternAction<C, T>(false)
        , m_func(std::move(func))
    {
    }

    bool check(const pattern_func_checker_type& checker, std::size_t elem_seq_index) const noexcept override final
    {
        return checker.template check<func_type>(elem_seq_index);
    }

    bool invoke(const pattern_args_parser_type& pattern_args_parser, bool has_error, value_parser_type& value_parser,
                error_accum_type& error_accum, int& exit_status) const override final
    {
        using params_tuple_type = core::TupleOfDecayedFuncParams<func_type>;
        params_tuple_type args;
        if (ARCHON_LIKELY(pattern_args_parser.parse(args, value_parser, error_accum))) { // Throws
            if (ARCHON_LIKELY(!has_error)) {
                using return_type = decltype(std::apply(m_func, std::move(args)));
                if constexpr (std::is_same_v<return_type, void>) {
                    std::apply(m_func, std::move(args)); // Throws
                    exit_status = EXIT_SUCCESS;
                }
                else {
                    static_assert(std::is_same_v<return_type, int>);
                    exit_status = std::apply(m_func, std::move(args)); // Throws
                }
                return true;
            }
        }
        return false;
    }

private:
    const std::function<func_type> m_func;
};



// ============================ DelegPatternAction ============================


template<class C, class T> class DelegPatternAction
    : public PatternAction<C, T> {
public:
    using command_line_type = BasicCommandLine<C, T>;
    using func_type = int(const command_line_type&);

    DelegPatternAction(std::function<func_type> func)
        : PatternAction<C, T>(true)
        , m_func(std::move(func))
    {
    }

    int deleg(const command_line_type& command_line) const override final
    {
        return m_func(command_line); // Throws
    }

private:
    const std::function<func_type> m_func;
};



// ============================ MakePatternAction ============================


template<class C, class T, class G> struct MakePatternAction {
    template<class F> static auto make(F&& func)
    {
        return std::make_unique<FuncExecPatternAction<C, T, G>>(std::forward<F>(func)); // Throws
    }
};


template<class C, class T, class R, class D, class U>
struct MakePatternAction<C, T, R(const BasicCommandLine<D, U>&)> {
    template<class F> static auto make(F&& func)
    {
        return std::make_unique<DelegPatternAction<C, T>>(std::forward<F>(func)); // Throws
    }
};



// ============================ make_pattern_action() ============================


template<class C, class T> auto make_pattern_action(cli::NoAction) noexcept
{
    // Return null
    return std::unique_ptr<impl::PatternAction<C, T>>();
}


template<class C, class T, class... U> auto make_pattern_action(std::tuple<U&...> refs) ->
    std::unique_ptr<impl::PatternAction<C, T>>
{
    return impl::make_pattern_action<C, T>([refs](const U&... params) {
        auto refs_2 = refs;
        refs_2 = std::tie(params...);
    }); // Throws
}


template<class C, class T, class F>
std::unique_ptr<impl::PatternAction<C, T>> make_pattern_action(F&& func)
{
    using G = core::FuncDecay<F>;
    return impl::MakePatternAction<C, T, G>::make(std::forward<F>(func)); // Throws
}


} // namespace archon::cli::impl

#endif // ARCHON_X_CLI_X_IMPL_X_PATTERN_ACTION_HPP
