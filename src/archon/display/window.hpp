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
#include <optional>
#include <string_view>

#include <archon/util/color.hpp>
#include <archon/display/geometry.hpp>
#include <archon/display/texture.hpp>


namespace archon::display {


/// \brief Representation of window of platform's graphical user interface.
///
/// An instance of this class represents a window of the platform's graphical user
/// interface. New windows can be created by calling \ref display::Connection::new_window().
///
/// For window events to be processed, the application must set an event handler for the
/// window using \ref set_event_handler(). To avoid loosing events, it is important that the
/// application sets the event handler before the next invocation of \ref
/// display::Connection::process_events() or \ref display::Connection::process_events_a() on
/// the connection object associated with the window.
///
/// Visually, a window consists of a rectangular area of contents optionally surrounded by
/// decorations (frame and title bar). The rectangular area of contents inside the
/// decorations is referred to as the window's *contents area* in the rest of the
/// documentation of the Archon Display Library.
///
/// The size of a window (\ref set_size(), \ref display::WindowSizeEvent::size) generally
/// refers to the size of the contents area, and the position of a window (\ref
/// display::WindowPosEvent::pos) generally refers to the position of the upper-left corner
/// of the contents area.
///
///
/// #### Redrawing
///
/// Windows will need to have their contents redrawn from time to time. Even when the window
/// does not get resized, its contents may get "damaged" due to the inner workings of the
/// platform's graphical user interface (X11). A resized window will generally also require
/// redrawing. For windows with static contents (contents depends at most on size of window)
/// redrawing can be done in an event handler for the "expose" event (\ref
/// display::WindowEventHandler::on_expose()). The "expose" event is generated both when the
/// window contents is damaged and when the window is resized.
///
/// In more complex scenarios, where the window contents is dynamic (may change for reasons
/// other than a resized window), applications can use a handler for the "before sleep"
/// pseudo event to perform redrawing (\ref
/// display::ConnectionEventHandler::before_sleep()). For example, a flag can be used to
/// indicate that redrawing is needed, and the "before sleep" handler can check the flag and
/// redraw when needed. An application with multiple windows may want a separate flag per
/// window.
///
/// In applications performing a frame-based update of the window contents, redrawing will
/// generally happen outside the even processor, i.e., between successive invocations of
/// \ref display::Connection::process_events_a(). With such an application, a "before sleep"
/// handler can be used to interrupt event processing when a "redraw" flag is raised. This
/// could be done to ensure minimal redraw latency in cases where the frame rate is low.
///
class Window {
public:
    struct Config;

    /// \brief Set new event handler for window.
    ///
    /// This function sets a new event handler for the window. Events generated on behalf of
    /// this window will be reported through the specified event handler. Elsewhere in the
    /// documentation, this is referred to as the window's *associated event handler*.
    ///
    /// The event handler, that is initially the window's associated event handler, does
    /// what an instance of \ref display::WindowEventHandler would do, i.e., it ignores all
    /// events except "close" events which will cause event processing to be terminated.
    ///
    /// It is important that a proper event handler is set before the event processor is
    /// invoked again, that is, before the next invocation of \ref
    /// display::Connection::process_events() or \ref
    /// display::Connection::process_events_a(). Otherwise events might be lost.
    ///
    /// \sa \ref display::Connection::process_events()
    /// \sa \ref display::ConnectionEventHandler
    /// \sa \ref unset_event_handler()
    ///
    virtual void set_event_handler(display::WindowEventHandler&) = 0;

    /// \brief Remove event handler from window.
    ///
    /// This function removes any previously set event handler for the window.
    ///
    /// \sa \ref set_event_handler()
    ///
    virtual void unset_event_handler() noexcept = 0;

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
    /// string (\p title). The characters must be encoded in accordance with the multi-byte
    /// encoding of the associated locale (\ref display::Implementation::new_connection()).
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
    /// \note Switching to or from fullscreen mode is supposed to generate "reposition"
    /// events, but this does not always happen. See \ref
    /// display::WindowEventHandler::on_reposition() for more information.
    ///
    virtual void set_fullscreen_mode(bool on) = 0;

    /// \brief    
    ///
    ///    
    ///
    virtual void set_immersive_mode(bool on) = 0;

    /// \{
    ///
    /// \brief Fill area with color.
    ///
    /// These function fill an area of the window with the specified color (\p color). The
    /// overload that takes an area argument (\p area) fills that area. The other overload
    /// fills the entire window. When a fill area is specified (\p area), it must be a valid
    /// box (\ref display::Box::is_valid()).
    ///
    /// Call \ref present() to present the result.     
    ///
    virtual void fill(util::Color color) = 0;
    virtual void fill(util::Color color, const display::Box& area) = 0;
    /// \}

    /// \brief Create new texture of specific size.
    ///
    /// This function creates a new texture of the specified size. A texture is an array of
    /// pixels and can act as a source for efficient and repeated copying of pixels to the
    /// window (see \ref put_texture()).
    ///
    /// The application must ensure that the returned texture object is destroyed before
    /// this window is destroyed.
    ///
    /// The initial contents of the texture is undefined.
    ///
    /// The contents of the texture can be set using \ref display::Texture::put_image().
    ///
    /// \sa \ref display::Texture::put_image()
    /// \sa \ref put_texture()
    ///
    virtual auto new_texture(display::Size size) -> std::unique_ptr<display::Texture> = 0;

    /// \{
    ///
    /// \brief Copy pixels from texture to window.
    ///
    /// These functions copy pixels from the specified texture (\p tex) to this window. The
    /// overload that takes a source area argument (\p source_area) copies the pixels from
    /// that area of the texture. The source area must be confined to the texture
    /// boundary. The other overload copies the entire texture. The specified position (\p
    /// pos) is the upper-left corner of the target area in the window. The target area is
    /// allowed to extend beyond the boundaries of the window, or even fall entirely outside
    /// those boundaries.
    ///
    /// Call \ref present() to present the result.     
    ///
    /// The specified texture must be associated with the same display connection as this
    /// window, i.e., the texture and the window must have been created from the same
    /// connection object.
    ///
    virtual void put_texture(const display::Texture& tex, const display::Pos& pos = {}) = 0;
    virtual void put_texture(const display::Texture& tex, const display::Box& source_area,
                             const display::Pos& pos) = 0;
    /// \}

    /// \brief    
    ///
    ///    
    ///
    virtual void present() = 0;

    /// \brief Bind OpenGL context of this window to the calling thread.
    ///
    /// A window, that is configured for OpenGL rendering (\ref
    /// Config::enable_opengl_rendering), is associated with an OpenGL rendering
    /// context. This function binds the calling thread to that rendering context and to
    /// this window, such that subsequent OpenGL rendering performed by the calling thread
    /// is directed onto this window. On an X11 platform, this function corresponds to
    /// `glXMakeCurrent()`.
    ///
    /// The creation of a new window that is configured for OpenGL rendering may or may not
    /// clobber the current context association for the calling thread. Applications must
    /// assume that such clobbering happens.
    ///
    /// Behavior is undefined if this function is called on a window that is not configured
    /// for OpenGL rendering.
    ///
    virtual void opengl_make_current() = 0;

    /// \brief Exchange front and back buffers for OpenGL rendering.
    ///
    /// This function swaps front and back buffers for OpenGL rendering in this window. On
    /// an X11 platform, this corresponds to `glXSwapBuffers()`.
    ///
    /// This function may, or may not by default involve a wait for "vertical retrace"
    /// (V-Sync). See \ref try_set_opengl_vsync_state() for a general explanation of V-Sync
    /// and a way to change the default.
    ///
    /// Behavior is undefined if this function is called on a window that is not configured
    /// for OpenGL rendering (\ref Config::enable_opengl_rendering).
    ///
    /// \sa \ref try_set_opengl_vsync_state()
    ///
    virtual void opengl_swap_buffers() = 0;

    /// \brief Try to determine whether V-Sync is on or off.
    ///
    /// This function attempts to determine whether V-Sync is currently turned on or off for
    /// OpenGL rendering in this window. This function fails if the platform has no support
    /// for V-Sync control, or if it specifically lacks support for querying the current
    /// state of V-Sync. If it succeeds, it returns true after setting \p on to true if
    /// regular or adaptive V-Sync is enabled and to false otherwise. If it fails, it
    /// returns false and leaves \p on unchanged.
    ///
    /// \warning This function may clobber the calling thread's current OpenGL context
    /// binding (\ref opengl_make_current()). The application must assume that it does, and
    /// not rely on any particular binding upon return.
    ///
    /// For general notes about V-Sync, see \ref try_set_opengl_vsync_state().
    ///
    /// Behavior is undefined if this function is called on a window that is not configured
    /// for OpenGL rendering (\ref Config::enable_opengl_rendering).
    ///
    /// \sa \ref try_set_opengl_vsync_state()
    ///
    virtual bool try_get_opengl_vsync_state(bool& on) = 0;

    /// \brief Try to turn V-Sync on or off.
    ///
    /// If \p on is true, this function attempts to turn V-Sync on for OpenGL rendering in
    /// this window. If \p on is false, it attempts to turn V-Sync off. If \ref
    /// prefer_opengl_adaptive_vsync is true and if then adaptive V-Sync (see below) is
    /// available, it will be used. Otherwise regular V-Sync will be used. This function
    /// fails if the platform does not support V-Sync control. When it succeeds, it returns
    /// true. When it fails, it returns false.
    ///
    /// \warning This function may clobber the calling thread's current OpenGL context
    /// binding (\ref opengl_make_current()). The application must assume that it does, and
    /// not rely on any particular binding upon return.
    ///
    /// *Regular V-Sync* is when the swapping of front and back buffers (\ref
    /// opengl_swap_buffers()) is synchronized to the "vertical retrace", and ensures no
    /// "tearing". *Adaptive V-Sync* is like regular V-Sync but allows for immediate buffer
    /// swap if the vertical retrace is missed, i.e., when the rendering of one frame has
    /// taken more time that was available between two successive vertical retraces. Not all
    /// platforms support adaptive V-Sync.
    ///
    /// On X11-based platforms, such as Linux, and when using the proprietary NVIDIA
    /// graphics driver, one can also turn V-Sync on or off using the environment variable
    /// `__GL_SYNC_TO_VBLANK`.
    ///
    /// On some platforms, such as those featuring an X11 compositor, V-Sync, from the point
    /// of view of this function, is a matter between the application and the
    /// compositor. When V-Sync is turned on, buffer swaps are synchronized with the
    /// compositor's buffer swaps, which, in turn, are synchronized with the actual
    /// "hardware retrace". However, when V-Sync is turned off, buffer swaps can happen many
    /// times per hardware retrace, but only the last one will have any effect. The presence
    /// of the compositor will prevent such redundant buffer swaps from ever becoming
    /// visible.
    ///
    /// When using the X11-based implementation of the display API (\ref
    /// display::get_x11_implementation_slot()), one can request that the compositor, if
    /// present, is bypassed for fullscreen windows. See \ref
    /// display::x11_connection_config::fullscreen_opengl_bypass_compositor. When such a
    /// request is honored, V-Sync again becomes a matter between the application and the
    /// "hardware retrace".
    ///
    /// Behavior is undefined if this function is called on a window that is not configured
    /// for OpenGL rendering (\ref Config::enable_opengl_rendering).
    ///
    /// \sa \ref try_get_opengl_vsync_state()
    /// \sa \ref Config::opengl_vsync
    /// \sa \ref opengl_swap_buffers()
    ///
    virtual bool try_set_opengl_vsync_state(bool on) = 0;

    virtual ~Window() noexcept = default;
};


/// \brief Window configuration parameters.
///
/// These are the available parameters for configuring a window.
///
struct Window::Config {
    /// \brief Screen on which window must appear.
    ///
    /// If specified, that is, if the specified value is non-negative, this is the index of
    /// the screen on which the window must appear. It is an index into the list of screens
    /// accessible through the display connection on behalf of which the window is being
    /// created. See \ref display::Connection for general information about connections and
    /// screens. The number of screens is returned by \ref
    /// display::Connection::get_num_screens() and the index of the default screen is
    /// returned by \ref display::Connection::get_default_screen().
    ///
    /// When a screen is not specified, i.e., when the specified value is negative, the
    /// window will be opened on the default screen.
    ///
    int screen = -1;

    /// \brief Cookie value to be passed to window event handlers.
    ///
    /// The value specified here will be passed faithfully in \ref
    /// display::WindowEvent::cookie to event handlers that handle events from
    /// windows created using this configuration.
    ///
    /// \sa \ref set_event_handler()
    ///
    int cookie = 0;

    /// \brief Allow for window to be resized.
    ///
    /// If set to `true`, the window will be made resizable.
    ///
    bool resizable = false;

    /// \brief Start out in fullscreen mode.
    ///
    /// If set to `true`, the window will start out in fullscreen mode (see \ref
    /// set_fullscreen_mode()).
    ///
    bool fullscreen = false;

    /// \brief Start out in immersive mode.
    ///
    /// If set to `true`, the window will start out in immersive mode (see \ref
    /// set_immersive_mode()).
    ///
    bool immersive = false;

    /// \brief Enable OpenGL-based rendering.
    ///
    /// If set to `true`, the window will be configured to support OpenGL rendering. Such a
    /// window will be associated with a core profile OpenGL 4.1 context. The selected
    /// framebuffer configuration will be one that supports sRGB
    /// (`GLX_FRAMEBUFFER_SRGB_CAPABLE_ARB`).
    ///
    bool enable_opengl_rendering = false;

    /// \brief Whether OpenGL depth buffer is required.
    ///
    /// If set to `true` and if \ref enable_opengl_rendering is `true`, require that the
    /// window is created with an OpenGL depth buffer. If \p enable_opengl_rendering is
    /// `false`, this parameter has no effect.
    ///
    bool require_opengl_depth_buffer = true;

    /// \brief Prefer Adaptive V-Sync if V-Sync is turned on.
    ///
    /// Setting this flag to true does **not** by itself turn Adaptive V-Sync on. Instead,
    /// when V-Sync is turned on using \ref try_set_opengl_vsync_state(), and this flag is
    /// set to true, Adaptive V-Sync is turned on if it is supported by the platform. If
    /// Adaptive V-Sync is not supported by the platform, but regular V-Sync is, then
    /// regular V-Sync is turned on by \ref try_set_opengl_vsync_state() even when this flag
    /// is set to true. See \ref try_set_opengl_vsync_state() for more on V-Sync in general
    /// and the how Adaptive V-Sync differs from regular V-Sync.
    ///
    /// \sa \ref try_set_opengl_vsync_state()
    /// \sa \ref opengl_vsync
    ///
    bool prefer_opengl_adaptive_vsync = false;

    /// \brief Minimum size of window
    ///
    /// If the specified size of a new window is smaller than the minimum size specified
    /// here, the effective size will be the minimum size. Otherwise it will be the
    /// specified size. This applies separately in each direction, horizontally and
    /// vertically.
    ///
    /// Additionally, if the new window is made resizable (\ref resizable), it becomes
    /// impossible to interactively make the window smaller than the minimum size specified
    /// here.
    ///
    display::Size minimum_size = 16;

    /// \brief Set state of OpenGL V-Sync.
    ///
    /// If specified, and if \ref enable_opengl_rendering is true, an attempt will be made,
    /// during the creation of the window, to turn OpenGL V-Sync on or off as if by an
    /// invocation of \ref try_set_opengl_vsync_state(). If not specified, or if the
    /// invocation of \ref try_set_opengl_vsync_state() fails, V-Sync will be left in its
    /// default state. The default state depends on the platform.
    ///
    /// \sa \ref try_set_opengl_vsync_state()
    /// \sa \ref prefer_opengl_adaptive_vsync
    ///
    std::optional<bool> opengl_vsync;
};


} // namespace archon::display

#endif // ARCHON_X_DISPLAY_X_WINDOW_HPP
