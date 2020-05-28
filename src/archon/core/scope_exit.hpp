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

#ifndef ARCHON_X_CORE_X_SCOPE_EXIT_HPP
#define ARCHON_X_CORE_X_SCOPE_EXIT_HPP

/// \file


#include <type_traits>
#include <utility>
#include <optional>

#include <archon/core/features.h>


/// \brief Execute code at exit from scope.
///
/// Execute the specified code at exit from scope, even if exceptions are thrown. For
/// example:
///
/// \code{.cpp}
///
///   void func()
///   {
///       void* mem = std::malloc(256);
///       ARCHON_SCOPE_EXIT {
///           std::free(mem);
///       };
///       // ...
///   }
///
/// \endcode
///
/// \sa \ref archon::core::ScopeExit
///
#define ARCHON_SCOPE_EXIT \
    ARCHON_SCOPE_EXIT_V(ARCHON_CONCAT(_archon_scope_exit_, __LINE__))


/// \brief Execute code at exit from scope.
///
/// Same as \ref ARCHON_SCOPE_EXIT, but use a object of type
/// archon::core::ScopeExit with the specified name (\p var).
///
#define ARCHON_SCOPE_EXIT_V(var) \
    auto var = archon::core::impl::ScopeExitHelper() + [&]() noexcept


namespace archon::core {


/// \brief Call function at exit from scope.
///
/// When the ScopeExit object is destroyed, and release() has not been called, the specified
/// function is called. The specified function must be non-throwing.
///
/// \sa \ref ARCHON_SCOPE_EXIT
///
template<class F> class ScopeExit {
public:
    static_assert(noexcept(std::declval<F>()()));
    static_assert(std::is_nothrow_destructible_v<F>);

    ScopeExit(const ScopeExit&) = delete;
    auto operator=(const ScopeExit&) -> ScopeExit& = delete;

    explicit ScopeExit(const F& func) noexcept;
    explicit ScopeExit(F&& func) noexcept;
    void release() noexcept;
    ~ScopeExit() noexcept;

private:
    std::optional<F> m_func;
};








// Implementation


namespace impl {


struct ScopeExitHelper {};


template<class F>
inline auto operator+(ScopeExitHelper, F&& func) noexcept -> ScopeExit<F>
{
    return ScopeExit<F>(std::forward<F>(func));
}


} // namespace impl


template<class F>
inline ScopeExit<F>::ScopeExit(const F& func) noexcept
    : m_func(func)
{
    static_assert(std::is_nothrow_copy_constructible_v<F>);
}


template<class F>
inline ScopeExit<F>::ScopeExit(F&& func) noexcept
    : m_func(std::move(func))
{
    static_assert(std::is_nothrow_move_constructible_v<F>);
}


template<class F>
inline void ScopeExit<F>::release() noexcept
{
    m_func.reset();
}


template<class F>
inline ScopeExit<F>::~ScopeExit() noexcept
{
    if (m_func)
        (*m_func)();
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_SCOPE_EXIT_HPP
