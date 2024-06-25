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

#ifndef ARCHON_X_DISPLAY_X_VIEWPORT_HPP
#define ARCHON_X_DISPLAY_X_VIEWPORT_HPP

/// \file


#include <optional>
#include <string_view>

#include <archon/display/geometry.hpp>
#include <archon/display/resolution.hpp>


namespace archon::display {


/// \brief Description of single viewport (monitor) of screen.
///
/// Objects of this type are used to describe the viewports (monitors) of a screen (see \ref
/// display::Connection::try_get_screen_conf()).
///
/// \sa \ref display::Connection::try_get_screen_conf()
///
struct Viewport {
    /// \brief Name of video adapter output associated with viewport.
    ///
    /// This is the name of the video adapter output that is associated with this
    /// viewport. It is typically the name of one of the physical connectors on a graphics
    /// card. For example, it could be "DP-0", referring to the first display port on the
    /// card.
    ///
    std::string_view output_name;

    /// \brief Area of viewport within screen's coordinate space.
    ///
    /// This is the viewport area described relative to the coordinate system of the
    /// screen. In general, the viewports of a screen will be non-overlapping, but they are
    /// allowed to overlap, meaning that a particular section of a screen could be displayed
    /// on multiple monitors.
    ///
    display::Box bounds;

    /// \brief Name of attached monitor.
    ///
    /// When available, this is the name of the monitor that is currently attached to the
    /// video adapter output that is associated with this viewport (\ref output_name). In
    /// general, the monitor name will be available when the attached monitor provides EDID
    /// information (see https://glenwing.github.io/docs/VESA-EEDID-A2.pdf).
    ///
    /// \sa https://glenwing.github.io/docs/VESA-EEDID-A2.pdf
    ///
    std::optional<std::string_view> monitor_name;

    /// \brief Resolution of attached monitor in pixels per centimeter.
    ///
    /// When a monitor is attached, and when the information is made available by the
    /// underlying implementation, this is the physical resolution of the attached monitor
    /// in pixels per centimeter. Implementations should not provide this information unless
    /// it is reasonably accurate and reliable.
    ///
    std::optional<display::Resolution> resolution;

    /// \brief Current refresh rate.
    ///
    /// If provided by the underlying implementation, this is the current refresh rate in
    /// hertz (frames per second) of the video adapter output that is associated with this
    /// viewport. Implementations should not provide this information unless it reasonably
    /// accurate and reliable.
    ///
    std::optional<double> refresh_rate;
};


} // namespace archon::display

#endif // ARCHON_X_DISPLAY_X_VIEWPORT_HPP
