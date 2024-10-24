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
/// In order to handle window-specific events, the application must instantiate a
/// window-level event handler that overrides the relevant event handler functions, and then
/// call \ref display::Window::set_event_handler() to register that event event handler with
/// a window.
///
/// The individual event handler functions will be called by the event processor, and more
/// specifically, by the thread that calls \ref display::Connection::process_events() or
/// \ref display::Connection::process_events_a().
///
/// If any of the event handler functions return `false`, event processing will be
/// interrupted. See \ref display::Connection::process_events_a() for more on interruption
/// of event handling.
///
/// With the exception of \ref on_expose(), event handlers should generally execute fast,
/// which means that they should do very little work. See \ref
/// display::Connection::process_events_a() for more on this.
///
/// \sa \ref display::Window::set_event_handler()
/// \sa \ref display::ConnectionEventHandler
/// \sa \ref display::Connection::process_events(), \ref display::Connection::process_events_a()
///
class WindowEventHandler {
public:
    /// \{
    ///
    /// \brief A key was pressed down or released.
    ///
    /// These functions are called when the "key down", "key up", or "key repeat" events are
    /// generated. The targeted event handler object is the window's associated window
    /// handler (see \ref display::Connection::new_window()).
    ///
    /// A "key down" event is generated for a particular window when a key is pressed down
    /// while that window has input focus (see \ref on_focus()). Likewise, a "key up" event
    /// is generated for a particular window when a key is released while that window has
    /// input focus. A "key repeat" event is generated when a key repeats automatically,
    /// i.e., after having been held down for a short while. One "key repeat" event is
    /// generated for each key repetition.
    ///
    /// In some cases, "key up" and "key down" events are generated when windows lose or
    /// gain input focus while keys are pressed down. See \ref on_focus() for more on this.
    ///
    /// The default implementations of these functions do nothing other than return `true`.
    ///
    /// \sa \ref on_keydown()
    /// \sa \ref on_keyup()
    /// \sa \ref on_keyrepeat()
    /// \sa \ref on_mousedown()
    /// \sa \ref on_focus()
    ///
    virtual bool on_keydown(const display::KeyEvent&);
    virtual bool on_keyup(const display::KeyEvent&);
    virtual bool on_keyrepeat(const display::KeyEvent&);
    /// \}

    /// \{
    ///
    /// \brief A mouse button was pressed down or released.
    ///
    /// These functions are called when the "mouse down" or "mouse up" events are
    /// generated. The targeted event handler object is the window's associated window
    /// handler (see \ref display::Connection::new_window()).
    ///
    /// A "mouse down" event is generated for a particular window when a pointer grab is
    /// initiated in that window. A "mouse up" event is generated when the pointer grab
    /// ends.
    ///
    /// A *pointer grab* is initiated in a window, W, when the pointer is activated while W
    /// has pointer focus and input focus. The *pointer is activated* when a mouse button is
    /// pressed down, and no mouse buttons were pressed down already. The pointer grab is
    /// sustained for as long as at least one mouse button remains pressed down. See \ref
    /// on_mouseover() and \ref on_focus() for explanations of what it means for a window to
    /// have pointer focus and input focus respectively.
    ///
    /// In some cases, depending on platform and on platform configuration, a pointer grab
    /// can be effectively initiated in a window that does not already have input
    /// focus. This works, because in those cases, a window with pointer focus receives
    /// input focus as soon as the pointer is activated, and early enough for it to count
    /// towards pointer grab initiation. X11 with typical window managers (Mutter and
    /// Muffin) is an example of this. The Windows platform is an example of the opposite
    /// situation. Here, one separate button press is needed to assign input focus to the
    /// window. This then allows for the next button press to initiate a pointer grab.
    ///
    /// SDL quirks: A pointer grab is supposed to be sustained even if the initiating window
    /// loses input focus, and this is indeed the case when using the X11-based display
    /// implementation (\ref display::get_x11_implementation_slot()). Unfortunately, SDL has
    /// an inconsistent / buggy behavior when the window loses input focus while a pointer
    /// grab is in progress, and it does not appear to be possible for the SDL-based
    /// implementation (\ref display::get_sdl_implementation_slot()) to fully hide these
    /// inconsistencies. From the point of view of the "mouse out" and "mouse move" events,
    /// it looks like a pointer grab is terminated when the window loses input focus at a
    /// time where the pointer is outside the window. Something is amiss, through, because
    /// the the "mouse up" event is not generated at that time, and also not when the mouse
    /// button is released. Moreover, when the mouse reenters the window, "mouse move"
    /// events with nonzero `SDL_MouseMotionEvent::state` are generated even though no mouse
    /// button is pressed, suggesting that the grab is not properly terminated. Also, when a
    /// mouse button is pressed down, no "mouse down" event is generated, suggesting that
    /// the button was incorrectly registered as already pressed down. The quirky SDL
    /// behavior described here applies to SDL with X11 as a back-end and with SDL on
    /// Windows. On macOS, a pointer grab seems to be fully terminated when the window loses
    /// input focus, except for the "mouse up" event which is not generated at all in this
    /// case.
    ///
    /// SDL quirks: With SDL on Windows, when a pointer grab ends over a different window,
    /// the "mouse over" event for the different window is not generated until the mouse
    /// starts to move.
    ///
    /// FIXME: Consider reporting the inconsistent / buggy behavior of SDL when a window loses input focus while a grab is in progress                            
    ///
    /// The default implementations of these functions do nothing other than return `true`.
    ///
    /// \sa \ref on_mousedown()
    /// \sa \ref on_mouseup()
    /// \sa \ref on_mousemove()
    /// \sa \ref on_keydown()
    /// \sa \ref on_focus()
    ///
    virtual bool on_mousedown(const display::MouseButtonEvent&);
    virtual bool on_mouseup(const display::MouseButtonEvent&);
    /// \}

    /// \brief Mouse pointer moved during pointer grab.
    ///
    /// This function is called when the "mouse move" event is generated. The targeted event
    /// handler object is the window's associated window handler (see \ref
    /// display::Connection::new_window()).
    ///
    /// A "mouse move" event is generated for a particular window when the mouse moves
    /// during a pointer grab that was initiated in that window. See \ref on_mousedown() for
    /// more on pointer grabs.
    ///
    /// FIXME: Verify above claims on macOS and Windows platforms                            
    ///
    /// The default implementation of this function does nothing other than return `true`.
    ///
    /// \sa \ref on_mousedown()
    /// \sa \ref on_scroll()
    ///
    virtual bool on_mousemove(const display::MouseEvent&);

    /// \brief Mouse scroll wheel moved.
    ///
    /// This function is called when the "scroll" event is generated. The targeted event
    /// handler object is the window's associated window handler (see \ref
    /// display::Connection::new_window()).
    ///
    /// A "scroll" event is generated for a particular window when the scroll wheel is
    /// moved. It is generated regardless of whether the window has input focus (\ref
    /// on_focus()).
    ///
    /// FIXME: Verify above claims on macOS and Windows platforms                            
    ///
    /// With a scroll wheel that turns in discrete steps (detents), one "scroll" event is
    /// normally generated per step.
    ///
    /// The default implementation of this function does nothing other than return `true`.
    ///
    /// \sa \ref on_mousemove()
    ///
    virtual bool on_scroll(const display::ScrollEvent&);

    /// \{
    ///
    /// \brief Mouse pointer entered or left window.
    ///
    /// These functions are called when the "mouse over" or "mouse out" events are
    /// generated. The targeted event handler object is the window's associated window
    /// handler (see \ref display::Connection::new_window()).
    ///
    /// In the absence of pointer grabs, a "mouse out" event is generated for a particular
    /// window when that window transitions from having pointer focus to not having pointer
    /// focus. A "mouse over" event is generated when the opposite transition occurs.
    ///
    /// A window, W, has *pointer focus* when, and only when the pointer is inside the
    /// contents area of W and there is no other window stacked on top of W such that W is
    /// obscured at the position of the pointer. See \ref display::Window for the exact
    /// meaning of "contents area".
    ///
    /// If a pointer grab is initiated in window, W, the "mouse out" event for W is not
    /// generated until after the grab ends (see \ref on_mousedown() for more on pointer
    /// grabs). The "mouse out" event is generated when the grab ends if the grab ends while
    /// W does not have pointer focus. If the grab ends while W has pointer focus, the time
    /// of generation of the "mouse out" event depends on what happens later.
    ///
    /// The "mouse over" and "mouse out" events are generated regardless of whether the
    /// window has input focus (\ref on_focus()).
    ///
    /// The default implementations of these functions do nothing other than return `true`.
    ///
    /// \sa \ref on_mouseover()
    /// \sa \ref on_mouseout()
    /// \sa \ref display::Window
    ///
    virtual bool on_mouseover(const display::TimedWindowEvent&);
    virtual bool on_mouseout(const display::TimedWindowEvent&);
    /// \}

    /// \{
    ///
    /// \brief A window gained or lost input focus.
    ///
    /// These functions are called when the "focus" or "blur" events are generated. The
    /// targeted event handler object is the window's associated window handler (see \ref
    /// display::Connection::new_window()).
    ///
    /// A "focus" event is generated for a particular window when that window gains input
    /// focus. Likewise, a "blur" event is generated for a particular window when that
    /// window loses input focus.
    ///
    /// A window has *input focus* when it is the receiver of keyboard events (\ref
    /// on_keydown()). Only one window at a time can have the input focus. When a window
    /// gains input focus, another window loses it.
    ///
    /// In general, input focus is switched to a particular window by pressing a mouse
    /// button while that window has pointer focus. The exact details depend on the platform
    /// and on the configuration of the platform. See \ref on_mouseover() for en explanation
    /// of what it means for a window to have pointer focus.
    ///
    /// The default implementations of these functions do nothing other than return `true`.
    ///
    /// It is unspecified what happens if a window loses or gains input focus while keys are
    /// pressed down. At the time of writing, some implementations (notably SDL) will
    /// generate "key up" events for certain keys when a window loses input focus while
    /// those keys are pressed down, and will also generate "key down" events for certain
    /// keys if they are already pressed down when a window gains focus. Other
    /// implementations will not do this (notably X11). An application, that wants to
    /// enforce a regime where keys are released when the window loses input focus, must
    /// keep track of the set of currently pressed down keys, and then synthetically
    /// generate "key up" events when the window loses focus (\ref on_blur()). Such an
    /// application will probably also want to ignore any "key up" event that does not
    /// correspond to a pressed down key according to its own record of pressed down
    /// keys. See also discussion in the SDL-based display implementation
    /// (`sdl_implementation.cpp`) near the handling of `SDL_KEYDOWN`.
    ///
    /// \sa \ref on_focus()
    /// \sa \ref on_blur()
    /// \sa \ref on_keydown()
    /// \sa \ref on_mousedown()
    ///
    virtual bool on_focus(const display::WindowEvent&);
    virtual bool on_blur(const display::WindowEvent&);
    /// \}

    /// \brief A window was exposed and needs to be redrawn.
    ///
    /// This function is called when the "expose" event is generated. The targeted event
    /// handler object is the window's associated window handler (see \ref
    /// display::Connection::new_window()).
    ///
    /// An "expose" event is generated for a particular window when all or some of its
    /// contents need to be redrawn either because the affected area was never drawn before,
    /// or because the contents in that area was damaged. Damage to the window's contents
    /// can happen due to the inner workings of the platform's graphical user interface
    /// (X11).
    ///
    /// Additionally, "expose" events are generated after every "resize" event (\ref
    /// on_resize()) regardless of whether it shrinks or enlarges the window area.
    ///
    /// All "expose" events are buffered in the sense that when multiple "expose" events are
    /// generated within a short amount of time, the handler will generally only be called
    /// once. This makes the "expose" event handler a good place for redrawing window
    /// contents in many applications. See the notes on redrawing strategies in the
    /// documentation of \ref display::Window. See additional details under \ref
    /// display::Connection::process_events_a().
    ///
    /// The default implementation of this function does nothing other than return `true`.
    ///
    /// \sa \ref display::Window
    /// \sa \ref display::Connection::process_events_a()
    ///
    virtual bool on_expose(const display::WindowEvent&);

    /// \brief The size of a window changed.
    ///
    /// This function is called when the "resize" event is generated. The targeted event
    /// handler object is the window's associated window handler (see \ref
    /// display::Connection::new_window()).
    ///
    /// A "resize" event is generated for a particular window when that window is resized
    /// either interactively by a user of a graphical user interface or programmatically
    /// through use of \ref display::Window::set_size() or \ref
    /// display::Window::set_fullscreen_mode().
    ///
    /// Every "resize" event will be followed by an "expose" event (\ref on_expose()). This
    /// allows many applications to limit redrawing of window contents to the "expose" event
    /// handler. See \ref display::Connection::process_events_a() for more on this.
    ///
    /// The default implementation of this function does nothing other than return `true`.
    ///
    virtual bool on_resize(const display::WindowSizeEvent&);

    /// \brief The position of a window changed.
    ///
    /// This function is called when the "reposition" event is generated. The targeted event
    /// handler object is the window's associated window handler (see \ref
    /// display::Connection::new_window()).
    ///
    /// A "reposition" event is generated for a particular window when that window is
    /// moved. A switch to or from fullscreen mode (\ref
    /// display::Window::set_fullscreen_mode()) will generally cause the windows position to
    /// change, and when it does, reposition events will be generated (but see note below).
    ///
    /// \note Due to quirks in the behavior of some X11 window managers, "reposition" events
    /// are not always generated when switching to and from fullscreen mode. Specifically,
    /// when using the X11-based display implementation (\ref
    /// display::get_x11_implementation_slot()), and when the window manager is Xfwm4
    /// (Xfce), "reposition" events are generated when switching to and from fullscreen
    /// mode. Conversely, when the window manager is Mutter (Gnome) or Muffin (Cinnamon),
    /// "reposition" events are generally not generated when switching to and from
    /// fullscreen mode.
    ///
    /// FIXME: Look for a way to work around the mentioned problem with "reposition" events
    /// not always being generated when switching to or from fullscreen mode. When SDL uses
    /// its X11-based backend, it works around this problem by manually translating relative
    /// positions in non-synthetic `ConfigureNotify` events into absolute
    /// positions. Unfortunately, this scheme is not very reliable. First of all, it only
    /// works when the client's window changes size or is repositioned relative to its
    /// parent when it is taken to or from fullscreen mode, but this is not guaranteed to
    /// happen, so some switches may be missed. Another problem is that the translated
    /// relative position refers to a time that no longer exists when the event is processed
    /// by the client, so a manual translation mixes information from two different points
    /// in time, which can lead to unexpected behavior. A more robust scheme might be to
    /// track the position of the parent window, i.e., the window created by the window
    /// manager, but this will only work if the parent of the parent is the root.
    ///
    /// The default implementation of this function does nothing other than return `true`.
    ///
    virtual bool on_reposition(const display::WindowPosEvent&);

    /// \brief Request to close a window.
    ///
    /// This function is called when the "close" event is generated. The targeted event
    /// handler object is the window's associated window handler (see \ref
    /// display::Connection::new_window()).
    ///
    /// The "close" event is generated for a particular window when a user of a graphical
    /// user interface requests the closure of the window. Usually, this means that the user
    /// has clicked on the 'close' symbol of the window.
    ///
    /// The default implementation of this function does nothing other than return
    /// `false`. This will cause event processing to be interrupted. This default
    /// implementation will generally be appropriate only for single-window application.
    ///
    /// If the application ignores a close event (by returning `true` instead of `false`),
    /// the close event will be generated again as a result of a subsequent request to close
    /// the window.
    ///
    /// \sa \ref display::ConnectionEventHandler::on_quit()
    ///
    virtual bool on_close(const display::WindowEvent&);

    virtual ~WindowEventHandler() noexcept = default;
};



/// \brief Handle connection-level events.
///
/// In order to handle connection-level events, the application must instantiate a
/// connection-level event handler that overrides the relevant event handler functions, and
/// then call \ref display::Connetion::set_event_handler() to register that event event
/// handler with the connection. The individual event handler functions will then be called
/// by the event processor, or more specifically, by the thread that calls \ref
/// display::Connection::process_events() or \ref display::Connection::process_events_a().
///
/// If any of the event handler functions return `false`, event processing will be
/// interrupted. See \ref display::Connection::process_events_a() for more on interruption
/// of event handling.
///
/// With the exception of \ref before_sleep(), event handlers should generally execute fast,
/// which means that they should do very little work. See \ref
/// display::Connection::process_events_a() for more on this.
///
/// \sa \ref display::Connection::set_event_handler()
/// \sa \ref display::WindowEventHandler
/// \sa \ref display::Connection::process_events(), \ref display::Connection::process_events_a()
///
class ConnectionEventHandler {
public:
    /// \brief A screen configuration changed.
    ///
    /// This function is called when the "screen change" event is generated, provided that
    /// the display implementation exposes information about screen configurations. For more
    /// on this, see \ref display::Connection::try_get_screen_conf(). The \p screen argument
    /// specifies the index of the screen whose configuration changed.
    ///
    virtual bool on_screen_change(int screen);

    /// \brief Opportunity to do stuff before sleep.
    ///
    /// This function is called right before the event processor (\ref
    /// display::Connection::process_events()) goes to sleep, which it does when it needs to
    /// wait for more events to occur. This function is also called periodically in
    /// situations where no sleeping takes place because the event processor is fully
    /// saturated by incoming events.
    ///
    /// The main purpose of this function is to allow for the creation of specialized
    /// buffered redrawing mechanisms. Many simple applications can get away with doing all
    /// redrawing of window contents in \ref display::WindowEventHandler::on_expose(), but
    /// sometimes this not sufficient. See the notes on redrawing strategies in the
    /// documentation of \ref display::Window. See additional details under \ref
    /// display::Connection::process_events_a().
    ///
    /// The default implementation of this function does nothing other than return `true`.
    ///
    /// \sa \ref display::Window
    /// \sa \ref display::Connection::process_events_a()
    ///
    virtual bool before_sleep();

    /// \brief Request to quit application.
    ///
    /// This function is called when the "quit" event is generated. The quit event is
    /// similar to the "close" event for windows (\ref
    /// display::WindowEventHandler::on_close()). It is a request to close an entire program
    /// or application, not just one of its windows.
    ///
    /// On the Apple macOS platform and when using the SDL-based implementation (\ref
    /// display::get_sdl_implementation_slot()), the quit event is generated when Command-Q
    /// is pressed on the keyboard. When using the X11-based implementation (\ref
    /// display::get_x11_implementation_slot()), the quit event is never generated.
    ///
    /// The default implementation of this function does nothing other than return
    /// `false`. This will cause event processing to be interrupted. This default
    /// implementation will generally be appropriate only for applications where the
    /// interruption of event processing always leads to the termination of the
    /// application. Applications, that need to interrupt event processing for other
    /// reasons, will have to override this function and set a flag to indicate the reason
    /// for the interruption. Applications that engage in frame-based rendering is an
    /// example of this.
    ///
    /// If the application ignores a quit event (by returning `true` instead of `false`),
    /// the quit event will be generated again as a result of a subsequent request to close
    /// the application.
    ///
    /// \sa \ref display::WindowEventHandler::on_close()
    ///
    virtual bool on_quit();

    virtual ~ConnectionEventHandler() noexcept = default;
};


} // namespace archon::display

#endif // ARCHON_X_DISPLAY_X_EVENT_HANDLER_HPP
