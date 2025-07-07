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

#ifndef ARCHON_X_CORE_X_TYPED_OBJECT_REGISTRY_HPP
#define ARCHON_X_CORE_X_TYPED_OBJECT_REGISTRY_HPP

/// \file


#include <cstddef>
#include <type_traits>
#include <stdexcept>

#include <archon/core/features.h>
#include <archon/core/type_ident.hpp>
#include <archon/core/flat_map.hpp>


namespace archon::core {


/// \brief Registry of objects identified by their type.
///
/// A typed object registry allows for objects to be registered by their type. If an object
/// of type `T` has been registered using \ref register_(), a reference to that object can
/// later be retrieved using \ref get(). Registered objects must be kept alive by the
/// application.
///
/// If the specified base class (\p B) is `void`, objects of any type can by registered;
/// otherwise, only objects that inherit from the base class can be registered (including
/// the base class itself).
///
/// If the specified base class (\p B) is `const` qualified, only references to `const`
/// qualified objects can be retrieved from the registry. This constraint is enforced at
/// compile time.
///
template<class B, std::size_t N = 0> class TypedObjectRegistry {
public:
    using object_base_type = B;

    static constexpr std::size_t static_capacity = N;

    TypedObjectRegistry() noexcept = default;

    /// \brief Register object by type.
    ///
    /// This function adds the specified object reference to the registry. The reference can
    /// later be retrieved using \ref get(). If an object of the same type was previously
    /// registered, the earlier registration will be forgotten.
    ///
    template<class T> void register_(T& obj);

    /// \brief Retrieve registered object by type.
    ///
    /// This function retrieves a reference to a previously registered object of the
    /// specified type (\ref register_()). If an object is registered using
    /// `register_(obj)`, it can be retrieved only by the type of `obj`, i.e., using
    /// `get<decltype(obj)>()`. On the other hand, if an object is registered using
    /// `register_<T>(obj)`, the object will be registered under the type `T` irrespective
    /// of the actual type of `obj`, and it can then only be retrieved by `T`.
    ///
    template<class T> auto get() const noexcept -> T*;

private:
    using key_type = core::type_ident_type;

    template<class T> static bool try_get_key(key_type&) noexcept;

    core::FlatMap<key_type, object_base_type*, static_capacity> m_map;
};








// Implementation


template<class B, std::size_t N>
template<class T> void TypedObjectRegistry<B, N>::register_(T& obj)
{
    key_type key = {};
    if (ARCHON_LIKELY(try_get_key<T>(key))) {
        m_map[key] = &obj; // Throws
        return;
    }
    throw std::length_error("Registry size");
}


template<class B, std::size_t N>
template<class T> auto TypedObjectRegistry<B, N>::get() const noexcept -> T*
{
    key_type key = {};
    if (ARCHON_LIKELY(try_get_key<T>(key))) {
        auto i = m_map.find(key);
        if (i != m_map.end())
            return static_cast<T*>(i->second);
    }
    return nullptr;
}


template<class B, std::size_t N>
template<class T> inline bool TypedObjectRegistry<B, N>::try_get_key(key_type& key) noexcept
{
    return core::try_get_type_ident<std::remove_cv_t<T>>(key);
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_TYPED_OBJECT_REGISTRY_HPP
