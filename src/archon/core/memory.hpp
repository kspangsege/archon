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

#ifndef ARCHON_X_CORE_X_MEMORY_HPP
#define ARCHON_X_CORE_X_MEMORY_HPP

/// \file


#include <cstddef>
#include <type_traits>
#include <limits>
#include <algorithm>
#include <memory>
#include <utility>
#include <stdexcept>

#include <archon/core/features.h>
#include <archon/core/type.hpp>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/impl/memory.hpp>


namespace archon::core {


/// \brief Suggest a new buffer size.
///
/// Suggest an appropriate new buffer size while taking the current buffer size into
/// account, and while respecting the specified minimum size.
///
/// If the current size is already greater than, or equal to the specified minimum size, the
/// current size is returned. Otherwise, the returned size is generally 50% more than the
/// current size, although at least as big as the specified minimum size, and not bigger
/// than specified maximum size.
///
/// The specified maximum size must be greater than, or equal to the greater of the current
/// size and the minimum size.
///
auto suggest_new_buffer_size(std::size_t cur_size, std::size_t min_size, std::size_t max_size = -1) noexcept ->
    std::size_t;



/// \{
///
/// \brief Functions for working with uninitialized memory.
///
/// These functions are useful for working with uninitialized memory for storage of objects
/// of non-trivial type. When \p T is a trivial type (`std::is_trivial`), these functions
/// generally change into a simpler more efficient form.
///
/// \param uninit Pointer to uninitialized storage for \p N objects of type \p T. Here, \p N
/// depends on the operation in question (see below). If `bytes` refers to an array of bytes
/// long enough and suitably aligned to store \p N objects of type \p T, then \p uninit can
/// be `reinterpret_cast<T*>(bytes)`. In general, storage must be considered uninitialized
/// if it is not already explicitly initialized to hold representations of objects of type
/// \p T.
///
/// \param args Arguments passed to constructor when constructing an object of type \p T.
///
/// \param data Pointer to one or more objects of type \p T.
///
/// \param size The number of objects of type \p T dealt with by the operation.
///
/// \param begin, end Range of objects from which to create objects of type \p T.
///
/// \param dist Distance of movement in number of objects of type \p T for moving
/// operations. This distance must be strictly greater than zero.
///
/// `uninit_create(uninit, args)` constructs one object (\p N is 1) of type `T` in the
/// uninitialized memory pointed to by \p uninit. The object is constructed from the
/// specified arguments (\p args). On success, the referenced memory is no longer
/// uninitialized.
///
/// `uninit_destroy(data, size)` destroys the \p size objects stored in the array pointed to
/// by \p data. Afterwards, the referenced memory must be considered uninitialized. This
/// function requires \p T to have a non-throwing destructor.
///
/// `uninit_safe_fill(size, uninit)` and `uninit_safe_fill(size, value, uninit)` construct
/// objects of type \p T in the uninitialized memory pointed to by \p uninit. One object is
/// constructed for each of the objects pointed to be \p data, which is \p size (\p N is \p
/// size). These operation are exception safe in the sense that if the construction of any
/// of the objects fail, then all previously constructed objects will be destroyed before
/// control is returned to the caller. For the two-argument version, the objects are
/// default-constructed. For the three-argument version, the objects are copy-constructed
/// from \p value. These functions require \p T to have a non-throwing destructor.
///
/// `uninit_safe_copy(begin, end, uninit)` constructs objects of type \p T in the
/// uninitialized memory pointed to by \p uninit. One object is constructed for each object
/// in the range \p begin to \p end (\p N is the number of objects in that range). This
/// operation is exception safe in the sense that if the construction of any of the objects
/// fail, then all previously constructed objects will be destroyed before control is
/// returned to the caller. This function requires \p T to have a non-throwing destructor.
///
/// `uninit_safe_move(data, size, uninit)` constructs objects of type \p T in the
/// uninitialized memory pointed to by \p uninit. One object is constructed for each object
/// in the array pointed to be \p data. The number of objects in that array is \p size (\p N
/// is \p size). If \p T has a non-throwing move-constructor, then the objects are
/// move-constructed from those in \p data. Otherwise they are copy-constructed. Finally,
/// this function destroys the original objects as if by `uninit_destroy(data, size)`. This
/// operation is exception safe in the sense that if it fails, it leaves the original
/// objects in place, and no objects created in the specified uninitialized memory (\p
/// uninit). Failure can only happen during copy-construction, which occurs only if \p T has
/// a throwing move-constructor. If copy-construction fails for one of the object, all
/// previously created copies are destroyed. This function requires \p T to have a
/// non-throwing destructor, and also that \p T either has a non-throwing move-constructor
/// or is copy-constructible.
///
/// `uninit_safe_move_a(data, size, uninit)` has the same effect as `uninit_safe_move(data,
/// size, uninit)` except that the original objects are left undestroyed.
///
/// `uninit_move_downwards(data, size, dist)` and `uninit_move_upwards(data, size, dist)`
/// move an array of objects of type \p T towards lower and higher memory addresses
/// respectively. In contrast to `uninit_safe_move()`, these functions allow for the
/// destination memory region to overlap with the origin region, but they can be used only
/// when \p T has a non-throwing move-constructor. Like `uninit_safe_move()`, these
/// functions destroy the original objects. These functions require \p T to have a
/// non-throwing destructor.
///
template<class T, class... A> void uninit_create(T* uninit, A&&... args);
template<class T> void uninit_destroy(T* data, std::size_t size) noexcept;
template<class T> void uninit_safe_fill(std::size_t size, T* uninit);
template<class T> void uninit_safe_fill(std::size_t size, const T& value, T* uninit);
template<class I, class T> void uninit_safe_copy(I begin, I end, T* uninit);
template<class T> void uninit_safe_move(T* data, std::size_t size, T* uninit)
    noexcept(std::is_nothrow_move_constructible_v<T>);
template<class T> void uninit_safe_move_a(T* data, std::size_t size, T* uninit)
    noexcept(std::is_nothrow_move_constructible_v<T>);
template<class T> void uninit_move_downwards(T* data, std::size_t size, std::size_t dist) noexcept;
template<class T> void uninit_move_upwards(T* data, std::size_t size, std::size_t dist) noexcept;
/// \}



/// \brief A slab of memory adjacent objects.
///
/// A slab object owns a single chunk of dynamically allocated memory with enough space for
/// the requested number of objects. This memory will never be reallocated, other than
/// through explicit invocation of \ref recreate(), so any pointers to contained objects
/// will remain valid as long as \ref recreate() and \ref operator=() are not called, even
/// across move construction and move assignment.
///
/// The slab has a maximum size (the \p capacity argument passed to the constructor), and a
/// current size (\ref size()). The size increases by one every time \ref add() is
/// called. Behavior is undefined if \ref add() is called at a time where the current size
/// is equal to the capacity.
///
/// When the slab is destroyed, the objects will be destroyed in reverse order. That is, the
/// object, that was added last, will be destroyed first.
///
template<class T> class Slab {
public:
    static_assert(std::is_nothrow_destructible_v<T>);

    using value_type = T;

    using iterator       = T*;
    using const_iterator = const T*;

    Slab() noexcept = default;
    explicit Slab(std::size_t capacity);
    explicit Slab(std::size_t size, T fill_value);
    template<class U> explicit Slab(core::Span<U> data);
    Slab(Slab&&) noexcept;
    ~Slab() noexcept;

    auto operator=(Slab&&) noexcept -> Slab&;

    void recreate(std::size_t capacity);
    void recreate(std::size_t size, T fill_value);
    template<class U> void recreate(core::Span<U> data);

    template<class... A> auto add(A&&... args) noexcept(std::is_nothrow_constructible_v<T, A...>) -> T&;

    auto operator[](std::size_t) noexcept       -> T&;
    auto operator[](std::size_t) const noexcept -> const T&;

    bool empty() const noexcept;
    auto size() const noexcept -> std::size_t;

    auto data() noexcept       -> T*;
    auto data() const noexcept -> const T*;

    auto begin() noexcept -> T*;
    auto end() noexcept   -> T*;
    auto begin() const noexcept -> const T*;
    auto end() const noexcept   -> const T*;

private:
    std::unique_ptr<std::byte[]> m_memory;
    std::size_t m_size = 0;

    void destroy() noexcept;
};


template<class T> Slab(core::Span<T>) -> Slab<std::remove_const_t<T>>;



/// \brief Provide aligned storage for array of objects.
///
/// An object of this type contains an array of bytes long enough and sufficiently aligned
/// to act as storage for an array of \p N objects of type \p T. The length of the array of
/// bytes is at least `N * sizeof (T)`. The array of bytes is directly contained in this
/// class, meaning that the size of this class accounts for the size of the array. The
/// pointer to the array of bytes is returned by \ref addr(). If \p N is zero, this class is
/// guaranteed to be an empty class and \ref addr() will return null.
///
template<class T, std::size_t N> class AlignedStorage
    : private impl::AlignedStorage<T, N> {
public:
    using value_type = T;
    static constexpr std::size_t size = N;

    /// \brief Get pointer to array of bytes.
    ///
    /// If \p N is greater than zero, this function returns a pointer to the first byte in
    /// the contained array of bytes. Otherwise this function returns null.
    ///
    auto addr() noexcept -> void*;
};








// Implementation


inline auto suggest_new_buffer_size(std::size_t cur_size, std::size_t min_size,
                                    std::size_t max_size) noexcept -> std::size_t
{
    ARCHON_ASSERT(max_size >= cur_size);
    ARCHON_ASSERT(max_size >= min_size);

    std::size_t new_size = cur_size;
    if (ARCHON_LIKELY(new_size >= min_size))
        return new_size;

    // Use growth factor 1.5.
    std::size_t half = new_size / 2;
    if (ARCHON_LIKELY(half <= std::size_t(max_size - new_size))) {
        new_size += half;
        if (ARCHON_LIKELY(new_size >= min_size))
            return new_size;
        return min_size;
    }
    return max_size;
}


namespace impl {


template<class T, bool is_trivial> struct Uninit_0 {};

template<class T> struct Uninit_0<T, false> {
    template<class... A> static void create(T* uninit, A&&... args)
    {
        new (uninit) T(std::forward<A>(args)...); // Throws
    }

    static void destroy(T* data, std::size_t size) noexcept
    {
        static_assert(std::is_nothrow_destructible_v<T>);
        for (std::size_t i = 0; i < size; ++i)
            data[i].~T();
    }

    static void safe_fill(std::size_t size, T* uninit)
    {
        std::size_t i = 0;
        try {
            while (i < size) {
                new (&uninit[i]) T(); // Throws
                ++i;
            }
        }
        catch (...) {
            // If an exception was thrown above, we need to back out by destroying the
            // instances that were already created.
            while (i > 0) {
                --i;
                static_assert(std::is_nothrow_destructible_v<T>);
                uninit[i].~T();
            }
            throw;
        }
    }

    static void safe_fill(std::size_t size, const T& value, T* uninit)
    {
        std::size_t i = 0;
        try {
            while (i < size) {
                new (&uninit[i]) T(value); // Throws
                ++i;
            }
        }
        catch (...) {
            // If an exception was thrown above, we need to back out by destroying the
            // copies that were already made.
            while (i > 0) {
                --i;
                static_assert(std::is_nothrow_destructible_v<T>);
                uninit[i].~T();
            }
            throw;
        }
    }

    template<class I> static void safe_copy(I begin, I end, T* uninit)
    {
        // Try to copy all elements. If a copy operation fails, destroy the copies that were
        // already made.
        std::size_t i = 0;
        I j = begin;
        try {
            while (j != end) {
                new (&uninit[i]) T(*j); // Throws
                ++i;
                ++j;
            }
        }
        catch (...) {
            destroy(uninit, i);
            throw;
        }
    }

    static void safe_move(T* data, std::size_t size, T* uninit) noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        if constexpr (std::is_nothrow_move_constructible_v<T>) {
            for (std::size_t i = 0; i < size; ++i) {
                new (&uninit[i]) T(std::move(data[i]));
                static_assert(std::is_nothrow_destructible_v<T>);
                data[i].~T();
            }
        }
        else {
            safe_copy(data, data + size, uninit); // Throws
            destroy(data, size); // Destroy originals
        }
    }

    static void safe_move_a(T* data, std::size_t size, T* uninit) noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        if constexpr (std::is_nothrow_move_constructible_v<T>) {
            for (std::size_t i = 0; i < size; ++i)
                new (&uninit[i]) T(std::move(data[i]));
        }
        else {
            safe_copy(data, data + size, uninit); // Throws
        }
    }

    static void move_downwards(T* data, std::size_t size, std::size_t dist) noexcept
    {
        // Move elements towards lower addresses. This is only safe when the elements are
        // nothrow move constructible.
        static_assert(std::is_nothrow_move_constructible_v<T>);
        static_assert(std::is_nothrow_destructible_v<T>);
        T* data_2 = data - dist;
        for (std::size_t i = 0; i < size; ++i) {
            new (&data_2[i]) T(std::move(data[i]));
            data[i].~T();
        }
    }

    static void move_upwards(T* data, std::size_t size, std::size_t dist) noexcept
    {
        // Move elements towards higher addresses. This is only safe when the elements are
        // nothrow move constructible.
        static_assert(std::is_nothrow_move_constructible_v<T>);
        static_assert(std::is_nothrow_destructible_v<T>);
        T* data_2 = data + dist;
        for (std::size_t i = 0; i < size; ++i) {
            std::size_t j = std::size_t((size - 1) - i);
            new (&data_2[j]) T(std::move(data[j]));
            data[j].~T();
        }
    }
};


template<class T> struct Uninit_0<T, true> {
    template<class... A> static void create(T* uninit, A&&... args)
    {
        *uninit = T(std::forward<A>(args)...); // Throws
    }

    static void destroy(T*, std::size_t) noexcept
    {
    }

    static void safe_fill(std::size_t size, T* uninit) noexcept
    {
        std::fill_n(uninit, size, T());
    }

    static void safe_fill(std::size_t size, const T& value, T* uninit) noexcept
    {
        std::fill_n(uninit, size, value);
    }

    template<class I> static void safe_copy(I begin, I end, T* uninit) noexcept
    {
        std::copy(begin, end, uninit);
    }

    static void safe_move(T* data, std::size_t size, T* uninit) noexcept
    {
        std::copy_n(data, size, uninit);
    }

    static void safe_move_a(T* data, std::size_t size, T* uninit) noexcept
    {
        std::copy_n(data, size, uninit);
    }

    static void move_downwards(T* data, std::size_t size, std::size_t dist) noexcept
    {
        std::copy_n(data, size, data - dist);
    }

    static void move_upwards(T* data, std::size_t size, std::size_t dist) noexcept
    {
        std::copy_backward(data, data + size, data + size + dist);
    }
};


template<class T> struct Uninit : Uninit_0<T, std::is_trivial_v<T> && std::is_copy_constructible_v<T> &&
                                           std::is_copy_assignable_v<T>> {
    static_assert(std::is_nothrow_destructible_v<T>);
};


} // namespace impl


template<class T, class... A> void uninit_create(T* uninit, A&&... args)
{
    impl::Uninit<T>::create(uninit, std::forward<A>(args)...); // Throws
}


template<class T> inline void uninit_destroy(T* data, std::size_t size) noexcept
{
    impl::Uninit<T>::destroy(data, size);
}


template<class T> inline void uninit_safe_fill(std::size_t size, T* uninit)
{
    impl::Uninit<T>::safe_fill(size, uninit); // Throws
}


template<class T> inline void uninit_safe_fill(std::size_t size, const T& value, T* uninit)
{
    impl::Uninit<T>::safe_fill(size, value, uninit); // Throws
}


template<class I, class T> inline void uninit_safe_copy(I begin, I end, T* uninit)
{
    impl::Uninit<T>::safe_copy(begin, end, uninit); // Throws
}


template<class T>
inline void uninit_safe_move(T* data, std::size_t size, T* uninit) noexcept(std::is_nothrow_move_constructible_v<T>)
{
    impl::Uninit<T>::safe_move(data, size, uninit); // Throws
}


template<class T>
inline void uninit_safe_move_a(T* data, std::size_t size, T* uninit) noexcept(std::is_nothrow_move_constructible_v<T>)
{
    impl::Uninit<T>::safe_move_a(data, size, uninit); // Throws
}


template<class T> inline void uninit_move_downwards(T* data, std::size_t size, std::size_t dist) noexcept
{
    ARCHON_ASSERT(dist > 0);
    impl::Uninit<T>::move_downwards(data, size, dist);
}


template<class T> inline void uninit_move_upwards(T* data, std::size_t size, std::size_t dist) noexcept
{
    ARCHON_ASSERT(dist > 0);
    impl::Uninit<T>::move_upwards(data, size, dist);
}



// ============================ Slab ============================


template<class T>
inline Slab<T>::Slab(std::size_t capacity)
{
    std::size_t num_bytes = capacity;
    if (ARCHON_UNLIKELY(!core::try_int_mul(num_bytes, sizeof (T))))
        throw std::length_error("Slab capacity");
    m_memory = std::make_unique<std::byte[]>(num_bytes); // Throws
}


template<class T>
Slab<T>::Slab(std::size_t size, T fill_value)
    : Slab(size) // Throws
{
    for (std::size_t i = 0; i < size; ++i)
        add(fill_value); // Throws
}


template<class T>
template<class U> Slab<T>::Slab(core::Span<U> data)
    : Slab(data.size()) // Throws
{
    for (U& elem : data)
        add(elem); // Throws
}


template<class T>
inline Slab<T>::Slab(Slab&& other) noexcept
    : m_memory(std::move(other.m_memory))
    , m_size(other.m_size)
{
    other.m_size = 0;
}


template<class T>
Slab<T>::~Slab() noexcept
{
    destroy();
}


template<class T>
inline auto Slab<T>::operator=(Slab&& other) noexcept -> Slab&
{
    destroy();
    m_memory = std::move(other.m_memory);
    m_size = other.m_size;
    other.m_size = 0;
    return *this;
}


template<class T>
inline void Slab<T>::recreate(std::size_t capacity)
{
    *this = Slab(capacity); // Throws
}


template<class T>
inline void Slab<T>::recreate(std::size_t size, T fill_value)
{
    *this = Slab(size, std::move(fill_value)); // Throws
}


template<class T>
template<class U> inline void Slab<T>::recreate(core::Span<U> data)
{
    *this = Slab(data); // Throws
}


template<class T>
template<class... A> inline auto Slab<T>::add(A&&... args) noexcept(std::is_nothrow_constructible_v<T, A...>) -> T&
{
    T* ptr = data() + m_size;
    new (ptr) T(std::forward<A>(args)...); // Throws
    ++m_size;
    return *ptr;
}


template<class T>
inline auto Slab<T>::operator[](std::size_t i) noexcept -> T&
{
    return data()[i];
}


template<class T>
inline auto Slab<T>::operator[](std::size_t i) const noexcept -> const T&
{
    return data()[i];
}


template<class T>
inline bool Slab<T>::empty() const noexcept
{
    return size() == 0;
}


template<class T>
inline auto Slab<T>::size() const noexcept -> std::size_t
{
    return m_size;
}


template<class T>
inline auto Slab<T>::data() noexcept -> T*
{
    return reinterpret_cast<T*>(m_memory.get());
}


template<class T>
inline auto Slab<T>::data() const noexcept -> const T*
{
    return reinterpret_cast<T*>(m_memory.get());
}


template<class T>
inline auto Slab<T>::begin() noexcept -> T*
{
    return data();
}


template<class T>
inline auto Slab<T>::end() noexcept -> T*
{
    return data() + size();
}


template<class T>
inline auto Slab<T>::begin() const noexcept -> const T*
{
    return data();
}


template<class T>
inline auto Slab<T>::end() const noexcept -> const T*
{
    return data() + size();
}


template<class T>
void Slab<T>::destroy() noexcept
{
    T* base = data();
    T* ptr = base + m_size;
    while (ptr != base) {
        --ptr;
        ptr->~T();
    }
}



// ============================ AlignedStorage ============================


template<class T, std::size_t N>
inline auto AlignedStorage<T, N>::addr() noexcept -> void*
{
    return impl::AlignedStorage<T, N>::addr();
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_MEMORY_HPP
