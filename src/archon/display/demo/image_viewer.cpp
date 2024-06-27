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


#include <cstdint>
#include <cstdlib>
#include <memory>
#include <tuple>
#include <optional>
#include <string_view>
#include <string>
#include <system_error>
#include <locale>
#include <filesystem>

#include <archon/core/features.h>
#include <archon/core/integer.hpp>
#include <archon/core/locale.hpp>
#include <archon/core/value_parser.hpp>
#include <archon/core/as_int.hpp>
#include <archon/core/quote.hpp>
#include <archon/core/file.hpp>
#include <archon/log.hpp>
#include <archon/cli.hpp>
#include <archon/image.hpp>
#include <archon/display.hpp>


using namespace archon;


namespace {


class EventLoop final
    : public display::WindowEventHandler {
public:
    EventLoop(display::Connection& conn, display::Window& win, const display::Texture& tex) noexcept
        : m_conn(conn)
        , m_win(win)
        , m_tex(tex)
    {
    }

    void process_events()
    {
        m_conn.process_events(); // Throws
    }

    bool on_keydown(const display::KeyEvent& ev) override
    {
        display::Key key = {};
        if (ARCHON_LIKELY(m_conn.try_map_key_code_to_key(ev.key_code, key))) { // Throws
            if (ARCHON_UNLIKELY(key == display::Key::escape || key == display::Key::small_q))
                return false; // Terminate
        }
        return true;
    }

    bool on_expose(const display::WindowEvent&) override
    {
        m_win.put_texture(m_tex); // Throws
        m_win.present(); // Throws
        return true;
    }

private:
    display::Connection& m_conn;
    display::Window& m_win;
    const display::Texture& m_tex;
};


} // unnamed namespace



int main(int argc, char* argv[])
{
    std::locale locale = core::get_default_locale(); // Throws

    namespace fs = std::filesystem;
    fs::path path;
    bool list_display_implementations = false;
    log::LogLevel log_level_limit = log::LogLevel::warn;
    std::optional<std::string> optional_display_implementation;
    std::optional<int> optional_screen;
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

    cli::Spec spec;
    pat("<path>", cli::no_attributes, spec,
        "Lorem ipsum.",
        std::tie(path)); // Throws

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

    opt("-s, --screen", "<number>", cli::no_attributes, spec,
        "Target the specified screen (@A). This is an index between zero and the number of screens minus one. If this "
        "option is not specified, the default screen of the display will be targeted.",
        cli::assign(optional_screen)); // Throws

    opt("-D, --x11-display", "<string>", cli::no_attributes, spec,
        "When using the X11-based display implementation, target the specified X11 display (@A). If this option is "
        "not specified, the value of the DISPLAY environment variable will be used.",
        cli::assign(optional_x11_display)); // Throws

    opt("-d, --x11-visual-depth", "<num>", cli::no_attributes, spec,
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

    std::unique_ptr<image::WritableImage> img;
    {
        image::LoadConfig load_config;
        log::PrefixLogger load_logger(logger, "Load: "); // Throws
        load_config.logger = &load_logger;
        std::error_code ec;
        if (!image::try_load(path, img, locale, load_config, ec)) { // Throws
            logger.error("Failed to load image: %s", ec.message()); // Throws
            return EXIT_FAILURE;
        }
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
    logger.detail("Display implementation: %s", impl->get_slot().ident()); // Throws

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
    std::unique_ptr<display::Connection> conn;
    std::string error;
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

    display::Size size = img->get_size();
    display::Window::Config window_config;
    window_config.screen = screen;
    std::unique_ptr<display::Window> win;
    if (ARCHON_UNLIKELY(!conn->try_new_window("Archon Image Viewer", size, window_config, win, error))) { // Throws
        logger.error("Failed to create window: %s", error); // Throws
        return EXIT_FAILURE;
    }

    std::unique_ptr<display::Texture> tex = win->new_texture(size); // Throws
    tex->put_image(*img); // Throws
    EventLoop event_loop(*conn, *win, *tex);
    win->set_event_handler(event_loop); // Throws
    win->show(); // Throws
    event_loop.process_events(); // Throws
}
