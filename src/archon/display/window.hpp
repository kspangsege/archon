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

/**
 * \file
 *
 * \author Kristian Spangsege
 */

#ifndef ARCHON_DISPLAY_WINDOW_HPP
#define ARCHON_DISPLAY_WINDOW_HPP

#include <string>

#include <archon/display/drawable.hpp>


namespace archon
{
  namespace display
  {
    /**
     * Representation of a cursor image. Call \c Window::set_cursor to
     * request that a specific cursor image be used when the mouse is
     * inside that window. New instances of this class are created
     * with <tt>Connection::new_cursor</tt>.
     *
     * \sa Connection::new_cursor
     * \sa Window::set_cursor
     */
    struct Cursor
    {
      typedef core::SharedPtr<Cursor> Ptr;
      typedef Ptr const &Arg;
      virtual ~Cursor() {}
    };



    /**
     * A window whose main purpose is OpenGL rendering.
     *
     * On the X Window System this is a wrapper around the Window
     * structure of Xlib.
     *
     * New window objects are created by calling
     * Connection::new_window().
     *
     * Note that a window is not shown immediately when created. It
     * remains invisible until its \c show method is called. This is
     * to allow you to configure it before it becomes visible.
     *
     * A window is automatically closed when the last reference to the
     * object is dropped.
     *
     * \sa Connection::new_window
     * \sa http://www.opengl.org
     * \sa http://www.mesa3d.org
     * \sa http://www.x.org
     */
    struct Window: virtual Drawable
    {
      typedef core::SharedPtr<Window> Ptr;
      typedef Ptr const &Arg;

      /**
       * Make it such that this window becomes visible on the selected
       * display.
       *
       * Calling this method for a window that is already in the
       * visible state, has no effect.
       *
       * \note This method is thread-safe.
       */
      virtual void show() = 0;


      /**
       * Remove this window from the display without destroying it. It
       * merely becomes invisible, and can be brought back any time
       * with the \c show method.
       *
       * Calling this method for a window that is not currently in the
       * visible state, has no effect.
       *
       * \note This method is thread-safe.
       */
      virtual void hide() = 0;


      /**
       * Sets a new title in the title bar of this window. The default
       * title is empty.
       *
       * \param title New title.
       *
       * \note This method is thread-safe.
       *
       * \todo FIXME: Is this specified using a UTF-8 encoded string?
       */
      virtual void set_title(std::string title) = 0;


      /**
       * Set a new position for this window. This has no effect unless
       * the window is in the visible state. If you wish to position
       * the window right after calling the \c show method of the
       * window, you must call the \c flush_output method of the
       * display connection between the call to \c open and the call
       * to <tt>set_position</tt>.
       *
       * The initial position of the window (after the first call to
       * <tt>show</tt>) is determined by the window manager.
       *
       * \param left Initial horizontal displacement of the left
       * outside edge of the window measured from the left edge of the
       * screen. Increasing this value moves the window further to the
       * right.
       *
       * \param top Initial vertical displacement of upper outside
       * edge of the window measured from the top edge of the
       * screen. Increasing this value moves the window further down.
       *
       * \note This method is thread-safe.
       */
      virtual void set_position(int left, int top) = 0;


      /**
       * Resize this window.
       *
       * This generates a resize event.
       *
       * \param width, height New width and height in pixels of the
       * windows contents area.
       *
       * \note This method is thread-safe.
       */
      virtual void set_size(int width, int height) = 0;


      /**
       * Set the background color of this window.
       *
       * The color is specified in the RGB color space with 8 bits per
       * channel, thus, each channel has a value between 0 and 255. It
       * is specified as a single 24 bit value on the form 0xRRGGBB,
       * that is the blue channel occupies the 8 least significant
       * bits.
       *
       * \todo FIXME: Is this specified in device dependent
       * intensities, or does it refer to the sRGB color space?
       */
      virtual void set_bg_color(long rgb) = 0;


      /**
       * Request that the specified mouse cursor be used when the
       * mouse is inside this window.
       *
       * New cursors are created with
       * <tt>Connection::new_cursor</tt>. Cursors are created for a
       * specific screen, and can only be used with windows on that
       * screen.
       *
       * The specified cursor must belong to the same implementation
       * as this window does. Further more, both must have been
       * created through the same display connection. \c
       * std::invalid_argument will be thrown if this is not the case.
       *
       * \param cursor The cursor to use for this window.
       *
       * \note This method is thread-safe.
       *
       * \sa Connection::new_cursor
       */
      virtual void set_cursor(Cursor::Arg cursor) = 0;


      /**
       * Enable or disable fullscreen mode.
       *
       * \note This method is thread-safe.
       */
      virtual void set_fullscreen_enabled(bool enable = true) = 0;


      /**
       * Get the current position of this window.
       *
       * \return A pair \c x,y reflecting the current position of the
       * upper left corner of the window frame.
       *
       * \note This method is thread-safe.
       */
      virtual std::pair<int, int> get_position() const = 0;


      /**
       * Get the current width and height of this window.
       *
       * \return A pair \c width,height reflecting the current size of
       * contents area of the window.
       *
       * \note This method is thread-safe.
       */
      virtual std::pair<int, int> get_size() const = 0;


      /**
       * Enable or disable reporting of mouse move events regardless
       * of whether a mouse button is pressed or not. By default mouse
       * move events are only reported while at least one mouse button
       * is pressed. The reason for not always reporting these events,
       * is that, due to the high volume, it may waste significant
       * processing power when they are not needed.
       *
       * \note This method is thread-safe.
       */
      virtual void report_mouse_move(bool enable = true) = 0;


      /**
       * Swap foreground and background buffers for windows that are
       * double buffered. A window is double buffered if a visual with
       * double buffering was specified at window creation
       * time. Generally, only visuals with OpenGL support can have
       * double buffering. If the window is not double buffered, this
       * method has no effect.
       *
       * If the window is double buffered and has OpenGL support, this
       * method performs an implicit <tt>glFlush</tt>.
       *
       * \note This method is thread-safe.
       */
      virtual void swap_buffers() = 0;
    };
  }
}

#endif // ARCHON_DISPLAY_WINDOW_HPP
