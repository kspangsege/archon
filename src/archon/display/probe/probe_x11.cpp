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
#include <algorithm>
#include <stdexcept>
#include <utility>
#include <memory>
#include <optional>
#include <tuple>
#include <array>
#include <vector>
#include <string_view>
#include <locale>
#include <system_error>
#include <filesystem>
#include <ostream>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/scope_exit.hpp>
#include <archon/core/index_range.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/string_buffer_contents.hpp>
#include <archon/core/vector.hpp>
#include <archon/core/flat_map.hpp>
#include <archon/core/string.hpp>
#include <archon/core/format.hpp>
#include <archon/core/value_parser.hpp>
#include <archon/core/as_int.hpp>
#include <archon/core/as_list.hpp>
#include <archon/core/format_as.hpp>
#include <archon/core/quote.hpp>
#include <archon/core/endianness.hpp>
#include <archon/core/filesystem.hpp>
#include <archon/core/build_environment.hpp>
#include <archon/core/file.hpp>
#include <archon/log.hpp>
#include <archon/cli.hpp>
#include <archon/util/color.hpp>
#include <archon/util/as_css_color.hpp>
#include <archon/image.hpp>
#include <archon/image/channel_packing.hpp>
#include <archon/image/packed_pixel_format.hpp>
#include <archon/display/impl/config.h>
#include <archon/display/geometry.hpp>
#include <archon/display/resolution.hpp>
#include <archon/display/screen.hpp>
#include <archon/display/noinst/edid.hpp>
#include <archon/display/connection_config_x11.hpp>
#include <archon/display/noinst/x11_support.hpp>


using namespace archon;
namespace impl = display::impl;


#if HAVE_X11


namespace {


auto get_visual_class_name(int class_) noexcept -> const char*
{
    switch (class_) {
        case StaticGray:
            return "StaticGray";
        case GrayScale:
            return "GrayScale";
        case StaticColor:
            return "StaticColor";
        case PseudoColor:
            return "PseudoColor";
        case TrueColor:
            return "TrueColor";
        case DirectColor:
            return "DirectColor";
    }
    return nullptr;
}


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


// Check whether the masks of the specified visual are all zero.
inline bool zero_mask_match(const XVisualInfo& info) noexcept
{
    return (info.red_mask == 0 && info.green_mask == 0 && info.blue_mask == 0);
}


// Check whether the masks of the specified visual correspond to the specified channel
// packing under the asumption that the channel order is normal (normal channel order is RGB
// as opposed to BGR).
template<class P> inline bool norm_mask_match(const XVisualInfo& info) noexcept
{
    using packing_type = P;
    static_assert(packing_type::num_fields == 3);
    using word_type = decltype(info.red_mask + info.green_mask + info.blue_mask);
    return (info.red_mask   == image::get_bit_field_mask<word_type>(packing_type::fields, 3, 0) &&
            info.green_mask == image::get_bit_field_mask<word_type>(packing_type::fields, 3, 1) &&
            info.blue_mask  == image::get_bit_field_mask<word_type>(packing_type::fields, 3, 2));
}


// Check whether the masks of the specified visual correspond to the specified channel
// packing under the asumption that the channel order is reversed (reversed channel order is
// BGR as opposed to RGB).
template<class P> inline bool rev_mask_match(const XVisualInfo& info) noexcept
{
    using packing_type = P;
    static_assert(packing_type::num_fields == 3);
    using word_type = decltype(info.red_mask + info.green_mask + info.blue_mask);
    return (info.red_mask   == image::get_bit_field_mask<word_type>(packing_type::fields, 3, 2) &&
            info.green_mask == image::get_bit_field_mask<word_type>(packing_type::fields, 3, 1) &&
            info.blue_mask  == image::get_bit_field_mask<word_type>(packing_type::fields, 3, 0));
}


auto make_lum_image(image::Size size)
{
    using word_type = char;
    constexpr int bits_per_word = 8;
    using format_type = image::IntegerPixelFormat_Lum<word_type, bits_per_word>;
    return std::make_unique<image::BufferedImage<format_type>>(size); // Throws
}


template<class T, class P, int N, bool R> auto make_packed_image(image::Size size)
{
    using compound_type = T;
    using packing_type = P;
    constexpr int bytes_per_pixel = N;
    constexpr bool reverse_channel_order = R;

    using word_type = char;
    constexpr int bits_per_word = 8;
    constexpr core::Endianness word_order = core::Endianness::little;
    constexpr bool alpha_channel_first = false;

    using format_type = image::PackedPixelFormat<image::ChannelSpec_RGB, compound_type, packing_type, word_type,
                                                 bits_per_word, bytes_per_pixel, word_order, alpha_channel_first,
                                                 reverse_channel_order>;

    return std::make_unique<image::BufferedImage<format_type>>(size); // Throws
}


struct BitFields {
    int red_width,   red_shift;
    int green_width, green_shift;
    int blue_width,  blue_shift;
};


template<class P> inline void record_bit_fields(BitFields& fields) noexcept
{
    using packing_type = P;
    static_assert(packing_type::num_fields == 3);
    fields.red_width   = image::get_bit_field_width(packing_type::fields, 3, 0);
    fields.red_shift   = image::get_bit_field_shift(packing_type::fields, 3, 0);
    fields.green_width = image::get_bit_field_width(packing_type::fields, 3, 1);
    fields.green_shift = image::get_bit_field_shift(packing_type::fields, 3, 1);
    fields.blue_width  = image::get_bit_field_width(packing_type::fields, 3, 2);
    fields.blue_shift  = image::get_bit_field_shift(packing_type::fields, 3, 2);
}


template<class P> inline void record_rev_bit_fields(BitFields& fields) noexcept
{
    using packing_type = P;
    static_assert(packing_type::num_fields == 3);
    fields.red_width   = image::get_bit_field_width(packing_type::fields, 3, 2);
    fields.red_shift   = image::get_bit_field_shift(packing_type::fields, 3, 2);
    fields.green_width = image::get_bit_field_width(packing_type::fields, 3, 1);
    fields.green_shift = image::get_bit_field_shift(packing_type::fields, 3, 1);
    fields.blue_width  = image::get_bit_field_width(packing_type::fields, 3, 0);
    fields.blue_shift  = image::get_bit_field_shift(packing_type::fields, 3, 0);
}


// This function assumes that the specified fields reflect the visual that is associated
// with the specified colormap. This function also assumes that all colormap entries have
// been allocated writable. This function also assumes that all three specified widths are
// strictly greater than zero, and that the sum of them is strictly less than 16.
void setup_direct_color_colormap(Display* dpy, Colormap colormap, const BitFields& fields, bool weird)
{
    ARCHON_ASSERT(fields.red_width > 0   && fields.red_width < 16);
    ARCHON_ASSERT(fields.green_width > 0 && fields.green_width < 16);
    ARCHON_ASSERT(fields.blue_width > 0  && fields.blue_width < 16);

    using ushort = unsigned short;
    using ulong  = unsigned long;

    // FIXME: Consider filling any unused entries components with zero (if one or more
    // channels use a number of bits, N, such that 2^N is less than the number of entrioes
    // in the colormap)    

    // FIXME: Consider updating all three channels at the same time    

    {
        int n = 1 << fields.red_width;
        auto colors = std::make_unique<XColor[]>(n); // Throws
        for (int i = 0; i < n; ++i) {
            // FIXME: Is this up-scaling scheme correct according to X11 specification?                             
            int val = i << (16 - fields.red_width);
            if (weird)
                val = 65535 - val;
            XColor& color = colors[i];
            color.pixel = ulong(i) << fields.red_shift;
            color.red = ushort(val);
            color.flags = DoRed;
        }
        XStoreColors(dpy, colormap, colors.get(), n);
    }
    {
        int n = 1 << fields.green_width;
        auto colors = std::make_unique<XColor[]>(n); // Throws
        for (int i = 0; i < n; ++i) {
            // FIXME: Is this up-scaling scheme correct according to X11 specification?                             
            int val = i << (16 - fields.green_width);
            XColor& color = colors[i];
            color.pixel = ulong(i) << fields.green_shift;
            color.green = ushort(val);
            color.flags = DoGreen;
        }
        XStoreColors(dpy, colormap, colors.get(), n);
    }
    {
        int n = 1 << fields.blue_width;
        auto colors = std::make_unique<XColor[]>(n); // Throws
        for (int i = 0; i < n; ++i) {
            // FIXME: Is this up-scaling scheme correct according to X11 specification?                             
            int val = i << (16 - fields.blue_width);
            XColor& color = colors[i];
            color.pixel = ulong(i) << fields.blue_shift;
            color.blue = ushort(val);
            color.flags = DoBlue;
        }
        XStoreColors(dpy, colormap, colors.get(), n);
    }
}


// This function assumes that the specified colormap has 2^N entries, where N is the
// specified width. This function also assumes that all colormap entries have been allocated
// writable. This function also assumes that the specified width is strictly greater than 0
// and strictly less than 16.
void setup_gray_scale_colormap(Display* dpy, Colormap colormap, int width, bool weird)
{
    ARCHON_ASSERT(width > 0 && width < 16);

    using ushort = unsigned short;
    using ulong  = unsigned long;

    int n = 1 << width;
    auto colors = std::make_unique<XColor[]>(n); // Throws
    for (int i = 0; i < n; ++i) {
        // FIXME: Is this up-scaling scheme correct according to X11 specification?                                 
        int val = i << (16 - width);
        if (weird)
            val = 65535 - val;
        XColor& color = colors[i];
        color.pixel = ulong(i);
        color.red   = ushort(val);
        color.green = ushort(val);
        color.blue  = ushort(val);
        color.flags = DoRed | DoGreen | DoBlue;
    }
    XStoreColors(dpy, colormap, colors.get(), n);
}


// This function assumes that the specified colormap has 2^N entries, where N is the sum of
// the three specified widths. This function also assumes that all colormap entries have
// been allocated writable. This function also assumes that all three specified widths are
// strictly greater than zero, and that the sum of them is strictly less than 16.
void setup_pseudo_color_colormap(Display* dpy, Colormap colormap, int red_width, int green_width, int blue_width,
                                 bool weird)
{
    ARCHON_ASSERT(red_width > 0);
    ARCHON_ASSERT(green_width > 0);
    ARCHON_ASSERT(blue_width > 0);
    int width = red_width + green_width + blue_width;
    ARCHON_ASSERT(width < 16);

    using ushort = unsigned short;
    using ulong  = unsigned long;

    int n = 1 << width;
    auto colors = std::make_unique<XColor[]>(n); // Throws
    int red_shift   = green_width + blue_width;
    int green_shift = blue_width;
    int blue_shift  = 0;
    for (int i = 0; i < n; ++i) {
        // FIXME: Is this up-scaling scheme correct according to X11 specification?                                 
        int red_val   = ((i >> red_shift)   << (16 - red_width))   & 65535;
        int green_val = ((i >> green_shift) << (16 - green_width)) & 65535;
        int blue_val  = ((i >> blue_shift)  << (16 - blue_width))  & 65535;
        if (weird)
            red_val = 65535 - red_val;
        XColor& color = colors[i];
        color.pixel = ulong(i);
        color.red   = ushort(red_val);
        color.green = ushort(green_val);
        color.blue  = ushort(blue_val);
        color.flags = DoRed | DoGreen | DoBlue;
    }
    XStoreColors(dpy, colormap, colors.get(), n);
}


} // unnamed namespace



int main(int argc, char* argv[])
{
    std::locale locale("");

    namespace fs = std::filesystem;
    std::optional<fs::path> optional_path;
    bool list_visuals = false;
    bool list_pixmap_formats = false;
    int num_windows = 0;
    std::optional<int> optional_visual_depth;
    std::optional<display::ConnectionConfigX11::VisualClass> optional_visual_class;
    std::optional<VisualID> optional_visual_type;
    bool disable_double_buffering = false;
    bool use_weird_palette = false;
    bool install_colormap = false;
    bool disable_detectable_autorepeat = false;
    std::optional<display::Pos> optional_pos;
    log::LogLevel log_level_limit = log::LogLevel::warn;
    bool report_mouse_move = false;
    bool use_synchronous_mode = false;

    cli::Spec spec;
    pat("[<path>]", cli::no_attributes, spec,
        "Lorem ipsum.",
        std::tie(optional_path)); // Throws

    opt(cli::help_tag, spec); // Throws
    opt(cli::stop_tag, spec); // Throws

    opt("-L, --list-visuals", "", cli::no_attributes, spec,
        "List the supported X11 visuals.",
        cli::raise_flag(list_visuals)); // Throws

    opt("-M, --list-pixmap-formats", "", cli::no_attributes, spec,
        "List the supported ZPixmap formats.",
        cli::raise_flag(list_pixmap_formats)); // Throws

    opt("-n, --num-windows", "<num>", cli::no_attributes, spec,
        "The number of windows to be opened. The default number is @V.",
        std::tie(num_windows)); // Throws

    opt("-d, --visual-depth", "<num>", cli::no_attributes, spec,
        "Pick a visual of the specified depth (@A).",
        cli::assign(optional_visual_depth)); // Throws

    opt("-c, --visual-class", "<name>", cli::no_attributes, spec,
        "Pick a visual of the specified class (@A). The class can be \"StaticGray\", \"GrayScale\", \"StaticColor\", "
        "\"PseudoColor\", \"TrueColor\", or \"DirectColor\".",
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

    opt("-D, --disable-double-buffering", "", cli::no_attributes, spec,
        "Disable use of double buffering, even when the selected visual supports double buffering.",
        cli::raise_flag(disable_double_buffering)); // Throws

    opt("-w, --use-weird-palette", "", cli::no_attributes, spec,
        "Use a weird (non-standard) palette when using a visual that allows for palette mutation (`PseudoColor`, "
        "`GrayScale`, and `DirectColor`).",
        cli::raise_flag(use_weird_palette)); // Throws

    opt("-i, --install-colormap", "", cli::no_attributes, spec,
        "Install the colormap, i.e., make it current. This should only be done when there is no window manager.",
        cli::raise_flag(install_colormap)); // Throws

    opt("-A, --disable-detectable-autorepeat", "", cli::no_attributes, spec,
        "Do not enable detectable key auto-repeat mode even when it is supported.",
        cli::raise_flag(disable_detectable_autorepeat)); // Throws

    opt("-p, --pos", "<position>", cli::no_attributes, spec,
        "Specify the desired position of the windows. This may or may not be honored by the window manager. If no "
        "position is specified, the position will be determined by the window manager.",
        std::tie(optional_pos)); // Throws

    opt("-l, --log-level", "<level>", cli::no_attributes, spec,
        "Set the log level limit. The possible levels are \"off\", \"fatal\", \"error\", \"warn\", \"info\", "
        "\"detail\", \"debug\", \"trace\", and \"all\". The default limit is \"@V\".",
        std::tie(log_level_limit)); // Throws

    opt("-m, --report-mouse-move", "", cli::no_attributes, spec,
        "Turn on reporting of \"mouse move\" events.",
        cli::raise_flag(report_mouse_move)); // Throws

    opt("-s, --use-synchronous-mode", "", cli::no_attributes, spec,
        "Turn on X11's synchronous mode. In this mode, buffering of X protocol requests is turned off, and the Xlib "
        "functions, that generate X requests, wait for a response from the server before they return. This is "
        "sometimes useful when debugging.",
        cli::raise_flag(use_synchronous_mode)); // Throws

    int exit_status = 0;
    if (ARCHON_UNLIKELY(cli::process(argc, argv, spec, exit_status, locale))) // Throws
        return exit_status;

    log::FileLogger root_logger(core::File::get_cout(), locale); // Throws
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

    // Load image
    std::unique_ptr<image::WritableImage> img;
    {
        fs::path path;
        if (optional_path.has_value()) {
            path = std::move(optional_path.value());
        }
        else {
            path = resource_path / core::make_fs_path_generic("image.png", locale);
        }
        image::LoadConfig load_config;
        log::PrefixLogger load_logger(logger, "Load: "); // Throws
        load_config.logger = &load_logger;
        std::error_code ec;
        if (!image::try_load(path, img, locale, load_config, ec)) { // Throws
            logger.error("Failed to load image: %s", ec.message()); // Throws
            return EXIT_FAILURE;
        }
    }
    image::Size img_size = img->get_size();

    // Connect to display
    Display* dpy = XOpenDisplay(nullptr);
    ARCHON_STEADY_ASSERT(dpy);
    ARCHON_SCOPE_EXIT {
        XCloseDisplay(dpy);
    };
    int screen = DefaultScreen(dpy);
    Window root = RootWindow(dpy, screen);
    unsigned long black = BlackPixel(dpy, screen);

    if (ARCHON_UNLIKELY(use_synchronous_mode))
        XSynchronize(dpy, True);

#if HAVE_XDBE
    bool have_xdbe = false;
    int xdbe_major = 0;
    int xdbe_minor = 0;
    if (ARCHON_LIKELY(XdbeQueryExtension(dpy, &xdbe_major, &xdbe_minor))) {
        if (ARCHON_LIKELY(xdbe_major >= 1))
            have_xdbe = true;
    }
#endif // HAVE_XDBE

    bool have_xkb = false;
    int xkb_major = 0;
    int xkb_minor = 0;
    int xkb_lib_major = XkbMajorVersion;
    int xkb_lib_minor = XkbMinorVersion;
    if (XkbLibraryVersion(&xkb_lib_major, &xkb_lib_minor)) {
        int opcode = 0;
        int event_base = 0;
        int error_base = 0;
        if (ARCHON_LIKELY(XkbQueryExtension(dpy, &opcode, &event_base, &error_base, &xkb_major, &xkb_minor))) {
            if (ARCHON_LIKELY(xkb_major >= 1))
                have_xkb = true;
        }
    }
    bool detectable_autorepeat_enabled = false;
    if (ARCHON_LIKELY(have_xkb && !disable_detectable_autorepeat)) {
        Bool detectable = True;
        Bool supported = {};
        XkbSetDetectableAutoRepeat(dpy, detectable, &supported);
        if (ARCHON_LIKELY(supported))
            detectable_autorepeat_enabled = true;
    }

#if HAVE_XRANDR
    bool have_xrandr = false;
    int xrandr_event_base = 0;
    int xrandr_error_base = 0;
    int xrandr_major = 0;
    int xrandr_minor = 0;
    if (ARCHON_LIKELY(XRRQueryExtension(dpy, &xrandr_event_base, &xrandr_error_base))) {
        Status status = XRRQueryVersion(dpy, &xrandr_major, &xrandr_minor);
        ARCHON_STEADY_ASSERT(status != 0);
        if (ARCHON_LIKELY(xrandr_major > 1 || (xrandr_major == 1 && xrandr_minor >= 5)))
            have_xrandr = true;
    }
    if (ARCHON_LIKELY(have_xrandr)) {
        int mask = RROutputChangeNotifyMask | RRCrtcChangeNotifyMask;
        XRRSelectInput(dpy, root, mask);
    }
#endif // HAVE_XRANDR

#if HAVE_XRENDER
    bool have_xrender = false;
    int xrender_event_base = 0;
    int xrender_error_base = 0;
    int xrender_major = 0;
    int xrender_minor = 0;
    if (ARCHON_LIKELY(XRenderQueryExtension(dpy, &xrender_event_base, &xrender_error_base))) {
        Status status = XRenderQueryVersion(dpy, &xrender_major, &xrender_minor);
        ARCHON_STEADY_ASSERT(status != 0);
        if (ARCHON_LIKELY(xrender_major > 0 || (xrender_major == 0 && xrender_minor >= 7)))
            have_xrender = true;
    }
#endif // HAVE_XRENDER

#if HAVE_GLX
    bool have_glx = false;
    int glx_error_base = 0;
    int glx_event_base = 0;
    int glx_major = 0;
    int glx_minor = 0;
    if (ARCHON_LIKELY(glXQueryExtension(dpy, &glx_error_base, &glx_event_base))) {
        Bool success = glXQueryVersion(dpy, &glx_major, &glx_minor);
        ARCHON_STEADY_ASSERT(success);
        if (ARCHON_LIKELY(glx_major > 1 || (glx_major == 1 && glx_minor >= 4)))
            have_glx = true;
    }
#endif // HAVE_GLX

    // Fetch information about supported visuals
    struct VisualSpec {
        XVisualInfo info;
        bool double_buffered;
        bool opengl_supported;
        bool opengl_double_buffered;
        bool opengl_stereo;
        int double_buffered_perflevel;
        int opengl_level;
        int opengl_num_aux_buffers;
        int opengl_depth_buffer_bits;
        int opengl_stencil_buffer_bits;
        int opengl_accum_buffer_bits;
    };
    core::Slab<VisualSpec> visual_specs;
    {
        core::FlatMap<std::tuple<int, int, VisualID>, int> double_buffered_visuals;
#if HAVE_XDBE
        if (ARCHON_LIKELY(have_xdbe)) {
            int n = 0;
            XdbeScreenVisualInfo* entries = XdbeGetVisualInfo(dpy, nullptr, &n);
            ARCHON_STEADY_ASSERT(entries);
            ARCHON_STEADY_ASSERT(n == int(ScreenCount(dpy)));
            ARCHON_SCOPE_EXIT {
                XdbeFreeVisualInfo(entries);
            };
            for (int i = 0; i < n; ++i) {
                XdbeScreenVisualInfo& entry = entries[i];
                for (int j = 0; j < entry.count; ++j) {
                    XdbeVisualInfo& subentry = entry.visinfo[j];
                    auto p = double_buffered_visuals.emplace(std::make_tuple(i, subentry.depth, subentry.visual),
                                                             subentry.perflevel); // Throws
                    bool was_inserted = p.second;
                    ARCHON_STEADY_ASSERT(was_inserted);
                }
            }
        }
#endif // HAVE_XDBE
        int n = {};
        long vinfo_mask = 0;
        XVisualInfo vinfo_template = {};
        XVisualInfo* entries = XGetVisualInfo(dpy, vinfo_mask, &vinfo_template, &n);
        if (ARCHON_LIKELY(entries)) {
            ARCHON_SCOPE_EXIT {
                XFree(entries);
            };
            std::size_t n_2 = {};
            core::int_cast(n , n_2); // Throws
            visual_specs.recreate(n_2); // Throws
            for (std::size_t i = 0; i < n_2; ++i) {
                XVisualInfo& info = entries[i];
                bool double_buffered = false;
                int double_buffered_perflevel = 0;
                {
                    auto i = double_buffered_visuals.find(std::make_tuple(info.screen, info.depth, info.visualid));
                    if (i != double_buffered_visuals.end()) {
                        double_buffered = true;
                        double_buffered_perflevel = i->second;
                    }
                }
                bool opengl_supported = false;
                int opengl_level = 0;
                bool opengl_double_buffered = false;
                bool opengl_stereo = false;
                int opengl_num_aux_buffers = 0;
                int opengl_depth_buffer_bits = 0;
                int opengl_stencil_buffer_bits = 0;
                int opengl_accum_buffer_bits = 0;
#if HAVE_GLX
                if (ARCHON_LIKELY(have_glx)) {
                    auto get = [&](int attrib) noexcept {
                        int value = {};
                        int ret = glXGetConfig(dpy, &info, attrib, &value);
                        ARCHON_STEADY_ASSERT(ret == 0);
                        return value;
                    };
                    if (get(GLX_USE_GL) != 0) {
                        opengl_supported           = true;
                        opengl_level               = get(GLX_LEVEL);
                        opengl_double_buffered     = (get(GLX_DOUBLEBUFFER) != 0);
                        opengl_stereo              = (get(GLX_STEREO) != 0);
                        opengl_num_aux_buffers     = get(GLX_AUX_BUFFERS);
                        opengl_depth_buffer_bits   = get(GLX_DEPTH_SIZE);
                        opengl_stencil_buffer_bits = get(GLX_STENCIL_SIZE);
                        opengl_accum_buffer_bits   = (get(GLX_ACCUM_RED_SIZE) + get(GLX_ACCUM_GREEN_SIZE) +
                                                      get(GLX_ACCUM_BLUE_SIZE) + get(GLX_ACCUM_ALPHA_SIZE));
                    }
                }
#endif // HAVE_GLX
                VisualSpec spec = {
                    info,
                    double_buffered,
                    opengl_supported,
                    opengl_double_buffered,
                    opengl_stereo,
                    double_buffered_perflevel,
                    opengl_level,
                    opengl_num_aux_buffers,
                    opengl_depth_buffer_bits,
                    opengl_stencil_buffer_bits,
                    opengl_accum_buffer_bits,
                };
                visual_specs.add(spec);
            }
        }
    }

    // Fetch ZPixmap formats
    core::FlatMap<int, XPixmapFormatValues> pixmap_formats;
    {
        int n = 0;
        XPixmapFormatValues* entries = XListPixmapFormats(dpy, &n);
        if (!entries)
            throw std::runtime_error("XListPixmapFormats() failed");
        ARCHON_SCOPE_EXIT {
            XFree(entries);
        };
        std::size_t n_2 = {};
        core::int_cast(n, n_2); // Throws
        pixmap_formats.reserve(n_2); // Throws
        for (std::size_t i = 0; i < n_2; ++i) {
            XPixmapFormatValues& format = entries[i];
            pixmap_formats[format.depth] = format;
        }
    }

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

    // List supported visuals
    if (list_visuals) {
        std::size_t n = visual_specs.size();
        for (std::size_t i = 0; i < n; ++i) {
            const VisualSpec& spec = visual_specs[i];
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
                        "red_mask = 0x%s, green_mask = 0x%s, blue_mask = 0x%s, colormap_size = %s, bits_per_rgb = %s, "
                        "double_buffered = %s, supports_opengl = %s, opengl_level = %s, opengl_double_buffered = %s, "
                        "opengl_stereo = %s, opengl_num_aux_buffers = %s, opengl_depth_buffer_bits = %s, "
                        "opengl_stencil_buffer_bits = %s, opengl_accum_buffer_bits = %s", i + 1,
                        core::as_flex_int_h(info.visualid), core::as_int(info.screen), core::as_int(info.depth),
                        get_visual_class_name(info.c_class), core::as_hex_int(info.red_mask),
                        core::as_hex_int(info.green_mask), core::as_hex_int(info.blue_mask),
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
    VisualID default_visual = XVisualIDFromVisual(DefaultVisual(dpy, screen));
    VisualSpec visual_spec;
    {
        const std::optional<int> depth_override = optional_visual_depth;
        const std::optional<int> class_override = impl::map_opt_visual_class(optional_visual_class);
        const std::optional<VisualID> visual_override = optional_visual_type;
        bool prefer_default_visual_type = true;             
        bool prefer_default_visual_depth = true;             
        bool prefer_double_buffered = !disable_double_buffering;
        bool require_opengl = false;
        bool require_opengl_depth_buffer = require_opengl;             
        bool require_opengl_stencil_buffer = false;             
        bool require_opengl_accum_buffer = false;             
        int min_opengl_depth_buffer_bits = 8;             
        int min_opengl_stencil_buffer_bits = 1;             
        int min_opengl_accum_buffer_bits = 32;             
        auto filter = [&](const VisualSpec& spec) {
            if (depth_override.has_value() && spec.info.depth != depth_override.value())
                return false;
            if (class_override.has_value() && spec.info.c_class != class_override.value())
                return false;
            if (visual_override.has_value() && spec.info.visualid != visual_override.value())
                return false;
            if (require_opengl && !spec.opengl_supported)
                return false;
            if (spec.opengl_level != 0)
                return false;
            if (spec.opengl_stereo)
                return false;
            if (require_opengl_depth_buffer && spec.opengl_depth_buffer_bits < min_opengl_depth_buffer_bits)
                return false;
            if (require_opengl_stencil_buffer && spec.opengl_stencil_buffer_bits < min_opengl_stencil_buffer_bits)
                return false;
            if (require_opengl_accum_buffer && spec.opengl_accum_buffer_bits < min_opengl_accum_buffer_bits)
                return false;
            return true;
        };
        auto get_class_value = [](int class_) {
            switch (class_) {
                case StaticGray:
                    return 1;
                case GrayScale:
                    return 0;
                case StaticColor:
                    return 3;
                case PseudoColor:
                    return 2;
                case TrueColor:
                    return 5;
                case DirectColor:
                    return 4;
            }
            ARCHON_ASSERT_UNREACHABLE();
            return 0;
        };
        int default_depth = DefaultDepth(dpy, screen);
        auto less = [&](const VisualSpec& a, const VisualSpec& b) {
            constexpr int max_criteria = 18;
            int values_1[max_criteria];
            int values_2[max_criteria];
            int num_criteria = 0;

            // Criterion 1: Prefer default visual
            if (prefer_default_visual_type) {
                int i = num_criteria++;
                values_1[i] = int(a.info.visualid == default_visual);
                values_2[i] = int(b.info.visualid == default_visual);
            }

            // Critera 2 and 3: Prefer default depth
            if (prefer_default_visual_depth) {
                int i_1 = num_criteria++;
                int i_2 = num_criteria++;
                values_1[i_1] = 0;
                values_1[i_2] = 0;
                if (a.info.depth >= default_depth) {
                    values_1[i_1] = 1;
                    values_1[i_2] = -a.info.depth; // Non-positive
                }
                values_2[i_1] = 0;
                values_2[i_2] = 0;
                if (b.info.depth >= default_depth) {
                    values_2[i_1] = 1;
                    values_2[i_2] = -b.info.depth; // Non-positive
                }
            }

            // Criterion 4: Best class
            {
                int i = num_criteria++;
                values_1[i] = get_class_value(a.info.c_class);
                values_2[i] = get_class_value(b.info.c_class);
            }

            // Criterion 5: Prefer double buffered
            if (prefer_double_buffered) {
                int i = num_criteria++;
                values_1[i] = int(a.double_buffered);
                values_2[i] = int(b.double_buffered);
            }

            // Criterion 6: Prefer OpenGL double buffered
            if (require_opengl) {
                int i = num_criteria++;
                values_1[i] = int(a.opengl_double_buffered);
                values_2[i] = int(b.opengl_double_buffered);
            }

            // Criterion 7: Greatest depth
            {
                int i = num_criteria++;
                values_1[i] = a.info.depth;
                values_2[i] = b.info.depth;
            }

            // Criterion 8: Highest depth buffer bit width
            if (require_opengl_depth_buffer) {
                int i = num_criteria++;
                values_1[i] = a.opengl_depth_buffer_bits;
                values_2[i] = b.opengl_depth_buffer_bits;
            }

            // Criterion 9: Highest stencil buffer bit width
            if (require_opengl_stencil_buffer) {
                int i = num_criteria++;
                values_1[i] = a.opengl_stencil_buffer_bits;
                values_2[i] = b.opengl_stencil_buffer_bits;
            }

            // Criterion 10: Highest accumulation buffer bit width
            if (require_opengl_accum_buffer) {
                int i = num_criteria++;
                values_1[i] = a.opengl_accum_buffer_bits;
                values_2[i] = b.opengl_accum_buffer_bits;
            }

            // Criterion 11: Highest double buffer performance
            if (prefer_double_buffered) {
                int i = num_criteria++;
                values_1[i] = a.double_buffered_perflevel;
                values_2[i] = b.double_buffered_perflevel;
            }

            // Criterion 12: Prefer not double buffered
            if (!prefer_double_buffered) {
                int i = num_criteria++;
                values_1[i] = -int(a.double_buffered);
                values_2[i] = -int(b.double_buffered);
            }

            // Criterion 13: Prefer not OpenGL double buffered
            if (!require_opengl) {
                int i = num_criteria++;
                values_1[i] = -int(a.opengl_double_buffered);
                values_2[i] = -int(b.opengl_double_buffered);
            }

            // Criterion 14: Lowest depth buffer bit width
            if (!require_opengl_depth_buffer) {
                int i = num_criteria++;
                values_1[i] = -a.opengl_depth_buffer_bits;
                values_2[i] = -b.opengl_depth_buffer_bits;
            }

            // Criterion 15: Lowest stencil buffer bit width
            if (!require_opengl_stencil_buffer) {
                int i = num_criteria++;
                values_1[i] = -a.opengl_stencil_buffer_bits;
                values_2[i] = -b.opengl_stencil_buffer_bits;
            }

            // Criterion 16: Lowest accumulation buffer bit width
            if (!require_opengl_accum_buffer) {
                int i = num_criteria++;
                values_1[i] = -a.opengl_accum_buffer_bits;
                values_2[i] = -b.opengl_accum_buffer_bits;
            }

            // Criterion 17: Lowest number of OpenGL auxiliary buffers
            {
                int i = num_criteria++;
                values_1[i] = -a.opengl_num_aux_buffers;
                values_2[i] = -b.opengl_num_aux_buffers;
            }

            // Criterion 18: Prefer no OpenGL support
            if (!require_opengl) {
                int i = num_criteria++;
                values_1[i] = (a.opengl_supported ? 0 : 1);
                values_2[i] = (b.opengl_supported ? 0 : 1);
            }

            ARCHON_ASSERT(num_criteria <= max_criteria);
            return std::lexicographical_compare(values_1, values_1 + num_criteria, values_2, values_2 + num_criteria);
        };
        const VisualSpec* best = nullptr;
        for (const VisualSpec& spec : visual_specs) {
            bool have_new_best = (filter(spec) && (!best || less(*best, spec)));
            if (ARCHON_LIKELY(!have_new_best))
                continue;
            best = &spec;
        }
        if (ARCHON_UNLIKELY(!best)) {
            logger.error("No suitable X11 visual found");
            return EXIT_FAILURE;
        }
        visual_spec = *best;
    }
    const XVisualInfo& visual_info = visual_spec.info;
    int depth = visual_info.depth;
    VisualID visualid = visual_info.visualid;
    bool use_double_buffering = visual_spec.double_buffered;
    int bits_per_pixel = 0;
    {
        auto i = pixmap_formats.find(depth);
        ARCHON_STEADY_ASSERT(i != pixmap_formats.end());
        const XPixmapFormatValues& format = i->second;
        bits_per_pixel = format.bits_per_pixel;
    }

    // Ensure a colormap
    //
    // When a colormap is used on a window, the colormap and window must be associated with
    // the same visual type. Thefore, if the window uses a non-default visual type, it
    // cannot use the default colormap.
    //
    Colormap colormap;
    bool colormap_owned = false;
    bool colormap_needs_init = false;
    ARCHON_SCOPE_EXIT {
        if (colormap_owned)
            XFreeColormap(dpy, colormap);
    };
    if (ARCHON_LIKELY(visualid == default_visual)) {
        colormap = DefaultColormap(dpy, screen);
    }
    else {
        bool found = false;
        {
            // See command `xstdcmap` for a way to set up the standard colormaps
            XStandardColormap* std_colormap = {};
            int count = {};
            Status status = XGetRGBColormaps(dpy, root, &std_colormap, &count, XA_RGB_DEFAULT_MAP);
            if (status != 0) {
                ARCHON_SCOPE_EXIT {
                    XFree(std_colormap);
                };
                core::NumOfSpec standard_colormaps_spec = { "standard colormap", "standard colormaps" };
                logger.info("Found %s", core::as_num_of(count, standard_colormaps_spec)); // Throws
                for (int i = 0; i < count; ++i) {
                    const XStandardColormap& entry = std_colormap[i];
                    if (ARCHON_LIKELY(entry.visualid != visualid))
                        continue;
                    colormap = entry.colormap;
                    found = true;
                    break;
                }
            }
        }
        if (!found) {
            bool nonstatic = false;
            switch (visual_info.c_class) {
                case GrayScale:
                case PseudoColor:
                case DirectColor:
                    nonstatic = true;
            }
            colormap = XCreateColormap(dpy, root, visual_info.visual, (nonstatic ? AllocAll : AllocNone));
            colormap_owned = true;
            if (nonstatic)
                colormap_needs_init = true;
        }
    }

    auto format_have_xdbe = [&](std::ostream& out) {
#if HAVE_XDBE
        if (have_xdbe) {
            out << core::formatted("yes (%s.%s)", xdbe_major, xdbe_minor); // Throws
            return;
        }
#endif // HAVE_XDBE
        out << "no"; // Throws
    };

    auto format_have_xkb = [&](std::ostream& out) {
        if (have_xkb) {
            out << core::formatted("yes (%s.%s, %s.%s)", xkb_major, xkb_minor, xkb_lib_major, xkb_lib_minor); // Throws
            return;
        }
        out << "no"; // Throws
    };

    auto format_have_xrandr = [&](std::ostream& out) {
#if HAVE_XRANDR
        if (have_xrandr) {
            out << core::formatted("yes (%s.%s)", xrandr_major, xrandr_minor); // Throws
            return;
        }
#endif // HAVE_XRANDR
        out << "no"; // Throws
    };

    auto format_have_xrender = [&](std::ostream& out) {
#if HAVE_XRENDER
        if (have_xrender) {
            out << core::formatted("yes (%s.%s)", xrender_major, xrender_minor); // Throws
            return;
        }
#endif // HAVE_XRENDER
        out << "no"; // Throws
    };

    auto format_have_glx = [&](std::ostream& out) {
#if HAVE_GLX
        if (have_glx) {
            out << core::formatted("yes (%s.%s)", glx_major, glx_minor); // Throws
            return;
        }
#endif // HAVE_GLX
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
    logger.info("Default visual of screen:           0x%s",
                core::as_hex_int(XVisualIDFromVisual(DefaultVisual(dpy, screen)))); // Throws
    logger.info("Selected visual:                    0x%s", core::as_hex_int(visualid)); // Throws
    logger.info("Class of selected visual:           %s", get_visual_class_name(visual_info.c_class)); // Throws
    logger.info("Detectable auto-repeat enabled:     %s", (detectable_autorepeat_enabled ? "yes" : "no")); // Throws
    logger.info("Use double buffering:               %s", (use_double_buffering ? "yes" : "no")); // Throws

    if (ARCHON_UNLIKELY(!have_xkb))
        throw std::runtime_error("Required X Keyboard Extension is not available");

    auto intern_string = [&](const char* string) noexcept -> Atom {
        Atom atom = XInternAtom(dpy, string, False);
        ARCHON_STEADY_ASSERT(atom != None);
        return atom;
    };

#if HAVE_XRANDR
    struct ProtoScreen {
        core::IndexRange output_name;
        display::Box bounds;
        std::optional<core::IndexRange> monitor_name;
        std::optional<display::Resolution> resolution;
        std::optional<double> refresh_rate;
    };
    std::vector<ProtoScreen> screens;
    core::Buffer<char> screens_string_buffer;
    std::size_t screens_string_buffer_used_size = 0;
    Atom atom_edid = intern_string(RR_PROPERTY_RANDR_EDID);
    impl::EdidParser edid_parser(locale);
    auto try_update_display_info = [&](bool& changed) -> bool {
        XRRScreenResources* resources = XRRGetScreenResourcesCurrent(dpy, root);
        if (ARCHON_UNLIKELY(!resources))
            throw std::runtime_error("XRRGetScreenResourcesCurrent() failed");
        ARCHON_SCOPE_EXIT {
            XRRFreeScreenResources(resources);
        };
        struct Crtc {
            bool enabled;
            display::Box bounds;
            std::optional<double> refresh_rate;
        };
        core::FlatMap<RRCrtc, Crtc, 16> crtcs;
        crtcs.reserve(std::size_t(resources->ncrtc)); // Throws
        auto ensure_crtc = [&](RRCrtc id) -> const Crtc* {
            auto i = crtcs.find(id);
            if (i != crtcs.end())
                return &i->second;
            XRRCrtcInfo* info = XRRGetCrtcInfo(dpy, resources, id);
            if (ARCHON_UNLIKELY(!info))
                return nullptr;
            ARCHON_SCOPE_EXIT {
                XRRFreeCrtcInfo(info);
            };
            bool enabled = (info->mode != None);
            display::Size size = {};
            core::int_cast(info->width, size.width); // Throws
            core::int_cast(info->height, size.height); // Throws
            display::Box bounds = { display::Pos(info->x, info->y), size };
            std::optional<double> refresh_rate;
            if (enabled) {
                bool found = false;
                for (int j = 0; j < resources->nmode; ++j) {
                    const XRRModeInfo& mode = resources->modes[j];
                    if (ARCHON_LIKELY(mode.id != info->mode))
                        continue;
                    found = true;
                    if (mode.dotClock != 0)
                        refresh_rate = mode.dotClock / (mode.hTotal * double(mode.vTotal));
                    break;
                }
                ARCHON_ASSERT(found);
            }
            ARCHON_STEADY_ASSERT(crtcs.size() < crtcs.capacity());
            Crtc& crtc =  crtcs[id];
            crtc = { enabled, bounds, refresh_rate };
            return &crtc;
        };
        core::Vector<ProtoScreen, 16> new_screens;
        std::array<char, 16 * 24> strings_seed_memory = {};
        core::Buffer strings_buffer(strings_seed_memory);
        core::StringBufferContents strings(strings_buffer);
        for (int i = 0; i < resources->noutput; ++i) {
            RROutput id = resources->outputs[i];
            XRROutputInfo* info = XRRGetOutputInfo(dpy, resources, id);
            if (ARCHON_UNLIKELY(!info))
                return false;
            ARCHON_SCOPE_EXIT {
                XRRFreeOutputInfo(info);
            };
            // Note: Treating RR_UnknownConnection same as RR_Connected
            bool connected = (info->connection != RR_Disconnected);
            if (!connected || info->crtc == None)
                continue;
            const Crtc* crtc = ensure_crtc(info->crtc); // Throws
            if (ARCHON_UNLIKELY(!crtc))
                return false;
            if (!crtc->enabled)
                continue;
            // FIXME: Consider character encoding in output name                
            std::size_t offset = strings.size();
            std::size_t size = std::size_t(info->nameLen);
            strings.append({ info->name, size }); // Throws
            core::IndexRange output_name = { offset, size }; // Throws
            std::optional<display::Resolution> resolution;
            if (info->mm_width != 0 && info->mm_height != 0) {
                double horz_ppcm = crtc->bounds.size.width  / double(info->mm_width)  * 10;
                double vert_ppcm = crtc->bounds.size.height / double(info->mm_height) * 10;
                resolution = display::Resolution { horz_ppcm, vert_ppcm };
            }
            // Extract monitor name from EDID data when available
            std::optional<core::IndexRange> monitor_name;
            int nprop = {};
            Atom* props = XRRListOutputProperties(dpy, id, &nprop);
            if (ARCHON_LIKELY(props))  {
                ARCHON_SCOPE_EXIT {
                    XFree(props);
                };
                for (int j = 0; j < nprop; ++j) {
                    if (props[j] == atom_edid) {
                        long offset = 0;
                        long length = 128 / 4; // 128 bytes (32 longs) in basic EDID block
                        Bool _delete = False;
                        Bool pending = False;
                        Atom req_type = AnyPropertyType;
                        Atom actual_type = {};
                        int actual_format = {};
                        unsigned long nitems = {};
                        unsigned long bytes_after = {};
                        unsigned char* prop = {};
                        int ret = XRRGetOutputProperty(dpy, id, props[j], offset, length, _delete, pending, req_type,
                                                       &actual_type, &actual_format, &nitems, &bytes_after, &prop);
                        if (ARCHON_LIKELY(ret == Success)) {
                            ARCHON_SCOPE_EXIT {
                                XFree(prop);
                            };
                            if (ARCHON_LIKELY(actual_type == XA_INTEGER && actual_format == 8)) {
                                std::size_t size = {};
                                if (ARCHON_LIKELY(core::try_int_cast(nitems, size))) {
                                    std::string_view str = { reinterpret_cast<char*>(prop), size };
                                    impl::EdidInfo info = {};
                                    if (ARCHON_LIKELY(edid_parser.parse(str, info, strings))) // Throws
                                        monitor_name = info.monitor_name;
                                }
                            }
                        }
                    }
                }
            }
            ProtoScreen screen = { output_name, crtc->bounds, monitor_name, resolution, crtc->refresh_rate };
            new_screens.push_back(screen); // Throws
        }
        {
            const char* base_1 = strings.data();
            const char* base_2 = screens_string_buffer.data();
            auto cmp_opt_str = [&](const std::optional<core::IndexRange>& a,
                                   const std::optional<core::IndexRange>& b) {
                return (a.has_value() ?
                        b.has_value() && a.value().resolve_string(base_1) == b.value().resolve_string(base_2) :
                        !b.has_value());
            };
            auto cmp = [&](const ProtoScreen& a, const ProtoScreen& b) {
                return (a.bounds == b.bounds &&
                        a.resolution == b.resolution &&
                        a.refresh_rate == b.refresh_rate &&
                        a.output_name.resolve_string(base_1) == b.output_name.resolve_string(base_2) &&
                        cmp_opt_str(a.monitor_name, b.monitor_name));
            };
            if (std::equal(new_screens.begin(), new_screens.end(), screens.begin(), screens.end(), std::move(cmp))) {
                changed = false;
                return true;
            }
        }
        screens.reserve(new_screens.size()); // Throws
        screens_string_buffer.reserve(strings.size(), screens_string_buffer_used_size); // Throws
        // Non-throwing from here
        screens.clear();
        screens.insert(screens.begin(), new_screens.begin(), new_screens.end());
        screens_string_buffer.assign(strings);
        screens_string_buffer_used_size = strings.size();
        changed = true;
        return true;
    };
    auto update_display_info = [&] {
        int max_attempts = 16;
        for (int i = 0; i < max_attempts; ++i) {
            bool changed = false;
            if (ARCHON_LIKELY(try_update_display_info(changed)))
                return changed;
        }
        throw std::runtime_error("Failed to fetch screen configuration using XRandR within the allotted number of "
                                 "attempts");
    };
    auto dump_display_info = [&] {
        const char* strings_base = screens_string_buffer.data();
        std::size_t n = screens.size();
        for (std::size_t i = 0; i < n; ++i) {
            const ProtoScreen& screen = screens[i];
            auto format_monitor_name = [&](std::ostream& out) {
                if (ARCHON_LIKELY(screen.monitor_name.has_value())) {
                    out << core::quoted(screen.monitor_name.value().resolve_string(strings_base)); // Throws
                }
                else {
                    out << "unknown"; // Throws
                }
            };
            logger.info("Screen %s/%s: output_name=%s, bounds=%s, monitor_name=%s, resolution=%s, refresh_rate=%s",
                        i + 1, n, core::quoted(screen.output_name.resolve_string(strings_base)), screen.bounds,
                        core::as_format_func(format_monitor_name), core::as_optional(screen.resolution, "unknown"),
                        core::as_optional(screen.refresh_rate, "unknown")); // Throws
        }
    };
    if (ARCHON_LIKELY(have_xrandr)) {
        update_display_info(); // Throws
        dump_display_info(); // Throws
    }
#endif // HAVE_XRANDR

    // Create graphics context
    XGCValues gc_values = {};
    gc_values.graphics_exposures = False;
    GC gc = XCreateGC(dpy, root, GCGraphicsExposures, &gc_values);
    ARCHON_SCOPE_EXIT {
        XFreeGC(dpy, gc);
    };
    XSetForeground(dpy, gc, black);

    // Upload image
    Pixmap img_pixmap = XCreatePixmap(dpy, root, unsigned(img_size.width), unsigned(img_size.height), depth);
    ARCHON_SCOPE_EXIT {
        XFreePixmap(dpy, img_pixmap);
    };
    {
        // Setup X11 colormap when necessary and convert image to suitable pixel format.

        std::unique_ptr<image::WritableImage> img_2;
        char* data = nullptr;

        if (depth == 8) {
            if (bits_per_pixel != 8)
                goto unsupported_bits_per_pixel;
            constexpr int bytes_per_pixel = 1;
            if (visual_info.c_class == StaticGray || visual_info.c_class == GrayScale) {
                if (ARCHON_UNLIKELY(visual_info.colormap_size != 256))
                    goto unexpected_colormap_size;
                if (zero_mask_match(visual_info)) {
                    if (colormap_needs_init) {
                        ARCHON_ASSERT(visual_info.c_class == GrayScale);
                        setup_gray_scale_colormap(dpy, colormap, depth, use_weird_palette); // Throws
                    }
                    auto img = make_lum_image(img_size); // Throws
                    data = img->get_buffer().data();
                    img_2 = std::move(img);
                    goto matched;
                }
                goto unsupported_channel_masks;
            }
            if (visual_info.c_class == StaticColor) {
                if (ARCHON_UNLIKELY(visual_info.colormap_size != 256))
                    goto unexpected_colormap_size;
                // According to the X specification, masks only make sense for TrueColor and
                // DirectColor visuals. Despite that, it appears that some X servers choose
                // to expose the color structure of StaticColor visuals using masks, most
                // notably Xvfb + X.Org (e.g., using `Xvfb :1 -screen 0 1600x1200x8).
                if (norm_mask_match<image::ChannelPacking_332>(visual_info)) {
                    constexpr bool reverse_channel_order = false;
                    auto img = make_packed_image<image::int8_type, image::ChannelPacking_332, bytes_per_pixel,
                                                 reverse_channel_order>(img_size); // Throws
                    data = img->get_buffer().data();
                    img_2 = std::move(img);
                    goto matched;
                }
                if (rev_mask_match<image::ChannelPacking_233>(visual_info)) {
                    constexpr bool reverse_channel_order = true;
                    auto img = make_packed_image<image::int8_type, image::ChannelPacking_233, bytes_per_pixel,
                                                 reverse_channel_order>(img_size); // Throws
                    data = img->get_buffer().data();
                    img_2 = std::move(img);
                    goto matched;
                }
                if (zero_mask_match(visual_info)) {
                    int n = 1 << 8;
                    auto colors = std::make_unique<XColor[]>(n); // Throws
                    for (int i = 0; i < n; ++i) {
                        XColor& color = colors[i];
                        color.pixel = unsigned(i);
                    }
                    XQueryColors(dpy, colormap, colors.get(), n);
/*
                    for (int i = 0; i < n; ++i) {
                        const XColor& color = colors[i];
                        using comp_type = util::Color::comp_type;
                        util::Color color_2(comp_type(color.red   >> 8),
                                            comp_type(color.green >> 8),
                                            comp_type(color.blue  >> 8));
                        logger.info("Color %s: %s", core::as_int(i + 1), util::as_css_color(color_2));
                    }
*/
                    // Create image with indirect color     
                    // FIXME: Read out palette and produce indirect color version of image with respect to that palette                                        
                    bool implemented = false;
                    ARCHON_STEADY_ASSERT(implemented);                    
                    goto matched;
                }
                goto unsupported_channel_masks;
            }
            if (visual_info.c_class == PseudoColor) {
                if (ARCHON_UNLIKELY(visual_info.colormap_size != 256))
                    goto unexpected_colormap_size;
                if (zero_mask_match(visual_info)) {
                    if (colormap_needs_init) {
                        constexpr bool reverse_channel_order = false;
                        auto img = make_packed_image<image::int8_type, image::ChannelPacking_332, bytes_per_pixel,
                                                     reverse_channel_order>(img_size); // Throws
                        data = img->get_buffer().data();
                        img_2 = std::move(img);
                        int red_width   = 3;
                        int green_width = 3;
                        int blue_width  = 2;
                        setup_pseudo_color_colormap(dpy, colormap, red_width, green_width, blue_width,
                                                    use_weird_palette); // Throws
                    }
                    else {
                        // Create image with indirect color     
                        // FIXME: Read out palette and produce indirect color version of image with respect to that palette                                        
                        bool implemented = false;
                        ARCHON_STEADY_ASSERT(implemented);                    
                    }
                    goto matched;
                }
                goto unsupported_channel_masks;
            }
            if (visual_info.c_class == TrueColor || visual_info.c_class == DirectColor) {
                if (ARCHON_UNLIKELY(visual_info.colormap_size != 8))
                    goto unexpected_colormap_size;
                // FIXME: Unformatunately, it looks like Xvfb + X.Org implements this visual
                // incorrectly at this depth (8). Colors come out wrong. Further
                // investigation is needed.    
                BitFields bit_fields = {};
                if (norm_mask_match<image::ChannelPacking_332>(visual_info)) {
                    constexpr bool reverse_channel_order = false;
                    auto img = make_packed_image<image::int8_type, image::ChannelPacking_332, bytes_per_pixel,
                                                 reverse_channel_order>(img_size); // Throws
                    data = img->get_buffer().data();
                    img_2 = std::move(img);
                    record_bit_fields<image::ChannelPacking_332>(bit_fields);
                    goto colormap_1;
                }
                if (rev_mask_match<image::ChannelPacking_233>(visual_info)) {
                    constexpr bool reverse_channel_order = true;
                    auto img = make_packed_image<image::int8_type, image::ChannelPacking_233, bytes_per_pixel,
                                                 reverse_channel_order>(img_size); // Throws
                    data = img->get_buffer().data();
                    img_2 = std::move(img);
                    record_rev_bit_fields<image::ChannelPacking_233>(bit_fields);
                    goto colormap_1;
                }
                goto unsupported_channel_masks;

              colormap_1:
                if (colormap_needs_init) {
                    ARCHON_ASSERT(visual_info.c_class == DirectColor);
                    setup_direct_color_colormap(dpy, colormap, bit_fields, use_weird_palette); // Throws
                }
                goto matched;
            }
            goto unexpected_visual_class;
        }
        if (depth == 15) {
            if (bits_per_pixel != 16)
                goto unsupported_bits_per_pixel;
            constexpr int bytes_per_pixel = 2;
            if (visual_info.c_class == TrueColor || visual_info.c_class == DirectColor) {
                if (ARCHON_UNLIKELY(visual_info.colormap_size != 32))
                    goto unexpected_colormap_size;
                BitFields bit_fields = {};
                if (norm_mask_match<image::ChannelPacking_555>(visual_info)) {
                    constexpr bool reverse_channel_order = false;
                    auto img = make_packed_image<image::int16_type, image::ChannelPacking_555, bytes_per_pixel,
                                                 reverse_channel_order>(img_size); // Throws
                    data = img->get_buffer().data();
                    img_2 = std::move(img);
                    record_bit_fields<image::ChannelPacking_555>(bit_fields);
                    goto colormap_2;
                }
                if (rev_mask_match<image::ChannelPacking_555>(visual_info)) {
                    constexpr bool reverse_channel_order = true;
                    auto img = make_packed_image<image::int16_type, image::ChannelPacking_555, bytes_per_pixel,
                                                 reverse_channel_order>(img_size); // Throws
                    data = img->get_buffer().data();
                    img_2 = std::move(img);
                    record_rev_bit_fields<image::ChannelPacking_555>(bit_fields);
                    goto colormap_2;
                }
                goto unsupported_channel_masks;

              colormap_2:
                if (colormap_needs_init) {
                    ARCHON_ASSERT(visual_info.c_class == DirectColor);
                    setup_direct_color_colormap(dpy, colormap, bit_fields, use_weird_palette); // Throws
                }
                goto matched;
            }
            goto unexpected_visual_class;
        }
        if (depth == 16) {
            if (bits_per_pixel != 16)
                goto unsupported_bits_per_pixel;
            constexpr int bytes_per_pixel = 2;
            if (visual_info.c_class == TrueColor || visual_info.c_class == DirectColor) {
                if (ARCHON_UNLIKELY(visual_info.colormap_size != 64))
                    goto unexpected_colormap_size;
                BitFields bit_fields = {};
                if (norm_mask_match<image::ChannelPacking_565>(visual_info)) {
                    constexpr bool reverse_channel_order = false;
                    auto img = make_packed_image<image::int16_type, image::ChannelPacking_565, bytes_per_pixel,
                                                 reverse_channel_order>(img_size); // Throws
                    data = img->get_buffer().data();
                    img_2 = std::move(img);
                    record_bit_fields<image::ChannelPacking_565>(bit_fields);
                    goto colormap_3;
                }
                else if (rev_mask_match<image::ChannelPacking_565>(visual_info)) {
                    constexpr bool reverse_channel_order = true;
                    auto img = make_packed_image<image::int16_type, image::ChannelPacking_565, bytes_per_pixel,
                                                 reverse_channel_order>(img_size); // Throws
                    data = img->get_buffer().data();
                    img_2 = std::move(img);
                    record_rev_bit_fields<image::ChannelPacking_565>(bit_fields);
                    goto colormap_3;
                }
                goto unsupported_channel_masks;

              colormap_3:
                if (colormap_needs_init) {
                    ARCHON_ASSERT(visual_info.c_class == DirectColor);
                    setup_direct_color_colormap(dpy, colormap, bit_fields, use_weird_palette); // Throws
                }
                goto matched;
            }
            goto unexpected_visual_class;
        }
        if (depth == 24) {
            if (bits_per_pixel != 32)
                goto unsupported_bits_per_pixel;
            constexpr int bytes_per_pixel = 4;
            if (visual_info.c_class == TrueColor || visual_info.c_class == DirectColor) {
                if (ARCHON_UNLIKELY(visual_info.colormap_size != 256))
                    goto unexpected_colormap_size;
                BitFields bit_fields = {};
                if (norm_mask_match<image::ChannelPacking_888>(visual_info)) {
                    constexpr bool reverse_channel_order = false;
                    auto img = make_packed_image<image::int32_type, image::ChannelPacking_888, bytes_per_pixel,
                                                 reverse_channel_order>(img_size); // Throws
                    data = img->get_buffer().data();
                    img_2 = std::move(img);
                    record_bit_fields<image::ChannelPacking_888>(bit_fields);
                    goto colormap_4;
                }
                else if (rev_mask_match<image::ChannelPacking_888>(visual_info)) {
                    constexpr bool reverse_channel_order = true;
                    auto img = make_packed_image<image::int32_type, image::ChannelPacking_888, bytes_per_pixel,
                                                 reverse_channel_order>(img_size); // Throws
                    data = img->get_buffer().data();
                    img_2 = std::move(img);
                    record_rev_bit_fields<image::ChannelPacking_888>(bit_fields);
                    goto colormap_4;
                }
                goto unsupported_channel_masks;

              colormap_4:
                if (colormap_needs_init) {
                    ARCHON_ASSERT(visual_info.c_class == DirectColor);
                    setup_direct_color_colormap(dpy, colormap, bit_fields, use_weird_palette); // Throws
                }
                goto matched;
            }
            goto unexpected_visual_class;
        }
        goto unsupported_depth;

      unsupported_depth:
        logger.error("Unsupported depth: %s", depth); // Throws
        return EXIT_FAILURE;

      unsupported_bits_per_pixel:
        logger.error("Unsupported number bits per pixel for depth %s: %s", depth, bits_per_pixel); // Throws
        return EXIT_FAILURE;

      unexpected_visual_class:
        logger.error("Unexpected class for visual 0x%s: %s", core::as_hex_int(visualid),
                     get_visual_class_name(visual_info.c_class)); // Throws
        return EXIT_FAILURE;

      unsupported_channel_masks:
        logger.error("Unsupported channel masks in visual 0x%s: red = %s, green = %s, blue = %s",
                     core::as_hex_int(visualid), core::as_hex_int(visual_info.red_mask),
                     core::as_hex_int(visual_info.green_mask), core::as_hex_int(visual_info.blue_mask)); // Throws
        return EXIT_FAILURE;

      unexpected_colormap_size:
        logger.error("Unexpected colormap size for visual 0x%s: %s", core::as_hex_int(visualid),
                     visual_info.colormap_size); // Throws
        return EXIT_FAILURE;

      matched:
        img_2->put_image({ 0, 0 }, *img); // Throws
        image::save(*img_2, "/tmp/out.png", locale); // Throws

        // Xlib requires that depth of image matches depth of pixmap. X11 requires that
        // depth of pixmap matches depth of target window (during XCopyArea()). Only ZPixmap
        // format is relevant. With ZPixmap, image data is ordered by pixel rather than by
        // bit-plane, and each scanline unit (word) holds one or more pixels. ZPixmap format
        // supports depths of any offered visual. XPutImage() can handle byte swapping and
        // changes in row alignment (`scanline_pad` / `bitmap_pad`).
        int scanline_pad = bits_per_pixel;
        XImage img_3;
        img_3.width            = img_size.width;
        img_3.height           = img_size.height;
        img_3.xoffset          = 0;
        img_3.format           = ZPixmap;
        img_3.data             = data;
        img_3.byte_order       = LSBFirst;
        img_3.bitmap_unit      = BitmapUnit(dpy); // Immaterial
        img_3.bitmap_bit_order = BitmapBitOrder(dpy); // Immaterial
        img_3.bitmap_pad       = scanline_pad;
        img_3.depth            = depth;
        img_3.bytes_per_line   = 0;
        img_3.bits_per_pixel   = bits_per_pixel;
        img_3.red_mask         = visual_info.red_mask;
        img_3.green_mask       = visual_info.green_mask;
        img_3.blue_mask        = visual_info.blue_mask;
        Status status = XInitImage(&img_3);
        ARCHON_STEADY_ASSERT(status != 0);
        XPutImage(dpy, img_pixmap, gc, &img_3, 0, 0, 0, 0, unsigned(img_size.width), unsigned(img_size.height));
    }

    struct WindowSlot {
        int no;
        Window window;
        Drawable drawable;
        display::Size size;
        bool redraw = false;
        bool suppress_redraw = false;

        WindowSlot(int no_2, Window window_2, Drawable drawable_2, display::Size size_2) noexcept
        {
            no       = no_2;
            window   = window_2;
            drawable = drawable_2;
            size     = size_2;
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

    Atom delete_window                = intern_string("WM_DELETE_WINDOW");
    Atom atom_net_wm_state            = intern_string("_NET_WM_STATE");
    Atom atom_net_wm_state_fullscreen = intern_string("_NET_WM_STATE_FULLSCREEN");


#if HAVE_XDBE
    XdbeSwapAction swap_action = XdbeUndefined; // Contents of swapped-out buffer becomes undefined
#endif

    int prev_window_no = 0;
    std::size_t max_seen_window_slots = 0;
    auto open_window = [&] {
        // Create window
        display::Pos pos;
        if (optional_pos.has_value())
            pos = optional_pos.value();
        unsigned long valuemask = CWEventMask | CWColormap;
        XSetWindowAttributes attributes;
        attributes.event_mask = (KeyPressMask | KeyReleaseMask |
                                 ButtonPressMask | ButtonReleaseMask |
                                 ButtonMotionMask |
                                 EnterWindowMask | LeaveWindowMask |
                                 FocusChangeMask |
                                 ExposureMask |
                                 StructureNotifyMask |
                                 KeymapStateMask);
        attributes.colormap = colormap;
        Window window = XCreateWindow(dpy, root, pos.x, pos.y, unsigned(img_size.width), unsigned(img_size.height),
                                      0, depth, InputOutput, visual_info.visual, valuemask, &attributes);

        // Set window name
        int no = ++prev_window_no;
        std::string name_1 = core::format(locale, "X11 Probe %s", no); // Throws
        char* name_2 = name_1.data();
        XTextProperty name_3;
        {
            Status status = XStringListToTextProperty(&name_2, 1, &name_3);
            ARCHON_STEADY_ASSERT(status != 0);
        }
        XSetWMName(dpy, window, &name_3);
        XFree(name_3.value);

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

        WindowSlot slot = { no, window, drawable, img_size };
        window_slots.emplace(window, slot); // Throws

        if (window_slots.size() > max_seen_window_slots)
            max_seen_window_slots = window_slots.size();

        return window;
    };

    auto close_window = [&](Window win) noexcept {
        XDestroyWindow(dpy, win);
        window_slots.erase(win);
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

    for (int i = 0; i < num_windows; ++i)
        open_window(); // Throws

    for (const auto& entry : window_slots) {
        const WindowSlot& slot = entry.second;
        XMapWindow(dpy, slot.window);
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
                            if (size != slot->size) {
                                slot->size = size;
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
                                break;
                            }
                            if (ev.type == KeyRelease && keysym == XK_f) {
                                XClientMessageEvent event = {};
                                event.type = ClientMessage;
                                event.window = slot->window;
                                event.message_type = atom_net_wm_state;
                                event.format = 32;
                                event.data.l[0] = 2; // Toggle property
                                event.data.l[1] = atom_net_wm_state_fullscreen;
                                event.data.l[2] = 0; // No second property to alter
                                event.data.l[3] = 1; // Request is from normal application
                                Bool propagate = False;
                                long event_mask = SubstructureRedirectMask | SubstructureNotifyMask;
                                Status status = XSendEvent(dpy, root, propagate, event_mask, reinterpret_cast<XEvent*>(&event));
                                ARCHON_STEADY_ASSERT(status != 0);
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
                if (have_xrandr && ev.type == xrandr_event_base + RRNotify) {
                    const auto& ev_2 = reinterpret_cast<const XRRNotifyEvent&>(ev);
                    switch (ev_2.subtype) {
                        case RRNotify_CrtcChange:
                        case RRNotify_OutputChange:
                            if (update_display_info()) // Throws
                                dump_display_info(); // Throws
                    }
                }
#endif // HAVE_XRANDR
            }
        }

        for (const auto& entry : window_slots) {
            const WindowSlot& slot = entry.second;
            if (slot.redraw && !slot.suppress_redraw) {
                int win_width  = slot.size.width;
                int win_height = slot.size.height;
                int left = 0, right = win_width;
                int top = 0, bottom = win_height;
                int x = 0, y = 0;
                int w = img_size.width, h = img_size.height;
                {
                    int width_diff  = win_width  - img_size.width;
                    int height_diff = win_height - img_size.height;
                    if (width_diff >= 0) {
                        left  = width_diff / 2;
                        right = left + img_size.width;
                    }
                    else {
                        x = (-width_diff + 1) / 2;
                        w = win_width;
                    }
                    if (height_diff >= 0) {
                        top    = height_diff / 2;
                        bottom = top + img_size.height;
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
                XCopyArea(dpy, img_pixmap, drawable, gc, x, y, w, h, left, top);
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
