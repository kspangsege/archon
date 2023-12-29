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
/// This function returns the display implementation slot for the implementation that is
/// based on the Simple DirectMedia Layer (SDL).
///
/// This implementation is available if enabled at compile time (`ARCHON_DISPLAY_HAVE_SDL`)
/// and the set of specified display guarantees includes \ref
/// display::Guarantees::only_one_connection, \ref
/// display::Guarantees::main_thread_exclusive, and \ref
/// display::Guarantees::no_other_use_of_sdl.
///
/// \sa https://www.libsdl.org
///
auto get_sdl_implementation_slot() noexcept -> const display::Implementation::Slot&;


} // namespace archon::display

#endif // ARCHON_X_DISPLAY_X_IMPLEMENTATION_SDL_HPP
