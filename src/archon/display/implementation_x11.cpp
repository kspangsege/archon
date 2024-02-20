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
#include <algorithm>
#include <memory>
#include <utility>
#include <string_view>
#include <vector>
#include <locale>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/index_range.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/string_buffer_contents.hpp>
#include <archon/core/deque.hpp>
#include <archon/core/flat_set.hpp>
#include <archon/core/flat_map.hpp>
#include <archon/core/literal_hash_map.hpp>
#include <archon/core/string.hpp>
#include <archon/core/format.hpp>
#include <archon/core/quote.hpp>
#include <archon/core/platform_support.hpp>
#include <archon/math/vector.hpp>
#include <archon/display/impl/config.h>
#include <archon/display/mouse_button.hpp>
#include <archon/display/screen.hpp>
#include <archon/display/noinst/timestamp_unwrapper.hpp>
#include <archon/display/noinst/edid.hpp>
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
#  if ARCHON_DISPLAY_HAVE_X11_XDBE
#    define HAVE_XDBE 1
#  else
#    define HAVE_XDBE 0
#  endif
#  if ARCHON_DISPLAY_HAVE_X11_XRANDR
#    define HAVE_XRANDR 1
#  else
#    define HAVE_XRANDR 0
#  endif
#  if ARCHON_DISPLAY_HAVE_X11_XRENDER
#    define HAVE_XRENDER 1
#  else
#    define HAVE_XRENDER 0
#  endif
#endif

#if HAVE_X11
#  include <poll.h>
#  include <X11/Xlib.h>
#  include <X11/Xatom.h>
#  include <X11/Xutil.h>
#  include <X11/keysym.h>
#  include <X11/XKBlib.h>
#  if HAVE_XDBE
#    include <X11/extensions/Xdbe.h>
#  endif
#  if HAVE_XRANDR
#    include <X11/extensions/Xrandr.h>
#  endif
#  if HAVE_XRENDER
#    include <X11/extensions/Xrender.h>
#  endif
#endif


// As of Jan 7, 2024, Release 7.7 is the latest release of X11. It was released on June 6, 2012.
//
//
// Relevant links:
//
// * X11 documentation overview: https://www.x.org/releases/X11R7.7/doc/
//
// * X11 protocol specification: https://www.x.org/releases/X11R7.7/doc/xproto/x11protocol.html
//
// * X11 API documentation: https://www.x.org/releases/X11R7.7/doc/libX11/libX11/libX11.html
//
// * Inter-Client Communication Conventions Manual: https://x.org/releases/X11R7.7/doc/xorg-docs/icccm/icccm.html
//
// * Extended Window Manager Hints: https://specifications.freedesktop.org/wm-spec/latest/
//
// * Xkb protocol specification: https://www.x.org/releases/X11R7.7/doc/kbproto/xkbproto.html
//
// * Xkb API documentation: https://www.x.org/releases/X11R7.7/doc/libX11/XKB/xkblib.html
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
namespace impl = display::impl;


namespace {


constexpr std::string_view g_implementation_ident = "x11";


#if HAVE_X11


// Compatible with XKeymapEvent::key_vector
class X11KeyCodeSet {
public:
    void assign(const char* bytes) noexcept
    {
        std::copy_n(bytes, 32, m_bytes);
    }

    bool contains(KeyCode keycode) const noexcept
    {
        ARCHON_ASSERT(core::int_greater_equal(keycode, 0) && core::int_less_equal(keycode, 255));
        int i = int(keycode);
        return ((byte(i) & bit(i)) != 0);
    }

    void add(KeyCode keycode) noexcept
    {
        ARCHON_ASSERT(core::int_greater_equal(keycode, 0) && core::int_less_equal(keycode, 255));
        int i = int(keycode);
        byte(i) |= bit(i);
    }

    void remove(KeyCode keycode) noexcept
    {
        ARCHON_ASSERT(core::int_greater_equal(keycode, 0) && core::int_less_equal(keycode, 255));
        int i = int(keycode);
        byte(i) &= ~bit(i);
    }

private:
    char m_bytes[32] = {};

    auto byte(int i) const noexcept -> const unsigned char&
    {
        return reinterpret_cast<const unsigned char*>(m_bytes)[i / 8];
    }

    auto byte(int i) noexcept -> unsigned char&
    {
        return reinterpret_cast<unsigned char*>(m_bytes)[i / 8];
    }

    static int bit(int i) noexcept
    {
        return 1 << (i % 8);
    }
};


#if HAVE_XRANDR

struct ProtoScreen {
    core::IndexRange output_name;
    display::Box bounds;
    std::optional<core::IndexRange> monitor_name;
    std::optional<display::Resolution> resolution;
    std::optional<double> refresh_rate;
};

#endif // HAVE_XRANDR


// One slot for each X11 screen
struct ScreenSlot {
    bool is_initialized = false;
    bool use_double_buffering = {};
    XVisualInfo visual_info = {};
    Window root = {};
    Colormap colormap = {};

#if HAVE_XRANDR
    std::vector<ProtoScreen> screens;
    core::Buffer<char> screens_string_buffer;
    std::size_t screens_string_buffer_used_size = 0;
#endif // HAVE_XRANDR
};


bool map_key(display::KeyCode, display::Key&) noexcept;
bool rev_map_key(display::Key, display::KeyCode&) noexcept;

bool try_map_mouse_button(unsigned x11_button, bool& is_scroll, display::MouseButton& button,
                          math::Vector2F& amount) noexcept;


class WindowImpl;


class ImplementationImpl
    : public display::Implementation {
public:
    ImplementationImpl(Slot&) noexcept;

    auto new_connection(const std::locale&, const display::Connection::Config&) const ->
        std::unique_ptr<display::Connection> override final;
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
    Atom atom_net_wm_state;
    Atom atom_net_wm_state_fullscreen;

#if HAVE_XRANDR
    Atom atom_edid;
#endif

    ConnectionImpl(const ImplementationImpl&, const std::locale&, const display::ConnectionConfigX11&) noexcept;
    ~ConnectionImpl() noexcept override;

    void open(const display::ConnectionConfigX11&);
    auto ensure_screen_slot(int screen) const -> ScreenSlot&;
    void register_window(::Window id, WindowImpl&);
    void unregister_window(::Window id) noexcept;

    bool try_map_key_to_key_code(display::Key, display::KeyCode&) const override final;
    bool try_map_key_code_to_key(display::KeyCode, display::Key&) const override final;
    bool try_get_key_name(display::KeyCode, std::string_view&) const override final;
    auto new_window(std::string_view, display::Size, display::WindowEventHandler&,
                    const display::Window::Config&) -> std::unique_ptr<display::Window> override final;
    auto new_window(int, std::string_view, display::Size, display::WindowEventHandler&,
                    const display::Window::Config&) -> std::unique_ptr<display::Window> override final;
    void process_events(display::ConnectionEventHandler*) override final;
    bool process_events(time_point_type, display::ConnectionEventHandler*) override final;
    int get_num_displays() const override final;
    int get_default_display() const override final;
    bool try_get_display_conf(int, core::Buffer<display::Screen>&, core::Buffer<char>&,
                              std::size_t&, bool&) const override final;
    auto get_implementation() const noexcept -> const display::Implementation& override final;

private:
    const std::optional<int> m_depth_override;
    const std::optional<VisualID> m_visual_override;
    const bool m_disable_double_buffering;

    // X protocol extension availability
    bool m_have_xdbe    = false; // X Double Buffer Extension
    bool m_have_xrandr  = false; // X Resize, Rotate and Reflect Extension
    bool m_have_xrender = false; // X Rendering Extension

    bool m_detectable_autorepeat_enabled = false;
    bool m_expect_keymap_notify = false;
    bool m_have_curr_window = false;

    int m_xrandr_event_base = 0;

    mutable std::unique_ptr<ScreenSlot[]> m_screen_slots;
    mutable core::FlatMap<::Window, int> m_x11_screens_by_root;

#if HAVE_XRANDR
    mutable std::optional<impl::EdidParser> m_edid_parser;
#endif

    X11KeyCodeSet m_pressed_keys;

    core::FlatMap<::Window, WindowImpl&> m_windows;

    // Track pointer grabs so that "mouse over" and "mouse out" events can be ignored when
    // they occur during a grab.
    //
    // If the pointer leaves the window during a pointer grab and the grab ends outside the
    // window, there is a question of whether the "mouse out" event should occur when the
    // pointer leaves the window or when the grab ends. SDL (Simple DirectMedia Layer) opts
    // to let the "mouse out" event occur when the grab ends, and, unfortunately, there is
    // no way to emulate the other behavior when using SDL.
    //
    // X11, on the other hand, generates a "mouse out" event in both cases, that is when the
    // pointer leaves the window and when the grab ends. With this, we can emulate the SDL
    // behavior using X11 by ignoring all "mouse over" and "mouse out" event while a grab is
    // in progress.
    //
    // In the interest of alignment across display implementations and with the SDL-based
    // implementation in particular (`implementation_sdl.cpp`), the required behavior of
    // display implementations is to generate the "mouse out" event when the grab ends. See
    // also display::EventHandler::on_mouseover().
    //
    core::FlatSet<::Window> m_pointer_grab_buttons;
    ::Window m_pointer_grab_window_id = {};

    // A queue of windows with pending expose events (push to back and pop from
    // front). Windows occur at most once in this queue.
    //
    // INVARIANT: A window is in `m_exposed_windows` if and only if it is in `m_windows` and
    // has `has_pending_expose_event` set to `true`.
    core::Deque<::Window> m_exposed_windows;

    // X11 timestamps are 32-bit unsigned integers and `Time` refers to the unsigned integer
    // type that X11 uses to store these timestamps.
    using timestamp_unwrapper_type = impl::TimestampUnwrapper<Time, 32>;
    timestamp_unwrapper_type m_timestamp_unwrapper;

    // If `m_have_curr_window` is true, then `m_curr_window` specifies the window identified
    // by `m_curr_window_id`. If `m_have_curr_window` is false, `m_curr_window_id` and
    // `m_curr_window` have no meaning.
    //
    // If `m_have_curr_window` is true, but `m_curr_window` is null, it means that the X
    // client has no knowledge of a window with the ID specified by `m_curr_window_id`. This
    // state is entered if the window specified by `m_curr_window_id` is unregistered
    // (unregister_window()). The state is updated whenever a new window is registered
    // (register_window()). This takes care of the case where a new window reuses the ID
    // specified by `m_curr_window_id`.
    //
    ::Window m_curr_window_id = {};
    WindowImpl* m_curr_window = nullptr;

    int m_num_events = 0;

    auto intern_string(const char*) noexcept -> Atom;
    bool lookup_visual_info(int screen, int depth, VisualID visual_id, XVisualInfo&) const noexcept;
    auto do_new_window(int screen, std::string_view title, display::Size size, display::WindowEventHandler&,
                       const display::Window::Config&) -> std::unique_ptr<display::Window>;
    bool do_process_events(const time_point_type* deadline, display::ConnectionEventHandler*);
    bool lookup_window(::Window window_id, WindowImpl*& window) noexcept;
    void track_pointer_grabs(::Window window_id, unsigned button, bool is_press);
    bool is_pointer_grabbed() const noexcept;

#if HAVE_XRANDR
    bool update_display_info(ScreenSlot& slot) const;
    bool try_update_display_info(ScreenSlot& slot, bool& changed) const;
    auto ensure_edid_parser() const -> const impl::EdidParser&;
#endif
};


class WindowImpl
    : public display::Window {
public:
    ConnectionImpl& conn;
    const ScreenSlot& screen_slot;
    display::WindowEventHandler& event_handler;
    const int cookie;

    ::Window win = {};
    GC gc = {};

    bool has_pending_expose_event = false;

    WindowImpl(ConnectionImpl&, const ScreenSlot&, display::WindowEventHandler&, int cookie) noexcept;
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
#if HAVE_XDBE
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

    if (ARCHON_UNLIKELY(config.synchronous_mode))
        XSynchronize(dpy, True);

    atom_wm_protocols            = intern_string("WM_PROTOCOLS");
    atom_wm_delete_window        = intern_string("WM_DELETE_WINDOW");
    atom_net_wm_state            = intern_string("_NET_WM_STATE");
    atom_net_wm_state_fullscreen = intern_string("_NET_WM_STATE_FULLSCREEN");
#if HAVE_XRANDR
    atom_edid = intern_string(RR_PROPERTY_RANDR_EDID);
#endif

#if HAVE_XDBE
    {
        int major = 0;
        int minor = 0;
        if (ARCHON_LIKELY(XdbeQueryExtension(dpy, &major, &minor))) {
            if (ARCHON_LIKELY(major >= 1))
                m_have_xdbe = true;
        }
    }
#endif // HAVE_XDBE

    {
        int have_xkb = false;
        int opcode = 0;     // Unused
        int event_base = 0; // Unused
        int error_base = 0; // Unused
        int major = 0;
        int minor = 0;
        if (ARCHON_LIKELY(XkbQueryExtension(dpy, &opcode, &event_base, &error_base, &major, &minor))) {
            if (ARCHON_LIKELY(major >= 1))
                have_xkb = true;
        }
        if (ARCHON_UNLIKELY(!have_xkb))
            throw std::runtime_error("X Keyboard Extension is required but not available");
    }

    if (!config.disable_detectable_autorepeat) {
        Bool detectable = True;
        Bool supported = {};
        XkbSetDetectableAutoRepeat(dpy, detectable, &supported);
        if (ARCHON_LIKELY(supported))
            m_detectable_autorepeat_enabled = true;
    }

#if HAVE_XRANDR
    {
        int error_base = 0; // Unused
        if (ARCHON_LIKELY(XRRQueryExtension(dpy, &m_xrandr_event_base, &error_base))) {
            int major = 0;
            int minor = 0;
            Status status = XRRQueryVersion(dpy, &major, &minor);
            if (ARCHON_UNLIKELY(status == 0))
                throw std::runtime_error("XRRQueryVersion() failed");
            if (ARCHON_LIKELY(major > 1 || (major == 1 && minor >= 5)))
                m_have_xrandr = true;
        }
    }
#endif // HAVE_XRANDR

#if HAVE_XRENDER
    {
        int event_base = 0; // Unused
        int error_base = 0; // Unused
        if (ARCHON_LIKELY(XRenderQueryExtension(dpy, &event_base, &error_base))) {
            int major = 0;
            int minor = 0;
            Status status = XRenderQueryVersion(dpy, &major, &minor);
            if (ARCHON_UNLIKELY(status == 0))
                throw std::runtime_error("XRenderQueryVersion() failed");
            if (ARCHON_LIKELY(major > 0 || (major == 0 && minor >= 7)))
                m_have_xrender = true;
        }
    }
#endif // HAVE_XRENDER
}


auto ConnectionImpl::ensure_screen_slot(int screen) const -> ScreenSlot&
{
    if (ARCHON_UNLIKELY(!m_screen_slots))
        m_screen_slots = std::make_unique<ScreenSlot[]>(std::size_t(ScreenCount(dpy))); // Throws
    ARCHON_ASSERT(screen >= 0 && screen <= ScreenCount(dpy));
    ScreenSlot& slot = m_screen_slots[screen];
    if (ARCHON_UNLIKELY(!slot.is_initialized)) {
        ::Window root = RootWindow(dpy, screen);
        slot.root = root;
        m_x11_screens_by_root[root] = screen; // Throws

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
        slot.visual_info = visual_info;

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
        slot.colormap = colormap;

        bool use_double_buffering = false;
#if HAVE_XDBE
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
#endif // HAVE_XDBE
        slot.use_double_buffering = use_double_buffering;

#if HAVE_XRANDR
        if (ARCHON_LIKELY(m_have_xrandr)) {
            int mask = RROutputChangeNotifyMask | RRCrtcChangeNotifyMask;
            XRRSelectInput(dpy, root, mask);
            update_display_info(slot); // Throws
        }
#endif // HAVE_XRANDR

        slot.is_initialized = true;
        colormap = None;
    }
    return slot;
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

    if (ARCHON_UNLIKELY(m_pointer_grab_window_id == id))
        m_pointer_grab_buttons.clear();

    auto i = std::find(m_exposed_windows.begin(), m_exposed_windows.end(), id);
    if (i != m_exposed_windows.end())
        m_exposed_windows.erase(i);

    if (ARCHON_LIKELY(m_have_curr_window && id == m_curr_window_id))
        m_curr_window = nullptr;
}


bool ConnectionImpl::try_map_key_to_key_code(display::Key key, display::KeyCode& key_code) const
{
    return ::rev_map_key(key, key_code);
}


bool ConnectionImpl::try_map_key_code_to_key(display::KeyCode key_code, display::Key& key) const
{
    return ::map_key(key_code, key);
}


bool ConnectionImpl::try_get_key_name(display::KeyCode key_code, std::string_view& name) const
{
    // XKeysymToString() returns a string consisting entirely of characters from the X
    // Portable Character Set. Since all locales, that are compatible with Xlib, agree on
    // the encoding of characters in this character set, and since we assume that the
    // selected locale is compatible with Xlib, we can assume that the returned string is
    // valid in the selected locale.

    auto keysym = KeySym(key_code.code);
    const char* c_str = XKeysymToString(keysym);
    if (ARCHON_LIKELY(c_str)) {
        name = std::string_view(c_str);
        return true;
    }
    return false;
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


bool ConnectionImpl::try_get_display_conf(int display, core::Buffer<display::Screen>& screens,
                                          core::Buffer<char>& strings, std::size_t& num_screens, bool& reliable) const
{
    if (ARCHON_UNLIKELY(display < 0 || display >= int(ScreenCount(dpy))))
        throw std::invalid_argument("Bad display index");

#if HAVE_XRANDR
    const ScreenSlot& slot = ensure_screen_slot(display); // Throws
    std::size_t n = slot.screens.size();
    screens.reserve(n); // Throws
    const char* strings_base = slot.screens_string_buffer.data();
    strings.assign({ strings_base, slot.screens_string_buffer_used_size }); // Throws
    const char* strings_base_2 = strings.data();
    for (std::size_t i = 0; i < n; ++i) {
        const ProtoScreen& proto = slot.screens[i];
        std::optional<std::string_view> monitor_name;
        if (proto.monitor_name.has_value())
            monitor_name = proto.monitor_name.value().resolve_string(strings_base_2); // Throws
        screens[i] = {
            proto.output_name.resolve_string(strings_base_2), // Throws
            proto.bounds,
            monitor_name,
            proto.resolution,
            proto.refresh_rate,
        };
    }
    num_screens = n;
    reliable = true;
    return true;
#else // !HAVE_XRANDR
    static_cast<void>(screens);
    static_cast<void>(strings);
    static_cast<void>(num_screens);
    static_cast<void>(reliable);
    return false;
#endif // !HAVE_XRANDR
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
    const ScreenSlot& screen_slot = ensure_screen_slot(screen); // Throws
    auto win = std::make_unique<WindowImpl>(*this, screen_slot, event_handler, config.cookie); // Throws
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
    //   there are no events that are already queued (must be called after XFlush()).
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

    XEvent ev = {};
    WindowImpl* window = {};
    timestamp_unwrapper_type::Session unwrap_session(m_timestamp_unwrapper);
    bool expect_keymap_notify;

  process_1:
    if (ARCHON_LIKELY(m_num_events > 0))
        goto process_2;
    goto post;

  process_2:
    ARCHON_ASSERT(m_num_events > 0);
    XNextEvent(dpy, &ev);
    --m_num_events;
    expect_keymap_notify = m_expect_keymap_notify;
    m_expect_keymap_notify = false;
    ARCHON_ASSERT(!expect_keymap_notify || ev.type == KeymapNotify);
    switch (ev.type) {
        case MotionNotify:
            if (ARCHON_LIKELY(lookup_window(ev.xmotion.window, window))) {
                display::MouseEvent event;
                event.cookie = window->cookie;
                event.timestamp = unwrap_session.unwrap_next_timestamp(ev.xmotion.time); // Throws
                event.pos = { ev.xmotion.x, ev.xmotion.y };
                bool proceed = window->event_handler.on_mousemove(event); // Throws
                if (ARCHON_LIKELY(proceed))
                    break;
                return false; // Interrupt
            }
            break;

        case ConfigureNotify:
            if (ARCHON_LIKELY(lookup_window(ev.xconfigure.window, window))) {
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
                bool proceed;
                if (ev.xconfigure.send_event) {
                    display::WindowPosEvent event;
                    event.cookie = window->cookie;
                    event.pos = { ev.xconfigure.x, ev.xconfigure.y };
                    proceed = window->event_handler.on_reposition(event); // Throws
                }
                else {
                    display::WindowSizeEvent event;
                    event.cookie = window->cookie;
                    event.size = { ev.xconfigure.width, ev.xconfigure.height };
                    proceed = window->event_handler.on_resize(event); // Throws
                }
                if (ARCHON_LIKELY(proceed))
                    break;
                return false; // Interrupt
            }
            break;

        case Expose:
            if (ARCHON_LIKELY(!lookup_window(ev.xexpose.window, window) || window->has_pending_expose_event))
                break;
            m_exposed_windows.push_back(ev.xexpose.window); // Throws
            window->has_pending_expose_event = true;
            break;

        case ButtonPress:
        case ButtonRelease:
            if (ARCHON_LIKELY(lookup_window(ev.xbutton.window, window))) {
                track_pointer_grabs(ev.xbutton.window, ev.xbutton.button, ev.type == ButtonPress); // Throws
                bool is_scroll = {};
                display::MouseButton button = {};
                math::Vector2F amount;
                if (ARCHON_LIKELY(try_map_mouse_button(ev.xbutton.button, is_scroll, button, amount))) {
                    if (ARCHON_LIKELY(is_scroll)) {
                        display::ScrollEvent event;
                        event.cookie = window->cookie;
                        event.timestamp = unwrap_session.unwrap_next_timestamp(ev.xbutton.time); // Throws
                        event.amount = amount;
                        bool proceed = window->event_handler.on_scroll(event); // Throws
                        if (ARCHON_LIKELY(proceed))
                            break;
                        return false; // Interrupt
                    }
                    else {
                        display::MouseButtonEvent event;
                        event.cookie = window->cookie;
                        event.timestamp = unwrap_session.unwrap_next_timestamp(ev.xbutton.time); // Throws
                        event.pos = { ev.xbutton.x, ev.xbutton.y };
                        event.button = button;
                        bool proceed;
                        if (ev.type == ButtonPress) {
                            proceed = window->event_handler.on_mousedown(event); // Throws
                        }
                        else {
                            proceed = window->event_handler.on_mouseup(event); // Throws
                        }
                        if (ARCHON_LIKELY(proceed))
                            break;
                        return false; // Interrupt
                    }
                }
            }
            break;

        case KeyPress:
        case KeyRelease:
            if (ARCHON_LIKELY(lookup_window(ev.xkey.window, window))) {
                using timestamp_type = display::TimedWindowEvent::Timestamp;
                timestamp_type timestamp = unwrap_session.unwrap_next_timestamp(ev.xkey.time); // Throws
                bool is_repetition = false;
                if (ARCHON_LIKELY(m_detectable_autorepeat_enabled)) {
                    if (ev.type == KeyPress) {
                        if (!m_pressed_keys.contains(ev.xkey.keycode)) {
                            m_pressed_keys.add(ev.xkey.keycode);
                        }
                        else {
                            is_repetition = true;
                        }
                    }
                    else {
                        ARCHON_ASSERT(m_pressed_keys.contains(ev.xkey.keycode));
                        m_pressed_keys.remove(ev.xkey.keycode);
                    }
                }
                else {
                    // When "detectable auto-repeat" mode was not enabled, we need to use a
                    // fall-back detection mechanism, which works as follows: On "key up",
                    // if the next event is "key down" for the same key and at almost the
                    // same time, consider the pair to be caused by key repetition. This
                    // scheme assumes that the second "key down" event is immediately
                    // available, i.e., without having to block. This assumption appears to
                    // hold in practice, but it could conceivably fail, in which case the
                    // pair will be treated as genuine "key up" and "key down" events.
                    if (ev.type == KeyPress) {
                        ARCHON_ASSERT(!m_pressed_keys.contains(ev.xkey.keycode));
                        m_pressed_keys.add(ev.xkey.keycode);
                    }
                    else {
                        ARCHON_ASSERT(m_pressed_keys.contains(ev.xkey.keycode));
                        if (m_num_events == 0) {
                            int n = XEventsQueued(dpy, QueuedAfterReading);  // Non-blocking
                            if (n > 0)
                                m_num_events = 1;
                        }
                        if (m_num_events > 0) {
                            XEvent ev_2 = {};
                            XPeekEvent(dpy, &ev_2);
                            if (ev_2.type == KeyPress && ev_2.xkey.keycode == ev.xkey.keycode) {
                                ARCHON_ASSERT(ev_2.xkey.window == ev.xkey.window);
                                timestamp_type timestamp_2 =
                                    unwrap_session.unwrap_next_timestamp(ev_2.xkey.time); // Throws
                                ARCHON_ASSERT(timestamp_2 >= timestamp);
                                if ((timestamp_2 - timestamp).count() <= 1) {
                                    XNextEvent(dpy, &ev);
                                    timestamp = timestamp_2;
                                    --m_num_events;
                                    is_repetition = true;
                                }
                            }
                        }
                        if (!is_repetition)
                            m_pressed_keys.remove(ev.xkey.keycode);
                    }
                }
                // Map key code to a keyboard independent symbol identifier (in general the
                // symbol in the upper left corner on the corresponding key). See also
                // https://tronche.com/gui/x/xlib/input/keyboard-encoding.html.
                unsigned group = XkbGroup1Index;
                unsigned level = 0;
                KeySym keysym = XkbKeycodeToKeysym(dpy, ev.xkey.keycode, group, level);
                ARCHON_ASSERT(keysym != NoSymbol);
                display::KeyEvent event;
                event.cookie = window->cookie;
                event.timestamp = timestamp;
                event.key_code = { display::KeyCode::code_type(keysym) };
                bool proceed;
                if (ev.type == KeyPress) {
                    if (ARCHON_LIKELY(!is_repetition)) {
                        proceed = window->event_handler.on_keydown(event); // Throws
                    }
                    else {
                        proceed = window->event_handler.on_keyrepeat(event); // Throws
                    }
                }
                else {
                    proceed = window->event_handler.on_keyup(event); // Throws
                }
                if (ARCHON_LIKELY(proceed))
                    break;
                return false; // Interrupt
            }
            break;

        case KeymapNotify:
            // Note: For some unclear reason, `ev.xkeymap.window` does not specify the
            // target window like it does for other types of events. Instead, one can rely
            // on `KeymapNotify` to be generated immediately after every `FocusIn` event, so
            // this provides an implicit target window.
            if (expect_keymap_notify)
                m_pressed_keys.assign(ev.xkeymap.key_vector);
            break;

        case EnterNotify:
        case LeaveNotify:
            if (ARCHON_LIKELY(lookup_window(ev.xcrossing.window, window) && !is_pointer_grabbed())) {
                display::TimedWindowEvent event;
                event.cookie = window->cookie;
                event.timestamp = unwrap_session.unwrap_next_timestamp(ev.xcrossing.time); // Throws
                bool proceed;
                if (ev.type == EnterNotify) {
                    proceed = window->event_handler.on_mouseover(event); // Throws
                }
                else {
                    proceed = window->event_handler.on_mouseout(event); // Throws
                }
                if (ARCHON_LIKELY(proceed))
                    break;
                return false; // Interrupt
            }
            break;

        case FocusIn:
        case FocusOut:
            if (ev.type == FocusIn)
                m_expect_keymap_notify = true;
            if (ARCHON_LIKELY(lookup_window(ev.xfocus.window, window))) {
                display::WindowEvent event;
                event.cookie = window->cookie;
                bool proceed;
                if (ev.type == FocusIn) {
                    proceed = window->event_handler.on_focus(event); // Throws
                }
                else {
                    proceed = window->event_handler.on_blur(event); // Throws
                }
                if (ARCHON_LIKELY(proceed))
                    break;
                return false; // Interrupt
            }
            break;

        case ClientMessage:
            bool is_close = (ev.xclient.format == 32 && Atom(ev.xclient.data.l[0]) == atom_wm_delete_window);
            if (ARCHON_LIKELY(is_close && lookup_window(ev.xclient.window, window))) {
                display::WindowEvent event;
                event.cookie = window->cookie;
                bool proceed = window->event_handler.on_close(event); // Throws
                if (ARCHON_LIKELY(!proceed))
                    return false; // Interrupt
            }
            break;
    }

#if HAVE_XRANDR
    if (m_have_xrandr && ev.type == m_xrandr_event_base + RRNotify) {
        const auto& ev_2 = reinterpret_cast<const XRRNotifyEvent&>(ev);
        switch (ev_2.subtype) {
            case RRNotify_CrtcChange:
            case RRNotify_OutputChange:
                ::Window root = ev_2.window;
                auto i = m_x11_screens_by_root.find(root);
                ARCHON_ASSERT(i != m_x11_screens_by_root.end());
                int screen = i->second;
                ARCHON_ASSERT(screen >= 0 && screen < ScreenCount(dpy));
                ScreenSlot& slot = m_screen_slots[screen];
                if (update_display_info(slot)) // Throws
                    connection_event_handler_2.on_display_change(screen); // Throws
        }
    }
#endif // HAVE_XRANDR
    goto process_1;

  post:
    for (;;) {
        if (ARCHON_LIKELY(m_exposed_windows.empty()))
            break;
        ::Window window_id = m_exposed_windows.front();
        m_exposed_windows.pop_front();
        WindowImpl* window = {};
        if (ARCHON_LIKELY(lookup_window(window_id, window))) {
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

        if (ARCHON_LIKELY(m_num_events > 0)) {
            unwrap_session.reset_now();
            goto process_2;
        }

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


bool ConnectionImpl::lookup_window(::Window window_id, WindowImpl*& window) noexcept
{
    WindowImpl* window_2 = nullptr;
    if (ARCHON_LIKELY(m_have_curr_window && window_id == m_curr_window_id)) {
        window_2 = m_curr_window;
    }
    else {
        auto i = m_windows.find(window_id);
        if (ARCHON_LIKELY(i != m_windows.end()))
            window_2 = &i->second;
        m_curr_window_id = window_id;
        m_curr_window = window_2;
        m_have_curr_window = true;
    }
    if (ARCHON_LIKELY(window_2)) {
        window = window_2;
        return true;
    }
    return false;
}


void ConnectionImpl::track_pointer_grabs(::Window window_id, unsigned button, bool is_press)
{
    ARCHON_ASSERT(!is_pointer_grabbed() || window_id == m_pointer_grab_window_id);
    if (is_press) {
        bool grab_in_progress = is_pointer_grabbed();
        auto p = m_pointer_grab_buttons.insert(button); // Throws
        bool was_inserted = p.second;
        ARCHON_ASSERT(was_inserted);
        if (ARCHON_LIKELY(!grab_in_progress))
            m_pointer_grab_window_id = window_id;
    }
    else {
        auto n = m_pointer_grab_buttons.erase(button);
        ARCHON_ASSERT(n == 1);
    }
}


inline bool ConnectionImpl::is_pointer_grabbed() const noexcept
{
    return !m_pointer_grab_buttons.empty();
}


#if HAVE_XRANDR


bool ConnectionImpl::update_display_info(ScreenSlot& slot) const
{
    int max_attempts = 16;
    for (int i = 0; i < max_attempts; ++i) {
        bool changed = false;
        if (ARCHON_LIKELY(try_update_display_info(slot, changed)))
            return changed;
    }
    throw std::runtime_error("Failed to fetch screen configuration using XRandR within the allotted number of "
                             "attempts");
}


bool ConnectionImpl::try_update_display_info(ScreenSlot& slot, bool& changed) const
{
    XRRScreenResources* resources = XRRGetScreenResourcesCurrent(dpy, slot.root);
    if (ARCHON_UNLIKELY(!resources))
        throw std::runtime_error("XRRGetScreenResourcesCurrent() failed");
    ARCHON_SCOPE_EXIT {
        XRRFreeScreenResources(resources);
    };
    struct Crtc {
        bool enabled;
        display::Box bounds;
        std::optional<double> refresh_rate;
    };
    core::FlatMap<RRCrtc, Crtc, 16> crtcs;
    crtcs.reserve(std::size_t(resources->ncrtc)); // Throws
    auto ensure_crtc = [&](RRCrtc id) -> const Crtc* {
        auto i = crtcs.find(id);
        if (i != crtcs.end())
            return &i->second;
        XRRCrtcInfo* info = XRRGetCrtcInfo(dpy, resources, id);
        if (ARCHON_UNLIKELY(!info))
            return nullptr;
        ARCHON_SCOPE_EXIT {
            XRRFreeCrtcInfo(info);
        };
        bool enabled = (info->mode != None);
        display::Size size = {};
        core::int_cast(info->width, size.width); // Throws
        core::int_cast(info->height, size.height); // Throws
        display::Box bounds = { display::Pos(info->x, info->y), size };
        std::optional<double> refresh_rate;
        if (enabled) {
            bool found = false;
            for (int j = 0; j < resources->nmode; ++j) {
                const XRRModeInfo& mode = resources->modes[j];
                if (ARCHON_LIKELY(mode.id != info->mode))
                    continue;
                found = true;
                if (mode.dotClock != 0)
                    refresh_rate = mode.dotClock / (mode.hTotal * double(mode.vTotal));
                break;
            }
            ARCHON_ASSERT(found);
        }
        ARCHON_STEADY_ASSERT(crtcs.size() < crtcs.capacity());
        Crtc& crtc =  crtcs[id];
        crtc = { enabled, bounds, refresh_rate };
        return &crtc;
    };
    core::Vector<ProtoScreen, 16> new_screens;
    std::array<char, 16 * 24> strings_seed_memory = {};
    core::Buffer strings_buffer(strings_seed_memory);
    core::StringBufferContents strings(strings_buffer);
    const impl::EdidParser& edid_parser = ensure_edid_parser(); // Throws
    for (int i = 0; i < resources->noutput; ++i) {
        RROutput id = resources->outputs[i];
        XRROutputInfo* info = XRRGetOutputInfo(dpy, resources, id);
        if (ARCHON_UNLIKELY(!info))
            return false;
        ARCHON_SCOPE_EXIT {
            XRRFreeOutputInfo(info);
        };
        // Note: Treating RR_UnknownConnection same as RR_Connected
        bool connected = (info->connection != RR_Disconnected);
        if (!connected || info->crtc == None)
            continue;
        const Crtc* crtc = ensure_crtc(info->crtc); // Throws
        if (ARCHON_UNLIKELY(!crtc))
            return false;
        if (!crtc->enabled)
            continue;
        // FIXME: Consider character encoding in output name                
        std::size_t offset = strings.size();
        std::size_t size = std::size_t(info->nameLen);
        strings.append({ info->name, size }); // Throws
        // The base address is not necessarily correct anymore, but this Will be fixed up later
        core::IndexRange output_name = { offset, size }; // Throws
        std::optional<display::Resolution> resolution;
        if (info->mm_width != 0 && info->mm_height != 0) {
            double horz_ppcm = crtc->bounds.size.width  / double(info->mm_width)  * 10;
            double vert_ppcm = crtc->bounds.size.height / double(info->mm_height) * 10;
            resolution = display::Resolution { horz_ppcm, vert_ppcm };
        }
        // Extract monitor name from EDID data when available
        std::optional<core::IndexRange> monitor_name;
        int nprop = {};
        Atom* props = XRRListOutputProperties(dpy, id, &nprop);
        if (ARCHON_LIKELY(props))  {
            ARCHON_SCOPE_EXIT {
                XFree(props);
            };
            for (int j = 0; j < nprop; ++j) {
                if (props[j] == atom_edid) {
                    long offset = 0;
                    long length = 128 / 4; // 128 bytes (32 longs) in basic EDID block
                    Bool _delete = False;
                    Bool pending = False;
                    Atom req_type = AnyPropertyType;
                    Atom actual_type = {};
                    int actual_format = {};
                    unsigned long nitems = {};
                    unsigned long bytes_after = {};
                    unsigned char* prop = {};
                    int ret = XRRGetOutputProperty(dpy, id, props[j], offset, length, _delete, pending, req_type,
                                                   &actual_type, &actual_format, &nitems, &bytes_after, &prop);
                    if (ARCHON_LIKELY(ret == Success)) {
                        ARCHON_SCOPE_EXIT {
                            XFree(prop);
                        };
                        if (ARCHON_LIKELY(actual_type == XA_INTEGER && actual_format == 8)) {
                            std::size_t size = {};
                            if (ARCHON_LIKELY(core::try_int_cast(nitems, size))) {
                                std::string_view str = { reinterpret_cast<char*>(prop), size };
                                impl::EdidInfo info = {};
                                if (ARCHON_LIKELY(edid_parser.parse(str, info, strings))) // Throws
                                    monitor_name = info.monitor_name;
                            }
                        }
                    }
                }
            }
        }
        ProtoScreen screen = { output_name, crtc->bounds, monitor_name, resolution, crtc->refresh_rate };
        new_screens.push_back(screen); // Throws
    }
    {
        const char* base_1 = strings.data();
        const char* base_2 = slot.screens_string_buffer.data();
        auto cmp_opt_str = [&](const std::optional<core::IndexRange>& a, const std::optional<core::IndexRange>& b) {
            return (a.has_value() ?
                    b.has_value() && a.value().resolve_string(base_1) == b.value().resolve_string(base_2) :
                    !b.has_value());
        };
        auto cmp = [&](const ProtoScreen& a, const ProtoScreen& b) {
            return (a.bounds == b.bounds &&
                    a.resolution == b.resolution &&
                    a.refresh_rate == b.refresh_rate &&
                    a.output_name.resolve_string(base_1) == b.output_name.resolve_string(base_2) &&
                    cmp_opt_str(a.monitor_name, b.monitor_name));
        };
        bool equal = std::equal(new_screens.begin(), new_screens.end(), slot.screens.begin(), slot.screens.end(),
                                std::move(cmp));
        if (equal) {
            changed = false;
            return true;
        }
    }
    slot.screens.reserve(new_screens.size()); // Throws
    slot.screens_string_buffer.reserve(strings.size(), slot.screens_string_buffer_used_size); // Throws
    // Non-throwing from here
    slot.screens.clear();
    slot.screens.insert(slot.screens.begin(), new_screens.begin(), new_screens.end());
    slot.screens_string_buffer.assign(strings);
    slot.screens_string_buffer_used_size = strings.size();
    changed = true;
    return true;
}


auto ConnectionImpl::ensure_edid_parser() const -> const impl::EdidParser&
{
    if (ARCHON_LIKELY(m_edid_parser.has_value()))
        return m_edid_parser.value();
    m_edid_parser.emplace(locale); // Throws
    return m_edid_parser.value();
}


#endif // HAVE_XRANDR



inline WindowImpl::WindowImpl(ConnectionImpl& conn_2, const ScreenSlot& screen_slot_2,
                              display::WindowEventHandler& event_handler_2, int cookie_2) noexcept
    : conn(conn_2)
    , screen_slot(screen_slot_2)
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

    ::Window parent = screen_slot.root;
    int x = 0, y = 0;
    unsigned width  = unsigned(adjusted_size.width);
    unsigned height = unsigned(adjusted_size.height);
    unsigned border_width = 0;
    int depth = screen_slot.visual_info.depth;
    unsigned class_ = InputOutput;
    Visual* visual = screen_slot.visual_info.visual;
    unsigned long valuemask = CWEventMask | CWColormap;
    XSetWindowAttributes attributes = {};
    attributes.event_mask = (KeyPressMask | KeyReleaseMask |
                             ButtonPressMask | ButtonReleaseMask |
                             ButtonMotionMask |
                             EnterWindowMask | LeaveWindowMask |
                             FocusChangeMask |
                             ExposureMask |
                             StructureNotifyMask |
                             KeymapStateMask);
    attributes.colormap = screen_slot.colormap;
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
#if HAVE_XDBE
    if (ARCHON_LIKELY(screen_slot.use_double_buffering)) {
        m_swap_action = XdbeUndefined; // Contents of swapped-out buffer becomes undefined
        XdbeBackBuffer back_buffer = XdbeAllocateBackBufferName(conn.dpy, win, m_swap_action);
        m_drawable = back_buffer;
        m_is_double_buffered = true;
    }
#endif // HAVE_XDBE

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
    if (ARCHON_UNLIKELY(size.width < 0 || size.height < 0))
        throw std::invalid_argument("Bad window size");
    unsigned w = unsigned(size.width);
    unsigned h = unsigned(size.height);
    XResizeWindow(conn.dpy, win, w, h);
}


void WindowImpl::set_fullscreen_mode(bool on)
{
    XClientMessageEvent event = {};
    event.type = ClientMessage;
    event.window = win;
    event.message_type = conn.atom_net_wm_state;
    event.format = 32;
    event.data.l[0] = (on ? 1 : 0); // Add / remove property
    event.data.l[1] = conn.atom_net_wm_state_fullscreen;
    event.data.l[2] = 0; // No second property to alter
    event.data.l[3] = 1; // Request is from normal application
    Bool propagate = False;
    long event_mask = SubstructureRedirectMask | SubstructureNotifyMask;
    Status status = XSendEvent(conn.dpy, screen_slot.root, propagate, event_mask, reinterpret_cast<XEvent*>(&event));
    if (ARCHON_UNLIKELY(status == 0))
        throw std::runtime_error("XSendEvent() failed");
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
#if HAVE_XDBE
    if (m_is_double_buffered) {
        XdbeSwapInfo info;
        info.swap_window = win;
        info.swap_action = m_swap_action;
        Status status = XdbeSwapBuffers(conn.dpy, &info, 1);
        if (ARCHON_UNLIKELY(status == 0))
            throw std::runtime_error("XdbeSwapBuffers() failed");
    }
#endif // HAVE_XDBE
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

    

    return WhitePixel(conn.dpy, screen_slot.visual_info.screen);                                                                                                                                                                        
}


// FIXME: Add more keys                                                                                                                                                 
constexpr std::pair<KeySym, display::Key> key_assocs[] {
    // TTY functions
    { XK_BackSpace,    display::Key::backspace            },
    { XK_Tab,          display::Key::tab                  },
    { XK_Linefeed,     display::Key::line_feed            },
    { XK_Clear,        display::Key::clear                },
    { XK_Return,       display::Key::return_              },
    { XK_Pause,        display::Key::pause                },
    { XK_Scroll_Lock,  display::Key::scroll_lock          },
    { XK_Sys_Req,      display::Key::sys_req              },
    { XK_Escape,       display::Key::escape               },
    { XK_Delete,       display::Key::delete_              },

    // Cursor control & motion

    // Misc functions
    { XK_Menu,         display::Key::menu                 },

    // Keypad
    { XK_KP_Add,       display::Key::keypad_plus_sign     },
    { XK_KP_Subtract,  display::Key::keypad_minus_sign    },

    // Function keys

    // Modifier keys
    { XK_Shift_L,      display::Key::shift_left           },
    { XK_Shift_R,      display::Key::shift_right          },
    { XK_Control_L,    display::Key::ctrl_left            },
    { XK_Control_R,    display::Key::ctrl_right           },
    { XK_Alt_L,        display::Key::alt_left             },
    { XK_Alt_R,        display::Key::alt_right            },
    { XK_Meta_L,       display::Key::meta_left            },
    { XK_Meta_R,       display::Key::meta_right           },

    // Basic Latin
    { XK_space,        display::Key::space                },
    { XK_exclam,       display::Key::exclamation_mark     },
    { XK_quotedbl,     display::Key::quotation_mark       },
    { XK_numbersign,   display::Key::hash_mark            },
    { XK_dollar,       display::Key::dollar_sign          },
    { XK_percent,      display::Key::percent_sign         },
    { XK_ampersand,    display::Key::ampersand            },
    { XK_apostrophe,   display::Key::apostrophe           },
    { XK_parenleft,    display::Key::left_parenthesis     },
    { XK_parenright,   display::Key::right_parenthesis    },
    { XK_asterisk,     display::Key::asterisk             },
    { XK_plus,         display::Key::plus_sign            },
    { XK_comma,        display::Key::comma                },
    { XK_minus,        display::Key::minus_sign           },
    { XK_period,       display::Key::period               },
    { XK_slash,        display::Key::slash                },
    { XK_0,            display::Key::digit_0              },
    { XK_1,            display::Key::digit_1              },
    { XK_2,            display::Key::digit_2              },
    { XK_3,            display::Key::digit_3              },
    { XK_4,            display::Key::digit_4              },
    { XK_5,            display::Key::digit_5              },
    { XK_6,            display::Key::digit_6              },
    { XK_7,            display::Key::digit_7              },
    { XK_8,            display::Key::digit_8              },
    { XK_9,            display::Key::digit_9              },
    { XK_colon,        display::Key::colon                },
    { XK_semicolon,    display::Key::semicolon            },
    { XK_less,         display::Key::less_than_sign       },
    { XK_equal,        display::Key::equal_sign           },
    { XK_greater,      display::Key::greater_than_sign    },
    { XK_question,     display::Key::question_mark        },
    { XK_at,           display::Key::at_sign              },
    { XK_A,            display::Key::upper_case_a         },
    { XK_B,            display::Key::upper_case_b         },
    { XK_C,            display::Key::upper_case_c         },
    { XK_D,            display::Key::upper_case_d         },
    { XK_E,            display::Key::upper_case_e         },
    { XK_F,            display::Key::upper_case_f         },
    { XK_G,            display::Key::upper_case_g         },
    { XK_H,            display::Key::upper_case_h         },
    { XK_I,            display::Key::upper_case_i         },
    { XK_J,            display::Key::upper_case_j         },
    { XK_K,            display::Key::upper_case_k         },
    { XK_L,            display::Key::upper_case_l         },
    { XK_M,            display::Key::upper_case_m         },
    { XK_N,            display::Key::upper_case_n         },
    { XK_O,            display::Key::upper_case_o         },
    { XK_P,            display::Key::upper_case_p         },
    { XK_Q,            display::Key::upper_case_q         },
    { XK_R,            display::Key::upper_case_r         },
    { XK_S,            display::Key::upper_case_s         },
    { XK_T,            display::Key::upper_case_t         },
    { XK_U,            display::Key::upper_case_u         },
    { XK_V,            display::Key::upper_case_v         },
    { XK_W,            display::Key::upper_case_w         },
    { XK_X,            display::Key::upper_case_x         },
    { XK_Y,            display::Key::upper_case_y         },
    { XK_Z,            display::Key::upper_case_z         },
    { XK_bracketleft,  display::Key::left_square_bracket  },
    { XK_backslash,    display::Key::backslash            },
    { XK_bracketright, display::Key::right_square_bracket },
    { XK_asciicircum,  display::Key::circumflex_accent    },
    { XK_underscore,   display::Key::underscore           },
    { XK_grave,        display::Key::grave_accent         },
    { XK_a,            display::Key::lower_case_a         },
    { XK_b,            display::Key::lower_case_b         },
    { XK_c,            display::Key::lower_case_c         },
    { XK_d,            display::Key::lower_case_d         },
    { XK_e,            display::Key::lower_case_e         },
    { XK_f,            display::Key::lower_case_f         },
    { XK_g,            display::Key::lower_case_g         },
    { XK_h,            display::Key::lower_case_h         },
    { XK_i,            display::Key::lower_case_i         },
    { XK_j,            display::Key::lower_case_j         },
    { XK_k,            display::Key::lower_case_k         },
    { XK_l,            display::Key::lower_case_l         },
    { XK_m,            display::Key::lower_case_m         },
    { XK_n,            display::Key::lower_case_n         },
    { XK_o,            display::Key::lower_case_o         },
    { XK_p,            display::Key::lower_case_p         },
    { XK_q,            display::Key::lower_case_q         },
    { XK_r,            display::Key::lower_case_r         },
    { XK_s,            display::Key::lower_case_s         },
    { XK_t,            display::Key::lower_case_t         },
    { XK_u,            display::Key::lower_case_u         },
    { XK_v,            display::Key::lower_case_v         },
    { XK_w,            display::Key::lower_case_w         },
    { XK_x,            display::Key::lower_case_x         },
    { XK_y,            display::Key::lower_case_y         },
    { XK_z,            display::Key::lower_case_z         },
    { XK_braceleft,    display::Key::left_curly_bracket   },
    { XK_bar,          display::Key::vertical_bar         },
    { XK_braceright,   display::Key::right_curly_bracket  },
    { XK_asciitilde,   display::Key::tilde                },

    // Latin-1 Supplement
};


constexpr core::LiteralHashMap g_key_map     = core::make_literal_hash_map(key_assocs);
constexpr core::LiteralHashMap g_rev_key_map = core::make_rev_literal_hash_map(key_assocs);


inline bool map_key(display::KeyCode key_code, display::Key& key) noexcept
{
    auto keysym = KeySym(key_code.code);
    return g_key_map.find(keysym, key);
}


inline bool rev_map_key(display::Key key, display::KeyCode& key_code) noexcept
{
    KeySym keysym = {};
    if (ARCHON_LIKELY(g_rev_key_map.find(key, keysym))) {
        key_code = { display::KeyCode::code_type(keysym) };
        return true;
    }
    return false;
}


bool try_map_mouse_button(unsigned x11_button, bool& is_scroll, display::MouseButton& button,
                          math::Vector2F& amount) noexcept
{
    switch (x11_button) {
        case 1:
            is_scroll = false;
            button = display::MouseButton::left;
            return true;
        case 2:
            is_scroll = false;
            button = display::MouseButton::middle;
            return true;
        case 3:
            is_scroll = false;
            button = display::MouseButton::right;
            return true;
        case 4:
            is_scroll = true;
            amount = { 0, +1 }; // Scroll up
            return true;
        case 5:
            is_scroll = true;
            amount = { 0, -1 }; // Scroll down
            return true;
        case 6:
            is_scroll = true;
            amount = { -1, 0 }; // Scroll left
            return true;
        case 7:
            is_scroll = true;
            amount = { +1, 0 }; // Scroll right
            return true;
        case 8:
            is_scroll = false;
            button = display::MouseButton::x1;
            return true;
        case 9:
            is_scroll = false;
            button = display::MouseButton::x2;
            return true;
    }
    return false;
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
