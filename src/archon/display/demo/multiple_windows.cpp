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
#include <utility>
#include <memory>
#include <optional>
#include <string_view>
#include <string>
#include <map>
#include <locale>

#include <archon/core/features.h>
#include <archon/core/math.hpp>
#include <archon/core/format.hpp>
#include <archon/core/quote.hpp>
#include <archon/core/file.hpp>
#include <archon/log.hpp>
#include <archon/cli.hpp>
#include <archon/math/vector.hpp>
#include <archon/util/color_space.hpp>
#include <archon/util/color.hpp>
#include <archon/display.hpp>


using namespace archon;


namespace {


display::Size g_small = 256;
display::Size g_large = { 512, 384 };


class EventLoop
    : public display::WindowEventHandler {
public:
    EventLoop(display::Connection& conn, int display) noexcept
        : m_impl(conn.get_implementation())
        , m_conn(conn)
        , m_display(display)
    {
    }

    void add_window()
    {
        int id = m_prev_window_id + 1;
        m_prev_window_id = id;
        std::string title = core::format("Window #%s", id); // Throws
        display::Window::Config config;
        config.cookie = id;
        config.resizable = true;
        std::unique_ptr<display::Window> win =
            m_conn.new_window(m_display, title, g_small, *this, std::move(config)); // Throws
        win->show(); // Throws
        math::Vector<3, double> hsv = { m_next_hue, 0.3, 0.5 };
        m_next_hue = core::periodic_mod(m_next_hue + core::golden_fraction<double>, 1.0);
        math::Vector rgb = util::cvt_HSV_to_sRGB(hsv);
        m_windows[id] = { std::move(win), util::Color::from_vec(rgb) }; // Throws
    }

    void process_events()
    {
        m_conn.process_events(); // Throws
    }

    bool on_keydown(const display::KeyEvent& ev) override final
    {
        display::Key key = {};
        if (ARCHON_LIKELY(m_impl.try_map_key_code_to_key(ev.key_code, key))) { // Throws
            switch (key) {
                case display::Key::digit_1: {
                    m_target_window = 1;
                    break;
                }
                case display::Key::digit_2: {
                    m_target_window = 2;
                    break;
                }
                case display::Key::digit_3: {
                    m_target_window = 3;
                    break;
                }
                default:
                    break;
            }
        }
        return true;
    }

    bool on_keyup(const display::KeyEvent& ev) override final
    {
        display::Key key = {};
        if (ARCHON_LIKELY(m_impl.try_map_key_code_to_key(ev.key_code, key))) { // Throws
            switch (key) {
                case display::Key::digit_1: {
                    if (m_target_window == 1)
                        m_target_window = 0;
                    break;
                }
                case display::Key::digit_2: {
                    if (m_target_window == 2)
                        m_target_window = 0;
                    break;
                }
                case display::Key::digit_3: {
                    if (m_target_window == 3)
                        m_target_window = 0;
                    break;
                }
                case display::Key::lower_case_s: {
                    int window_id = ev.cookie;
                    if (m_target_window != 0)
                        window_id = m_target_window;
                    auto i = m_windows.find(window_id);
                    if (ARCHON_LIKELY(i != m_windows.end())) {
                        WindowEntry& entry = i->second;
                        entry.large = !entry.large;
                        entry.window->set_size(entry.large ? g_large : g_small); // Throws
                    }
                    break;
                }
                case display::Key::lower_case_f: {
                    int window_id = ev.cookie;
                    if (m_target_window != 0)
                        window_id = m_target_window;
                    auto i = m_windows.find(window_id);
                    if (ARCHON_LIKELY(i != m_windows.end())) {
                        WindowEntry& entry = i->second;
                        entry.fullscreen = !entry.fullscreen;
                        entry.window->set_fullscreen_mode(entry.fullscreen); // Throws
                    }
                    break;
                }
                case display::Key::lower_case_o: {
                    add_window(); // Throws
                    break;
                }
                case display::Key::escape: {
                    m_windows.erase(ev.cookie);
                    break;
                }
                default:
                    break;
            }
        }
        return true;
    }

    bool on_expose(const display::WindowEvent& ev) override final
    {
        auto i = m_windows.find(ev.cookie);
        if (ARCHON_LIKELY(i != m_windows.end())) {
            WindowEntry& entry = i->second;
            entry.fill(); // Throws
        }
        return true;
    }

    bool on_close(const display::TimedWindowEvent& ev) override final
    {
        m_windows.erase(ev.cookie);
        return true;
    }

private:
    struct WindowEntry {
        std::unique_ptr<display::Window> window;
        util::Color color;
        bool large = false;
        bool fullscreen = false;

        void fill()
        {
            window->fill(color); // Throws
            window->present(); // Throws
        }
    };

    const display::Implementation& m_impl;
    display::Connection& m_conn;
    const int m_display;
    int m_prev_window_id = 0;
    double m_next_hue = 0;
    std::map<int, WindowEntry> m_windows;
    int m_target_window = 0;
};


} // unnamed namespace



int main(int argc, char* argv[])
{
    std::locale locale(""); // Throws

    bool list_display_implementations = false;
    log::LogLevel log_level_limit = log::LogLevel::warn;
    std::optional<std::string> optional_display_implementation;

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

    int exit_status = 0;
    if (ARCHON_UNLIKELY(cli::process(argc, argv, spec, exit_status, locale))) // Throws
        return exit_status;

    display::Guarantees guarantees;

    // Promise to not open more than one display connection at a time.
    guarantees.only_one_connection = true;

    // Promise that all use of the display API happens on behalf of the main thread.
    guarantees.main_thread_exclusive = true;

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

    std::unique_ptr<display::Connection> conn = impl->new_connection(locale); // Throws
    int display = conn->get_default_display();
    EventLoop event_loop(*conn, display);

    int num_windows = 2;
    for (int i = 0; i < num_windows; ++i)
        event_loop.add_window(); // Throws

    event_loop.process_events(); // Throws
}
