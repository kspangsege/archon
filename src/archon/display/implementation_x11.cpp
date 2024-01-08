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
#include <archon/core/flat_map.hpp>
#include <archon/core/format.hpp>
#include <archon/core/quote.hpp>
#include <archon/core/platform_support.hpp>
#include <archon/display/impl/config.h>
#include <archon/display/implementation.hpp>
#include <archon/display/implementation_x11.hpp>
#include <archon/log.hpp>

#if !ARCHON_WINDOWS && ARCHON_DISPLAY_HAVE_X11
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
#  if ARCHON_DISPLAY_HAVE_XRANDR
#    include <X11/extensions/Xrandr.h>
#  endif
#  if ARCHON_DISPLAY_HAVE_XRENDER
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
// * RandR extension: https://www.x.org/releases/X11R7.7/doc/randrproto/randrproto.txt
//
// * RandR general documentation: https://www.x.org/wiki/libraries/libxrandr/
//
// * RandR library source code: https://gitlab.freedesktop.org/xorg/lib/libxrandr
//
// * Xrender extension: https://www.x.org/releases/X11R7.7/doc/renderproto/renderproto.txt
//
// * Xrender API documentation: https://www.x.org/releases/X11R7.7/doc/libXrender/libXrender.txt
//

using namespace archon;


namespace {


constexpr std::string_view g_implementation_ident = "x11";


#if HAVE_X11


struct X11Screen {
    bool is_initialized = false;
    Window root = {};
    int depth = {};
    Visual* visual = {};
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
    bool check_depth_visual_combi(int screen, int dept, VisualID visual_id, Visual*& visual) const noexcept;
    void register_window(::Window id, display::WindowEventHandler&, int cookie);
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

    std::unique_ptr<X11Screen[]> m_x11_screens;

    struct WindowEntry {
        display::WindowEventHandler& event_handler;
        int cookie;
    };

    core::FlatMap<::Window, WindowEntry> m_window_entries;

    // If `m_have_curr_window_id` is true, then `m_curr_window_entry` specifies the entry
    // for the window specified by `m_curr_window_id`. If `m_have_curr_window_id` is false,
    // `m_curr_window_id` and `m_curr_window_entry` have no meaning.
    //
    // If `m_have_curr_window_id` is true, but `m_curr_window_entry` is null, it means that
    // the X client has no knowledge of a window with the ID specified by
    // `m_curr_window_id`. This state is entered if the window specified by
    // `m_curr_window_id` is unregistered (unregister_window()). The state is updated
    // whenever a new window is registered (register_window()), which takes care of the case
    // where a new window reuses the ID specified by `m_curr_window_id`.
    //
    bool m_have_curr_window_id = false;
    ::Window m_curr_window_id = {};
    const WindowEntry* m_curr_window_entry = nullptr;

    auto intern(const char*) noexcept -> Atom;

    auto do_new_window(int screen, std::string_view title, display::Size size, display::WindowEventHandler&,
                       const display::Window::Config&) -> std::unique_ptr<display::Window>;

    bool process_outstanding_events(display::ConnectionEventHandler&);
    void wait_for_events();

    auto lookup_window_entry(::Window window_id) noexcept -> const WindowEntry*;
};


class WindowImpl
    : public display::Window {
public:
    ConnectionImpl& conn;
    ::Window win = {};

    WindowImpl(ConnectionImpl&) noexcept;
    ~WindowImpl() noexcept override;

    void create(const X11Screen&, display::Size, display::WindowEventHandler&, const Config&);

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

    void set_property(Atom name, Atom value) noexcept;
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

    atom_wm_protocols     = intern("WM_PROTOCOLS");
    atom_wm_delete_window = intern("WM_DELETE_WINDOW");
}


auto ConnectionImpl::ensure_x11_screen(int screen) -> X11Screen&
{
    if (ARCHON_UNLIKELY(!m_x11_screens))
        m_x11_screens = std::make_unique<X11Screen[]>(std::size_t(ScreenCount(dpy))); // Throws
    X11Screen& screen_2 = m_x11_screens[screen];
    if (ARCHON_UNLIKELY(!screen_2.is_initialized)) {
        ::Window root = RootWindow(dpy, screen);
        int depth = DefaultDepth(dpy, screen);
        VisualID default_visual_id = XVisualIDFromVisual(DefaultVisual(dpy, screen));
        VisualID visual_id = default_visual_id;
        if (m_depth_override.has_value())
            depth = m_depth_override.value();
        if (m_visual_override.has_value())
            visual_id = m_visual_override.value();
        Visual* visual = {};
        if (ARCHON_UNLIKELY(!check_depth_visual_combi(screen, depth, visual_id, visual))) {
            ARCHON_STEADY_ASSERT(m_depth_override.has_value() || m_visual_override.has_value());
            std::string message = core::format(locale, "Combination of selected depth (%s) and selected visual type "
                                               "(0x%s) is invalid for targeted screen (%s)", depth,
                                               core::as_hex_int(visual_id, 2), screen); // Throws
            throw std::runtime_error(message);
        }
        Colormap colormap = DefaultColormap(dpy, screen);
        if (ARCHON_UNLIKELY(visual_id != default_visual_id)) {
            // By creating a new colormap, rather than reusing the one used by the root
            // window, it becomes possible to use a visual for the new window that differs
            // from the one used by the root window. The colormap and the new window must
            // agree on visual.
            colormap = XCreateColormap(dpy, root, visual, AllocNone);
        }
        screen_2.root = root;
        screen_2.depth = depth;
        screen_2.visual = visual;
        screen_2.colormap = colormap;
        screen_2.is_initialized = true;
    }
    return screen_2;
}


bool ConnectionImpl::check_depth_visual_combi(int screen, int depth, VisualID visual_id,
                                               Visual*& visual) const noexcept
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
        visual = entries[0].visual;
        XFree(entries);
        return true;
    }
    return false;
}


inline void ConnectionImpl::register_window(::Window id, display::WindowEventHandler& event_handler, int cookie)
{
    WindowEntry entry = { event_handler, cookie };
    auto i = m_window_entries.emplace(id, entry); // Throws
    bool was_inserted = i.second;
    ARCHON_ASSERT(was_inserted);
    // Because a new window might reuse the ID currently specified by `m_curr_window_id`, it
    // is necessary, and not just desirable to reset the "current window state" here.
    m_curr_window_id = id;
    m_curr_window_entry = &i.first->second;
    m_have_curr_window_id = true;
}


inline void ConnectionImpl::unregister_window(::Window id) noexcept
{
    std::size_t n = m_window_entries.erase(id);
    ARCHON_ASSERT(n == 1);
    if (ARCHON_LIKELY(m_have_curr_window_id && id == m_curr_window_id))
        m_curr_window_entry = nullptr;
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
    display::ConnectionEventHandler& connection_event_handler_2 =
        (connection_event_handler ? *connection_event_handler : *this);
  again:
    if (ARCHON_LIKELY(process_outstanding_events(connection_event_handler_2))) { // Throws
        wait_for_events(); // Throws
        goto again;
    }
}


bool ConnectionImpl::process_events(time_point_type deadline,
                                    display::ConnectionEventHandler* connection_event_handler)
{
    static_cast<void>(deadline);    
    static_cast<void>(connection_event_handler);    
    throw std::runtime_error("*click*");     
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


inline auto ConnectionImpl::intern(const char* string) noexcept -> Atom
{
    Atom atom = XInternAtom(dpy, string, False);
    ARCHON_ASSERT(atom != None);
    return atom;
}


auto ConnectionImpl::do_new_window(int screen, std::string_view title, display::Size size,
                                   display::WindowEventHandler& event_handler,
                                   const display::Window::Config& config) -> std::unique_ptr<display::Window>
{
    if (ARCHON_UNLIKELY(size.width < 0 || size.height < 0))
        throw std::invalid_argument("Bad window size");
    const X11Screen& screen_2 = ensure_x11_screen(screen); // Throws
    auto win = std::make_unique<WindowImpl>(*this); // Throws
    win->create(screen_2, size, event_handler, config); // Throws
    win->set_title(title); // Throws
    if (ARCHON_UNLIKELY(config.fullscreen))
        win->set_fullscreen_mode(true); // Throws
    return win;
}


bool ConnectionImpl::process_outstanding_events(display::ConnectionEventHandler& connection_event_handler)
{
    XEvent ev;

    int i = 0;
    int n = 0;

    goto read_events;

  next_event_1:
    if (i < n)
        goto next_event_2;

  read_events:
    ARCHON_ASSERT(i == n);
    i = 0;
    n = XEventsQueued(dpy, QueuedAfterReading); // Non-blocking read
    if (ARCHON_LIKELY(n > 0))
        goto next_event_2;
    goto exhausted;

  next_event_2:
    ARCHON_ASSERT(i < n);
    XNextEvent(dpy, &ev);
    ++i;
    switch (ev.type) {
        case ConfigureNotify: {
            const WindowEntry* entry = lookup_window_entry(ev.xconfigure.window);
            if (ARCHON_LIKELY(entry)) {
                // When there is a window manager, the window manager will generally
                // reparent the client's window. This generally means that the client's
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
                    event.cookie = entry->cookie;
                    event.pos = { ev.xconfigure.x, ev.xconfigure.y };
                    bool proceed = entry->event_handler.on_reposition(event); // Throws
                    if (ARCHON_LIKELY(!proceed))
                        return false; // Interrupt
                }
                else {
                    display::WindowSizeEvent event;
                    event.cookie = entry->cookie;
                    event.size = { ev.xconfigure.width, ev.xconfigure.height };
                    bool proceed = entry->event_handler.on_resize(event); // Throws
                    if (ARCHON_LIKELY(!proceed))
                        return false; // Interrupt
                }
            }
            break;
        }
        case Expose: {
            const WindowEntry* entry = lookup_window_entry(ev.xexpose.window);
            if (ARCHON_LIKELY(entry)) {
                display::WindowEvent event;
                event.cookie = entry->cookie;
                bool proceed = entry->event_handler.on_expose(event); // Throws
                if (ARCHON_LIKELY(!proceed))
                    return false; // Interrupt
            }
            break;
        }
        case ClientMessage: {
            bool is_close = (ev.xclient.format == 32 && Atom(ev.xclient.data.l[0]) == atom_wm_delete_window);
            if (ARCHON_LIKELY(is_close)) {
                const WindowEntry* entry = lookup_window_entry(ev.xclient.window);
                if (ARCHON_LIKELY(entry)) {
                    display::WindowEvent event;
                    event.cookie = entry->cookie;
                    bool proceed = entry->event_handler.on_close(event); // Throws
                    if (ARCHON_LIKELY(!proceed))
                        return false; // Interrupt
                }
            }
            break;
        }
    }
    goto next_event_1;

  exhausted:
    {
        bool proceed = connection_event_handler.before_sleep(); // Throws
        if (ARCHON_UNLIKELY(!proceed))
            return false; // Interrupt
    }

    // Ensure that all outgoing X11 requests are flushed before we start waiting
    XFlush(dpy);

    return true; // Wait for more events to occur
}


void ConnectionImpl::wait_for_events()
{
    pollfd fds[1] {};
    int nfds = 1;
    int timeout = -1; // No timeout
    fds[0].fd = ConnectionNumber(dpy);
    fds[0].events = POLLIN;
    int ret, err;
  again:
    ret = ::poll(fds, nfds, timeout);
    if (ARCHON_LIKELY(ret >= 0)) {
        ARCHON_ASSERT(ret == 1);
        return;
    }
    err = errno; // Eliminate any risk of clobbering
    if (ARCHON_LIKELY(err == EINTR))
        goto again;
    core::throw_system_error(err, "Failed to poll fire descriptor of X11 connection"); // Throws
}


auto ConnectionImpl::lookup_window_entry(::Window window_id) noexcept -> const WindowEntry*
{
    if (ARCHON_LIKELY(m_have_curr_window_id && window_id == m_curr_window_id))
        return m_curr_window_entry;

    const WindowEntry* entry = nullptr;
    auto i = m_window_entries.find(window_id);
    if (ARCHON_LIKELY(i != m_window_entries.end()))
        entry = &i->second;
    m_curr_window_id = window_id;
    m_curr_window_entry = entry;
    m_have_curr_window_id = true;
    return entry;
}



inline WindowImpl::WindowImpl(ConnectionImpl& conn_2) noexcept
    : conn(conn_2)
{
}


WindowImpl::~WindowImpl() noexcept
{
    if (ARCHON_LIKELY(m_have_window)) {
        if (m_is_registered)
            conn.unregister_window(win);
        XDestroyWindow(conn.dpy, win);
    }
}


void WindowImpl::create(const X11Screen& screen, display::Size size, display::WindowEventHandler& event_handler,
                        const Config& config)
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
    int depth = screen.depth;
    unsigned class_ = InputOutput;
    Visual* visual = screen.visual;
    unsigned long valuemask = CWEventMask | CWColormap;
    XSetWindowAttributes attributes = {};
    attributes.event_mask = (KeyPressMask | ExposureMask | StructureNotifyMask |
                             ButtonMotionMask | ButtonPressMask | ButtonReleaseMask);                            
    attributes.colormap = screen.colormap;
    win = XCreateWindow(conn.dpy, parent, x, y, width, height, border_width, depth, class_, visual,
                        valuemask, &attributes);
    m_have_window = true;

    conn.register_window(win, event_handler, config.cookie); // Throws
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
    ARCHON_STEADY_ASSERT(status != 0);                                              
    XSetWMName(conn.dpy, win, &name_3);                             
    XFree(name_3.value);
}


void WindowImpl::set_size(display::Size size)
{
    static_cast<void>(size);    
    throw std::runtime_error("*click*");     
}


void WindowImpl::set_fullscreen_mode(bool on)
{
    static_cast<void>(on);    
    throw std::runtime_error("*click*");     
}


void WindowImpl::fill(util::Color color)
{
/*
    GC gc = ensure_graphics_context(); // Throws
    int x = 0;
    int y = 0;
    unsigned int width, height;
    XFillRectangle(impl.dpy, win, gc, x, y, width, height);
*/
    static_cast<void>(color);    
    throw std::runtime_error("*click*");     
}


void WindowImpl::fill(util::Color color, const display::Box& area)
{
    static_cast<void>(color);    
    static_cast<void>(area);    
    throw std::runtime_error("*click*");     
}


auto WindowImpl::new_texture(display::Size size) -> std::unique_ptr<display::Texture>
{
    static_cast<void>(size);    
    throw std::runtime_error("*click*");     
}


void WindowImpl::put_texture(const display::Texture& tex, const display::Pos& pos)
{
    static_cast<void>(tex);    
    static_cast<void>(pos);    
    throw std::runtime_error("*click*");     
}


void WindowImpl::put_texture(const display::Texture& tex, const display::Box& source_area, const display::Pos& pos)
{
    static_cast<void>(tex);    
    static_cast<void>(source_area);    
    static_cast<void>(pos);    
    throw std::runtime_error("*click*");     
}


void WindowImpl::present()
{
    throw std::runtime_error("*click*");     
}


void WindowImpl::opengl_make_current()
{
    throw std::runtime_error("*click*");     
}


void WindowImpl::opengl_swap_buffers()
{
    throw std::runtime_error("*click*");     
}


void WindowImpl::set_property(Atom name, Atom value) noexcept
{
    XChangeProperty(conn.dpy, win, name, XA_ATOM, 32, PropModeReplace, reinterpret_cast<unsigned char*>(&value), 1);
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
