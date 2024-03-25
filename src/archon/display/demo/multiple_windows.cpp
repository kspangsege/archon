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


#include <cstdint>
#include <cstdlib>
#include <utility>
#include <memory>
#include <optional>
#include <string_view>
#include <string>
#include <map>
#include <locale>

#include <archon/core/features.h>
#include <archon/core/integer.hpp>
#include <archon/core/math.hpp>
#include <archon/core/format.hpp>
#include <archon/core/value_parser.hpp>
#include <archon/core/as_int.hpp>
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
        : m_conn(conn)
        , m_display(display)
    {
    }

    void add_window()
    {
        int id = m_prev_window_id + 1;
        m_prev_window_id = id;
        std::string title = core::format("Window #%s", id); // Throws
        display::Window::Config config;
        config.display = m_display;
        config.cookie = id;
        config.resizable = true;
        std::unique_ptr<display::Window> win = m_conn.new_window(title, g_small, *this, config); // Throws
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
        if (ARCHON_LIKELY(m_conn.try_map_key_code_to_key(ev.key_code, key))) { // Throws
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
        if (ARCHON_LIKELY(m_conn.try_map_key_code_to_key(ev.key_code, key))) { // Throws
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
                case display::Key::small_s: {
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
                case display::Key::small_f: {
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
                case display::Key::small_o: {
                    add_window(); // Throws
                    break;
                }
                case display::Key::escape:
                case display::Key::small_q: {
                    if (m_windows.size() > 1) {
                        m_windows.erase(ev.cookie);
                        break;
                    }
                    return false; // Terminate
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

    bool on_close(const display::WindowEvent& ev) override final
    {
        if (m_windows.size() > 1) {
            m_windows.erase(ev.cookie);
            return true;
        }
        return false; // Terminate
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
    std::optional<int> optional_x11_visual_depth;
    std::optional<display::ConnectionConfigX11::VisualClass> optional_x11_visual_class;
    std::optional<std::uint_fast32_t> optional_x11_visual_type;
    bool x11_disable_double_buffering = false;
    bool x11_disable_glx_direct_rendering = false;
    bool x11_disable_detectable_autorepeat = false;
    bool x11_use_synchronous_mode = false;

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

    opt("-d, --x11-visual-depth", "<num>", cli::no_attributes, spec,
        "When using the X11-based display implementation, pick a visual of the specified depth (@A).",
        cli::assign(optional_x11_visual_depth)); // Throws

    opt("-c, --x11-visual-class", "<name>", cli::no_attributes, spec,
        "When using the X11-based display implementation, pick a visual of the specified class (@A). The class can be "
        "\"StaticGray\", \"GrayScale\", \"StaticColor\", \"PseudoColor\", \"TrueColor\", or \"DirectColor\".",
        cli::assign(optional_x11_visual_class)); // Throws

    opt("-V, --x11-visual-type", "<num>", cli::no_attributes, spec,
        "When using the X11-based display implementation, pick a visual of the specified type (@A). The type, also "
        "known as the visual ID, is a 32-bit unsigned integer that can be expressed in decimal, hexadecumal (with "
        "prefix '0x'), or octal (with prefix '0') form.",
        cli::exec([&](std::string_view str) {
            core::ValueParser parser(locale);
            std::uint_fast32_t type = {};
            if (ARCHON_LIKELY(parser.parse(str, core::as_flex_int(type)))) {
                if (ARCHON_LIKELY(type <= core::int_mask<std::uint_fast32_t>(32))) {
                    optional_x11_visual_type.emplace(type);
                    return true;
                }
            }
            return false;
        })); // Throws

    opt("-B, --x11-disable-double-buffering", "", cli::no_attributes, spec,
        "When using the X11-based display implementation, disable use of double buffering, even when the selected "
        "visual supports double buffering.",
        cli::raise_flag(x11_disable_double_buffering)); // Throws

    opt("-D, --x11-disable-glx-direct-rendering", "", cli::no_attributes, spec,
        "When using the X11-based display implementation, disable use of GLX direct rendering, even in cases where "
        "GLX direct rendering is possible.",
        cli::raise_flag(x11_disable_glx_direct_rendering)); // Throws

    opt("-A, --x11-disable-detectable-autorepeat", "", cli::no_attributes, spec,
        "When using the X11-based display implementation, do not turn on \"detectable auto-repeat\" mode, as it is "
        "offered by the X Keyboard Extension, even when it can be turned on. Instead, rely on the fall-back detection "
        "mechanism.",
        cli::raise_flag(x11_disable_detectable_autorepeat)); // Throws

    opt("-s, --x11-use-synchronous-mode", "", cli::no_attributes, spec,
        "When using the X11-based display implementation, turn on X11's synchronous mode. In this mode, buffering of "
        "X protocol requests is turned off, and the Xlib functions, that generate X requests, wait for a response "
        "from the server before they return. This is sometimes useful when debugging.",
        cli::raise_flag(x11_use_synchronous_mode)); // Throws

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

    log::PrefixLogger display_logger(logger, "Display: "); // Throws
    display::Connection::Config connection_config;
    connection_config.logger = &display_logger;
    connection_config.x11.visual_depth = optional_x11_visual_depth;
    connection_config.x11.visual_class = optional_x11_visual_class;
    connection_config.x11.visual_type = optional_x11_visual_type;
    connection_config.x11.disable_double_buffering = x11_disable_double_buffering;
    connection_config.x11.disable_glx_direct_rendering = x11_disable_glx_direct_rendering;
    connection_config.x11.disable_detectable_autorepeat = x11_disable_detectable_autorepeat;
    connection_config.x11.synchronous_mode = x11_use_synchronous_mode;
    std::unique_ptr<display::Connection> conn = impl->new_connection(locale, connection_config); // Throws

    int display = conn->get_default_display();
    EventLoop event_loop(*conn, display);

    int num_windows = 2;
    for (int i = 0; i < num_windows; ++i)
        event_loop.add_window(); // Throws

    event_loop.process_events(); // Throws
}
