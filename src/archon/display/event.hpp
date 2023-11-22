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

#ifndef ARCHON_X_DISPLAY_X_EVENT_HPP
#define ARCHON_X_DISPLAY_X_EVENT_HPP

/// \file


#include <chrono>

#include <archon/math/vector.hpp>
#include <archon/display/geometry.hpp>
#include <archon/display/key_code.hpp>
#include <archon/display/mouse_button.hpp>


namespace archon::display {


/// \brief Common base for all window-specific events.
///
/// This is a common base for all window-specific types of events.
///
/// \sa \ref display::TimedWindowEvent
///
struct WindowEvent {
    /// \brief General purpose context value useful as window identifier.
    ///
    /// This general purpose field can be used to identify the window from which the event
    /// originated. It will carry the value that was specified when the origin window was
    /// created (\ref display::Window::Config::cookie). The display library will not
    /// interpret this value in any way at all.
    ///
    int cookie;
};



/// \brief Common base for events carrying timestamps.
///
/// This is a common base for those types of events that carry a timestamp. Examples are
/// \ref display::KeyEvent and \ref display::MouseEvent.
///
struct TimedWindowEvent : display::WindowEvent {
    /// \brief Type used for timestamps.
    ///
    /// This is the type used for timestamps (\ref timestamp).
    ///
    using Timestamp = std::chrono::milliseconds;

    /// \brief Time of event.
    ///
    /// The point in time where the event occurred relative to a fixed, but arbitrary
    /// origin. Note that the origin is not necessarily, and most likely not equal to the
    /// beginning of the UNIX Epoch. For this reason, these timestamps can only be used for
    /// measuring time between events. All timestamps will be non-negative.
    ///
    Timestamp timestamp;
};



/// \brief When keyboard key is pressed or released.
///
/// This type of event is generated whenever a key on the keyboard is pressed down or
/// released. It is generated in the context of the window that is in focus when the event
/// occurs.
///
/// \sa \ref display::WindowEventHandler::on_keydown()
/// \sa \ref display::WindowEventHandler::on_keyup()
///
struct KeyEvent : display::TimedWindowEvent {
    /// \brief The key that was pressed or released.
    ///
    /// This key code identifies the key that was pressed or released.
    ///
    display::KeyCode key_code;
};



/// \brief When mouse moves or mouse button is pressed or released.
///
/// This type of event is generated both when the mouse moves and when a mouse button is
/// pressed or released. In the case of button activity, the actual event type will be \ref
/// display::MouseButtonEvent.
///
/// \sa \ref display::WindowEventHandler::on_mousemove()
/// \sa \ref display::MouseButtonEvent
///
struct MouseEvent : display::TimedWindowEvent {
    /// \brief Position of mouse.
    ///
    /// This is the position of the mouse at the time that the event was generated. The
    /// position is relative to the top-left corner of the screen.
    ///
    display::Pos pos;
};



/// \brief When mouse button is pressed or released.
///
/// This type of event is generated both when a mouse button is pressed or released.
///
/// \sa \ref display::WindowEventHandler::on_mousedown()
/// \sa \ref display::WindowEventHandler::on_mouseup()
///
struct MouseButtonEvent : display::MouseEvent {
    /// \brief Concerned mouse button.
    ///
    /// This value specifies which mouse button the event concerns.
    ///
    display::MouseButton button;
};



/// \brief When scroll wheel is moved.
///
/// This type of event is generated when the scroll wheel is moved.
///
/// \sa \ref display::WindowEventHandler::on_scroll()
///
struct ScrollEvent : display::TimedWindowEvent {
    /// \brief Amount of scroll wheel motion.
    ///
    /// This is the amount of the motion of the scroll wheel. A positive Y-coordinate
    /// corresponds to an upwards scroll, i.e., towards the top of the scrolled
    /// medium. Likewise, a positive X-coordinate corresponds to a rightwards scroll, i.e.,
    /// towards the right side the scrolled medium.
    ///
    /// When the scroll wheel turns in discrete steps (detents), the unit of motion is
    /// generally one such step. When the scroll wheel turns freely, the unit is generally
    /// chosen to match that of a wheel that turns in discrete steps.
    ///
    math::Vector2F amount;
};



/// \brief When a window is resized.
///
/// This type of event is generated when a window changes size.
///
/// \sa \ref display::WindowEventHandler::on_resize()
///
struct WindowSizeEvent : display::WindowEvent {
    /// \brief Size of window.
    ///
    /// This is the size of the contents area of the window at the time the event was
    /// generated.
    ///
    display::Size size;
};



/// \brief When a window is repositioned.
///
/// This type of event is generated when a window changes position.
///
/// \sa \ref display::WindowEventHandler::on_reposition()
///
struct WindowPosEvent : display::WindowEvent {
    /// \brief Position of window.
    ///
    /// This is the position of the upper-left corner of the contents area of the window at
    /// the time the event was generated.
    ///
    display::Pos pos;
};


} // namespace archon::display

#endif // ARCHON_X_DISPLAY_X_EVENT_HPP
