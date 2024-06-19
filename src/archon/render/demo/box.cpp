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


#include <utility>
#include <optional>
#include <tuple>
#include <string_view>
#include <string>
#include <locale>
#include <filesystem>

#include <archon/core/features.h>
#include <archon/core/math.hpp>
#include <archon/core/locale.hpp>
#include <archon/core/quote.hpp>
#include <archon/core/file.hpp>
#include <archon/log.hpp>
#include <archon/cli.hpp>
#include <archon/math/vector.hpp>
#include <archon/math/rotation.hpp>
#include <archon/display.hpp>
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

    opt("-s, --window-size", "<size>", cli::no_attributes, spec,
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

    const display::Implementation::Slot* display_implementation;
    if (optional_display_implementation.has_value()) {
        std::string_view ident = optional_display_implementation.value();
        const display::Implementation::Slot* slot = display::lookup_implementation(ident);
        if (ARCHON_UNLIKELY(!slot)) {
            logger.error("Unknown display implementation (%s)", core::quoted(ident)); // Throws
            return EXIT_FAILURE;
        }
        if (ARCHON_UNLIKELY(!slot->is_available(guarantees))) {
            logger.error("Unavailable display implementation (%s)", core::quoted(ident)); // Throws
            return EXIT_FAILURE;
        }
        display_implementation = slot;
    }
    else {
        const display::Implementation* impl = display::get_default_implementation_a(guarantees);
        if (ARCHON_UNLIKELY(!impl)) {
            logger.error("No display implementations are available"); // Throws
            return EXIT_FAILURE;
        }
        display_implementation = &impl->get_slot();
    }
    logger.detail("Display implementation: %s", display_implementation->ident()); // Throws

    engine_config.display_implementation = display_implementation;
    engine_config.display_guarantees = guarantees;
    engine_config.allow_window_resize = true;
    engine_config.logger = &logger;

    render::Engine engine("Archon Box", window_size, locale, engine_config); // Throws
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
