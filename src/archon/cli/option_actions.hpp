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

#ifndef ARCHON_X_CLI_X_OPTION_ACTIONS_HPP
#define ARCHON_X_CLI_X_OPTION_ACTIONS_HPP

/// \file


#include <utility>


namespace archon::cli {


/// \{
///
/// \brief Option actions.
///
/// FIXME: Explain: Arguments passed for \p default_arg, \p cond, or \p func are copied only
/// if they are a r-values. If they are l-values, and if the returned assignment / execution
/// objects are passed to \ref cli::BasicSpec::add_option(), then the caller must ensure
/// that the objects referenced by the l-values stay alive for as long as the spec remains
/// in use. By extension, if the spec is used to create a processor (\ref BasicProcessor),
/// the caller must ensure that the objects referenced by the l-values stay alive for as
/// long as the processor remains is in use.                               
///
auto raise_flag(bool& flag);
auto lower_flag(bool& flag);
template<class R> auto assign(R&& ref);
template<class R, class D> auto assign(R&& ref, D&& default_arg);
template<class R, class E> auto assign_c(R&& ref, E&& cond);
template<class R, class E, class D> auto assign_c(R& ref, E&& cond, D&& default_arg);
template<class F> auto exec(F&& func);
template<class F, class D> auto exec(F&& func, D&& default_arg);
template<class F, class E> auto exec_c(F&& func, E&& cond);
template<class F, class E, class D> auto exec_c(F&& func, E&& cond, D&& default_arg);
/// \}








// Implementation


namespace impl {


template<class R> struct Assign1 {
    R ref;
};


template<class R, class D> struct Assign2 {
    R ref;
    D default_arg;
};


template<class R, class E> struct Assign3 {
    R ref;
    E cond;
};


template<class R, class E, class D> struct Assign4 {
    R ref;
    E cond;
    D default_arg;
};

template<class R> inline auto assign(R&& ref) -> Assign1<R>
{
    return {
        std::forward<R>(ref),
    }; // Throws
}

template<class R, class D> inline auto assign(R&& ref, D&& default_arg) -> Assign2<R, D>
{
    return {
        std::forward<R>(ref),
        std::forward<D>(default_arg),
    }; // Throws
}

template<class R, class E> inline auto assign_c(R&& ref, E&& cond) -> Assign3<R, E>
{
    return {
        std::forward<R>(ref),
        std::forward<E>(cond),
    }; // Throws
}

template<class R, class E, class D> inline auto assign_c(R&& ref, E&& cond, D&& default_arg) -> Assign4<R, E, D>
{
    return {
        std::forward<R>(ref),
        std::forward<E>(cond),
        std::forward<D>(default_arg),
    }; // Throws
}


template<class F> struct Exec1 {
    F func;
};

template<class F, class D> struct Exec2 {
    F func;
    D default_arg;
};

template<class F, class E> struct Exec3 {
    F func;
    E cond;
};

template<class F, class E, class D> struct Exec4 {
    F func;
    E cond;
    D default_arg;
};

template<class F> inline auto exec(F&& func) -> Exec1<F>
{
    return {
        std::forward<F>(func),
    }; // Throws
}

template<class F, class D> inline auto exec(F&& func, D&& default_arg) -> Exec2<F, D>
{
    return {
        std::forward<F>(func),
        std::forward<D>(default_arg),
    }; // Throws
}

template<class F, class E> inline auto exec_c(F&& func, E&& cond) -> Exec3<F, E>
{
    return {
        std::forward<F>(func),
        std::forward<E>(cond),
    }; // Throws
}

template<class F, class E, class D> inline auto exec_c(F&& func, E&& cond, D&& default_arg) -> Exec4<F, E, D>
{
    return {
        std::forward<F>(func),
        std::forward<E>(cond),
        std::forward<D>(default_arg),
    }; // Throws
}


} // namespace impl



inline auto raise_flag(bool& flag)
{
    return impl::assign(flag, true);
}


inline auto lower_flag(bool& flag)
{
    return impl::assign(flag, false);
}


template<class R> inline auto assign(R&& ref)
{
    return impl::assign(std::forward<R>(ref)); // Throws
}


template<class R, class D> inline auto assign(R&& ref, D&& default_arg)
{
    return impl::assign(std::forward<R>(ref), std::forward<D>(default_arg)); // Throws
}


template<class R, class E> inline auto assign_c(R&& ref, E&& cond)
{
    return impl::assign_c(std::forward<R>(ref), std::forward<E>(cond)); // Throws
}


template<class R, class E, class D> inline auto assign_c(R&& ref, E&& cond, D&& default_arg)
{
    return impl::assign_c(std::forward<R>(ref), std::forward<E>(cond), std::forward<D>(default_arg)); // Throws
}


template<class F> inline auto exec(F&& func)
{
    return impl::exec(std::forward<F>(func)); // Throws
}


template<class F, class D> inline auto exec(F&& func, D&& default_arg)
{
    return impl::exec(std::forward<F>(func), std::forward<D>(default_arg)); // Throws
}


template<class F, class E> inline auto exec_c(F&& func, E&& cond)
{
    return impl::exec_c(std::forward<F>(func), std::forward<E>(cond)); // Throws
}


template<class F, class E, class D> inline auto exec_c(F&& func, E&& cond, D&& default_arg)
{
    return impl::exec_c(std::forward<F>(func), std::forward<E>(cond), std::forward<D>(default_arg)); // Throws
}


} // namespace archon::cli

#endif // ARCHON_X_CLI_X_OPTION_ACTIONS_HPP
