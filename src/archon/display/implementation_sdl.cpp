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


#include <cstddef>
#include <cstdint>
#include <memory>
#include <chrono>
#include <stdexcept>
#include <array>
#include <string_view>
#include <string>
#include <system_error>
#include <locale>
#include <mutex>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/flat_map.hpp>
#include <archon/core/literal_hash_map.hpp>
#include <archon/core/locale.hpp>
#include <archon/core/unicode_bridge.hpp>
#include <archon/core/format.hpp>
#include <archon/util/color.hpp>
#include <archon/image.hpp>
#include <archon/display/impl/config.h>
#include <archon/display/key.hpp>
#include <archon/display/key_code.hpp>
#include <archon/display/event.hpp>
#include <archon/display/event_handler.hpp>
#include <archon/display/noinst/timestamp_unwrapper.hpp>
#include <archon/display/guarantees.hpp>
#include <archon/display/window.hpp>
#include <archon/display/connection.hpp>
#include <archon/display/implementation.hpp>
#include <archon/display/implementation_sdl.hpp>

#if ARCHON_DISPLAY_HAVE_SDL
#  define HAVE_SDL 1
#else
#  define HAVE_SDL 0
#endif

#if HAVE_SDL
#  if ARCHON_CLANG
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wold-style-cast"
#  endif
#  define SDL_MAIN_HANDLED
#  include <SDL.h>
#  if ARCHON_CLANG
#    pragma clang diagnostic pop
#  endif
#endif


// Current minimum required SDL version is 2.0.22 for the following reasons:
//
// * Need SDL_HINT_QUIT_ON_LAST_WINDOW_CLOSE which was introduced in SDL 2.0.22.
//
// * Need proper and automatic mouse capturing behavior when mouse buttons are pressed
//   allowing for mouse move events while mouse is outside window. This was introduced in
//   SDL 2.0.22.
//


using namespace archon;
namespace impl = display::impl;


namespace {


constexpr std::string_view g_implementation_ident = "sdl";


#if HAVE_SDL


class WindowImpl;
class TextureImpl;


auto get_sdl_error(const std::locale& locale, std::string_view message) -> std::string
{
    return core::format(locale, "%s: %s", message, SDL_GetError()); // Throws
}


[[noreturn]] void throw_sdl_error(const std::locale& locale, std::string_view message)
{
    std::string msg = get_sdl_error(locale, message); // Throws
    throw std::runtime_error(std::move(msg));
}


void init_rect(SDL_Rect* rect, const display::Box& area) noexcept
{
    rect->x = area.pos.x;
    rect->y = area.pos.y;
    rect->w = area.size.width;
    rect->h = area.size.height;
}


bool map_key(display::KeyCode, display::Key&) noexcept;
bool rev_map_key(display::Key, display::KeyCode&) noexcept;
auto map_mouse_button(Uint8 button) noexcept -> display::MouseButton;


class ImplementationImpl final
    : public display::Implementation {
public:
    mutable std::mutex mutex;
    mutable bool have_connection = false;

    ImplementationImpl(Slot&) noexcept;

    auto new_connection(const std::locale&, const display::Connection::Config&) const ->
        std::unique_ptr<display::Connection> override;
    auto get_slot() const noexcept -> const Slot& override;

private:
    const Slot& m_slot;
};


class SlotImpl final
    : public display::Implementation::Slot {
public:
    SlotImpl() noexcept;

    auto ident() const noexcept -> std::string_view override;
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

    ConnectionImpl(const ImplementationImpl&, const std::locale&) noexcept;
    ~ConnectionImpl() noexcept override;

    void open();
    void register_window(Uint32 id, WindowImpl&);
    void unregister_window(Uint32 id) noexcept;

    bool try_map_key_to_key_code(display::Key, display::KeyCode&) const override;
    bool try_map_key_code_to_key(display::KeyCode, display::Key&) const override;
    bool try_get_key_name(display::KeyCode, std::string_view&) const override;
    auto new_window(std::string_view, display::Size, const display::Window::Config&) ->
        std::unique_ptr<display::Window> override;
    void process_events(display::ConnectionEventHandler*) override;
    bool process_events(time_point_type, display::ConnectionEventHandler*) override;
    int get_num_screens() const override;
    int get_default_screen() const override;
    bool try_get_screen_conf(int, core::Buffer<display::Viewport>&, core::Buffer<char>&,
                             std::size_t&, bool&) const override;
    auto get_implementation() const noexcept -> const display::Implementation& override;

private:
    using millis_type = std::chrono::milliseconds;

    bool m_was_opened = false;

    core::FlatMap<Uint32, WindowImpl&> m_windows;

    // SDL timestamps are 32-bit unsigned integers and `Uint32` refers to the unsigned
    // integer type that SDL uses to store these timestamps.
    using timestamp_unwrapper_type = impl::TimestampUnwrapper<Uint32, 32>;
    timestamp_unwrapper_type m_timestamp_unwrapper;

    // If `m_curr_window_id` is greater than zero, then `m_curr_window` specifies the window
    // identified by `m_curr_window_id` (valid window IDs are always greater than zero). If
    // `m_curr_window_id` is zero, `m_curr_window` has no meaning.
    //
    // If `m_curr_window_id` is greater than zero, but `m_curr_window` is null, it means
    // that the application has no knowledge of a window with that ID. This state is entered
    // if the window specified by `m_curr_window_id` is unregistered
    // (unregister_window()). The state is updated whenever a new window is registered
    // (register_window()). This takes care of the case where a new window reuses the ID
    // specified by `m_curr_window_id`.
    //
    Uint32 m_curr_window_id = 0;
    const WindowImpl* m_curr_window = nullptr;

    bool process_outstanding_events(display::ConnectionEventHandler&);
    void wait_for_events();
    bool wait_for_events(time_point_type deadline);

    bool lookup_window(Uint32 window_id, const WindowImpl*& window) noexcept;
};


class WindowImpl final
    : private display::WindowEventHandler
    , public display::Window {
public:
    ConnectionImpl& conn;
    const int cookie;
    display::WindowEventHandler* event_handler = this;

    WindowImpl(ConnectionImpl&, int cookie) noexcept;
    ~WindowImpl() noexcept override;

    void create(std::string_view title, display::Size size, const Config&);
    auto ensure_renderer() -> SDL_Renderer*;
    void set_draw_color(SDL_Renderer* renderer, util::Color color);

    void set_event_handler(display::WindowEventHandler&) noexcept override;
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
    bool m_have_minimum_size = false;
    display::Size m_minimum_size;
    SDL_Window* m_win = nullptr;
    Uint32 m_id = 0; // If nonzero, this window has been registered in the connection object
    SDL_Renderer* m_renderer = nullptr;
    SDL_GLContext m_gl_context = nullptr;

    auto create_renderer() -> SDL_Renderer*;
    void do_put_texture(const TextureImpl&, const display::Box& source_area, const display::Box& target_area);
};


class TextureImpl final
    : public display::Texture {
public:
    WindowImpl& win;
    const display::Size size;

    TextureImpl(WindowImpl&, const display::Size& size) noexcept;
    ~TextureImpl() noexcept override;

    void create();
    auto get() const noexcept -> SDL_Texture*;

    void put_image(const image::Image&) override;

private:
    SDL_Texture* m_tex = nullptr;
};



inline ImplementationImpl::ImplementationImpl(Slot& slot) noexcept
    : m_slot(slot)
{
}


auto ImplementationImpl::new_connection(const std::locale& locale, const display::Connection::Config&) const ->
    std::unique_ptr<display::Connection>
{
    auto conn = std::make_unique<ConnectionImpl>(*this, locale); // Throws
    conn->open(); // Throws
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
    bool is_available = (guarantees.no_other_use_of_sdl &&
                         guarantees.main_thread_exclusive &&
                         guarantees.only_one_connection);
    if (ARCHON_LIKELY(is_available))
        return &m_impl;
    return nullptr;
}



inline ConnectionImpl::ConnectionImpl(const ImplementationImpl& impl_2, const std::locale& locale_2) noexcept
    : impl(impl_2)
    , locale(locale_2)
{
}


ConnectionImpl::~ConnectionImpl() noexcept
{
    if (ARCHON_LIKELY(m_was_opened)) {
        SDL_Quit();
        std::lock_guard lock(impl.mutex);
        impl.have_connection = false;
    }
}


void ConnectionImpl::open()
{
    ARCHON_ASSERT(!m_was_opened);
    std::lock_guard lock(impl.mutex);
    if (impl.have_connection)
        throw std::runtime_error("Overlapping connections");
    SDL_SetMainReady();
    if (ARCHON_UNLIKELY(!SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1")))
        throw std::runtime_error("Failed to set SDL hint " SDL_HINT_NO_SIGNAL_HANDLERS);
    if (ARCHON_UNLIKELY(!SDL_SetHint(SDL_HINT_QUIT_ON_LAST_WINDOW_CLOSE, "0")))
        throw std::runtime_error("Failed to set SDL hint " SDL_HINT_QUIT_ON_LAST_WINDOW_CLOSE);
    Uint32 flags = SDL_INIT_VIDEO;
    int ret = SDL_Init(flags);
    if (ARCHON_LIKELY(ret >= 0)) {
        impl.have_connection = true;
        m_was_opened = true;
        return;
    }
    throw_sdl_error(locale, "SDL_Init() failed"); // Throws
}


inline void ConnectionImpl::register_window(Uint32 id, WindowImpl& window)
{
    ARCHON_ASSERT(id > 0);
    auto i = m_windows.emplace(id, window); // Throws
    bool was_inserted = i.second;
    ARCHON_ASSERT(was_inserted);
    // Because a new window might reuse the ID currently specified by `m_curr_window_id`, it
    // is necessary, and not just desirable to reset the "current window state" here.
    m_curr_window_id = id;
    m_curr_window = &window;
}


inline void ConnectionImpl::unregister_window(Uint32 id) noexcept
{
    ARCHON_ASSERT(id > 0);
    std::size_t n = m_windows.erase(id);
    ARCHON_ASSERT(n == 1);
    if (ARCHON_LIKELY(id == m_curr_window_id))
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
    // String returned by SDL_GetKeyName() is in UTF-8 encoding
    if (ARCHON_LIKELY(core::assume_utf8_locale(locale))) {
        SDL_Keycode code = core::cast_from_twos_compl_a<SDL_Keycode>(key_code.code);
        const char* name_2 = SDL_GetKeyName(code);
        if (ARCHON_LIKELY(name_2)) {
            name = std::string_view(name_2); // Throws
            return true;
        }
    }
    return false;
}


auto ConnectionImpl::new_window(std::string_view title, display::Size size,
                                const display::Window::Config& config) -> std::unique_ptr<display::Window>
{
    int screen = config.screen;
    if (ARCHON_UNLIKELY(screen >= 0 && screen != 0)) {
        throw std::invalid_argument("Bad screen index");
    }
    auto win = std::make_unique<WindowImpl>(*this, config.cookie); // Throws
    win->create(title, size, config); // Throws
    return win;
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
    display::ConnectionEventHandler& connection_event_handler_2 =
        (connection_event_handler ? *connection_event_handler : *this);
  again:
    if (ARCHON_LIKELY(process_outstanding_events(connection_event_handler_2))) { // Throws
        if (ARCHON_LIKELY(wait_for_events(deadline))) // Throws
            goto again;
        return true;
    }
    return false;
}


int ConnectionImpl::get_num_screens() const
{
    // On an X11 platform, SDL does not provide access to more than one screen at a time.
    return 1;
}


int ConnectionImpl::get_default_screen() const
{
    return 0;
}


bool ConnectionImpl::try_get_screen_conf(int screen, core::Buffer<display::Viewport>&, core::Buffer<char>&,
                                         std::size_t&, bool&) const
{
    if (ARCHON_UNLIKELY(screen != 0))
        throw std::invalid_argument("Bad screen index");
    return false;
}


auto ConnectionImpl::get_implementation() const noexcept -> const display::Implementation&
{
    return impl;
}


bool ConnectionImpl::process_outstanding_events(display::ConnectionEventHandler& connection_event_handler)
{
    SDL_Event event = {};
    const WindowImpl* window = {};
    timestamp_unwrapper_type::Session unwrap_session(m_timestamp_unwrapper);

  next_event_1:
    {
        int ret = SDL_PollEvent(&event);
        if (ARCHON_LIKELY(ret == 1))
            goto next_event_2;
        ARCHON_ASSERT(ret == 0);
    }
    goto exhausted;

  next_event_2:
    switch (event.type) {
        case SDL_MOUSEMOTION:
            if (ARCHON_LIKELY(event.motion.state == 0))
                break;
            if (ARCHON_LIKELY(lookup_window(event.motion.windowID, window))) {
                display::MouseButtonEvent event_2;
                event_2.cookie = window->cookie;
                event_2.timestamp = unwrap_session.unwrap_next_timestamp(event.motion.timestamp); // Throws
                event_2.pos = { event.motion.x, event.motion.y };
                bool proceed = window->event_handler->on_mousemove(event_2); // Throws
                if (ARCHON_LIKELY(proceed))
                    break;
                return false; // Interrupt
            }
            break;

        case SDL_MOUSEWHEEL:
            if (ARCHON_LIKELY(lookup_window(event.wheel.windowID, window))) {
                display::ScrollEvent event_2;
                event_2.cookie = window->cookie;
                event_2.timestamp = unwrap_session.unwrap_next_timestamp(event.wheel.timestamp); // Throws
                event_2.amount = { event.wheel.preciseX, event.wheel.preciseY };
                bool proceed = window->event_handler->on_scroll(event_2); // Throws
                if (ARCHON_LIKELY(proceed))
                    break;
                return false; // Interrupt
            }
            break;

        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            if (ARCHON_LIKELY(lookup_window(event.button.windowID, window))) {
                display::MouseButtonEvent event_2;
                event_2.cookie = window->cookie;
                event_2.timestamp = unwrap_session.unwrap_next_timestamp(event.button.timestamp); // Throws
                event_2.pos = { event.button.x, event.button.y };
                event_2.button = map_mouse_button(event.button.button);
                bool proceed;
                if (event.type == SDL_MOUSEBUTTONDOWN) {
                    proceed = window->event_handler->on_mousedown(event_2); // Throws
                }
                else {
                    proceed = window->event_handler->on_mouseup(event_2); // Throws
                }
                if (ARCHON_LIKELY(proceed))
                    break;
                return false; // Interrupt
            }
            break;

        case SDL_KEYDOWN:
        case SDL_KEYUP:
            // Some keys may remain pressed down when a window loses input focus, and some
            // keys may already be pressed down when a window gains input focus. With the
            // SDL-based display implementation (i.e., this implementation), synthetic "key
            // up" events are generated for keys that remain pressed down when a window
            // loses focus, and synthetic "key down" events are generated for keys that are
            // already pressed down when a window gains focus. This behavior deviates from
            // the plain X11 behavior, and is also inconsistent with the way mouse buttons
            // behave under the SDL-based implementation.
            //
            // Ideally, the behavior when using the SDL-based implementation should be
            // changed to match the behavior of X11, which is to only report "key down" and
            // "key up" events when the keys are actually pressed down or released (ignoring
            // auto-repeat here). Unfortunately, it does not appear to be possible to
            // configure SDL to behave this way, nor does it appear to be possible to
            // emulate the X11 behavior by somehow translating between them in the SDL-based
            // implementation (no way of knowing whether a "key up" event is synthetic or
            // genuine).
            //
            // FIXME: Consider proposing an SDL improvement in the form of a hint to disable synthesis of key events at focus and blur-time. The difficulty may lie in getting consistent behavior across all platforms supported by SDL.                       
            //
            // Alternatively, in the interest of alignment across implementations, it should
            // be considered whether the X11-based implementation (`implementation_x11.cpp`)
            // could be made to emulate the SDL-mandated behavior, i.e., with the generation
            // of synthetic "key up" and "key down" events when window loses or gains input
            // focus while keys are pressed down. The problem here, is that X11 key events
            // carry timestamps, but X11 focus and blur events do not, so there are no
            // timestamps to pass along for the synthetically generated key events.
            //
            // SDL evades the problem with the missing timestamps, because it generates all
            // event timestamps using a local client-side clock. The downside of doing that,
            // however, is that significant precision in the relative timing between
            // successive events can be lost. Such precision is important for some
            // applications. Using locally generated timestamps instead of those provided by
            // the X server is therefore deemed to not be a viable option for the X11-based
            // implementation. The loss of precision when using locally generated timestamps
            // is further aggravated by the tendency of events to be processed in batches on
            // the client-side, which means that timestamps will then also be obtained in
            // batches.
            //
            // Because of the issues described above, there seems to be no basis for picking
            // a particular behavior and requiring all display implementations to adhere to
            // that. Consequently, the API of the Archon Display Library (see
            // display::EventHandler::on_focus()) does not mandate a particular behavior for
            // pressed keys when windows gain or lose input focus. While this is
            // unfortunate, it allows for the unavoidable differences in behavior between
            // the SDL and X11-based implementations.
            //
            if (ARCHON_LIKELY(lookup_window(event.key.windowID, window))) {
                display::KeyEvent event_2;
                event_2.cookie = window->cookie;
                event_2.timestamp = unwrap_session.unwrap_next_timestamp(event.key.timestamp); // Throws
                event_2.key_code = { display::KeyCode::code_type(event.key.keysym.sym) };
                bool proceed;
                if (event.type == SDL_KEYDOWN) {
                    if (ARCHON_LIKELY(event.key.repeat == 0)) {
                        proceed = window->event_handler->on_keydown(event_2); // Throws
                    }
                    else {
                        proceed = window->event_handler->on_keyrepeat(event_2); // Throws
                    }
                }
                else {
                    proceed = window->event_handler->on_keyup(event_2); // Throws
                }
                if (ARCHON_LIKELY(proceed))
                    break;
                return false; // Interrupt
            }
            break;

        case SDL_WINDOWEVENT:
            if (ARCHON_LIKELY(lookup_window(event.window.windowID, window))) {
                switch (event.window.event) {
                    case SDL_WINDOWEVENT_SIZE_CHANGED: {
                        display::WindowSizeEvent event_2;
                        event_2.cookie = window->cookie;
                        core::int_cast(event.window.data1, event_2.size.width); // Throws
                        core::int_cast(event.window.data2, event_2.size.height); // Throws
                        bool proceed = window->event_handler->on_resize(event_2); // Throws
                        if (ARCHON_LIKELY(proceed))
                            break;
                        return false; // Interrupt
                    }
                    case SDL_WINDOWEVENT_MOVED: {
                        display::WindowPosEvent event_2;
                        event_2.cookie = window->cookie;
                        core::int_cast(event.window.data1, event_2.pos.x); // Throws
                        core::int_cast(event.window.data2, event_2.pos.y); // Throws
                        bool proceed = window->event_handler->on_reposition(event_2); // Throws
                        if (ARCHON_LIKELY(proceed))
                            break;
                        return false; // Interrupt
                    }
                    case SDL_WINDOWEVENT_EXPOSED: {
                        display::WindowEvent event_2;
                        event_2.cookie = window->cookie;
                        bool proceed = window->event_handler->on_expose(event_2); // Throws
                        if (ARCHON_LIKELY(proceed))
                            break;
                        return false; // Interrupt
                    }
                    case SDL_WINDOWEVENT_ENTER:
                    case SDL_WINDOWEVENT_LEAVE: {
                        display::TimedWindowEvent event_2;
                        event_2.cookie = window->cookie;
                        event_2.timestamp = unwrap_session.unwrap_next_timestamp(event.window.timestamp); // Throws
                        bool proceed;
                        if (event.window.event == SDL_WINDOWEVENT_ENTER) {
                            proceed = window->event_handler->on_mouseover(event_2); // Throws
                        }
                        else {
                            proceed = window->event_handler->on_mouseout(event_2); // Throws
                        }
                        if (ARCHON_LIKELY(proceed))
                            break;
                        return false; // Interrupt
                    }
                    case SDL_WINDOWEVENT_FOCUS_GAINED:
                    case SDL_WINDOWEVENT_FOCUS_LOST: {
                        display::WindowEvent event_2;
                        event_2.cookie = window->cookie;
                        bool proceed;
                        if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
                            proceed = window->event_handler->on_focus(event_2); // Throws
                        }
                        else {
                            proceed = window->event_handler->on_blur(event_2); // Throws
                        }
                        if (ARCHON_LIKELY(proceed))
                            break;
                        return false; // Interrupt
                    }
                    case SDL_WINDOWEVENT_CLOSE: {
                        display::WindowEvent event_2;
                        event_2.cookie = window->cookie;
                        bool proceed = window->event_handler->on_close(event_2); // Throws
                        if (ARCHON_LIKELY(!proceed))
                            return false; // Interrupt
                        break;
                    }
                }
            }
            break;
/*
        case SDL_DISPLAYEVENT: {
            // Fetch updated display information          
            // Call on_screen_change(screens) where screens is span of ScreenInfo objects
            // ScreenInfo has fields `name`, `bounds`, `resolution`, and `primary`
            // Question: How does SDL_GetDisplayUsableBounds() work with X11? --> See X11_GetDisplayUsableBounds()                                                                                                                                                         
            //
            // X11 driver of SDL: Look to X11_HandleXRandROutputChange()
            //
            int ret = SDL_GetNumVideoDisplays();
            if (ARCHON_LIKELY(ret >= 0)) {
                ARCHON_ASSERT(ret >= 1);
                int n = ret;
                for (int i = 0; i < n; ++i) {
                    const char* name = SDL_GetDisplayName(i);
                    if (ARCHON_UNLIKELY(!name))
                        throw_sdl_error(locale, "SDL_GetDisplayName() failed"); // Throws
                    SDL_Rect rect = {};
                    ret = SDL_GetDisplayBounds(i, &rect);
                    if (ARCHON_UNLIKELY(ret < 0))
                        throw_sdl_error(locale, "SDL_GetDisplayBounds() failed"); // Throws
                    ARCHON_ASSERT(ret == 0);
                    display::Box bounds = { display::Pos(rect.x, rect.y), display::Size(rect.w, rect.h) };
                    SDL_DisplayMode mode = {};
                    ret = SDL_GetDesktopDisplayMode(i, &mode);
                    if (ARCHON_UNLIKELY(ret < 0))
                        throw_sdl_error(locale, "SDL_GetDesktopDisplayMode() failed"); // Throws
                    ARCHON_ASSERT(ret == 0);
                    log::info("SCREEN[%s]: name = %s, bounds = %s, resolution = ?, frame_rate = %s, ", i + 1, name, bounds, mode.refresh_rate);              
                }
                break;
            }
            throw_sdl_error(locale, "SDL_GetNumVideoDisplays() failed"); // Throws
            break;
        }
*/
        case SDL_QUIT: {
            bool proceed = connection_event_handler.on_quit(); // Throws
            if (ARCHON_LIKELY(!proceed))
                return false; // Interrupt
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
    return true; // Wait for more events to occur
}


void ConnectionImpl::wait_for_events()
{
    SDL_Event* event = nullptr;
    int ret = SDL_WaitEvent(event);
    if (ARCHON_LIKELY(ret == 1))
        return;
    ARCHON_ASSERT(ret == 0);
    throw_sdl_error(locale, "SDL_WaitEvent() failed"); // Throws
}


bool ConnectionImpl::wait_for_events(time_point_type deadline)
{
    time_point_type now;
  again:
    now = clock_type::now();
    if (ARCHON_LIKELY(deadline > now)) {
        int timeout = core::int_max<int>();
        bool complete = false;
        auto duration = std::chrono::ceil<std::chrono::milliseconds>(deadline - now).count();
        if (ARCHON_LIKELY(core::int_less_equal(duration, timeout))) {
            timeout = int(duration);
            complete = true;
        }
        // FIXME: There is something broken about the design of
        // SDL_WaitEventTimeout(). According to the documentation, when that function
        // returns zero, it means that an error occurred or the timeout was reached, But,
        // unfortunately, there is no way to tell which of the two happened. The only viable
        // resolution seems to be to assume that the function can never fail, and that zero
        // always means that the timeout was reached. Calling SDL_WaitEventTimeout() to see
        // if an error occurred is not an option, as it will sometimes report errors when
        // none occurred even if SDL_ClearError() is called before calling
        // SDL_WaitEventTimeout().
        //
        // See also https://discourse.libsdl.org/t/proposal-for-sdl-3-return-value-improvement-for-sdl-waiteventtimeout/45743
        //
        SDL_Event* event = nullptr;
        int ret = SDL_WaitEventTimeout(event, timeout);
        if (ARCHON_LIKELY(ret == 1))
            return true;
        ARCHON_ASSERT(ret == 0);
        if (ARCHON_LIKELY(complete))
            goto expired;
        goto again;
    }
  expired:
    return false;
}


bool ConnectionImpl::lookup_window(Uint32 window_id, const WindowImpl*& window) noexcept
{
    const WindowImpl* window_2 = nullptr;
    if (ARCHON_LIKELY(window_id == m_curr_window_id)) {
        window_2 = m_curr_window;
    }
    else {
        auto i = m_windows.find(window_id);
        if (ARCHON_LIKELY(i != m_windows.end()))
            window_2 = &i->second;
        m_curr_window_id = window_id;
        m_curr_window = window_2;
    }
    if (ARCHON_LIKELY(window_2)) {
        window = window_2;
        return true;
    }
    return false;
}



inline WindowImpl::WindowImpl(ConnectionImpl& conn_2, int cookie_2) noexcept
    : conn(conn_2)
    , cookie(cookie_2)
{
}


WindowImpl::~WindowImpl() noexcept
{
    if (ARCHON_LIKELY(m_win)) {
        if (m_id > 0)
            conn.unregister_window(m_id);
        if (m_renderer)
            SDL_DestroyRenderer(m_renderer);
        if (m_gl_context)
            SDL_GL_DeleteContext(m_gl_context);
        SDL_DestroyWindow(m_win);
    }
}


void WindowImpl::create(std::string_view title, display::Size size, const Config& config)
{
    if (config.resizable && config.minimum_size.has_value()) {
        m_have_minimum_size = true;
        m_minimum_size = config.minimum_size.value();
    }

    display::Size adjusted_size = size;
    if (m_have_minimum_size)
        adjusted_size = max(adjusted_size, m_minimum_size);

    std::array<char, 128> seed_memory;
    core::Buffer buffer(seed_memory);
    {
        core::native_mb_to_utf8_transcoder transcoder(conn.locale);
        std::size_t buffer_offset = 0;
        transcoder.transcode_l(title, buffer, buffer_offset); // Throws
        buffer.append_a('\0', buffer_offset); // Throws
    }
    const char* title_2 = buffer.data();

    int x = SDL_WINDOWPOS_UNDEFINED;
    int y = SDL_WINDOWPOS_UNDEFINED;
    int w = adjusted_size.width;
    int h = adjusted_size.height;
    Uint32 flags = SDL_WINDOW_HIDDEN;
    if (config.resizable)
        flags |= SDL_WINDOW_RESIZABLE;
    if (config.fullscreen)
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    if (config.enable_opengl_rendering)
        flags |= SDL_WINDOW_OPENGL;
    SDL_Window* win = SDL_CreateWindow(title_2, x, y, w, h, flags);
    if (ARCHON_UNLIKELY(!win))
        throw_sdl_error(conn.locale, "SDL_CreateWindow() failed"); // Throws
    m_win = win;
    Uint32 id = SDL_GetWindowID(m_win);
    if (ARCHON_UNLIKELY(id <= 0))
        throw_sdl_error(conn.locale, "SDL_GetWindowID() failed"); // Throws
    conn.register_window(id, *this); // Throws
    m_id = id;

    // With the X11 back end, and when OpenGL support is not explicitly requested, the
    // window will be recreated when a renderer is created. Presumably, this is because a
    // renderer requires OpenGL support, but when OpenGL support is not requested initially,
    // a visual without OpenGL support is selected initially. Unfortunately, this leads to a
    // very visible flicker / artifact if the recreation occurs while the window is
    // visible. To work around this problem, we request the creation of the renderer before
    // the window is made visible when OpenGL support is not explicitly requested.
    if (!config.enable_opengl_rendering)
        ensure_renderer(); // Throws

    // Set minimum window size if requested
    if (m_have_minimum_size)
        SDL_SetWindowMinimumSize(m_win, m_minimum_size.width, m_minimum_size.height);
}


inline auto WindowImpl::ensure_renderer() -> SDL_Renderer*
{
    if (ARCHON_LIKELY(m_renderer))
        return m_renderer;
    return create_renderer(); // Throws
}


void WindowImpl::set_draw_color(SDL_Renderer* renderer, util::Color color)
{
    int ret = SDL_SetRenderDrawColor(renderer, color.red(), color.green(), color.blue(), color.alpha());
    if (ARCHON_LIKELY(ret >= 0)) {
        ARCHON_ASSERT(ret == 0);
        return;
    }
    throw_sdl_error(conn.locale, "SDL_SetRenderDrawColor() failed"); // Throws
}


void WindowImpl::set_event_handler(display::WindowEventHandler& handler) noexcept
{
    event_handler = &handler;
}


void WindowImpl::show()
{
    SDL_ShowWindow(m_win);
}


void WindowImpl::hide()
{
    SDL_HideWindow(m_win);
}


void WindowImpl::set_title(std::string_view title)
{
    std::array<char, 128> seed_memory;
    core::Buffer buffer(seed_memory);
    {
        core::native_mb_to_utf8_transcoder transcoder(conn.locale);
        std::size_t buffer_offset = 0;
        transcoder.transcode_l(title, buffer, buffer_offset); // Throws
        buffer.append_a('\0', buffer_offset); // Throws
    }
    const char* title_2 = buffer.data();
    SDL_SetWindowTitle(m_win, title_2);
}


void WindowImpl::set_size(display::Size size)
{
    SDL_SetWindowSize(m_win, size.width, size.height);
}


void WindowImpl::set_fullscreen_mode(bool on)
{
    int ret = SDL_SetWindowFullscreen(m_win, (on ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0));
    if (ARCHON_LIKELY(ret >= 0)) {
        ARCHON_ASSERT(ret == 0);
        return;
    }
    throw_sdl_error(conn.locale, "SDL_SetWindowFullscreen() failed"); // Throws
}


void WindowImpl::fill(util::Color color)
{
    SDL_Renderer* renderer = ensure_renderer(); // Throws
    set_draw_color(renderer, color); // Throws
    int ret = SDL_RenderClear(renderer);
    if (ARCHON_LIKELY(ret >= 0)) {
        ARCHON_ASSERT(ret == 0);
        return;
    }
    throw_sdl_error(conn.locale, "SDL_RenderClear() failed"); // Throws
}


void WindowImpl::fill(util::Color color, const display::Box& area)
{
    SDL_Renderer* renderer = ensure_renderer(); // Throws
    set_draw_color(renderer, color); // Throws
    SDL_Rect rect;
    init_rect(&rect, area);
    int ret = SDL_RenderFillRect(renderer, &rect);
    if (ARCHON_LIKELY(ret >= 0)) {
        ARCHON_ASSERT(ret == 0);
        return;
    }
    throw_sdl_error(conn.locale, "SDL_RenderFillRect() failed"); // Throws
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
    do_put_texture(tex_2, tex_2.size, { pos, tex_2.size }); // Throws
}


void WindowImpl::put_texture(const display::Texture& tex, const display::Box& source_area, const display::Pos& pos)
{
    const TextureImpl& tex_2 = dynamic_cast<const TextureImpl&>(tex); // Throws
    do_put_texture(tex_2, source_area, { pos, source_area.size }); // Throws
}


void WindowImpl::present()
{
    SDL_Renderer* renderer = ensure_renderer(); // Throws
    SDL_RenderPresent(renderer);
}


void WindowImpl::opengl_make_current()
{
    if (ARCHON_LIKELY(m_gl_context)) {
        int ret = SDL_GL_MakeCurrent(m_win, m_gl_context);
        if (ARCHON_LIKELY(ret == 0))
            return;
        throw_sdl_error(conn.locale, "SDL_GL_MakeCurrent() failed"); // Throws
    }
    SDL_GLContext ret = SDL_GL_CreateContext(m_win);
    if (ARCHON_LIKELY(ret)) {
        m_gl_context = ret;
        return;
    }
    throw_sdl_error(conn.locale, "SDL_GL_CreateContext() failed"); // Throws
}


void WindowImpl::opengl_swap_buffers()
{
    SDL_GL_SwapWindow(m_win);
}


auto WindowImpl::create_renderer() -> SDL_Renderer*
{
    ARCHON_ASSERT(!m_renderer);
    int driver_index = -1;
    Uint32 flags = 0;
    SDL_Renderer* renderer = SDL_CreateRenderer(m_win, driver_index, flags);
    if (ARCHON_UNLIKELY(!renderer))
        throw_sdl_error(conn.locale, "SDL_CreateRenderer() failed"); // Throws

    m_renderer = renderer;

    // Due to a bug in SDL (https://github.com/libsdl-org/SDL/issues/8805), the setting of
    // the minimum window size has to be repeated after the creation of the renderer.
    if (m_have_minimum_size)
        SDL_SetWindowMinimumSize(m_win, m_minimum_size.width, m_minimum_size.height);

    return renderer;
}


void WindowImpl::do_put_texture(const TextureImpl& tex, const display::Box& source_area,
                                const display::Box& target_area)
{
    ARCHON_ASSERT(&tex.win.conn.impl == &conn.impl);
    ARCHON_ASSERT(m_renderer);
    SDL_Rect src_rect, dst_rect;
    init_rect(&src_rect, source_area);
    init_rect(&dst_rect, target_area);
    int ret = SDL_RenderCopy(m_renderer, tex.get(), &src_rect, &dst_rect);
    if (ARCHON_LIKELY(ret >= 0)) {
        ARCHON_ASSERT(ret == 0);
        return;
    }
    throw_sdl_error(conn.locale, "SDL_RenderCopy() failed"); // Throws
}



inline TextureImpl::TextureImpl(WindowImpl& win_2, const display::Size& size_2) noexcept
    : win(win_2)
    , size(size_2)
{
}


TextureImpl::~TextureImpl() noexcept
{
    if (m_tex)
        SDL_DestroyTexture(m_tex);
}


void TextureImpl::create()
{
    SDL_Renderer* renderer = win.ensure_renderer(); // Throws
    Uint32 format = SDL_PIXELFORMAT_ARGB32;    
    int access = SDL_TEXTUREACCESS_STATIC;    
    SDL_Texture* tex = SDL_CreateTexture(renderer, format, access, size.width, size.height);
    if (ARCHON_LIKELY(tex)) {
        m_tex = tex;
        return;
    }
    throw_sdl_error(win.conn.locale, "SDL_CreateTexture() failed"); // Throws
}


inline auto TextureImpl::get() const noexcept -> SDL_Texture*
{
    ARCHON_ASSERT(m_tex);
    return m_tex;
}


void TextureImpl::put_image(const image::Image& img)
{
/*
Choose preferred pixel format for texture:
If list contains ARGB8888, use that
Otherwise, if list contains any of ARGB8888, ABGR8888, RGBA8888, BGRA8888, use the one of those that occurs first in the list
If list contains RGB888, use that
*/

    // Allocate pixel buffer using same format as texture (up to 4096 pixels ~ 16KiB buffer)
    // For each subsection:
    //   Copy subsection from image to buffer
    //   Invoke SDL_UpdateTexture() for subsection

    // Assumption: The pixel format expected by SDL_UpdateTexture() is always exactly the
    // pixel format passed to SDL_CreateTexture()
    using ARGB32 = image::IntegerPixelFormat<image::ChannelSpec_RGBA, image::int8_type, 8, image::int8_type, 1, core::Endianness::big, true, false>;    
    image::BufferedImage<ARGB32> img_2(size); // Throws
    image::Writer writer(img_2); // Throws
    image::Reader reader(img); // Throws
    writer.put_image_a({ 0, 0 }, reader, size); // Throws

    const SDL_Rect* rect = nullptr;
    // FIXME: Risk of overflow goes away with subdivision and use of fixed size "image bridge"      
    int pitch = size.width;
    core::int_mul(pitch, 4); // Throws
    int ret = SDL_UpdateTexture(m_tex, rect, img_2.get_buffer().data(), pitch);
    if (ARCHON_LIKELY(ret >= 0)) {
        ARCHON_ASSERT(ret == 0);
        return;
    }
    throw_sdl_error(win.conn.locale, "SDL_UpdateTexture() failed"); // Throws
}



constexpr std::pair<SDL_Keycode, display::Key> key_assocs[] {
    // TTY functions
    { SDLK_BACKSPACE,    display::Key::backspace                   },
    { SDLK_TAB,          display::Key::tab                         },
    { SDLK_CLEAR,        display::Key::clear                       },
    { SDLK_RETURN,       display::Key::return_                     },
    { SDLK_PAUSE,        display::Key::pause                       },
    { SDLK_SCROLLLOCK,   display::Key::scroll_lock                 },
    { SDLK_SYSREQ,       display::Key::sys_req                     },
    { SDLK_ESCAPE,       display::Key::escape                      },
    { SDLK_DELETE,       display::Key::delete_                     },

    // Cursor control
    { SDLK_LEFT,         display::Key::left                        },
    { SDLK_RIGHT,        display::Key::right                       },
    { SDLK_UP,           display::Key::up                          },
    { SDLK_DOWN,         display::Key::down                        },
    { SDLK_PAGEUP,       display::Key::prior                       },
    { SDLK_PAGEDOWN,     display::Key::next                        },
    { SDLK_HOME,         display::Key::home                        },
    { SDLK_END,          display::Key::end                         },

    // Misc functions
    { SDLK_SELECT,       display::Key::select                      },
    { SDLK_PRINTSCREEN,  display::Key::print_screen                },
    { SDLK_EXECUTE,      display::Key::execute                     },
    { SDLK_INSERT,       display::Key::insert                      },
    { SDLK_UNDO,         display::Key::undo                        },
    { SDLK_MENU,         display::Key::menu                        },
    { SDLK_FIND,         display::Key::find                        },
    { SDLK_CANCEL,       display::Key::cancel                      },
    { SDLK_HELP,         display::Key::help                        },
    { SDLK_MODE,         display::Key::mode_switch                 },
    { SDLK_NUMLOCKCLEAR, display::Key::num_lock                    },

    // Keypad
    { SDLK_KP_PLUS,      display::Key::keypad_add                  },
    { SDLK_KP_MINUS,     display::Key::keypad_subtract             },
    { SDLK_KP_MULTIPLY,  display::Key::keypad_multiply             },
    { SDLK_KP_DIVIDE,    display::Key::keypad_divide               },
    { SDLK_KP_ENTER,     display::Key::keypad_enter                },
    // Weirdly, SDL uses the numerical symbols to identify these keys
    { SDLK_KP_0,         display::Key::keypad_insert               },
    { SDLK_KP_1,         display::Key::keypad_end                  },
    { SDLK_KP_2,         display::Key::keypad_down                 },
    { SDLK_KP_3,         display::Key::keypad_next                 },
    { SDLK_KP_4,         display::Key::keypad_left                 },
    { SDLK_KP_5,         display::Key::keypad_begin                },
    { SDLK_KP_6,         display::Key::keypad_right                },
    { SDLK_KP_7,         display::Key::keypad_home                 },
    { SDLK_KP_8,         display::Key::keypad_up                   },
    { SDLK_KP_9,         display::Key::keypad_prior                },
    { SDLK_KP_PERIOD,    display::Key::keypad_delete               },
    { SDLK_KP_COMMA,     display::Key::keypad_thousands_separator  },
    { SDLK_KP_EQUALS,    display::Key::keypad_equal_sign           },
    { SDLK_KP_SPACE,     display::Key::keypad_space                },
    { SDLK_KP_TAB,       display::Key::keypad_tab                  },

    // Function keys
    { SDLK_F1,           display::Key::f1                          },
    { SDLK_F2,           display::Key::f2                          },
    { SDLK_F3,           display::Key::f3                          },
    { SDLK_F4,           display::Key::f4                          },
    { SDLK_F5,           display::Key::f5                          },
    { SDLK_F6,           display::Key::f6                          },
    { SDLK_F7,           display::Key::f7                          },
    { SDLK_F8,           display::Key::f8                          },
    { SDLK_F9,           display::Key::f9                          },
    { SDLK_F10,          display::Key::f10                         },
    { SDLK_F11,          display::Key::f11                         },
    { SDLK_F12,          display::Key::f12                         },
    { SDLK_F13,          display::Key::f13                         },
    { SDLK_F14,          display::Key::f14                         },
    { SDLK_F15,          display::Key::f15                         },
    { SDLK_F16,          display::Key::f16                         },
    { SDLK_F17,          display::Key::f17                         },
    { SDLK_F18,          display::Key::f18                         },
    { SDLK_F19,          display::Key::f19                         },
    { SDLK_F20,          display::Key::f20                         },
    { SDLK_F21,          display::Key::f21                         },
    { SDLK_F22,          display::Key::f22                         },
    { SDLK_F23,          display::Key::f23                         },
    { SDLK_F24,          display::Key::f24                         },

    // Modifier keys
    { SDLK_LSHIFT,       display::Key::shift_left                  },
    { SDLK_RSHIFT,       display::Key::shift_right                 },
    { SDLK_LCTRL,        display::Key::ctrl_left                   },
    { SDLK_RCTRL,        display::Key::ctrl_right                  },
    { SDLK_LALT,         display::Key::alt_left                    },
    { SDLK_RALT,         display::Key::alt_right                   },
    { SDLK_LGUI,         display::Key::meta_left                   },
    { SDLK_RGUI,         display::Key::meta_right                  },
    { SDLK_CAPSLOCK,     display::Key::caps_lock                   },
    // Strangely, SDL lacks the key codes for the dead accent keys (grave, acute,
    // circumflex, tilde, diaresis, ...)

    // Basic Latin
    {  32,               display::Key::space                       },
    {  33,               display::Key::exclamation_mark            },
    {  34,               display::Key::quotation_mark              },
    {  35,               display::Key::number_sign                 },
    {  36,               display::Key::dollar_sign                 },
    {  37,               display::Key::percent_sign                },
    {  38,               display::Key::ampersand                   },
    {  39,               display::Key::apostrophe                  },
    {  40,               display::Key::left_parenthesis            },
    {  41,               display::Key::right_parenthesis           },
    {  42,               display::Key::asterisk                    },
    {  43,               display::Key::plus_sign                   },
    {  44,               display::Key::comma                       },
    {  45,               display::Key::hyphen_minus                },
    {  46,               display::Key::full_stop                   },
    {  47,               display::Key::solidus                     },
    {  48,               display::Key::digit_0                     },
    {  49,               display::Key::digit_1                     },
    {  50,               display::Key::digit_2                     },
    {  51,               display::Key::digit_3                     },
    {  52,               display::Key::digit_4                     },
    {  53,               display::Key::digit_5                     },
    {  54,               display::Key::digit_6                     },
    {  55,               display::Key::digit_7                     },
    {  56,               display::Key::digit_8                     },
    {  57,               display::Key::digit_9                     },
    {  58,               display::Key::colon                       },
    {  59,               display::Key::semicolon                   },
    {  60,               display::Key::less_than_sign              },
    {  61,               display::Key::equals_sign                 },
    {  62,               display::Key::greater_than_sign           },
    {  63,               display::Key::question_mark               },
    {  64,               display::Key::commercial_at               },
    {  65,               display::Key::capital_a                   },
    {  66,               display::Key::capital_b                   },
    {  67,               display::Key::capital_c                   },
    {  68,               display::Key::capital_d                   },
    {  69,               display::Key::capital_e                   },
    {  70,               display::Key::capital_f                   },
    {  71,               display::Key::capital_g                   },
    {  72,               display::Key::capital_h                   },
    {  73,               display::Key::capital_i                   },
    {  74,               display::Key::capital_j                   },
    {  75,               display::Key::capital_k                   },
    {  76,               display::Key::capital_l                   },
    {  77,               display::Key::capital_m                   },
    {  78,               display::Key::capital_n                   },
    {  79,               display::Key::capital_o                   },
    {  80,               display::Key::capital_p                   },
    {  81,               display::Key::capital_q                   },
    {  82,               display::Key::capital_r                   },
    {  83,               display::Key::capital_s                   },
    {  84,               display::Key::capital_t                   },
    {  85,               display::Key::capital_u                   },
    {  86,               display::Key::capital_v                   },
    {  87,               display::Key::capital_w                   },
    {  88,               display::Key::capital_x                   },
    {  89,               display::Key::capital_y                   },
    {  90,               display::Key::capital_z                   },
    {  91,               display::Key::left_square_bracket         },
    {  92,               display::Key::reverse_solidus             },
    {  93,               display::Key::right_square_bracket        },
    {  94,               display::Key::circumflex_accent           },
    {  95,               display::Key::low_line                    },
    {  96,               display::Key::grave_accent                },
    {  97,               display::Key::small_a                     },
    {  98,               display::Key::small_b                     },
    {  99,               display::Key::small_c                     },
    { 100,               display::Key::small_d                     },
    { 101,               display::Key::small_e                     },
    { 102,               display::Key::small_f                     },
    { 103,               display::Key::small_g                     },
    { 104,               display::Key::small_h                     },
    { 105,               display::Key::small_i                     },
    { 106,               display::Key::small_j                     },
    { 107,               display::Key::small_k                     },
    { 108,               display::Key::small_l                     },
    { 109,               display::Key::small_m                     },
    { 110,               display::Key::small_n                     },
    { 111,               display::Key::small_o                     },
    { 112,               display::Key::small_p                     },
    { 113,               display::Key::small_q                     },
    { 114,               display::Key::small_r                     },
    { 115,               display::Key::small_s                     },
    { 116,               display::Key::small_t                     },
    { 117,               display::Key::small_u                     },
    { 118,               display::Key::small_v                     },
    { 119,               display::Key::small_w                     },
    { 120,               display::Key::small_x                     },
    { 121,               display::Key::small_y                     },
    { 122,               display::Key::small_z                     },
    { 123,               display::Key::left_curly_bracket          },
    { 124,               display::Key::vertical_line               },
    { 125,               display::Key::right_curly_bracket         },
    { 126,               display::Key::tilde                       },

    // Latin-1 Supplement
    { 160,               display::Key::nobreak_space               },
    { 161,               display::Key::inverted_exclamation_mark   },
    { 162,               display::Key::cent_sign                   },
    { 163,               display::Key::pound_sign                  },
    { 164,               display::Key::currency_sign               },
    { 165,               display::Key::yen_sign                    },
    { 166,               display::Key::broken_bar                  },
    { 167,               display::Key::section_sign                },
    { 168,               display::Key::diaeresis                   },
    { 169,               display::Key::copyright_sign              },
    { 170,               display::Key::feminine_ordinal_indicator  },
    { 171,               display::Key::left_guillemet              },
    { 172,               display::Key::not_sign                    },
    { 173,               display::Key::soft_hyphen                 },
    { 174,               display::Key::registered_sign             },
    { 175,               display::Key::macron                      },
    { 176,               display::Key::degree_sign                 },
    { 177,               display::Key::plus_minus_sign             },
    { 178,               display::Key::superscript_two             },
    { 179,               display::Key::superscript_three           },
    { 180,               display::Key::acute_accent                },
    { 181,               display::Key::micro_sign                  },
    { 182,               display::Key::pilcrow_sign                },
    { 183,               display::Key::middle_dot                  },
    { 184,               display::Key::cedilla                     },
    { 185,               display::Key::superscript_one             },
    { 186,               display::Key::masculine_ordinal_indicator },
    { 187,               display::Key::right_guillemet             },
    { 188,               display::Key::one_quarter                 },
    { 189,               display::Key::one_half                    },
    { 190,               display::Key::three_quarters              },
    { 191,               display::Key::inverted_question_mark      },
    { 192,               display::Key::capital_a_grave             },
    { 193,               display::Key::capital_a_acute             },
    { 194,               display::Key::capital_a_circumflex        },
    { 195,               display::Key::capital_a_tilde             },
    { 196,               display::Key::capital_a_diaeresis         },
    { 197,               display::Key::capital_a_ring              },
    { 198,               display::Key::capital_ae_ligature         },
    { 199,               display::Key::capital_c_cedilla           },
    { 200,               display::Key::capital_e_grave             },
    { 201,               display::Key::capital_e_acute             },
    { 202,               display::Key::capital_e_circumflex        },
    { 203,               display::Key::capital_e_diaeresis         },
    { 204,               display::Key::capital_i_grave             },
    { 205,               display::Key::capital_i_acute             },
    { 206,               display::Key::capital_i_circumflex        },
    { 207,               display::Key::capital_i_diaeresis         },
    { 208,               display::Key::capital_eth                 },
    { 209,               display::Key::capital_n_tilde             },
    { 210,               display::Key::capital_o_grave             },
    { 211,               display::Key::capital_o_acute             },
    { 212,               display::Key::capital_o_circumflex        },
    { 213,               display::Key::capital_o_tilde             },
    { 214,               display::Key::capital_o_diaeresis         },
    { 215,               display::Key::multiplication_sign         },
    { 216,               display::Key::capital_o_stroke            },
    { 217,               display::Key::capital_u_grave             },
    { 218,               display::Key::capital_u_acute             },
    { 219,               display::Key::capital_u_circumflex        },
    { 220,               display::Key::capital_u_diaeresis         },
    { 221,               display::Key::capital_y_acute             },
    { 222,               display::Key::capital_thorn               },
    { 223,               display::Key::sharp_s                     },
    { 224,               display::Key::small_a_grave               },
    { 225,               display::Key::small_a_acute               },
    { 226,               display::Key::small_a_circumflex          },
    { 227,               display::Key::small_a_tilde               },
    { 228,               display::Key::small_a_diaeresis           },
    { 229,               display::Key::small_a_ring                },
    { 230,               display::Key::small_ae_ligature           },
    { 231,               display::Key::small_c_cedilla             },
    { 232,               display::Key::small_e_grave               },
    { 233,               display::Key::small_e_acute               },
    { 234,               display::Key::small_e_circumflex          },
    { 235,               display::Key::small_e_diaeresis           },
    { 236,               display::Key::small_i_grave               },
    { 237,               display::Key::small_i_acute               },
    { 238,               display::Key::small_i_circumflex          },
    { 239,               display::Key::small_i_diaeresis           },
    { 240,               display::Key::small_eth                   },
    { 241,               display::Key::small_n_tilde               },
    { 242,               display::Key::small_o_grave               },
    { 243,               display::Key::small_o_acute               },
    { 244,               display::Key::small_o_circumflex          },
    { 245,               display::Key::small_o_tilde               },
    { 246,               display::Key::small_o_diaeresis           },
    { 247,               display::Key::division_sign               },
    { 248,               display::Key::small_o_stroke              },
    { 249,               display::Key::small_u_grave               },
    { 250,               display::Key::small_u_acute               },
    { 251,               display::Key::small_u_circumflex          },
    { 252,               display::Key::small_u_diaeresis           },
    { 253,               display::Key::small_y_acute               },
    { 254,               display::Key::small_thorn                 },
    { 255,               display::Key::small_y_diaeresis           },
};


constexpr core::LiteralHashMap g_key_map     = core::make_literal_hash_map(key_assocs);
constexpr core::LiteralHashMap g_rev_key_map = core::make_rev_literal_hash_map(key_assocs);


inline bool map_key(display::KeyCode key_code, display::Key& key) noexcept
{
    SDL_Keycode code = core::cast_from_twos_compl_a<SDL_Keycode>(key_code.code);
    return g_key_map.find(code, key);
}


inline bool rev_map_key(display::Key key, display::KeyCode& key_code) noexcept
{
    SDL_Keycode code = {};
    if (ARCHON_LIKELY(g_rev_key_map.find(key, code))) {
        key_code = { display::KeyCode::code_type(code) };
        return true;
    }
    return false;
}


inline auto map_mouse_button(Uint8 button) noexcept -> display::MouseButton
{
    switch (button) {
        case SDL_BUTTON_LEFT:
            return display::MouseButton::left;
        case SDL_BUTTON_MIDDLE:
            return display::MouseButton::middle;
        case SDL_BUTTON_RIGHT:
            return display::MouseButton::right;
        case SDL_BUTTON_X1:
            return display::MouseButton::x1;
        case SDL_BUTTON_X2:
            return display::MouseButton::x2;
    }
    ARCHON_ASSERT_UNREACHABLE();
    return {};
}


#else // !HAVE_SDL


class SlotImpl final
    : public display::Implementation::Slot {
public:
    auto ident() const noexcept -> std::string_view override;
    auto get_implementation_a(const display::Guarantees&) const noexcept -> const display::Implementation* override;
};


auto SlotImpl::ident() const noexcept -> std::string_view
{
    return g_implementation_ident;
}


auto SlotImpl::get_implementation_a(const display::Guarantees&) const noexcept -> const display::Implementation*
{
    return nullptr;
}


#endif // !HAVE_SDL


} // unnamed namespace


auto display::get_sdl_implementation_slot() noexcept -> const display::Implementation::Slot&
{
    static SlotImpl slot;
    return slot;
}
