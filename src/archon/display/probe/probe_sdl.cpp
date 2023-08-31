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


#include <string>
#include <stdexcept>
#include <thread>

#include <archon/core/features.h>
#include <archon/core/format.hpp>
#include <archon/core/quote.hpp>
#include <archon/log.hpp>
#include <archon/image/geom.hpp>
#include <archon/display/impl/config.h>

#if ARCHON_DISPLAY_HAVE_SDL
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


#if ARCHON_DISPLAY_HAVE_SDL


namespace {


auto get_sdl_error(std::string_view message) -> std::string
{
    return core::format("%s: %s", message, SDL_GetError()); // Throws
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


void show_renderer_info(const SDL_RendererInfo& info)
{
    log::info("  Name: %s", core::quoted(info.name));
    log::info("  Flags:");
    if ((info.flags & SDL_RENDERER_SOFTWARE) != 0)
        log::info("    SOFTWARE");
    if ((info.flags & SDL_RENDERER_ACCELERATED) != 0)
        log::info("    ACCELERATED");
    if ((info.flags & SDL_RENDERER_PRESENTVSYNC) != 0)
        log::info("    PRESENTVSYNC");
    if ((info.flags & SDL_RENDERER_TARGETTEXTURE) != 0)
        log::info("    TARGETTEXTURE");
    log::info("  Pixel formats:");
    for (int i = 0; i < int(info.num_texture_formats); ++i)
        log::info("    %s", pixel_format_name(info.texture_formats[i]));
    log::info("  Max texture size: %s", image::Size(info.max_texture_width, info.max_texture_height));
}


} // unnamed namespace


void func()
{
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
    log::info("num_drivers = %s", num_drivers);
    for (int i = 0; i < num_drivers; ++i) {
        SDL_RendererInfo info;
        int ret = SDL_GetRenderDriverInfo(i, &info);
        if (ret < 0)
            throw_sdl_error("SDL_GetRenderDriverInfo() failed");
        log::info("Driver %s:", i);
        show_renderer_info(info);
    }
    SDL_Window* win;
    {
        Uint32 flags = 0;
        win = SDL_CreateWindow("Probe", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 256, 256, flags);
        if (!win)
            throw_sdl_error("SDL_CreateWindow() failed");
    }
    SDL_Renderer* rend;
    {
        int driver_index = -1;
        Uint32 flags = 0;
        rend = SDL_CreateRenderer(win, driver_index, flags);
        if (!rend)
            throw_sdl_error("SDL_CreateRenderer() failed");
    }
    {
        SDL_RendererInfo info;
        int ret = SDL_GetRendererInfo(rend, &info);
        if (ret < 0)
            throw_sdl_error("SDL_GetRenderInfo() failed");
        log::info("Renderer:");
        show_renderer_info(info);
    }
    {
        Uint8 r = 255;
        Uint8 g = 0;
        Uint8 b = 0;
        Uint8 a = 255;
        int ret = SDL_SetRenderDrawColor(rend, r, g, b, a);
        if (ret < 0)
            throw_sdl_error("SDL_SetRenderDrawColor() failed");
    }
    {
        int ret = SDL_RenderClear(rend);
        if (ret < 0)
            throw_sdl_error("SDL_RenderClear() failed");
    }
    SDL_RenderPresent(rend);
    SDL_Event event;
    for (;;) {
        if (SDL_WaitEvent(&event)) {
            if (event.type == SDL_QUIT)
                break;
        }
    }
    SDL_DestroyWindow(win);
    SDL_Quit();
}

int main()
{
    SDL_SetMainReady();
    std::thread thread([] {
        func();
    });
    thread.join();
}

#else // !ARCHON_DISPLAY_HAVE_SDL


int main()
{
    throw std::runtime_error("No SDL support");
}

#endif // !ARCHON_DISPLAY_HAVE_SDL
