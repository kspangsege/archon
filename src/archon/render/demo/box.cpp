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
#include <memory>
#include <optional>
#include <tuple>
#include <string_view>
#include <string>
#include <locale>
#include <filesystem>

#include <archon/core/features.h>
#include <archon/core/integer.hpp>
#include <archon/core/math.hpp>
#include <archon/core/locale.hpp>
#include <archon/core/value_parser.hpp>
#include <archon/core/as_int.hpp>
#include <archon/core/file.hpp>
#include <archon/log.hpp>
#include <archon/cli.hpp>
#include <archon/math/vector.hpp>
#include <archon/math/rotation.hpp>
#include <archon/display.hpp>
#include <archon/display/connection_config_x11.hpp>
#include <archon/render/opengl.hpp>
#include <archon/render/engine.hpp>


using namespace archon;


namespace {

class BallScene final
    : public render::Engine::Scene {
public:
    void init() override;
    void render() override;
};


void BallScene::init()
{
#if ARCHON_RENDER_HAVE_OPENGL

    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);

#ifdef GL_LIGHT_MODEL_COLOR_CONTROL
    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
#endif
#ifdef GL_LIGHT_MODEL_LOCAL_VIEWER
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
#endif

#endif // ARCHON_RENDER_HAVE_OPENGL
}


void BallScene::render()
{
#if ARCHON_RENDER_HAVE_OPENGL

    float scale_factor = 0.5;
    math::Vector3F a = scale_factor * math::Vector3F(-1, -1, -1);
    math::Vector3F b = scale_factor * math::Vector3F(+1, +1, +1);

    glBegin(GL_QUADS);

    // Left side of box
    glNormal3f(-1, 0, 0);
    glVertex3f(a[0], a[1], a[2]);
    glVertex3f(a[0], a[1], b[2]);
    glVertex3f(a[0], b[1], b[2]);
    glVertex3f(a[0], b[1], a[2]);

    // Right side of box
    glNormal3f(+1, 0, 0);
    glVertex3f(b[0], a[1], a[2]);
    glVertex3f(b[0], b[1], a[2]);
    glVertex3f(b[0], b[1], b[2]);
    glVertex3f(b[0], a[1], b[2]);

    // Bottom of box
    glNormal3f(0, -1, 0);
    glVertex3f(a[0], a[1], a[2]);
    glVertex3f(b[0], a[1], a[2]);
    glVertex3f(b[0], a[1], b[2]);
    glVertex3f(a[0], a[1], b[2]);

    // Top of box
    glNormal3f(0, +1, 0);
    glVertex3f(a[0], b[1], a[2]);
    glVertex3f(a[0], b[1], b[2]);
    glVertex3f(b[0], b[1], b[2]);
    glVertex3f(b[0], b[1], a[2]);

    // Back side of box
    glNormal3f(0, 0, -1);
    glVertex3f(a[0], a[1], a[2]);
    glVertex3f(a[0], b[1], a[2]);
    glVertex3f(b[0], b[1], a[2]);
    glVertex3f(b[0], a[1], a[2]);

    // Front side of box
    glNormal3f(0, 0, +1);
    glVertex3f(a[0], a[1], b[2]);
    glVertex3f(b[0], a[1], b[2]);
    glVertex3f(b[0], b[1], b[2]);
    glVertex3f(a[0], b[1], b[2]);

    glEnd();

#endif // ARCHON_RENDER_HAVE_OPENGL
}


} // unnamed namespace


int main(int argc, char* argv[])
{
    std::locale locale = core::get_default_locale(); // Throws

    namespace fs = std::filesystem;
    bool list_display_implementations = false;
    render::Engine::Config engine_config;
    display::Size window_size = 512;
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

    opt("-r, --frame-rate", "<rate>", cli::no_attributes, spec,
        "The initial frame rate. The frame rate marks the upper limit of number of frames per second. The default "
        "rate is @V.",
        cli::assign(engine_config.frame_rate)); // Throws

    opt("-S, --window-size", "<size>", cli::no_attributes, spec,
        "Set the window size in number of pixels. \"@A\" can be specified either as a pair \"<width>,<height>\", or "
        "as a single value, which is then used as both width and height. The default size is @V.",
        cli::assign(window_size)); // Throws

    opt("-f, --fullscreen", "", cli::no_attributes, spec,
        "Open window in fullscreen mode.",
        cli::raise_flag(engine_config.fullscreen_mode)); // Throws

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
    // other than through the Archon Display Library, and that there is also no direct or
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

    const display::Implementation* impl = {};
    std::string error;
    if (ARCHON_UNLIKELY(!display::try_pick_implementation(optional_display_implementation, guarantees,
                                                          impl, error))) { // Throws
        logger.error("Failed to pick display implementation: %s", error); // Throws
        return EXIT_FAILURE;
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

    engine_config.screen = screen;
    engine_config.allow_window_resize = true;
    engine_config.logger = &logger;

    render::Engine engine;
    if (ARCHON_UNLIKELY(!engine.try_create(*conn, "Archon Box", window_size, locale, engine_config,
                                           error))) { // Throws
        logger.error("Failed to create render engine: %s", error); // Throws
        return EXIT_FAILURE;
    }

    BallScene ball_scene;
    engine.set_scene(ball_scene);
    engine.set_base_spin(math::Rotation({ 0, 1, 0 }, core::deg_to_rad(90))); // Throws

    engine.bind_key(display::Key::small_s, "Spin", [&](bool down) {
        if (down) {
            engine.set_spin(math::Rotation({ 0, 1, 0 }, core::deg_to_rad(90))); // Throws
        }
        else {
            engine.set_spin(math::Rotation({ 0, 1, 0 }, core::deg_to_rad(0))); // Throws
        }
    }); // Throws

    engine.run(); // Throws
}
