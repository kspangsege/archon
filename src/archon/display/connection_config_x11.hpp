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

#ifndef ARCHON_X_DISPLAY_X_CONNECTION_CONFIG_X11_HPP
#define ARCHON_X_DISPLAY_X_CONNECTION_CONFIG_X11_HPP

/// \file


#include <cstdint>
#include <optional>
#include <string_view>


namespace archon::display {


/// \brief Connection configuration parameters specific to X11-based implementation.
///
/// This is the set of configuration parameters that pertain to connections when using the
/// X11-based display implementation.
///
struct ConnectionConfigX11 {
    /// \brief The X11 display to be connected to.
    ///
    /// This is a string that identifies a particular X11 display to connect to. Refer to
    /// `XOpenDisplay()` for information on how this works. If left empty, the value of the
    /// `DISPLAY` environment variable will be used.
    ///
    std::string_view display;

    /// \brief Depth (bits per pixel) to be used for X11 windows.
    ///
    /// If set, X11 windows will be configured for this depth instead of the default depth
    /// for the targeted X11 screen. This works only if the specified depth and the selected
    /// visual (\ref visual) is a valid combination for the targeted X11 screen (see output
    /// from command `xdpyinfo`).
    ///
    /// \sa \ref visual
    ///
    std::optional<int> depth;

    /// \brief Visual type to be used for X11 windows.
    ///
    /// If set, X11 windows will be configured for the visual type identified by the
    /// specified value. If left unset, the default visual for the targeted X11 screen will
    /// be used. Specifying a visual works only if it and the selected depth (\ref depth) is
    /// a valid combination for the targeted X11 screen (see output from command
    /// `xdpyinfo`).
    ///
    /// \sa \ref depth
    ///
    std::optional<std::uint_fast32_t> visual;

    /// \brief Disable use of double buffering even when supported.
    ///
    /// By default, i.e., when `false`, double buffering will be used when the Double Buffer
    /// Extension (Xdbe) is available (enabled at build time), and double buffering is
    /// supported on the targeted X11 screen with the selected depth and visual (\ref depth,
    /// \ref visual). When set to `true`, double buffering will not be used at all.
    ///
    bool disable_double_buffering = false;

    /// \brief Turn on synchronous mode on X11 connection.
    ///
    /// If set to `true`, *synchronous mode* will be turned on for the X11 connection to be
    /// established. This is useful only for debugging the use of the X11 client
    /// library. See `XSynchronize()` for additional information.
    ///
    bool synchronous_mode = false;
};


} // namespace archon::display

#endif // ARCHON_X_DISPLAY_X_CONNECTION_CONFIG_X11_HPP
