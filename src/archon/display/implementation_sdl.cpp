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
#include <archon/util/colors.hpp>                 
#include <archon/image.hpp>
#include <archon/display/impl/config.h>
#include <archon/display/window.hpp>
#include <archon/display/event.hpp>
#include <archon/display/keysyms.hpp>
#include <archon/display/event_handler.hpp>
#include <archon/display/mandates.hpp>
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


auto map_key(SDL_Keycode key_sym) noexcept -> display::KeyEvent::KeySym;


class ImplementationImpl
    : public display::Implementation {
public:
    mutable std::mutex mutex;
    mutable bool have_connection = false;

    auto ident() const noexcept -> std::string_view override final;
    bool is_available(const display::Mandates&) const noexcept override final;
    auto new_connection(const std::locale&, const display::Mandates&) const ->
        std::unique_ptr<display::Connection> override final;
};


class ConnectionImpl
    : public display::Connection {
public:
    const ImplementationImpl& impl;
    const std::locale locale;

    ConnectionImpl(const ImplementationImpl&, const std::locale&) noexcept;
    ~ConnectionImpl() noexcept override;

    void open();
    void register_window(Uint32 id, display::EventHandler&, int cookie);
    void unregister_window(Uint32 id) noexcept;
    auto map_timestamp(Uint32 timestamp) -> display::TimedEvent::Timestamp;

    auto new_window(std::string_view, display::Size, display::EventHandler&, display::Window::Config) ->
        std::unique_ptr<display::Window> override final;
    void process_events() override final;
    bool process_events(time_point_type) override final;
    int get_num_screens() const override final;
    auto get_screen_bounds(int) const -> display::Box override final;
    auto get_screen_resolution(int) const -> display::Resolution override final;
    int get_num_screen_visuals(int) const override final;
    int get_default_screen() const override final;

private:
    bool m_was_opened = false;

    struct WindowEntry {
        display::EventHandler& event_handler;
        int cookie;
    };

    core::FlatMap<Uint32, WindowEntry> m_windows;

    Uint32 m_prev_timestamp = 0;
    std::int_fast64_t m_timestamp_major = 0;

    int get_display_index(int screen) const noexcept;

    bool process_outstanding_events();
    void wait_for_events();
    bool wait_for_events(time_point_type deadline);
};


class WindowImpl
    : public display::Window {
public:
    ConnectionImpl& conn;

    WindowImpl(ConnectionImpl&) noexcept;
    ~WindowImpl() noexcept override;

    void create(std::string_view title, display::Size size, display::EventHandler&, Config);
    auto ensure_renderer() -> SDL_Renderer*;

    void show() override final;
    void hide() override final;
    void set_title(std::string_view) override final;
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


bool ImplementationImpl::is_available(const display::Mandates& mandates) const noexcept
{
    return bool(mandates.exclusive_sdl_mandate);
}


auto ImplementationImpl::new_connection(const std::locale& locale, const display::Mandates& mandates) const ->
    std::unique_ptr<display::Connection>
{
    if (ARCHON_LIKELY(is_available(mandates))) {
        auto conn = std::make_unique<ConnectionImpl>(*this, locale); // Throws
        conn->open(); // Throws
        return conn;
    }
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
    Uint32 flags = SDL_INIT_VIDEO;
    int ret = SDL_Init(flags);
    if (ARCHON_LIKELY(ret >= 0)) {
        impl.have_connection = true;
        m_was_opened = true;
        return;
    }
    throw_sdl_error(locale, "SDL_Init() failed"); // Throws
}


inline void ConnectionImpl::register_window(Uint32 id, display::EventHandler& event_handler, int cookie)
{
    WindowEntry entry = { event_handler, cookie };
    auto i = m_windows.emplace(id, entry); // Throws
    bool was_inserted = i.second;
    ARCHON_ASSERT(was_inserted);
}


inline void ConnectionImpl::unregister_window(Uint32 id) noexcept
{
    m_windows.erase(id);
}


auto ConnectionImpl::map_timestamp(Uint32 timestamp) -> display::TimedEvent::Timestamp
{
    // Try to fix wrap-around "disaster" after 49 days. This assumes that SDL timestamps
    // originate from a steady / monotonic clock.
    bool wrapped_around = (timestamp < m_prev_timestamp &&
                           (m_prev_timestamp & std::uint_fast32_t(1) << 31) != 0);
    if (ARCHON_UNLIKELY(wrapped_around))
        m_timestamp_major += (std::int_fast64_t(1) << 32);
    m_prev_timestamp = timestamp;
    auto value = std::chrono::milliseconds::rep(m_timestamp_major + timestamp);
    return std::chrono::milliseconds(value);
}


auto ConnectionImpl::new_window(std::string_view title, display::Size size, display::EventHandler& event_handler,
                                display::Window::Config config) -> std::unique_ptr<display::Window>
{
    auto win = std::make_unique<WindowImpl>(*this); // Throws
    win->create(title, size, event_handler, config); // Throws
    return win;
}


void ConnectionImpl::process_events()
{
  again:
    if (ARCHON_LIKELY(process_outstanding_events())) { // Throws
        wait_for_events(); // Throws
        goto again;
    }
}


bool ConnectionImpl::process_events(time_point_type deadline)
{
  again:
    if (ARCHON_LIKELY(process_outstanding_events())) { // Throws
        if (ARCHON_LIKELY(wait_for_events(deadline))) // Throws
            goto again;
        return true; // Deadline expired
    }
    return false; // QUIT event occurred
}


int ConnectionImpl::get_num_screens() const
{
    int ret = SDL_GetNumVideoDisplays();
    if (ARCHON_LIKELY(ret >= 0)) {
        ARCHON_ASSERT(ret >= 1);
        return ret;
    }
    throw_sdl_error(locale, "SDL_GetNumVideoDisplays() failed"); // Throws
}


auto ConnectionImpl::get_screen_bounds(int screen) const -> display::Box
{
    int display_index = get_display_index(screen);
    SDL_Rect rect;
    int ret = SDL_GetDisplayBounds(display_index, &rect);
    if (ARCHON_LIKELY(ret >= 0)) {
        ARCHON_ASSERT(ret == 0);
        return { display::Pos(rect.x, rect.y), display::Size(rect.w, rect.h) };
    }
    throw_sdl_error(locale, "SDL_GetDisplayBounds() failed"); // Throws
}


auto ConnectionImpl::get_screen_resolution(int screen) const -> display::Resolution
{
    int display_index = get_display_index(screen);
    float* ddpi = nullptr;
    float hdpi = 0, vdpi = 0;
    int ret = SDL_GetDisplayDPI(display_index, ddpi, &hdpi, &vdpi);
    if (ARCHON_LIKELY(ret >= 0)) {
        ARCHON_ASSERT(ret == 0);
        auto ppi_to_ppcm = [](double val) {
            return 1/2.54 * val;
        };
        return { ppi_to_ppcm(hdpi), ppi_to_ppcm(vdpi) };
    }
    throw_sdl_error(locale, "SDL_GetDisplayDPI() failed"); // Throws
}


int ConnectionImpl::get_num_screen_visuals(int screen) const
{
    int display_index = get_display_index(screen);
    int ret = SDL_GetNumDisplayModes(display_index);
    if (ARCHON_LIKELY(ret >= 0)) {
        ARCHON_ASSERT(ret >= 1);
        return ret;
    }
    throw_sdl_error(locale, "SDL_GetNumDisplayModes() failed"); // Throws
}


int ConnectionImpl::get_default_screen() const
{
    // SDL claims that 0 refers to the "primary display". It makes sense to use this as the
    // default screen.
    return 0;
}


inline int ConnectionImpl::get_display_index(int screen) const noexcept
{
    if (screen > 0)
        return screen;
    return get_default_screen();
}


bool ConnectionImpl::process_outstanding_events()
{
    for (;;) {
        SDL_Event event;
        int ret = SDL_PollEvent(&event);
        if (ARCHON_LIKELY(ret == 0))
            break;
        ARCHON_ASSERT(ret == 1);
        switch (event.type) {
            case SDL_KEYDOWN: {
                // FIXME: What, if anything, ensures that event.key.windowID refers to the window that is currently registered under than identifier, and not to some earlier window that was also identified by that value?                         
                // FIXME: Expect same window as before            
                auto i = m_windows.find(event.key.windowID);
                ARCHON_ASSERT(i != m_windows.end());
                const WindowEntry& entry = i->second;
                display::KeyEvent event_2;
                event_2.cookie = entry.cookie;
                event_2.timestamp = map_timestamp(event.key.timestamp);
                event_2.key_sym = map_key(event.key.keysym.sym);
                if (ARCHON_LIKELY(entry.event_handler.on_keydown(event_2))) // Throws
                    break;
                return false; // Quit
            }
            case SDL_QUIT:
                return false; // Quit
        }
    }
    return true; // Events are ready to be processed
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
        // FIXME: When SDL_WaitEventTimeout() returns zero, we need to determine whether it
        // was because an error occurred or the timeout was reached, but how? The
        // documentation for SDL_GetError() strongly discourages using that function as a
        // way of checking whether an error has occurred (for good reason). Using the
        // discouraged method for now. See
        // https://discourse.libsdl.org/t/proposal-for-sdl-3-return-value-improvement-for-sdl-waiteventtimeout/45743
        SDL_ClearError();
        SDL_Event* event = nullptr;
        int ret = SDL_WaitEventTimeout(event, timeout);
        if (ARCHON_LIKELY(ret == 1))
            return true;
        ARCHON_ASSERT(ret == 0);
        bool error_occurred = (SDL_GetError()[0] != '\0');
        if (ARCHON_LIKELY(!error_occurred)) {
            if (ARCHON_LIKELY(complete))
                goto expired;
            goto again;
        }
        throw_sdl_error(locale, "SDL_WaitEventTimeout() failed"); // Throws
    }
  expired:
    return false;
}


inline WindowImpl::WindowImpl(ConnectionImpl& conn_2) noexcept
    : conn(conn_2)
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


void WindowImpl::create(std::string_view title, display::Size size, display::EventHandler& event_handler,
                        Config config)
{
    std::string title_2 = std::string(title); // Throws
    int x = SDL_WINDOWPOS_UNDEFINED;
    int y = SDL_WINDOWPOS_UNDEFINED;
    int w = size.width;
    int h = size.height;
    Uint32 flags = SDL_WINDOW_HIDDEN;
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


constexpr std::pair<SDL_Keycode, display::KeyEvent::KeySym> key_assocs[] {
    { SDLK_BACKSPACE,  display::key_BackSpace  },
    { SDLK_TAB,        display::key_Tab        },
    { SDLK_CLEAR,      display::key_Clear      },
    { SDLK_RETURN,     display::key_Return     },
    { SDLK_PAUSE,      display::key_Pause      },
    { SDLK_SCROLLLOCK, display::key_ScrollLock },
    { SDLK_SYSREQ,     display::key_SysReq     },
    { SDLK_ESCAPE,     display::key_Escape     },
    { SDLK_DELETE,     display::key_Delete     },

                                                                               
    { SDLK_0,          display::key_Digit0     },
    { SDLK_1,          display::key_Digit1     },
    { SDLK_2,          display::key_Digit2     },
    { SDLK_3,          display::key_Digit3     },
    { SDLK_4,          display::key_Digit4     },
    { SDLK_5,          display::key_Digit5     },
    { SDLK_6,          display::key_Digit6     },
    { SDLK_7,          display::key_Digit7     },
    { SDLK_8,          display::key_Digit8     },
    { SDLK_9,          display::key_Digit9     },
};


constexpr core::LiteralHashMap g_key_map     = core::make_literal_hash_map(key_assocs);
//constexpr core::LiteralHashMap g_rev_key_map = core::make_rev_literal_hash_map(key_assocs);     


inline auto map_key(SDL_Keycode key_sym) noexcept -> display::KeyEvent::KeySym
{
    display::KeyEvent::KeySym key_sym_2 = display::key_Unknown;
    g_key_map.find(key_sym, key_sym_2);
    return key_sym_2;
}


#else // !ARCHON_DISPLAY_HAVE_SDL


class ImplementationImpl
    : public display::Implementation {
public:
    auto ident() const noexcept -> std::string_view override final;
    bool is_available(const display::Mandates&) const noexcept override final;
    auto new_connection(const std::locale&, const display::Mandates&) const ->
        std::unique_ptr<display::Connection> override final;
};


auto ImplementationImpl::ident() const noexcept -> std::string_view
{
    return g_implementation_ident;
}


bool ImplementationImpl::is_available(const display::Mandates&) const noexcept
{
    return false;
}


auto ImplementationImpl::new_connection(const std::locale&, const display::Mandates&) const ->
    std::unique_ptr<display::Connection>
{
    return nullptr;
}


#endif // !ARCHON_DISPLAY_HAVE_SDL


} // unnamed namespace


auto display::get_sdl_implementation() noexcept -> const display::Implementation&
{
    static ImplementationImpl impl;
    return impl;
}
