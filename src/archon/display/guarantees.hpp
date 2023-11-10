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

#ifndef ARCHON_X_DISPLAY_X_GUARANTEES_HPP
#define ARCHON_X_DISPLAY_X_GUARANTEES_HPP

/// \file


#include <utility>


namespace archon::display {


/// \brief Guarantees to enable display implementations.
///
/// This is a set of guarantees that an application can provide when querying for available
/// display implementations (\ref display::Implementation::is_available()). In general, the
/// more guarantees are provided, the more implementations will be available.
///
/// | Guarantee               | SDL
/// |-------------------------|----------
/// | `only_one_connection`   | required
/// | `main_thread_exclusive` | required
/// | `no_other_use_of_sdl`   | required
///
struct Guarantees {
    /// \brief No overlapping display connections.
    ///
    /// The application may set this field to `true` if it promises to never open more than
    /// one display connection at a time (\ref display::Implementation::new_connection())
    /// from the same display implementation, or from different display implementations.
    ///
    bool only_one_connection = false;

    /// \brief Everything happens on behalf of main thread.
    ///
    /// The application may set this field to `true` if it promises that all use of the API
    /// of the Archon Display Library happens on behalf of the main thread.
    ///
    /// Note that destruction of unique pointers to connections and windows count as use of
    /// the API of the Archon Display Library, and so must also happen on behalf of the main
    /// thread.
    ///
    bool main_thread_exclusive = false;

    /// \brief No other use of SDL, or of anything that conflicts with SDL.
    ///
    /// The application may set this field to `true` if it promises that there is no direct
    /// or indirect use of SDL (Simple DirectMedia Layer) other than through the Archon
    /// Display Library, and that there is also no direct or indirect use of anything that
    /// would conflict with use of SDL.
    ///
    bool no_other_use_of_sdl = false;

    /// \brief Default constructor.
    ///
    /// This default constructor constructs an initially empty set of guarantees (all
    /// guarantees are absent).
    ///
    Guarantees() noexcept = default;

    /// \{
    ///
    /// \brief Moveability, but no copyability.
    ///
    /// A set of guarantees is movable, but not copyable.
    ///
    Guarantees(Guarantees&&) noexcept;
    auto operator=(Guarantees&&) noexcept -> Guarantees&;
    /// \}
};








// Implementation


inline Guarantees::Guarantees(Guarantees&& other) noexcept
{
    *this = std::move(other);
}


inline auto Guarantees::operator=(Guarantees&& other) noexcept -> Guarantees&
{
    only_one_connection   = other.only_one_connection;
    main_thread_exclusive = other.main_thread_exclusive;
    no_other_use_of_sdl   = other.no_other_use_of_sdl;

    other.only_one_connection   = false;
    other.main_thread_exclusive = false;
    other.no_other_use_of_sdl   = false;

    return *this;
}


} // namespace archon::display

#endif // ARCHON_X_DISPLAY_X_GUARANTEES_HPP
