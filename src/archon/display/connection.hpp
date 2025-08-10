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
#include <archon/display/x11_connection_config.hpp>
#include <archon/display/sdl_connection_config.hpp>
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
/// can be obtained by calling \ref try_get_screen_conf(), and the application can register
/// to be informed about screen configuration changes by using a connection-level event
/// handler (passed to \ref display::Connection::process_events()) and implementing \ref
/// display::ConnectionEventHandler::on_screen_change().
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
    /// determined by a window manager. In any case, the initial position will be reported
    /// to the application by way of a "reposition" event (\ref
    /// display::WindowEventHandler::on_reposition()).
    ///
    /// The initial size of the window will generally be as requested (\p size), but may be
    /// different. If it is different, a "resize" event is guaranteed to be generated
    /// initially (\ref display::WindowEventHandler::on_resize()). If the initial size is as
    /// requested, a "resize" event may or may not be generated initially.
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

    /// \brief Set new connection-level event handler.
    ///
    /// This function sets a new event handler at the connection level. Connection-level
    /// events will be reported through the specified event handler.
    ///
    /// The default connection-level event handler behaves as a instance of \ref
    /// display::ConnectionEventHandler would do with no overridden event handler functions.
    ///
    /// The avoid losing events, the application must set the desired event handler before
    /// calling \ref process_events() or \ref process_events_a().
    ///
    /// \sa \ref process_events(), \ref process_events_a()
    /// \sa \ref display::Window::set_event_handler()
    ///
    virtual void set_event_handler(display::ConnectionEventHandler&) = 0;

    using clock_type      = std::chrono::steady_clock;
    using time_point_type = std::chrono::time_point<clock_type>;

    /// \brief Process event with no deadline.
    ///
    /// This function has he same effect as \ref process_events_a() except that there is no
    /// deadline. Return will not happen until an event handler function returns `false`.
    ///
    /// \sa \ref process_events_a()
    ///
    virtual void process_events() = 0;

    /// \brief Process events until deadline expires.
    ///
    /// This function processes events as they occur until the specified deadline expires
    /// (\p deadline) or until event processing is interrupted by an event handler that
    /// returns `false` (see \ref display::WindowEventHandler and \ref
    /// display::ConectionEventHandler). The calling thread will be repeatedly blocked while
    /// it waits for more events to occur.
    ///
    /// Events from the underlying implementation can be thought of as being added to a
    /// queue as they are generated. Event processing then works by taking events from that
    /// queue one by one and calling the event handler that corresponds to the type of the
    /// next event. Window-specific events are directed to the event handler that is
    /// installed for the relevant window (see \ref
    /// display::Window::set_event_handler()). Connection-level events are directed to the
    /// installed connection-level event handler (see \ref set_event_handler()).
    ///
    /// This function returns `false` when it is interrupted by an event handler that
    /// returned `false`. If the deadline expires before an event handler has returned
    /// `false`, this function returns `true`. Deadline expiration may be detected slightly
    /// later than the true point of expiration. When an event handler returns `false`, no
    /// additional event handlers will be called before the return of this function.
    ///
    /// If the deadline expires at a time where events were available to be processed, then
    /// a subsequent invocation of `process_events_a()` cannot return due to deadline
    /// expiration until at least some of those events have been processed (at least one,
    /// but ideally more if more were available). This ensures that event processing cannot
    /// become completely "starved" in a situation where `process_events_a()` is called
    /// repeatedly with an expired or almost expired deadline.
    ///
    /// In a situation where new events are generated fast enough by the underlying
    /// implementation to fully saturate event processing, implementations are required to
    /// ensure that return from `process_events_a()` due to deadline expiration cannot be
    /// indefinitely delayed (there must be an upper bound on the number of events that get
    /// processed after expiration).
    ///
    /// "Expose" events (\ref display::WindowEventHandler::on_expose()) are buffered in the
    /// sense that if many actual "expose" events occur at almost the same time, only the
    /// last one will generally be reported to the application. The implementation must
    /// ensure that the delay in the signaling of "expose" events caused by this buffering
    /// mechanism is upwards bounded (bound on number of events that will be processed after
    /// the occurrence of an actual "expose" event and before the signaling of it to the
    /// application). Due to this buffering mechanism, a handler for the "expose" event may
    /// be a good place to perform redrawing of the window's contents, as it generally will
    /// ensure that redrawing is performed no more often than it needs to be. See the notes
    /// on redrawing strategies in the documentation of \ref display::Window.
    ///
    /// Display implementations are required to ensure that an "expose" event is generated
    /// after every generated "resize" event (\ref display::WindowEventHandler::on_resize())
    /// regardless of whether the "resize" event corresponded to an expansion or a shrinkage
    /// of the window area. These "expose" events must be subject to buffering as explained
    /// above. The purpose of these requirements is to expand the number of cases where
    /// redrawing of window contents can be dealt with entirely in the "expose" event
    /// handler.
    ///
    /// "Before sleep" pseudo events (\ref display::ConnectionEventHandler::before_sleep())
    /// are generated immediately before the event processing thread goes to sleep while
    /// waiting for more events to occur. It will also be called regularly in a situation
    /// where event processing is fully saturated so that no sleeping takes place. This
    /// behavior allows the "before sleep" pseudo event to be used as a basis for redrawing
    /// window contents. For more on this, see the notes on redrawing strategies in the
    /// documentation of \ref display::Window.
    ///
    /// Display implementations must ensure that "before sleep" pseudo events are generated
    /// at regular intervals even when there is no time to sleep, and this must be
    /// sufficiently often to allow for reasonable redrawing behavior as described above.
    ///
    /// Applications must assume that the signaling of the expiration of a deadline can be
    /// indefinitely delayed by interruptions of event processing. For example, if the
    /// "before sleep" handler returns `false` every time it is called, repeated invocations
    /// of `process_events_a()` with the same expired deadline may return `false` and
    /// continue to do that for ever, indicating interruption of event processing rather
    /// than deadline expiration. Whether this can happen in practice depends on the
    /// underlying display implementation.
    ///
    /// With two exceptions, event handlers should execute fast, which means that they
    /// should do very little work and never anything that might block the calling
    /// thread. The two exceptions are \ref display::WindowEventHandler::on_expose() and
    /// \ref display::ConnectionEventHandler::before_sleep(). Handlers for these two types
    /// of events can engage in longer running tasks such as redrawing window contents. See
    /// \ref display::Window for notes on redrawing strategies.
    ///
    /// Display implementations are allowed to assume that event handlers other than \ref
    /// display::WindowEventHandler::on_expose() and \ref
    /// display::ConnectionEventHandler::before_sleep() execute fast in the sense explained
    /// above.
    ///
    /// \sa \ref process_events()
    /// \sa \ref display::WindowEventHandler, \ref display::ConectionEventHandler
    /// \sa \ref set_event_handler(), \ref display::Window::set_event_handler()
    ///
    virtual bool process_events_a(time_point_type deadline) = 0;

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
    /// of the specified screen (\p screen). The screen must be non-negative and less than
    /// the number of screens (\ref get_num_screens()).
    ///
    /// A particular display implementation (\ref display::Implementation) is not required
    /// to expose information about the configuration of each of the accessible screens. If
    /// the implementation in use for this connection does not expose this information, this
    /// function returns `false` and leaves all arguments unchanged. Otherwise, this
    /// function returns `true` after placing an entry in \p viewports for each of the
    /// viewports that are currently part of the specified screen, placing associated string
    /// data in \p strings, and setting \p num_viewports to the number of viewports placed
    /// in \p viewports.
    ///
    /// If the implementation exposes the screen configuration, i.e., when this function
    /// returns `true`, the implementation will also generate "screen changed" events
    /// whenever a screen configuration changes (\ref
    /// display::ConnectionEventHandler::on_screen_change()).
    ///
    virtual bool try_get_screen_conf(int screen, core::Buffer<display::Viewport>& viewports,
                                     core::Buffer<char>& strings, std::size_t& num_viewports) const = 0;

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
    /// used to create a connection. The important thing is that the character encodings
    /// agree (`std::codecvt` facet).
    ///
    log::Logger* logger = nullptr;

    /// \brief Parameters specific to X11-based implementation.
    ///
    /// These are the parameters that are specific to the X11-based implementation.
    ///
    display::x11_connection_config x11;

    /// \brief Parameters specific to SDL-based implementation.
    ///
    /// These are the parameters that are specific to the SDL-based implementation.
    ///
    display::sdl_connection_config sdl;
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
