// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2025 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_CORE_X_OPT_OWNING_PTR_HPP
#define ARCHON_X_CORE_X_OPT_OWNING_PTR_HPP

/// \file


#include <cstddef>
#include <memory>


namespace archon::core {


/// \brief Optionally owning smart pointer.
///
/// An instance of this class is a pointer to an object of the specified type (\p T). It is
/// optionally an owning pointer. When it is owning, the lifetime of the referenced object
/// is tied to the lifetime of the pointer, but the ownership can be transferred in various
/// ways.
///
/// An optionally owning pointer is owning if it is constructed from a unique pointer
/// (`std::unique_ptr`), or if a unique pointer is assigned to it. An optionally owning
/// pointer is not owning if it is constructed from a regular pointer, or if a regular
/// pointer is assigned to it.
///
/// The memory footprint of an optionally owning pointer is twice that of a regular pointer.
///
template<class T> class opt_owning_ptr {
public:
    using element_type = T;
    using pointer = T*;

    constexpr opt_owning_ptr() noexcept = default;

    opt_owning_ptr(std::nullptr_t) noexcept;
    opt_owning_ptr(T*) noexcept;
    template<class U> opt_owning_ptr(std::unique_ptr<U>&&) noexcept;

    opt_owning_ptr(opt_owning_ptr&&) noexcept = default;

    ~opt_owning_ptr() noexcept = default;

    auto operator=(std::nullptr_t) noexcept -> opt_owning_ptr&;
    auto operator=(T*) noexcept -> opt_owning_ptr&;
    template<class U> auto operator=(std::unique_ptr<U>&&) noexcept -> opt_owning_ptr&;

    auto operator=(opt_owning_ptr&&) noexcept -> opt_owning_ptr&;
    template<class U> auto operator=(opt_owning_ptr<U>&&) noexcept -> opt_owning_ptr&;

    explicit operator bool() const noexcept;

    auto operator*() const noexcept(noexcept(*std::declval<T*>())) -> T&;
    auto operator->() const noexcept -> T*;

    /// \brief Whether this is an owning pointer.
    ///
    /// This function returns `true` if, and only if this optionally owning pointer is
    /// owning.
    ///
    bool is_owning() const noexcept;

    /// \brief Get pointer to referenced object.
    ///
    /// This function returns a pointer to the referenced object.
    ///
    auto get() const noexcept -> T*;

    /// \brief Release ownership and transfer to caller.
    ///
    /// This function releases the ownership of the referenced object and transfers it to
    /// the caller.
    ///
    /// If the optionally owning pointer is a null pointer or if it is not owning (\ref
    /// is_owning), this function returns null.
    ///
    auto release() noexcept -> std::unique_ptr<T>;

private:
    T* m_ptr = nullptr;
    std::unique_ptr<T> m_owner;
};








// Implementation


template<class T> inline opt_owning_ptr<T>::opt_owning_ptr(std::nullptr_t) noexcept
{
}


template<class T> inline opt_owning_ptr<T>::opt_owning_ptr(T* ptr) noexcept
    : m_ptr(ptr)
{
}


template<class T> template<class U> inline opt_owning_ptr<T>::opt_owning_ptr(std::unique_ptr<U>&& ptr) noexcept
    : m_ptr(ptr.get())
    , m_owner(std::move(ptr))
{
}


template<class T> inline auto opt_owning_ptr<T>::operator=(std::nullptr_t) noexcept -> opt_owning_ptr&
{
    m_ptr = nullptr;
    m_owner = nullptr;
    return *this;
}


template<class T> inline auto opt_owning_ptr<T>::operator=(T* ptr) noexcept -> opt_owning_ptr&
{
    m_ptr = ptr;
    m_owner = nullptr;
    return *this;
}


template<class T>
template<class U> inline auto opt_owning_ptr<T>::operator=(std::unique_ptr<U>&& ptr) noexcept -> opt_owning_ptr&
{
    m_ptr = ptr.get();
    m_owner = std::move(ptr);
    return *this;
}


template<class T> inline auto opt_owning_ptr<T>::operator=(opt_owning_ptr&& ptr) noexcept -> opt_owning_ptr&
{
    m_ptr = ptr.m_ptr;
    m_owner = std::move(ptr.m_owner);
    return *this;
}


template<class T>
template<class U> inline auto opt_owning_ptr<T>::operator=(opt_owning_ptr<U>&& ptr) noexcept -> opt_owning_ptr&
{
    m_ptr = ptr.m_ptr;
    m_owner = std::move(ptr.m_owner);
    return *this;
}


template<class T> inline opt_owning_ptr<T>::operator bool() const noexcept
{
    return bool(m_ptr);
}


template<class T> inline auto opt_owning_ptr<T>::operator*() const noexcept(noexcept(*std::declval<T*>())) -> T&
{
    return *m_ptr;
}


template<class T> inline auto opt_owning_ptr<T>::operator->() const noexcept -> T*
{
    return m_ptr;
}


template<class T> inline bool opt_owning_ptr<T>::is_owning() const noexcept
{
    return bool(m_owner);
}


template<class T> inline auto opt_owning_ptr<T>::get() const noexcept -> T*
{
    return m_ptr;
}


template<class T> inline auto opt_owning_ptr<T>::release() noexcept -> std::unique_ptr<T>
{
    return std::move(m_owner);
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_OPT_OWNING_PTR_HPP
