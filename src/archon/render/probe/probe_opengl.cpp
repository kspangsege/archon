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


#include <cstddef>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <utility>
#include <chrono>
#include <optional>
#include <tuple>
#include <string>
#include <locale>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/math.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/locale.hpp>
#include <archon/core/as_int.hpp>
#include <archon/core/format_as.hpp>
#include <archon/core/file.hpp>
#include <archon/log.hpp>
#include <archon/cli.hpp>
#include <archon/display.hpp>
#include <archon/render/opengl.hpp>


using namespace archon;


#if ARCHON_RENDER_HAVE_OPENGL


namespace {


class event_loop final
    : private display::ConnectionEventHandler
    , private display::WindowEventHandler {
public:
    event_loop(const std::locale& locale, log::Logger& logger, display::Connection& conn, int screen) noexcept;
    ~event_loop() noexcept;

    bool try_init(display::Size window_size);

    void run();

private:
    using clock_type = display::Connection::clock_type;

    static constexpr double s_default_frame_rate = 60;

    std::locale m_locale;
    log::Logger& m_logger;
    display::Connection& m_conn;
    const int m_screen;

    std::unique_ptr<display::Window> m_window;

    core::Buffer<display::Viewport> m_viewports;
    core::Buffer<char> m_viewport_strings;
    std::size_t m_num_viewports = 0;
    display::Size m_window_size;
    display::Pos m_window_pos;
    double m_frame_rate;
    clock_type::duration m_time_per_frame;

    bool m_initialized = false;
    bool m_started = false;
    int m_max_opengl_errors = 8;
    double m_angle = 0;

    void render_frame();

    bool on_keydown(const display::KeyEvent&) override;
    bool on_resize(const display::WindowSizeEvent&) override;
    bool on_reposition(const display::WindowPosEvent&) override;
    bool on_screen_change(int) override;

    void fetch_screen_conf();
    void track_screen_conf();

    void update_frame_rate(double);
};


inline event_loop::event_loop(const std::locale& locale, log::Logger& logger, display::Connection& conn,
                              int screen) noexcept
    : m_locale(locale)
    , m_logger(logger)
    , m_conn(conn)
    , m_screen(screen)
{
}


event_loop::~event_loop() noexcept
{
    m_conn.unset_event_handler();
}


bool event_loop::try_init(display::Size window_size)
{
    ARCHON_ASSERT(!m_initialized);

    m_conn.set_event_handler(*this); // Throws

    m_window_size = window_size;
    update_frame_rate(s_default_frame_rate); // Throws

    display::Window::Config window_config;
    window_config.screen = m_screen;
    window_config.enable_opengl_rendering = true;
    std::unique_ptr<display::Window> window;
    std::string error;
    if (ARCHON_UNLIKELY(!m_conn.try_new_window("Probe OpenGL", window_size, window_config, window, error))) { // Throws
        m_logger.error("Failed to create window: %s", error); // Throws
        return false;
    }

    window->set_event_handler(*this); // Throws
    window->opengl_make_current(); // Throws

    m_logger.info("OpenGL Vendor: %s", glGetString(GL_VENDOR)); // Throws
    m_logger.info("OpenGL Renderer: %s", glGetString(GL_RENDERER)); // Throws
    m_logger.info("OpenGL Version: %s", glGetString(GL_VERSION)); // Throws

    m_window = std::move(window);
    m_initialized = true;
    return true;
}


void event_loop::run()
{
    ARCHON_ASSERT(m_initialized);
    ARCHON_ASSERT(!m_started);

    fetch_screen_conf(); // Throws
    track_screen_conf(); // Throws

    glClearColor(0.2, 0.3, 0.3, 1.0);
    glColor3f(1.0, 0.5, 0.2);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    double view_plane_dist  = 1;
    double view_plane_right = 1;
    double view_plane_top   = 1;
    double far_clip_dist    = 100;
    glFrustum(-view_plane_right, view_plane_right, -view_plane_top, view_plane_top, view_plane_dist, far_clip_dist);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    double camera_dist = 10;
    glTranslated(0, 0, -camera_dist);

    m_window->show(); // Throws
    m_started = true;
    update_frame_rate(m_frame_rate); // Throws

    clock_type::time_point deadline = clock_type::now();
    do {
        deadline += m_time_per_frame;
        clock_type::time_point now = clock_type::now();
        if (ARCHON_UNLIKELY(deadline < now))
            deadline = now;

        render_frame(); // Throws
        m_window->opengl_swap_buffers(); // Throws

        if (m_max_opengl_errors > 0) {
            GLenum error = glGetError();
            if (error != GL_NO_ERROR) {
                m_logger.error("OpenGL error: %s", render::get_opengl_error_message(error)); // Throws
                m_max_opengl_errors -= 1;
                if (m_max_opengl_errors == 0)
                    m_logger.error("No more OpenGL error will be reported"); // Throws
            }
        }
    }
    while (m_conn.process_events_a(deadline)); // Throws
}


void event_loop::render_frame()
{
    glClear(GL_COLOR_BUFFER_BIT);

    glPushMatrix();
    glRotated(core::rad_to_deg(m_angle), 0, 0, -1);

    m_angle += 1 / m_frame_rate;
    for (;;) {
        if (ARCHON_LIKELY(m_angle < 2 * core::pi<double>))
            break;
        m_angle -= 2 * core::pi<double>;
    }

    glBegin(GL_QUADS);
    glVertex3f(-5, -5, 0);
    glVertex3f(+5, -5, 0);
    glVertex3f(+5, +5, 0);
    glVertex3f(-5, +5, 0);
    glEnd();

    glPopMatrix();
}


bool event_loop::on_keydown(const display::KeyEvent& ev)
{
    display::Key key = {};
    if (ARCHON_LIKELY(m_conn.try_map_key_code_to_key(ev.key_code, key))) { // Throws
        if (ARCHON_UNLIKELY(key == display::Key::escape))
            return false;
    }
    return true;
}


bool event_loop::on_resize(const display::WindowSizeEvent& ev)
{
    m_window_size = ev.size;
    track_screen_conf(); // Throws
    return true;
}


bool event_loop::on_reposition(const display::WindowPosEvent& ev)
{
    m_window_pos = ev.pos;
    track_screen_conf(); // Throws
    return true;
}


bool event_loop::on_screen_change(int screen)
{
    if (screen == m_screen) {
        fetch_screen_conf(); // Throws
        track_screen_conf(); // Throws
    }
    return true;
}


void event_loop::fetch_screen_conf()
{
    m_num_viewports = 0;
    m_conn.try_get_screen_conf(m_screen, m_viewports, m_viewport_strings, m_num_viewports); // Throws
}


void event_loop::track_screen_conf()
{
    double frame_rate = s_default_frame_rate;
    core::Span viewports = { m_viewports.data(), m_num_viewports };
    std::size_t i = display::find_viewport(viewports, m_window_pos, m_window_size);
    if (ARCHON_LIKELY(i != std::size_t(-1))) {
        const display::Viewport& viewport = m_viewports[i];
        if (ARCHON_LIKELY(viewport.refresh_rate.has_value()))
            frame_rate = viewport.refresh_rate.value();
    }
    if (ARCHON_UNLIKELY(frame_rate != m_frame_rate))
        update_frame_rate(frame_rate); // Throws
}


void event_loop::update_frame_rate(double rate)
{
    m_frame_rate = rate;
    if (m_started) {
        auto nanos_per_frame = std::chrono::nanoseconds::rep(std::floor(1E9 / m_frame_rate));
        auto time_per_frame = std::chrono::nanoseconds(nanos_per_frame);
        m_time_per_frame = std::chrono::duration_cast<clock_type::duration>(time_per_frame);
        m_logger.detail("Frame rate: %sf/s (%s per frame)", m_frame_rate, core::as_time(m_time_per_frame)); // Throws
    }
}


} // unnamed namespace


#endif // ARCHON_RENDER_HAVE_OPENGL



int main(int argc, char* argv[])
{
    std::locale locale = core::get_default_locale(); // Throws

    bool list_display_implementations = false;
    display::Size window_size = 512;
    log::LogLevel log_level_limit = log::LogLevel::info;
    std::optional<std::string> optional_display_implementation;
    std::optional<int> optional_screen;
    std::optional<std::string> optional_x11_display;

    cli::Spec spec;
    pat("", cli::no_attributes, spec,
        "Lorem ipsum.",
        std::tie()); // Throws

    pat("--list-display-implementations", cli::no_attributes, spec,
        "List known display implementations.",
        [&] {
            list_display_implementations = true;
        }); // Throws

    opt(cli::help_tag, spec); // Throws
    opt(cli::stop_tag, spec); // Throws

    opt("-S, --window-size", "<size>", cli::no_attributes, spec,
        "Set the window size in number of pixels. \"@A\" can be specified either as a pair \"<width>,<height>\", or "
        "as a single value, which is then used as both width and height. The default size is @V.",
        cli::assign(window_size)); // Throws

    opt("-l, --log-level", "<level>", cli::no_attributes, spec,
        "Set the log level limit. The possible levels are @G. The default limit is @Q.",
        cli::assign(log_level_limit)); // Throws

    opt("-i, --display-implementation", "<ident>", cli::no_attributes, spec,
        "Use the specified display implementation. Use `--list-display-implementations` to see which implementations "
        "are available. It is possible that no implementations are available. By default, if any implementations are "
        "available, the one, that is listed first by `--list-display-implementations`, is used.",
        cli::assign(optional_display_implementation)); // Throws

    opt("-s, --screen", "<number>", cli::no_attributes, spec,
        "Target the specified screen (@A). This is an index between zero and the number of screens minus one. If this "
        "option is not specified, the default screen of the display will be targeted.",
        cli::assign(optional_screen)); // Throws

    opt("-D, --x11-display", "<string>", cli::no_attributes, spec,
        "When using the X11-based display implementation, target the specified X11 display (@A). If this option is "
        "not specified, the value of the DISPLAY environment variable will be used.",
        cli::assign(optional_x11_display)); // Throws

    int exit_status = 0;
    if (ARCHON_UNLIKELY(cli::process(argc, argv, spec, exit_status, locale))) // Throws
        return exit_status;

    display::Guarantees guarantees;

    // Promise to not open more than one display connection at a time.
    guarantees.only_one_connection = true;

    // Promise that all use of the display API happens on behalf of the main thread.
    guarantees.main_thread_exclusive = true;

    // Promise that there is no direct or indirect use of the Xlib library (X Window System
    // client library) other than through the Archon display library.
    guarantees.no_other_use_of_x11 = true;

    // Promise that there is no direct or indirect use of SDL (Simple DirectMedia Layer)
    // other than through the Archon display library, and that there is also no direct or
    // indirect use of anything that would conflict with use of SDL.
    guarantees.no_other_use_of_sdl = true;

    if (list_display_implementations) {
        display::list_implementations(core::File::get_stdout(), locale, guarantees); // Throws
        return EXIT_SUCCESS;
    }

    log::FileLogger root_logger(core::File::get_stdout(), locale); // Throws
    log::LimitLogger logger(root_logger, log_level_limit); // Throws

    const display::Implementation* impl = {};
    std::string error;
    if (ARCHON_UNLIKELY(!display::try_pick_implementation(optional_display_implementation, guarantees,
                                                          impl, error))) { // Throws
        logger.error("Failed to pick display implementation: %s", error); // Throws
        return EXIT_FAILURE;
    }
    logger.detail("Display implementation: %s", impl->get_slot().get_ident()); // Throws

    log::PrefixLogger display_logger(logger, "Display: "); // Throws
    display::Connection::Config connection_config;
    connection_config.logger = &display_logger;
    connection_config.x11.display = optional_x11_display;
    std::unique_ptr<display::Connection> conn;
    if (ARCHON_UNLIKELY(!impl->try_new_connection(locale, connection_config, conn, error))) { // Throws
        logger.error("Failed to open display connection: %s", error); // Throws
        return EXIT_FAILURE;
    }

    int screen;
    if (!optional_screen.has_value()) {
        screen = conn->get_default_screen();
    }
    else {
        int val = optional_screen.value();
        int num_screens = conn->get_num_screens();
        if (ARCHON_UNLIKELY(val < 0 || val >= num_screens)) {
            logger.error("Specified screen index (%s) is out of range", core::as_int(val)); // Throws
            return EXIT_FAILURE;
        }
        screen = val;
    }

#if ARCHON_RENDER_HAVE_OPENGL

    event_loop loop(locale, logger, *conn, screen);
    if (ARCHON_UNLIKELY(!loop.try_init(window_size))) // Throws
        return EXIT_FAILURE;
    loop.run(); // Throws

#else // !ARCHON_RENDER_HAVE_OPENGL

    static_cast<void>(screen);
    logger.error("No OpenGL support"); // Throws
    return EXIT_FAILURE;

#endif // !ARCHON_RENDER_HAVE_OPENGL
}
