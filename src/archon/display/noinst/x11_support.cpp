// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2024 Kristian Spangsege <kristian.spangsege@gmail.com>
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


#include <optional>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/display/connection_config_x11.hpp>
#include <archon/display/noinst/x11_support.hpp>


using namespace archon;
namespace impl = display::impl;


auto impl::map_opt_visual_class(const std::optional<display::ConnectionConfigX11::VisualClass>& class_) noexcept ->
    std::optional<int>
{
    if (ARCHON_LIKELY(!class_.has_value()))
        return {};
    switch (class_.value()) {
        case display::ConnectionConfigX11::VisualClass::static_gray:
            return StaticGray;
        case display::ConnectionConfigX11::VisualClass::gray_scale:
            return GrayScale;
        case display::ConnectionConfigX11::VisualClass::static_color:
            return StaticColor;
        case display::ConnectionConfigX11::VisualClass::pseudo_color:
            return PseudoColor;
        case display::ConnectionConfigX11::VisualClass::true_color:
            return TrueColor;
        case display::ConnectionConfigX11::VisualClass::direct_color:
            return DirectColor;
    }
    ARCHON_ASSERT_UNREACHABLE();
    return {};
}
