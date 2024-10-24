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
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <memory>
#include <optional>
#include <tuple>
#include <vector>
#include <string_view>
#include <string>
#include <locale>
#include <system_error>
#include <filesystem>
#include <ostream>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/scope_exit.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/memory.hpp>
#include <archon/core/flat_map.hpp>
#include <archon/core/locale.hpp>
#include <archon/core/value_parser.hpp>
#include <archon/core/format.hpp>
#include <archon/core/as_int.hpp>
#include <archon/core/as_list.hpp>
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
#include <archon/display/impl/config.h>
#include <archon/display/geometry.hpp>
#include <archon/display/noinst/edid.hpp>
#include <archon/display/x11_fullscreen_monitors.hpp>
#include <archon/display/x11_connection_config.hpp>
#include <archon/display/noinst/impl_util.hpp>
#include <archon/display/noinst/x11/support.hpp>


using namespace archon;
namespace impl = display::impl;
namespace x11 = impl::x11;


#if HAVE_X11


namespace {


auto get_crossing_mode_name(int mode) noexcept -> const char*
{
    switch (mode) {
        case NotifyNormal:
            return "NotifyNormal";
        case NotifyGrab:
            return "NotifyGrab";
        case NotifyUngrab:
            return "NotifyUngrab";
    }
    return "?";
}


class ColormapFinderImpl final
    : public x11::ColormapFinder {
public:
    using standard_colormaps_type = core::FlatMap<VisualID, XStandardColormap>;
    ColormapFinderImpl(VisualID m_default_visual, Colormap m_default_colormap,
                       const standard_colormaps_type& m_standard_colormaps) noexcept;

    bool find_default_colormap(VisualID, Colormap&) const noexcept override;
    bool find_standard_colormap(VisualID, XStandardColormap&) const override;

private:
    VisualID m_default_visual;
    Colormap m_default_colormap;
    const core::FlatMap<VisualID, XStandardColormap>& m_standard_colormaps;
};


inline ColormapFinderImpl::ColormapFinderImpl(VisualID default_visual, Colormap default_colormap,
                                              const standard_colormaps_type& standard_colormaps) noexcept
    : m_default_visual(default_visual)
    , m_default_colormap(default_colormap)
    , m_standard_colormaps(standard_colormaps)
{
}


bool ColormapFinderImpl::find_default_colormap(VisualID visual, Colormap& colormap) const noexcept
{
    if (visual == m_default_visual) {
        colormap = m_default_colormap;
        return true;
    }
    return false;
}


bool ColormapFinderImpl::find_standard_colormap(VisualID visual, XStandardColormap& colormap_params) const
{
    auto i = m_standard_colormaps.find(visual);
    if (i != m_standard_colormaps.end()) {
        colormap_params = i->second;
        return true;
    }
    return false;
}


} // unnamed namespace



int main(int argc, char* argv[])
{
    std::locale locale = core::get_default_locale();

    namespace fs = std::filesystem;
    std::vector<fs::path> paths;
    bool list_visuals = false;
    bool list_pixmap_formats = false;
    std::optional<std::size_t> optional_num_windows;
    std::optional<std::string> optional_window_title;
    util::Color background_color = util::colors::black;
    bool fullscreen = false;
    std::optional<std::string> optional_display;
    std::optional<int> optional_screen;
    std::optional<display::x11_fullscreen_monitors> optional_fullscreen_monitors;
    std::optional<int> optional_visual_depth;
    std::optional<display::x11_connection_config::VisualClass> optional_visual_class;
    std::optional<VisualID> optional_visual_type;
    bool prefer_default_nondecomposed_colormap = false;
    bool disable_double_buffering = false;
    bool disable_detectable_autorepeat = false;
    std::optional<display::Pos> optional_pos;
    log::LogLevel log_level_limit = log::LogLevel::warn;
    bool report_mouse_move = false;
    bool synchronous_mode = false;
    bool install_colormap = false;
    bool colormap_weirdness = false;

    cli::Spec spec;
    pat("[<path>...]", cli::no_attributes, spec,
        "Lorem ipsum.",
        std::tie(paths)); // Throws

    opt(cli::help_tag, spec); // Throws
    opt(cli::stop_tag, spec); // Throws

    opt("-L, --list-visuals", "", cli::no_attributes, spec,
        "List the supported X11 visuals.",
        cli::raise_flag(list_visuals)); // Throws

    opt("-M, --list-pixmap-formats", "", cli::no_attributes, spec,
        "List the supported ZPixmap formats.",
        cli::raise_flag(list_pixmap_formats)); // Throws

    opt("-n, --num-windows", "<num>", cli::no_attributes, spec,
        "The number of windows to be opened. It can be zero. By default, one window will be opened for each of the "
        "specified images, or if no images are specified, the default number of windows is 1.",
        std::tie(optional_num_windows)); // Throws

    opt("-T, --window-title", "<string>", cli::no_attributes, spec,
        "Set an alternate text to be used as window title.",
        cli::assign(optional_window_title)); // Throws

    opt("-b, --background-color", "<color>", cli::no_attributes, spec,
        "Set the background color. \"@A\" can be any valid CSS3 color value with, or without an alpha component, as "
        "well as the extended hex-forms, \"#RGBA\" and \"#RRGGBBAA\", accommodating the alpha component. The default "
        "color is @Q.",
        cli::assign(util::as_css_color(background_color))); // Throws

    opt("-f, --fullscreen", "", cli::no_attributes, spec,
        "Open first window in fullscreen mode.",
        cli::raise_flag(fullscreen)); // Throws

    opt("-D, --display", "<string>", cli::no_attributes, spec,
        "Target the specified X11 display (@A). If this option is not specified, the value of the DISPLAY environment "
        "variable will be used.",
        cli::assign(optional_display)); // Throws

    opt("-s, --screen", "<number>", cli::no_attributes, spec,
        "Target the specified screen (@A) of the targeted display. If this option is not specified, the default "
        "screen will be used.",
        cli::assign(optional_screen)); // Throws

    opt("-F, --fullscreen-monitors", "<monitors>", cli::no_attributes, spec,
        "Use the specified Xinerama screens (monitors) to define the fullscreen area. \"@A\" can be specified as one, "
        "two, or four comma-separated Xinerama screen indexes (`xrandr --listactivemonitors`). When four values are "
        "specified they will be interpreted as the Xinerama screens that determine the top, bottom, left, and right "
        "edges of the fullscreen area. When two values are specified, the first one determines both top and left "
        "edges and the second one determines bottom and right edges. When one value is specified, it determines all "
        "edges.",
        cli::assign(optional_fullscreen_monitors)); // Throws

    opt("-d, --visual-depth", "<num>", cli::no_attributes, spec,
        "Pick a visual of the specified depth (@A).",
        cli::assign(optional_visual_depth)); // Throws

    opt("-c, --visual-class", "<name>", cli::no_attributes, spec,
        "Pick a visual of the specified class (@A). The class can be @F.",
        cli::assign(optional_visual_class)); // Throws

    opt("-V, --visual-type", "<num>", cli::no_attributes, spec,
        "Pick a visual of the specified type (@A). The type, also known as the visual ID, is a 32-bit unsigned "
        "integer that can be expressed in decimal, hexadecumal (with prefix '0x'), or octal (with prefix '0') form.",
        cli::exec([&](std::string_view str) {
            core::ValueParser parser(locale);
            std::uint_fast32_t type = {};
            if (ARCHON_LIKELY(parser.parse(str, core::as_flex_int(type)))) {
                if (ARCHON_LIKELY(type <= core::int_mask<std::uint_fast32_t>(32))) {
                    optional_visual_type.emplace(VisualID(type));
                    return true;
                }
            }
            return false;
        })); // Throws

    opt("-C, --prefer-default-nondecomposed-colormap", "", cli::no_attributes, spec,
        "Prefer the use of the default colormap when the default visual is used and is a PseudoColor or GrayScale "
        "visual. This succeeds if enough colors can be allocated. Otherwise a new colormap is created.",
        cli::raise_flag(prefer_default_nondecomposed_colormap)); // Throws

    opt("-B, --disable-double-buffering", "", cli::no_attributes, spec,
        "Disable use of double buffering, even when the selected visual supports double buffering.",
        cli::raise_flag(disable_double_buffering)); // Throws

    opt("-A, --disable-detectable-autorepeat", "", cli::no_attributes, spec,
        "Do not enable detectable key auto-repeat mode even when it is supported.",
        cli::raise_flag(disable_detectable_autorepeat)); // Throws

    opt("-p, --pos", "<position>", cli::no_attributes, spec,
        "Specify the desired position of the windows. This may or may not be honored by the window manager. If no "
        "position is specified, the position will be determined by the window manager.",
        std::tie(optional_pos)); // Throws

    opt("-l, --log-level", "<level>", cli::no_attributes, spec,
        "Set the log level limit. The possible levels are @G. The default limit is @Q.",
        std::tie(log_level_limit)); // Throws

    opt("-m, --report-mouse-move", "", cli::no_attributes, spec,
        "Turn on reporting of \"mouse move\" events.",
        cli::raise_flag(report_mouse_move)); // Throws

    opt("-y, --synchronous-mode", "", cli::no_attributes, spec,
        "Turn on X11's synchronous mode. In this mode, buffering of X protocol requests is turned off, and the Xlib "
        "functions, that generate X requests, wait for a response from the server before they return. This is "
        "sometimes useful when debugging.",
        cli::raise_flag(synchronous_mode)); // Throws

    opt("-I, --install-colormap", "", cli::no_attributes, spec,
        "Install the colormap, i.e., make it current. This should only be done when there is no window manager.",
        cli::raise_flag(install_colormap)); // Throws

    opt("-W, --colormap-weirdness", "", cli::no_attributes, spec,
        "Use a weird (non-standard) palette when using a visual that allows for palette mutation (`PseudoColor`, "
        "`GrayScale`, and `DirectColor`).",
        cli::raise_flag(colormap_weirdness)); // Throws

    int exit_status = 0;
    if (ARCHON_UNLIKELY(cli::process(argc, argv, spec, exit_status, locale))) // Throws
        return exit_status;

    log::FileLogger root_logger(core::File::get_stdout(), locale); // Throws
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
    build_env_params.bin_path  = "archon/display/probe/archon-probe-x11";
    build_env_params.src_path  = "archon/display/probe/probe_x11.cpp";
    build_env_params.src_root  = "src";
    build_env_params.source_from_build_path = core::archon_source_from_build_path;
    core::BuildEnvironment build_env = core::BuildEnvironment(argv[0], build_env_params, locale); // Throws

    fs::path resource_path = (build_env.get_relative_source_root() /
                              core::make_fs_path_generic("archon/display/probe", locale)); // Throws

    // Load images
    if (paths.empty()) {
        fs::path path = resource_path / core::make_fs_path_generic("image.png", locale); // Throws
        paths.push_back(path); // Throws
    }
    core::Slab<std::unique_ptr<image::Image>> images(paths.size()); // Throws
    {
        log::PrefixLogger load_logger(logger, "Load: "); // Throws
        image::LoadConfig load_config;
        load_config.logger = &load_logger;
        for (const fs::path& path : paths) {
            std::unique_ptr<image::WritableImage> img;
            std::error_code ec;
            if (!image::try_load(path, img, locale, load_config, ec)) { // Throws
                logger.error("%s: Failed to load image: %s", core::as_native_path(path), ec.message()); // Throws
                return EXIT_FAILURE;
            }
            images.add(std::move(img));
        }
    }

    // Connect to display
    std::string_view display = x11::get_display_string(optional_display);
    x11::DisplayWrapper dpy_owner;
    if (ARCHON_UNLIKELY(!x11::try_connect(display, dpy_owner))) { // Throws
        logger.error("Failed to open X11 display connection to %s", core::quoted(display)); // Throws
        return EXIT_FAILURE;
    }
    Display* dpy = dpy_owner;

    if (ARCHON_UNLIKELY(synchronous_mode))
        XSynchronize(dpy, True);

    int screen = x11::get_screen_index(dpy, optional_screen);
    if (ARCHON_UNLIKELY(!x11::valid_screen_index(dpy, screen))) {
        logger.error("Invalid screen index (%s)", core::as_int(screen)); // Throws
        return EXIT_FAILURE;
    }

    Window root = RootWindow(dpy, screen);
    VisualID default_visual = XVisualIDFromVisual(DefaultVisual(dpy, screen));
    Colormap default_colormap = DefaultColormap(dpy, screen);

    x11::ExtensionInfo extension_info = x11::init_extensions(dpy); // Throws

    bool detectable_autorepeat_enabled = false;
    if (ARCHON_LIKELY(extension_info.have_xkb && !disable_detectable_autorepeat)) {
        Bool detectable = True;
        Bool supported = {};
        XkbSetDetectableAutoRepeat(dpy, detectable, &supported);
        if (ARCHON_LIKELY(supported))
            detectable_autorepeat_enabled = true;
    }

#if HAVE_XRANDR
    if (ARCHON_LIKELY(extension_info.have_xrandr)) {
        int mask = RROutputChangeNotifyMask | RRCrtcChangeNotifyMask;
        XRRSelectInput(dpy, root, mask);
    }
#endif // HAVE_XRANDR

    // Key is visual depth
    core::FlatMap<int, XPixmapFormatValues> pixmap_formats = x11::fetch_pixmap_formats(dpy); // Throws

    core::FlatMap<VisualID, XStandardColormap> standard_colormaps = x11::fetch_standard_colormaps(dpy, root); // Throws

    // Fetch depths
    std::vector<int> depths;
    {
        int n = 0;
        int* entries = XListDepths(dpy, screen, &n);
        ARCHON_STEADY_ASSERT(entries);
        for (int i = 0; i < n; ++i)
            depths.push_back(entries[i]);
        XFree(entries);
    }

    core::Slab<x11::VisualSpec> visual_specs = x11::load_visuals(dpy, screen, extension_info); // Throws

    // List supported visuals
    if (list_visuals) {
        std::size_t n = visual_specs.size();
        for (std::size_t i = 0; i < n; ++i) {
            const x11::VisualSpec& spec = visual_specs[i];
            const XVisualInfo& info = spec.info;
            auto format_double_buffered = [&](std::ostream& out) {
                if (spec.double_buffered) {
                    if (spec.double_buffered_perflevel != 0) {
                        out << core::formatted("yes (%s)", core::as_int(spec.double_buffered_perflevel)); // Throws
                    }
                    else {
                        out << "yes"; // Throws
                    }
                }
                else {
                    out << "no"; // Throws
                }
            };
            logger.info("Visual %s: visualid = %s, screen = %s, depth = %s, class = %s, "
                        "red_mask = %s, green_mask = %s, blue_mask = %s, colormap_size = %s, bits_per_rgb = %s, "
                        "double_buffered = %s, supports_opengl = %s, opengl_level = %s, opengl_double_buffered = %s, "
                        "opengl_stereo = %s, opengl_num_aux_buffers = %s, opengl_depth_buffer_bits = %s, "
                        "opengl_stencil_buffer_bits = %s, opengl_accum_buffer_bits = %s", i + 1,
                        core::as_flex_int_h(info.visualid), core::as_int(info.screen), core::as_int(info.depth),
                        x11::get_visual_class_name(info.c_class), core::as_flex_int_h(info.red_mask),
                        core::as_flex_int_h(info.green_mask), core::as_flex_int_h(info.blue_mask),
                        core::as_int(info.colormap_size), core::as_int(info.bits_per_rgb),
                        core::as_format_func(format_double_buffered), (spec.opengl_supported ? "yes" : "no"),
                        core::as_int(spec.opengl_level), (spec.opengl_double_buffered ? "yes" : "no"),
                        (spec.opengl_stereo ? "yes" : "no"), core::as_int(spec.opengl_num_aux_buffers),
                        core::as_int(spec.opengl_depth_buffer_bits), core::as_int(spec.opengl_stencil_buffer_bits),
                        core::as_int(spec.opengl_accum_buffer_bits)); // Throws
        }
    }

    // List supported ZPixmap formats
    if (list_pixmap_formats) {
        std::size_t i = 0;
        for (const auto& entry : pixmap_formats) {
            const XPixmapFormatValues& format = entry.second;
            logger.info("Format %s: depth = %s, bits_per_pixel = %s, scanline_pad = %s", i + 1, format.depth,
                        format.bits_per_pixel, format.scanline_pad); // Throws
            ++i;
        }
    }

    // Choose visual (depth and type)
    x11::FindVisualParams params;
    params.visual_depth = optional_visual_depth;
    params.visual_class = x11::map_opt_visual_class(optional_visual_class);
    params.visual_type = optional_visual_type;
    params.prefer_double_buffered = !disable_double_buffering;
    std::size_t index = {};
    if (ARCHON_UNLIKELY(!x11::find_visual(dpy, screen, visual_specs, params, index))) { // Throws
        logger.error("No suitable X11 visual found");
        return EXIT_FAILURE;
    }
    const x11::VisualSpec& visual_spec = visual_specs[index];
    const XVisualInfo& visual_info = visual_spec.info;
    int depth = visual_info.depth;
    VisualID visualid = visual_info.visualid;
    bool use_double_buffering = visual_spec.double_buffered;
    const XPixmapFormatValues& pixmap_format = pixmap_formats.at(depth); // Throws

    auto format_have_xdbe = [&](std::ostream& out) {
        if (extension_info.have_xdbe) {
            out << core::formatted("yes (%s.%s)", extension_info.xdbe_major, extension_info.xdbe_minor); // Throws
            return;
        }
        out << "no"; // Throws
    };

    auto format_have_xkb = [&](std::ostream& out) {
        if (extension_info.have_xkb) {
            out << core::formatted("yes (%s.%s)", extension_info.xkb_major, extension_info.xkb_minor); // Throws
            return;
        }
        out << "no"; // Throws
    };

    auto format_have_xrandr = [&](std::ostream& out) {
        if (extension_info.have_xrandr) {
            out << core::formatted("yes (%s.%s)", extension_info.xrandr_major, extension_info.xrandr_minor); // Throws
            return;
        }
        out << "no"; // Throws
    };

    auto format_have_xrender = [&](std::ostream& out) {
        if (extension_info.have_xrender) {
            out << core::formatted("yes (%s.%s)", extension_info.xrender_major, extension_info.xrender_minor); // Throws
            return;
        }
        out << "no"; // Throws
    };

    auto format_have_glx = [&](std::ostream& out) {
        if (extension_info.have_glx) {
            out << core::formatted("yes (%s.%s)", extension_info.glx_major, extension_info.glx_minor); // Throws
            return;
        }
        out << "no"; // Throws
    };

    logger.info("Display string:                     %s", DisplayString(dpy)); // Throws
    logger.info("Server vendor:                      %s", ServerVendor(dpy)); // Throws
    logger.info("Vendor release:                     %s", core::as_int(VendorRelease(dpy))); // Throws
    logger.info("Have Xdbe:                          %s", core::as_format_func(format_have_xdbe)); // Throws
    logger.info("Have Xkb:                           %s", core::as_format_func(format_have_xkb)); // Throws
    logger.info("Have Xrandr:                        %s", core::as_format_func(format_have_xrandr)); // Throws
    logger.info("Have Xrender:                       %s", core::as_format_func(format_have_xrender)); // Throws
    logger.info("Have GLX:                           %s", core::as_format_func(format_have_glx)); // Throws
    logger.info("Image byte order:                   %s",
                (ImageByteOrder(dpy) == LSBFirst ? "little-endian" : "big-endian")); // Throws
    logger.info("Bitmap bit order:                   %s",
                (BitmapBitOrder(dpy) == LSBFirst ? "least significant bit first" :
                 "most significant bit first")); // Throws
    logger.info("Bitmap scanline pad:                %s", core::as_int(BitmapPad(dpy))); // Throws
    logger.info("Bitmap scanline unit:               %s", core::as_int(BitmapUnit(dpy))); // Throws
    logger.info("Number of screens:                  %s", core::as_int(ScreenCount(dpy))); // Throws
    logger.info("Selected screen:                    %s", core::as_int(screen + 1)); // Throws
    logger.info("Size of screen:                     %spx x %spx (%smm x %smm)",
                core::as_int(DisplayWidth(dpy, screen)), core::as_int(DisplayHeight(dpy, screen)),
                core::as_int(DisplayWidthMM(dpy, screen)), core::as_int(DisplayHeightMM(dpy, screen))); // Throws
    logger.info("Resolution of screen (dpcm):        %s x %s",
                10 * (DisplayWidth(dpy, screen) / double(DisplayWidthMM(dpy, screen))),
                10 * (DisplayHeight(dpy, screen) / double(DisplayHeightMM(dpy, screen)))); // Throws
    logger.info("Concurrent colormaps of screen:     %s -> %s",
                MinCmapsOfScreen(ScreenOfDisplay(dpy, screen)),
                MaxCmapsOfScreen(ScreenOfDisplay(dpy, screen))); // Throws
    logger.info("Size of default colormap of screen: %s", core::as_int(DisplayCells(dpy, screen))); // Throws
    logger.info("Supported depths on screen:         %s", core::as_list(depths)); // Throws
    logger.info("Default depth of screen:            %s", core::as_int(DefaultDepth(dpy, screen))); // Throws
    logger.info("Selected depth:                     %s", core::as_int(depth)); // Throws
    logger.info("Default visual of screen:           %s", core::as_flex_int_h(default_visual)); // Throws
    logger.info("Selected visual:                    %s", core::as_flex_int_h(visualid)); // Throws
    logger.info("Class of selected visual:           %s", x11::get_visual_class_name(visual_info.c_class)); // Throws
    logger.info("Detectable auto-repeat enabled:     %s", (detectable_autorepeat_enabled ? "yes" : "no")); // Throws
    logger.info("Use double buffering:               %s", (use_double_buffering ? "yes" : "no")); // Throws

    if (ARCHON_UNLIKELY(!extension_info.have_xkb)) {
        logger.error("Required X Keyboard Extension is not available");
        return EXIT_FAILURE;
    }

    ColormapFinderImpl colormap_finder(default_visual, default_colormap, standard_colormaps);
    std::unique_ptr<x11::PixelFormat> pixel_format =
        x11::create_pixel_format(dpy, root, visual_info, pixmap_format, colormap_finder, locale, logger,
                                 prefer_default_nondecomposed_colormap, colormap_weirdness); // Throws
    unsigned long interned_background_color = pixel_format->intern_color(background_color); // Throws
    Colormap colormap = pixel_format->get_colormap();

    auto intern_string = [&](const char* string) noexcept -> Atom {
        Atom atom = XInternAtom(dpy, string, False);
        ARCHON_STEADY_ASSERT(atom != None);
        return atom;
    };

#if HAVE_XRANDR
    x11::ScreenConf screen_conf;
    Atom atom_edid = intern_string(RR_PROPERTY_RANDR_EDID);
    impl::EdidParser edid_parser(locale);
    auto update_screen_conf = [&] {
        return x11::update_screen_conf(dpy, root, atom_edid, edid_parser, locale, screen_conf); // Throws
    };
    auto dump_screen_conf = [&] {
        const char* strings_base = screen_conf.string_buffer.data();
        std::size_t n = screen_conf.viewports.size();
        for (std::size_t i = 0; i < n; ++i) {
            const x11::ProtoViewport& viewport = screen_conf.viewports[i];
            auto format_monitor_name = [&](std::ostream& out) {
                if (ARCHON_LIKELY(viewport.monitor_name.has_value())) {
                    out << core::quoted(viewport.monitor_name.value().resolve_string(strings_base)); // Throws
                }
                else {
                    out << "unknown"; // Throws
                }
            };
            logger.info("Viewport %s/%s: output_name=%s, bounds=%s, monitor_name=%s, resolution=%s, refresh_rate=%s",
                        i + 1, n, core::quoted(viewport.output_name.resolve_string(strings_base)), viewport.bounds,
                        core::as_format_func(format_monitor_name), core::as_optional(viewport.resolution, "unknown"),
                        core::as_optional(viewport.refresh_rate, "unknown")); // Throws
        }
    };
    if (ARCHON_LIKELY(extension_info.have_xrandr)) {
        update_screen_conf(); // Throws
        dump_screen_conf(); // Throws
    }
#endif // HAVE_XRANDR

    // Create graphics context
    XGCValues gc_values = {};
    gc_values.graphics_exposures = False;
    GC gc = XCreateGC(dpy, root, GCGraphicsExposures, &gc_values);
    ARCHON_SCOPE_EXIT {
        XFreeGC(dpy, gc);
    };
    XSetForeground(dpy, gc, interned_background_color);

    // Upload images
    struct PixmapSlot {
        image::Size size;
        Pixmap pixmap;
    };
    core::Slab<PixmapSlot> pixmaps(images.size()); // Throws
    ARCHON_SCOPE_EXIT {
        for (const auto& slot : pixmaps)
            XFreePixmap(dpy, slot.pixmap);
    };
    {
        std::unique_ptr<x11::ImageBridge> bridge =
            pixel_format->create_image_bridge(impl::subdivide_max_subbox_size); // Throws
        image::Writer writer(bridge->img_1); // Throws
        for (const std::unique_ptr<image::Image>& img : images) {
            image::Size size = img->get_size();
            Pixmap pixmap = XCreatePixmap(dpy, root, unsigned(size.width), unsigned(size.height), depth);
            pixmaps.add(PixmapSlot { size, pixmap });
            image::Reader reader(*img); // Throws
            impl::subdivide(size, [&](const display::Box& subbox) {
                image::Pos pos = { 0, 0 };
                writer.put_image_a(pos, reader, subbox); // Throws
                int src_x = pos.x, src_y = pos.y;
                int dest_x = subbox.pos.x, dest_y = subbox.pos.y;
                unsigned width = unsigned(subbox.size.width);
                unsigned height = unsigned(subbox.size.height);
                XPutImage(dpy, pixmap, gc, &bridge->img_2, src_x, src_y, dest_x, dest_y, width, height);
            }); // Throws
        }
    }

    struct WindowSlot {
        bool is_first;
        int no;
        Window window;
        Drawable drawable;
        image::Size win_size;
        image::Size img_size;
        Pixmap pixmap;
        bool fullscreen = false;
        bool redraw = false;
        bool suppress_redraw = false;

        WindowSlot(bool is_first_2, int no_2, Window window_2, Drawable drawable_2, image::Size size,
                   Pixmap pixmap_2) noexcept
        {
            is_first = is_first_2;
            no       = no_2;
            window   = window_2;
            drawable = drawable_2;
            win_size = size;
            img_size = size;
            pixmap   = pixmap_2;
        }
    };

    core::FlatMap<Window, WindowSlot> window_slots;

    auto try_get_window_slot = [&](Window win, WindowSlot*& slot) {
        auto i = window_slots.find(win);
        if (ARCHON_LIKELY(i != window_slots.end())) {
            slot = &i->second;
            return true;
        }
        return false;
    };

    Atom delete_window                   = intern_string("WM_DELETE_WINDOW");
    Atom atom_net_wm_fullscreen_monitors = intern_string("_NET_WM_FULLSCREEN_MONITORS");
    Atom atom_net_wm_state               = intern_string("_NET_WM_STATE");
    Atom atom_net_wm_state_fullscreen    = intern_string("_NET_WM_STATE_FULLSCREEN");

#if HAVE_XDBE
    XdbeSwapAction swap_action = XdbeUndefined; // Contents of swapped-out buffer becomes undefined
#endif

    std::size_t next_window_index = 0;
    std::size_t max_seen_window_slots = 0;
    auto open_window = [&] {
        // Link to an image (round-robin)
        int window_index = next_window_index++;
        ARCHON_ASSERT(pixmaps.size() > 0);
        std::size_t pixmap_index = window_index % pixmaps.size();
        const PixmapSlot& pixmap_slot = pixmaps[pixmap_index];
        image::Size size = pixmap_slot.size;

        // Create window
        display::Pos pos;
        if (optional_pos.has_value())
            pos = optional_pos.value();
        unsigned long valuemask = CWBackPixel | CWEventMask | CWColormap;
        XSetWindowAttributes attributes;
        attributes.background_pixel = interned_background_color;
        attributes.event_mask = (KeyPressMask | KeyReleaseMask |
                                 ButtonPressMask | ButtonReleaseMask |
                                 ButtonMotionMask |
                                 EnterWindowMask | LeaveWindowMask |
                                 FocusChangeMask |
                                 ExposureMask |
                                 StructureNotifyMask |
                                 KeymapStateMask);
        attributes.colormap = colormap;
        Window window = XCreateWindow(dpy, root, pos.x, pos.y, unsigned(size.width), unsigned(size.height), 0, depth,
                                      InputOutput, visual_info.visual, valuemask, &attributes);

        // Set window name
        int no = int(window_index + 1);
        std::string name_1;
        std::string_view name_2;
        if (optional_window_title.has_value()) {
            name_2 = optional_window_title.value();
        }
        else {
            name_1 = core::format(locale, "X11 Probe %s", core::as_int(no)); // Throws
            name_2 = name_1;
        }
        x11::TextPropertyWrapper name_3(dpy, name_2, locale); // Throws
        XSetWMName(dpy, window, &name_3.prop);

        // Tell window manager to assign input focus to this window
        XWMHints hints = {};
        hints.flags = InputHint;
        hints.input = True;
        XSetWMHints(dpy, window, &hints);

        // Set minimum window size
        XSizeHints size_hints;
        size_hints.flags = PMinSize;
        size_hints.min_width  = 128;
        size_hints.min_height = 128;
        if (optional_pos.has_value()) {
            size_hints.flags |= USPosition;
            size_hints.x = pos.x; // Mostly ignored!?
            size_hints.y = pos.y; // Mostly ignored!?
        }
        XSetWMNormalHints(dpy, window, &size_hints);

        // Ask X to notify rather than close connection when window is closed
        XSetWMProtocols(dpy, window, &delete_window, 1);

        // Allocate back buffer when using double buffering
        Drawable drawable = window;
#if HAVE_XDBE
        if (use_double_buffering) {
            XdbeBackBuffer back_buffer = XdbeAllocateBackBufferName(dpy, window, swap_action);
            drawable = back_buffer;
        }
#endif // HAVE_XDBE

        bool is_first = (window_index == 0);
        WindowSlot slot = { is_first, no, window, drawable, size, pixmap_slot.pixmap };
        window_slots.emplace(window, slot); // Throws

        if (window_slots.size() > max_seen_window_slots)
            max_seen_window_slots = window_slots.size();

        return window;
    };

    auto close_window = [&](Window win) noexcept {
        XDestroyWindow(dpy, win);
        window_slots.erase(win);
    };

    auto set_fullscreen_monitors = [&](Window win) {
        if (ARCHON_LIKELY(!optional_fullscreen_monitors.has_value()))
            return;
        x11::set_fullscreen_monitors(dpy, win, optional_fullscreen_monitors.value(), root,
                                     atom_net_wm_fullscreen_monitors); // Throws
    };

    auto set_fullscreen_mode = [&](Window win, bool on) {
        x11::set_fullscreen_mode(dpy, win, on, root, atom_net_wm_state, atom_net_wm_state_fullscreen); // Throws
    };

    auto get_keysym = [&](KeyCode keycode) noexcept -> KeySym {
        // Map key code to a keyboard independent symbol identifier (in general the symbol
        // in the upper left corner on the corresponding key). See also
        // https://tronche.com/gui/x/xlib/input/keyboard-encoding.html.
        KeySym keysym = XkbKeycodeToKeysym(dpy, keycode, XkbGroup1Index, 0);
        ARCHON_STEADY_ASSERT(keysym != NoSymbol);
        return keysym;
    };

    auto get_key_name = [&](KeySym keysym) -> std::string_view {
        // XKeysymToString() returns a string consisting entirely of characters from the X
        // Portable Character Set. Since all locales, that are compatible with Xlib, agree
        // on the encoding of characters in this character set, and since we assume that the
        // selected locale is compatible with Xlib, we can assume that the returned string
        // is valid in the selected locale.
        return XKeysymToString(keysym);
    };

    auto log = [&](int window_no, std::string_view message, const auto&... args) {
        if (max_seen_window_slots < 2) {
            logger.info(message, args...); // Throws
        }
        else {
            logger.info("WINDOW %s: %s", window_no, core::formatted(message, args...)); // Throws
        }
    };

    std::size_t num_windows = 1;
    if (!paths.empty())
        num_windows = paths.size();
    if (optional_num_windows.has_value())
        num_windows = optional_num_windows.value();
    for (std::size_t i = 0; i < num_windows; ++i)
        open_window(); // Throws

    for (auto& entry : window_slots) {
        WindowSlot& slot = entry.second;
        XMapWindow(dpy, slot.window);
        set_fullscreen_monitors(slot.window); // Throws
        if (slot.is_first && fullscreen) {
            slot.fullscreen = true;
            set_fullscreen_mode(slot.window, true); // Throws
        }
    }

    if (install_colormap)
        XInstallColormap(dpy, colormap);

    // Event loop
    bool expect_keymap_notify = false;
    std::vector<std::string_view> key_names;
    while (!window_slots.empty()) {
        XEvent ev = {};
        XPeekEvent(dpy, &ev);
        for (;;) {
            int n = XEventsQueued(dpy, QueuedAfterReading);
            if (n == 0)
                break;
            for (int i = 0; i < n; ++i) {
                XNextEvent(dpy, &ev);
                bool expect_keymap_notify_2 = expect_keymap_notify;
                expect_keymap_notify = false;
                ARCHON_ASSERT(!expect_keymap_notify_2 || ev.type == KeymapNotify);
                WindowSlot* slot = {};
                switch (ev.type) {
                    case MotionNotify:
                        if (ARCHON_LIKELY(try_get_window_slot(ev.xmotion.window, slot))) {
                            display::Pos pos = { ev.xmotion.x, ev.xmotion.y };
                            if (report_mouse_move)
                                log(slot->no, "MOUSE MOVE: %s", pos); // Throws
                        }
                        break;
                    case ConfigureNotify:
                        if (ARCHON_LIKELY(try_get_window_slot(ev.xconfigure.window, slot))) {
                            // When there is a window manager, the window manager will
                            // generally reparent the client's window. This generally means
                            // that the client's window will remain at a fixed position
                            // relative to it's parent, so there will be no configure
                            // notifications when the window is moved through user
                            // interaction. Also, if the user's window is moved relative to
                            // its parent, the reported position will be unreliable, as it
                            // will be relative to its parent, which is not the root window
                            // of the screen. Fortunately, in all those cases, the window
                            // manager is obligated to generate synthetic configure
                            // notifications in which the positions are absolute (relative
                            // to the root window of the screen).
                            if (ev.xconfigure.send_event) {
                                log(slot->no, "POS: %s", display::Pos(ev.xconfigure.x, ev.xconfigure.y));
                            }
                            else {
                                log(slot->no, "SIZE: %s", display::Size(ev.xconfigure.width, ev.xconfigure.height));
                            }
                            display::Size size = { ev.xconfigure.width, ev.xconfigure.height };
                            if (size != slot->win_size) {
                                slot->win_size = size;
                                slot->redraw = true;
                            }
                        }
                        break;
                    case Expose:
                        if (ARCHON_LIKELY(try_get_window_slot(ev.xexpose.window, slot)))
                            slot->redraw = true;
                        break;
                    case ButtonPress:
                    case ButtonRelease:
                        if (ARCHON_LIKELY(try_get_window_slot(ev.xbutton.window, slot))) {
                            log(slot->no, "%s: %s, (%s,%s)", (ev.type == ButtonPress ? "MOUSE DOWN" : "MOUSE UP"),
                                ev.xbutton.button, ev.xbutton.x, ev.xbutton.y); // Throws
                        }
                        break;
                    case KeyPress:
                    case KeyRelease:
                        if (ARCHON_LIKELY(try_get_window_slot(ev.xkey.window, slot))) {
                            KeySym keysym = get_keysym(ev.xkey.keycode);
                            std::string_view key_name = get_key_name(keysym); // Throws
                            log(slot->no, "%s: %s, %s -> %s", (ev.type == KeyPress ? "KEY DOWN" : "KEY UP"), key_name,
                                core::as_int(ev.xkey.keycode), core::as_int(keysym)); // Throws
                            if (ev.type == KeyPress && (keysym == XK_Escape || keysym == XK_q)) {
                                close_window(slot->window);
                                break;
                            }
                            if (ev.type == KeyRelease && keysym == XK_n) {
                                Window window = open_window(); // Throws
                                XMapWindow(dpy, window);
                                set_fullscreen_monitors(window); // Throws
                                break;
                            }
                            if (ev.type == KeyRelease && keysym == XK_f) {
                                slot->fullscreen = !slot->fullscreen;
                                set_fullscreen_mode(slot->window, slot->fullscreen); // Throws
                                break;
                            }
                            if (ev.type == KeyRelease && keysym == XK_r) {
                                slot->suppress_redraw = !slot->suppress_redraw;
                                break;
                            }
                        }
                        break;
                    case KeymapNotify:
                        // Note: For some unclear reason, `ev.xkeymap.window` does not
                        // specify the target window like it does for other types of
                        // events. Instead, one can rely on `KeymapNotify` to be generated
                        // immediately after every `FocusIn` event, so this provides an
                        // implicit target window.
                        if (expect_keymap_notify_2) {
                            key_names.clear();
                            // X11 key codes lie in the inclusive range [8,255]
                            for (int i = 8; i < 256; ++i) {
                                using uchar = unsigned char;
                                bool pressed = ((int(uchar(ev.xkeymap.key_vector[i / 8])) & (1 << (i % 8))) != 0);
                                if (ARCHON_LIKELY(!pressed))
                                    continue;
                                KeySym keysym = get_keysym(KeyCode(i));
                                std::string_view key_name = get_key_name(keysym); // Throws
                                key_names.push_back(key_name); // Throws
                            }
                            logger.info("KEYMAP: %s", core::as_sbr_list(key_names)); // Throws
                        }
                        break;
                    case EnterNotify:
                    case LeaveNotify:
                        if (ARCHON_LIKELY(try_get_window_slot(ev.xcrossing.window, slot)))
                            log(slot->no, "%s: %s", (ev.type == EnterNotify ? "MOUSE OVER" : "MOUSE OUT"),
                                get_crossing_mode_name(ev.xcrossing.mode)); // Throws
                        break;
                    case FocusIn:
                    case FocusOut:
                        if (ev.type == FocusIn)
                            expect_keymap_notify = true;
                        if (ARCHON_LIKELY(try_get_window_slot(ev.xfocus.window, slot)))
                            log(slot->no, (ev.type == FocusIn ? "FOCUS" : "BLUR")); // Throws
                        break;
                    case ClientMessage:
                        if (ARCHON_LIKELY(try_get_window_slot(ev.xclient.window, slot))) {
                            bool is_close = (ev.xclient.format == 32 && Atom(ev.xclient.data.l[0]) == delete_window);
                            if (is_close)
                                close_window(slot->window);
                        }
                        break;
                }
#if HAVE_XRANDR
                if (extension_info.have_xrandr && ev.type == extension_info.xrandr_event_base + RRNotify) {
                    const auto& ev_2 = reinterpret_cast<const XRRNotifyEvent&>(ev);
                    switch (ev_2.subtype) {
                        case RRNotify_CrtcChange:
                        case RRNotify_OutputChange:
                            if (update_screen_conf()) // Throws
                                dump_screen_conf(); // Throws
                    }
                }
#endif // HAVE_XRANDR
            }
        }

        for (const auto& entry : window_slots) {
            const WindowSlot& slot = entry.second;
            if (slot.redraw && !slot.suppress_redraw) {
                int win_width  = slot.win_size.width;
                int win_height = slot.win_size.height;
                int left = 0, right = win_width;
                int top = 0, bottom = win_height;
                int x = 0, y = 0;
                int w = slot.img_size.width, h = slot.img_size.height;
                {
                    int width_diff  = win_width  - slot.img_size.width;
                    int height_diff = win_height - slot.img_size.height;
                    if (width_diff >= 0) {
                        left  = width_diff / 2;
                        right = left + slot.img_size.width;
                    }
                    else {
                        x = (-width_diff + 1) / 2;
                        w = win_width;
                    }
                    if (height_diff >= 0) {
                        top    = height_diff / 2;
                        bottom = top + slot.img_size.height;
                    }
                    else {
                        y = (-height_diff + 1) / 2;
                        h = win_height;
                    }
                }
                Drawable drawable = slot.drawable;
                // Clear top area
                if (top > 0)
                    XFillRectangle(dpy, drawable, gc, 0, 0, unsigned(win_width), unsigned(top));
                // Clear left area
                if (left > 0)
                    XFillRectangle(dpy, drawable, gc, 0, top, unsigned(left), unsigned(h));
                // Copy image
                XCopyArea(dpy, slot.pixmap, drawable, gc, x, y, w, h, left, top);
                // Clear right area
                if (right < win_width)
                    XFillRectangle(dpy, drawable, gc, right, top, unsigned(win_width - right), unsigned(h));
                // Clear bottom area
                if (bottom < win_height)
                    XFillRectangle(dpy, drawable, gc, 0, bottom, unsigned(win_width), unsigned(win_height - bottom));

#if HAVE_XDBE
                if (use_double_buffering) {
                    XdbeSwapInfo info;
                    info.swap_window = slot.window;
                    info.swap_action = swap_action;
                    Status status = XdbeSwapBuffers(dpy, &info, 1);
                    ARCHON_STEADY_ASSERT(status != 0);
                }
#endif // HAVE_XDBE
            }
        }
    }
}


#else // !HAVE_X11


int main()
{
    throw std::runtime_error("No Xlib support");
}


#endif // !HAVE_X11
