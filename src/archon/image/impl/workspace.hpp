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

#ifndef ARCHON_X_IMAGE_X_IMPL_X_WORKSPACE_HPP
#define ARCHON_X_IMAGE_X_IMPL_X_WORKSPACE_HPP


#include <cstddef>
#include <type_traits>
#include <utility>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/buffer.hpp>
#include <archon/image/geom.hpp>
#include <archon/image/iter.hpp>
#include <archon/image/tray.hpp>


namespace archon::image::impl {


template<class T> class Workspace {
public:
    using comp_type = T;
    using span_type = core::Span<comp_type>;

    // **CAUTION:** It is the callers responsibility that the buffer memory
    // (`buffer.data()`) is suitably aligned for `T`.
    Workspace(core::Buffer<std::byte>& buffer) noexcept;
    Workspace(span_type seed_mem, core::Buffer<std::byte>& buffer) noexcept;
    Workspace(core::Buffer<std::byte>& buffer, int num_channels, image::Size image_size = 1);
    Workspace(span_type seed_mem, core::Buffer<std::byte>& buffer, int num_channels, image::Size image_size = 1);

    ~Workspace() noexcept = default;

    void reset(int num_channels, image::Size image_size = 1);

    auto data() noexcept       -> T*;
    auto data() const noexcept -> const T*;

    auto size() const noexcept -> std::size_t;

    auto operator[](std::size_t)       noexcept ->       T&;
    auto operator[](std::size_t) const noexcept -> const T&;

    using tray_type       = image::Tray<comp_type>;
    using const_tray_type = image::Tray<const comp_type>;

    // It is the callers responsibility that the total number of components (`num_channels`
    // times `image_size.width` times `image_size.height`) is less than, or equal to the
    // size of the workspace, i.e., the value returned by size().
    auto tray(int num_channels, image::Size image_size) noexcept       -> tray_type;
    auto tray(int num_channels, image::Size image_size) const noexcept -> const_tray_type;

private:
    span_type m_seed_mem;
    core::Buffer<std::byte>& m_buffer;

    class SubconstructDeleter;
    using subconstruct_type = std::unique_ptr<comp_type[], SubconstructDeleter>;

    subconstruct_type m_subconstruct;
    span_type m_span;
};








// Implementation


template<class T>
class Workspace<T>::SubconstructDeleter {
public:
    SubconstructDeleter(std::size_t size = 0) noexcept
        : m_size(size)
    {
    }
    void operator()(T* ptr) const noexcept
    {
        static_assert(std::is_nothrow_destructible_v<T>);
        for (std::size_t i = 0; i < m_size; ++i)
            ptr[i].~T();
    }
private:
    std::size_t m_size;
};


template<class T>
inline Workspace<T>::Workspace(core::Buffer<std::byte>& buffer) noexcept
    : Workspace(span_type(), buffer)
{
}


template<class T>
inline Workspace<T>::Workspace(span_type seed_mem, core::Buffer<std::byte>& buffer) noexcept
    : m_seed_mem(seed_mem)
    , m_buffer(buffer)
{
}


template<class T>
inline Workspace<T>::Workspace(core::Buffer<std::byte>& buffer, int num_channels, image::Size image_size)
    : Workspace(span_type(), buffer, num_channels, image_size) // Throws
{
}


template<class T>
inline Workspace<T>::Workspace(span_type seed_mem, core::Buffer<std::byte>& buffer, int num_channels, image::Size image_size)
    : Workspace(seed_mem, buffer)
{
    reset(num_channels, image_size); // Throws
}


template<class T>
void Workspace<T>::reset(int num_channels, image::Size image_size)
{
    std::size_t size = 1;
    core::int_mul(size, num_channels); // Throws
    core::int_mul(size, image_size.width); // Throws
    core::int_mul(size, image_size.height); // Throws
    if (ARCHON_LIKELY(size <= m_seed_mem.size())) {
        m_subconstruct = {};
        m_span = m_seed_mem;
    }
    else {
        std::size_t byte_size = size;
        core::int_mul(byte_size, sizeof (T)); // Throws
        m_buffer.reserve(byte_size); // Throws
        m_subconstruct = subconstruct_type(new (m_buffer.data()) T[size], SubconstructDeleter(size)); // Throws
        m_span = { m_subconstruct.get(), size };
    }
}


template<class T>
inline auto Workspace<T>::data() noexcept -> T*
{
    return m_span.data();
}


template<class T>
inline auto Workspace<T>::data() const noexcept -> const T*
{
    return m_span.data();
}


template<class T>
inline auto Workspace<T>::size() const noexcept -> std::size_t
{
    return m_span.size();
}


template<class T>
inline auto Workspace<T>::operator[](std::size_t i) noexcept -> T&
{
    return data()[i];
}


template<class T>
inline auto Workspace<T>::operator[](std::size_t i) const noexcept -> const T&
{
    return data()[i];
}


template<class T>
auto Workspace<T>::tray(int num_channels, image::Size image_size) noexcept -> tray_type
{
    std::ptrdiff_t horz_stride = std::ptrdiff_t(num_channels);
    std::ptrdiff_t vert_stride = std::ptrdiff_t(num_channels * image_size.width);
    image::Iter iter = { data(), horz_stride, vert_stride };
    return { iter, image_size };
}


template<class T>
auto Workspace<T>::tray(int num_channels, image::Size image_size) const noexcept -> const_tray_type
{
    std::ptrdiff_t horz_stride = std::ptrdiff_t(num_channels);
    std::ptrdiff_t vert_stride = std::ptrdiff_t(num_channels * image_size.width);
    image::Iter iter = { data(), horz_stride, vert_stride };
    return { iter, image_size };
}


} // namespace archon::image::impl

#endif // ARCHON_X_IMAGE_X_IMPL_X_WORKSPACE_HPP
