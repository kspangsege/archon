// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2022 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_MATH_X_VEC_VAL_HPP
#define ARCHON_X_MATH_X_VEC_VAL_HPP

/// \file


#include <cstddef>
#include <utility>
#include <array>

#include <archon/math/type_traits.hpp>
#include <archon/math/vec_fwd.hpp>


namespace archon::math {


template<std::size_t N, class T, class R, class A>
class VecVal {
public:
    using comp_type = T;
    using rep_type  = R;

    static_assert(math::is_valid_scalar<comp_type>);

    static constexpr std::size_t tensor_order = 1;

    static constexpr std::size_t size = N;

    VecVal(const VecVal&) = delete;

    constexpr auto operator[](std::size_t)       ->       T&;
    constexpr auto operator[](std::size_t) const -> const T&;

    template<class B> constexpr auto operator=(const B& x) -> A&;
    constexpr auto operator=(const VecVal&) -> VecVal&;

    template<class B> constexpr auto operator+=(const B& x) -> A&;
    template<class B> constexpr auto operator-=(const B& x) -> A&;
    template<class B> constexpr auto operator*=(const B& x) -> A&;
    template<class B> constexpr auto operator/=(const B& x) -> A&;

    template<class U, class S, class B> constexpr bool operator==(const math::VecVal<N, U, S, B>&) const;
    template<class U, class S, class B> constexpr bool operator!=(const math::VecVal<N, U, S, B>&) const;
    template<class U, class S, class B> constexpr bool operator< (const math::VecVal<N, U, S, B>&) const;
    template<class U, class S, class B> constexpr bool operator<=(const math::VecVal<N, U, S, B>&) const;
    template<class U, class S, class B> constexpr bool operator> (const math::VecVal<N, U, S, B>&) const;
    template<class U, class S, class B> constexpr bool operator>=(const math::VecVal<N, U, S, B>&) const;

    constexpr auto comp(std::size_t)       ->       T&;
    constexpr auto comp(std::size_t) const -> const T&;

protected:
    rep_type m_rep;

    template<class... P> constexpr VecVal(P&&... params);

    constexpr auto set(const math::Vec<N, T>&) -> A&;

    constexpr auto self() noexcept -> A&;
};








// Implementation


template<std::size_t N, class T, class R, class A>
constexpr auto VecVal<N, T, R, A>::operator[](std::size_t i) -> T&
{
    return comp(i); // Throws
}


template<std::size_t N, class T, class R, class A>
constexpr auto VecVal<N, T, R, A>::operator[](std::size_t i) const -> const T&
{
    return comp(i); // Throws
}


template<std::size_t N, class T, class R, class A>
template<class B> constexpr auto VecVal<N, T, R, A>::operator=(const B& x) -> A&
{
    return set(math::Vec<N, T>(x)); // Throws
}


template<std::size_t N, class T, class R, class A>
constexpr auto VecVal<N, T, R, A>::operator=(const VecVal& x) -> VecVal&
{
    return set(math::Vec<N, T>(x)); // Throws
}


template<std::size_t N, class T, class R, class A>
template<class B> constexpr auto VecVal<N, T, R, A>::operator+=(const B& x) -> A&
{
    return set(*this + x); // Throws
}


template<std::size_t N, class T, class R, class A>
template<class B> constexpr auto VecVal<N, T, R, A>::operator-=(const B& x) -> A&
{
    return set(*this - x); // Throws
}


template<std::size_t N, class T, class R, class A>
template<class B> constexpr auto VecVal<N, T, R, A>::operator*=(const B& x) -> A&
{
    return set(*this * x); // Throws
}


template<std::size_t N, class T, class R, class A>
template<class B> constexpr auto VecVal<N, T, R, A>::operator/=(const B& x) -> A&
{
    return set(*this / x); // Throws
}


template<std::size_t N, class T, class R, class A>
template<class U, class S, class B>
constexpr bool VecVal<N, T, R, A>::operator==(const math::VecVal<N, U, S, B>& x) const
{
    static_assert(math::is_valid_scalar_pair<T, U>);
    const auto& a = *this;
    const auto& b = x;
    for (std::size_t i = 0; i < N; ++i) {
        if (!(a[i] == b[i])) // Throws
            return false;
    }
    return true;
}


template<std::size_t N, class T, class R, class A>
template<class U, class S, class B>
constexpr bool VecVal<N, T, R, A>::operator!=(const math::VecVal<N, U, S, B>& x) const
{
    const auto& a = *this;
    const auto& b = x;
    return !(a == b); // Throws
}


template<std::size_t N, class T, class R, class A>
template<class U, class S, class B>
constexpr bool VecVal<N, T, R, A>::operator<(const math::VecVal<N, U, S, B>& x) const
{
    static_assert(math::is_valid_scalar_pair<T, U>);
    const auto& a = *this;
    const auto& b = x;
    for (std::size_t i = 0; i < N; ++i) {
        if (a[i] < b[i]) // Throws
            return true;
        if (b[i] < a[i]) // Throws
            return false;
    }
    return false;
}


template<std::size_t N, class T, class R, class A>
template<class U, class S, class B>
constexpr bool VecVal<N, T, R, A>::operator<=(const math::VecVal<N, U, S, B>& x) const
{
    const auto& a = *this;
    const auto& b = x;
    return !(b < a); // Throws
}


template<std::size_t N, class T, class R, class A>
template<class U, class S, class B>
constexpr bool VecVal<N, T, R, A>::operator>(const math::VecVal<N, U, S, B>& x) const
{
    const auto& a = *this;
    const auto& b = x;
    return b < a; // Throws
}


template<std::size_t N, class T, class R, class A>
template<class U, class S, class B>
constexpr bool VecVal<N, T, R, A>::operator>=(const math::VecVal<N, U, S, B>& x) const
{
    const auto& a = *this;
    const auto& b = x;
    return !(a < b); // Throws
}


template<std::size_t N, class T, class R, class A>
constexpr auto VecVal<N, T, R, A>::comp(std::size_t i) -> T&
{
    return m_rep.comp(i); // Throws
}


template<std::size_t N, class T, class R, class A>
constexpr auto VecVal<N, T, R, A>::comp(std::size_t i) const -> const T&
{
    return m_rep.comp(i); // Throws
}


template<std::size_t N, class T, class R, class A>
template<class... P> constexpr VecVal<N, T, R, A>::VecVal(P&&... params)
    : m_rep(std::forward<P>(params)...) // Throws
{
}


template<std::size_t N, class T, class R, class A>
constexpr auto VecVal<N, T, R, A>::set(const math::Vec<N, T>& x) -> A&
{
    for (std::size_t i = 0; i < N; ++i)
        comp(i) = x[i]; // Throws
    return self();
}


template<std::size_t N, class T, class R, class A>
constexpr auto VecVal<N, T, R, A>::self() noexcept -> A&
{
    return static_cast<A&>(*this);
}


} // namespace archon::math

#endif // ARCHON_X_MATH_X_VEC_VAL_HPP
