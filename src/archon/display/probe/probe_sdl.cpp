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

#include <archon/core/features.hpp>
#include <archon/core/scope_exit.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/flat_map.hpp>
#include <archon/core/string.hpp>
#include <archon/core/locale.hpp>
#include <archon/core/charenc_bridge.hpp>
#include <archon/core/format.hpp>
#include <archon/core/as_int.hpp>
#include <archon/core/quote.hpp>
#include <archon/core/file.hpp>
#include <archon/log.hpp>
#include <archon/cli.hpp>
#include <archon/math/vector.hpp>
#include <archon/display/impl/config.h>
#include <archon/display/geometry.hpp>

#if ARCHON_DISPLAY_HAVE_SDL
#  define HAVE_SDL 1
#else
#  define HAVE_SDL 0
#endif

#if HAVE_SDL
#  include <SDL3/SDL.h>
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

    log::FileLogger root_logger(core::File::get_stdout(), locale); // Throws
    log::LimitLogger logger(root_logger, log_level_limit); // Throws

    if (ARCHON_UNLIKELY(!SDL_SetHint(SDL_HINT_QUIT_ON_LAST_WINDOW_CLOSE, "0")))
        throw std::runtime_error("Failed to set SDL hint " SDL_HINT_QUIT_ON_LAST_WINDOW_CLOSE);

    {
        bool success = SDL_Init(SDL_INIT_VIDEO);
        if (ARCHON_UNLIKELY(!success))
            throw_sdl_error("SDL_Init() failed");
    }
    int num_drivers = SDL_GetNumRenderDrivers();
    logger.info("num_drivers = %s", num_drivers);
    for (int i = 0; i < num_drivers; ++i) {
        const char* name = SDL_GetRenderDriver(i);
        logger.info("Driver %s: %s", i + 1, core::quoted(name)); // Throws
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
            core::charenc_bridge bridge(locale);
            std::size_t buffer_offset = 0;
            bridge.native_mb_to_utf8_l(title_2, buffer, buffer_offset); // Throws
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
            window = SDL_CreateWindow(title_3, 256, 256, flags);
            if (!window)
                throw_sdl_error("SDL_CreateWindow() failed");
        }
        bool success = SDL_SetWindowMinimumSize(window, 128, 128);
        if (ARCHON_UNLIKELY(!success))
            throw_sdl_error("SDL_SetWindowMinimumSize() failed");
        Uint32 id = SDL_GetWindowID(window);
        if (ARCHON_UNLIKELY(id == 0))
            throw_sdl_error("SDL_GetWindowID() failed");
        WindowSlot& slot = window_slots[id];
        slot.no = no;
        slot.window_id = id;
        slot.window = window;
        window = {};
        {
            const char* name = nullptr;
            SDL_Renderer* renderer = SDL_CreateRenderer(slot.window, name);
            if (!renderer)
                throw_sdl_error("SDL_CreateRenderer() failed");
            slot.renderer = renderer;
        }
        {
            const char* name = SDL_GetRendererName(slot.renderer);
            if (ARCHON_UNLIKELY(!name))
                throw_sdl_error("SDL_GetRenderName() failed");
            logger.info("Renderer: %s", core::quoted(name)); // Throws
        }
        {
            Uint8 r = 255;
            Uint8 g = 0;
            Uint8 b = 0;
            Uint8 a = 255;
            bool success = SDL_SetRenderDrawColor(slot.renderer, r, g, b, a);
            if (ARCHON_UNLIKELY(!success))
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
        bool success = SDL_ShowWindow(slot.window);
        if (ARCHON_UNLIKELY(!success))
            throw_sdl_error("SDL_ShowWindow() failed");
    }

    // Event loop
    while (!quit) {
        {
            bool success = SDL_WaitEvent(nullptr);
            if (ARCHON_UNLIKELY(!success))
                throw_sdl_error("SDL_WaitEvent() failed");
        }

        while (!quit) {
            SDL_Event event = {};
            bool success = SDL_PollEvent(&event);
            if (ARCHON_UNLIKELY(!success))
                break;

            WindowSlot* slot = {};
            switch (event.type) {
                case SDL_EVENT_MOUSE_MOTION:
                    if (ARCHON_LIKELY(event.motion.state == 0))
                        break;
                    if (ARCHON_LIKELY(try_get_window_slot(event.motion.windowID, slot))) {
                        math::Vector2F pos = { event.motion.x, event.motion.y };
                        if (report_mouse_move)
                            log(slot->no, "MOUSE MOVE: %s", pos); // Throws
                    }
                    break;
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                case SDL_EVENT_MOUSE_BUTTON_UP:
                    if (ARCHON_LIKELY(try_get_window_slot(event.button.windowID, slot))) {
                        const char* name = (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ? "MOUSE DOWN" : "MOUSE UP");
                        log(slot->no, "%s: %s", name, core::promote(event.button.button)); // Throws
                    }
                    break;
                case SDL_EVENT_KEY_DOWN:
                case SDL_EVENT_KEY_UP:
                    if (ARCHON_LIKELY(try_get_window_slot(event.key.windowID, slot))) {
                        SDL_Keycode keysym = event.key.key;
                        const char* key = "?";
                        if (ARCHON_LIKELY(core::assume_utf8_locale(locale))) // Throws
                            key = SDL_GetKeyName(keysym); // Throws
                        const char* name = (event.type == SDL_EVENT_KEY_DOWN ? "KEY DOWN" : "KEY UP");
                        log(slot->no, "%s: %s, %s, %s, %s -> %s", name, event.key.which, key, bool(event.key.repeat),
                            core::as_int(int(event.key.scancode)), core::as_int(keysym)); // Throws
                        if (event.type == SDL_EVENT_KEY_DOWN && (keysym == SDLK_ESCAPE || keysym == SDLK_Q)) {
                            close_window(slot->window_id);
                            break;
                        }
                        if (event.type == SDL_EVENT_KEY_UP && keysym == SDLK_N) {
                            SDL_Window* window = open_window(); // Throws
                            bool success = SDL_ShowWindow(window);
                            if (ARCHON_UNLIKELY(!success))
                                throw_sdl_error("SDL_ShowWindow() failed");
                            break;
                        }
                    }
                    break;
                case SDL_EVENT_WINDOW_RESIZED:
                case SDL_EVENT_WINDOW_MOVED:
                case SDL_EVENT_WINDOW_EXPOSED:
                case SDL_EVENT_WINDOW_MOUSE_ENTER:
                case SDL_EVENT_WINDOW_MOUSE_LEAVE:
                case SDL_EVENT_WINDOW_FOCUS_GAINED:
                case SDL_EVENT_WINDOW_FOCUS_LOST:
                case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                    if (ARCHON_LIKELY(try_get_window_slot(event.window.windowID, slot))) {
                        switch (event.type) {
                            case SDL_EVENT_WINDOW_RESIZED:
                                log(slot->no, "SIZE: %s", display::Size(int(event.window.data1),
                                                                        int(event.window.data2))); // Throws
                                break;
                            case SDL_EVENT_WINDOW_MOVED:
                                log(slot->no, "POS: %s", display::Pos(int(event.window.data1),
                                                                      int(event.window.data2))); // Throws
                                break;
                            case SDL_EVENT_WINDOW_EXPOSED:
                                slot->redraw = true;
                                break;
                            case SDL_EVENT_WINDOW_MOUSE_ENTER:
                            case SDL_EVENT_WINDOW_MOUSE_LEAVE:
                                log(slot->no, (event.type == SDL_EVENT_WINDOW_MOUSE_ENTER ? "MOUSE OVER" :
                                               "MOUSE OUT")); // Throws
                                break;
                            case SDL_EVENT_WINDOW_FOCUS_GAINED:
                            case SDL_EVENT_WINDOW_FOCUS_LOST:
                                log(slot->no, (event.type == SDL_EVENT_WINDOW_FOCUS_GAINED ? "FOCUS" :
                                               "BLUR")); // Throws
                                break;
                            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                                close_window(slot->window_id);
                                break;
                        }
                    }
                    break;
                case SDL_EVENT_QUIT:
                    quit = true;
                    break;
            }
        }

        for (const auto& entry : window_slots) {
            const WindowSlot& slot = entry.second;
            if (slot.redraw) {
                bool success = SDL_RenderClear(slot.renderer);
                if (ARCHON_UNLIKELY(!success))
                    throw_sdl_error("SDL_RenderClear() failed");
                success = SDL_RenderPresent(slot.renderer);
                if (ARCHON_UNLIKELY(!success))
                    throw_sdl_error("SDL_RenderPresent() failed");
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
