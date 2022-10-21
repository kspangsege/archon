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

#ifndef ARCHON_X_DISPLAY_X_CONNECTION_HPP
#define ARCHON_X_DISPLAY_X_CONNECTION_HPP

/// \file


#include <memory>

#include <archon/display/types.hpp>
#include <archon/display/window.hpp>
#include <archon/display/event_handler.hpp>


namespace archon::display {


/// \brief    
///
///    
///
/// FIXME: Unfortunately, the following seems to be necessary: If impl::main_thread_exclusive() returns `true`, only the main threas is allowed to use the display API, and at most one connection is allowed to exist any any time. Destructions of conenctions and windows must also only happen on behalf of the main thread.                     
///
class Connection {
public:
    /// \brief    
    ///
    /// The window starts out in the "hidden" state. Call \ref display::Window::show() to
    /// unhide it.
    ///
    /// FIXME: What are the constraints on allowed object lifetime?                                               
    ///
    /// FIXME: Explain cookie (identify origin window when multiple windows use same event handler)      
    ///
    virtual auto new_window(std::string_view title, display::Size size, display::EventHandler&, int cookie = 0) ->
        std::unique_ptr<display::Window> = 0;

    /// \brief    
    ///
    ///    
    ///
    virtual void wait() = 0;

    /// \brief    
    ///
    ///    
    ///
    virtual void process_events(bool& quit) = 0;

    /// \brief Number of screens of target display.
    ///
    /// This function returns the number of screens associated with the display targeted by
    /// this connection. Each screen may have different properties. Use \ref
    /// get_screen_bounds(), \ref get_screen_resolution(), and \ref get_num_screen_visuals()
    /// to enquire about these properties.
    ///
    virtual int get_num_screens() const = 0;

    /// \brief Size of specified screen.
    ///
    /// This function returns the size, in pixels, of the specified screen (\p screen) of
    /// the display targeted by this connection.
    ///
    /// This function simply returns `get_screen_bounds(screen).size` (see \ref
    /// get_screen_bounds()).
    ///
    auto get_screen_size(int screen = -1) const -> display::Size;

    /// \brief Coordinate bounds of specified screen.
    ///
    /// This function returns the coordinate bounds of the specified screen (\p screen) of
    /// the display targeted by this connection.
    ///
    /// \param screen The index of the screen to be enquired. A negative value will be
    /// interpreted as the default screen (see \ref get_default_screen()).
    ///
    virtual auto get_screen_bounds(int screen = -1) const -> display::Box = 0;

    /// \brief Resolution of specified screen.
    ///
    /// This function returns the resolution (dots per centimeter) of the specified screen
    /// (\p screen) of the display targeted by this connection.
    ///
    /// \param screen The index of the screen to be enquired. A negative value will be
    /// interpreted as the default screen (see \ref get_default_screen()).
    ///
    virtual auto get_screen_resolution(int screen = -1) const -> display::Resolution = 0;

    /// \brief    
    ///
    ///    
    ///
    virtual int get_num_screen_visuals(int screen = -1) const = 0;

    /// \brief Index of default screen of target display.
    ///
    /// This function returns the index of the default screen of the display taregted by
    /// this connection.
    ///
    virtual int get_default_screen() const = 0;

    virtual ~Connection() noexcept = default;
};








// Implementation


inline auto Connection::get_screen_size(int screen) const -> display::Size
{
    return get_screen_bounds(screen).size; // Throws
}


} // namespace archon::display

#endif // ARCHON_X_DISPLAY_X_CONNECTION_HPP
