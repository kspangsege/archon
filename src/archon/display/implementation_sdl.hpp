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

#ifndef ARCHON_X_DISPLAY_X_IMPLEMENTATION_SDL_HPP
#define ARCHON_X_DISPLAY_X_IMPLEMENTATION_SDL_HPP

/// \file


#include <archon/display/implementation.hpp>


namespace archon::display {


/// \brief Display implementation based on the Simple DirectMedia Layer (SDL).
///
/// If enabled at build time, this function returns the display implmentation that is based
/// on the Simple DirectMedia Layer (SDL). If disabled at compile time, it returns a
/// degenerate implementation where \ref display::Implementation::is_available() returns
/// `false` regardless of the specified guarantees (\ref display::Guarantees).
///
/// \sa https://www.libsdl.org
///
auto get_sdl_implementation() noexcept -> const display::Implementation&;


} // namespace archon::display

#endif // ARCHON_X_DISPLAY_X_IMPLEMENTATION_SDL_HPP
