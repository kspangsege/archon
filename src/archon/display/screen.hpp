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

#ifndef ARCHON_X_DISPLAY_X_SCREEN_HPP
#define ARCHON_X_DISPLAY_X_SCREEN_HPP

/// \file


#include <optional>
#include <string_view>

#include <archon/display/geometry.hpp>
#include <archon/display/resolution.hpp>


namespace archon::display {


/// \brief Description of single screen of display.
///
/// Objects of this type are used to describe individual screens / monitors of a
/// display. See \ref display::Connection for general information about displays.
///
/// See \ref display::Connection::try_get_display_conf() for an example of where this type
/// is used.
///
struct Screen {
    /// \brief Name of output associated with screen.
    ///
    /// This is the name of the output associated with this screen. It might be the name of
    /// one of the physical connectors of a graphics card. For example, it could be "DP-0"
    /// referring to the first display port of the card.
    ///
    std::string_view output_name;

    /// \brief Area of screen within display.
    ///
    /// This is the area of the screen within the coordinate system of the display. In
    /// general, the screens of a display will be non-overlapping, but they are allowed to
    /// overlap, meaning that more than one screen may display a particular section of a
    /// display.
    ///
    display::Box bounds;

    /// \brief Name of attached monitor.
    ///
    /// This is the name of the monitor attached to the output associated with this
    /// screen. It is available when EDID information is provided by the monitor, and it is
    /// also made available to the display implementation.
    ///
    /// \sa https://glenwing.github.io/docs/VESA-EEDID-A2.pdf
    ///
    std::optional<std::string_view> monitor_name;

    /// \brief Resolution of screen in pixels per centimeter.
    ///
    /// If provided by the implementation, this is the physical resolution of the screen in
    /// pixels per centimeter. Implementations are not required to provide this information,
    /// and should not do so unless the information is accurate and reliable.
    ///
    std::optional<display::Resolution> resolution;

    /// \brief Refresh rate of screen in hertz.
    ///
    /// If provided by the implementation, this is the refresh rate of the screen in hertz
    /// (frames per second). Implementations are not required to provide this information,
    /// and should not do so unless the information is accurate and reliable.
    ///
    std::optional<double> refresh_rate;
};


} // namespace archon::display

#endif // ARCHON_X_DISPLAY_X_SCREEN_HPP
