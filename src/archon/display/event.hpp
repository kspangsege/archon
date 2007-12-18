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

#ifndef ARCHON_DISPLAY_EVENT_HPP
#define ARCHON_DISPLAY_EVENT_HPP

#include <vector>
#include <string>

#include <archon/core/time.hpp>
#include <archon/display/keysyms.hpp>
#include <archon/display/window.hpp>


namespace Archon {
namespace Display {

/// The base class for all event objects. Event objects are created internally
/// in the event processor and passed as arguments to your event handler
/// methods. Most event object types are a derivative of this class, but it is
/// used directly for a few types of events.
///
/// \sa EventHandler
/// \sa EventProcessor
class Event {
public:
    /// This attribute is supposed to identify the source window of this
    /// event. It will carry the value that you specified when you registered
    /// the window that generated this event. This library will not interpret
    /// this value in any way. It is entirely up to you to decide how to utilize
    /// it, if you need to.
    const int cookie;

    Event(int cookie);
};


/// This event is generated whenever a window is resized.
///
/// \sa EventHandler::on_resize
class SizeEvent: public Event {
public:
    /// New size of the window. Always measured in pixels. The window frame is
    /// not included, it is the size of the area that is avaible to the
    /// application.
    const int width, height;

    SizeEvent(int cookie, int w, int h);
};


/// This object is used for events that relate to a specific rectangular area of
/// the origin window. It is currently only used by the \c damage event.
///
/// \sa EventHandler::on_damage
class AreaEvent: public Event {
public:
    /// The location of the upper right corner of the rectangular area
    /// designated by this event. It is measured in pixels from the upper left
    /// corner of the contents area of the window.
    const int x, y;

    /// The size of the rectangular area designated by this event. It is
    /// measured in pixels.
    const int width, height;

    AreaEvent(int cookie, int x_, int y_, int w, int h);
};


/// This event type is used for events that carry a time-stamp indicating when
/// the event occured relative to other timed events. Note that the timestampts
/// are not necessarily UNIX Epoch based.
///
/// \sa EventHandler::on_mouseover
/// \sa EventHandler::on_mouseout
class TimedEvent: public Event {
public:
    /// The time the event occured. The origin is arbitrary, but the same for
    /// all events. Thus, it does _not_ represent the time since the UNIX Epoch.
    const Core::Time time;

    TimedEvent(int cookie, Core::Time t);
};


/// This event type is generated whenever a key is pressed down or released. It
/// is generated in the context of the window that is in focus when the event
/// occurs.
///
/// \sa EventHandler::on_keydown
/// \sa EventHandler::on_keyup
class KeyEvent: public TimedEvent {
public:
    const KeySym key_sym;

    KeyEvent(int cookie, Core::Time t, KeySym s);
};


/// This event is generated both when the mouse moves and when mouse buttons are
/// activated. In the button case, the actual event type will be
/// <tt>MouseButtonEvent</tt>.
///
/// \sa EventHandler::on_mousemove
/// \sa MouseButtonEvent
class MouseEvent: public TimedEvent {
public:
    /// Position of the mouse pointer at the time the event occured. Always
    /// measured in pixels from the upper left corner of thw window.
    const int x,y;

    MouseEvent(int cookie, Core::Time t, int x, int y);
};


/// This event type is generated when any of the mouse buttons are pressed down
/// or released. The event is generated in the context of the window over which
/// the mouse appears when the event occurs.
///
/// \sa EventHandler::on_mousedown
/// \sa EventHandler::on_mouseup
class MouseButtonEvent: public MouseEvent {
public:
    /// Indicates which button has been pressed or released. Normally 1
    /// corresponds to the primary button, and 3 to the secondary button, while
    /// 2 will be the middle button or the scroll wheel when used as a button.
    const int button;

    MouseButtonEvent(int cookie, Core::Time t, int x, int y, int button);
};


/// A base for window event handlers. Derive your event handler from this class
/// and override the methods corresponding to those types of events in which you
/// are interested. The pass it as argument to
/// <tt>Connection::new_event_processor</tt>.
///
/// It is legal for any of the event handler methods (and <tt>before_sleep</tt>)
/// to throw exceptions, and this is indeed the recommended way to terminate the
/// event handling, and return control back to the caller of
/// <tt>EventProcessor::process</tt>.
///
/// \sa Connection::new_event_processor
/// \sa EventProcessor::process
/// \sa before_sleep
class EventHandler {
public:
    /// Called when a mouse button is pressed down over one of the windows with
    /// which this event handler is associated.
    virtual void on_mousedown(const MouseButtonEvent&);

    /// Called when a mouse button is released down over one of the windows with
    /// which this event handler is associated.
    virtual void on_mouseup(const MouseButtonEvent&);

    /// Called when a key is pressed down and the window that has focus, is one
    /// of the windows with which this event handler is associated.
    virtual void on_keydown(const KeyEvent&);

    /// Called when a key is released and the window that has focus, is one of
    /// the windows with which this event handler is associated.
    virtual void on_keyup(const KeyEvent&);

    /// Called when the mouse has moved over one of the windows with which this
    /// event handler is associated.
    ///
    /// By default these events are generated only while a mouse button is
    /// pressed down, but this can be changed on a per window basis using
    /// <tt>Window::report_mouse_move</tt>.
    ///
    /// There is no guarantee about the frequency of invocations in case of
    /// continuous motion, but it is guaranteed that at least one event is
    /// generated when the pointer moves and then rests.
    virtual void on_mousemove(const MouseEvent&);

    /// Called when the size changes of one of the windows with which this event
    /// handler is associated.
    virtual void on_resize(const SizeEvent&);

    /// Called when the mouse enters one of the windows with which this event
    /// handler is associated.
    virtual void on_mouseover(const TimedEvent&);

    /// Called when the mouse enters one of the windows with which this event
    /// handler is associated.
    virtual void on_mouseout(const TimedEvent&);

    /// Called when focus is acquired by one of the windows with which this
    /// event handler is associated.
    virtual void on_focus(const Event&);

    /// Called when focus is lost by one of the windows with which this event
    /// handler is associated.
    virtual void on_blur(const Event&);

    /// Called when one of the windows, with which this event handler is
    /// associated, becomes partially or fully visible, after having been
    /// completely hidden. This applies to the contents area of the window only.
    virtual void on_show(const Event&);

    /// Called when one of the windows, with which this event handler is
    /// associated, becomes completely hidden, after having been partially or
    /// completely visible. This applies to the contents area of the window
    /// only.
    virtual void on_hide(const Event&);

    /// Called when part of a window needs to be refreshed due to earlier
    /// damage.
    virtual void on_damage(const AreaEvent&);

    /// Called when the user request closure of one of the windows with which
    /// this event handler is associated. This usually means that the user has
    /// clicked on the 'close' symbol of the window.
    virtual void on_close(const Event&);

    /// Override this method if you want to take action after all currently
    /// available events have been processed and just before the event processer
    /// blocks itself waiting for new events to arrive.
    ///
    /// A good use of this method is to check if an update of the window
    /// contents is required, and if it is, either redraw it directly, or throw
    /// an exception such that the event processing loop is interrupted, and the
    /// caller can perform the redraw. The latter technique is especially
    /// usefull when the window contents is rendered in a frame based manner.
    ///
    /// \sa Connection::process_events
    virtual void before_sleep();

    virtual ~EventHandler();
};


/// You should only create an event processor if you intend to handle the events
/// of the associated windows. If you create an event processor and do not call
/// the \c process method, then precious CPU and memory resources will be
/// wasted, at least if the window generates many events.
///
/// The event processor will only buffer a finite number of events. This number
/// is currently 2000. For a high precision mouse, reporting at 500Hz, there
/// will be room for only 4 seconds of mouse motion events. This means that if
/// the \c process method returns, and more than 4 seconds pass before the \c
/// process method is reinvoked, then events might get lost. It is however
/// always the newest events that will be kept.
///
/// \todo FIXME: The 'max 2000 events' feature is not yet implemented. This
/// means that memory consumption could grow uncontrollably.
///
/// The methods of this object are not thread-safe. It is the intention that
/// each thread that wishes to engage in event handling has its own instance of
/// this class.
///
/// New instances are aquired by calling
/// <tt>Connection::new_event_processor</tt>.
class EventProcessor {
public:
    typedef Core::SharedPtr<EventProcessor> Ptr;
    typedef const Ptr& Arg;

    /// Register the specified window with this event processor. What this means
    /// is that all events originating from that window will be handled by this
    /// event processor, and only by this event processor. An event processor
    /// can handle multiple windows, but a window can only be handled by one
    /// processor. An attempt to register a window more than once will cause \c
    /// std::invalid_argument to be thrown.
    ///
    /// It shall be guaranteed that the thread that calls this method, is also
    /// the thread that executes the methods of the associated event
    /// handler. Futher more, event handler methods will be called in the same
    /// order that the events occur, however, there may be a significant delay
    /// from the time of the occurance of the event to the time that your
    /// handler is called.
    ///
    /// The specified window must belong to the same implementation as this
    /// processor does. Further more, both must have been created through the
    /// same display connection. \c std::invalid_argument will be thrown if this
    /// is not the case.
    ///
    /// \param w The window to be registered such that events from this window
    /// are handled by the event handler that is associated with this processor.
    ///
    /// \param cookie This integer value will be remembered by the processor,
    /// and all events originating from the specified window will carry this
    /// particular value such that you, in your event handler methods, can
    /// identify the origin window. However, you are free to use this 'cookie'
    /// for whatever purpose you desire.
    ///
    /// \note This method is _not_ thread-safe.
    virtual void register_window(Window::Arg w, int cookie = 0) = 0;

    /// Process events from all windows that are registered with this event
    /// processor. For each such event, one of the methods of your event handler
    /// will be called, that is, the event handler that was specified when the
    /// event processor was created. Your event handler methods will always be
    /// executed by the same thread that calls this \c process method. The call
    /// to \c process will not return unless one of the following events occur:
    /// 1) A non-zero \c timeout argument was specified, and that timeout is
    /// reached. 2) The calling thread is interrupted (see
    /// <tt>Thread::interrupt</tt>) either by some other thread, or by itself
    /// from one of the event handler methods. 3) One of the event handler
    /// methods throw an exception. The latter is the recommended way to
    /// terminate the event processing.
    ///
    /// \param timeout If non-zero, event processing will stop if the specified
    /// point in time is reached. Note that it is specified as an absolute point
    /// in time, and not a duration. If zero is specified (the default) the
    /// there will be no timeout condition.
    ///
    /// \note This method is _not_ thread-safe.
    ///
    /// \sa EventHandler::before_sleep
    virtual void process(Core::Time timeout = 0) = 0;

    /// Find the names of all the specified <tt>KeySym</tt>'s.
    ///
    /// An entry will be placed in \c names for each entry in \c key_syms with
    /// corresponding index. The \c names vector will be cleared initially
    /// should it be non-empty at the time of calling.
    ///
    /// It is possible that a name is not known for a particular
    /// <tt>KeySym</tt>. In this case the empty string will be returned.
    virtual void get_key_sym_names(const std::vector<KeySym>& key_syms,
                                   std::vector<std::string>& names) = 0;

    /// Convenince method for getting the name of a single <tt>KeySym</tt>. If
    /// you need to map several <tt>KeySym</tt>'s, please use
    /// <tt>get_key_sym_names</tt>, since a global display lock might need to be
    /// acquired, so better do as much as possible in one go.
    ///
    /// \sa get_key_sym_names
    std::string get_key_sym_name(KeySym key_sym);

    virtual ~EventProcessor();
};




// Implementation

inline Event::Event(int c):
    cookie(c)
{
}

inline SizeEvent::SizeEvent(int c, int w, int h):
    Event(c),
    width(w),
    height(h)
{
}

inline AreaEvent::AreaEvent(int c, int x_, int y_, int w, int h):
    Event(c),
    x(x_),
    y(y_),
    width(w),
    height(h)
{
}

inline TimedEvent::TimedEvent(int c, Core::Time t):
    Event(c),
    time(t)
{
}

inline KeyEvent::KeyEvent(int c, Core::Time t, KeySym s):
    TimedEvent(c,t),
    key_sym(s)
{
}

inline MouseEvent::MouseEvent(int c, Core::Time t, int x_, int y_):
    TimedEvent(c,t),
    x(x_),
    y(y_)
{
}

inline MouseButtonEvent::MouseButtonEvent(int c, Core::Time t, int x, int y, int b):
    MouseEvent(c,t,x,y),
    button(b)
{
}

inline void EventHandler::on_mousedown(const MouseButtonEvent&)
{
}

inline void EventHandler::on_mouseup(const MouseButtonEvent&)
{
}

inline void EventHandler::on_keydown(const KeyEvent&)
{
}

inline void EventHandler::on_keyup(const KeyEvent&)
{
}

inline void EventHandler::on_mousemove(const MouseEvent&)
{
}

inline void EventHandler::on_resize(const SizeEvent&)
{
}

inline void EventHandler::on_mouseover(const TimedEvent&)
{
}

inline void EventHandler::on_mouseout(const TimedEvent&)
{
}

inline void EventHandler::on_focus(const Event&)
{
}

inline void EventHandler::on_blur(const Event&)
{
}

inline void EventHandler::on_show(const Event&)
{
}

inline void EventHandler::on_hide(const Event&)
{
}

inline void EventHandler::on_damage(const AreaEvent&)
{
}

inline void EventHandler::on_close(const Event&)
{
}

inline void EventHandler::before_sleep()
{
}

inline EventHandler::~EventHandler()
{
}

inline std::string EventProcessor::get_key_sym_name(KeySym key_sym)
{
    std::vector<KeySym> in(1, key_sym);
    std::vector<std::string> out;
    get_key_sym_names(in, out);
    return out[0];
}

inline EventProcessor::~EventProcessor()
{
}

} // namespace Display
} // namespace Archon

#endif // ARCHON_DISPLAY_EVENT_HPP
