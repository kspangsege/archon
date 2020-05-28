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

#ifndef ARCHON_X_CORE_X_VALUE_RESET_GUARD_HPP
#define ARCHON_X_CORE_X_VALUE_RESET_GUARD_HPP

/// \file


#include <type_traits>
#include <utility>


namespace archon::core {


namespace impl {
template<class T, class U> class ValueResetGuard;
} // namespace impl



/// \brief A variable resetting scope guard.
///
/// Construct and return an object that, when destroyed, will reset the specified variable
/// to its default value, i.e., `T()`. This is a shorthand for
/// `core::make_value_reset_guard(var, T())`.
///
/// \sa \ref core::make_value_reset_guard(T&, U)
///
template<class T> [[nodiscard]] auto make_value_reset_guard(T& var) noexcept -> impl::ValueResetGuard<T, T>;



/// \brief A variable resetting scope guard.
///
/// Construct and return an object that, when destroyed, will reset the specified variable
/// to the specified value. This can be used to ensure that a variable is reset before
/// execution exits from the current scope, even when exceptions are thrown.
///
/// Here is an example of how it is intended to be used:
///
/// \code{.cpp}
///
///   void Foo::func()
///   {
///       auto guard = archon::core::make_value_reset_guard(m_redirect, 2);
///       m_redirect = 7;
///       // Stuff that may throw ...
///   }
///
/// \endcode
///
/// \sa \ref core::make_value_reset_guard(T&)
/// \sa \ref core::make_temp_assign()
///
template<class T, class U> [[nodiscard]] auto make_value_reset_guard(T& var, U val) noexcept ->
    impl::ValueResetGuard<T, U>;



/// \brief A variable resetting scope guard.
///
/// First, set the specified variable to \p val_1. Then, construct and return an object
/// that, when destroyed, will set the variable to \p val_2. This is a shorthand for `(var =
/// std::move(val_1), make_value_reset_guard(var, std::move(val_2)))`.
///
/// \sa \ref core::make_value_reset_guard(T&, U)
///
template<class T, class U> [[nodiscard]] auto make_temp_assign(T& var, U val_1, U val_2 = {}) noexcept ->
    impl::ValueResetGuard<T, U>;








// Implementation


namespace impl {


template<class T, class U> class ValueResetGuard {
public:
    static_assert(std::is_nothrow_move_constructible_v<U>);
    static_assert(std::is_nothrow_assignable_v<T&, U&&>);
    static_assert(std::is_nothrow_destructible_v<U>);

    ValueResetGuard(T& var, U val) noexcept;
    ValueResetGuard(ValueResetGuard&&) noexcept;
    ~ValueResetGuard() noexcept;

private:
    T* m_var;
    U m_val;
};


template<class T, class U>
inline ValueResetGuard<T, U>::ValueResetGuard(T& var, U val) noexcept
    : m_var(&var)
    , m_val(std::move(val))
{
}


template<class T, class U>
inline ValueResetGuard<T, U>::ValueResetGuard(ValueResetGuard&& other) noexcept
    : m_var(other.m_var)
    , m_val(std::move(other.m_val))
{
    other.m_var = nullptr;
}


template<class T, class U>
inline ValueResetGuard<T, U>::~ValueResetGuard() noexcept
{
    if (m_var)
        *m_var = std::move(m_val);
}


} // namespace impl


template<class T> inline auto make_value_reset_guard(T& var) noexcept -> impl::ValueResetGuard<T, T>
{
    static_assert(std::is_nothrow_default_constructible_v<T>);
    return impl::ValueResetGuard<T, T>(var, T());
}


template<class T, class U> inline auto make_value_reset_guard(T& var, U val) noexcept -> impl::ValueResetGuard<T, U>
{
    return impl::ValueResetGuard<T, U>(var, std::move(val));
}


template<class T, class U>
inline auto make_temp_assign(T& var, U val_1, U val_2) noexcept -> impl::ValueResetGuard<T, U>
{
    var = std::move(val_1);
    return make_value_reset_guard(var, std::move(val_2));
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_VALUE_RESET_GUARD_HPP
