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

#ifndef ARCHON_X_CORE_X_IMPL_X_TYPE_IDENT_IMPL_HPP
#define ARCHON_X_CORE_X_IMPL_X_TYPE_IDENT_IMPL_HPP


#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <mutex>

#include <archon/core/features.h>
#include <archon/core/integer.hpp>


namespace archon::core::impl {


#if ARCHON_HAVE_UINTPTR_T


using type_ident_type = std::uintptr_t;


struct TypeIdentHelper {};


template<class> constexpr impl::TypeIdentHelper type_ident_helper;


template<class T> inline bool try_get_type_ident(impl::type_ident_type& ident) noexcept
{
    static_assert(std::is_same_v<impl::type_ident_type, std::uintptr_t>);
    ident = std::uintptr_t(&impl::type_ident_helper<T>);
    return true; // Trivial success
}


#else // !ARCHON_HAVE_UINTPTR_T


using type_ident_type = std::size_t;


class NextTypeIdent {
public:
    auto get() noexcept -> type_ident_type
    {
        std::lock_guard guard(m_mutex);
        if (ARCHON_LIKELY(m_prev_ident < core::int_max<type_ident_type>()))
            return ++m_prev_ident; // Success
        return 0; // Failure (too many identifiers)
    }

private:
    std::mutex m_mutex;
    type_ident_type m_prev_ident = 0; // Protected by `m_mutex`
};

inline impl::NextTypeIdent next_type_ident;


template<class T> bool try_get_type_ident(impl::type_ident_type& ident) noexcept
{
    static impl::type_ident_type ident_2 = impl::next_type_ident.get();
    if (ARCHON_LIKELY(ident_2 > 0)) {
        ident = ident_2;
        return true; // Success
    }
    return false; // Failure (too many identifiers)
}


#endif // !ARCHON_HAVE_UINTPTR_T


} // namespace archon::core::impl

#endif // ARCHON_X_CORE_X_IMPL_X_TYPE_IDENT_IMPL_HPP
