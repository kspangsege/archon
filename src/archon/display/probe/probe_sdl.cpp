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


#include <cstddef>
#include <stdexcept>
#include <optional>
#include <array>
#include <string>
#include <thread>

#include <archon/core/features.h>
#include <archon/core/scope_exit.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/flat_map.hpp>
#include <archon/core/string.hpp>
#include <archon/core/locale.hpp>
#include <archon/core/unicode_bridge.hpp>
#include <archon/core/format.hpp>
#include <archon/core/as_int.hpp>
#include <archon/core/quote.hpp>
#include <archon/core/file.hpp>
#include <archon/log.hpp>
#include <archon/cli.hpp>
#include <archon/display/impl/config.h>
#include <archon/display/geometry.hpp>

#if ARCHON_DISPLAY_HAVE_SDL
#  define HAVE_SDL 1
#else
#  define HAVE_SDL 0
#endif

#if HAVE_SDL
#  if ARCHON_CLANG
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wold-style-cast"
#  endif
#  define SDL_MAIN_HANDLED
#  include <SDL.h>
#  if ARCHON_CLANG
#    pragma clang diagnostic pop
#  endif
#endif


using namespace archon;


#if HAVE_SDL


namespace {


auto get_sdl_error(std::string_view message) -> std::string
{
    using namespace std::literals;
    return core::concat(message, ": "sv, std::string_view(SDL_GetError())); // Throws
}


[[noreturn]] void throw_sdl_error(const char* message)
{
    std::string msg = get_sdl_error(message); // Throws
    throw std::runtime_error(std::move(msg));
}


auto pixel_format_name(Uint32 format) -> const char*
{
    switch (format) {
        case SDL_PIXELFORMAT_UNKNOWN:
            break;
        case SDL_PIXELFORMAT_INDEX1LSB:
            return "INDEX1LSB";
        case SDL_PIXELFORMAT_INDEX1MSB:
            return "INDEX1MSB";
        case SDL_PIXELFORMAT_INDEX4LSB:
            return "INDEX4LSB";
        case SDL_PIXELFORMAT_INDEX4MSB:
            return "INDEX4MSB";
        case SDL_PIXELFORMAT_INDEX8:
            return "INDEX8";
        case SDL_PIXELFORMAT_RGB332:
            return "RGB332";
        case SDL_PIXELFORMAT_RGB444:
            return "RGB444";
        case SDL_PIXELFORMAT_RGB555:
            return "RGB555";
        case SDL_PIXELFORMAT_BGR555:
            return "BGR555";
        case SDL_PIXELFORMAT_ARGB4444:
            return "ARGB4444";
        case SDL_PIXELFORMAT_RGBA4444:
            return "RGBA4444";
        case SDL_PIXELFORMAT_ABGR4444:
            return "ABGR4444";
        case SDL_PIXELFORMAT_BGRA4444:
            return "BGRA4444";
        case SDL_PIXELFORMAT_ARGB1555:
            return "ARGB1555";
        case SDL_PIXELFORMAT_RGBA5551:
            return "RGBA5551";
        case SDL_PIXELFORMAT_ABGR1555:
            return "ABGR1555";
        case SDL_PIXELFORMAT_BGRA5551:
            return "BGRA5551";
        case SDL_PIXELFORMAT_RGB565:
            return "RGB565";
        case SDL_PIXELFORMAT_BGR565:
            return "BGR565";
        case SDL_PIXELFORMAT_RGB24:
            return "RGB24";
        case SDL_PIXELFORMAT_BGR24:
            return "BGR24";
        case SDL_PIXELFORMAT_RGB888:
            return "RGB888";
        case SDL_PIXELFORMAT_RGBX8888:
            return "RGBX8888";
        case SDL_PIXELFORMAT_BGR888:
            return "BGR888";
        case SDL_PIXELFORMAT_BGRX8888:
            return "BGRX8888";
        case SDL_PIXELFORMAT_ARGB8888:
            return "ARGB8888";
        case SDL_PIXELFORMAT_RGBA8888:
            return "RGBA8888";
        case SDL_PIXELFORMAT_ABGR8888:
            return "ABGR8888";
        case SDL_PIXELFORMAT_BGRA8888:
            return "BGRA8888";
        case SDL_PIXELFORMAT_ARGB2101010:
            return "ARGB2101010";
        case SDL_PIXELFORMAT_YV12:
            return "YV12";
        case SDL_PIXELFORMAT_IYUV:
            return "IYUV";
        case SDL_PIXELFORMAT_YUY2:
            return "YUY2";
        case SDL_PIXELFORMAT_UYVY:
            return "UYVY";
        case SDL_PIXELFORMAT_YVYU:
            return "YVYU";
        case SDL_PIXELFORMAT_NV12:
            return "NV12";
        case SDL_PIXELFORMAT_NV21:
            return "NV21";
    }
    ARCHON_ASSERT_UNREACHABLE();
    return nullptr;
}


void show_renderer_info(const SDL_RendererInfo& info, log::Logger& logger)
{
    logger.info("  Name: %s", core::quoted(info.name)); // Throws
    logger.info("  Flags:"); // Throws
    if ((info.flags & SDL_RENDERER_SOFTWARE) != 0)
        logger.info("    SOFTWARE"); // Throws
    if ((info.flags & SDL_RENDERER_ACCELERATED) != 0)
        logger.info("    ACCELERATED"); // Throws
    if ((info.flags & SDL_RENDERER_PRESENTVSYNC) != 0)
        logger.info("    PRESENTVSYNC"); // Throws
    if ((info.flags & SDL_RENDERER_TARGETTEXTURE) != 0)
        logger.info("    TARGETTEXTURE"); // Throws
    logger.info("  Pixel formats:"); // Throws
    for (int i = 0; i < int(info.num_texture_formats); ++i)
        logger.info("    %s", pixel_format_name(info.texture_formats[i])); // Throws
    logger.info("  Max texture size: %s", display::Size(info.max_texture_width, info.max_texture_height)); // Throws
}


} // unnamed namespace


int main(int argc, char* argv[])
{
    std::locale locale = core::get_default_locale();

    int num_windows = 0;
    log::LogLevel log_level_limit = log::LogLevel::warn;
    bool report_mouse_move = false;
    std::optional<std::string> optional_window_title;

    cli::Spec spec;
    opt(cli::help_tag, spec); // Throws
    opt(cli::stop_tag, spec); // Throws

    opt("-n, --num-windows", "<num>", cli::no_attributes, spec,
        "The number of windows to be opened. The default number is @V.",
        std::tie(num_windows)); // Throws

    opt("-l, --log-level", "<level>", cli::no_attributes, spec,
        "Set the log level limit. The possible levels are @G. The default limit is @Q.",
        std::tie(log_level_limit)); // Throws

    opt("-m, --report-mouse-move", "", cli::no_attributes, spec,
        "Turn on reporting of \"mouse move\" events.",
        cli::raise_flag(report_mouse_move)); // Throws

    opt("-T, --window-title", "<string>", cli::no_attributes, spec,
        "Set an alternate text to be used as window title.",
        cli::assign(optional_window_title)); // Throws

    int exit_status = 0;
    if (ARCHON_UNLIKELY(cli::process(argc, argv, spec, exit_status, locale))) // Throws
        return exit_status;

    log::FileLogger root_logger(core::File::get_cout(), locale); // Throws
    log::LimitLogger logger(root_logger, log_level_limit); // Throws

    SDL_SetMainReady();
    if (ARCHON_UNLIKELY(!SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1")))
        throw std::runtime_error("Failed to set SDL hint " SDL_HINT_NO_SIGNAL_HANDLERS);
    if (ARCHON_UNLIKELY(!SDL_SetHint(SDL_HINT_QUIT_ON_LAST_WINDOW_CLOSE, "0")))
        throw std::runtime_error("Failed to set SDL hint " SDL_HINT_QUIT_ON_LAST_WINDOW_CLOSE);

    {
        int ret = SDL_Init(SDL_INIT_VIDEO);
        if (ret < 0)
            throw_sdl_error("SDL_Init() failed");
    }
    int num_drivers;
    {
        int ret = SDL_GetNumRenderDrivers();
        if (ret < 0)
            throw_sdl_error("SDL_GetNumRenderDrivers()");
        num_drivers = ret;
    }
    logger.info("num_drivers = %s", num_drivers);
    for (int i = 0; i < num_drivers; ++i) {
        SDL_RendererInfo info;
        int ret = SDL_GetRenderDriverInfo(i, &info);
        if (ret < 0)
            throw_sdl_error("SDL_GetRenderDriverInfo() failed");
        logger.info("Driver %s:", i);
        show_renderer_info(info, logger);
    }

    struct WindowSlot {
        int no = {};
        Uint32 window_id = {};
        SDL_Window* window = {};
        SDL_Renderer* renderer = {};
        display::Size size;
        bool redraw = false;
    };

    core::FlatMap<Uint32, WindowSlot> window_slots;

    auto try_get_window_slot = [&](Uint32 window_id, WindowSlot*& slot) {
        auto i = window_slots.find(window_id);
        if (ARCHON_LIKELY(i != window_slots.end())) {
            slot = &i->second;
            return true;
        }
        return false;
    };

    int prev_window_no = 0;
    std::size_t max_seen_window_slots = 0;
    auto open_window = [&] {
        int no = ++prev_window_no;

        std::string title_1;
        std::string_view title_2;
        if (optional_window_title.has_value()) {
            title_2 = optional_window_title.value();
        }
        else {
            title_1 = core::format(locale, "SDL Probe %s", core::as_int(no)); // Throws
            title_2 = title_1;
        }

        std::array<char, 128> seed_memory;
        core::Buffer buffer(seed_memory);
        {
            core::native_mb_to_utf8_transcoder transcoder(locale);
            std::size_t buffer_offset = 0;
            transcoder.transcode_l(title_2, buffer, buffer_offset); // Throws
            buffer.append_a('\0', buffer_offset); // Throws
        }
        const char* title_3 = buffer.data();

        SDL_Window* window = {};
        ARCHON_SCOPE_EXIT {
            if (ARCHON_UNLIKELY(window))
                SDL_DestroyWindow(window);
        };
        {
            Uint32 flags = SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE;
            window = SDL_CreateWindow(title_3, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 256, 256, flags);
            if (!window)
                throw_sdl_error("SDL_CreateWindow() failed");
        }
        Uint32 id = SDL_GetWindowID(window);
        if (ARCHON_UNLIKELY(id <= 0))
            throw_sdl_error("SDL_GetWindowID() failed"); // Throws
        WindowSlot& slot = window_slots[id];
        slot.no = no;
        slot.window_id = id;
        slot.window = window;
        window = {};
        {
            int driver_index = -1;
            Uint32 flags = 0;
            SDL_Renderer* renderer = SDL_CreateRenderer(slot.window, driver_index, flags);
            if (!renderer)
                throw_sdl_error("SDL_CreateRenderer() failed");
            slot.renderer = renderer;
        }
        // Due to bug in SDL (https://github.com/libsdl-org/SDL/issues/8805), the setting of
        // the minimum window size must come after the creation of the renderer.
        SDL_SetWindowMinimumSize(slot.window, 128, 128);
        {
            SDL_RendererInfo info;
            int ret = SDL_GetRendererInfo(slot.renderer, &info);
            if (ret < 0)
                throw_sdl_error("SDL_GetRenderInfo() failed");
            if (no == 1) {
                logger.info("Renderer:");
                show_renderer_info(info, logger);
            }
        }
        {
            Uint8 r = 255;
            Uint8 g = 0;
            Uint8 b = 0;
            Uint8 a = 255;
            int ret = SDL_SetRenderDrawColor(slot.renderer, r, g, b, a);
            if (ret < 0)
                throw_sdl_error("SDL_SetRenderDrawColor() failed");
        }
        if (window_slots.size() > max_seen_window_slots)
            max_seen_window_slots = window_slots.size();
        return slot.window;
    };

    for (int i = 0; i < num_windows; ++i)
        open_window();

    bool quit = window_slots.empty();
    auto close_window = [&](Uint32 window_id) noexcept {
        auto i = window_slots.find(window_id);
        ARCHON_ASSERT(i != window_slots.end());
        const WindowSlot& slot = i->second;
        if (slot.renderer)
            SDL_DestroyRenderer(slot.renderer);
        SDL_DestroyWindow(slot.window);
        window_slots.erase(window_id);
        if (window_slots.empty())
            quit = true;
    };

    auto log = [&](int window_no, std::string_view message, const auto&... args) {
        if (max_seen_window_slots < 2) {
            logger.info(message, args...); // Throws
        }
        else {
            logger.info("WINDOW %s: %s", window_no, core::formatted(message, args...)); // Throws
        }
    };

    for (const auto& entry : window_slots) {
        const WindowSlot& slot = entry.second;
        SDL_ShowWindow(slot.window);
    }

    // Event loop
    while (!quit) {
        {
            int ret = SDL_WaitEvent(nullptr);
            if (ARCHON_UNLIKELY(ret != 1)) {
                ARCHON_ASSERT(ret == 0);
                throw_sdl_error("SDL_WaitEvent() failed");
            }
        }

        while (!quit) {
            SDL_Event event = {};
            int ret = SDL_PollEvent(&event);
            if (ARCHON_UNLIKELY(ret != 1)) {
                ARCHON_ASSERT(ret == 0);
                break;
            }

            WindowSlot* slot = {};
            switch (event.type) {
                case SDL_MOUSEMOTION:
                    if (ARCHON_LIKELY(event.motion.state == 0))
                        break;
                    if (ARCHON_LIKELY(try_get_window_slot(event.motion.windowID, slot))) {
                        display::Pos pos = { event.motion.x, event.motion.y };
                        if (report_mouse_move)
                            log(slot->no, "MOUSE MOVE: %s", pos); // Throws
                    }
                    break;
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:
                    if (ARCHON_LIKELY(try_get_window_slot(event.button.windowID, slot))) {
                        log(slot->no, "%s: %s", (event.type == SDL_MOUSEBUTTONDOWN ? "MOUSE DOWN" : "MOUSE UP"),
                            core::promote(event.button.button)); // Throws
                    }
                    break;
                case SDL_KEYDOWN:
                case SDL_KEYUP:
                    if (ARCHON_LIKELY(try_get_window_slot(event.key.windowID, slot))) {
                        SDL_Keycode keysym = event.key.keysym.sym;
                        const char* key = "?";
                        if (ARCHON_LIKELY(core::assume_utf8_locale(locale))) // Throws
                            key = SDL_GetKeyName(keysym); // Throws
                        log(slot->no, "%s: %s, %s, %s -> %s", (event.type == SDL_KEYDOWN ? "KEY DOWN" : "KEY UP"), key,
                            core::as_int(event.key.repeat), core::as_int(int(event.key.keysym.scancode)),
                            core::as_int(keysym)); // Throws
                        if (event.type == SDL_KEYDOWN && (keysym == SDLK_ESCAPE || keysym == SDLK_q)) {
                            close_window(slot->window_id);
                            break;
                        }
                        if (event.type == SDL_KEYUP && keysym == SDLK_n) {
                            SDL_Window* window = open_window(); // Throws
                            SDL_ShowWindow(window);
                            break;
                        }
                    }
                    break;
                case SDL_WINDOWEVENT:
                    if (ARCHON_LIKELY(try_get_window_slot(event.window.windowID, slot))) {
                        switch (event.window.event) {
                            case SDL_WINDOWEVENT_MOVED:
                                log(slot->no, "POS: %s", display::Pos(int(event.window.data1),
                                                                      int(event.window.data2))); // Throws
                                break;
                            case SDL_WINDOWEVENT_EXPOSED:
                                slot->redraw = true;
                                break;
                            case SDL_WINDOWEVENT_ENTER:
                            case SDL_WINDOWEVENT_LEAVE:
                                log(slot->no, (event.window.event == SDL_WINDOWEVENT_ENTER ? "MOUSE OVER" :
                                               "MOUSE OUT")); // Throws
                                break;
                            case SDL_WINDOWEVENT_FOCUS_GAINED:
                            case SDL_WINDOWEVENT_FOCUS_LOST:
                                log(slot->no, (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED ? "FOCUS" :
                                               "BLUR")); // Throws
                                break;
                            case SDL_WINDOWEVENT_CLOSE:
                                close_window(slot->window_id);
                                break;
                        }
                    }
                    break;
                case SDL_QUIT:
                    quit = true;
                    break;
            }
        }

        for (const auto& entry : window_slots) {
            const WindowSlot& slot = entry.second;
            if (slot.redraw) {
                int ret = SDL_RenderClear(slot.renderer);
                if (ret < 0)
                    throw_sdl_error("SDL_RenderClear() failed");
                SDL_RenderPresent(slot.renderer);
            }
        }
    }
    while (!window_slots.empty()) {
        const WindowSlot& slot = window_slots.begin()->second;
        close_window(slot.window_id);
    }
    SDL_Quit();
}

#else // !HAVE_SDL


int main()
{
    throw std::runtime_error("No SDL support");
}

#endif // !HAVE_SDL
