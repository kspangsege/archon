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


#include <cstdlib>
#include <memory>
#include <chrono>
#include <optional>
#include <tuple>
#include <string>
#include <locale>
#include <stdexcept>

#include <archon/core/features.h>
#include <archon/core/locale.hpp>
#include <archon/core/file.hpp>
#include <archon/log.hpp>
#include <archon/cli.hpp>
#include <archon/display.hpp>
#include <archon/render/opengl.hpp>


using namespace archon;


#if ARCHON_RENDER_HAVE_OPENGL


namespace {


class EventLoop final
    : public display::WindowEventHandler {
public:
    EventLoop(display::Connection& conn, display::Window& win) noexcept
        : m_conn(conn)
        , m_win(win)
    {
    }

    void render_frame();

    void run()
    {
        using clock_type      = display::Connection::clock_type;
        using duration_type   = clock_type::duration;
        using time_point_type = display::Connection::time_point_type;

        duration_type time_per_frame = std::chrono::milliseconds(10);

        time_point_type deadline = clock_type::now();
        do {
            deadline += time_per_frame;
            time_point_type now = clock_type::now();
            if (ARCHON_UNLIKELY(deadline < now))
                deadline = now;

            render_frame(); // Throws
            m_win.opengl_swap_buffers(); // Throws
        }
        while (m_conn.process_events_a(deadline)); // Throws
    }

    bool on_keydown(const display::KeyEvent& ev) override
    {
        display::Key key = {};
        if (ARCHON_LIKELY(m_conn.try_map_key_code_to_key(ev.key_code, key))) { // Throws
            if (ARCHON_UNLIKELY(key == display::Key::escape))
                return false;
        }
        return true;
    }

private:
    display::Connection& m_conn;
    display::Window& m_win;
    double m_angle = 0;
};


void EventLoop::render_frame()
{
    double view_plane_dist  = 1;
    double view_plane_right = 1;
    double view_plane_top   = 1;
    double far_clip_dist    = 100;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-view_plane_right, view_plane_right, -view_plane_top, view_plane_top, view_plane_dist, far_clip_dist);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    double camera_dist = 10;

    glTranslated(0, 0, -camera_dist);
    glRotated(m_angle, 0, 0, -1);
    m_angle += 5;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render
    glColor3f(1, 1, 1);
    glBegin(GL_QUADS);
    glVertex3f(-5, -5, 0);
    glVertex3f(+5, -5, 0);
    glVertex3f(+5, +5, 0);
    glVertex3f(-5, +5, 0);
    glEnd();
}


} // unnamed namespace


#endif // ARCHON_RENDER_HAVE_OPENGL


int main(int argc, char* argv[])
{
    std::locale locale = core::get_default_locale(); // Throws

    bool list_display_implementations = false;
    log::LogLevel log_level_limit = log::LogLevel::info;
    std::optional<std::string> optional_display_implementation;

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

    opt("-l, --log-level", "<level>", cli::no_attributes, spec,
        "Set the log level limit. The possible levels are @G. The default limit is @Q.",
        cli::assign(log_level_limit)); // Throws

    opt("-i, --display-implementation", "<ident>", cli::no_attributes, spec,
        "Use the specified display implementation. Use `--list-display-implementations` to see which implementations "
        "are available. It is possible that no implementations are available. By default, if any implementations are "
        "available, the one, that is listed first by `--list-display-implementations`, is used.",
        cli::assign(optional_display_implementation)); // Throws

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
    std::unique_ptr<display::Connection> conn;
    if (ARCHON_UNLIKELY(!impl->try_new_connection(locale, connection_config, conn, error))) { // Throws
        logger.error("Failed to open display connection: %s", error); // Throws
        return EXIT_FAILURE;
    }

    display::Window::Config config;
    config.enable_opengl_rendering = true;
    std::unique_ptr<display::Window> win;
    if (ARCHON_UNLIKELY(!conn->try_new_window("Probe OpenGL", 512, config, win, error))) { // Throws
        logger.error("Failed to create window: %s", error); // Throws
        return EXIT_FAILURE;
    }

#if ARCHON_RENDER_HAVE_OPENGL

    win->opengl_make_current(); // Throws

    GLenum err = glewInit();
    if (err != GLEW_OK) {
        const GLubyte* str = glewGetErrorString(err);
        logger.error("Failed to initialize GLEW: %s", str); // Throws
        return EXIT_FAILURE;
    }

    logger.info("OpenGL Vendor: %s", glGetString(GL_VENDOR)); // Throws
    logger.info("OpenGL Renderer: %s", glGetString(GL_RENDERER)); // Throws
    logger.info("OpenGL Version: %s", glGetString(GL_VERSION)); // Throws
    if (const GLubyte* str = glGetString(GL_SHADING_LANGUAGE_VERSION))
        logger.info("GLSL Version: %s", str); // Throws
    logger.info("GLEW Version: %s", glewGetString(GLEW_VERSION)); // Throws

    EventLoop event_loop(*conn, *win);
    win->set_event_handler(event_loop); // throws
    win->show(); // Throws
    event_loop.run(); // Throws

#else // !ARCHON_RENDER_HAVE_OPENGL

    logger.error("No OpenGL support"); // Throws
    return EXIT_FAILURE;

#endif // !ARCHON_RENDER_HAVE_OPENGL
}
