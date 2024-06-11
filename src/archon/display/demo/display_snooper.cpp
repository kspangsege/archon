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
#include <cstdint>
#include <cstdlib>
#include <utility>
#include <memory>
#include <optional>
#include <string_view>
#include <string>
#include <locale>
#include <system_error>
#include <filesystem>
#include <ostream>

#include <archon/core/features.h>
#include <archon/core/integer.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/locale.hpp>
#include <archon/core/seed_memory_output_stream.hpp>
#include <archon/core/value_parser.hpp>
#include <archon/core/as_int.hpp>
#include <archon/core/format_as.hpp>
#include <archon/core/quote.hpp>
#include <archon/core/filesystem.hpp>
#include <archon/core/build_environment.hpp>
#include <archon/core/file.hpp>
#include <archon/log.hpp>
#include <archon/cli.hpp>
#include <archon/util/color.hpp>
#include <archon/util/colors.hpp>
#include <archon/util/as_css_color.hpp>
#include <archon/image.hpp>
#include <archon/display.hpp>
#include <archon/display/connection_config_x11.hpp>


using namespace archon;


namespace {


struct Config {
    bool report_mouse_move = false;
    util::Color background_color = util::colors::white;
    display::Pos texture_pos = { 16, 16 };
};


class EventLoop
    : public display::WindowEventHandler
    , public display::ConnectionEventHandler {
public:
    EventLoop(display::Connection& conn, display::Window& win, const display::Texture& tex,
              log::Logger& logger, const Config& config) noexcept
        : m_conn(conn)
        , m_win(win)
        , m_tex(tex)
        , m_logger(logger)
        , m_config(config)
    {
    }

    void dump_display_conf(int display)
    {
        std::size_t num_screens = {};
        bool reliable = {};
        if (m_conn.try_get_display_conf(display, m_screens, m_strings, num_screens, reliable)) {
            std::array<char, 512> seed_memory;
            core::SeedMemoryOutputStream out(seed_memory); // Throws
            out.exceptions(std::ios_base::badbit | std::ios_base::failbit); // Throws
            out.imbue(out.getloc()); // Throws
            out << core::formatted("Display configuration changed (display_index=%s, num_screens=%s, reliable=%s)",
                                   display, num_screens, reliable); // Throws
            if (num_screens > 0)
                out << ":"; // Throws
            for (std::size_t i = 0; i < num_screens; ++i) {
                const display::Screen& screen = m_screens[i];
                auto format_monitor_name = [&](std::ostream& out) {
                    if (ARCHON_LIKELY(screen.monitor_name.has_value())) {
                        out << core::quoted(screen.monitor_name.value()); // Throws
                    }
                    else {
                        out << "unknown"; // Throws
                    }
                };
                out << "\n"; // Throws
                out << core::formatted("    Screen %s/%s: output_name=%s, bounds=%s, monitor_name=%s, resolution=%s, "
                                       "refresh_rate=%s", i + 1, num_screens, core::quoted(screen.output_name),
                                       screen.bounds, core::as_format_func(format_monitor_name),
                                       core::as_optional(screen.resolution, "unknown"),
                                       core::as_optional(screen.refresh_rate, "unknown"));
            }
            m_logger.info("%s", out.view()); // Throws
        }
    }

    void process_events()
    {
        m_conn.process_events(this); // Throws
    }

    bool on_keydown(const display::KeyEvent& ev) override final
    {
        display::Key key = {};
        bool have_key = m_conn.try_map_key_code_to_key(ev.key_code, key); // Throws
        auto format_key = [&](std::ostream& out) {
            out << display::as_key_name(ev.key_code, m_conn); // Throws;
            if (ARCHON_LIKELY(have_key))
                out << core::formatted(" (%s)", int(key)); // Throws
        };
        m_logger.info("KEY DOWN: %s", core::as_format_func(format_key)); // Throws
        if (ARCHON_LIKELY(have_key)) { // Throws
            if (ARCHON_UNLIKELY(key == display::Key::escape || key == display::Key::small_q))
                return false;
            if (ARCHON_UNLIKELY(key == display::Key::small_f)) {
                bool on = !fullscreen;
                m_win.set_fullscreen_mode(on); // Throws
                fullscreen = on;
            }
        }
        return true;
    }

    bool on_keyup(const display::KeyEvent& ev) override final
    {
        display::Key key = {};
        bool have_key = m_conn.try_map_key_code_to_key(ev.key_code, key); // Throws
        auto format_key = [&](std::ostream& out) {
            out << display::as_key_name(ev.key_code, m_conn); // Throws;
            if (ARCHON_LIKELY(have_key))
                out << core::formatted(" (%s)", int(key)); // Throws
        };
        m_logger.info("KEY UP: %s", core::as_format_func(format_key)); // Throws
        return true;
    }

    bool on_keyrepeat(const display::KeyEvent& ev) override final
    {
        display::Key key = {};
        bool have_key = m_conn.try_map_key_code_to_key(ev.key_code, key); // Throws
        auto format_key = [&](std::ostream& out) {
            out << display::as_key_name(ev.key_code, m_conn); // Throws;
            if (ARCHON_LIKELY(have_key))
                out << core::formatted(" (%s)", int(key)); // Throws
        };
        m_logger.info("KEY REPEAT: %s", core::as_format_func(format_key)); // Throws
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
        if (m_config.report_mouse_move)
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
        m_win.fill(m_config.background_color); // Throws
        m_win.put_texture(m_tex, m_config.texture_pos); // Throws
        m_win.present(); // Throws
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

    bool on_display_change(int display) override final
    {
        dump_display_conf(display); // Throws
        return true;
    }

    bool on_quit() override final
    {
        m_logger.info("QUIT");
        return false; // Terminate
    }

private:
    display::Connection& m_conn;
    display::Window& m_win;
    const display::Texture& m_tex;
    log::Logger& m_logger;
    const Config& m_config;

    core::Buffer<display::Screen> m_screens;
    core::Buffer<char> m_strings;
    bool fullscreen = false;
};


} // unnamed namespace



int main(int argc, char* argv[])
{
    std::locale locale = core::get_default_locale(); // Throws

    namespace fs = std::filesystem;
    std::optional<fs::path> optional_path;
    bool list_display_implementations = false;
    display::Size window_size = 256;
    std::optional<display::Size> optional_texture_size;
    Config config;
    log::LogLevel log_level_limit = log::LogLevel::info;
    std::optional<std::string> optional_display_implementation;
    std::optional<int> optional_display;
    std::optional<std::string> optional_x11_display;
    std::optional<int> optional_x11_visual_depth;
    std::optional<display::ConnectionConfigX11::VisualClass> optional_x11_visual_class;
    std::optional<std::uint_fast32_t> optional_x11_visual_type;
    bool x11_prefer_default_nondecomposed_colormap = false;
    bool x11_disable_double_buffering = false;
    bool x11_disable_glx_direct_rendering = false;
    bool x11_disable_detectable_autorepeat = false;
    bool x11_synchronous_mode = false;
    bool x11_install_colormaps = false;
    bool x11_colormap_weirdness = false;
    std::optional<std::string> optional_window_title;

    cli::Spec spec;
    pat("[<path>]", cli::no_attributes, spec,
        "Lorem ipsum.",
        std::tie(optional_path)); // Throws

    pat("--list-display-implementations", cli::no_attributes, spec,
        "List known display implementations.",
        [&] {
            list_display_implementations = true;
        }); // Throws

    opt(cli::help_tag, spec); // Throws
    opt(cli::stop_tag, spec); // Throws

    opt("-s, --window-size", "<size>", cli::no_attributes, spec,
        "Set the initial size of the window. \"@A\" can be specified either as a pair \"<width>,<height>\", or as a "
        "single number, which is then used as both width and height. The default window size is @V.",
        cli::assign(window_size)); // Throws

    opt("-b, --background-color", "<color>", cli::no_attributes, spec,
        "Set the text background color. \"@A\" can be any valid CSS3 color value with, or without an alpha component, "
        "as well as the extended hex-forms, \"#RGBA\" and \"#RRGGBBAA\", accommodating the alpha component. The "
        "default color is @Q.",
        cli::assign(util::as_css_color(config.background_color))); // Throws

    opt("-S, --texture-size", "<size>", cli::no_attributes, spec,
        "Set the size in pixels of the texture that is placed in the window. \"@A\" can be specified either as a pair "
        "\"<width>,<height>\", or as a single number, which is then used as both width and height. If no texture size "
        "is specified, it will be set equal to the size of the specified image, or default image if no image is "
        "specified. The size of the default image is 96.",
        cli::assign(optional_texture_size)); // Throws

    opt("-p, --texture-pos", "<position>", cli::no_attributes, spec,
        "Set position in pixels of upper left corner of of the texture that is placed in the window. The position is "
        "specified as a pair \"<x>,<y>\". The X and Y coordinates grow towards the right and downwards respectively. "
        "The default position is @V.",
        cli::assign(config.texture_pos)); // Throws

    opt("-l, --log-level", "<level>", cli::no_attributes, spec,
        "Set the log level limit. The possible levels are @G. The default limit is @Q.",
        cli::assign(log_level_limit)); // Throws

    opt("-i, --display-implementation", "<ident>", cli::no_attributes, spec,
        "Use the specified display implementation. Use `--list-display-implementations` to see which implementations "
        "are available. It is possible that no implementations are available. By default, if any implementations are "
        "available, the one, that is listed first by `--list-display-implementations`, is used.",
        cli::assign(optional_display_implementation)); // Throws

    opt("-d, --display", "<number>", cli::no_attributes, spec,
        "Target the specified display (@A). This is an index between zero and the number of displays minus one. If "
        "this option is not specified, the default display will be targeted.",
        cli::assign(optional_display)); // Throws

    opt("-m, --report-mouse-move", "", cli::no_attributes, spec,
        "Turn on reporting of \"mouse move\" events.",
        cli::raise_flag(config.report_mouse_move)); // Throws

    opt("-D, --x11-display", "<string>", cli::no_attributes, spec,
        "When using the X11-based display implementation, target the specified X11 display (@A). If this option is "
        "not specified, the value of the DISPLAY environment variable will be used.",
        cli::assign(optional_x11_display)); // Throws

    opt("-e, --x11-visual-depth", "<num>", cli::no_attributes, spec,
        "When using the X11-based display implementation, pick a visual of the specified depth (@A).",
        cli::assign(optional_x11_visual_depth)); // Throws

    opt("-c, --x11-visual-class", "<name>", cli::no_attributes, spec,
        "When using the X11-based display implementation, pick a visual of the specified class (@A). The class can be "
        "@F.",
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

    opt("-C, --x11-prefer-default-nondecomposed-colormap", "", cli::no_attributes, spec,
        "When using the X11-based display implementation, prefer the use of the default colormap when the default "
        "visual is used and is a PseudoColor or GrayScale visual. This succeeds if enough colors can be allocated. "
        "Otherwise a new colormap is created.",
        cli::raise_flag(x11_prefer_default_nondecomposed_colormap)); // Throws

    opt("-B, --x11-disable-double-buffering", "", cli::no_attributes, spec,
        "When using the X11-based display implementation, disable use of double buffering, even when the selected "
        "visual supports double buffering.",
        cli::raise_flag(x11_disable_double_buffering)); // Throws

    opt("-R, --x11-disable-glx-direct-rendering", "", cli::no_attributes, spec,
        "When using the X11-based display implementation, disable use of GLX direct rendering, even in cases where "
        "GLX direct rendering is possible.",
        cli::raise_flag(x11_disable_glx_direct_rendering)); // Throws

    opt("-A, --x11-disable-detectable-autorepeat", "", cli::no_attributes, spec,
        "When using the X11-based display implementation, do not turn on \"detectable auto-repeat\" mode, as it is "
        "offered by the X Keyboard Extension, even when it can be turned on. Instead, rely on the fall-back detection "
        "mechanism.",
        cli::raise_flag(x11_disable_detectable_autorepeat)); // Throws

    opt("-y, --x11-synchronous-mode", "", cli::no_attributes, spec,
        "When using the X11-based display implementation, turn on X11's synchronous mode. In this mode, buffering of "
        "X protocol requests is turned off, and the Xlib functions, that generate X requests, wait for a response "
        "from the server before they return. This is sometimes useful when debugging.",
        cli::raise_flag(x11_synchronous_mode)); // Throws

    opt("-I, --x11-install-colormaps", "", cli::no_attributes, spec,
        "When using the X11-based display implementation, install a window's colormap right after the creation of the "
        "window. This mode should only be enabled for debugging purposes, or when running against a server where "
        "there is no window manager.",
        cli::raise_flag(x11_install_colormaps)); // Throws

    opt("-W, --x11-colormap-weirdness", "", cli::no_attributes, spec,
        "When using the X11-based display implementation, introduce detectable weirdness into newly created "
        "colormaps.",
        cli::raise_flag(x11_colormap_weirdness)); // Throws

    opt("-T, --window-title", "<string>", cli::no_attributes, spec,
        "Set an alternate text to be used as window title.",
        cli::assign(optional_window_title)); // Throws

    int exit_status = 0;
    if (ARCHON_UNLIKELY(cli::process(argc, argv, spec, exit_status, locale))) // Throws
        return exit_status;

    log::FileLogger root_logger(core::File::get_cerr(), locale); // Throws
    log::LimitLogger logger(root_logger, log_level_limit); // Throws

    // `src_root` is the relative path to the root of the source tree from the root of the
    // project.
    //
    // `src_path` is the relative path to this source file from the root of source tree.
    //
    // `bin_path` is the relative path to the executable from the root of the source root as
    // it is reflected into the build directory.
    //
    core::BuildEnvironment::Params build_env_params;
    build_env_params.file_path = __FILE__;
    build_env_params.bin_path  = "archon/display/demo/archon-display-snooper";
    build_env_params.src_path  = "archon/display/demo/display_snooper.cpp";
    build_env_params.src_root  = "src";
    build_env_params.source_from_build_path = core::archon_source_from_build_path;
    core::BuildEnvironment build_env = core::BuildEnvironment(argv[0], build_env_params, locale); // Throws

    fs::path resource_path = (build_env.get_relative_source_root() /
                              core::make_fs_path_generic("archon/display/demo", locale)); // Throws

    // Load image
    std::unique_ptr<image::WritableImage> img;
    {
        fs::path path;
        if (optional_path.has_value()) {
            path = std::move(optional_path.value());
        }
        else {
            path = resource_path / core::make_fs_path_generic("display_snooper.png", locale); // Throws
        }
        image::LoadConfig load_config;
        log::PrefixLogger load_logger(logger, "Load: "); // Throws
        load_config.logger = &load_logger;
        std::error_code ec;
        if (!image::try_load(path, img, locale, load_config, ec)) { // Throws
            logger.error("%s: Failed to load image: %s", core::as_native_path(path), ec.message()); // Throws
            return EXIT_FAILURE;
        }
    }

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
    connection_config.x11.display = optional_x11_display;
    connection_config.x11.visual_depth = optional_x11_visual_depth;
    connection_config.x11.visual_class = optional_x11_visual_class;
    connection_config.x11.visual_type = optional_x11_visual_type;
    connection_config.x11.prefer_default_nondecomposed_colormap = x11_prefer_default_nondecomposed_colormap;
    connection_config.x11.disable_double_buffering = x11_disable_double_buffering;
    connection_config.x11.disable_glx_direct_rendering = x11_disable_glx_direct_rendering;
    connection_config.x11.disable_detectable_autorepeat = x11_disable_detectable_autorepeat;
    connection_config.x11.synchronous_mode = x11_synchronous_mode;
    connection_config.x11.install_colormaps = x11_install_colormaps;
    connection_config.x11.colormap_weirdness = x11_colormap_weirdness;
    std::unique_ptr<display::Connection> conn = impl->new_connection(locale, connection_config); // Throws
    int num_displays = conn->get_num_displays();
    int default_display = conn->get_default_display();
    logger.info("Display implementation: %s", impl->get_slot().ident()); // Throws
    logger.info("Number of displays:     %s", num_displays); // Throws
    logger.info("Default display:        %s", default_display); // Throws

    int display = default_display;
    if (optional_display.has_value()) {
        int val = optional_display.value();
        if (ARCHON_UNLIKELY(val < 0 || val >= num_displays)) {
            logger.error("Specified display index (%s) is out of range", core::as_int(val)); // Throws
            return EXIT_FAILURE;
        }
        display = val;
    }

    std::string_view window_title = "Archon Display Snooper";
    if (optional_window_title.has_value())
        window_title = optional_window_title.value();

    display::Window::Config window_config;
    window_config.display = display;
    window_config.resizable = true;
    window_config.minimum_size = 128;
    std::unique_ptr<display::Window> win =
        conn->new_window(window_title, window_size, window_config); // Throws

    display::Size texture_size;
    if (optional_texture_size.has_value()) {
        texture_size = optional_texture_size.value();
    }
    else {
        texture_size = img->get_size();
    }
    std::unique_ptr<display::Texture> tex = win->new_texture(texture_size); // Throws
    tex->put_image(*img); // Throws

    EventLoop event_loop(*conn, *win, *tex, logger, config);
    for (int i = 0; i < num_displays; ++i)
        event_loop.dump_display_conf(i); // Throws
    win->set_event_handler(event_loop); // Throws
    win->show(); // Throws
    event_loop.process_events(); // Throws
}
