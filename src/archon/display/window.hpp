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

#ifndef ARCHON_X_DISPLAY_X_WINDOW_HPP
#define ARCHON_X_DISPLAY_X_WINDOW_HPP

/// \file


#include <memory>
#include <string_view>

#include <archon/util/color.hpp>
#include <archon/display/geometry.hpp>
#include <archon/display/texture.hpp>


namespace archon::display {


/// \brief Representation of window of platform's graphical user interface.
///
/// New windows can be created by calling \ref display::Connection::new_window().
///
/// Visually, a window consists of a rectangular area of contents optionally surrounded by
/// decorations (frame and title bar). The size of a window (\ref set_size(), \ref
/// display::WindowSizeEvent::size) generally refers to the size of the contents area, and
/// the position of a window (\ref display::WindowPosEvent::pos) generally refers to the
/// position of the upper-left corner of the contents area.
///
class Window {
public:
    struct Config;

    /// \{
    ///
    /// \brief Show or hide window.
    ///
    /// A window is either in the "hidden" or in the "unhidden" state. `show()` puts the
    /// window into the "unhidden" state. `hide()` puts the window into the "hidden" state.
    ///
    /// When the window is in the "unhidden" state, `show()` has no effect. When the window
    /// is in the "hidden" state, `hide()` has no effect.
    ///
    /// Initially, a window is in the "hidden" state.
    ///
    virtual void show() = 0;
    virtual void hide() = 0;
    /// \}

    /// \brief Set window title.
    ///
    /// This function changes the title in the title bar of this window to the specified
    /// string (\p title).
    ///
    /// FIXME: Is this specified using a UTF-8 encoded string?                                   
    ///
    virtual void set_title(std::string_view title) = 0;

    /// \brief Resize window.
    ///
    /// This function generates a request to resize the window such that the size of the
    /// contents area is as specified (\p size). The platform may, or may not honor this
    /// request. It may also choose to set a different size than the one specified. In any
    /// case, if the size of the window changes, a "resize" event will be generated (\ref
    /// display::WindowEventHandler::on_resize()), and it will specify the actual new size
    /// of the window.
    ///
    virtual void set_size(display::Size size) = 0;

    /// \brief Turn fullscreen mode on or off.
    ///
    /// This function turns fullscreen mode on or off for the window.
    ///
    /// More than one window can be in fullscreen mode at the same time, but the exact
    /// behavior depends on the implementation and the underlying platform.
    ///
    virtual void set_fullscreen_mode(bool on) = 0;

    /// \brief Fill window with color.
    ///
    /// This function fills the window with the specified color. Call \ref present() to
    /// present the result.
    ///
    virtual void fill(util::Color color) = 0;

    /// \brief     
    ///
    /// FIXME: What are the constraints on allowed object lifetime?                                               
    ///
    virtual auto new_texture(display::Size size) -> std::unique_ptr<display::Texture> = 0;

    /// \brief    
    ///
    ///    
    ///
    virtual void put_texture(const display::Texture&) = 0;

    /// \brief    
    ///
    ///    
    ///
    virtual void present() = 0;

    /// \brief Bind OpenGL context of this window to the calling thread.
    ///
    /// Every window is associated with an OpenGL rendering context. This function binds the
    /// calling thread to that rendering context, such that OpenGL rendering performed by
    /// the calling thread is directed at this window. On an X11 platform, this corresponds
    /// to `glXMakeCurrent()`.
    ///
    virtual void opengl_make_current() = 0;

    /// \brief Exchange front and back buffers for OpenGL rendering.
    ///
    /// This function swaps front and back buffers for OpenGL rendering in this window. On
    /// an X11 platform, this corresponds to `glXSwapBuffers()`.
    ///
    virtual void opengl_swap_buffers() = 0;

    virtual ~Window() noexcept = default;
};


/// \brief Window configuration parameters.
///
/// These are the available parameters for configuring a window.
///
struct Window::Config {
    /// \brief Cookie value to be passed to window event handlers.
    ///
    /// The value specified here will be passed faithfully by the event processor (\ref
    /// display::Connection::process_events()) to all handlers of events that originate from
    /// a window that was created using this configuration. See also \ref
    /// display::WindowEvent::cookie.
    ///
    int cookie = 0;

    /// \brief Allow for window to be resized.
    ///
    /// If set to `true`, the window will be made resizable.
    ///
    bool resizable = false;

    /// \brief start out in fullscreen mode.
    ///
    /// If set to `true`, the window will start out in fullscreen mode.
    ///
    bool fullscreen = false;

    /// \brief Enable OpenGL rendering.
    ///
    /// If set to `true`, the window will be configured to support OpenGL rendering.
    ///
    bool enable_opengl = false;
};


} // namespace archon::display

#endif // ARCHON_X_DISPLAY_X_WINDOW_HPP
