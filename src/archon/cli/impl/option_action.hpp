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

#ifndef ARCHON_X_CLI_X_IMPL_X_OPTION_ACTION_HPP
#define ARCHON_X_CLI_X_IMPL_X_OPTION_ACTION_HPP


#include <type_traits>
#include <memory>
#include <utility>
#include <functional>
#include <tuple>
#include <string_view>
#include <locale>
#include <ostream>

#include <archon/core/features.h>
#include <archon/core/type_traits.hpp>
#include <archon/cli/impl/call.hpp>
#include <archon/cli/impl/value_formatter.hpp>
#include <archon/cli/impl/value_parser.hpp>
#include <archon/cli/option_actions.hpp>
#include <archon/cli/spec_support.hpp>


namespace archon::cli::impl {


template<class C, class T> class OptionAction {
public:
    using char_type   = C;
    using traits_type = T;

    using string_view_type     = std::basic_string_view<C, T>;
    using ostream_type         = std::basic_ostream<C, T>;
    using value_parser_type    = impl::ValueParser<C, T>;
    using value_formatter_type = impl::ValueFormatter<C, T>;

    virtual ~OptionAction() noexcept = default;

    virtual bool allow_arg() const noexcept;
    virtual bool require_arg() const noexcept;
    virtual void invoke_without_arg() const;
    virtual bool invoke_with_arg(string_view_type, value_parser_type&) const;
    virtual bool format_orig_val(ostream_type&, value_formatter_type&, bool& has_value) const;
    virtual bool format_default_arg(ostream_type&, value_formatter_type&, bool& has_value) const;
    virtual bool format_enum_values(ostream_type&, value_formatter_type&, bool disjunctive, bool quote) const;
};



template<class C, class T> auto make_option_action(cli::NoAction) noexcept;

template<class C, class T, class U> auto make_option_action(std::tuple<U&>);

template<class C, class T, class U> auto make_option_action(impl::Assign1<U>&&);

template<class C, class T, class U, class V> auto make_option_action(impl::Assign2<U, V>&&);

template<class C, class T, class U, class D> auto make_option_action(impl::Assign3<U, D>&&);

template<class C, class T, class U, class D, class V> auto make_option_action(impl::Assign4<U, D, V>&&);

template<class C, class T, class F> auto make_option_action(impl::Exec1<F>&&);

template<class C, class T, class F, class U> auto make_option_action(impl::Exec2<F, U>&&);

template<class C, class T, class F, class D> auto make_option_action(impl::Exec3<F, D>&&);

template<class C, class T, class F, class D, class U> auto make_option_action(impl::Exec4<F, D, U>&&);

template<class C, class T, class F> auto make_option_action(F&& func);








// Implementation


// ============================ OptionAction ============================


template<class C, class T>
inline bool OptionAction<C, T>::allow_arg() const noexcept
{
    return false;
}


template<class C, class T>
inline bool OptionAction<C, T>::require_arg() const noexcept
{
    return false;
}


template<class C, class T>
inline void OptionAction<C, T>::invoke_without_arg() const
{
}


template<class C, class T>
inline bool OptionAction<C, T>::invoke_with_arg(string_view_type, value_parser_type&) const
{
    return false;
}


template<class C, class T>
inline bool OptionAction<C, T>::format_orig_val(ostream_type&, value_formatter_type&, bool&) const
{
    return false;
}


template<class C, class T>
inline bool OptionAction<C, T>::format_default_arg(ostream_type&, value_formatter_type&, bool&) const
{
    return false;
}


template<class C, class T>
inline bool OptionAction<C, T>::format_enum_values(ostream_type&, value_formatter_type&, bool, bool) const
{
    return false;
}



// ============================ OptionAssignAction ============================


template<class C, class T, class R> class OptionAssignAction final
    : public OptionAction<C, T> {
public:
    using ref_type = R;

    using string_view_type     = std::basic_string_view<C, T>;
    using ostream_type         = std::basic_ostream<C, T>;
    using value_parser_type    = impl::ValueParser<C, T>;
    using value_formatter_type = impl::ValueFormatter<C, T>;

    explicit OptionAssignAction(impl::Assign1<R>&& assign)
        : m_ref(std::forward<R>(assign.ref)) // Throws
    {
    }

    bool allow_arg() const noexcept override
    {
        return true;
    }

    bool require_arg() const noexcept override
    {
        return true;
    }

    void invoke_without_arg() const override
    {
        ARCHON_ASSERT_UNREACHABLE();
    }

    bool invoke_with_arg(string_view_type arg, value_parser_type& parser) const override
    {
        return parser.parse(arg, m_ref); // Throws
    }

    bool format_orig_val(ostream_type& out, value_formatter_type& formatter, bool& has_value) const override
    {
        has_value = formatter.format(m_ref, out); // Throws
        return true;
    }

    bool format_enum_values(ostream_type& out, value_formatter_type& formatter,
                            bool disjunctive, bool quote) const override
    {
        using type = core::RemoveOptional<std::remove_cvref_t<ref_type>>;
        return formatter.template format_enum_values<type>(out, disjunctive, quote); // Throws
    }

private:
    ref_type m_ref;
};


template<class C, class T, class V> class OptionAssignAction<C, T, V&> final
    : public OptionAction<C, T> {
public:
    using value_type = V;

    using string_view_type     = std::basic_string_view<C, T>;
    using ostream_type         = std::basic_ostream<C, T>;
    using value_parser_type    = impl::ValueParser<C, T>;
    using value_formatter_type = impl::ValueFormatter<C, T>;

    using cond_type = std::function<bool(const value_type&)>;

    template<class R> explicit OptionAssignAction(impl::Assign1<R>&& assign)
        : m_var(assign.ref) // Throws
        , m_default_arg() // Throws
    {
    }

    template<class R, class D> explicit OptionAssignAction(impl::Assign2<R, D>&& assign)
        : m_var(assign.ref) // Throws
        , m_default_arg(std::forward<D>(assign.default_arg)) // Throws
    {
    }

    template<class R, class E> explicit OptionAssignAction(impl::Assign3<R, E>&& assign)
        : m_var(assign.ref) // Throws
        , m_cond(std::forward<E>(assign.cond)) // Throws
        , m_default_arg() // Throws
    {
    }

    template<class R, class E, class D>
    explicit OptionAssignAction(impl::Assign4<R, E, D>&& assign)
        : m_var(assign.ref) // Throws
        , m_cond(std::forward<E>(assign.cond)) // Throws
        , m_default_arg(std::forward<D>(assign.default_arg)) // Throws
    {
    }

    bool allow_arg() const noexcept override
    {
        return true;
    }

    void invoke_without_arg() const override
    {
        m_var = m_default_arg; // Throws
    }

    bool invoke_with_arg(string_view_type arg, value_parser_type& parser) const override
    {
        value_type val = {}; // Throws
        if (ARCHON_LIKELY(parser.parse(arg, val))) { // Throws
            if (ARCHON_LIKELY(!m_cond || m_cond(val))) { // Throws
                m_var = std::move(val); // Throws
                return true;
            }
        }
        return false;
    }

    bool format_orig_val(ostream_type& out, value_formatter_type& formatter, bool& has_value) const override
    {
        has_value = formatter.format(m_var, out); // Throws
        return true;
    }

    bool format_default_arg(ostream_type& out, value_formatter_type& formatter, bool& has_value) const override
    {
        has_value = formatter.format(m_default_arg, out); // Throws
        return true;
    }

    bool format_enum_values(ostream_type& out, value_formatter_type& formatter,
                            bool disjunctive, bool quote) const override
    {
        using type = core::RemoveOptional<std::remove_cvref_t<value_type>>;
        return formatter.template format_enum_values<type>(out, disjunctive, quote); // Throws
    }

private:
    value_type& m_var;
    const cond_type m_cond;
    const value_type m_default_arg;
};



// ============================ OptionExecAction ============================


template<class C, class T, class U> class OptionExecAction;


template<class C, class T> class OptionExecAction<C, T, void()> final
    : public OptionAction<C, T> {
public:
    using func_type         = std::function<void()>;
    using string_view_type  = std::basic_string_view<C, T>;
    using value_parser_type = impl::ValueParser<C, T>;

    template<class F> explicit OptionExecAction(impl::Exec1<F>&& exec)
        : m_func(std::forward<F>(exec.func)) // Throws
    {
    }

    void invoke_without_arg() const override
    {
        m_func(); // Throws
    }

private:
    const func_type m_func;
};



template<class C, class T, class R, class P> class OptionExecAction<C, T, R(P)> final
    : public OptionAction<C, T> {
public:
    using string_view_type     = std::basic_string_view<C, T>;
    using ostream_type         = std::basic_ostream<C, T>;
    using value_parser_type    = impl::ValueParser<C, T>;
    using value_formatter_type = impl::ValueFormatter<C, T>;

    using value_type = std::remove_cv_t<std::remove_reference_t<P>>;
    using func_type  = std::function<R(P)>;
    using cond_type  = std::function<bool(const value_type&)>;

    template<class F> explicit OptionExecAction(impl::Exec1<F>&& exec)
        : m_func(std::forward<F>(exec.func)) // Throws
        , m_default_arg() // Throws
    {
    }

    template<class F, class D> explicit OptionExecAction(impl::Exec2<F, D>&& exec)
        : m_func(std::forward<F>(exec.func)) // Throws
        , m_default_arg(std::forward<D>(exec.default_arg)) // Throws
    {
    }

    template<class F, class E> explicit OptionExecAction(impl::Exec3<F, E>&& exec)
        : m_func(std::forward<F>(exec.func)) // Throws
        , m_cond(std::forward<E>(exec.cond)) // Throws
        , m_default_arg() // Throws
    {
    }

    template<class F, class E, class D> explicit OptionExecAction(impl::Exec4<F, E, D>&& exec)
        : m_func(std::forward<F>(exec.func)) // Throws
        , m_cond(std::forward<E>(exec.cond)) // Throws
        , m_default_arg(std::forward<D>(exec.default_arg)) // Throws
    {
    }

    bool allow_arg() const noexcept override
    {
        return true;
    }

    void invoke_without_arg() const override
    {
        m_func(m_default_arg); // Throws
    }

    bool invoke_with_arg(string_view_type arg, value_parser_type& parser) const override
    {
        value_type val = {}; // Throws
        if (ARCHON_LIKELY(parser.parse(arg, val))) { // Throws
            if (ARCHON_LIKELY(!m_cond || m_cond(val))) // Throws
                return impl::call(m_func, std::move(val)); // Throws
        }
        return false;
    }

    bool format_default_arg(ostream_type& out, value_formatter_type& formatter, bool& has_value) const override
    {
        has_value = formatter.format(m_default_arg, out); // Throws
        return true;
    }

    bool format_enum_values(ostream_type& out, value_formatter_type& formatter,
                            bool disjunctive, bool quote) const override
    {
        using type = core::RemoveOptional<std::remove_cvref_t<value_type>>;
        return formatter.template format_enum_values<type>(out, disjunctive, quote); // Throws
    }

private:
    const func_type m_func;
    const cond_type m_cond;
    const value_type m_default_arg;
};



// ============================ make_option_action() ============================


template<class C, class T> auto make_option_action(cli::NoAction) noexcept
{
    // Return null
    return std::unique_ptr<impl::OptionAction<C, T>>();
}


template<class C, class T, class U> inline auto make_option_action(std::tuple<U&> tuple)
{
    return impl::make_option_action<C, T>(cli::assign(std::get<0>(tuple))); // Throws
}


template<class C, class T, class R> inline auto make_option_action(impl::Assign1<R>&& assign)
{
    return std::make_unique<impl::OptionAssignAction<C, T, R>>(std::move(assign)); // Throws
}


template<class C, class T, class R, class D> inline auto make_option_action(impl::Assign2<R, D>&& assign)
{
    return std::make_unique<impl::OptionAssignAction<C, T, R>>(std::move(assign)); // Throws
}


template<class C, class T, class R, class E> inline auto make_option_action(impl::Assign3<R, E>&& assign)
{
    return std::make_unique<impl::OptionAssignAction<C, T, R>>(std::move(assign)); // Throws
}


template<class C, class T, class R, class E, class D> inline auto make_option_action(impl::Assign4<R, E, D>&& assign)
{
    return std::make_unique<impl::OptionAssignAction<C, T, R>>(std::move(assign)); // Throws
}


template<class C, class T, class F> inline auto make_option_action(impl::Exec1<F>&& exec)
{
    using func_type = core::FuncDecay<F>;
    return std::make_unique<impl::OptionExecAction<C, T, func_type>>(std::move(exec)); // Throws
}


template<class C, class T, class F, class D> inline auto make_option_action(impl::Exec2<F, D>&& exec)
{
    using func_type = core::FuncDecay<F>;
    return std::make_unique<impl::OptionExecAction<C, T, func_type>>(std::move(exec)); // Throws
}


template<class C, class T, class F, class E> inline auto make_option_action(impl::Exec3<F, E>&& exec)
{
    using func_type = core::FuncDecay<F>;
    return std::make_unique<impl::OptionExecAction<C, T, func_type>>(std::move(exec)); // Throws
}


template<class C, class T, class F, class E, class D> inline auto make_option_action(impl::Exec4<F, E, D>&& exec)
{
    using func_type = core::FuncDecay<F>;
    return std::make_unique<impl::OptionExecAction<C, T, func_type>>(std::move(exec)); // Throws
}


template<class C, class T, class F> inline auto make_option_action(F&& func)
{
    return make_option_action<C, T>(impl::exec(std::forward<F>(func))); // Throws
}


} // namespace archon::cli::impl

#endif // ARCHON_X_CLI_X_IMPL_X_OPTION_ACTION_HPP
