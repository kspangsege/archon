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

#ifndef ARCHON_X_DISPLAY_X_MOUSE_BUTTON_HPP
#define ARCHON_X_DISPLAY_X_MOUSE_BUTTON_HPP

/// \file


#include <archon/core/enum.hpp>


namespace archon::display {


/// \brief Mouse buttons.
///
/// These are the mouse buttons recognized by the Archon Display Library.
///
/// A specialization of \ref core::EnumTraits is provided, making stream input and output
/// readily available.
///
/// \sa \ref display::MouseButtonEvent
///
enum class MouseButton {
    left,
    middle,
    right,
    x1,
    x2,
};


} // namespace archon::display

namespace archon::core {

template<> struct EnumTraits<display::MouseButton> {
    static constexpr bool is_specialized = true;
    struct Spec {
        static constexpr core::EnumAssoc map[] = {
            { int(display::MouseButton::left),   "left"   },
            { int(display::MouseButton::middle), "middle" },
            { int(display::MouseButton::right),  "right"  },
            { int(display::MouseButton::x1),     "x1"     },
            { int(display::MouseButton::x2),     "x2"     },
        };
    };
    static constexpr bool ignore_case = true;
};

} // namespace archon::core

#endif // ARCHON_X_DISPLAY_X_MOUSE_BUTTON_HPP
