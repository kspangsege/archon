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

#ifndef ARCHON_X_DISPLAY_X_EVENT_HANDLER_HPP
#define ARCHON_X_DISPLAY_X_EVENT_HANDLER_HPP

/// \file


#include <archon/display/event.hpp>


namespace archon::display {


/// \brief Handle window-specific events.
///
/// In order to handle window-specific events, the application must override the relevant
/// handler functions in a subclass, and then pass an instance of that subclass to \ref
/// display::Connection::new_window(). Thereby, a window becomes associated with the passed
/// window event handler.
///
/// The individual event handler functions will be called by the event processor, and more
/// specifically, by the thread that calls \ref display::Connection::process_events().
///
/// If any of the event handler functions return `false`, event processing will be
/// interrupted. See \ref display::Connection::process_events() for more on interruption of
/// event handling.
///
/// \sa \ref display::ConnectionEventHandler
///
class WindowEventHandler {
public:
    /// \brief A key was pressed down.
    ///
    /// This function is called when a "key down" event is generated for one of the windows
    /// that are associated with the event handler. A "key down" event is generated for a
    /// particular window when a key is pressed down while that window is active (has
    /// keyboard focus).
    ///
    /// The default implementation of this function does nothing other than return `true`.
    ///
    virtual bool on_keydown(const display::KeyEvent&);

    /// \brief A key was released.
    ///
    /// This function is called when a "key up" event is generated for one of the windows
    /// that are associated with the event handler. A "key up" event is generated for a
    /// particular window when a key is released while that window is active (has keyboard
    /// focus).
    ///
    /// The default implementation of this function does nothing other than return `true`.
    ///
    /// FIXME: Clarify what happens if a window looses keyboard focus while a key is pressed down                                
    ///
    /// FIXME: Clarify what happens if a window receives keyboard focus while a key is pressed down                                
    ///
    virtual bool on_keyup(const display::KeyEvent&);

    /// \brief A mouse button was pressed down.
    ///
    /// This function is called when a "mouse down" event is generated for one of the
    /// windows that are associated with the event handler. A "mouse down" event is
    /// generated for a particular window when a mouse button is pressed down while that
    /// window is active (has keyboard focus).
    ///
    /// The default implementation of this function does nothing other than return `true`.
    ///
    virtual bool on_mousedown(const display::MouseButtonEvent&);

    /// \brief A mouse button was released.
    ///
    /// This function is called when a "mouse up" event is generated for one of the windows
    /// that are associated with the event handler. A "mouse up" event is generated for a
    /// particular window when a mouse button is released while that window is active (has
    /// keyboard focus).
    ///
    /// The default implementation of this function does nothing other than return `true`.
    ///
    /// FIXME: Clarify what happens if a window looses keyboard focus while a mouse button is pressed down                                
    ///
    /// FIXME: Clarify what happens if a window receives keyboard focus while a mouse button is pressed down                                
    ///
    virtual bool on_mouseup(const display::MouseButtonEvent&);

    /// \brief The mouse moved over a window or while mouse button was pressed.
    ///
    /// This function is called when a "mouse move" event is generated for one of the
    /// windows that are associated with the event handler. A "mouse move" event is
    /// generated for a particular window when the mouse moves while a mouse button is held
    /// down and that mouse button was pressed down while the window was active (had
    /// keyboard focus).
    ///
    /// The default implementation of this function does nothing other than return `true`.
    ///
    virtual bool on_mousemove(const display::MouseEvent&);

    /// \brief The scroll wheel was moved.
    ///
    /// This function is called when a "scroll" event is generated for one of the windows
    /// that are associated with the event handler. A "scroll" event is generated for a
    /// particular window when the scroll wheel is moved while that window is active (has
    /// keyboard focus).
    ///
    /// With a scroll wheel that turns in discrete steps (detents), this function is
    /// normally called once per step.
    ///
    /// The default implementation of this function does nothing other than return `true`.
    ///
    virtual bool on_scroll(const display::ScrollEvent&);

    /// \brief Mouse enters window.
    ///
    /// This function is called when a "mouse over" event is generated for one of the
    /// windows that are associated with the event handler. A "mouse over" event is
    /// generated for a particular window when the mouse enters the area of that window.
    ///
    /// The default implementation of this function does nothing other than return `true`.
    ///
    virtual bool on_mouseover(const display::TimedWindowEvent&);

    /// \brief Mouse leaves window.
    ///
    /// This function is called when a "mouse out" event is generated for one of the windows
    /// that are associated with the event handler. A "mouse out" event is generated for a
    /// particular window when the mouse leaves the area of that window.
    ///
    /// The default implementation of this function does nothing other than return `true`.
    ///
    virtual bool on_mouseout(const display::TimedWindowEvent&);

    /// \brief A window gained focus.
    ///
    /// This function is called when a "focus" event is generated for one of the windows
    /// that are associated with the event handler. A "focus" event is generated for a
    /// particular window when that window becomes the active window (when it gains keyboard
    /// focus).
    ///
    /// The default implementation of this function does nothing other than return `true`.
    ///
    virtual bool on_focus(const display::TimedWindowEvent&);

    /// \brief A window lost focus.
    ///
    /// This function is called when a "blur" event is generated for one of the windows that
    /// are associated with the event handler. A "blur" event is generated for a particular
    /// window when that window ceases to be the active window (when it looses keyboard
    /// focus).
    ///
    /// The default implementation of this function does nothing other than return `true`.
    ///
    virtual bool on_blur(const display::TimedWindowEvent&);

    /// \brief A window was exposed and needs to be redrawn.
    ///
    /// This function is called when an "expose" event is generated for one of the windows
    /// that are associated with the event handler. An "expose" event is generated for a
    /// particular window when that window is exposed in such a way that its contents need
    /// to be redrawn.
    ///
    /// The default implementation of this function does nothing other than return `true`.
    ///
    virtual bool on_expose(const display::WindowEvent&);

    /// \brief The size of a window changed.
    ///
    /// This function is called when a "resize" event is generated for one of the windows
    /// that are associated with the event handler. A "resize" event is generated for a
    /// particular window when that window is resized either interactively by a user of a
    /// graphical user interface or programmatically through use of \ref
    /// display::Window::set_size() or \ref display::Window::set_fullscreen_mode().
    ///
    /// The default implementation of this function does nothing other than return `true`.
    ///
    virtual bool on_resize(const display::WindowSizeEvent&);

    /// \brief Request to close a window.
    ///
    /// This function is called when the "close" event is generated for one of the windows
    /// that are associated with the event handler. The "close" event is generated for a
    /// particular window when a user of a graphical user interface requests the closure of
    /// the window. Usually, this means that the user has clicked on the 'close' symbol of
    /// the window.
    ///
    /// In general, when a request is made to close the last window, a "quit" event is also
    /// generated (see \ref display::ConnectionEventHandler::on_quit()).
    ///
    /// If the application ignores a "close" event, it will be generated again as a result
    /// of a subsequent request to close the window.
    ///
    /// The default implementation of this function does nothing other than return `true`.
    ///
    virtual bool on_close(const display::TimedWindowEvent&);

    virtual ~WindowEventHandler() noexcept = default;
};



/// \brief Handle connection-level events.
///
/// In order to handle connection-level events, the application must override the relevant
/// handler functions in a subclass, and then pass an instance of that subclass to \ref
/// display::Connection::process_events().
///
/// The individual event handler functions will be called by the event processor, and more
/// specifically, by the thread that calls \ref display::Connection::process_events().
///
/// If any of the event handler functions return `false`, event processing will be
/// interrupted. See \ref display::Connection::process_events() for more on interruption of
/// event handling.
///
/// \sa \ref display::WindowEventHandler
///
class ConnectionEventHandler {
public:
    /// \brief A display configuration changed.
    ///
    /// This function is called when a "display change" event is generated provided that the
    /// display implementation exposes information about display configurations. For more on
    /// this, see \ref display::Connection::try_get_display_conf(). The \p display argument
    /// specifies the index of the display whose configuration changed.
    ///
    virtual bool on_display_change(int display);

    /// \brief Opportunity to interrupt event processing before sleep.
    ///
    /// This function is called right before the event processor goes to sleep while waiting
    /// for more events to be generated. This function is intended as an opportunity for the
    /// application to interrupt event processing by returning `false` when appropriate, but
    /// only after processing all the currently queued up events.
    ///
    /// The default implementation of this function does nothing other than return `true`.
    ///
    virtual bool before_sleep();

    /// \brief Request to quit application.
    ///
    /// This function is called when the "quit" event occurs.
    ///
    /// The "quit" event is a request to close the application, and is generated in various
    /// situations. It is generated when there is a request to close a window, and that
    /// window is the only window that is currently open by the application. In this case,
    /// both the "close" and the "quit" events are generated (see \ref
    /// display::WindowEventHandler::on_close()). The "quit" is also generated if the last
    /// window is closed programmatically by the application, i.e., through destruction of
    /// the last \ref display::Window object.
    ///
    /// If the application ignores the "quit" event (by returning `true`), the "quit" will
    /// be generated again as a result of a subsequent request to quit the application.
    ///
    /// The default implementation of this function does nothing other than return
    /// `false`. This will cause event processing to be interrupted.
    ///
    virtual bool on_quit();

    virtual ~ConnectionEventHandler() noexcept = default;
};


} // namespace archon::display

#endif // ARCHON_X_DISPLAY_X_EVENT_HANDLER_HPP
