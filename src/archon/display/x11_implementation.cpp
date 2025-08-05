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
#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <memory>
#include <utility>
#include <stdexcept>
#include <optional>
#include <string_view>
#include <string>
#include <locale>
#include <chrono>

#include <archon/core/features.h>
#include <archon/core/pair.hpp>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/memory.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/deque.hpp>
#include <archon/core/flat_map.hpp>
#include <archon/core/flat_set.hpp>
#include <archon/core/literal_hash_map.hpp>
#include <archon/core/format.hpp>
#include <archon/core/as_int.hpp>
#include <archon/core/format_as.hpp>
#include <archon/core/quote.hpp>
#include <archon/core/platform_support.hpp>
#include <archon/log.hpp>
#include <archon/math/vector.hpp>
#include <archon/util/color.hpp>
#include <archon/image.hpp>
#include <archon/display/geometry.hpp>
#include <archon/display/key.hpp>
#include <archon/display/key_code.hpp>
#include <archon/display/mouse_button.hpp>
#include <archon/display/event.hpp>
#include <archon/display/event_handler.hpp>
#include <archon/display/viewport.hpp>
#include <archon/display/noinst/timestamp_unwrapper.hpp>
#include <archon/display/noinst/edid.hpp>
#include <archon/display/guarantees.hpp>
#include <archon/display/x11_fullscreen_monitors.hpp>
#include <archon/display/x11_connection_config.hpp>
#include <archon/display/texture.hpp>
#include <archon/display/window.hpp>
#include <archon/display/connection.hpp>
#include <archon/display/implementation.hpp>
#include <archon/display/x11_implementation.hpp>
#include <archon/display/noinst/impl_util.hpp>
#include <archon/display/noinst/x11/support.hpp>


using namespace archon;
namespace impl = display::impl;
namespace x11 = impl::x11;


namespace {


constexpr std::string_view g_implementation_ident = "x11";
constexpr std::string_view g_implementation_descr = "X11 (X Window System, Version 11)";


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


auto map_opt_visual_type(const std::optional<std::uint_fast32_t>& type) -> std::optional<VisualID>
{
    if (ARCHON_LIKELY(!type.has_value()))
        return {};
    std::uint_fast32_t type_2 = type.value();
    if (ARCHON_LIKELY(type_2 <= core::int_mask<std::uint_fast32_t>(32)))
        return VisualID(type_2);
    throw std::invalid_argument("Visual type out of range");
}


// One slot for each X11 screen
struct ScreenSlot {
    bool is_initialized = false;
    bool have_standard_colormaps = false;
    int screen = {};
    Window root = {};
    VisualID default_visual = {};
    Colormap default_colormap = {};
    core::Slab<x11::VisualSpec> visual_specs;
    core::FlatMap<VisualID, XStandardColormap> standard_colormaps;

    // Key is (depth, visual)
    core::FlatMap<core::Pair<int, VisualID>, std::unique_ptr<x11::PixelFormat>> pixel_formats;
    core::FlatMap<core::Pair<int, VisualID>, std::unique_ptr<x11::ImageBridge>> image_bridges;

#if HAVE_XRANDR
    x11::ScreenConf screen_conf;
#endif
};


class ColormapFinderImpl final
    : public x11::ColormapFinder {
public:
    ColormapFinderImpl(Display*, ScreenSlot&, log::Logger&) noexcept;

    bool find_default_colormap(VisualID, Colormap&) const noexcept override;
    bool find_standard_colormap(VisualID, XStandardColormap&) const override;

private:
    Display* const m_dpy;
    ScreenSlot& m_screen_slot;
    log::Logger& m_logger;
};


inline ColormapFinderImpl::ColormapFinderImpl(Display* dpy, ScreenSlot& screen_slot, log::Logger& logger) noexcept
    : m_dpy(dpy)
    , m_screen_slot(screen_slot)
    , m_logger(logger)
{
}


bool ColormapFinderImpl::find_default_colormap(VisualID visual, Colormap& colormap) const noexcept
{
    if (visual == m_screen_slot.default_visual) {
        colormap = m_screen_slot.default_colormap;
        return true;
    }
    return false;
}


bool ColormapFinderImpl::find_standard_colormap(VisualID visual, XStandardColormap& colormap_params) const
{
    if (ARCHON_LIKELY(m_screen_slot.have_standard_colormaps))
        goto have;
    m_screen_slot.standard_colormaps = x11::fetch_standard_colormaps(m_dpy, m_screen_slot.root); // Throws
    m_screen_slot.have_standard_colormaps = true;
    {
        core::NumOfSpec spec = { "standard colormap", "standard colormaps" };
        m_logger.detail("Found %s on screen %s", core::as_num_of(m_screen_slot.standard_colormaps.size(), spec),
                        core::as_int(m_screen_slot.screen)); // Throws
    }

  have:
    {
        auto i = m_screen_slot.standard_colormaps.find(visual);
        if (i != m_screen_slot.standard_colormaps.end()) {
            colormap_params = i->second;
            return true;
        }
    }
    return false;
}



bool map_key(display::KeyCode, display::Key&) noexcept;
bool rev_map_key(display::Key, display::KeyCode&) noexcept;

bool try_map_mouse_button(unsigned x11_button, bool& is_scroll, display::MouseButton& button,
                          math::Vector2F& amount) noexcept;


class WindowImpl;
class TextureImpl;


class ImplementationImpl final
    : public display::Implementation {
public:
    ImplementationImpl(Slot&) noexcept;

    bool try_new_connection(const std::locale&, const display::Connection::Config&,
                            std::unique_ptr<display::Connection>&, std::string&) const override;
    auto get_slot() const noexcept -> const Slot& override;

private:
    const Slot& m_slot;
};


class SlotImpl final
    : public display::Implementation::Slot {
public:
    SlotImpl() noexcept;

    auto get_ident() const noexcept -> std::string_view override;
    auto get_descr() const noexcept -> std::string_view override;
    auto get_implementation_a(const display::Guarantees&) const noexcept -> const display::Implementation* override;

private:
    ImplementationImpl m_impl;
};


class ConnectionImpl final
    : private display::ConnectionEventHandler
    , public display::Connection {
public:
    const ImplementationImpl& impl;
    const std::locale locale;
    log::Logger& logger;
    display::ConnectionEventHandler* event_handler = this;
    x11::DisplayWrapper dpy_owner;
    Display* dpy = nullptr;

    x11::ExtensionInfo extension_info = {};

    Atom atom_wm_protocols;
    Atom atom_wm_delete_window;
    Atom atom_net_wm_fullscreen_monitors;
    Atom atom_net_wm_state;
    Atom atom_net_wm_state_fullscreen;

#if HAVE_XRANDR
    Atom atom_edid;
#endif

    ConnectionImpl(const ImplementationImpl&, const std::locale&, log::Logger*, const display::x11_connection_config&);

    bool try_open(const display::x11_connection_config&, std::string&);
    void register_window(WindowImpl&);
    void unregister_window(WindowImpl&) noexcept;
    auto ensure_image_bridge(const XVisualInfo&, const x11::PixelFormat&) const -> x11::ImageBridge&;
    void set_fullscreen_monitors(::Window win, ::Window root);

    bool try_map_key_to_key_code(display::Key, display::KeyCode&) const override;
    bool try_map_key_code_to_key(display::KeyCode, display::Key&) const override;
    bool try_get_key_name(display::KeyCode, std::string_view&) const override;
    bool try_new_window(std::string_view, display::Size, const display::Window::Config&,
                        std::unique_ptr<display::Window>&, std::string&) override;
    void set_event_handler(display::ConnectionEventHandler&) override;
    void unset_event_handler() noexcept override;
    void process_events() override;
    bool process_events_a(time_point_type) override;
    int get_num_screens() const override;
    int get_default_screen() const override;
    bool try_get_screen_conf(int, core::Buffer<display::Viewport>&, core::Buffer<char>&, std::size_t&) const override;
    auto get_implementation() const noexcept -> const display::Implementation& override;

private:
    const std::optional<int> m_depth_override;
    const std::optional<int> m_class_override;
    const std::optional<VisualID> m_visual_override;
    const std::optional<display::x11_fullscreen_monitors> m_fullscreen_monitors;
    const bool m_prefer_default_nondecomposed_colormap;
    const bool m_disable_double_buffering;
    const bool m_disable_glx_direct_rendering;
    const bool m_install_colormaps;
    const bool m_colormap_weirdness;

    bool m_detectable_autorepeat_enabled = false;
    bool m_expect_keymap_notify = false;
    bool m_have_curr_window = false;

    core::FlatMap<int, XPixmapFormatValues> m_pixmap_formats; // Key is visual depth

    mutable std::unique_ptr<ScreenSlot[]> m_screen_slots;
    mutable core::FlatMap<::Window, int> m_screens_by_root;

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
    // implementation in particular (`sdl_implementation.cpp`), the required behavior of
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
    //
    core::Deque<WindowImpl*> m_exposed_windows;

    // X11 timestamps are 32-bit unsigned integers and `Time` refers to the unsigned integer
    // type that X11 uses to store these timestamps.
    //
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
    auto ensure_screen_slot(int screen) const -> ScreenSlot&;
    bool determine_visual_spec(const ScreenSlot&, bool prefer_double_buffered, bool require_opengl,
                               bool require_depth_buffer, const x11::VisualSpec*&, std::string& error) const;
    auto get_pixmap_format(int depth) const -> const XPixmapFormatValues&;
    auto ensure_pixel_format(ScreenSlot&, const XVisualInfo&) const -> const x11::PixelFormat&;
    bool do_process_events(const time_point_type* deadline);
    bool process_event_batch();
    bool after_event_batch();
    bool lookup_window(::Window window_id, WindowImpl*& window) noexcept;
    void track_pointer_grabs(::Window window_id, unsigned button, bool is_press);
    bool is_pointer_grabbed() const noexcept;

#if HAVE_XRANDR
    bool update_screen_conf(ScreenSlot& slot) const;
    auto ensure_edid_parser() const -> const impl::EdidParser&;
#endif
};


class WindowImpl final
    : private display::WindowEventHandler
    , public display::Window {
public:
    ConnectionImpl& conn;
    const ScreenSlot& screen_slot;
    const x11::VisualSpec& visual_spec;
    const int cookie;
    display::WindowEventHandler* event_handler = this;

    ::Window win = None;

    bool has_pending_expose_event = false;

    WindowImpl(ConnectionImpl&, const ScreenSlot&, const x11::VisualSpec&, const x11::PixelFormat&,
               int cookie) noexcept;
    ~WindowImpl() noexcept override;

    void create(display::Size size, const Config& config, bool enable_double_buffering,
                bool enable_opengl, bool enable_glx_direct_rendering);
    auto ensure_image_bridge() -> x11::ImageBridge&;
    auto ensure_graphics_context() noexcept -> GC;

    void set_event_handler(display::WindowEventHandler&) noexcept override;
    void unset_event_handler() noexcept override;
    void show() override;
    void hide() override;
    void set_title(std::string_view) override;
    void set_size(display::Size) override;
    void set_fullscreen_mode(bool) override;
    void fill(util::Color) override;
    void fill(util::Color, const display::Box&) override;
    auto new_texture(display::Size) -> std::unique_ptr<display::Texture> override;
    void put_texture(const display::Texture&, const display::Pos&) override;
    void put_texture(const display::Texture&, const display::Box&, const display::Pos&) override;
    void present() override;
    void opengl_make_current() override;
    void opengl_swap_buffers() override;

private:
    bool m_is_registered = false;
    bool m_is_double_buffered = false;
    bool m_is_mapped = false;
    bool m_fullscreen_mode = false;

    const x11::PixelFormat& m_pixel_format;
    x11::ImageBridge* m_image_bridge = nullptr;

    GC m_gc = None;

    Drawable m_drawable;
#if HAVE_XDBE
    XdbeSwapAction m_swap_action;
#endif

#if HAVE_GLX
    GLXContext m_ctx = nullptr;
#endif

    void set_property(Atom name, Atom value) noexcept;
    void do_set_fullscreen_mode(bool on);
    void do_fill(util::Color color, int x, int y, unsigned w, unsigned h);
    void do_put_texture(const TextureImpl&, const display::Box& source_area, const display::Pos& pos);
    auto create_image_bridge() -> x11::ImageBridge&;
    auto create_graphics_context() noexcept -> GC;
    auto intern_color(util::Color) -> unsigned long;
};


class TextureImpl final
    : public display::Texture {
public:
    WindowImpl& win;
    const display::Size size;
    Pixmap pixmap = None;

    TextureImpl(WindowImpl&, const display::Size& size);
    ~TextureImpl() noexcept override;

    void create();

    void put_image(const image::Image&) override;
};



inline ImplementationImpl::ImplementationImpl(Slot& slot) noexcept
    : m_slot(slot)
{
}


bool ImplementationImpl::try_new_connection(const std::locale& locale, const display::Connection::Config& config,
                                            std::unique_ptr<display::Connection>& conn, std::string& error) const
{
    auto conn_2 = std::make_unique<ConnectionImpl>(*this, locale, config.logger, config.x11); // Throws
    if (ARCHON_LIKELY(conn_2->try_open(config.x11, error))) { // Throws
        conn = std::move(conn_2);
        return true;
    }
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


auto SlotImpl::get_ident() const noexcept -> std::string_view
{
    return g_implementation_ident;
}


auto SlotImpl::get_descr() const noexcept -> std::string_view
{
    return g_implementation_descr;
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
                                      log::Logger* logger, const display::x11_connection_config& config)
    : impl(impl_2)
    , locale(locale_2)
    , logger(log::Logger::or_null(logger))
    , m_depth_override(config.visual_depth)
    , m_class_override(x11::map_opt_visual_class(config.visual_class))
    , m_visual_override(map_opt_visual_type(config.visual_type)) // Throws
    , m_fullscreen_monitors(config.fullscreen_monitors)
    , m_prefer_default_nondecomposed_colormap(config.prefer_default_nondecomposed_colormap)
    , m_disable_double_buffering(config.disable_double_buffering)
    , m_disable_glx_direct_rendering(config.disable_glx_direct_rendering)
    , m_install_colormaps(config.install_colormaps)
    , m_colormap_weirdness(config.colormap_weirdness)
{
}


bool ConnectionImpl::try_open(const display::x11_connection_config& config, std::string& error)
{
    std::string_view display = x11::get_display_string(config.display);
    if (ARCHON_UNLIKELY(!x11::try_connect(display, dpy_owner))) { // Throws
        error = core::format(locale, "Failed to connect to %s", core::quoted(display)); // Throws
        return false;
    }

    dpy = dpy_owner;

    if (ARCHON_UNLIKELY(config.synchronous_mode))
        XSynchronize(dpy, True);

    extension_info = x11::init_extensions(dpy); // Throws
    if (ARCHON_UNLIKELY(!extension_info.have_xkb)) {
        error = "X Keyboard Extension is required but not available"; // Throws
        return false;
    }

    if (!config.disable_detectable_autorepeat) {
        Bool detectable = True;
        Bool supported = {};
        XkbSetDetectableAutoRepeat(dpy, detectable, &supported);
        if (ARCHON_LIKELY(supported))
            m_detectable_autorepeat_enabled = true;
    }

    m_pixmap_formats = x11::fetch_pixmap_formats(dpy); // Throws

    atom_wm_protocols               = intern_string("WM_PROTOCOLS");
    atom_wm_delete_window           = intern_string("WM_DELETE_WINDOW");
    atom_net_wm_fullscreen_monitors = intern_string("_NET_WM_FULLSCREEN_MONITORS");
    atom_net_wm_state               = intern_string("_NET_WM_STATE");
    atom_net_wm_state_fullscreen    = intern_string("_NET_WM_STATE_FULLSCREEN");

#if HAVE_XRANDR
    atom_edid = intern_string(RR_PROPERTY_RANDR_EDID);
#endif

    m_screen_slots = std::make_unique<ScreenSlot[]>(std::size_t(ScreenCount(dpy))); // Throws

    return true;
}


inline void ConnectionImpl::register_window(WindowImpl& window)
{
    ::Window id = window.win;
    auto i = m_windows.emplace(id, window); // Throws
    bool was_inserted = i.second;
    ARCHON_ASSERT(was_inserted);
    // Because a new window might reuse the ID currently specified by `m_curr_window_id`, it
    // is necessary, and not just desirable to reset the "current window state" here.
    m_curr_window_id = id;
    m_curr_window = &window;
    m_have_curr_window = true;
}


inline void ConnectionImpl::unregister_window(WindowImpl& window) noexcept
{
    ::Window id = window.win;
    std::size_t n = m_windows.erase(id);
    ARCHON_ASSERT(n == 1);

    if (ARCHON_UNLIKELY(m_pointer_grab_window_id == id))
        m_pointer_grab_buttons.clear();

    auto i = std::find(m_exposed_windows.begin(), m_exposed_windows.end(), &window);
    if (i != m_exposed_windows.end())
        m_exposed_windows.erase(i);

    if (ARCHON_LIKELY(m_have_curr_window && m_curr_window_id == id))
        m_curr_window = nullptr;
}


auto ConnectionImpl::ensure_image_bridge(const XVisualInfo& visual_info,
                                         const x11::PixelFormat& pixel_format) const -> x11::ImageBridge&
{
    int screen = visual_info.screen;
    ARCHON_ASSERT(m_screen_slots);
    ARCHON_ASSERT(screen >= 0 && screen <= ScreenCount(dpy));
    ScreenSlot& screen_slot = m_screen_slots[screen];
    core::Pair key = { visual_info.depth, visual_info.visualid };
    auto i = screen_slot.image_bridges.find(key);
    if (ARCHON_LIKELY(i != screen_slot.image_bridges.end()))
        return *i->second;

    std::unique_ptr<x11::ImageBridge> image_bridge =
        pixel_format.create_image_bridge(impl::subdivide_max_subbox_size); // Throws
    auto p = screen_slot.image_bridges.emplace(key, std::move(image_bridge));
    bool was_inserted = p.second;
    ARCHON_ASSERT(was_inserted);
    i = p.first;
    return *i->second;
}


void ConnectionImpl::set_fullscreen_monitors(::Window win, ::Window root)
{
    if (ARCHON_LIKELY(!m_fullscreen_monitors.has_value()))
        return;
    x11::set_fullscreen_monitors(dpy, win, m_fullscreen_monitors.value(), root,
                                 atom_net_wm_fullscreen_monitors); // Throws
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


bool ConnectionImpl::try_new_window(std::string_view title, display::Size size, const display::Window::Config& config,
                                    std::unique_ptr<display::Window>& win, std::string& error)
{
    if (ARCHON_UNLIKELY(size.width < 0 || size.height < 0))
        throw std::invalid_argument("Bad window size");
    int screen = config.screen;
    if (ARCHON_LIKELY(screen < 0)) {
        screen = DefaultScreen(dpy);
    }
    else if (ARCHON_UNLIKELY(screen < 0 || screen >= int(ScreenCount(dpy)))) {
        throw std::invalid_argument("Bad screen index");
    }
    bool prefer_double_buffered = false;
    if (ARCHON_LIKELY(!m_disable_double_buffering))
        prefer_double_buffered = true;
    bool enable_opengl = false;
    if (config.enable_opengl_rendering) {
        if (ARCHON_UNLIKELY(!extension_info.have_glx)) {
            error = "OpenGL rendering not available";
            return false;
        }
        enable_opengl = true;
    }
    ScreenSlot& screen_slot = ensure_screen_slot(screen); // Throws
    bool require_depth_buffer = config.require_opengl_depth_buffer;
    const x11::VisualSpec* visual_spec = {};
    if (ARCHON_LIKELY(determine_visual_spec(screen_slot, prefer_double_buffered, enable_opengl, require_depth_buffer,
                                            visual_spec, error))) { // Throws
        const XVisualInfo& info = visual_spec->info;
        logger.detail("Using %s visual (%s) of depth %s for new X11 window", x11::get_visual_class_name(info.c_class),
                      core::as_flex_int_h(info.visualid), info.depth); // Throws
        const x11::PixelFormat& pixel_format = ensure_pixel_format(screen_slot, info); // Throws
        auto win_2 = std::make_unique<WindowImpl>(*this, screen_slot, *visual_spec, pixel_format, config.cookie); // Throws
        bool enable_double_buffering = visual_spec->double_buffered && !m_disable_double_buffering;
        bool enable_glx_direct_rendering = !m_disable_glx_direct_rendering;
        win_2->create(size, config, enable_double_buffering, enable_opengl, enable_glx_direct_rendering); // Throws
        win_2->set_title(title); // Throws
        if (ARCHON_UNLIKELY(m_install_colormaps))
            XInstallColormap(dpy, pixel_format.get_colormap());
        win = std::move(win_2);
        return true;
    }
    return false;
}


void ConnectionImpl::set_event_handler(display::ConnectionEventHandler& handler)
{
    event_handler = &handler;
}


void ConnectionImpl::unset_event_handler() noexcept
{
    event_handler = this;
}


void ConnectionImpl::process_events()
{
    const time_point_type* deadline = nullptr;
    do_process_events(deadline); // Throws
}


bool ConnectionImpl::process_events_a(time_point_type deadline)
{
    return do_process_events(&deadline); // Throws
}


int ConnectionImpl::get_num_screens() const
{
    return int(ScreenCount(dpy));
}


int ConnectionImpl::get_default_screen() const
{
    return int(DefaultScreen(dpy));
}


bool ConnectionImpl::try_get_screen_conf(int screen, core::Buffer<display::Viewport>& viewports,
                                         core::Buffer<char>& strings, std::size_t& num_viewports) const
{
    if (ARCHON_UNLIKELY(screen < 0 || screen >= int(ScreenCount(dpy))))
        throw std::invalid_argument("Bad screen index");

#if HAVE_XRANDR
    if (extension_info.have_xrandr) {
        const ScreenSlot& slot = ensure_screen_slot(screen); // Throws
        const x11::ScreenConf& conf = slot.screen_conf;
        std::size_t n = conf.viewports.size();
        viewports.reserve(n); // Throws
        const char* strings_base = conf.string_buffer.data();
        strings.assign({ strings_base, conf.string_buffer_used_size }); // Throws
        const char* strings_base_2 = strings.data();
        for (std::size_t i = 0; i < n; ++i) {
            const x11::ProtoViewport& proto = conf.viewports[i];
            std::optional<std::string_view> monitor_name;
            if (proto.monitor_name.has_value())
                monitor_name = proto.monitor_name.value().resolve_string(strings_base_2); // Throws
            viewports[i] = {
                proto.output_name.resolve_string(strings_base_2), // Throws
                proto.bounds,
                monitor_name,
                proto.resolution,
                proto.refresh_rate,
            };
        }
        num_viewports = n;
        return true;
    }
    return false;
#else // !HAVE_XRANDR
    static_cast<void>(viewports);
    static_cast<void>(strings);
    static_cast<void>(num_viewports);
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
    ARCHON_STEADY_ASSERT(atom != None);
    return atom;
}


auto ConnectionImpl::ensure_screen_slot(int screen) const -> ScreenSlot&
{
    ARCHON_ASSERT(m_screen_slots);
    ARCHON_ASSERT(screen >= 0 && screen <= ScreenCount(dpy));
    ScreenSlot& slot = m_screen_slots[screen];
    if (ARCHON_UNLIKELY(!slot.is_initialized)) {
        ::Window root = RootWindow(dpy, screen);
        slot.screen = screen;
        slot.root = root;
        slot.default_visual = XVisualIDFromVisual(DefaultVisual(dpy, screen));
        slot.default_colormap = DefaultColormap(dpy, screen);
        m_screens_by_root[root] = screen; // Throws

        // Fetch information about supported visuals
        slot.visual_specs = x11::load_visuals(dpy, screen, extension_info); // Throws

        // Fetch initial screen configuration
#if HAVE_XRANDR
        if (ARCHON_LIKELY(extension_info.have_xrandr)) {
            int mask = RROutputChangeNotifyMask | RRCrtcChangeNotifyMask;
            XRRSelectInput(dpy, root, mask);
            update_screen_conf(slot); // Throws
        }
#endif // HAVE_XRANDR

        slot.is_initialized = true;
    }
    return slot;
}


bool ConnectionImpl::determine_visual_spec(const ScreenSlot& screen_slot, bool prefer_double_buffered,
                                           bool require_opengl, bool require_depth_buffer,
                                           const x11::VisualSpec*& spec, std::string& error) const
{
    core::Span visual_specs = screen_slot.visual_specs;
    x11::FindVisualParams params;
    params.visual_depth = m_depth_override;
    params.visual_class = m_class_override;
    params.visual_type = m_visual_override;
    params.prefer_double_buffered = prefer_double_buffered;
    params.require_opengl = require_opengl;
    params.require_opengl_depth_buffer = require_opengl && require_depth_buffer;
    std::size_t index = {};
    if (ARCHON_LIKELY(x11::find_visual(dpy, screen_slot.screen, visual_specs, params, index))) { // Throws
        spec = &visual_specs[index];
        return true;
    }
    error = "No suitable X11 visual found"; // Throws
    return false;
}


inline auto ConnectionImpl::get_pixmap_format(int depth) const -> const XPixmapFormatValues&
{
    auto i = m_pixmap_formats.find(depth);
    if (i != m_pixmap_formats.end())
        return i->second;
    throw std::runtime_error("Pixmap format not found for selected depth");
}


auto ConnectionImpl::ensure_pixel_format(ScreenSlot& screen_slot,
                                         const XVisualInfo& visual_info) const -> const x11::PixelFormat&
{
    core::Pair key = { visual_info.depth, visual_info.visualid };
    auto i = screen_slot.pixel_formats.find(key);
    if (ARCHON_LIKELY(i != screen_slot.pixel_formats.end()))
        return *i->second;
    const XPixmapFormatValues& pixmap_format = get_pixmap_format(visual_info.depth); // Throws
    ColormapFinderImpl colormap_finder(dpy, screen_slot, logger);
    std::unique_ptr<x11::PixelFormat> pixel_format =
        x11::create_pixel_format(dpy, screen_slot.root, visual_info, pixmap_format, colormap_finder,
                                 locale, logger, m_prefer_default_nondecomposed_colormap,
                                 m_colormap_weirdness); // Throws
    auto p = screen_slot.pixel_formats.emplace(key, std::move(pixel_format)); // Throws
    bool was_inserted = p.second;
    ARCHON_ASSERT(was_inserted);
    i = p.first;
    return *i->second;
}


bool ConnectionImpl::do_process_events(const time_point_type* deadline)
{
    // The implementation below takes care to meet the general requirements for display
    // implementations (see display::Connection::process_events_a()) as well as the
    // following additional requirements:
    //
    //  * There must be no unflushed X11 requests when sleeping takes place. Below, this is
    //    ensured by the fact there is no opportunity for X11 requests to be generated
    //    between the flushing read (call to read() with QueuedAfterFlush) and the sleep
    //    (call to wait()).
    //
    //  * There must be no events buffered inside Xlib when sleeping takes place. Below,
    //    this is ensured by the fact that there is no invocation of any Xlib function
    //    between the sleep (call to wait()) and the preceding read (call to read()). Not
    //    that due to the nature of the X11 protocol and the design of Xlib, there can be
    //    events that have been read from the network connection but have not yet been seen
    //    by the application. Since such events will be invisible to poll(), an explicit
    //    check is necessary.
    //

    auto read = [&](int mode) noexcept {
        int n = XEventsQueued(dpy, mode);
        // If generation of X11 events happens fast enough to saturate processing, `n` could
        // grow without bounds over time. A ceiling is put on `n` in order to avoid this,
        // and to live up to the starvation prevention requirements for the implementations
        // of display::Connection::process_events_a().
        m_num_events = std::min(n, 256);
    };

    auto determine_timeout = [&](int& timeout, bool& partial) noexcept {
        if (ARCHON_LIKELY(deadline)) {
            time_point_type now = clock_type::now();
            if (ARCHON_LIKELY(*deadline > now)) {
                auto duration = std::chrono::ceil<std::chrono::milliseconds>(*deadline - now).count();
                timeout = core::int_max<int>();
                partial = true;
                if (ARCHON_LIKELY(core::int_less_equal(duration, timeout))) {
                    timeout = int(duration);
                    partial = false;
                }
                return true; // Timeout determined
            }
            return false; // Deadline expired
        }
        timeout = -1;
        partial = false;
        return true; // No timeout
    };

    auto wait = [&](int timeout, bool& partial) {
        pollfd fds[1] {};
        int nfds = 1;
        fds[0].fd = ConnectionNumber(dpy);
        fds[0].events = POLLIN;
        int ret = ::poll(fds, nfds, timeout);
        if (ARCHON_LIKELY(ret > 0)) {
            ARCHON_ASSERT(ret == 1);
            return true; // Ready for reading
        }
        if (ARCHON_LIKELY(ret == 0)) {
            ARCHON_ASSERT(timeout >= 0);
            return false; // Timed out
        }
        int err = errno; // Eliminate any risk of clobbering
        if (ARCHON_LIKELY(err == EINTR)) {
            partial = true;
            return false; // Interrupted system call
        }
        core::throw_system_error(err, "Failed to poll file descriptor of X11 connection"); // Throws
    };

  process:
    if (ARCHON_LIKELY(process_event_batch())) { // Throws
        if (ARCHON_LIKELY(after_event_batch())) { // Throws
            if (ARCHON_LIKELY(event_handler->before_sleep())) { // Throws
                ARCHON_ASSERT(m_num_events == 0);
                read(QueuedAfterFlush); // Non-blocking read with preceding flush
                for (;;) {
                    int timeout = {};
                    bool partial = {};
                    if (ARCHON_LIKELY(determine_timeout(timeout, partial))) {
                        if (ARCHON_LIKELY(m_num_events > 0))
                            goto process;
                        if (ARCHON_LIKELY(wait(timeout, partial))) { // Throws
                            read(QueuedAfterReading); // Non-blocking read without preceding flush
                            continue;
                        }
                        if (ARCHON_LIKELY(!partial))
                            return true; // Deadline expired
                        continue;
                    }
                    return true; // Deadline expired
                }
            }
        }
    }
    return false; // Interrupted
}


bool ConnectionImpl::process_event_batch()
{
    XEvent ev = {};
    WindowImpl* window = {};
    timestamp_unwrapper_type::Session unwrap_session(m_timestamp_unwrapper);
    bool expect_keymap_notify;

    auto expose = [&] {
        if (ARCHON_LIKELY(window->has_pending_expose_event))
            return;
        m_exposed_windows.push_back(window); // Throws
        window->has_pending_expose_event = true;
    };

  process_1:
    if (ARCHON_LIKELY(m_num_events > 0))
        goto process_2;
    return true; // Batch was fully processed

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
                bool proceed = window->event_handler->on_mousemove(event); // Throws
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
                    proceed = window->event_handler->on_reposition(event); // Throws
                }
                else {
                    expose(); // Throws
                    display::WindowSizeEvent event;
                    event.cookie = window->cookie;
                    event.size = { ev.xconfigure.width, ev.xconfigure.height };
                    proceed = window->event_handler->on_resize(event); // Throws
                }
                if (ARCHON_LIKELY(proceed))
                    break;
                return false; // Interrupt
            }
            break;

        case Expose:
            if (ARCHON_LIKELY(!lookup_window(ev.xexpose.window, window)))
                break;
            expose(); // Throws
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
                        bool proceed = window->event_handler->on_scroll(event); // Throws
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
                            proceed = window->event_handler->on_mousedown(event); // Throws
                        }
                        else {
                            proceed = window->event_handler->on_mouseup(event); // Throws
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
                        proceed = window->event_handler->on_keydown(event); // Throws
                    }
                    else {
                        proceed = window->event_handler->on_keyrepeat(event); // Throws
                    }
                }
                else {
                    proceed = window->event_handler->on_keyup(event); // Throws
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
                    proceed = window->event_handler->on_mouseover(event); // Throws
                }
                else {
                    proceed = window->event_handler->on_mouseout(event); // Throws
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
                    proceed = window->event_handler->on_focus(event); // Throws
                }
                else {
                    proceed = window->event_handler->on_blur(event); // Throws
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
                bool proceed = window->event_handler->on_close(event); // Throws
                if (ARCHON_LIKELY(!proceed))
                    return false; // Interrupt
            }
            break;
    }

#if HAVE_XRANDR
    if (extension_info.have_xrandr && ev.type == extension_info.xrandr_event_base + RRNotify) {
        const auto& ev_2 = reinterpret_cast<const XRRNotifyEvent&>(ev);
        switch (ev_2.subtype) {
            case RRNotify_CrtcChange:
            case RRNotify_OutputChange:
                ::Window root = ev_2.window;
                auto i = m_screens_by_root.find(root);
                ARCHON_ASSERT(i != m_screens_by_root.end());
                int screen = i->second;
                ARCHON_ASSERT(screen >= 0 && screen < ScreenCount(dpy));
                ScreenSlot& slot = m_screen_slots[screen];
                if (update_screen_conf(slot)) // Throws
                    event_handler->on_screen_change(screen); // Throws
        }
    }
#endif // HAVE_XRANDR
    goto process_1;
}


bool ConnectionImpl::after_event_batch()
{
    ARCHON_ASSERT(m_num_events == 0);
    for (;;) {
        if (ARCHON_LIKELY(m_exposed_windows.empty()))
            break;
        WindowImpl& window = *m_exposed_windows.front();
        m_exposed_windows.pop_front();
        window.has_pending_expose_event = false;
        display::WindowEvent event;
        event.cookie = window.cookie;
        bool proceed = window.event_handler->on_expose(event); // Throws
        if (ARCHON_LIKELY(!proceed))
            return false; // Interrupt
    }
    return true; // No interruption
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


inline bool ConnectionImpl::update_screen_conf(ScreenSlot& slot) const
{
    const impl::EdidParser& edid_parser = ensure_edid_parser(); // Throws
    return x11::update_screen_conf(dpy, slot.root, atom_edid, edid_parser, locale, slot.screen_conf); // Throws
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
                              const x11::VisualSpec& visual_spec_2, const x11::PixelFormat& pixel_format,
                              int cookie_2) noexcept
    : conn(conn_2)
    , screen_slot(screen_slot_2)
    , visual_spec(visual_spec_2)
    , cookie(cookie_2)
    , m_pixel_format(pixel_format)
{
}


WindowImpl::~WindowImpl() noexcept
{
#if HAVE_GLX
    if (m_ctx)
        glXDestroyContext(conn.dpy, m_ctx);
#endif // HAVE_GLX

    if (ARCHON_LIKELY(win != None)) {
        if (ARCHON_LIKELY(m_is_registered)) {
            if (m_gc != None)
                XFreeGC(conn.dpy, m_gc);
            conn.unregister_window(*this);
        }
        XDestroyWindow(conn.dpy, win);
    }
}


void WindowImpl::create(display::Size size, const Config& config, bool enable_double_buffering, bool enable_opengl,
                        bool enable_glx_direct_rendering)
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
    int depth = visual_spec.info.depth;
    unsigned class_ = InputOutput;
    Visual* visual = visual_spec.info.visual;
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
    attributes.colormap = m_pixel_format.get_colormap();
    win = XCreateWindow(conn.dpy, parent, x, y, width, height, border_width, depth, class_, visual,
                        valuemask, &attributes);

    conn.register_window(*this); // Throws
    m_is_registered = true;

    // Tell window manager to assign input focus to this window
    XWMHints hints = {};
    hints.flags = InputHint;
    hints.input = True;
    XSetWMHints(conn.dpy, win, &hints);

    // Disable resizability if requested
    if (!config.resizable) {
        XSizeHints size_hints = {};
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
        XSizeHints size_hints = {};
        size_hints.flags = PMinSize;
        size_hints.min_width  = min_size.width;
        size_hints.min_height = min_size.height;
        XSetWMSizeHints(conn.dpy, win, &size_hints, XA_WM_NORMAL_HINTS);
    }

    // Ask X server to notify rather than close connection when window is closed
    set_property(conn.atom_wm_protocols, conn.atom_wm_delete_window);

    // Enable double buffering
    m_drawable = win;
#if HAVE_XDBE
    if (ARCHON_LIKELY(enable_double_buffering)) {
        m_swap_action = XdbeUndefined; // Contents of swapped-out buffer becomes undefined
        XdbeBackBuffer back_buffer = XdbeAllocateBackBufferName(conn.dpy, win, m_swap_action);
        m_drawable = back_buffer;
        m_is_double_buffered = true;
    }
#else // !HAVE_XDBE
    static_cast<void>(enable_double_buffering);
#endif // !HAVE_XDBE

    // Create OpenGL rendering context
#if HAVE_GLX
    if (enable_opengl) {
        GLXContext share_context = {}; // No sharing, so far
        Bool direct = Bool(enable_glx_direct_rendering);
        int attrib_list[] = {
            None // End of list
        };
        GLXContext ctx = conn.extension_info.glx_create_context(conn.dpy, visual_spec.fb_config,
                                                                share_context, direct, attrib_list);
        if (ARCHON_UNLIKELY(!ctx))
            throw std::runtime_error("glXCreateContextAttribsARB() failed");
        m_ctx = ctx;
        glXMakeCurrent(conn.dpy, win, m_ctx);
        GLenum err = glewInit();
        if (ARCHON_UNLIKELY(err != GLEW_OK)) {
            const GLubyte* str = glewGetErrorString(err);
            std::string message = core::format(conn.locale, "Failed to initialize GLEW: %s", str); // Throws
            throw std::runtime_error(std::move(message));
        }
    }
#else // !HAVE_GLX
    ARCHON_ASSERT(!enable_opengl);
    static_cast<void>(enable_glx_direct_rendering);
#endif // !HAVE_GLX

    m_fullscreen_mode = config.fullscreen;
}


auto WindowImpl::ensure_image_bridge() -> x11::ImageBridge&
{
    if (ARCHON_LIKELY(m_image_bridge))
        return *m_image_bridge;
    return create_image_bridge(); // Throws
}


inline auto WindowImpl::ensure_graphics_context() noexcept -> GC
{
    if (ARCHON_LIKELY(m_gc != None))
        return m_gc;
    return create_graphics_context();
}


void WindowImpl::set_event_handler(display::WindowEventHandler& handler) noexcept
{
    event_handler = &handler;
}


void WindowImpl::unset_event_handler() noexcept
{
    event_handler = this;
}


void WindowImpl::show()
{
    XMapWindow(conn.dpy, win);
    m_is_mapped = true;
    conn.set_fullscreen_monitors(win, screen_slot.root); // Throws
    if (m_fullscreen_mode)
        do_set_fullscreen_mode(true); // Throws
}


void WindowImpl::hide()
{
    XUnmapWindow(conn.dpy, win);
    m_is_mapped = false;
}


void WindowImpl::set_title(std::string_view title)
{
    x11::TextPropertyWrapper title_2(conn.dpy, title, conn.locale); // Throws
    XSetWMName(conn.dpy, win, &title_2.prop);
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
    m_fullscreen_mode = on;
    if (m_is_mapped)
        do_set_fullscreen_mode(m_fullscreen_mode); // Throws
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
    auto tex = std::make_unique<TextureImpl>(*this, size); // Throws
    tex->create(); // Throws
    return tex;
}


void WindowImpl::put_texture(const display::Texture& tex, const display::Pos& pos)
{
    const TextureImpl& tex_2 = dynamic_cast<const TextureImpl&>(tex); // Throws
    do_put_texture(tex_2, tex_2.size, pos);
}


void WindowImpl::put_texture(const display::Texture& tex, const display::Box& source_area, const display::Pos& pos)
{
    const TextureImpl& tex_2 = dynamic_cast<const TextureImpl&>(tex); // Throws
    if (ARCHON_UNLIKELY(!source_area.contained_in(tex_2.size)))
        throw std::invalid_argument("Source area escapes texture boundary");
    do_put_texture(tex_2, source_area, pos);
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
#if HAVE_GLX
    glXMakeCurrent(conn.dpy, win, m_ctx);
#endif
}


void WindowImpl::opengl_swap_buffers()
{
#if HAVE_GLX
    glXSwapBuffers(conn.dpy, win);
#endif
}


void WindowImpl::set_property(Atom name, Atom value) noexcept
{
    XChangeProperty(conn.dpy, win, name, XA_ATOM, 32, PropModeReplace, reinterpret_cast<unsigned char*>(&value), 1);
}


void WindowImpl::do_set_fullscreen_mode(bool on)
{
    x11::set_fullscreen_mode(conn.dpy, win, on, screen_slot.root, conn.atom_net_wm_state,
                             conn.atom_net_wm_state_fullscreen); // Throws
}


void WindowImpl::do_fill(util::Color color, int x, int y, unsigned w, unsigned h)
{
    GC gc = ensure_graphics_context();
    unsigned long color_2 = intern_color(color); // Throws
    XSetForeground(conn.dpy, gc, color_2);
    XFillRectangle(conn.dpy, m_drawable, gc, x, y, w, h);
}


void WindowImpl::do_put_texture(const TextureImpl& tex, const display::Box& source_area, const display::Pos& pos)
{
    GC gc = ensure_graphics_context();
    int src_x = source_area.pos.x, src_y = source_area.pos.y;
    unsigned width = unsigned(source_area.size.width), height = unsigned(source_area.size.height);
    int dest_x = pos.x, dest_y = pos.y;
    XCopyArea(conn.dpy, tex.pixmap, m_drawable, gc, src_x, src_y, width, height, dest_x, dest_y);
}


auto WindowImpl::create_image_bridge() -> x11::ImageBridge&
{
    m_image_bridge = &conn.ensure_image_bridge(visual_spec.info, m_pixel_format); // Throws
    return *m_image_bridge;
}


auto WindowImpl::create_graphics_context() noexcept -> GC
{
    ARCHON_ASSERT(m_gc == None);
    unsigned long valuemask = GCGraphicsExposures;
    XGCValues values = {};
    values.graphics_exposures = False;
    m_gc = XCreateGC(conn.dpy, m_drawable, valuemask, &values);
    return m_gc;
}


auto WindowImpl::intern_color(util::Color color) -> unsigned long
{
    return m_pixel_format.intern_color(color);
}


inline TextureImpl::TextureImpl(WindowImpl& win_2, const display::Size& size_2)
    : win(win_2)
    , size(size_2)
{
    if (ARCHON_LIKELY(size.is_valid()))
        return;
    throw std::invalid_argument("Invalid texture size");
}


TextureImpl::~TextureImpl() noexcept
{
    if (ARCHON_LIKELY(pixmap != None))
        XFreePixmap(win.conn.dpy, pixmap);
}


void TextureImpl::create()
{
    if (ARCHON_LIKELY(!size.is_empty()))
        pixmap = XCreatePixmap(win.conn.dpy, win.screen_slot.root, unsigned(size.width), unsigned(size.height),
                               win.visual_spec.info.depth);
}


void TextureImpl::put_image(const image::Image& img)
{
    x11::ImageBridge& bridge = win.ensure_image_bridge(); // Throws
    ARCHON_ASSERT(bridge.img_1.get_size().contains(impl::subdivide_max_subbox_size));
    image::Writer writer(bridge.img_1); // Throws
    image::Reader reader(img); // Throws
    GC gc = win.ensure_graphics_context();
    impl::subdivide(size, [&](const display::Box& subbox) {
        image::Pos pos = { 0, 0 };
        writer.put_image_a(pos, reader, subbox); // Throws
        int src_x = pos.x, src_y = pos.y;
        int dest_x = subbox.pos.x, dest_y = subbox.pos.y;
        unsigned width = unsigned(subbox.size.width);
        unsigned height = unsigned(subbox.size.height);
        XPutImage(win.conn.dpy, pixmap, gc, &bridge.img_2, src_x, src_y, dest_x, dest_y, width, height);
    }); // Throws
}



constexpr std::pair<KeySym, display::Key> key_assocs[] {
    // TTY functions
    { XK_BackSpace,             display::Key::backspace                   },
    { XK_Tab,                   display::Key::tab                         },
    { XK_Linefeed,              display::Key::line_feed                   },
    { XK_Clear,                 display::Key::clear                       },
    { XK_Return,                display::Key::return_                     },
    { XK_Pause,                 display::Key::pause                       },
    { XK_Scroll_Lock,           display::Key::scroll_lock                 },
    { XK_Sys_Req,               display::Key::sys_req                     },
    { XK_Escape,                display::Key::escape                      },
    { XK_Delete,                display::Key::delete_                     },

    // Cursor control
    { XK_Left,                  display::Key::left                        },
    { XK_Right,                 display::Key::right                       },
    { XK_Up,                    display::Key::up                          },
    { XK_Down,                  display::Key::down                        },
    { XK_Prior,                 display::Key::prior                       },
    { XK_Next,                  display::Key::next                        },
    { XK_Home,                  display::Key::home                        },
    { XK_Begin,                 display::Key::begin                       },
    { XK_End,                   display::Key::end                         },

    // Misc functions
    { XK_Select,                display::Key::select                      },
    { XK_Print,                 display::Key::print_screen                },
    { XK_Execute,               display::Key::execute                     },
    { XK_Insert,                display::Key::insert                      },
    { XK_Undo,                  display::Key::undo                        },
    { XK_Redo,                  display::Key::redo                        },
    { XK_Menu,                  display::Key::menu                        },
    { XK_Find,                  display::Key::find                        },
    { XK_Cancel,                display::Key::cancel                      },
    { XK_Help,                  display::Key::help                        },
    { XK_Break,                 display::Key::break_                      },
    { XK_Mode_switch,           display::Key::mode_switch                 },
    { XK_Num_Lock,              display::Key::num_lock                    },

    // Keypad
    { XK_KP_Add,                display::Key::keypad_add                  },
    { XK_KP_Subtract,           display::Key::keypad_subtract             },
    { XK_KP_Multiply,           display::Key::keypad_multiply             },
    { XK_KP_Divide,             display::Key::keypad_divide               },
    { XK_KP_Left,               display::Key::keypad_left                 },
    { XK_KP_Right,              display::Key::keypad_right                },
    { XK_KP_Up,                 display::Key::keypad_up                   },
    { XK_KP_Down,               display::Key::keypad_down                 },
    { XK_KP_Prior,              display::Key::keypad_prior                },
    { XK_KP_Next,               display::Key::keypad_next                 },
    { XK_KP_Home,               display::Key::keypad_home                 },
    { XK_KP_Begin,              display::Key::keypad_begin                },
    { XK_KP_End,                display::Key::keypad_end                  },
    { XK_KP_Insert,             display::Key::keypad_insert               },
    { XK_KP_Delete,             display::Key::keypad_delete               },
    { XK_KP_Enter,              display::Key::keypad_enter                },
    { XK_KP_0,                  display::Key::keypad_digit_0              },
    { XK_KP_1,                  display::Key::keypad_digit_1              },
    { XK_KP_2,                  display::Key::keypad_digit_2              },
    { XK_KP_3,                  display::Key::keypad_digit_3              },
    { XK_KP_4,                  display::Key::keypad_digit_4              },
    { XK_KP_5,                  display::Key::keypad_digit_5              },
    { XK_KP_6,                  display::Key::keypad_digit_6              },
    { XK_KP_7,                  display::Key::keypad_digit_7              },
    { XK_KP_8,                  display::Key::keypad_digit_8              },
    { XK_KP_9,                  display::Key::keypad_digit_9              },
    { XK_KP_Decimal,            display::Key::keypad_decimal_separator    },
    { XK_KP_Separator,          display::Key::keypad_thousands_separator  },
    { XK_KP_Equal,              display::Key::keypad_equal_sign           },
    { XK_KP_Space,              display::Key::keypad_space                },
    { XK_KP_Tab,                display::Key::keypad_tab                  },
    { XK_KP_F1,                 display::Key::keypad_f1                   },
    { XK_KP_F2,                 display::Key::keypad_f2                   },
    { XK_KP_F3,                 display::Key::keypad_f3                   },
    { XK_KP_F4,                 display::Key::keypad_f4                   },

    // Function keys
    { XK_F1,                    display::Key::f1                          },
    { XK_F2,                    display::Key::f2                          },
    { XK_F3,                    display::Key::f3                          },
    { XK_F4,                    display::Key::f4                          },
    { XK_F5,                    display::Key::f5                          },
    { XK_F6,                    display::Key::f6                          },
    { XK_F7,                    display::Key::f7                          },
    { XK_F8,                    display::Key::f8                          },
    { XK_F9,                    display::Key::f9                          },
    { XK_F10,                   display::Key::f10                         },
    { XK_F11,                   display::Key::f11                         },
    { XK_F12,                   display::Key::f12                         },
    { XK_F13,                   display::Key::f13                         },
    { XK_F14,                   display::Key::f14                         },
    { XK_F15,                   display::Key::f15                         },
    { XK_F16,                   display::Key::f16                         },
    { XK_F17,                   display::Key::f17                         },
    { XK_F18,                   display::Key::f18                         },
    { XK_F19,                   display::Key::f19                         },
    { XK_F20,                   display::Key::f20                         },
    { XK_F21,                   display::Key::f21                         },
    { XK_F22,                   display::Key::f22                         },
    { XK_F23,                   display::Key::f23                         },
    { XK_F24,                   display::Key::f24                         },
    { XK_F25,                   display::Key::f25                         },
    { XK_F26,                   display::Key::f26                         },
    { XK_F27,                   display::Key::f27                         },
    { XK_F28,                   display::Key::f28                         },
    { XK_F29,                   display::Key::f29                         },
    { XK_F30,                   display::Key::f30                         },
    { XK_F31,                   display::Key::f31                         },
    { XK_F32,                   display::Key::f32                         },
    { XK_F33,                   display::Key::f33                         },
    { XK_F34,                   display::Key::f34                         },
    { XK_F35,                   display::Key::f35                         },

    // Modifier keys
    { XK_Shift_L,               display::Key::shift_left                  },
    { XK_Shift_R,               display::Key::shift_right                 },
    { XK_Control_L,             display::Key::ctrl_left                   },
    { XK_Control_R,             display::Key::ctrl_right                  },
    { XK_Alt_L,                 display::Key::alt_left                    },
    { XK_Alt_R,                 display::Key::alt_right                   },
    { XK_Meta_L,                display::Key::meta_left                   },
    { XK_Meta_R,                display::Key::meta_right                  },
    { XK_Caps_Lock,             display::Key::caps_lock                   },
    { XK_Shift_Lock,            display::Key::shift_lock                  },
    { XK_dead_grave,            display::Key::dead_grave                  },
    { XK_dead_acute,            display::Key::dead_acute                  },
    { XK_dead_circumflex,       display::Key::dead_circumflex             },
    { XK_dead_tilde,            display::Key::dead_tilde                  },
    { XK_dead_macron,           display::Key::dead_macron                 },
    { XK_dead_breve,            display::Key::dead_breve                  },
    { XK_dead_abovedot,         display::Key::dead_abovedot               },
    { XK_dead_diaeresis,        display::Key::dead_diaeresis              },
    { XK_dead_abovering,        display::Key::dead_abovering              },
    { XK_dead_doubleacute,      display::Key::dead_doubleacute            },
    { XK_dead_caron,            display::Key::dead_caron                  },
    { XK_dead_cedilla,          display::Key::dead_cedilla                },
    { XK_dead_ogonek,           display::Key::dead_ogonek                 },
    { XK_dead_iota,             display::Key::dead_iota                   },
    { XK_dead_voiced_sound,     display::Key::dead_voiced_sound           },
    { XK_dead_semivoiced_sound, display::Key::dead_semivoiced_sound       },
    { XK_dead_belowdot,         display::Key::dead_belowdot               },
    { XK_dead_hook,             display::Key::dead_hook                   },
    { XK_dead_horn,             display::Key::dead_horn                   },
    { XK_dead_stroke,           display::Key::dead_stroke                 },
    { XK_dead_psili,            display::Key::dead_psili                  },
    { XK_dead_dasia,            display::Key::dead_dasia                  },
    { XK_dead_doublegrave,      display::Key::dead_doublegrave            },
    { XK_dead_belowring,        display::Key::dead_belowring              },
    { XK_dead_belowmacron,      display::Key::dead_belowmacron            },
    { XK_dead_belowcircumflex,  display::Key::dead_belowcircumflex        },
    { XK_dead_belowtilde,       display::Key::dead_belowtilde             },
    { XK_dead_belowbreve,       display::Key::dead_belowbreve             },
    { XK_dead_belowdiaeresis,   display::Key::dead_belowdiaeresis         },
    { XK_dead_invertedbreve,    display::Key::dead_invertedbreve          },
    { XK_dead_belowcomma,       display::Key::dead_belowcomma             },
    { XK_dead_currency,         display::Key::dead_currency               },

    // Basic Latin
    { XK_space,                 display::Key::space                       },
    { XK_exclam,                display::Key::exclamation_mark            },
    { XK_quotedbl,              display::Key::quotation_mark              },
    { XK_numbersign,            display::Key::number_sign                 },
    { XK_dollar,                display::Key::dollar_sign                 },
    { XK_percent,               display::Key::percent_sign                },
    { XK_ampersand,             display::Key::ampersand                   },
    { XK_apostrophe,            display::Key::apostrophe                  },
    { XK_parenleft,             display::Key::left_parenthesis            },
    { XK_parenright,            display::Key::right_parenthesis           },
    { XK_asterisk,              display::Key::asterisk                    },
    { XK_plus,                  display::Key::plus_sign                   },
    { XK_comma,                 display::Key::comma                       },
    { XK_minus,                 display::Key::hyphen_minus                },
    { XK_period,                display::Key::full_stop                   },
    { XK_slash,                 display::Key::solidus                     },
    { XK_0,                     display::Key::digit_0                     },
    { XK_1,                     display::Key::digit_1                     },
    { XK_2,                     display::Key::digit_2                     },
    { XK_3,                     display::Key::digit_3                     },
    { XK_4,                     display::Key::digit_4                     },
    { XK_5,                     display::Key::digit_5                     },
    { XK_6,                     display::Key::digit_6                     },
    { XK_7,                     display::Key::digit_7                     },
    { XK_8,                     display::Key::digit_8                     },
    { XK_9,                     display::Key::digit_9                     },
    { XK_colon,                 display::Key::colon                       },
    { XK_semicolon,             display::Key::semicolon                   },
    { XK_less,                  display::Key::less_than_sign              },
    { XK_equal,                 display::Key::equals_sign                 },
    { XK_greater,               display::Key::greater_than_sign           },
    { XK_question,              display::Key::question_mark               },
    { XK_at,                    display::Key::commercial_at               },
    { XK_A,                     display::Key::capital_a                   },
    { XK_B,                     display::Key::capital_b                   },
    { XK_C,                     display::Key::capital_c                   },
    { XK_D,                     display::Key::capital_d                   },
    { XK_E,                     display::Key::capital_e                   },
    { XK_F,                     display::Key::capital_f                   },
    { XK_G,                     display::Key::capital_g                   },
    { XK_H,                     display::Key::capital_h                   },
    { XK_I,                     display::Key::capital_i                   },
    { XK_J,                     display::Key::capital_j                   },
    { XK_K,                     display::Key::capital_k                   },
    { XK_L,                     display::Key::capital_l                   },
    { XK_M,                     display::Key::capital_m                   },
    { XK_N,                     display::Key::capital_n                   },
    { XK_O,                     display::Key::capital_o                   },
    { XK_P,                     display::Key::capital_p                   },
    { XK_Q,                     display::Key::capital_q                   },
    { XK_R,                     display::Key::capital_r                   },
    { XK_S,                     display::Key::capital_s                   },
    { XK_T,                     display::Key::capital_t                   },
    { XK_U,                     display::Key::capital_u                   },
    { XK_V,                     display::Key::capital_v                   },
    { XK_W,                     display::Key::capital_w                   },
    { XK_X,                     display::Key::capital_x                   },
    { XK_Y,                     display::Key::capital_y                   },
    { XK_Z,                     display::Key::capital_z                   },
    { XK_bracketleft,           display::Key::left_square_bracket         },
    { XK_backslash,             display::Key::reverse_solidus             },
    { XK_bracketright,          display::Key::right_square_bracket        },
    { XK_asciicircum,           display::Key::circumflex_accent           },
    { XK_underscore,            display::Key::low_line                    },
    { XK_grave,                 display::Key::grave_accent                },
    { XK_a,                     display::Key::small_a                     },
    { XK_b,                     display::Key::small_b                     },
    { XK_c,                     display::Key::small_c                     },
    { XK_d,                     display::Key::small_d                     },
    { XK_e,                     display::Key::small_e                     },
    { XK_f,                     display::Key::small_f                     },
    { XK_g,                     display::Key::small_g                     },
    { XK_h,                     display::Key::small_h                     },
    { XK_i,                     display::Key::small_i                     },
    { XK_j,                     display::Key::small_j                     },
    { XK_k,                     display::Key::small_k                     },
    { XK_l,                     display::Key::small_l                     },
    { XK_m,                     display::Key::small_m                     },
    { XK_n,                     display::Key::small_n                     },
    { XK_o,                     display::Key::small_o                     },
    { XK_p,                     display::Key::small_p                     },
    { XK_q,                     display::Key::small_q                     },
    { XK_r,                     display::Key::small_r                     },
    { XK_s,                     display::Key::small_s                     },
    { XK_t,                     display::Key::small_t                     },
    { XK_u,                     display::Key::small_u                     },
    { XK_v,                     display::Key::small_v                     },
    { XK_w,                     display::Key::small_w                     },
    { XK_x,                     display::Key::small_x                     },
    { XK_y,                     display::Key::small_y                     },
    { XK_z,                     display::Key::small_z                     },
    { XK_braceleft,             display::Key::left_curly_bracket          },
    { XK_bar,                   display::Key::vertical_line               },
    { XK_braceright,            display::Key::right_curly_bracket         },
    { XK_asciitilde,            display::Key::tilde                       },

    // Latin-1 Supplement
    { XK_nobreakspace,          display::Key::nobreak_space               },
    { XK_exclamdown,            display::Key::inverted_exclamation_mark   },
    { XK_cent,                  display::Key::cent_sign                   },
    { XK_sterling,              display::Key::pound_sign                  },
    { XK_currency,              display::Key::currency_sign               },
    { XK_yen,                   display::Key::yen_sign                    },
    { XK_brokenbar,             display::Key::broken_bar                  },
    { XK_section,               display::Key::section_sign                },
    { XK_diaeresis,             display::Key::diaeresis                   },
    { XK_copyright,             display::Key::copyright_sign              },
    { XK_ordfeminine,           display::Key::feminine_ordinal_indicator  },
    { XK_guillemotleft,         display::Key::left_guillemet              },
    { XK_notsign,               display::Key::not_sign                    },
    { XK_hyphen,                display::Key::soft_hyphen                 },
    { XK_registered,            display::Key::registered_sign             },
    { XK_macron,                display::Key::macron                      },
    { XK_degree,                display::Key::degree_sign                 },
    { XK_plusminus,             display::Key::plus_minus_sign             },
    { XK_twosuperior,           display::Key::superscript_two             },
    { XK_threesuperior,         display::Key::superscript_three           },
    { XK_acute,                 display::Key::acute_accent                },
    { XK_mu,                    display::Key::micro_sign                  },
    { XK_paragraph,             display::Key::pilcrow_sign                },
    { XK_periodcentered,        display::Key::middle_dot                  },
    { XK_cedilla,               display::Key::cedilla                     },
    { XK_onesuperior,           display::Key::superscript_one             },
    { XK_masculine,             display::Key::masculine_ordinal_indicator },
    { XK_guillemotright,        display::Key::right_guillemet             },
    { XK_onequarter,            display::Key::one_quarter                 },
    { XK_onehalf,               display::Key::one_half                    },
    { XK_threequarters,         display::Key::three_quarters              },
    { XK_questiondown,          display::Key::inverted_question_mark      },
    { XK_Agrave,                display::Key::capital_a_grave             },
    { XK_Aacute,                display::Key::capital_a_acute             },
    { XK_Acircumflex,           display::Key::capital_a_circumflex        },
    { XK_Atilde,                display::Key::capital_a_tilde             },
    { XK_Adiaeresis,            display::Key::capital_a_diaeresis         },
    { XK_Aring,                 display::Key::capital_a_ring              },
    { XK_AE,                    display::Key::capital_ae_ligature         },
    { XK_Ccedilla,              display::Key::capital_c_cedilla           },
    { XK_Egrave,                display::Key::capital_e_grave             },
    { XK_Eacute,                display::Key::capital_e_acute             },
    { XK_Ecircumflex,           display::Key::capital_e_circumflex        },
    { XK_Ediaeresis,            display::Key::capital_e_diaeresis         },
    { XK_Igrave,                display::Key::capital_i_grave             },
    { XK_Iacute,                display::Key::capital_i_acute             },
    { XK_Icircumflex,           display::Key::capital_i_circumflex        },
    { XK_Idiaeresis,            display::Key::capital_i_diaeresis         },
    { XK_ETH,                   display::Key::capital_eth                 },
    { XK_Ntilde,                display::Key::capital_n_tilde             },
    { XK_Ograve,                display::Key::capital_o_grave             },
    { XK_Oacute,                display::Key::capital_o_acute             },
    { XK_Ocircumflex,           display::Key::capital_o_circumflex        },
    { XK_Otilde,                display::Key::capital_o_tilde             },
    { XK_Odiaeresis,            display::Key::capital_o_diaeresis         },
    { XK_multiply,              display::Key::multiplication_sign         },
    { XK_Oslash,                display::Key::capital_o_stroke            },
    { XK_Ugrave,                display::Key::capital_u_grave             },
    { XK_Uacute,                display::Key::capital_u_acute             },
    { XK_Ucircumflex,           display::Key::capital_u_circumflex        },
    { XK_Udiaeresis,            display::Key::capital_u_diaeresis         },
    { XK_Yacute,                display::Key::capital_y_acute             },
    { XK_THORN,                 display::Key::capital_thorn               },
    { XK_ssharp,                display::Key::sharp_s                     },
    { XK_agrave,                display::Key::small_a_grave               },
    { XK_aacute,                display::Key::small_a_acute               },
    { XK_acircumflex,           display::Key::small_a_circumflex          },
    { XK_atilde,                display::Key::small_a_tilde               },
    { XK_adiaeresis,            display::Key::small_a_diaeresis           },
    { XK_aring,                 display::Key::small_a_ring                },
    { XK_ae,                    display::Key::small_ae_ligature           },
    { XK_ccedilla,              display::Key::small_c_cedilla             },
    { XK_egrave,                display::Key::small_e_grave               },
    { XK_eacute,                display::Key::small_e_acute               },
    { XK_ecircumflex,           display::Key::small_e_circumflex          },
    { XK_ediaeresis,            display::Key::small_e_diaeresis           },
    { XK_igrave,                display::Key::small_i_grave               },
    { XK_iacute,                display::Key::small_i_acute               },
    { XK_icircumflex,           display::Key::small_i_circumflex          },
    { XK_idiaeresis,            display::Key::small_i_diaeresis           },
    { XK_eth,                   display::Key::small_eth                   },
    { XK_ntilde,                display::Key::small_n_tilde               },
    { XK_ograve,                display::Key::small_o_grave               },
    { XK_oacute,                display::Key::small_o_acute               },
    { XK_ocircumflex,           display::Key::small_o_circumflex          },
    { XK_otilde,                display::Key::small_o_tilde               },
    { XK_odiaeresis,            display::Key::small_o_diaeresis           },
    { XK_division,              display::Key::division_sign               },
    { XK_oslash,                display::Key::small_o_stroke              },
    { XK_ugrave,                display::Key::small_u_grave               },
    { XK_uacute,                display::Key::small_u_acute               },
    { XK_ucircumflex,           display::Key::small_u_circumflex          },
    { XK_udiaeresis,            display::Key::small_u_diaeresis           },
    { XK_yacute,                display::Key::small_y_acute               },
    { XK_thorn,                 display::Key::small_thorn                 },
    { XK_ydiaeresis,            display::Key::small_y_diaeresis           },
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


class SlotImpl final
    : public display::Implementation::Slot {
public:
    auto get_ident() const noexcept -> std::string_view override;
    auto get_descr() const noexcept -> std::string_view override;
    auto get_implementation_a(const display::Guarantees&) const noexcept -> const display::Implementation* override;
};


auto SlotImpl::get_ident() const noexcept -> std::string_view
{
    return g_implementation_ident;
}


auto SlotImpl::get_descr() const noexcept -> std::string_view
{
    return g_implementation_descr;
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
