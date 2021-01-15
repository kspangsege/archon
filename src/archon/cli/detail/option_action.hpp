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

#ifndef ARCHON_X_CLI_X_DETAIL_X_OPTION_ACTION_HPP
#define ARCHON_X_CLI_X_DETAIL_X_OPTION_ACTION_HPP

/// \file


#include <memory>
#include <utility>
#include <functional>
#include <string_view>
#include <ostream>

#include <archon/base/type_traits.hpp>
#include <archon/cli/option_actions.hpp>
#include <archon/cli/detail/value_parser.hpp>
#include <archon/cli/detail/call.hpp>


namespace archon::cli::detail {


template<class C, class T> class OptionAction {
public:
    using char_type         = C;
    using traits_type       = T;
    using string_view_type  = std::basic_string_view<C, T>;
    using value_parser_type = ValueParser<C, T>;
    using ostream_type      = std::basic_ostream<C, T>;

    virtual ~OptionAction() noexcept = default;

    virtual bool allow_arg() const noexcept;
    virtual void enact_without_arg();
    virtual bool enact_with_arg(string_view_type, value_parser_type&);
    virtual bool format_orig_val(ostream_type&) const;
    virtual bool format_default_arg(ostream_type&) const;
};



template<class C, class T, class U> std::unique_ptr<OptionAction<C, T>> make_option_action(Assign<U>);
template<class C, class T, class F> std::unique_ptr<OptionAction<C, T>> make_option_action(F&& func);








// Implementation


// ============================ OptionAction ============================


template<class C, class T> inline bool OptionAction<C, T>::allow_arg() const noexcept
{
    return false;
}


template<class C, class T> inline void OptionAction<C, T>::enact_without_arg()
{
}


template<class C, class T>
inline bool OptionAction<C, T>::enact_with_arg(string_view_type, value_parser_type&)
{
    return false;
}


template<class C, class T> inline bool OptionAction<C, T>::format_orig_val(ostream_type&) const
{
    return false;
}


template<class C, class T> inline bool OptionAction<C, T>::format_default_arg(ostream_type&) const
{
    return false;
}



// ============================ OptionAssignAction ============================


template<class C, class T, class U> class OptionAssignAction :
        public OptionAction<C, T> {
public:
    using string_view_type  = std::basic_string_view<C, T>;
    using value_parser_type = ValueParser<C, T>;
    using ostream_type      = std::basic_ostream<C, T>;

    explicit OptionAssignAction(Assign<U>&& assign) :
        m_assign(std::move(assign)) // Throws
    {
    }

    bool allow_arg() const noexcept override final
    {
        return true;
    }

    void enact_without_arg() override final
    {
        *m_assign.var = m_assign.default_arg; // Throws
    }

    bool enact_with_arg(string_view_type arg, value_parser_type& parser) override final
    {
        U val = U(); // Throws
        if (ARCHON_LIKELY(parser.parse(arg, val))) { // Throws
            *m_assign.var = std::move(val); // Throws
            return true;
        }
        return false;
    }

    bool format_orig_val(ostream_type& out) const override final
    {
        out << *m_assign.var; // Throws
        return true;
    }

    bool format_default_arg(ostream_type& out) const override final
    {
        out << m_assign.default_arg; // Throws
        return true;
    }

private:
    const Assign<U> m_assign;
};



// ============================ OptionExecAction ============================


template<class C, class T, class U> class OptionExecAction;


template<class C, class T> class OptionExecAction<C, T, void()> :
        public OptionAction<C, T> {
public:
    using string_view_type  = std::basic_string_view<C, T>;
    using value_parser_type = ValueParser<C, T>;
    using func_type         = std::function<void()>;

    explicit OptionExecAction(func_type func) :
        m_func(std::move(func))
    {
    }

    void enact_without_arg() override final
    {
        m_func(); // Throws
    }

    bool enact_with_arg(string_view_type, value_parser_type&) override final
    {
        return false;
    }

private:
    const func_type m_func;
};



template<class C, class T, class R, class U> class OptionExecAction<C, T, R(U)> :
        public OptionAction<C, T> {
public:
    using string_view_type  = std::basic_string_view<C, T>;
    using value_parser_type = ValueParser<C, T>;
    using func_type         = std::function<R(U)>;

    explicit OptionExecAction(func_type func) :
        m_func(std::move(func)) // Throws
    {
    }

    bool allow_arg() const noexcept override final
    {
        return true;
    }

    void enact_without_arg() override final
    {
        m_func(U()); // Throws
    }

    bool enact_with_arg(string_view_type arg, value_parser_type& parser) override final
    {
        U val = U(); // Throws
        if (ARCHON_LIKELY(parser.parse(arg, val))) // Throws
            return call(m_func, std::move(val)); // Throws
        return false;
    }

private:
    const func_type m_func;
};



// ============================ make_option_action() ============================


template<class C, class T, class U>
std::unique_ptr<OptionAction<C, T>> make_option_action(Assign<U> assign)
{
    return std::make_unique<OptionAssignAction<C, T, U>>(std::move(assign)); // Throws
}


template<class C, class T, class F>
std::unique_ptr<OptionAction<C, T>> make_option_action(F&& func)
{
    using G = base::FuncDecay<F>;
    return std::make_unique<OptionExecAction<C, T, G>>(std::forward<F>(func)); // Throws
}


} // namespace archon::cli::detail

#endif // ARCHON_X_CLI_X_DETAIL_X_OPTION_ACTION_HPP
