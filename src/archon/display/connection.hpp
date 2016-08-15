/*
 * This file is part of the Archon library framework.
 *
 * Copyright (C) 2012  Kristian Spangsege <kristian.spangsege@gmail.com>
 *
 * The Archon library framework is free software: You can redistribute
 * it and/or modify it under the terms of the GNU Lesser General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * The Archon library framework is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the Archon library framework.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/// \file
///
/// \author Kristian Spangsege

#ifndef ARCHON_DISPLAY_CONNECTION_HPP
#define ARCHON_DISPLAY_CONNECTION_HPP

#include <exception>

#include <archon/core/unique_ptr.hpp>
#include <archon/image/image.hpp>
#include <archon/display/window.hpp>
#include <archon/display/pixel_buffer.hpp>
#include <archon/display/context.hpp>
#include <archon/display/event.hpp>


namespace archon {
namespace Display {

/// Thrown if no visual is avaialble that matches your requirements.
///
/// \sa Connection::choose_visual
class NoSuchVisualException;

/// Thrown by various methods to indicate that OpenGL support is not availble
/// through the used connection.
///
/// \sa Connection::has_gl_support.
class NoGLException;


/// A connection to some display. A display is an arbitrary but fixed collection
/// of physical screens, each identified by an index.
///
/// Each screen makes a fixed number of 'visuals' available. A visual is more or
/// less the same as a video mode. Each visual of a screen is identified by an
/// index.
///
/// On the X Window System the Connection class is a wrapper around the Display
/// structure of Xlib. The notions of screen and visual correpond with those of
/// the X Window System.
///
/// \sa http://www.x.org
class Connection {
public:
    typedef core::SharedPtr<Connection> Ptr;
    typedef const Ptr& Arg;

    /// Get the index of the default screen of the connected display.
    ///
    /// \return The index of the default screen.
    ///
    /// \note This method is thread-safe.
    virtual int get_default_screen() const = 0;

    /// Get the index of the default visual of the specified screen.
    ///
    /// \param screen The index of the screen to be enquired. A negative value
    /// will be interpreted as the default screen of this connection.
    ///
    /// \return The index of the default visual.
    ///
    /// \note This method is thread-safe.
    virtual int get_default_visual(int screen = -1) const = 0;

    /// Create a new window to be displayed on the specified screen and with a
    /// pixel buffers configuration as described by the specified visual. The
    /// window is not displayed until you call the \c show method of the
    /// returned window object.
    ///
    /// If you specify a visual with OpenGL support, then this window will allow
    /// OpenGL rendering. Use \c choose_gl_visual to select an appropriate
    /// visual. OpenGL capable visuals are available if, and only if the \c
    /// has_gl_support method returns true for this connection.
    ///
    /// \param width, height Initial width and height in pixels of the windows
    /// contents area.
    ///
    /// \param screen The index of the screen on which the window must be
    /// created. A negative value means that the default screen of this
    /// connection must be used.
    ///
    /// \param visual The index of the visual that describes which pixel buffer
    /// must be available in the new window. This index refers to the visuals
    /// available on the specified screen. A negative value means that the
    /// default visual of the specified screen must be used.
    ///
    /// \note This method is thread-safe.
    ///
    /// \sa Window
    virtual Window::Ptr new_window(int width, int height, int screen = -1, int visual = -1) = 0;

    /// Create a new off-screen pixel buffer which can be used as a target for
    /// OpenGL rendering.
    ///
    /// \param screen The index of the screen to which the new pixel buffer must
    /// be tied. A negative value means that the default screen of this
    /// connection is to be used.
    ///
    /// \param visual The index of the visual to use with the new pixel
    /// buffer. It must be the same visual as the context to which it eventually
    /// will be bound. This index refers to the visuals available on the
    /// specified screen. A negative value means that the default visual of the
    /// specified screen must be used.
    ///
    /// \throw NoGLException If the OpenGL is not supported by this display
    /// connection.
    ///
    /// \note This method is thread-safe.
    virtual PixelBuffer::Ptr new_pixel_buffer(int width, int height, int screen = -1,
                                              int visual = -1) = 0;

    /// Check whether this display connection has support for OpenGL rendering.
    ///
    /// With the X11 Library as the backend, OpenGL support requires both the
    /// client and the server side to support the GLX extension.
    ///
    /// \return True iff OpenGL is supported through this display connection.
    virtual bool has_gl_support() const = 0;

    /// Choose an OpenGL capable visual based on the specified minimum
    /// requirements.
    ///
    /// On the X Window System this is a wrapper around the glXChooseVisual
    /// function of Xlib.
    ///
    /// \param screen The index of the screen to be inquired. A negative value
    /// will be interpreted as the default screen of this connection.
    ///
    /// \param double_buffer Set to true if you require a double-buffered
    /// visual. In this case you should call to \c Window::swap_buffers after
    /// rendering each frame.
    ///
    /// \param stereo Set this one to 'true' if you require a quad-buffered (or
    /// stereoscopic) visual. In this case, use \c glDrawBuffer to switch
    /// between right and left buffers. In single-buffered stereoscopic mode you
    /// would alternate between \c GL_LEFT and \c GL_RIGHT, and in
    /// doublebuffered stereoscopic mode you would alternate between \c
    /// GL_BACK_LEFT and \c GL_BACK_RIGHT. Note also, that this mode is only
    /// available on special hardware with the stereoscopic capability.
    ///
    /// \param red The minimum number of bits in the red buffer. If this value
    /// is zero, the smallest available red buffer is preferred. Otherwise, the
    /// largest available red buffer of at least the specified size is
    /// preferred. After the visual is created, you may call the \c get_red
    /// method to retrieve the actual bit-width of the red buffer that was
    /// chosen.
    ///
    /// \param green, blue, alpha, depth Just like parameter 'red'.
    ///
    /// \param stencil The minimum number of bits in the stencil buffer. The
    /// smallest available stencil buffer of at least the specified size is
    /// preferred. After the visual is created, you may call the \c get_stencil
    /// method to retrieve the actual bit-width of the red buffer that was
    /// chosen.
    ///
    /// \param accum_red, accum_green, accum_blue, accum_alpha Just like
    /// parameter 'red'.
    ///
    /// \return The index of the chosen visual.
    ///
    /// \throw NoGLException If OpenGL is not available through this display
    /// connection.
    ///
    /// \throw NoSuchVisualException If one or more of the parameters could not
    /// be satisfied.
    ///
    /// \note This method is thread-safe.
    virtual int choose_gl_visual(int screen = -1,
                                 bool double_buffer = true,
                                 bool stereo = false,
                                 int red = 8, int green = 8,
                                 int blue = 8, int alpha = 8,
                                 int depth = 16, int stencil = 0,
                                 int accum_red = 0, int accum_green = 0,
                                 int accum_blue = 0, int accum_alpha = 0) const = 0;

    /// Create a new OpenGL rendering context. If the display connection does
    /// not support OpenGL, this method will throw <tt>NoGLException</tt>.
    ///
    /// On the X Window System this is a wrapper around the \c glXCreateContext
    /// call, thus, the OpenGL extesion to X (GLX) must be available.
    ///
    /// A rendering context must be bound to a window (or other drawable) before
    /// any OpenGL rendering can take place. A rendering context can only be
    /// bound to a window if both specify the same screen and visual.
    ///
    /// \param screen The index of the screen that this context must be
    /// associated with. A negative value means that the default screen of this
    /// connection must be used. The new context can be bound only to windows
    /// created on this screen.
    ///
    /// \param visual The index of the visual that this context must be
    /// associated with. This index refers to the visuals available on the
    /// specified screen. A negative value means that the default visual must be
    /// used. The new context can be bound only to windows created with respect
    /// to this visual.
    ///
    /// \param direct If true a direct rendering context will be used if
    /// available. Direct rendering contexts offers better performance but
    /// generally works only on a display of the local host. After the Context
    /// is created you may call \c Context::is_direct to determine if a direct
    /// rendering context was in fact aquired.
    ///
    /// \param share_with Another context with which display lists and texture
    /// objects must be shared. Note however, that the default texture object
    /// (0) is never shared between contexts, but any texture object created
    /// using glGenTextures() will be shared.
    ///
    /// \throw invalid_argument If the context, specified for
    /// <tt>share_with</tt>, belongs to a different implementation that this
    /// context.
    ///
    /// \throw NoGLException If the OpenGL is not supported by this display
    /// connection.
    ///
    /// \note This method is thread-safe.
    ///
    /// \sa Context
    /// \sa http://www.opengl.org/documentation/specs/#glx
    virtual Context::Ptr new_gl_context(int screen = -1, int visual = -1, bool direct = true,
                                        Context::Arg share_with = Context::Ptr()) = 0;

    /// Create a new event processor capable of relaying events from one or more
    /// windows to the specified event handler.
    ///
    /// NOTE: The ownership of the specified event handler remains with the
    /// caller, and it is the callers responsibility that the very last call to
    /// \c EventProcessor::process has returned before the event handler is
    /// destroyed.
    ///
    /// \note Although it is valid to use a single event handler instance with
    /// multiple event processors, it is probaly not a good idea. The reason is
    /// that each processor typically will be run by distinct threads, thus you
    /// now have to provide a completely re-entrant and tread-safe event
    /// handler.
    ///
    /// \param handler Your event handler.
    ///
    /// \return The newly created event processor. The ownership of this object
    /// is passed back to the caller.
    ///
    /// \note This method is thread-safe.
    ///
    /// \sa EventProcessor
    /// \sa EventHandler
    virtual EventProcessor::Ptr new_event_processor(EventHandler* handler) = 0;

    /// Create a new mouse cursor from the specified image. The resulting cursor
    /// object can be applied to a window by calling
    /// <tt>Window::set_cursor</tt>. A single cursor object may be used with
    /// several windows at the same time.
    ///
    /// If the specified image cannot be used directly as a mouse cursor image
    /// due to limitations of the underlying platform, it will be transformed in
    /// such a way as to preserve as much as possible of the original.
    ///
    /// The alpha channel of the specified image will be used as a mask such
    /// that the contents of the window shows through where the alpha component
    /// is not at full intencity. If you image has no alpha channel, the
    /// resulting mouse cursor will appear as a solid block with a size equal to
    /// the specified image.
    ///
    /// When using the X11 backend, full-color mouse cursors are supported only
    /// when the X Render Extension is available.
    ///
    /// \param image The image whose contents should be used to create the
    /// cursor.
    ///
    /// \param hotspot_x, hotspot_y The position of the hotspot within the
    /// specified image measured in pixels from the upper left corner. The
    /// hotspot is the point that determinaes the exact location of the mouse,
    /// for example, the point of an arrow.
    ///
    /// \param screen The index of the screen that this cursor must be
    /// associated with. A negative value will be interpreted as the default
    /// screen of this connection. The new cursor can only be used with windows
    /// created on this screen.
    ///
    /// \return The newly created cursor.
    ///
    /// \note This method is thread-safe.
    ///
    /// \sa Window::set_cursor
    ///
    /// \todo FIXME: Is it really reauired that a mouse be associated with a
    /// specific screen? This has to ve verified. If, not, there is no reason to
    /// pass a screen argument to this method.
    virtual Cursor::Ptr new_cursor(image::Image::Ref image,
                                   int hotspot_x = 0, int hotspot_y = 0,
                                   int screen = -1) = 0;

    /// Flush the output queue.
    ///
    /// In a single threaded application that runs the event processer, there is
    /// most likely no reason to call this method ever. However, if your
    /// application is multi-threaded and one thread runs the event processer,
    /// then as a rule of thumb, each of the other threads must manually flush
    /// the output queue after each chunk of window manipulation.
    ///
    /// \note This method is thread-safe.
    virtual void flush_output() = 0;

    /// Get the number of screens associated with the connected display. Each
    /// screen may have different properties.
    ///
    /// \return The number of available screens.
    ///
    /// \note This method is thread-safe.
    virtual int get_num_screens() const = 0;

    /// Get the horizontal size in pixels of the specified screen.
    ///
    /// \param screen The index of the screen to be enquired. A negative value
    /// will be interpreted as the default screen of this connection.
    ///
    /// \return The width of this screen in pixels.
    ///
    /// \note This method is thread-safe.
    virtual int get_screen_width(int screen = -1) const = 0;

    /// Get the vertical size in pixels of the specified screen.
    ///
    /// \param screen The index of the screen to be enquired. A negative value
    /// will be interpreted as the default screen of this connection.
    ///
    /// \return The height of this screen in pixels.
    ///
    /// \note This method is thread-safe.
    virtual int get_screen_height(int screen = -1) const = 0;

    /// Get the horizontal distance between pixels on the specified screen.
    ///
    /// \param screen The index of the screen to be enquired. A negative value
    /// will be interpreted as the default screen of this connection.
    ///
    /// \return The horizontal distance between pixels in meters.
    ///
    /// \note This method is thread-safe.
    virtual double get_horiz_dot_pitch(int screen = -1) const = 0;

    /// Get the vertical distance between pixels on the specified screen.
    ///
    /// \param screen The index of the screen to be enquired. A negative value
    /// will be interpreted as the default screen of this connection.
    ///
    /// \return The vertical distance between pixels in meters.
    ///
    /// \note This method is thread-safe.
    virtual double get_vert_dot_pitch(int screen = -1) const = 0;

    /// Get the number of visuals associated with the specified screen.
    ///
    /// \param screen The index of the screen to be enquired. A negative value
    /// will be interpreted as the default screen of this connection.
    ///
    /// \return The number of available visuals.
    ///
    /// \note This method is thread-safe.
    virtual int get_num_visuals(int screen = -1) const = 0;

    enum BufferType {
        buf_red, buf_green, buf_blue,
        buf_alpha, buf_depth, buf_stencil,
        buf_accum_red, buf_accum_green, buf_accum_blue,
        buf_accum_alpha
    };

    /// Get the number of bits per pixel in the specified buffer of the
    /// specified OpenGL capable visual of the specified screen.
    ///
    /// \param t The buffer whose bit-width you are enquiring about.
    ///
    /// \param screen The index of the screen to be enquired. A negative value
    /// will be interpreted as the default screen of this connection.
    ///
    /// \param visual The index of the visual to be enquired. This index refers
    /// to the list of visuals provided by the specified screen. A negative
    /// value will be interpreted as the default visual that screen.
    ///
    /// \return The bit-width of the specified buffer.
    ///
    /// \throw NoGLException If the OpenGL is not supported by this display
    /// connection.
    ///
    /// \note This method is thread-safe.
    virtual int get_gl_buf_width(BufferType t, int screen = -1, int visual = -1) const = 0;

    virtual ~Connection() {}
};




// Implementation

class NoSuchVisualException: public std::exception {
public:
    const char* what() const noexcept override
    {
        return "NoSuchVisualException";
    }
};

class NoGLException: public std::exception {
public:
    const char* what() const noexcept override
    {
        return "OpenGL is not availble";
    }
};

} // namespace Display
} // namespace archon

#endif // ARCHON_DISPLAY_CONNECTION_HPP
