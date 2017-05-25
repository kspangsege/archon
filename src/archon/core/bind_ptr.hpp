/*
 * This file is part of the Archon library framework.
 *
 * Copyright (C) 2012  Kristian Spangsege <kristian.spangsege@gmail.com>
 *
 * The Archon library framework is free software: You can redistribute
 * it and/or modify it under the terms of the GNU Lesser General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * The Archon library framework is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the Archon library framework.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef ARCHON_CORE_BIND_PTR_HPP
#define ARCHON_CORE_BIND_PTR_HPP

#include <algorithm>
#include <atomic>
#include <ostream>
#include <utility>

#include <archon/core/assert.hpp>


namespace archon {
namespace core {

class BindPtrBase {
public:
    struct AdoptTag {
    };
};


/// A generic intrusive smart pointer that binds itself to the target object.
///
/// This class is agnostic towards what 'binding' means for the target object,
/// but a common use case is 'reference counting'. See RefCountBase for an
/// example of that.
template<class T> class BindPtr: public BindPtrBase {
public:
    static_assert(noexcept(std::declval<T>().bind_ptr()),   "");
    static_assert(noexcept(std::declval<T>().unbind_ptr()), "");

    constexpr BindPtr() noexcept
    {
    }

    ~BindPtr() noexcept
    {
        unbind();
    }

    explicit BindPtr(T* ptr) noexcept
    {
        bind(ptr);
    }

    template<class U> explicit BindPtr(U* ptr) noexcept
    {
        bind(ptr);
    }

    BindPtr(T* ptr, AdoptTag) noexcept
    {
        m_ptr = ptr;
    }

    template<class U> BindPtr(U* ptr, AdoptTag) noexcept
    {
        m_ptr = ptr;
    }

    // Copy construct

    BindPtr(const BindPtr& ptr) noexcept
    {
        bind(ptr.m_ptr);
    }

    template<class U> BindPtr(const BindPtr<U>& ptr) noexcept
    {
        bind(ptr.m_ptr);
    }

    // Copy assign

    BindPtr& operator=(const BindPtr& ptr) noexcept
    {
        BindPtr{ptr}.swap(*this);
        return *this;
    }

    template<class U> BindPtr& operator=(const BindPtr<U>& ptr) noexcept
    {
        BindPtr{ptr}.swap(*this);
        return *this;
    }

    // Move construct

    BindPtr(BindPtr&& ptr) noexcept:
        m_ptr{ptr.release()}
    {
    }

    template<class U> BindPtr(BindPtr<U>&& ptr) noexcept:
        m_ptr{ptr.release()}
    {
    }

    // Move assign

    BindPtr& operator=(BindPtr&& ptr) noexcept
    {
        BindPtr{std::move(ptr)}.swap(*this);
        return *this;
    }

    template<class U> BindPtr& operator=(BindPtr<U>&& ptr) noexcept
    {
        BindPtr{std::move(ptr)}.swap(*this);
        return *this;
    }

    // Comparison

    template<class U> bool operator==(const BindPtr<U>&) const noexcept;
    template<class U> bool operator!=(const BindPtr<U>&) const noexcept;
    template<class U> bool operator< (const BindPtr<U>&) const noexcept;
    template<class U> bool operator> (const BindPtr<U>&) const noexcept;
    template<class U> bool operator<=(const BindPtr<U>&) const noexcept;
    template<class U> bool operator>=(const BindPtr<U>&) const noexcept;

    // Dereference

    T& operator*() const noexcept
    {
        return *m_ptr;
    }

    T* operator->() const noexcept
    {
        return m_ptr;
    }

    explicit operator bool() const noexcept
    {
        return bool(m_ptr);
    }

    T* get() const noexcept
    {
        return m_ptr;
    }

    void reset() noexcept
    {
        BindPtr{}.swap(*this);
    }

    void reset(T* ptr) noexcept
    {
        BindPtr{ptr}.swap(*this);
    }

    template<class U> void reset(U* ptr) noexcept
    {
        BindPtr{ptr}.swap(*this);
    }

    T* release() noexcept
    {
        T* ptr = m_ptr;
        m_ptr = nullptr;
        return ptr;
    }

    void swap(BindPtr& ptr) noexcept
    {
        std::swap(m_ptr, ptr.m_ptr);
    }

    friend void swap(BindPtr& a, BindPtr& b) noexcept
    {
        a.swap(b);
    }

private:
    T* m_ptr = nullptr;

    void bind(T* ptr) noexcept
    {
        if (ptr)
            ptr->bind_ptr();
        m_ptr = ptr;
    }

    void unbind() noexcept
    {
        if (m_ptr)
            m_ptr->unbind_ptr();
    }

    template<class> friend class BindPtr;
};


template<class T, class U> bool operator==(T*, const BindPtr<U>&) noexcept;
template<class T, class U> bool operator!=(T*, const BindPtr<U>&) noexcept;
template<class T, class U> bool operator< (T*, const BindPtr<U>&) noexcept;
template<class T, class U> bool operator> (T*, const BindPtr<U>&) noexcept;
template<class T, class U> bool operator<=(T*, const BindPtr<U>&) noexcept;
template<class T, class U> bool operator>=(T*, const BindPtr<U>&) noexcept;

template<class T, class U> bool operator==(const BindPtr<T>&, U*) noexcept;
template<class T, class U> bool operator!=(const BindPtr<T>&, U*) noexcept;
template<class T, class U> bool operator< (const BindPtr<T>&, U*) noexcept;
template<class T, class U> bool operator> (const BindPtr<T>&, U*) noexcept;
template<class T, class U> bool operator<=(const BindPtr<T>&, U*) noexcept;
template<class T, class U> bool operator>=(const BindPtr<T>&, U*) noexcept;


template<class C, class T, class U>
inline std::basic_ostream<C,T>& operator<<(std::basic_ostream<C,T>& out, const BindPtr<U>& ptr)
{
    out << static_cast<const void*>(ptr.get());
    return out;
}


/// Polymorphic convenience base class for reference counting objects.
///
/// Together with bind_ptr, this class delivers simple instrusive reference
/// counting.
///
/// \sa bind_ptr
class RefCountBase {
public:
    RefCountBase() noexcept:
    {
    }
    virtual ~RefCountBase() noexcept
    {
        ARCHON_ASSERT(m_ref_count == 0);
    }

    RefCountBase(const RefCountBase&) = delete;
    RefCountBase(RefCountBase&&) = delete;

    void operator=(const RefCountBase&) = delete;
    void operator=(RefCountBase&&) = delete;

protected:
    void bind_ptr() const noexcept
    {
        ++m_ref_count;
    }
    void unbind_ptr() const noexcept
    {
        if (--m_ref_count == 0)
            delete this;
    }

private:
    mutable unsigned long m_ref_count = 0;

    template<class> friend class BindPtr;
};


/// Same as RefCountBase, but this one makes the copying and destruction of
/// counted references thread-safe.
///
/// \sa RefCountBase
/// \sa BindPtr
class AtomicRefCountBase {
public:
    AtomicRefCountBase() noexcept
    {
    }
    virtual ~AtomicRefCountBase() noexcept
    {
        ARCHON_ASSERT(m_ref_count == 0);
    }

    AtomicRefCountBase(const AtomicRefCountBase&) = delete;
    AtomicRefCountBase(AtomicRefCountBase&&) = delete;

    void operator=(const AtomicRefCountBase&) = delete;
    void operator=(AtomicRefCountBase&&) = delete;

protected:
    // With a reference counting scheme like this, there is no need for inter-thread synchronization except in the case where the last reference disappears and the object needs to be destroyed. In that case, it is important that all prior mutations of the object have taken place before the object memory is released.

    void bind_ptr() const noexcept
    {
        std::atomic_fetch_add_explicit(&m_ref_count, 1, std::memory_order_relaxed);
    }

    void unbind_ptr() const noexcept
    {
        if (std::atomic_fetch_sub_explicit(&m_ref_count, 1, std::memory_order_release) == 1 ) {
            std::atomic_thread_fence(std::memory_order_acquire);
            delete this;
        }
    }

private:
    mutable std::atomic<unsigned long> m_ref_count = 0;

    template<class> friend class BindPtr;
};




// Implementation

                                    
template <class T>
template <class U>
bool bind_ptr<T>::operator==(const bind_ptr<U>& p) const noexcept
{
    return m_ptr == p.m_ptr;
}

template <class T>
template <class U>
bool bind_ptr<T>::operator==(U* p) const noexcept
{
    return m_ptr == p;
}

template <class T>
template <class U>
bool bind_ptr<T>::operator!=(const bind_ptr<U>& p) const noexcept
{
    return m_ptr != p.m_ptr;
}

template <class T>
template <class U>
bool bind_ptr<T>::operator!=(U* p) const noexcept
{
    return m_ptr != p;
}

template <class T>
template <class U>
bool bind_ptr<T>::operator<(const bind_ptr<U>& p) const noexcept
{
    return m_ptr < p.m_ptr;
}

template <class T>
template <class U>
bool bind_ptr<T>::operator<(U* p) const noexcept
{
    return m_ptr < p;
}

template <class T>
template <class U>
bool bind_ptr<T>::operator>(const bind_ptr<U>& p) const noexcept
{
    return m_ptr > p.m_ptr;
}

template <class T>
template <class U>
bool bind_ptr<T>::operator>(U* p) const noexcept
{
    return m_ptr > p;
}

template <class T>
template <class U>
bool bind_ptr<T>::operator<=(const bind_ptr<U>& p) const noexcept
{
    return m_ptr <= p.m_ptr;
}

template <class T>
template <class U>
bool bind_ptr<T>::operator<=(U* p) const noexcept
{
    return m_ptr <= p;
}

template <class T>
template <class U>
bool bind_ptr<T>::operator>=(const bind_ptr<U>& p) const noexcept
{
    return m_ptr >= p.m_ptr;
}

template <class T>
template <class U>
bool bind_ptr<T>::operator>=(U* p) const noexcept
{
    return m_ptr >= p;
}

template <class T, class U>
bool operator==(T* a, const bind_ptr<U>& b) noexcept
{
    return b == a;
}

template <class T, class U>
bool operator!=(T* a, const bind_ptr<U>& b) noexcept
{
    return b != a;
}

template <class T, class U>
bool operator<(T* a, const bind_ptr<U>& b) noexcept
{
    return b > a;
}

template <class T, class U>
bool operator>(T* a, const bind_ptr<U>& b) noexcept
{
    return b < a;
}

template <class T, class U>
bool operator<=(T* a, const bind_ptr<U>& b) noexcept
{
    return b >= a;
}

template <class T, class U>
bool operator>=(T* a, const bind_ptr<U>& b) noexcept
{
    return b <= a;
}


} // namespace core
} // namespace archon

#endif // ARCHON_CORE_BIND_PTR_HPP
