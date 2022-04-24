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


#include <stdexcept>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/font/loader_fallback.hpp>
#include <archon/font/loader_freetype.hpp>


using namespace archon;


namespace {


using impl_getter_type = auto (*)() noexcept -> const font::Loader::Implementation*;


constexpr impl_getter_type g_known_implementations[] {
    &font::loader_freetype_impl,
};


constexpr int g_num_known_implementations = int(std::size(g_known_implementations));



struct Init {
    int num_implementations = 0;
    const font::Loader::Implementation* implementations[g_num_known_implementations + 1];

    Init() noexcept
    {
        for (int i = 0; i < g_num_known_implementations; ++i) {
            const font::Loader::Implementation* impl = (*g_known_implementations[i])();
            if (impl) {
                implementations[num_implementations] = impl;
                ++num_implementations;
            }
        }
        implementations[num_implementations] = &font::loader_fallback_impl();
        ++num_implementations;
    }
};

inline auto get_init() noexcept -> const Init&
{
    static Init init;
    return init;
}


} // unnamed namespace


using font::Loader;


auto Loader::get_default_implementation() noexcept -> const Implementation&
{
    const Init& init = get_init();
    ARCHON_ASSERT(init.num_implementations > 0);
    return *init.implementations[0];
}


int Loader::get_num_implementations() noexcept
{
    const Init& init = get_init();
    return init.num_implementations;
}


auto Loader::get_implementation(int index) -> const Implementation&
{
    const Init& init = get_init();
    if (ARCHON_LIKELY(index >= 0 && index < init.num_implementations))
        return *init.implementations[index];
    throw std::out_of_range("Implementation index");
}


auto Loader::lookup_implementation(std::string_view ident) noexcept -> const Implementation*
{
    const Init& init = get_init();
    int n = init.num_implementations;
    for (int i = 0; i < n; ++i) {
        const Implementation& impl = *init.implementations[i];
        if (ARCHON_LIKELY(impl.ident() != ident))
            continue;
        return &impl;
    }
    return nullptr;
}
