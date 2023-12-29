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

#include <archon/core/enum.hpp>


namespace archon::display {


/// \brief Connection configuration parameters specific to X11-based implementation.
///
/// This is the set of configuration parameters that pertain to connections when using the
/// X11-based display implementation.
///
struct ConnectionConfigX11 {
    enum class VisualClass;

    /// \brief The X11 display to be connected to.
    ///
    /// This is a string that identifies a particular X11 display to connect to. Refer to
    /// `XOpenDisplay()` for information on how this works. If unspecified, the value of the
    /// `DISPLAY` environment variable will be used.
    ///
    std::optional<std::string_view> display;

    /// \brief Visual depth to be used for X11 windows.
    ///
    /// If specified, only that depth (number of bits per pixel) will be considered when
    /// picking an X11 visual to be used with a particular window (see output from command
    /// `xdpyinfo`).
    ///
    /// \sa \ref visual_class
    /// \sa \ref visual_type
    ///
    std::optional<int> visual_depth;

    /// \brief Visual class to be used for X11 windows.
    ///
    /// If specified, only that visual class will be considered when picking an X11 visual
    /// to be used with a particular window (see output from command `xdpyinfo`).
    ///
    /// \sa \ref visual_depth
    /// \sa \ref visual_type
    ///
    std::optional<VisualClass> visual_class;

    /// \brief Visual type to be used for X11 windows.
    ///
    /// If specified, only that visual type will be considered when picking an X11 visual to
    /// be used with a particular window (see output from command `xdpyinfo`).
    ///
    /// It is an error of the specified value is larger than 2^32-1 (two to the power of 32
    /// minus one).
    ///
    /// \sa \ref visual_depth
    /// \sa \ref visual_class
    ///
    std::optional<std::uint_fast32_t> visual_type;

    /// \brief Prefer use of default colormap for nonstatic nondecomposed visuals.
    ///
    /// By default, when using a nonstatic nondecomposed visual (`PseudoColor` or
    /// `GrayScale`) and no standard colormap is found (window property `RGB_DEFAULT_MAP`),
    /// a new colormap is created. If this parameter is set to `true`, however, and the
    /// selected visual is also the default visual, an attempt will be made to allocate a
    /// reasonable number of colors from the default colormap. If that succeeds, the default
    /// colormap will be used. Otherwise a new colormap will be created.
    ///
    bool prefer_default_nondecomposed_colormap = false;

    /// \brief Disable use of double buffering even when supported.
    ///
    /// By default, i.e., when `false`, double buffering will be used when the Double Buffer
    /// Extension (Xdbe) is available (enabled at build time), and double buffering is
    /// supported by the selected X11 visual (see \ref visual_depth, \ref visual_class, and
    /// \ref visual_type). When set to `true`, double buffering will not be used at all.
    ///
    bool disable_double_buffering = false;

    /// \brief Disable use of OpenGL GLX direct rendering.
    ///
    /// If set to `true`, direct rendering will be disabled for the OpenGL GLX rendering
    /// context created for each window. When set to `false` (the default), direct rendering
    /// will be used when available.
    ///
    bool disable_glx_direct_rendering = false;

    /// \brief Disable use of "detectable auto-repeat" mode.
    ///
    /// The X Keyboard Extension makes a so-called "detectable auto-repeat" mode
    /// conditionally available. By default, "detectable auto-repeat" mode is turned on when
    /// possible. If `disable_detectable_autorepeat` is set to `true`, this mode will not be
    /// turned on, even when it can be. When "detectable auto-repeat" mode is **not** turned
    /// on, either because it is unavailable or because it is disabled by
    /// `disable_detectable_autorepeat`, a "poor man's" fall-back mechanism is used for
    /// detecting when key events are caused by key repetition.
    ///
    /// This flag exists primarily for debugging purposes.
    ///
    bool disable_detectable_autorepeat = false;

    /// \brief Turn on synchronous mode on X11 connection (debugging).
    ///
    /// If set to `true`, *synchronous mode* will be turned on for the X11 connection to be
    /// established. This is useful only for debugging the use of the X11 client
    /// library. See `XSynchronize()` for additional information.
    ///
    bool synchronous_mode = false;

    /// \brief Install colormap after window creation (debugging).
    ///
    /// If set to `true`, a window's colormap will be installed right after the creation of
    /// the window. This mode should only be enabled for debugging purposes, or when running
    /// against a server where there is no window manager. Normally, it is the job of the
    /// window manager to install colormaps.
    ///
    bool install_colormaps = false;

    /// \brief Introduce detectable weirdness when creating new colormaps.
    ///
    /// If set to `true`, detectable weirdness will be introduced into any new colormap that
    /// is created. This is a debugging aid. I t allow one to see whether a window uses a
    /// newly created colormap.
    ///
    bool colormap_weirdness = false;
};


/// \brief Mouse buttons.
///
/// These are the six visual classes that are used to characterize X11 visuals.
///
/// A specialization of \ref core::EnumTraits is provided, making stream input and output
/// readily available.
///
enum class ConnectionConfigX11::VisualClass {
    static_gray  = 0,
    gray_scale   = 1,
    static_color = 2,
    pseudo_color = 3,
    true_color   = 4,
    direct_color = 5,
};


} // namespace archon::display

namespace archon::core {

template<> struct EnumTraits<display::ConnectionConfigX11::VisualClass> {
    static constexpr bool is_specialized = true;
    struct Spec {
        static constexpr core::EnumAssoc map[] = {
            { int(display::ConnectionConfigX11::VisualClass::static_gray),  "StaticGray"  },
            { int(display::ConnectionConfigX11::VisualClass::gray_scale),   "GrayScale"   },
            { int(display::ConnectionConfigX11::VisualClass::static_color), "StaticColor" },
            { int(display::ConnectionConfigX11::VisualClass::pseudo_color), "PseudoColor" },
            { int(display::ConnectionConfigX11::VisualClass::true_color),   "TrueColor"   },
            { int(display::ConnectionConfigX11::VisualClass::direct_color), "DirectColor" },
        };
    };
    static constexpr bool ignore_case = false;
};

} // namespace archon::core

#endif // ARCHON_X_DISPLAY_X_CONNECTION_CONFIG_X11_HPP
