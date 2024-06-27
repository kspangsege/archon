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


#include <cstddef>
#include <memory>
#include <string_view>
#include <string>
#include <locale>
#include <chrono>

#include <archon/core/buffer.hpp>
#include <archon/log/logger.hpp>
#include <archon/display/implementation_fwd.hpp>
#include <archon/display/geometry.hpp>
#include <archon/display/key.hpp>
#include <archon/display/key_code.hpp>
#include <archon/display/event_handler.hpp>
#include <archon/display/resolution.hpp>
#include <archon/display/viewport.hpp>
#include <archon/display/guarantees.hpp>
#include <archon/display/connection_config_x11.hpp>
#include <archon/display/connection_config_sdl.hpp>
#include <archon/display/window.hpp>


namespace archon::display {


/// \brief Connection to platform's graphical user interface.
///
/// When using the X11-based display implementation (\ref
/// display::get_x11_implementation_slot()), an instance of this class represents a
/// connection to an X11 display. More generally, an instance of this class can be thought
/// of as a session of access to the graphical user interfaces of the platform. Such access
/// will ordinarily consist of the creation of one or more windows (\ref new_window(), \ref
/// display::Window).
///
/// A display gives access to one or more screens, and the number of screens is fixed for
/// the lifetime of a connection. Screens are identified by their index (\ref
/// display::Window::Config::screen). The order of screens is determined by the platform and
/// the underlying implementation and it remains fixed for the duration of the
/// connection. See also \ref get_num_screens() and \ref get_default_screen().
///
/// A screen is a spatial (i.e., planar) arrangement of viewports (zero or more), with each
/// viewport corresponding to a video adapter output (CRTC) of a certain size (width and
/// height in number of pixels). Viewports generally materialize and vanish in response to
/// monitors being attached and detached. In any case, the configuration of a screen is
/// dynamic, and this includes both the set of viewports that make up the screen, the
/// properties of those viewports, and their spatial arrangement.
///
/// When supported by the underlying implementation, the current configuration of a screen
/// can be obtained by calling \ref try_get_screen_conf(), and the the application can
/// register to be informed about screen configuration changes by using a connection-level
/// event handler (passed to \ref display::Connection::process_events()) and implementing
/// \ref display::ConnectionEventHandler::on_screen_change().
///
/// New connections can be established by calling \ref display::new_connection() or \ref
/// display::Implementation::new_connection(). The latter one allows you to establish a
/// connection using a specific underlying implementation (X11, SDL, ...).
///
class Connection {
public:
    struct Config;

    /// \brief Map well-known key to key code.
    ///
    /// This function maps the specified well-known key (\p key) to the corresponding key
    /// code (\p key_code). If a corresponding key code exists, this function returns `true`
    /// after setting \p key_code to that key code. If a corresponding key code does not
    /// exist, this function returns `false` and leaves \p key_code unchanged.
    ///
    virtual bool try_map_key_to_key_code(display::Key key, display::KeyCode& key_code) const = 0;

    /// \brief Map key code to well-known key.
    ///
    /// This function maps the specified key code (\p key_code) to the corresponding
    /// well-known key (\p key). If a corresponding well-known key exists, this function
    /// returns `true` after setting \p key to that key. If a corresponding well-known key
    /// does not exist, this function returns `false` and leaves \p key unchanged.
    ///
    virtual bool try_map_key_code_to_key(display::KeyCode key_code, display::Key& key) const = 0;

    /// \brief Get key name.
    ///
    /// This function returns the name of the specified key as known to this
    /// implementation. Key names are not guaranteed to be invariant across implementations.
    ///
    /// If the implementation knows the name of the specified key (\p key_code), this
    /// function returns `true` after setting \p name to that name. Otherwise this function
    /// returns `false` and leaves \p name unchanged.
    ///
    virtual bool try_get_key_name(display::KeyCode key_code, std::string_view& name) const = 0;

    /// \brief Create a new window.
    ///
    /// This function creates a new window with the specified title (\p title) and size (\p
    /// size), and configured according to the specified configuration parameters (\p
    /// config). The target screen is specified through \ref
    /// display::Window::Config::screen.
    ///
    /// This function is shorthand for calling \ref try_new_window() and then returning the
    /// created window on success, and throwing an exception on failure.
    ///
    /// \sa \ref try_new_window()
    ///
    auto new_window(std::string_view title, display::Size size, const display::Window::Config& config = {}) ->
        std::unique_ptr<display::Window>;

    /// \brief Try to create a new window.
    ///
    /// This function attempts to create a new window with the specified title (\p title)
    /// and size (\p size), and configured according to the specified configuration
    /// parameters (\p config). The target screen is specified through \ref
    /// display::Window::Config::screen.
    ///
    /// On success, this function returns `true` after setting \p win to refer to the new
    /// window object. On failure, it returns `false` after setting \p error to a message
    /// that describes the cause of the failure. \p error is left untouched on success, and
    /// \p win is left untouched on failure.
    ///
    /// The application will generally have to set a new event handler for the window using
    /// \ref display::Window::set_event_handler(), and in order to not lose any events, this
    /// has to happen before the next invocation of the event processor (\ref
    /// process_events()).
    ///
    /// The window starts out in the "hidden" state. Call \ref display::Window::show() to
    /// unhide it.
    ///
    /// The initial position of the window is determined by the platform and / or
    /// implementation. When using the X11-based implementation (\ref
    /// display::get_x11_implementation_slot()), the initial position is generally
    /// determined by a window manager.
    ///
    /// The destruction of the returned window object (\p win) must happen before the
    /// destruction of this connection object.
    ///
    /// If the application chooses to provide the display guarantee, \ref
    /// display::Guarantees::main_thread_exclusive, then this function must be called only
    /// by the main thread. Further more, the returned window must be used only by the main
    /// thread. This includes the destruction of the window object.
    ///
    /// \sa \ref new_window()
    ///
    virtual bool try_new_window(std::string_view title, display::Size size, const display::Window::Config& config,
                                std::unique_ptr<display::Window>& win, std::string& error) = 0;

    using clock_type      = std::chrono::steady_clock;
    using time_point_type = std::chrono::time_point<clock_type>;

    /// \{
    ///
    /// \brief Process events until deadline expires or event processing is interrupted.
    ///
    /// `process_events(display::ConnectionEventHandler*)` will process events as they occur
    /// until the event processing is interrupted.
    ///
    /// Event processing is interrupted when any event handler function returns `false`. See
    /// \ref display::WindowEventHandler.
    ///
    /// `process_events(time_point_type, display::ConnectionEventHandler*)` will process
    /// events as they occur until the specified deadline expires or event processing is
    /// interrupted. If the deadline expires before event processing is interrupted, this
    /// function returns `true`. Otherwise this function returns `false`, which means that
    /// event processing was interrupted.
    ///
    /// So long as event processing is not interrupted, `process_events(time_point_type,
    /// display::ConnectionEventHandler*)` will process at least those events that were
    /// immediately available prior to the invocation of this function, even when the
    /// specified deadline was already expired prior to the invocation.
    ///
    /// These functions block the calling thread while waiting for events to occur or the
    /// deadline to expire.
    ///
    virtual void process_events(display::ConnectionEventHandler* = nullptr) = 0;
    virtual bool process_events(time_point_type deadline, display::ConnectionEventHandler* = nullptr) = 0;
    /// \}

    /// \brief Number of screens accessible through connection.
    ///
    /// This function returns the number of separate screens that are accessible through
    /// this connection. See \ref display::Connection for general information about screens.
    ///
    /// When using the X11-based implementation (\ref
    /// display::get_x11_implementation_slot()), a screen corresponds to the X11 concept of
    /// the same name.
    ///
    /// When using the SDL-based implementation (\ref
    /// display::get_sdl_implementation_slot()), only one screen will be available. When SDL
    /// bridges to the X Window System, the `DISPLAY` environment variable can be used to
    /// select which of the X screens to target.
    ///
    /// \sa \ref get_default_screen()
    ///
    virtual int get_num_screens() const = 0;

    /// \brief Index of default screen.
    ///
    /// This function returns the index of the default screen of this connection. The index
    /// refers to an order of the accessible screens that is determined by the platform and
    /// implementation. See \ref display::Connection for general information about screens.
    ///
    /// When using the X11-based implementation (\ref
    /// display::get_x11_implementation_slot()), the default screen is determined by the
    /// screen number specified in the value of the `DISPLAY` environment variable.
    ///
    /// \sa \ref get_num_screens()
    ///
    virtual int get_default_screen() const = 0;

    /// \brief Retrieve current configuration of screen.
    ///
    /// If supported by the implementation, this function returns the current configuration
    /// of the specified screen (\p screen).
    ///
    /// A particular display implementation (\ref display::Implementation) is not required
    /// to expose information about the configuration of each of the accessible screens. If
    /// the implementation in use for this connection does not expose this information, this
    /// function returns `false` and leaves all arguments unchanged. Otherwise, this
    /// function returns `true` after placing an entry in \p viewports for each of the
    /// viewports that are currently parts of the specified screen, placing associated
    /// string data in \p strings, and setting \p num_viewports to the number of viewports
    /// placed in \p viewports.
    ///
    /// If the implementation exposes the screen configuration, i.e., when this function
    /// returns `true`, the implementation will also generate "screen changed" events
    /// whenever a screen configuration changes (\ref
    /// display::ConnectionEventHandler::on_screen_change()).
    ///
    /// Some display implementations are able to provide the screen configurations, but in a
    /// less than reliable manner due to quirks in the underlying subsystem (SDL is an
    /// example of this). Such implementations must set \p reliable to `false` when
    /// `try_get_screen_conf()` returns `true`. Display implementations that provide the
    /// screen configuration in a reliable manner should set \p reliable to `true` when
    /// `try_get_screen_conf()` returns `true`.
    ///
    virtual bool try_get_screen_conf(int screen, core::Buffer<display::Viewport>& viewports,
                                     core::Buffer<char>& strings, std::size_t& num_viewports,
                                     bool& reliable) const = 0;

    /// \brief Associated implementation.
    ///
    /// This function returns a reference to the implementation that this connection is
    /// associated with.
    ///
    virtual auto get_implementation() const noexcept -> const display::Implementation& = 0;

    virtual ~Connection() noexcept = default;
};


/// \brief Connection configuration parameters.
///
/// This is a collection of all the configuration parameters that are specific to each of
/// the display implementations.
///
struct Connection::Config {
    /// \brief Log through specified logger.
    ///
    /// If no logger is specified, nothing is logged. If a logger is specified, it must use
    /// a locale that is compatible with the locale that is passed to \ref
    /// display::Implementation::new_connection() or one of the other functions that can be
    /// used to create a connection.. The important thing is that the character encodings
    /// agree (`std::codecvt` facet).
    ///
    log::Logger* logger = nullptr;

    /// \brief Parameters specific to X11-based implementation.
    ///
    /// These are the parameters that are specific to the X11-based implementation.
    ///
    display::ConnectionConfigX11 x11;

    /// \brief Parameters specific to SDL-based implementation.
    ///
    /// These are the parameters that are specific to the SDL-based implementation.
    ///
    display::ConnectionConfigSDL sdl;
};


/// \brief Establish display connection using default implementation.
///
/// This function is shorthand for calling \ref display::try_new_connection() and then, on
/// success, return the new connection object, and on failure, throw an exception.
///
/// \sa \ref display::try_new_connection()
///
auto new_connection(const std::locale&, const display::Guarantees&, const display::Connection::Config& = {}) ->
    std::unique_ptr<display::Connection>;


/// \{
///
/// \brief Try to establish display connection using default implementation if available.
///
/// These functions attempt to establishes a connection to the display using the default
/// display implementation (\ref display::get_default_implementation()).
///
/// `try_new_connection(locale, guarantees, config, conn, error)` is shorthand for calling
/// `try_new_connection_a(locale, guarantees, config, conn, error)`, and then, if no
/// implementations were available, generate an error with a suitable message (\p error).
///
/// The overload, that does not take a configuration argument (\p config), uses the default
/// configuration.
///
/// \sa \ref display::new_connection(), \ref display::try_new_connection_a()
/// \sa \ref display::get_default_implementation()
///
bool try_new_connection(const std::locale& locale, const display::Guarantees& guarantees,
                        std::unique_ptr<display::Connection>& conn, std::string& error);
bool try_new_connection(const std::locale& locale, const display::Guarantees& guarantees,
                        const display::Connection::Config& config,
                        std::unique_ptr<display::Connection>& conn, std::string& error);
/// \}


/// \brief Try to establish display connection using default implementation if available.
///
/// This function attempts to establishes a connection to the display using the default
/// display implementation. It is a shorthand for calling \ref
/// display::get_default_implementation_a(), and then calling
/// display::Implementation::try_new_connection() on the implementation object, if an
/// implementation was available. If no implementations were available, this function
/// returns `true` after setting \p conn to null.
///
/// Note that if \p guarantees include \ref display::Guarantees::only_one_connection, then
/// at most one connection may exist per operating system process at any given time.
///
/// Note that if \p guarantees include \ref display::Guarantees::main_thread_exclusive, then
/// this function must be called only by the main thread. Further more, the returned
/// connection must be used only by the main thread. This includes the destruction of the
/// connection returned by this function.
///
/// \sa \ref display::try_new_connection()
/// \sa \ref display::get_default_implementation_a()
///
bool try_new_connection_a(const std::locale& locale, const display::Guarantees& guarantees,
                          const display::Connection::Config& config,
                          std::unique_ptr<display::Connection>& conn, std::string& error);








// Implementation


inline bool try_new_connection(const std::locale& locale, const display::Guarantees& guarantees,
                               std::unique_ptr<display::Connection>& conn, std::string& error)
{
    display::Connection::Config config;
    return display::try_new_connection(locale, guarantees, config, conn, error); // Throws
}


} // namespace archon::display

#endif // ARCHON_X_DISPLAY_X_CONNECTION_HPP
