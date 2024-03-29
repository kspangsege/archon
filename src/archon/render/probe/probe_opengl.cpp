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


#include <memory>
#include <stdexcept>
#include <chrono>
#include <locale>

#include <archon/core/features.h>
#include <archon/display.hpp>
#include <archon/render/impl/config.h>

#if ARCHON_RENDER_HAVE_OPENGL
#  if ARCHON_APPLE
#    define GL_SILENCE_DEPRECATION
#    include <OpenGL/gl.h>
#  elif ARCHON_WINDOWS
#    define NOMINMAX
#    include <windows.h>
#    include <gl/GL.h>
#  else
#    include <GL/gl.h>
#  endif
#endif // ARCHON_RENDER_HAVE_OPENGL


using namespace archon;


#if ARCHON_RENDER_HAVE_OPENGL


namespace {


class EventLoop
    : public display::WindowEventHandler {
public:
    EventLoop(display::Connection& conn) noexcept
        : m_impl(conn.get_implementation())
        , m_conn(conn)
    {
    }

    void render_frame();

    void run(display::Window& win)
    {
        win.opengl_make_current(); // Throws

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
            win.opengl_swap_buffers(); // Throws
        }
        while (m_conn.process_events(deadline)); // Throws
    }

    bool on_keydown(const display::KeyEvent& ev) override final
    {
        display::Key key = {};
        if (ARCHON_LIKELY(m_impl.try_map_key_code_to_key(ev.key_code, key))) { // Throws
            if (ARCHON_UNLIKELY(key == display::Key::escape))
                return false;
        }
        return true;
    }

private:
    const display::Implementation& m_impl;
    display::Connection& m_conn;
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


int main()
{
    display::Guarantees guarantees;

    // Promise to not open more than one display connection at a time.
    guarantees.only_one_connection = true;

    // Promise that all use of the display API happens on behalf of the main thread.
    guarantees.main_thread_exclusive = true;

    // Promise that there is no direct or indirect use of SDL (Simple DirectMedia Layer)
    // other than through the Archon display library, and that there is also no direct or
    // indirect use of anything that would conflict with use of SDL.
    guarantees.no_other_use_of_sdl = true;

    std::locale locale(""); // Throws
    std::unique_ptr<display::Connection> conn = display::new_connection(locale, guarantees); // Throws
    EventLoop event_loop(*conn);
    display::Window::Config config;
    config.enable_opengl = true;
    std::unique_ptr<display::Window> win = conn->new_window("Probe OpenGL", 512, event_loop, config); // Throws
    win->show(); // Throws
    event_loop.run(*win); // Throws
}


#else // !ARCHON_RENDER_HAVE_OPENGL


int main()
{
    throw std::runtime_error("No OpenGL support");
}


#endif // !ARCHON_RENDER_HAVE_OPENGL
