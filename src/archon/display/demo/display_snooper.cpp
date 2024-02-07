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
#include <cstdlib>
#include <utility>
#include <memory>
#include <optional>
#include <string_view>
#include <string>
#include <locale>

#include <archon/core/features.h>
#include <archon/core/buffer.hpp>
#include <archon/core/format_as.hpp>
#include <archon/core/quote.hpp>
#include <archon/core/file.hpp>
#include <archon/log.hpp>
#include <archon/cli.hpp>
#include <archon/util/colors.hpp>
#include <archon/display.hpp>


using namespace archon;


namespace {


class EventLoop
    : public display::WindowEventHandler
    , public display::ConnectionEventHandler {
public:
    EventLoop(display::Connection& conn, log::Logger& logger) noexcept
        : m_conn(conn)
        , m_logger(logger)
    {
    }

    void set_window(std::unique_ptr<display::Window> win) noexcept
    {
        m_win = std::move(win);
    }

    void set_report_mouse_move(bool val) noexcept
    {
        m_report_mouse_move = val;
    }

    void process_events()
    {
        m_conn.process_events(this); // Throws
    }

    bool on_keydown(const display::KeyEvent& ev) override final
    {
        m_logger.info("KEY DOWN: %s", display::as_key_name (ev.key_code, m_conn)); // Throws
        display::Key key = {};
        if (ARCHON_LIKELY(m_conn.try_map_key_code_to_key(ev.key_code, key))) { // Throws
            if (ARCHON_UNLIKELY(key == display::Key::escape || key == display::Key::lower_case_q))
                return false;
        }
        return true;
    }

    bool on_keyup(const display::KeyEvent& ev) override final
    {
        m_logger.info("KEY UP: %s", display::as_key_name (ev.key_code, m_conn)); // Throws
        return true;
    }

    bool on_keyrepeat(const display::KeyEvent& ev) override final
    {
        m_logger.info("KEY REPEAT: %s", display::as_key_name (ev.key_code, m_conn)); // Throws
        return true;
    }

    bool on_mousedown(const display::MouseButtonEvent& ev) override final
    {
        m_logger.info("MOUSE DOWN: %s, (%s)", ev.button, ev.pos); // Throws
        return true;
    }

    bool on_mouseup(const display::MouseButtonEvent& ev) override final
    {
        m_logger.info("MOUSE UP: %s, (%s)", ev.button, ev.pos); // Throws
        return true;
    }

    bool on_mousemove(const display::MouseEvent& ev) override final
    {
        if (m_report_mouse_move)
            m_logger.info("MOUSE MOVE: %s", ev.pos); // Throws
        return true;
    }

    bool on_scroll(const display::ScrollEvent& ev) override final
    {
        m_logger.info("SCROLL: %s", ev.amount); // Throws
        return true;
    }

    bool on_mouseover(const display::TimedWindowEvent&) override final
    {
        m_logger.info("MOUSE OVER"); // Throws
        return true;
    }

    bool on_mouseout(const display::TimedWindowEvent&) override final
    {
        m_logger.info("MOUSE OUT"); // Throws
        return true;
    }

    bool on_focus(const display::WindowEvent&) override final
    {
        m_logger.info("FOCUS"); // Throws
        return true;
    }

    bool on_blur(const display::WindowEvent&) override final
    {
        m_logger.info("BLUR"); // Throws
        return true;
    }

    bool on_expose(const display::WindowEvent&) override final
    {
        m_logger.info("EXPOSE"); // Throws
        m_win->fill(util::colors::green); // Throws
        m_win->fill(util::colors::orange, display::Box({ 16, 16 }, 96)); // Throws
        m_win->present(); // Throws
        return true;
    }

    bool on_resize(const display::WindowSizeEvent& ev) override final
    {
        m_logger.info("SIZE: %s", ev.size); // Throws
        return true;
    }

    bool on_reposition(const display::WindowPosEvent& ev) override final
    {
        m_logger.info("POS: %s", ev.pos); // Throws
        return true;
    }

    bool on_close(const display::WindowEvent&) override final
    {
        m_logger.info("CLOSE"); // Throws
        return false; // Terminate
    }

    bool on_quit() override final
    {
        m_logger.info("QUIT");
        return false; // Terminate
    }

private:
    display::Connection& m_conn;
    log::Logger& m_logger;
    std::unique_ptr<display::Window> m_win;
    bool m_report_mouse_move = false;
};


} // unnamed namespace



int main(int argc, char* argv[])
{
    std::locale locale(""); // Throws

    bool list_display_implementations = false;
    log::LogLevel log_level_limit = log::LogLevel::info;
    std::optional<std::string> optional_display_implementation;
    bool disable_double_buffering = false;
    bool disable_detectable_autorepeat = false;
    bool report_mouse_move = false;
    bool use_synchronous_mode = false;

    cli::Spec spec;
    pat("", cli::no_attributes, spec,
        "Lorem ipsum.",
        cli::no_action); // Throws

    pat("--list-display-implementations", cli::no_attributes, spec,
        "List known display implementations.",
        [&] {
            list_display_implementations = true;
        }); // Throws

    opt(cli::help_tag, spec); // Throws
    opt(cli::stop_tag, spec); // Throws

    opt("-l, --log-level", "<level>", cli::no_attributes, spec,
        "Set the log level limit. The possible levels are \"off\", \"fatal\", \"error\", \"warn\", \"info\", "
        "\"detail\", \"debug\", \"trace\", and \"all\". The default limit is \"@V\".",
        cli::assign(log_level_limit)); // Throws

    opt("-i, --display-implementation", "<ident>", cli::no_attributes, spec,
        "Use the specified display implementation. Use `--list-display-implementations` to see which implementations "
        "are available. It is possible that no implementations are available. By default, if any implementations are "
        "available, the one, that is listed first by `--list-display-implementations`, is used.",
        cli::assign(optional_display_implementation)); // Throws

    opt("-D, --disable-double-buffering", "", cli::no_attributes, spec,
        "When using the X11-based display implementation, disable use of double buffering, even when the selected "
        "visual supports double buffering.",
        cli::raise_flag(disable_double_buffering)); // Throws

    opt("-A, --disable-detectable-autorepeat", "", cli::no_attributes, spec,
        "When using the X11-based display implementation, do not turn on \"detectable auto-repeat\" mode, as it is "
        "offered by the X Keyboard Extension, even when it can be turned on. Instead, rely on the fall-back detection "
        "mechanism.",
        cli::raise_flag(disable_detectable_autorepeat)); // Throws

    opt("-m, --report-mouse-move", "", cli::no_attributes, spec,
        "Turn on reporting of \"mouse move\" events.",
        cli::raise_flag(report_mouse_move)); // Throws

    opt("-s, --use-synchronous-mode", "", cli::no_attributes, spec,
        "When using the X11-based display implementation, turn on X11's synchronous mode. In this mode, buffering of "
        "X protocol requests is turned off, and the Xlib functions, that generate X requests, wait for a response "
        "from the server before they return. This is sometimes useful when debugging.",
        cli::raise_flag(use_synchronous_mode)); // Throws

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
        log::FileLogger stdout_logger(core::File::get_cout(), locale); // Throws
        int n = display::get_num_implementation_slots();
        for (int i = 0; i < n; ++i) {
            const display::Implementation::Slot& slot = display::get_implementation_slot(i); // Throws
            if (slot.is_available(guarantees)) {
                stdout_logger.info("%s", slot.ident()); // Throws
            }
            else {
                stdout_logger.info("%s (unavailable)", slot.ident()); // Throws
            }
        }
        return EXIT_SUCCESS;
    }

    log::FileLogger root_logger(core::File::get_cerr(), locale); // Throws
    log::LimitLogger logger(root_logger, log_level_limit); // Throws

    const display::Implementation* impl;
    if (optional_display_implementation.has_value()) {
        std::string_view ident = optional_display_implementation.value();
        const display::Implementation::Slot* slot = display::lookup_implementation(ident);
        if (ARCHON_UNLIKELY(!slot)) {
            logger.error("Unknown display implementation (%s)", core::quoted(ident)); // Throws
            return EXIT_FAILURE;
        }
        impl = slot->get_implementation_a(guarantees);
        if (ARCHON_UNLIKELY(!impl)) {
            logger.error("Unavailable display implementation (%s)", core::quoted(ident)); // Throws
            return EXIT_FAILURE;
        }
    }
    else {
        impl = display::get_default_implementation_a(guarantees);
        if (ARCHON_UNLIKELY(!impl)) {
            logger.error("No display implementations are available"); // Throws
            return EXIT_FAILURE;
        }
    }

    display::Connection::Config connection_config;
    connection_config.x11.disable_double_buffering = disable_double_buffering;
    connection_config.x11.disable_detectable_autorepeat = disable_detectable_autorepeat;
    connection_config.x11.synchronous_mode = use_synchronous_mode;
    std::unique_ptr<display::Connection> conn = impl->new_connection(locale, connection_config); // Throws
    int display = conn->get_default_display();
    logger.info("Number of displays: %s", conn->get_num_displays()); // Throws
    logger.info("Default display:    %s", display); // Throws
    {
        core::Buffer<display::Screen> screens;
        core::Buffer<char> strings;
        std::size_t num_screens = 0;
        if (conn->try_get_display_conf(display, screens, strings, num_screens)) {
            for (std::size_t i = 0; i < num_screens; ++i) {
                const display::Screen& screen = screens[i];
                logger.info("Screen %s/%s: output_name=%s, bounds=%s, resolution=%s, refresh_rate=%s", i + 1,
                            num_screens, core::quoted(screen.output_name), screen.bounds,
                            core::as_optional(screen.resolution, "unknown"),
                            core::as_optional(screen.refresh_rate, "unknown")); // Throws
            }
        }
    }

    EventLoop event_loop(*conn, logger);
    display::Size size = 256;
    display::Window::Config window_config;
    window_config.resizable = true;
    window_config.minimum_size = 128;
    std::unique_ptr<display::Window> win =
        conn->new_window(display, "Archon Display Snooper", size, event_loop, window_config); // Throws
    win->show(); // Throws
    event_loop.set_window(std::move(win));
    event_loop.set_report_mouse_move(report_mouse_move);
    event_loop.process_events(); // Throws
}
