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


#include <cstdlib>
#include <tuple>
#include <optional>
#include <locale>
#include <filesystem>
#include <chrono>
#include <thread>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/quote.hpp>
#include <archon/core/file.hpp>
#include <archon/log.hpp>
#include <archon/cli.hpp>
#include <archon/image.hpp>
#include <archon/display.hpp>


using namespace archon;


namespace {


class Context
    : public display::EventHandler {
public:
    Context(display::Connection& conn) noexcept
        : m_conn(conn)
    {
    }

    void process_events()
    {
        bool quit = false;
        do {
            m_conn.wait(); // Throws
            m_conn.process_events(quit); // Throws
        }
        while (!quit);
    }

    bool on_keydown(const display::KeyEvent& ev) override final
    {
        if (ARCHON_UNLIKELY(ev.key_sym == display::key_Escape))
            return false;
        return true;
    }

private:
    display::Connection& m_conn;
};


} // unnamed namespace



int main(int argc, char* argv[])
{
    std::locale locale(""); // Throws

    namespace fs = std::filesystem;
    fs::path path;
    bool list_display_implementations = false;
    log::LogLevel log_level_limit = log::LogLevel::warn;
    std::optional<std::string> optional_display_implementation;

    cli::Spec spec;
    pat("<path>", cli::no_attributes, spec,
        "Lorem ipsum.",
        std::tie(path)); // Throws

    pat("--list-display-implementations", cli::no_attributes, spec,
        "Lorem ipsum.",
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
        "are available. It is possible that no implem,entations are available. By default, if any implementations are "
        "available, the one, that is listed first by `--list-display-implementations`, is used.",
        cli::assign(optional_display_implementation)); // Throws

    int exit_status = 0;
    if (ARCHON_UNLIKELY(cli::process(argc, argv, spec, exit_status, locale))) // Throws
        return exit_status;

    if (list_display_implementations) {
        log::FileLogger stdout_logger(core::File::get_cout(), locale); // Throws
        int n = display::get_num_implementations();
        for (int i = 0; i < n; ++i) {
            const display::Implementation& impl = display::get_implementation(i); // Throws
            stdout_logger.info("%s", impl.ident()); // Throws
        }
        return EXIT_SUCCESS;
    }

    log::FileLogger root_logger(core::File::get_cerr(), locale); // Throws
    log::LimitLogger logger(root_logger, log_level_limit); // Throws

    // Promise that this application does not use the SDL library (Simple DirectMedia Layer)                                                                 
    // in any way other than through the Archon Display Library, neither directly nor
    // indirectly. This makes the SDL-based display implementation available.
    display::ExclusiveSDLMandate exclusive_sdl_mandate;

    display::Implementation::Mandates mandates;
    mandates.exclusive_sdl_mandate = &exclusive_sdl_mandate;

    const display::Implementation* impl;
    if (optional_display_implementation.has_value()) {
        std::string_view ident = optional_display_implementation.value();
        impl = archon::display::lookup_implementation(ident);
        if (ARCHON_UNLIKELY(!impl)) {
            logger.error("Unknown display implementation (%s)", core::quoted(ident)); // Throws
            return EXIT_FAILURE;
        }
        if (ARCHON_UNLIKELY(!impl->is_available(mandates))) {
            logger.error("Unavailable display implementation (%s)", core::quoted(ident)); // Throws
            return EXIT_FAILURE;
        }
    }
    else {
        impl = archon::display::get_default_implementation(mandates);
        if (ARCHON_UNLIKELY(!impl)) {
            logger.error("No display implementations are available"); // Throws
            return EXIT_FAILURE;
        }
    }
    std::unique_ptr<display::Connection> conn = impl->new_connection(locale, mandates); // Throws
    ARCHON_ASSERT(conn);
    logger.detail("Num screens:                      %s", conn->get_num_screens()); // Throws
    logger.detail("Default screen bounds:            %s", conn->get_screen_bounds()); // Throws
    logger.detail("Default screen resolution (ppcm): %s", conn->get_screen_resolution()); // Throws
    logger.detail("Number of default screen visuals: %s", conn->get_num_screen_visuals()); // Throws

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

    Context context(*conn);
    image::Size size = img->get_size();
    std::unique_ptr<display::Window> win = conn->new_window("Archon Image Viewer", size, context); // Throws

    std::unique_ptr<display::Texture> tex = win->new_texture(size); // Throws
    tex->put_image(*img); // Throws
    win->show(); // Throws
    win->put_texture(*tex); // Throws
    win->present(); // Throws

    context.process_events(); // Throws
}
