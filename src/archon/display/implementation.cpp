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


#include <array>
#include <string_view>
#include <stdexcept>

#include <archon/core/features.h>
#include <archon/display/implementation.hpp>
#include <archon/display/implementation_sdl.hpp>


using namespace archon;


namespace {


using impl_getter_type = auto (*)() noexcept -> const display::Implementation&;


constexpr impl_getter_type g_implementations[] {
    &display::get_sdl_implementation,
};


constexpr int g_num_implementations = int(std::size(g_implementations));


} // unnamed namespace


auto display::get_default_implementation(const display::Implementation::Mandates& mandates) noexcept ->
    const display::Implementation*
{
    int n = g_num_implementations;
    for (int i = 0; i < n; ++i) {
        const display::Implementation& impl = (*g_implementations[i])();
        if (ARCHON_LIKELY(!impl.is_available(mandates)))
            continue;
        return &impl;
    }
    return nullptr;
}


int display::get_num_implementations() noexcept
{
    return g_num_implementations;
}


auto display::get_implementation(int index) -> const display::Implementation&
{
    if (ARCHON_LIKELY(index >= 0 && index < g_num_implementations))
        return (*g_implementations[index])();
    throw std::out_of_range("Implementation index");
}


auto display::lookup_implementation(std::string_view ident) noexcept -> const display::Implementation*
{
    int n = g_num_implementations;
    for (int i = 0; i < n; ++i) {
        const display::Implementation& impl = (*g_implementations[i])();
        if (ARCHON_LIKELY(impl.ident() != ident))
            continue;
        return &impl;
    }
    return nullptr;
}
