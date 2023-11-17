// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2023 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_CORE_X_IMPL_X_MEMORY_HPP
#define ARCHON_X_CORE_X_IMPL_X_MEMORY_HPP

/// \file


#include <cstddef>


namespace archon::core::impl {


template<class T, std::size_t N> class AlignedStorage {
public:
    static constexpr std::size_t num_bytes = N * sizeof (T);

    alignas (T) std::byte bytes[num_bytes];

    auto addr() noexcept -> void*
    {
        return bytes;
    }
};

template<class T> class AlignedStorage<T, 0> {
public:
    auto addr() noexcept -> void*
    {
        return nullptr;
    }
};


} // namespace archon::core::impl

#endif // ARCHON_X_CORE_X_IMPL_X_MEMORY_HPP
