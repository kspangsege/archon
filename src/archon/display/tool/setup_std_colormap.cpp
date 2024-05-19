// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2024 Kristian Spangsege <kristian.spangsege@gmail.com>
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
#include <stdexcept>
#include <optional>
#include <string_view>
#include <string>
#include <vector>
#include <set>
#include <locale>
#include <ostream>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/memory.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/value_parser.hpp>
#include <archon/core/format.hpp>
#include <archon/core/as_int.hpp>
#include <archon/core/format_as.hpp>
#include <archon/core/quote.hpp>
#include <archon/core/file.hpp>
#include <archon/log.hpp>
#include <archon/cli.hpp>
#include <archon/display/connection_config_x11.hpp>
#include <archon/display/noinst/x11/support.hpp>


using namespace archon;
namespace impl = display::impl;
namespace x11 = impl::x11;


#if HAVE_X11


namespace {


void create_grayscale_colormap(Display* dpy, Window root, const XVisualInfo& visual_info, bool weird,
                               log::Logger& logger, x11::ColormapWrapper& colormap_owner,
                               XStandardColormap& colormap_params)
{
    Colormap colormap = XCreateColormap(dpy, root, visual_info.visual, AllocAll);
    x11::ColormapWrapper owner;
    owner.set_owned(dpy, colormap);

    int depth = visual_info.depth;
    int colormap_size = visual_info.colormap_size;
    x11::setup_standard_grayscale_colormap(dpy, colormap, depth, colormap_size, weird); // Throws

    VisualID visual = visual_info.visualid;
    x11::BitFields fields = {};
    fields.red_width = depth;
    logger.info("Setting up standard colormap (%s) for GrayScale visual (%s): depth = %s, colormap_size = %s, "
                "shift = %s, width = %s", core::as_flex_int_h(colormap), core::as_flex_int_h(visual),
                core::as_int(depth), core::as_int(colormap_size), core::as_int(fields.red_shift),
                core::as_int(fields.red_width)); // Throws

    XStandardColormap params = {};
    params.colormap = colormap;
    params.visualid = visual;
    params.killid = None;

    bool is_gray = true;
    x11::MultFields fields_2(fields, is_gray);
    fields_2.assign_to(params);

    colormap_owner = std::move(owner);
    colormap_params = params;
}


void create_pseudocolor_colormap(Display* dpy, Window root, const XVisualInfo& visual_info, bool weird,
                                 log::Logger& logger, x11::ColormapWrapper& colormap_owner,
                                 XStandardColormap& colormap_params)
{
    Colormap colormap = XCreateColormap(dpy, root, visual_info.visual, AllocAll);
    x11::ColormapWrapper owner;
    owner.set_owned(dpy, colormap);

    int depth = visual_info.depth;
    int colormap_size = visual_info.colormap_size;
    x11::BitFields fields = {};
    x11::setup_standard_pseudocolor_colormap(dpy, colormap, depth, colormap_size, fields, weird); // Throws

    VisualID visual = visual_info.visualid;
    logger.info("Setting up standard colormap (%s) for PseudoColor visual (%s): depth = %s, colormap_size = %s, "
                "red_shift = %s, red_width = %s, green_shift = %s, green_width = %s, blue_shift = %s, blue_width = %s",
                core::as_flex_int_h(colormap), core::as_flex_int_h(visual), core::as_int(depth),
                core::as_int(colormap_size), core::as_int(fields.red_shift), core::as_int(fields.red_width),
                core::as_int(fields.green_shift), core::as_int(fields.green_width), core::as_int(fields.blue_shift),
                core::as_int(fields.blue_width)); // Throws

    XStandardColormap params = {};
    params.colormap = colormap;
    params.visualid = visual;
    params.killid = None;

    bool is_gray = false;
    x11::MultFields fields_2(fields, is_gray);
    fields_2.assign_to(params);

    colormap_owner = std::move(owner);
    colormap_params = params;
}


void create_directcolor_colormap(Display* dpy, Window root, const XVisualInfo& visual_info, bool weird,
                                 log::Logger& logger, x11::ColormapWrapper& colormap_owner,
                                 XStandardColormap& colormap_params)
{
    Colormap colormap = XCreateColormap(dpy, root, visual_info.visual, AllocAll);
    x11::ColormapWrapper owner;
    owner.set_owned(dpy, colormap);

    int colormap_size = visual_info.colormap_size;
    x11::BitFields fields = x11::record_bit_fields(visual_info); // Throws
    x11::init_directcolor_colormap(dpy, colormap, fields, colormap_size, weird); // Throws

    VisualID visual = visual_info.visualid;
    logger.info("Setting up standard colormap (%s) for DirectColor visual (%s): colormap_size = %s, red_shift = %s, "
                "red_width = %s, green_shift = %s, green_width = %s, blue_shift = %s, blue_width = %s",
                core::as_flex_int_h(colormap), core::as_flex_int_h(visual), core::as_int(colormap_size),
                core::as_int(fields.red_shift), core::as_int(fields.red_width),
                core::as_int(fields.green_shift), core::as_int(fields.green_width),
                core::as_int(fields.blue_shift), core::as_int(fields.blue_width)); // Throws

    XStandardColormap params = {};
    params.colormap = colormap;
    params.visualid = visual;
    params.killid = None;

    bool is_gray = false;
    x11::MultFields fields_2(fields, is_gray);
    fields_2.assign_to(params);

    colormap_owner = std::move(owner);
    colormap_params = params;
}


bool create_standard_colormap(Display* dpy, Window root, const XVisualInfo& visual_info, bool weird,
                              log::Logger& logger, x11::ColormapWrapper& colormap_owner,
                              XStandardColormap& colormap_params)
{
    switch (visual_info.c_class) {
        case StaticGray:
            return false;
        case GrayScale:
            create_grayscale_colormap(dpy, root, visual_info, weird, logger,
                                      colormap_owner, colormap_params); // Throws
            return true;
        case StaticColor:
            return false;
        case PseudoColor:
            create_pseudocolor_colormap(dpy, root, visual_info, weird, logger,
                                        colormap_owner, colormap_params); // Throws
            return true;
        case TrueColor:
            return false;
        case DirectColor:
            create_directcolor_colormap(dpy, root, visual_info, weird, logger,
                                        colormap_owner, colormap_params); // Throws
            return true;
    }
    ARCHON_ASSERT_UNREACHABLE();
    return false;
}


} // unnamed namespace


int main(int argc, char* argv[])
{
    std::locale locale(""); // Throws

    bool all = false;
    log::LogLevel log_level_limit = log::LogLevel::info;
    std::optional<std::string> optional_display;
    std::optional<int> optional_screen;
    std::optional<int> optional_depth;
    std::optional<display::ConnectionConfigX11::VisualClass> optional_class;
    std::optional<VisualID> optional_visual;
    bool weirdness = false;
    bool synchronous_mode = false;

    cli::Spec spec;
    pat("", cli::no_attributes, spec,
        "Lorem ipsum.",
        cli::no_action); // Throws

    opt("-a, --all", "", cli::no_attributes, spec,
        "Instead of creating a standard colormap for one visual, create one for all the visuals that match the "
        "specified criteria while skipping over those with static colormaps.",
        cli::raise_flag(all)); // Throws

    opt("-l, --log-level", "<level>", cli::no_attributes, spec,
        "Set the log level limit. The possible levels are @G. The default limit is @Q.",
        cli::assign(log_level_limit)); // Throws

    opt("-D, --display", "<string>", cli::no_attributes, spec,
        "Target the specified X11 display (@A). If this option is not specified, the value of the DISPLAY environment "
        "variable will be used.",
        cli::assign(optional_display)); // Throws

    opt("-s, --screen", "<number>", cli::no_attributes, spec,
        "Target the specified screen (@A) of the targeted display. If this option is not specified, the default "
        "screen will be used.",
        cli::assign(optional_screen)); // Throws

    opt("-d, --depth", "<number>", cli::no_attributes, spec,
        "Pick a visual of the specified depth (@A).",
        cli::assign(optional_depth)); // Throws

    opt("-c, --class", "<name>", cli::no_attributes, spec,
        "Pick a visual of the specified class (@A). The class can be @F.",
        cli::assign(optional_class)); // Throws

    opt("-v, --visual", "<number>", cli::no_attributes, spec,
        "Target the specified visual type (@A). It can be expressed in decimal, hexadecumal (with prefix '0x'), or "
        "octal (with prefix '0') form. If this option is not specified, the default visual type for the targeted "
        "screen will be used.",
        cli::exec([&](std::string_view str) {
            core::ValueParser parser(locale);
            std::uint_fast32_t val = {};
            if (ARCHON_LIKELY(parser.parse(str, core::as_flex_int(val)))) {
                if (ARCHON_LIKELY(val <= core::int_mask<std::uint_fast32_t>(32))) {
                    optional_visual.emplace(VisualID(val));
                    return true;
                }
            }
            return false;
        })); // Throws

    opt("-w, --weirdness", "", cli::no_attributes, spec,
        "Introduce some detectable weirdness for the purpose of testing the use of the produced colormap.",
        cli::raise_flag(weirdness)); // Throws

    opt("-t, --synchronous-mode", "", cli::no_attributes, spec,
        "Turn on X11's synchronous mode. This is sometimes useful when debugging.",
        cli::raise_flag(synchronous_mode)); // Throws

    opt(cli::help_tag, spec); // Throws
    opt(cli::stop_tag, spec); // Throws

    int exit_status = 0;
    if (ARCHON_UNLIKELY(cli::process(argc, argv, spec, exit_status, locale))) // Throws
        return exit_status;

    log::FileLogger root_logger(core::File::get_cerr(), locale); // Throws
    log::LimitLogger logger(root_logger, log_level_limit); // Throws

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

    x11::ExtensionInfo extension_info = x11::init_extensions(dpy); // Throws
    core::Slab<x11::VisualSpec> visual_specs = x11::load_visuals(dpy, screen, extension_info); // Throws

    x11::FindVisualParams params;
    params.visual_depth = optional_depth;
    params.visual_class = x11::map_opt_visual_class(optional_class);
    params.visual_type = optional_visual;
    core::Buffer<std::size_t> indexes;
    std::size_t num_indexes = 0;
    if (all) {
        num_indexes = x11::find_visuals(dpy, screen, visual_specs, params, indexes); // Throws
    }
    else {
        std::size_t index = {};
        if (ARCHON_LIKELY(x11::find_visual(dpy, screen, visual_specs, params, index))) // Throws
            indexes.append_a(index, num_indexes); // Throws
    }
    if (ARCHON_UNLIKELY(num_indexes == 0)) {
        auto format_conditions = [&](std::ostream& out) {
            out << core::formatted("screen %s", core::as_int(screen)); // Throws
            if (ARCHON_UNLIKELY(optional_depth.has_value()))
                out << core::formatted(", depth %s", core::as_int(optional_depth.value())); // Throws
            if (ARCHON_UNLIKELY(optional_class.has_value()))
                out << core::formatted(", %s", optional_class.value()); // Throws
            if (ARCHON_UNLIKELY(optional_visual.has_value()))
                out << core::formatted(", type %s", core::as_flex_int_h(optional_visual.value())); // Throws
        };
        logger.error("Found no visuals matching specified criteria (%s)",
                     core::as_format_func(format_conditions)); // Throws
        return EXIT_FAILURE;
    }

    // For motivation of the process below, see
    // https://tronche.com/gui/x/xlib/ICC/standard-colormaps/XSetRGBColormaps.html

    x11::ServerGrab grab(dpy);
    Window root = RootWindow(dpy, screen);
    if (ARCHON_UNLIKELY(x11::has_property(dpy, root, XA_RGB_DEFAULT_MAP))) { // Throws
        logger.error("Property `RGB_DEFAULT_MAP` already exists on root window of targeted screen"); // Throws
        return EXIT_FAILURE;
    }

    std::set<VisualID> seen;
    std::vector<x11::ColormapWrapper> colormap_owners;
    std::vector<XStandardColormap> colormap_param_entries;
    for (std::size_t i = 0; i < num_indexes; ++i) {
        std::size_t index = indexes[i];
        const XVisualInfo& visual_info = visual_specs[index].info;
        VisualID visual = visual_info.visualid;
        if (seen.count(visual) == 0) {
            seen.insert(visual); // Throws
            x11::ColormapWrapper colormap_owner;
            XStandardColormap colormap_params = {};
            bool success = create_standard_colormap(dpy, root, visual_info, weirdness, logger,
                                                    colormap_owner, colormap_params); // Throws
            if (ARCHON_LIKELY(success)) {
                colormap_owners.push_back(std::move(colormap_owner)); // Throws
                colormap_param_entries.push_back(colormap_params); // Throws
                continue;
            }
            if (all) {
                logger.info("Skipping %s visual (%s): Has static colormap",
                            x11::get_visual_class_name(visual_info.c_class), core::as_flex_int_h(visual)); // Throws
                continue;
            }
            logger.error("Cannot setup standard colormap for %s visual (%s): Has static colormap",
                         x11::get_visual_class_name(visual_info.c_class), core::as_flex_int_h(visual)); // Throws
            return EXIT_FAILURE;
        }
    }

    int count = {};
    core::int_cast(colormap_param_entries.size(), count); // Throws

    // Ask server to not destroy colormap when client connection is closed
    XSetCloseDownMode(dpy, RetainPermanent);

    XSetRGBColormaps(dpy, root, colormap_param_entries.data(), count, XA_RGB_DEFAULT_MAP);

    for (x11::ColormapWrapper& owner : colormap_owners)
        owner.release_ownership();

    core::NumOfSpec spec_2 = { "standard colormap was", "standard colormaps were" };
    logger.info("%s set up", core::as_num_of(count, spec_2)); // Throws
}


#else // !HAVE_X11


int main()
{
    throw std::runtime_error("No Xlib support");
}


#endif // !HAVE_X11
