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
#include <string_view>
#include <string>
#include <locale>
#include <mutex>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/flat_map.hpp>
#include <archon/core/literal_hash_map.hpp>
#include <archon/core/format.hpp>
#include <archon/util/color.hpp>
#include <archon/util/colors.hpp>                 
#include <archon/image.hpp>
#include <archon/display/impl/config.h>
#include <archon/display/key.hpp>
#include <archon/display/key_code.hpp>
#include <archon/display/event.hpp>
#include <archon/display/event_handler.hpp>
#include <archon/display/guarantees.hpp>
#include <archon/display/window.hpp>
#include <archon/display/connection.hpp>
#include <archon/display/implementation.hpp>
#include <archon/display/implementation_sdl.hpp>

#if ARCHON_DISPLAY_HAVE_SDL
#  if ARCHON_CLANG
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wold-style-cast"
#  endif
#  include <SDL.h>
#  if ARCHON_CLANG
#    pragma clang diagnostic pop
#  endif
#endif


using namespace archon;


namespace {


constexpr std::string_view g_implementation_ident = "sdl";


#if ARCHON_DISPLAY_HAVE_SDL


auto get_sdl_error(const std::locale& locale, std::string_view message) -> std::string
{
    return core::format(locale, "%s: %s", message, SDL_GetError()); // Throws
}


[[noreturn]] void throw_sdl_error(const std::locale& locale, std::string_view message)
{
    std::string msg = get_sdl_error(locale, message); // Throws
    throw std::runtime_error(std::move(msg));
}


auto map_mouse_button(Uint8 button) noexcept -> display::MouseButton;
bool map_key(display::KeyCode, display::Key&) noexcept;
bool rev_map_key(display::Key, display::KeyCode&) noexcept;


class ImplementationImpl
    : public display::Implementation {
public:
    mutable std::mutex mutex;
    mutable bool have_connection = false;

    auto ident() const noexcept -> std::string_view override final;
    bool is_available(const display::Guarantees&) const noexcept override final;
    auto new_connection(const std::locale&, const display::Guarantees&) const ->
        std::unique_ptr<display::Connection> override final;
    bool try_map_key_to_key_code(display::Key, display::KeyCode&) const override final;
    bool try_map_key_code_to_key(display::KeyCode, display::Key&) const override final;
    bool try_get_key_name(display::KeyCode, std::string_view&) const override final;
};


class ConnectionImpl
    : private display::ConnectionEventHandler
    , public display::Connection {
public:
    const ImplementationImpl& impl;
    const std::locale locale;

    ConnectionImpl(const ImplementationImpl&, const std::locale&) noexcept;
    ~ConnectionImpl() noexcept override;

    void open();
    void register_window(Uint32 id, display::WindowEventHandler&, int cookie);
    void unregister_window(Uint32 id) noexcept;

    auto new_window(std::string_view, display::Size, display::WindowEventHandler&, display::Window::Config) ->
        std::unique_ptr<display::Window> override final;
    auto new_window(int, std::string_view, display::Size, display::WindowEventHandler&, display::Window::Config) ->
        std::unique_ptr<display::Window> override final;
    void process_events(display::ConnectionEventHandler*) override final;
    bool process_events(time_point_type, display::ConnectionEventHandler*) override final;
    int get_num_displays() const override final;
    int get_default_display() const override final;
    bool try_get_display_conf(int, core::Buffer<display::Screen>&, core::Buffer<char>&,
                              std::size_t&) const override final;
    auto get_implementation() const noexcept -> const display::Implementation& override final;

private:
    using millis_type = std::chrono::milliseconds;

    bool m_was_opened = false;

    struct WindowEntry {
        display::WindowEventHandler& event_handler;
        int cookie;
    };

    core::FlatMap<Uint32, WindowEntry> m_windows;

    Uint32 m_curr_window_id = 0;
    const WindowEntry* m_curr_window_entry = nullptr;
    bool m_have_prev_timestamp = false;
    Uint32 m_prev_timestamp = 0;
    time_point_type m_prev_timestamp_2 = {};
    millis_type::rep m_timestamp_offset = 0;

    bool process_outstanding_events(display::ConnectionEventHandler&);
    void wait_for_events();
    bool wait_for_events(time_point_type deadline);

    auto lookup_window_entry(Uint32 window_id) noexcept -> const WindowEntry*;
    auto map_next_timestamp(Uint32 timestamp) -> display::TimedWindowEvent::Timestamp;
};


class WindowImpl
    : public display::Window {
public:
    ConnectionImpl& conn;

    WindowImpl(ConnectionImpl&) noexcept;
    ~WindowImpl() noexcept override;

    void create(std::string_view title, display::Size size, display::WindowEventHandler&, Config);
    auto ensure_renderer() -> SDL_Renderer*;

    void show() override final;
    void hide() override final;
    void set_title(std::string_view) override final;
    void set_size(display::Size) override final;
    void set_fullscreen_mode(bool) override final;
    void fill(util::Color) override final;
    auto new_texture(display::Size) -> std::unique_ptr<display::Texture> override final;
    void put_texture(const display::Texture&) override final;
    void present() override final;
    void opengl_make_current() override final;
    void opengl_swap_buffers() override final;

private:
    SDL_Window* m_win = nullptr;
    Uint32 m_id = 0; // If nonzero, this window has been registered in the connection object
    SDL_Renderer* m_renderer = nullptr;
    SDL_GLContext m_gl_context = nullptr;

    auto create_renderer() -> SDL_Renderer*;
};


class TextureImpl
    : public display::Texture {
public:
    WindowImpl& win;

    TextureImpl(WindowImpl&) noexcept;
    ~TextureImpl() noexcept override;

    void create(display::Size size);
    auto get() const noexcept -> SDL_Texture*;

    void put_image(const image::Image&) override final;

private:
    SDL_Texture* m_tex = nullptr;
};


auto ImplementationImpl::ident() const noexcept -> std::string_view
{
    return g_implementation_ident;
}


bool ImplementationImpl::is_available(const display::Guarantees& guarantees) const noexcept
{
    return (guarantees.no_other_use_of_sdl &&
            guarantees.main_thread_exclusive &&
            guarantees.only_one_connection);
}


auto ImplementationImpl::new_connection(const std::locale& locale, const display::Guarantees& guarantees) const ->
    std::unique_ptr<display::Connection>
{
    if (ARCHON_LIKELY(is_available(guarantees))) {
        auto conn = std::make_unique<ConnectionImpl>(*this, locale); // Throws
        conn->open(); // Throws
        return conn;
    }
    return nullptr;
}


bool ImplementationImpl::try_map_key_to_key_code(display::Key key, display::KeyCode& key_code) const
{
    return ::rev_map_key(key, key_code);
}


bool ImplementationImpl::try_map_key_code_to_key(display::KeyCode key_code, display::Key& key) const
{
    return ::map_key(key_code, key);
}


bool ImplementationImpl::try_get_key_name(display::KeyCode key_code, std::string_view& name) const
{
    SDL_Keycode code = core::cast_from_twos_compl_a<SDL_Keycode>(key_code.code);
    const char* name_2 = SDL_GetKeyName(code);
    if (ARCHON_LIKELY(name_2)) {
        name = std::string_view(name_2); // Throws
        return true;
    }
    return false;
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
    Uint32 flags = SDL_INIT_VIDEO;
    int ret = SDL_Init(flags);
    if (ARCHON_LIKELY(ret >= 0)) {
        impl.have_connection = true;
        m_was_opened = true;
        return;
    }
    throw_sdl_error(locale, "SDL_Init() failed"); // Throws
}


inline void ConnectionImpl::register_window(Uint32 id, display::WindowEventHandler& event_handler, int cookie)
{
    WindowEntry entry = { event_handler, cookie };
    auto i = m_windows.emplace(id, entry); // Throws
    bool was_inserted = i.second;
    ARCHON_ASSERT(was_inserted);
    // Inserting into `m_windows` will generally invalidate `m_curr_window_entry`, so it
    // needs to be reset here.
    m_curr_window_id = id;
    m_curr_window_entry = &i.first->second;
}


inline void ConnectionImpl::unregister_window(Uint32 id) noexcept
{
    m_windows.erase(id);
    if (ARCHON_LIKELY(id == m_curr_window_id))
        m_curr_window_entry = nullptr;
}


auto ConnectionImpl::new_window(std::string_view title, display::Size size,
                                display::WindowEventHandler& event_handler,
                                display::Window::Config config) -> std::unique_ptr<display::Window>
{
    auto win = std::make_unique<WindowImpl>(*this); // Throws
    win->create(title, size, event_handler, config); // Throws
    return win;
}


auto ConnectionImpl::new_window(int display, std::string_view title, display::Size size,
                                display::WindowEventHandler& event_handler,
                                display::Window::Config config) -> std::unique_ptr<display::Window>
{
    if (ARCHON_UNLIKELY(display != 0))
        throw std::invalid_argument("Bad display index");
    return new_window(title, size, event_handler, std::move(config)); // Throws
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


int ConnectionImpl::get_num_displays() const
{
    // SDL does not provide access to more than one display (X screen) at a time.
    return 1;
}


int ConnectionImpl::get_default_display() const
{
    return 0;
}


bool ConnectionImpl::try_get_display_conf(int display, core::Buffer<display::Screen>&, core::Buffer<char>&,
                                          std::size_t&) const
{
    if (ARCHON_UNLIKELY(display != 0))
        throw std::invalid_argument("Bad display index");
    return false;
}


auto ConnectionImpl::get_implementation() const noexcept -> const display::Implementation&
{
    return display::get_sdl_implementation();
}


bool ConnectionImpl::process_outstanding_events(display::ConnectionEventHandler& connection_event_handler)
{
    for (;;) {
        SDL_Event event;
        int ret = SDL_PollEvent(&event);
        if (ARCHON_LIKELY(ret == 0))
            break;
        ARCHON_ASSERT(ret == 1);
        switch (event.type) {
            case SDL_MOUSEMOTION: {
                if (ARCHON_LIKELY(event.motion.state == 0))
                    break;
                const WindowEntry* entry = lookup_window_entry(event.motion.windowID);
                if (ARCHON_LIKELY(entry)) {
                    display::MouseButtonEvent event_2;
                    event_2.cookie = entry->cookie;
                    event_2.timestamp = map_next_timestamp(event.motion.timestamp); // Throws
                    event_2.pos = { event.motion.x, event.motion.y };
                    bool proceed = entry->event_handler.on_mousemove(event_2); // Throws
                    if (ARCHON_LIKELY(proceed))
                        break;
                    return false; // Interrupt
                }
                break;
            }
            case SDL_MOUSEWHEEL: {
                const WindowEntry* entry = lookup_window_entry(event.wheel.windowID);
                if (ARCHON_LIKELY(entry)) {
                    display::ScrollEvent event_2;
                    event_2.cookie = entry->cookie;
                    event_2.timestamp = map_next_timestamp(event.wheel.timestamp); // Throws
                    event_2.amount = { event.wheel.preciseX, event.wheel.preciseY };
                    bool proceed = entry->event_handler.on_scroll(event_2); // Throws
                    if (ARCHON_LIKELY(proceed))
                        break;
                    return false; // Interrupt
                }
                break;
            }
            case SDL_MOUSEBUTTONDOWN: {
                const WindowEntry* entry = lookup_window_entry(event.button.windowID);
                if (ARCHON_LIKELY(entry)) {
                    display::MouseButtonEvent event_2;
                    event_2.cookie = entry->cookie;
                    event_2.timestamp = map_next_timestamp(event.button.timestamp); // Throws
                    event_2.pos = { event.button.x, event.button.y };
                    event_2.button = map_mouse_button(event.button.button);
                    bool proceed = entry->event_handler.on_mousedown(event_2); // Throws
                    if (ARCHON_LIKELY(proceed))
                        break;
                    return false; // Interrupt
                }
                break;
            }
            case SDL_MOUSEBUTTONUP: {
                const WindowEntry* entry = lookup_window_entry(event.button.windowID);
                if (ARCHON_LIKELY(entry)) {
                    display::MouseButtonEvent event_2;
                    event_2.cookie = entry->cookie;
                    event_2.timestamp = map_next_timestamp(event.button.timestamp); // Throws
                    event_2.pos = { event.button.x, event.button.y };
                    event_2.button = map_mouse_button(event.button.button);
                    bool proceed = entry->event_handler.on_mouseup(event_2); // Throws
                    if (ARCHON_LIKELY(proceed))
                        break;
                    return false; // Interrupt
                }
                break;
            }
            case SDL_KEYDOWN: {
                if (ARCHON_LIKELY(event.key.repeat == 0)) {
                    const WindowEntry* entry = lookup_window_entry(event.key.windowID);
                    if (ARCHON_LIKELY(entry)) {
                        display::KeyEvent event_2;
                        event_2.cookie = entry->cookie;
                        event_2.timestamp = map_next_timestamp(event.key.timestamp); // Throws
                        event_2.key_code = { display::KeyCode::code_type(event.key.keysym.sym) };
                        bool proceed = entry->event_handler.on_keydown(event_2); // Throws
                        if (ARCHON_LIKELY(proceed))
                            break;
                        return false; // Interrupt
                    }
                }
                break;
            }
            case SDL_KEYUP: {
                const WindowEntry* entry = lookup_window_entry(event.key.windowID);
                if (ARCHON_LIKELY(entry)) {
                    display::KeyEvent event_2;
                    event_2.cookie = entry->cookie;
                    event_2.timestamp = map_next_timestamp(event.key.timestamp); // Throws
                    event_2.key_code = { display::KeyCode::code_type(event.key.keysym.sym) };
                    bool proceed = entry->event_handler.on_keyup(event_2); // Throws
                    if (ARCHON_LIKELY(proceed))
                        break;
                    return false; // Interrupt
                }
                break;
            }
            case SDL_WINDOWEVENT:
                switch (event.window.event) {
                    case SDL_WINDOWEVENT_ENTER: {
                        const WindowEntry* entry = lookup_window_entry(event.window.windowID);
                        if (ARCHON_LIKELY(entry)) {
                            display::TimedWindowEvent event_2;
                            event_2.cookie = entry->cookie;
                            event_2.timestamp = map_next_timestamp(event.window.timestamp); // Throws
                            bool proceed = entry->event_handler.on_mouseover(event_2); // Throws
                            if (ARCHON_LIKELY(proceed))
                                break;
                            return false; // Interrupt
                        }
                        break;
                    }
                    case SDL_WINDOWEVENT_LEAVE: {
                        const WindowEntry* entry = lookup_window_entry(event.window.windowID);
                        if (ARCHON_LIKELY(entry)) {
                            display::TimedWindowEvent event_2;
                            event_2.cookie = entry->cookie;
                            event_2.timestamp = map_next_timestamp(event.window.timestamp); // Throws
                            bool proceed = entry->event_handler.on_mouseout(event_2); // Throws
                            if (ARCHON_LIKELY(proceed))
                                break;
                            return false; // Interrupt
                        }
                        break;
                    }
                    case SDL_WINDOWEVENT_FOCUS_GAINED: {
                        const WindowEntry* entry = lookup_window_entry(event.window.windowID);
                        if (ARCHON_LIKELY(entry)) {
                            display::TimedWindowEvent event_2;
                            event_2.cookie = entry->cookie;
                            event_2.timestamp = map_next_timestamp(event.window.timestamp); // Throws
                            bool proceed = entry->event_handler.on_focus(event_2); // Throws
                            if (ARCHON_LIKELY(proceed))
                                break;
                            return false; // Interrupt
                        }
                        break;
                    }
                    case SDL_WINDOWEVENT_FOCUS_LOST: {
                        const WindowEntry* entry = lookup_window_entry(event.window.windowID);
                        if (ARCHON_LIKELY(entry)) {
                            display::TimedWindowEvent event_2;
                            event_2.cookie = entry->cookie;
                            event_2.timestamp = map_next_timestamp(event.window.timestamp); // Throws
                            bool proceed = entry->event_handler.on_blur(event_2); // Throws
                            if (ARCHON_LIKELY(proceed))
                                break;
                            return false; // Interrupt
                        }
                        break;
                    }
                    case SDL_WINDOWEVENT_EXPOSED: {
                        const WindowEntry* entry = lookup_window_entry(event.window.windowID);
                        if (ARCHON_LIKELY(entry)) {
                            display::WindowEvent event_2;
                            event_2.cookie = entry->cookie;
                            bool proceed = entry->event_handler.on_expose(event_2); // Throws
                            if (ARCHON_LIKELY(proceed))
                                break;
                            return false; // Interrupt
                        }
                        break;
                    }
                    case SDL_WINDOWEVENT_SIZE_CHANGED: {
                        const WindowEntry* entry = lookup_window_entry(event.window.windowID);
                        if (ARCHON_LIKELY(entry)) {
                            display::WindowSizeEvent event_2;
                            event_2.cookie = entry->cookie;
                            core::int_cast(event.window.data1, event_2.size.width); // Throws
                            core::int_cast(event.window.data2, event_2.size.height); // Throws
                            bool proceed = entry->event_handler.on_resize(event_2); // Throws
                            if (ARCHON_LIKELY(proceed))
                                break;
                            return false; // Interrupt
                        }
                        break;
                    }
                    case SDL_WINDOWEVENT_MOVED: {
                        const WindowEntry* entry = lookup_window_entry(event.window.windowID);
                        if (ARCHON_LIKELY(entry)) {
                            display::WindowPosEvent event_2;
                            event_2.cookie = entry->cookie;
                            core::int_cast(event.window.data1, event_2.pos.x); // Throws
                            core::int_cast(event.window.data2, event_2.pos.y); // Throws
                            bool proceed = entry->event_handler.on_reposition(event_2); // Throws
                            if (ARCHON_LIKELY(proceed))
                                break;
                            return false; // Interrupt
                        }
                        break;
                    }
                    case SDL_WINDOWEVENT_CLOSE: {
                        const WindowEntry* entry = lookup_window_entry(event.window.windowID);
                        if (ARCHON_LIKELY(entry)) {
                            display::TimedWindowEvent event_2;
                            event_2.cookie = entry->cookie;
                            event_2.timestamp = map_next_timestamp(event.window.timestamp); // Throws
                            bool proceed = entry->event_handler.on_close(event_2); // Throws
                            if (ARCHON_LIKELY(proceed))
                                break;
                            return false; // Interrupt
                        }
                        break;
                    }
                }
                break;
/*
            case SDL_DISPLAYEVENT: {
                // Fetch updated display information          
                // Call on_display_change(screens) where screens is span of ScreenInfo objects
                // ScreenInfo has fields `name`, `bounds`, `resolution`, and `primary`
                // Question: How does SDL_GetDisplayUsableBounds() work with X11? --> See X11_GetDisplayUsableBounds()                                                                                                                                                         
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
                if (ARCHON_LIKELY(proceed))
                    break;
                return false; // Interrupt
            }
        }
    }
    if (m_windows.empty()) {
        bool proceed = connection_event_handler.on_quit(); // Throws
        if (ARCHON_UNLIKELY(!proceed))
            return false; // Interrupt
    }
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
        // if an error occured is not an option, as it will sometimes report errors when
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


auto ConnectionImpl::lookup_window_entry(Uint32 window_id) noexcept -> const WindowEntry*
{
    // FIXME: What, if anything, ensures that event.key.windowID refers to the window that is currently registered under that identifier, and not to some earlier window that was also identified by that value?
    // These expectation should be that there is a type of event the marks the end of the use of a window identifier               

    if (ARCHON_LIKELY(window_id == m_curr_window_id))
        return m_curr_window_entry;

    const WindowEntry* entry = nullptr;
    auto i = m_windows.find(window_id);
    if (ARCHON_LIKELY(i != m_windows.end()))
        entry = &i->second;
    m_curr_window_id = window_id;
    m_curr_window_entry = entry;
    return entry;
}


auto ConnectionImpl::map_next_timestamp(Uint32 timestamp) -> display::TimedWindowEvent::Timestamp
{
    // Try to fix wrap-around "disaster", which occurs after 49 days when Uint32 is a 32-bit
    // integer.
    //
    // Assumptions:
    // - SDL timestamps originate from a steady / monotonic clock
    // - SDL timestamps is N lowest order bits of true timestamp where N is number of value
    //   bits in Uint32
    //
    if constexpr (core::int_width<Uint32>() < core::int_width<millis_type::rep>()) {
        constexpr millis_type::rep module = millis_type::rep(1) << core::int_width<Uint32>();
        time_point_type timestamp_2 = clock_type::now();
        if (ARCHON_LIKELY(m_have_prev_timestamp)) {
            millis_type::rep millis = std::chrono::round<millis_type>(timestamp_2 - m_prev_timestamp_2).count();
            core::int_add(millis, module / 2 - (timestamp - m_prev_timestamp)); // Throws
            core::int_add(m_timestamp_offset, (millis / module) * module); // Throws
        }
        m_prev_timestamp = timestamp;
        m_prev_timestamp_2 = timestamp_2;
        m_have_prev_timestamp = true;
    }
    return millis_type(millis_type::rep(m_timestamp_offset + timestamp));
}


inline WindowImpl::WindowImpl(ConnectionImpl& conn_2) noexcept
    : conn(conn_2)
{
}


WindowImpl::~WindowImpl() noexcept
{
    // FIXME: Maybe safer to inject a custom event here such that the following can be carried out in the context of the event handler                                                               
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


void WindowImpl::create(std::string_view title, display::Size size, display::WindowEventHandler& event_handler,
                        Config config)
{
    std::string title_2 = std::string(title); // Throws
    int x = SDL_WINDOWPOS_UNDEFINED;
    int y = SDL_WINDOWPOS_UNDEFINED;
    int w = size.width;
    int h = size.height;
    Uint32 flags = SDL_WINDOW_HIDDEN;
    if (config.resizable)
        flags |= SDL_WINDOW_RESIZABLE;
    if (config.fullscreen)
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    if (config.enable_opengl)
        flags |= SDL_WINDOW_OPENGL;
    SDL_Window* win = SDL_CreateWindow(title_2.c_str(), x, y, w, h, flags);
    if (ARCHON_LIKELY(win)) {
        m_win = win;
        Uint32 id = SDL_GetWindowID(m_win);
        if (ARCHON_LIKELY(id > 0)) {
            conn.register_window(id, event_handler, config.cookie); // Throws
            m_id = id;
            return;
        }
        throw_sdl_error(conn.locale, "SDL_GetWindowID() failed"); // Throws
    }
    throw_sdl_error(conn.locale, "SDL_CreateWindow() failed"); // Throws
}


inline auto WindowImpl::ensure_renderer() -> SDL_Renderer*
{
    if (ARCHON_LIKELY(m_renderer))
        return m_renderer;
    return create_renderer(); // Throws
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
    std::string title_2 = std::string(title); // Throws
    SDL_SetWindowTitle(m_win, title_2.c_str());
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
    int ret = SDL_SetRenderDrawColor(renderer, color.red(), color.green(), color.blue(), color.alpha());
    if (ARCHON_LIKELY(ret >= 0)) {
        ARCHON_ASSERT(ret == 0);
        ret = SDL_RenderClear(renderer);
        if (ARCHON_LIKELY(ret >= 0)) {
            ARCHON_ASSERT(ret == 0);
            return;
        }
        throw_sdl_error(conn.locale, "SDL_RenderClear() failed"); // Throws
    }
    throw_sdl_error(conn.locale, "SDL_SetRenderDrawColor() failed"); // Throws
}


auto WindowImpl::new_texture(display::Size size) -> std::unique_ptr<display::Texture>
{
    auto tex = std::make_unique<TextureImpl>(*this); // Throws
    tex->create(size); // Throws
    return tex;
}


void WindowImpl::put_texture(const display::Texture& tex)
{
    ARCHON_ASSERT(dynamic_cast<const TextureImpl*>(&tex));
    const TextureImpl& tex_2 = static_cast<const TextureImpl&>(tex);
    ARCHON_ASSERT(&tex_2.win.conn.impl == &conn.impl);
    ARCHON_ASSERT(m_renderer);
    const SDL_Rect* src_rect = nullptr;
    const SDL_Rect* dst_rect = nullptr;
    int ret = SDL_RenderCopy(m_renderer, tex_2.get(), src_rect, dst_rect);
    if (ARCHON_LIKELY(ret >= 0)) {
        ARCHON_ASSERT(ret == 0);
        return;
    }
    throw_sdl_error(conn.locale, "SDL_RenderCopy() failed"); // Throws
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
    if (ARCHON_LIKELY(renderer)) {
        m_renderer = renderer;
        return renderer;
    }
    throw_sdl_error(conn.locale, "SDL_CreateRenderer() failed"); // Throws
}


inline TextureImpl::TextureImpl(WindowImpl& win_2) noexcept
    : win(win_2)
{
}


TextureImpl::~TextureImpl() noexcept
{
    if (m_tex)
        SDL_DestroyTexture(m_tex);
}


void TextureImpl::create(display::Size size)
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

    using ARGB32 = image::IntegerPixelFormat<image::ChannelSpec_RGBA, image::int8_type, 8, image::int8_type, 1, core::Endianness::big, true, false>;    
    image::BufferedImage<ARGB32> img_2(img.get_size()); // Throws
    image::Writer writer(img_2); // Throws
/*
    writer.set_foreground_color(util::colors::green);    
    writer.fill();    
*/
    writer.put_image({ 0, 0 }, img); // Throws

    const SDL_Rect* rect = nullptr;
    int pitch = 4 * img.get_size().width;
    int ret = SDL_UpdateTexture(m_tex, rect, img_2.get_buffer().data(), pitch);
    if (ARCHON_LIKELY(ret >= 0)) {
        ARCHON_ASSERT(ret == 0);
        return;
    }
    throw_sdl_error(win.conn.locale, "SDL_UpdateTexture() failed"); // Throws
}


constexpr std::pair<SDL_Keycode, display::Key> key_assocs[] {
    { SDLK_BACKSPACE,    display::Key::backspace            },
    { SDLK_TAB,          display::Key::tab                  },
    { SDLK_CLEAR,        display::Key::clear                },
    { SDLK_RETURN,       display::Key::return_              },
    { SDLK_PAUSE,        display::Key::pause                },
    { SDLK_SCROLLLOCK,   display::Key::scroll_lock          },
    { SDLK_SYSREQ,       display::Key::sys_req              },
    { SDLK_ESCAPE,       display::Key::escape               },
    { SDLK_SPACE,        display::Key::space                },
    { SDLK_EXCLAIM,      display::Key::exclamation_mark     },
    { SDLK_QUOTEDBL,     display::Key::quotation_mark       },
    { SDLK_HASH,         display::Key::hash_mark            },
    { SDLK_DOLLAR,       display::Key::dollar_sign          },
    { SDLK_PERCENT,      display::Key::percent_sign         },
    { SDLK_AMPERSAND,    display::Key::ampersand            },
    { SDLK_QUOTE,        display::Key::apostrophe           },
    { SDLK_LEFTPAREN,    display::Key::left_parenthesis     },
    { SDLK_RIGHTPAREN,   display::Key::right_parenthesis    },
    { SDLK_ASTERISK,     display::Key::asterisk             },
    { SDLK_PLUS,         display::Key::plus_sign            },
    { SDLK_COMMA,        display::Key::comma                },
    { SDLK_MINUS,        display::Key::minus_sign           },
    { SDLK_PERIOD,       display::Key::period               },
    { SDLK_SLASH,        display::Key::slash                },
    { SDLK_0,            display::Key::digit_0              },
    { SDLK_1,            display::Key::digit_1              },
    { SDLK_2,            display::Key::digit_2              },
    { SDLK_3,            display::Key::digit_3              },
    { SDLK_4,            display::Key::digit_4              },
    { SDLK_5,            display::Key::digit_5              },
    { SDLK_6,            display::Key::digit_6              },
    { SDLK_7,            display::Key::digit_7              },
    { SDLK_8,            display::Key::digit_8              },
    { SDLK_9,            display::Key::digit_9              },
    { SDLK_COLON,        display::Key::colon                },
    { SDLK_SEMICOLON,    display::Key::semicolon            },
    { SDLK_LESS,         display::Key::less_than_sign       },
    { SDLK_EQUALS,       display::Key::equal_sign           },
    { SDLK_GREATER,      display::Key::greater_than_sign    },
    { SDLK_QUESTION,     display::Key::question_mark        },
    { SDLK_AT,           display::Key::at_sign              },
    { SDLK_LEFTBRACKET,  display::Key::left_square_bracket  },
    { SDLK_BACKSLASH,    display::Key::backslash            },
    { SDLK_RIGHTBRACKET, display::Key::right_square_bracket },
    { SDLK_CARET,        display::Key::circumflex_accent    },
    { SDLK_UNDERSCORE,   display::Key::underscore           },
    { SDLK_BACKQUOTE,    display::Key::grave_accent         },
    { SDLK_a,            display::Key::lower_case_a         },
    { SDLK_b,            display::Key::lower_case_b         },
    { SDLK_c,            display::Key::lower_case_c         },
    { SDLK_d,            display::Key::lower_case_d         },
    { SDLK_e,            display::Key::lower_case_e         },
    { SDLK_f,            display::Key::lower_case_f         },
    { SDLK_g,            display::Key::lower_case_g         },
    { SDLK_h,            display::Key::lower_case_h         },
    { SDLK_i,            display::Key::lower_case_i         },
    { SDLK_j,            display::Key::lower_case_j         },
    { SDLK_k,            display::Key::lower_case_k         },
    { SDLK_l,            display::Key::lower_case_l         },
    { SDLK_m,            display::Key::lower_case_m         },
    { SDLK_n,            display::Key::lower_case_n         },
    { SDLK_o,            display::Key::lower_case_o         },
    { SDLK_p,            display::Key::lower_case_p         },
    { SDLK_q,            display::Key::lower_case_q         },
    { SDLK_r,            display::Key::lower_case_r         },
    { SDLK_s,            display::Key::lower_case_s         },
    { SDLK_t,            display::Key::lower_case_t         },
    { SDLK_u,            display::Key::lower_case_u         },
    { SDLK_v,            display::Key::lower_case_v         },
    { SDLK_w,            display::Key::lower_case_w         },
    { SDLK_x,            display::Key::lower_case_x         },
    { SDLK_y,            display::Key::lower_case_y         },
    { SDLK_z,            display::Key::lower_case_z         },
    { SDLK_DELETE,       display::Key::delete_              },

    { SDLK_LSHIFT,       display::Key::shift_left           },
    { SDLK_RSHIFT,       display::Key::shift_right          },

    { SDLK_KP_PLUS,      display::Key::keypad_plus_sign     },
    { SDLK_KP_MINUS,     display::Key::keypad_minus_sign    },
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


#else // !ARCHON_DISPLAY_HAVE_SDL


class ImplementationImpl
    : public display::Implementation {
public:
    auto ident() const noexcept -> std::string_view override final;
    bool is_available(const display::Guarantees&) const noexcept override final;
    auto new_connection(const std::locale&, const display::Guarantees&) const ->
        std::unique_ptr<display::Connection> override final;
    bool try_map_key_code_to_key(display::KeyCode, display::Key&) const override final;
    bool try_get_key_name(display::KeyCode, std::string_view&) const override final;
};


auto ImplementationImpl::ident() const noexcept -> std::string_view
{
    return g_implementation_ident;
}


bool ImplementationImpl::is_available(const display::Guarantees&) const noexcept
{
    return false;
}


auto ImplementationImpl::new_connection(const std::locale&, const display::Guarantees&) const ->
    std::unique_ptr<display::Connection>
{
    return nullptr;
}


bool ImplementationImpl::try_map_key_code_to_key(display::KeyCode, display::Key&) const
{
    return false;
}


bool ImplementationImpl::try_get_key_name(display::KeyCode, std::string_view&) const
{
    return false;
}


#endif // !ARCHON_DISPLAY_HAVE_SDL


} // unnamed namespace


auto display::get_sdl_implementation() noexcept -> const display::Implementation&
{
    static ImplementationImpl impl;
    return impl;
}
