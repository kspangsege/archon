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


namespace archon::display {


/// \brief    
///
/// The base class for all event objects. Event objects are created internally in the event                       
/// processor and passed as arguments to your event handler methods. Most event object types
/// are derivatives of this class, but a few use this class directly.
///
/// \sa \ref display::TimedEvent
///
struct Event {
    /// \brief    
    ///
    /// This attribute is supposed to identify the source window of this event. It will                              
    /// carry the value that you specified when you registered the window that generated
    /// this event. This library will not interpret this value in any way. It is entirely up
    /// to you to decide how to utilize it, if you need to.
    ///
    int cookie;
};



/// \brief    
///
/// This event type is used for events that carry a time-stamp indicating when the event
/// occured relative to other timed events. Note that the timestampts are not necessarily
/// UNIX Epoch based.
///
/// \sa \ref display::KeyEvent
///
struct TimedEvent : display::Event {
    using Timestamp = std::chrono::milliseconds;

    /// \brief    
    ///
    /// The point in time where the event occurred relative to a fixed, but arbitrary origin
    /// (epoch). Note that the origin is not necessarily, and most likely not equal to the
    /// beginning of the UNIX Epoch. For this reason, these timestamps can only be used for
    /// measuring time between events. All timestamps will be nonnegative.
    ///
    Timestamp timestamp;
};



/// \brief    
///
/// This event type is generated whenever a key is pressed down or released. It is generated
/// in the context of the window that is in focus when the event occurs.
///
/// \sa \ref display::EventHandler::on_keydown()
/// \sa \ref display::EventHandler::on_keyup()
///
struct KeyEvent : display::TimedEvent {
    /// \brief    
    ///
    ///    
    ///
    using KeySym = int;

    /// \brief    
    ///
    ///    
    ///
    KeySym key_sym;
};


} // namespace archon::display

#endif // ARCHON_X_DISPLAY_X_EVENT_HPP
