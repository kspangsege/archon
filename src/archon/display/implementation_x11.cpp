// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2023 Kristian Spangsege <kristian.spangsege@gmail.com>
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


                                            
#include <cerrno>
#include <cstdlib>
#include <memory>
#include <string_view>
#include <locale>

#include <archon/core/features.h>
#include <archon/core/deque.hpp>
#include <archon/core/flat_map.hpp>
#include <archon/core/format.hpp>
#include <archon/core/quote.hpp>
#include <archon/core/platform_support.hpp>
#include <archon/display/impl/config.h>
#include <archon/display/implementation.hpp>
#include <archon/display/implementation_x11.hpp>

#if !ARCHON_WINDOWS && ARCHON_DISPLAY_HAVE_X11 && ARCHON_DISPLAY_HAVE_X11_XKB
#  include <unistd.h>
#  if defined _POSIX_VERSION && _POSIX_VERSION >= 200112L // POSIX.1-2001
#    define HAVE_X11 1
#  else
#    define HAVE_X11 0
#  endif
#else
#  define HAVE_X11 0
#endif

#if HAVE_X11
#  include <poll.h>
#  include <X11/Xlib.h>
#  include <X11/Xatom.h>
#  include <X11/Xutil.h>
#  include <X11/keysym.h>
#  include <X11/XKBlib.h>
#  if ARCHON_DISPLAY_HAVE_X11_XDBE
#    include <X11/extensions/Xdbe.h>
#  endif
#  if ARCHON_DISPLAY_HAVE_X11_XRANDR
#    include <X11/extensions/Xrandr.h>
#  endif
#  if ARCHON_DISPLAY_HAVE_X11_XRENDER
#    include <X11/extensions/Xrender.h>
#  endif
#endif


// As of Jan 7, 2024, Release 7.7 is the latest release of X11. It was released on June 6, 2012.
//
//
// Relevant links:
//
// * X11 protocol specification: https://www.x.org/releases/X11R7.7/doc/xproto/x11protocol.html
//
// * X11 API documentation: https://www.x.org/releases/X11R7.7/doc/libX11/libX11/libX11.html
//
// * Inter-Client Communication Conventions Manual: https://x.org/releases/X11R7.7/doc/xorg-docs/icccm/icccm.html
//
// * Extended Window Manager Hints: https://specifications.freedesktop.org/wm-spec/latest/
//
// * Xdbe protocol specification: https://www.x.org/releases/X11R7.7/doc/xextproto/dbe.html
//
// * Xdbe API documentation: https://www.x.org/releases/X11R7.7/doc/libXext/dbelib.html
//
// * XRandR protocol specification: https://www.x.org/releases/X11R7.7/doc/randrproto/randrproto.txt
//
// * XRandR general documentation: https://www.x.org/wiki/libraries/libxrandr/
//
// * XRandR library source code: https://gitlab.freedesktop.org/xorg/lib/libxrandr
//
// * Xrender protocol specification: https://www.x.org/releases/X11R7.7/doc/renderproto/renderproto.txt
//
// * Xrender API documentation: https://www.x.org/releases/X11R7.7/doc/libXrender/libXrender.txt
//

using namespace archon;


namespace {


constexpr std::string_view g_implementation_ident = "x11";


#if HAVE_X11


class WindowImpl;


struct X11Screen {
    bool is_initialized = false;
    bool use_double_buffering = {};
    XVisualInfo visual_info = {};
    Window root = {};
    Colormap colormap = {};
};


class ImplementationImpl
    : public display::Implementation {
public:
    ImplementationImpl(Slot&) noexcept;

    auto new_connection(const std::locale&, const display::Connection::Config&) const ->
        std::unique_ptr<display::Connection> override final;
    bool try_map_key_to_key_code(display::Key, display::KeyCode&) const override final;
    bool try_map_key_code_to_key(display::KeyCode, display::Key&) const override final;
    bool try_get_key_name(display::KeyCode, std::string_view&) const override final;
    auto get_slot() const noexcept -> const Slot& override final;

private:
    const Slot& m_slot;
};


class SlotImpl
    : public display::Implementation::Slot {
public:
    SlotImpl() noexcept;

    auto ident() const noexcept -> std::string_view override final;
    auto get_implementation_a(const display::Guarantees&) const noexcept ->
        const display::Implementation* override final;

private:
    ImplementationImpl m_impl;
};


class ConnectionImpl
    : private display::ConnectionEventHandler
    , public display::Connection {
public:
    const ImplementationImpl& impl;
    const std::locale locale;
    Display* dpy = nullptr;

    Atom atom_wm_protocols;
    Atom atom_wm_delete_window;

    ConnectionImpl(const ImplementationImpl&, const std::locale&, const display::ConnectionConfigX11&) noexcept;
    ~ConnectionImpl() noexcept override;

    void open(const display::ConnectionConfigX11&);
    auto ensure_x11_screen(int screen) -> X11Screen&;
    void register_window(::Window id, WindowImpl&);
    void unregister_window(::Window id) noexcept;

    auto new_window(std::string_view, display::Size, display::WindowEventHandler&,
                    const display::Window::Config&) -> std::unique_ptr<display::Window> override final;
    auto new_window(int, std::string_view, display::Size, display::WindowEventHandler&,
                    const display::Window::Config&) -> std::unique_ptr<display::Window> override final;
    void process_events(display::ConnectionEventHandler*) override final;
    bool process_events(time_point_type, display::ConnectionEventHandler*) override final;
    int get_num_displays() const override final;
    int get_default_display() const override final;
    bool try_get_display_conf(int, core::Buffer<display::Screen>&, core::Buffer<char>&,
                              std::size_t&) const override final;
    auto get_implementation() const noexcept -> const display::Implementation& override final;

private:
    const std::optional<int> m_depth_override;
    const std::optional<VisualID> m_visual_override;
    const bool m_disable_double_buffering;

    bool m_have_xdbe = false;

    std::unique_ptr<X11Screen[]> m_x11_screens;

    core::FlatMap<::Window, WindowImpl&> m_windows;

    // A queue of windows with pending expose events (push to back and pop from
    // front). Windows occur at most once in this queue.
    //
    // INVARIANT: A window is in `m_exposed_windows` if and only if it is in `m_windows` and
    // has `has_pending_expose_event` set to `true`.
    core::Deque<::Window> m_exposed_windows;

    // If `m_have_curr_window` is true, then `m_curr_window` specifies the window identified
    // by `m_curr_window_id`. If `m_have_curr_window` is false, `m_curr_window_id` and
    // `m_curr_window` have no meaning.
    //
    // If `m_have_curr_window` is true, but `m_curr_window` is null, it means that the X
    // client has no knowledge of a window with the ID specified by `m_curr_window_id`. This
    // state is entered if the window specified by `m_curr_window_id` is unregistered
    // (unregister_window()). The state is updated whenever a new window is registered
    // (register_window()), which takes care of the case where a new window reuses the ID
    // specified by `m_curr_window_id`.
    //
    bool m_have_curr_window = false;
    ::Window m_curr_window_id = {};
    WindowImpl* m_curr_window = nullptr;

    int m_num_events = 0;

    auto intern_string(const char*) noexcept -> Atom;
    bool lookup_visual_info(int screen, int depth, VisualID visual_id, XVisualInfo&) const noexcept;
    auto do_new_window(int screen, std::string_view title, display::Size size, display::WindowEventHandler&,
                       const display::Window::Config&) -> std::unique_ptr<display::Window>;
    bool do_process_events(const time_point_type* deadline, display::ConnectionEventHandler*);
    auto lookup_window(::Window window_id) noexcept -> WindowImpl*;
};


class WindowImpl
    : public display::Window {
public:
    ConnectionImpl& conn;
    const X11Screen& screen;
    display::WindowEventHandler& event_handler;
    const int cookie;

    ::Window win = {};
    GC gc = {};

    bool has_pending_expose_event = false;

    WindowImpl(ConnectionImpl&, const X11Screen&, display::WindowEventHandler&, int cookie) noexcept;
    ~WindowImpl() noexcept override;

    void create(display::Size size, const Config&);

    void show() override final;
    void hide() override final;
    void set_title(std::string_view) override final;
    void set_size(display::Size) override final;
    void set_fullscreen_mode(bool) override final;
    void fill(util::Color) override final;
    void fill(util::Color, const display::Box&) override final;
    auto new_texture(display::Size) -> std::unique_ptr<display::Texture> override final;
    void put_texture(const display::Texture&, const display::Pos&) override final;
    void put_texture(const display::Texture&, const display::Box&, const display::Pos&) override final;
    void present() override final;
    void opengl_make_current() override final;
    void opengl_swap_buffers() override final;

private:
    bool m_have_window = false;
    bool m_is_registered = false;
    bool m_is_double_buffered = false;

    Drawable m_drawable;
#if ARCHON_DISPLAY_HAVE_X11_XDBE
    XdbeSwapAction m_swap_action;
#endif

    void set_property(Atom name, Atom value) noexcept;
    void do_fill(util::Color color, int x, int y, unsigned w, unsigned h);
    auto ensure_graphics_context() noexcept -> GC;
    auto create_graphics_context() noexcept -> GC;
    auto intern_color(util::Color) -> unsigned long;
};



inline ImplementationImpl::ImplementationImpl(Slot& slot) noexcept
    : m_slot(slot)
{
}


auto ImplementationImpl::new_connection(const std::locale& locale, const display::Connection::Config& config) const ->
    std::unique_ptr<display::Connection>
{
    auto conn = std::make_unique<ConnectionImpl>(*this, locale, config.x11); // Throws
    conn->open(config.x11); // Throws
    return conn;
}


bool ImplementationImpl::try_map_key_to_key_code(display::Key key, display::KeyCode& key_code) const
{
    static_cast<void>(key);    
    static_cast<void>(key_code);    
    return false;                      
}


bool ImplementationImpl::try_map_key_code_to_key(display::KeyCode key_code, display::Key& key) const
{
    static_cast<void>(key_code);    
    static_cast<void>(key);    
    return false;                      
}


bool ImplementationImpl::try_get_key_name(display::KeyCode key_code, std::string_view& name) const
{
    static_cast<void>(key_code);    
    static_cast<void>(name);    
    return false;                      
}


auto ImplementationImpl::get_slot() const noexcept -> const Slot&
{
    return m_slot;
}



inline SlotImpl::SlotImpl() noexcept
    : m_impl(*this)
{
}


auto SlotImpl::ident() const noexcept -> std::string_view
{
    return g_implementation_ident;
}


auto SlotImpl::get_implementation_a(const display::Guarantees& guarantees) const noexcept ->
    const display::Implementation*
{
    bool is_available = (guarantees.no_other_use_of_x11 &&
                         guarantees.main_thread_exclusive);
    if (ARCHON_LIKELY(is_available))
        return &m_impl;
    return nullptr;
}



inline ConnectionImpl::ConnectionImpl(const ImplementationImpl& impl_2, const std::locale& locale_2,
                                      const display::ConnectionConfigX11& config) noexcept
    : impl(impl_2)
    , locale(locale_2)
    , m_depth_override(config.depth)
    , m_visual_override(config.visual)
    , m_disable_double_buffering(config.disable_double_buffering)
{
}


ConnectionImpl::~ConnectionImpl() noexcept
{
    if (dpy)
        XCloseDisplay(dpy);
}


void ConnectionImpl::open(const display::ConnectionConfigX11& config)
{
    std::string display_name;
    if (!config.display.empty()) {
        display_name = std::string(config.display); // Throws
    }
    else if (char* val = std::getenv("DISPLAY")) {
        display_name = std::string(val); // Throws
    }
    Display* dpy_2 = XOpenDisplay(display_name.data());
    if (ARCHON_UNLIKELY(!dpy_2)) {
        std::string message = core::format(locale, "Failed to open X11 display connection (%s)",
                                           core::quoted(std::string_view(display_name))); // Throws
        throw std::runtime_error(message);
    }

    dpy = dpy_2;

    if (config.synchronous_mode)
        XSynchronize(dpy, True);

    atom_wm_protocols     = intern_string("WM_PROTOCOLS");
    atom_wm_delete_window = intern_string("WM_DELETE_WINDOW");

#if ARCHON_DISPLAY_HAVE_X11_XDBE
    {
        int major = 0;
        int minor = 0;
        if (XdbeQueryExtension(dpy, &major, &minor)) {
            if (major >= 1)
                m_have_xdbe = true;
        }
    }
#endif // ARCHON_DISPLAY_HAVE_X11_XDBE
}


auto ConnectionImpl::ensure_x11_screen(int screen) -> X11Screen&
{
    if (ARCHON_UNLIKELY(!m_x11_screens))
        m_x11_screens = std::make_unique<X11Screen[]>(std::size_t(ScreenCount(dpy))); // Throws
    X11Screen& screen_2 = m_x11_screens[screen];
    if (ARCHON_UNLIKELY(!screen_2.is_initialized)) {
        VisualID default_visualid = XVisualIDFromVisual(DefaultVisual(dpy, screen));
        XVisualInfo visual_info = {};
        {
            int depth = DefaultDepth(dpy, screen);
            VisualID visual = default_visualid;
            if (m_depth_override.has_value())
                depth = m_depth_override.value();
            if (m_visual_override.has_value())
                visual = m_visual_override.value();
            if (ARCHON_UNLIKELY(!lookup_visual_info(screen, depth, visual, visual_info))) {
                ARCHON_STEADY_ASSERT(m_depth_override.has_value() || m_visual_override.has_value());
                std::string message = core::format(locale, "Combination of selected depth (%s) and selected visual "
                                                   "type (0x%s) is invalid for targeted screen (%s)", depth,
                                                   core::as_hex_int(visual, 2), screen); // Throws
                throw std::runtime_error(message);
            }
        }

        ::Window root = RootWindow(dpy, screen);
        Colormap colormap = DefaultColormap(dpy, screen);
        ARCHON_SCOPE_EXIT {
            if (colormap != None)
                XFreeColormap(dpy, colormap);
        };
        if (ARCHON_UNLIKELY(visual_info.visualid != default_visualid)) {
            // By creating a new colormap, rather than reusing the one used by the root
            // window, it becomes possible to use a visual for the new window that differs
            // from the one used by the root window. The colormap and the new window must
            // agree on visual.
            colormap = XCreateColormap(dpy, root, visual_info.visual, AllocNone);
        }

        bool use_double_buffering = false;
#if ARCHON_DISPLAY_HAVE_X11_XDBE
        if (!m_disable_double_buffering && m_have_xdbe) {
            int n = 1;
            XdbeScreenVisualInfo* entries = XdbeGetVisualInfo(dpy, &root, &n);
            if (ARCHON_UNLIKELY(!entries))
                throw std::runtime_error("XdbeGetVisualInfo() failed");
            ARCHON_SCOPE_EXIT {
                XdbeFreeVisualInfo(entries);
            };
            const XdbeScreenVisualInfo& entry = entries[0];
            for (int i = 0; i < entry.count; ++i) {
                XdbeVisualInfo& subentry = entry.visinfo[i];
                bool is_match = (subentry.depth == visual_info.depth && subentry.visual == visual_info.visualid);
                if (is_match) {
                    use_double_buffering = true;
                    break;
                }
            }
        }
#endif // ARCHON_DISPLAY_HAVE_X11_XDBE

        screen_2.use_double_buffering = use_double_buffering;
        screen_2.visual_info = visual_info;
        screen_2.root = root;
        screen_2.colormap = colormap;
        screen_2.is_initialized = true;
        colormap = None;
    }
    return screen_2;
}


inline void ConnectionImpl::register_window(::Window id, WindowImpl& window)
{
    auto i = m_windows.emplace(id, window); // Throws
    bool was_inserted = i.second;
    ARCHON_ASSERT(was_inserted);
    // Because a new window might reuse the ID currently specified by `m_curr_window_id`, it
    // is necessary, and not just desirable to reset the "current window state" here.
    m_curr_window_id = id;
    m_curr_window = &window;
    m_have_curr_window = true;
}


inline void ConnectionImpl::unregister_window(::Window id) noexcept
{
    std::size_t n = m_windows.erase(id);
    ARCHON_ASSERT(n == 1);
    if (ARCHON_LIKELY(m_have_curr_window && id == m_curr_window_id))
        m_curr_window = nullptr;
}


auto ConnectionImpl::new_window(std::string_view title, display::Size size,
                                display::WindowEventHandler& event_handler,
                                const display::Window::Config& config) -> std::unique_ptr<display::Window>
{
    return do_new_window(DefaultScreen(dpy), title, size, event_handler, config); // Throws
}


auto ConnectionImpl::new_window(int display, std::string_view title, display::Size size,
                                display::WindowEventHandler& event_handler,
                                const display::Window::Config& config) -> std::unique_ptr<display::Window>
{
    if (ARCHON_UNLIKELY(display < 0 || display >= int(ScreenCount(dpy))))
        throw std::invalid_argument("Bad display index");
    return do_new_window(display, title, size, event_handler, config); // Throws
}


void ConnectionImpl::process_events(display::ConnectionEventHandler* connection_event_handler)
{
    const time_point_type* deadline = nullptr;
    do_process_events(deadline, connection_event_handler); // Throws
}


bool ConnectionImpl::process_events(time_point_type deadline,
                                    display::ConnectionEventHandler* connection_event_handler)
{
    return do_process_events(&deadline, connection_event_handler); // Throws
}


int ConnectionImpl::get_num_displays() const
{
    return int(ScreenCount(dpy));
}


int ConnectionImpl::get_default_display() const
{
    return int(DefaultScreen(dpy));
}


bool ConnectionImpl::try_get_display_conf(int display, core::Buffer<display::Screen>&, core::Buffer<char>&,
                                          std::size_t&) const
{
    if (ARCHON_UNLIKELY(display < 0 || display >= int(ScreenCount(dpy))))
        throw std::invalid_argument("Bad display index");
    return false;                       
}


auto ConnectionImpl::get_implementation() const noexcept -> const display::Implementation&
{
    return impl;
}


inline auto ConnectionImpl::intern_string(const char* string) noexcept -> Atom
{
    Atom atom = XInternAtom(dpy, string, False);
    ARCHON_ASSERT(atom != None);
    return atom;
}


bool ConnectionImpl::lookup_visual_info(int screen, int depth, VisualID visual_id,
                                        XVisualInfo& visual_info) const noexcept
{
    int n = 0;
    long vinfo_mask = VisualScreenMask | VisualDepthMask | VisualIDMask;
    XVisualInfo vinfo_template;
    vinfo_template.screen = screen;
    vinfo_template.depth = depth;
    vinfo_template.visualid = visual_id;
    XVisualInfo* entries = XGetVisualInfo(dpy, vinfo_mask, &vinfo_template, &n);
    if (ARCHON_LIKELY(entries)) {
        ARCHON_STEADY_ASSERT(n == 1);
        visual_info = entries[0];
        XFree(entries);
        return true;
    }
    return false;
}


auto ConnectionImpl::do_new_window(int screen, std::string_view title, display::Size size,
                                   display::WindowEventHandler& event_handler,
                                   const display::Window::Config& config) -> std::unique_ptr<display::Window>
{
    if (ARCHON_UNLIKELY(size.width < 0 || size.height < 0))
        throw std::invalid_argument("Bad window size");
    const X11Screen& screen_2 = ensure_x11_screen(screen); // Throws
    auto win = std::make_unique<WindowImpl>(*this, screen_2, event_handler, config.cookie); // Throws
    win->create(size, config); // Throws
    win->set_title(title); // Throws
    if (ARCHON_UNLIKELY(config.fullscreen))
        win->set_fullscreen_mode(true); // Throws
    return win;
}


bool ConnectionImpl::do_process_events(const time_point_type* deadline,
                                       display::ConnectionEventHandler* connection_event_handler)
{
    // This function takes care to meet the following requirements:
    //
    // - XFlush() must be called before waiting (poll()) whenever there is a chance that
    //   there are unflushed commands.
    //
    // - XEventsQueued() must be called immediately before waiting (poll()) to ensure that
    //   there are no events that are already queued (must be called after Xflush()).
    //
    // - There must be no way for the execution of WindowEventHandler::on_expose() and
    //   ConnectionEventHandler::before_sleep() to be starved indefinitely by event
    //   saturation. This is ensured by fully exhausting one batch of events at a time
    //   (m_remaining_events_in_batch).
    //
    // - There must be no way for the return from do_process_events() due to expiration of
    //   the deadline to be starved indefinitely by event saturation. This is ensured by
    //   fully exhausting one batch of events at a time (m_remaining_events_in_batch).
    //

    display::ConnectionEventHandler& connection_event_handler_2 =
        (connection_event_handler ? *connection_event_handler : *this);

    XEvent ev;

  process_1:
    if (ARCHON_LIKELY(m_num_events > 0))
        goto process_2;
    goto post;

  process_2:
    XNextEvent(dpy, &ev);
    --m_num_events;
    switch (ev.type) {
        case ConfigureNotify: {
            WindowImpl* window = lookup_window(ev.xconfigure.window);
            if (ARCHON_LIKELY(window)) {
                // When there is a window manager, the window manager will generally
                // re-parent the client's window. This generally means that the client's
                // window will remain at a fixed position relative to it's parent, so there
                // will be no configure notifications when the window is moved through user
                // interaction. Also, if the user's window is moved relative to its parent,
                // the reported position will be unreliable, as it will be relative to its
                // parent, which is not the root window of the screen. Fortunately, in all
                // those cases, the window manager is obligated to generate synthetic
                // configure notifications in which the positions are absolute (relative to
                // the root window of the screen).
                if (ev.xconfigure.send_event) {
                    display::WindowPosEvent event;
                    event.cookie = window->cookie;
                    event.pos = { ev.xconfigure.x, ev.xconfigure.y };
                    bool proceed = window->event_handler.on_reposition(event); // Throws
                    if (ARCHON_LIKELY(!proceed))
                        return false; // Interrupt
                }
                else {
                    display::WindowSizeEvent event;
                    event.cookie = window->cookie;
                    event.size = { ev.xconfigure.width, ev.xconfigure.height };
                    bool proceed = window->event_handler.on_resize(event); // Throws
                    if (ARCHON_LIKELY(!proceed))
                        return false; // Interrupt
                }
            }
            break;
        }
        case Expose: {
            WindowImpl* window = lookup_window(ev.xexpose.window);
            if (ARCHON_LIKELY(!window || window->has_pending_expose_event))
                break;
            m_exposed_windows.push_back(ev.xexpose.window); // Throws
            window->has_pending_expose_event = true;
            break;
        }
        case ClientMessage: {
            bool is_close = (ev.xclient.format == 32 && Atom(ev.xclient.data.l[0]) == atom_wm_delete_window);
            if (ARCHON_LIKELY(is_close)) {
                const WindowImpl* window = lookup_window(ev.xclient.window);
                if (ARCHON_LIKELY(window)) {
                    display::WindowEvent event;
                    event.cookie = window->cookie;
                    bool proceed = window->event_handler.on_close(event); // Throws
                    if (ARCHON_LIKELY(!proceed))
                        return false; // Interrupt
                }
            }
            break;
        }
    }
    goto process_1;

  post:
    for (;;) {
        if (ARCHON_LIKELY(m_exposed_windows.empty()))
            break;
        ::Window win = m_exposed_windows.front();
        m_exposed_windows.pop_front();
        WindowImpl* window = lookup_window(win);
        if (ARCHON_LIKELY(window)) {
            window->has_pending_expose_event = false;
            display::WindowEvent event;
            event.cookie = window->cookie;
            bool proceed = window->event_handler.on_expose(event); // Throws
            if (ARCHON_LIKELY(!proceed))
                return false; // Interrupt
        }
    }
    {
        bool proceed = connection_event_handler_2.before_sleep(); // Throws
        if (ARCHON_UNLIKELY(!proceed))
            return false; // Interrupt
    }
    XFlush(dpy);

  read:
    m_num_events = XEventsQueued(dpy, QueuedAfterReading); // Non-blocking

  wait:
    {
        int timeout = -1;
        bool complete = false;
        if (ARCHON_LIKELY(deadline)) {
            time_point_type now = clock_type::now();
            if (ARCHON_LIKELY(*deadline > now))
                goto not_expired;
            return true; // Deadline expired
          not_expired:
            auto duration = std::chrono::ceil<std::chrono::milliseconds>(*deadline - now).count();
            timeout = core::int_max<int>();
            if (ARCHON_LIKELY(core::int_less_equal(duration, timeout))) {
                timeout = int(duration);
                complete = true;
            }
        }

        if (ARCHON_LIKELY(m_num_events > 0))
            goto process_2;

        pollfd fds[1] {};
        int nfds = 1;
        fds[0].fd = ConnectionNumber(dpy);
        fds[0].events = POLLIN;
        int ret, err;
      poll:
        ret = ::poll(fds, nfds, timeout);
        if (ARCHON_LIKELY(ret > 0)) {
            ARCHON_ASSERT(ret == 1);
            goto read;
        }
        if (ARCHON_LIKELY(ret == 0)) {
            ARCHON_ASSERT(timeout < 0);
            if (ARCHON_LIKELY(complete))
                return true; // Deadline expired
            goto wait;
        }
        err = errno; // Eliminate any risk of clobbering
        if (ARCHON_LIKELY(err == EINTR))
            goto poll;
        core::throw_system_error(err, "Failed to poll file descriptor of X11 connection"); // Throws
    }
}


auto ConnectionImpl::lookup_window(::Window window_id) noexcept -> WindowImpl*
{
    if (ARCHON_LIKELY(m_have_curr_window && window_id == m_curr_window_id))
        return m_curr_window;

    WindowImpl* window = nullptr;
    auto i = m_windows.find(window_id);
    if (ARCHON_LIKELY(i != m_windows.end()))
        window = &i->second;
    m_curr_window_id = window_id;
    m_curr_window = window;
    m_have_curr_window = true;
    return window;
}



inline WindowImpl::WindowImpl(ConnectionImpl& conn_2, const X11Screen& screen_2,
                              display::WindowEventHandler& event_handler_2, int cookie_2) noexcept
    : conn(conn_2)
    , screen(screen_2)
    , event_handler(event_handler_2)
    , cookie(cookie_2)
{
}


WindowImpl::~WindowImpl() noexcept
{
    if (ARCHON_LIKELY(m_have_window)) {
        if (ARCHON_LIKELY(m_is_registered)) {
            if (gc)
                XFreeGC(conn.dpy, gc);
            conn.unregister_window(win);
        }
        XDestroyWindow(conn.dpy, win);
    }
}


void WindowImpl::create(display::Size size, const Config& config)
{
    display::Size adjusted_size = size;
    bool has_minimum_size = (config.resizable && config.minimum_size.has_value());
    if (has_minimum_size)
        adjusted_size = max(adjusted_size, config.minimum_size.value());

    ::Window parent = screen.root;
    int x = 0, y = 0;
    unsigned width  = unsigned(adjusted_size.width);
    unsigned height = unsigned(adjusted_size.height);
    unsigned border_width = 0;
    int depth = screen.visual_info.depth;
    unsigned class_ = InputOutput;
    Visual* visual = screen.visual_info.visual;
    unsigned long valuemask = CWEventMask | CWColormap;
    XSetWindowAttributes attributes = {};
    attributes.event_mask = (KeyPressMask | ExposureMask | StructureNotifyMask |
                             ButtonMotionMask | ButtonPressMask | ButtonReleaseMask);                            
    attributes.colormap = screen.colormap;
    win = XCreateWindow(conn.dpy, parent, x, y, width, height, border_width, depth, class_, visual,
                        valuemask, &attributes);
    m_have_window = true;

    conn.register_window(win, *this); // Throws
    m_is_registered = true;

    // Ask X server to notify rather than close connection when window is closed
    set_property(conn.atom_wm_protocols, conn.atom_wm_delete_window);

    // Disable resizability if requested
    if (!config.resizable) {
        XSizeHints size_hints;
        size_hints.flags = PMinSize | PMaxSize;
        size_hints.min_width  = adjusted_size.width;
        size_hints.min_height = adjusted_size.height;
        size_hints.max_width  = adjusted_size.width;
        size_hints.max_height = adjusted_size.height;
        XSetWMSizeHints(conn.dpy, win, &size_hints, XA_WM_NORMAL_HINTS);
    }

    // Set minimum window size if requested
    if (has_minimum_size) {
        display::Size min_size = config.minimum_size.value();
        XSizeHints size_hints;
        size_hints.flags = PMinSize;
        size_hints.min_width  = min_size.width;
        size_hints.min_height = min_size.height;
        XSetWMSizeHints(conn.dpy, win, &size_hints, XA_WM_NORMAL_HINTS);
    }

    // Enable double buffering
    m_drawable = win;
#if ARCHON_DISPLAY_HAVE_X11_XDBE
    if (ARCHON_LIKELY(screen.use_double_buffering)) {
        m_swap_action = XdbeUndefined; // Contents of swapped-out buffer becomes undefined
        XdbeBackBuffer back_buffer = XdbeAllocateBackBufferName(conn.dpy, win, m_swap_action);
        m_drawable = back_buffer;
        m_is_double_buffered = true;
    }
#endif // ARCHON_DISPLAY_HAVE_X11_XDBE

    // FIXME: Tend to config.enable_opengl                                                                                                                                                       
}


void WindowImpl::show()
{
    XMapWindow(conn.dpy, win);
}


void WindowImpl::hide()
{
    XUnmapWindow(conn.dpy, win);
}


void WindowImpl::set_title(std::string_view title)
{
    // FIXME: Tend to character encoding. How does SDL do it? Doe it support UTF-8? See also https://tronche.com/gui/x/xlib/ICC/client-to-window-manager/XmbTextListToTextProperty.html              
    // FIXME: Consider placing a reusable string buffer in ConnectionImpl for use cases like the one below      
    std::string name_1 = std::string(title); // Throws
    char* name_2 = name_1.data();
    XTextProperty name_3;
    Status status = XStringListToTextProperty(&name_2, 1, &name_3);
    if (ARCHON_UNLIKELY(status == 0))
        throw std::runtime_error("XStringListToTextProperty() failed");
    XSetWMName(conn.dpy, win, &name_3);                             
    XFree(name_3.value);
}


void WindowImpl::set_size(display::Size size)
{
    static_cast<void>(size);    
    throw std::runtime_error("*click* -> set_size()");     
}


void WindowImpl::set_fullscreen_mode(bool on)
{
    static_cast<void>(on);    
    throw std::runtime_error("*click* -> set_fullscreen_mode()");     
}


void WindowImpl::fill(util::Color color)
{
    int x = 0;
    int y = 0;
    unsigned w = core::int_max<unsigned>();
    unsigned h = core::int_max<unsigned>();
    do_fill(color, x, y, w, h); // Throws
}


void WindowImpl::fill(util::Color color, const display::Box& area)
{
    if (ARCHON_LIKELY(area.is_valid())) {
        int x = area.pos.x;
        int y = area.pos.y;
        unsigned w = unsigned(area.size.width);
        unsigned h = unsigned(area.size.height);
        do_fill(color, x, y, w, h); // Throws
        return;
    }
    throw std::invalid_argument("Fill area");
}


auto WindowImpl::new_texture(display::Size size) -> std::unique_ptr<display::Texture>
{
    static_cast<void>(size);    
    throw std::runtime_error("*click* -> new_texture()");     
}


void WindowImpl::put_texture(const display::Texture& tex, const display::Pos& pos)
{
    static_cast<void>(tex);    
    static_cast<void>(pos);    
    throw std::runtime_error("*click* -> put_texture(1/2)");     
}


void WindowImpl::put_texture(const display::Texture& tex, const display::Box& source_area, const display::Pos& pos)
{
    static_cast<void>(tex);    
    static_cast<void>(source_area);    
    static_cast<void>(pos);    
    throw std::runtime_error("*click* -> put_texture(2/2)");     
}


void WindowImpl::present()
{
#if ARCHON_DISPLAY_HAVE_X11_XDBE
    if (m_is_double_buffered) {
        XdbeSwapInfo info;
        info.swap_window = win;
        info.swap_action = m_swap_action;
        Status status = XdbeSwapBuffers(conn.dpy, &info, 1);
        if (ARCHON_UNLIKELY(status == 0))
            throw std::runtime_error("XdbeSwapBuffers() failed");
    }
#endif // ARCHON_DISPLAY_HAVE_X11_XDBE
}


void WindowImpl::opengl_make_current()
{
    throw std::runtime_error("*click* -> opengl_make_current()");     
}


void WindowImpl::opengl_swap_buffers()
{
    throw std::runtime_error("*click* -> opengl_swap_buffers()");     
}


void WindowImpl::set_property(Atom name, Atom value) noexcept
{
    XChangeProperty(conn.dpy, win, name, XA_ATOM, 32, PropModeReplace, reinterpret_cast<unsigned char*>(&value), 1);
}


void WindowImpl::do_fill(util::Color color, int x, int y, unsigned w, unsigned h)
{
    GC gc = ensure_graphics_context();
    unsigned long color_2 = intern_color(color); // Throws
    XSetForeground(conn.dpy, gc, color_2);
    XFillRectangle(conn.dpy, m_drawable, gc, x, y, w, h);
}


inline auto WindowImpl::ensure_graphics_context() noexcept -> GC
{
    if (ARCHON_LIKELY(gc))
        return gc;
    return create_graphics_context();
}


auto WindowImpl::create_graphics_context() noexcept -> GC
{
    unsigned long valuemask = GCGraphicsExposures;
    XGCValues values = {};
    values.graphics_exposures = False;
    gc = XCreateGC(conn.dpy, m_drawable, valuemask, &values);
    return gc;
}


auto WindowImpl::intern_color(util::Color) -> unsigned long
{
    // StaticColor:
    // PseudoColor:
    // StaticGray:
    // GrayScale:
    // TrueColor and DirectColor:
    //   - RGB masks
    //   - Masks are available through XVisualInfo
    //   - Masks are guaranteed to consist of contiguous bit positions

    

    return WhitePixel(conn.dpy, screen.visual_info.screen);                                                                                                                                                                        
}


#else // !HAVE_X11


class SlotImpl
    : public display::Implementation::Slot {
public:
    auto ident() const noexcept -> std::string_view override final;
    auto get_implementation_a(const display::Guarantees&) const noexcept ->
        const display::Implementation* override final;
};


auto SlotImpl::ident() const noexcept -> std::string_view
{
    return g_implementation_ident;
}


auto SlotImpl::get_implementation_a(const display::Guarantees&) const noexcept -> const display::Implementation*
{
    return nullptr;
}


#endif // !HAVE_X11


} // unnamed namespace


auto display::get_x11_implementation_slot() noexcept -> const display::Implementation::Slot&
{
    static SlotImpl slot;
    return slot;
}
