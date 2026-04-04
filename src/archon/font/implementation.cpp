// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2026 Kristian Spangsege <kristian.spangsege@gmail.com>
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


#include <iterator>
#include <stdexcept>
#include <string_view>

#include <archon/core/features.hpp>
#include <archon/core/assert.hpp>
#include <archon/font/implementation.hpp>
#include <archon/font/fallback_implementation.hpp>
#include <archon/font/freetype_implementation.hpp>


using namespace archon;


namespace {


using implementation_getter_type = auto (*)() noexcept -> const font::implementation&;

constexpr implementation_getter_type g_implementation_getters[] {
    &font::get_freetype_implementation,
    &font::get_fallback_implementation,
};

constexpr int g_num_implementations = int(std::size(g_implementation_getters));


struct Init {
    const font::implementation* implementations[g_num_implementations] = {};
    const font::implementation* default_implementation = {};

    Init() noexcept
    {
        for (int i = 0; i < g_num_implementations; ++i) {
            implementation_getter_type getter = g_implementation_getters[i];
            const font::implementation& impl = getter();
            implementations[i] = &impl;
            if (!default_implementation && impl.is_available())
                default_implementation = &impl;
        }
        // Because the fallback implementation is required to always be avilable, a default
        // implementation will always have been selected
        ARCHON_ASSERT(default_implementation);
    }
};

inline auto get_init() noexcept -> const Init&
{
    static Init init;
    return init;
}


} // unnamed namespace


auto font::get_default_implementation() noexcept -> const font::implementation&
{
    const Init& init = get_init();
    return *init.default_implementation;
}


int font::get_num_implementations() noexcept
{
    return g_num_implementations;
}


auto font::get_implementation(int index) -> const font::implementation&
{
    const Init& init = get_init();
    if (ARCHON_LIKELY(index >= 0 && index < g_num_implementations))
        return *init.implementations[index];
    throw std::out_of_range("Implementation index");
}


auto font::lookup_implementation(std::string_view ident) noexcept -> const font::implementation*
{
    const Init& init = get_init();
    int n = g_num_implementations;
    for (int i = 0; i < n; ++i) {
        const font::implementation& impl = *init.implementations[i];
        if (ARCHON_LIKELY(impl.get_ident() != ident))
            continue;
        return &impl;
    }
    return nullptr;
}
