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


#include <limits>
#include <stdexcept>
#include <string>
#include <locale>
#include <filesystem>
#include <iostream>

#include <archon/core/assert.hpp>
#include <archon/core/filesystem.hpp>
#include <archon/core/build_environment.hpp>
#include <archon/log.hpp>
#include <archon/cli.hpp>
#include <archon/util/color.hpp>
#include <archon/util/as_css_color.hpp>
#include <archon/image.hpp>
#include <archon/image/channel_packing.hpp>
#include <archon/image/packed_pixel_format.hpp>
#include <archon/display/impl/config.h>

#if ARCHON_DISPLAY_HAVE_XLIB
#  include <X11/Xlib.h>
#  include <X11/Xutil.h>
#  include <X11/keysym.h>
#  include <X11/XKBlib.h>
#endif


using namespace archon;


#if ARCHON_DISPLAY_HAVE_XLIB


namespace {


auto get_visual_class_name(int class_) -> const char*
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


inline bool zero_mask_match(const XVisualInfo& info)
{
    return (info.red_mask == 0 && info.green_mask == 0 && info.blue_mask == 0);
}


template<class P> inline bool mask_match(const XVisualInfo& info)
{
    using packing_type = P;
    static_assert(packing_type::num_fields == 3);
    using word_type = decltype(info.red_mask + info.green_mask + info.blue_mask);
    return (info.red_mask   == image::get_bit_field_mask<word_type>(packing_type::fields, 3, 0) &&
            info.green_mask == image::get_bit_field_mask<word_type>(packing_type::fields, 3, 1) &&
            info.blue_mask  == image::get_bit_field_mask<word_type>(packing_type::fields, 3, 2));
}


template<class P> inline bool rev_mask_match(const XVisualInfo& info)
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


void setup_direct_color_colormap(Display* display, Colormap colormap, const BitFields& fields, const XVisualInfo& info)
{
    Bool contig = True;
    unsigned long pixel = 0;
    int ncolors = 1;
    unsigned long red_mask = 0, green_mask = 0, blue_mask = 0;
    Status status = XAllocColorPlanes(display, colormap, contig, &pixel, ncolors,
                                      fields.red_width, fields.green_width, fields.blue_width,
                                      &red_mask, &green_mask, &blue_mask);
    ARCHON_STEADY_ASSERT(status != 0);
    // The following assertions rely on the assumption that the number of bit-planes is
    // equal to the depth, and that we requested all the bitplanes that are available.
    ARCHON_STEADY_ASSERT(pixel == 0);
    ARCHON_STEADY_ASSERT(red_mask   == info.red_mask);
    ARCHON_STEADY_ASSERT(green_mask == info.green_mask);
    ARCHON_STEADY_ASSERT(blue_mask  == info.blue_mask);
    {
        int n = 1 << fields.red_width;
        auto colors = std::make_unique<XColor[]>(n); // Throws
        for (int i = 0; i < n; ++i) {
            XColor& color = colors[i];
            color.pixel = static_cast<unsigned long>(i) << fields.red_shift;
            color.red = static_cast<unsigned short>(i << (16 - fields.red_width));
            color.flags = DoRed;
        }
        XStoreColors(display, colormap, colors.get(), n);
    }
    {
        int n = 1 << fields.green_width;
        auto colors = std::make_unique<XColor[]>(n); // Throws
        for (int i = 0; i < n; ++i) {
            XColor& color = colors[i];
            color.pixel = static_cast<unsigned long>(i) << fields.green_shift;
            color.green = static_cast<unsigned short>(i << (16 - fields.green_width));
            color.flags = DoGreen;
        }
        XStoreColors(display, colormap, colors.get(), n);
    }
    {
        int n = 1 << fields.blue_width;
        auto colors = std::make_unique<XColor[]>(n); // Throws
        for (int i = 0; i < n; ++i) {
            XColor& color = colors[i];
            color.pixel = static_cast<unsigned long>(i) << fields.blue_shift;
            color.blue = static_cast<unsigned short>(i << (16 - fields.blue_width));
            color.flags = DoBlue;
        }
        XStoreColors(display, colormap, colors.get(), n);
    }
}


void setup_gray_scale_colormap(Display* display, Colormap colormap, int width)
{
    ARCHON_ASSERT(width > 0 && width < 16);
    Bool contig = True;
    auto masks = std::make_unique<unsigned long[]>(std::size_t(width)); // Throws
    int nplanes = width;
    unsigned long pixel = 0;
    int npixels = 1;
    Status status = XAllocColorCells(display, colormap, contig, masks.get(), nplanes,
                                     &pixel, npixels);
    ARCHON_STEADY_ASSERT(status != 0);
    unsigned long mask = 0;
    for (int i = 0; i < width; ++i)
        mask |= masks[i];
    // The following assertions rely on the assumption that the number of bit-planes is
    // equal to the depth, and that we requested all the bitplanes that are available.
    ARCHON_STEADY_ASSERT(pixel == 0);
    ARCHON_STEADY_ASSERT(mask == core::int_mask<unsigned long>(width));
    int n = 1 << width;
    auto colors = std::make_unique<XColor[]>(n); // Throws
    for (int i = 0; i < n; ++i) {
        XColor& color = colors[i];
        color.pixel = i;
        color.red   = static_cast<unsigned short>(i << (16 - width));
        color.green = color.red;
        color.blue  = color.red;
        color.flags = DoRed | DoGreen | DoBlue;
    }
    XStoreColors(display, colormap, colors.get(), n);
}


void setup_pseudo_color_colormap(Display* display, Colormap colormap, int red_width, int green_width, int blue_width)
{
    int width = red_width + green_width + blue_width;
    ARCHON_ASSERT(width > 0 && width < 16);
    Bool contig = True;
    auto masks = std::make_unique<unsigned long[]>(std::size_t(width)); // Throws
    int nplanes = width;
    unsigned long pixel = 0;
    int npixels = 1;
    Status status = XAllocColorCells(display, colormap, contig, masks.get(), nplanes, &pixel, npixels);
    ARCHON_STEADY_ASSERT(status != 0);
    unsigned long mask = 0;
    for (int i = 0; i < width; ++i)
        mask |= masks[i];
    // The following assertions rely on the assumption that the number of bit-planes is
    // equal to the depth, and that we requested all the bitplanes that are available.
    ARCHON_STEADY_ASSERT(pixel == 0);
    ARCHON_STEADY_ASSERT(mask == core::int_mask<unsigned long>(width));
    int n = 1 << width;
    auto colors = std::make_unique<XColor[]>(n); // Throws
    int red_shift   = green_width + blue_width;
    int green_shift = blue_width;
    int blue_shift  = 0;
    for (int i = 0; i < n; ++i) {
        XColor& color = colors[i];
        color.pixel = unsigned(i);
        using ushort = unsigned short;
        color.red   = ushort(((i >> red_shift)   << (16 - red_width))   & 65535);
        color.green = ushort(((i >> green_shift) << (16 - green_width)) & 65535);
        color.blue  = ushort(((i >> blue_shift)  << (16 - blue_width))  & 65535);
        color.flags = DoRed | DoGreen | DoBlue;
    }
    XStoreColors(display, colormap, colors.get(), n);
}


} // unnamed namespace



int main(int argc, char* argv[])
{
    std::locale locale("");

    namespace fs = std::filesystem;
    std::optional<fs::path> optional_path;
    std::optional<VisualID> optional_visual;
    log::LogLevel log_level_limit = log::LogLevel::warn;

    cli::Spec spec;
    pat("[<path>]", cli::no_attributes, spec,
        "Lorem ipsum.",
        std::tie(optional_path)); // Throws

    opt(cli::help_tag, spec); // Throws
    opt(cli::stop_tag, spec); // Throws

    opt("-v, --visual", "<hex>", cli::no_attributes, spec,
        "Specify the hexadeciaml ID of the X11 visual to be used (see command `xdpyinfo`). By default, the default "
        "visual for the selected screen will be used.",
        cli::exec([&](std::string_view str) {
            core::ValueParser parser(locale);
            VisualID id = {};
            if (ARCHON_LIKELY(parser.parse(str, core::as_hex_int(id)))) {
                optional_visual.emplace(id);
                return true;
            }
            return false;
        })); // Throws

    opt("-l, --log-level", "<level>", cli::no_attributes, spec,
        "Set the log level limit. The possible levels are \"off\", \"fatal\", \"error\", \"warn\", \"info\", "
        "\"detail\", \"debug\", \"trace\", and \"all\". The default limit is \"@V\".",
        cli::assign(log_level_limit)); // Throws

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

    // FIXME: Consider allowing for fullscreen toggle                                                      

    // Connect to display
    Display* display = XOpenDisplay(nullptr);
    ARCHON_STEADY_ASSERT(display);
    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);
    unsigned long black = BlackPixel(display, screen);

    // List visuals
    {
        int n = 0;
        XVisualInfo info_template;
        XVisualInfo* entries = XGetVisualInfo(display, 0, &info_template, &n);
        ARCHON_STEADY_ASSERT(entries);
        for (int i = 0; i < n; ++i) {
            const XVisualInfo& info = entries[i];
            logger.info("Visual %s: visualid = 0x%s, screen = %s, depth = %s, class = %s, "
                        "red_mask = 0x%s, green_mask = 0x%s, blue_mask = 0x%s, colormap_size = %s, bits_per_rgb = %s",
                        i + 1, core::as_hex_int(info.visualid), info.screen, info.depth,
                        get_visual_class_name(info.c_class), core::as_hex_int(info.red_mask),
                        core::as_hex_int(info.green_mask), core::as_hex_int(info.blue_mask), info.colormap_size,
                        info.bits_per_rgb);
        }
        XFree(entries);
    }

    // Choose depth and visual
    VisualID visual_id = XVisualIDFromVisual(DefaultVisual(display, screen));
    if (optional_visual.has_value())
        visual_id = optional_visual.value();
    XVisualInfo visual_info = {};
    {
        int n = 0;
        XVisualInfo info_template;
        info_template.visualid = visual_id;
        XVisualInfo* entries = XGetVisualInfo(display, VisualIDMask, &info_template, &n);
        if (ARCHON_UNLIKELY(!entries)) {
            ARCHON_STEADY_ASSERT(optional_visual.has_value());
            logger.error("Invalid visual ID: 0x%s", core::as_hex_int(visual_id)); // Throws
            return EXIT_FAILURE;
        }
        ARCHON_STEADY_ASSERT(n == 1);
        visual_info = entries[0];
        XFree(entries);
    }
    int depth = visual_info.depth;
    Visual* visual = visual_info.visual;

    // List ZPixmap formats and find "bits per pixel" for selected depth
    int bits_per_pixel = 0;
    {
        int n = 0;
        XPixmapFormatValues* entries = XListPixmapFormats(display, &n);
        ARCHON_STEADY_ASSERT(entries);
        bool found = false;
        for (int i = 0; i < n; ++i) {
            const XPixmapFormatValues& values = entries[i];
            logger.info("Format %s: depth = %s, bits_per_pixel = %s, scanline_pad = %s", i + 1, values.depth,
                        values.bits_per_pixel, values.scanline_pad);
            if (values.depth == depth) {
                bits_per_pixel = values.bits_per_pixel;
                found = true;
            }
        }
        ARCHON_STEADY_ASSERT(found);
        XFree(entries);
    }

    // Fetch depths
    std::vector<int> depths;
    {
        int n = 0;
        int* entries = XListDepths(display, screen, &n);
        ARCHON_STEADY_ASSERT(entries);
        for (int i = 0; i < n; ++i)
            depths.push_back(entries[i]);
        XFree(entries);
    }

    int num_bitplanes = DisplayPlanes(display, screen);

    logger.info("Display string:                     %s", DisplayString(display));
    logger.info("Server vendor:                      %s", ServerVendor(display));
    logger.info("Vendor release:                     %s", core::as_int(VendorRelease(display)));
    logger.info("Image byte order:                   %s",
                (ImageByteOrder(display) == LSBFirst ? "little-endian" : "big-endian"));
    logger.info("Bitmap bit order:                   %s",
                (BitmapBitOrder(display) == LSBFirst ? "least signitfiant bit first" :
                 "most significant bit first"));
    logger.info("Bitmap scanline pad:                %s", core::as_int(BitmapPad(display)));
    logger.info("Bitmap scanline unit:               %s", core::as_int(BitmapUnit(display)));
    logger.info("Number of screens:                  %s", core::as_int(ScreenCount(display)));
    logger.info("Selected screen:                    %s", core::as_int(screen + 1));
    logger.info("Size of screen:                     %spx x %spx (%smm x %smm)",
                core::as_int(DisplayWidth(display, screen)),
                core::as_int(DisplayHeight(display, screen)), DisplayWidthMM(display, screen),
                DisplayHeightMM(display, screen));
    logger.info("Concurrent colormaps of screen:     %s -> %s",
                MinCmapsOfScreen(ScreenOfDisplay(display, screen)),
                MaxCmapsOfScreen(ScreenOfDisplay(display, screen)));
    logger.info("Size of default colormap of screen: %s", core::as_int(DisplayCells(display, screen)));
    logger.info("Supported depths on screen:         %s", core::as_list(depths));
    logger.info("Default depth of screen:            %s", core::as_int(DefaultDepth(display, screen)));
    logger.info("Selected depth:                     %s", core::as_int(depth));
    logger.info("Bit-planes of screen:               %s", core::as_int(num_bitplanes));
    logger.info("Default visual of screen:           0x%s",
                core::as_hex_int(XVisualIDFromVisual(DefaultVisual(display, screen))));
    logger.info("Selected visual:                    0x%s", core::as_hex_int(visual_id));
    logger.info("Class of selected visual:           %s", get_visual_class_name(visual_info.c_class));

    // Create graphics context
    XGCValues gc_values = {};
    gc_values.graphics_exposures = False;
    GC gc = XCreateGC(display, root, GCGraphicsExposures, &gc_values);
    XSetForeground(display, gc, black);

    // By creating a new colormap, rather than reusing the one used by the root window, it
    // becomes possible to use a visual for the new window that differs from the one used by
    // the root windows. The colormap and the new window must agree on visual.
    Colormap colormap = XCreateColormap(display, root, visual, AllocNone);

    // Upload image
    Pixmap img_pixmap = XCreatePixmap(display, root, unsigned(img_size.width), unsigned(img_size.height), depth);
    {
        // Setup X11 colormap when necessary and convert image to suitable pixel format.

        std::unique_ptr<image::WritableImage> img_2;
        char* data = nullptr;

        if (depth == 8) {
            if (bits_per_pixel != 8)
                goto unsupported_bits_per_pixel;
            constexpr int bytes_per_pixel = 1;
            if (visual_info.c_class == StaticColor) {
                if (ARCHON_UNLIKELY(visual_info.colormap_size != 256))
                    goto unexpected_colormap_size;
                if (zero_mask_match(visual_info) || true) {                            
                    int n = 1 << 8;
                    auto colors = std::make_unique<XColor[]>(n); // Throws
                    for (int i = 0; i < n; ++i) {
                        XColor& color = colors[i];
                        color.pixel = unsigned(i);
                    }
                    XQueryColors(display, colormap, colors.get(), n);
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
                if (mask_match<image::ChannelPacking_332>(visual_info)) {
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
                goto unsupported_channel_masks;
            }
            if (visual_info.c_class == PseudoColor) {
                if (ARCHON_UNLIKELY(visual_info.colormap_size != 256))
                    goto unexpected_colormap_size;
                if (zero_mask_match(visual_info)) {
                    // FIXME: Consider XGetRGBColormaps() --> https://tronche.com/gui/x/xlib/ICC/standard-colormaps/XGetRGBColormaps.html --> Fetch all available colormaps, look for onw with matching visual ID. If one is found, use that colormap. This requires that the image is converted to indirect color pixel format.                
                    // FIXME: Consider alternative: Generate optimal palette for image of, say 248, entries, then request that many color slots, then initialize those slots with the colors of the palette, then convert image to indirect color using that palette.      
                    constexpr bool reverse_channel_order = false;
                    auto img = make_packed_image<image::int8_type, image::ChannelPacking_332, bytes_per_pixel,
                                                 reverse_channel_order>(img_size); // Throws
                    data = img->get_buffer().data();
                    img_2 = std::move(img);
                    int red_width   = 3;
                    int green_width = 3;
                    int blue_width  = 2;
                    setup_pseudo_color_colormap(display, colormap, red_width, green_width, blue_width); // Throws
                    goto matched;
                }
                goto unsupported_channel_masks;
            }
            if (visual_info.c_class == StaticGray || visual_info.c_class == GrayScale) {
                if (ARCHON_UNLIKELY(visual_info.colormap_size != 256))
                    goto unexpected_colormap_size;
                if (zero_mask_match(visual_info)) {
                    if (visual_info.c_class == GrayScale)
                        setup_gray_scale_colormap(display, colormap, depth); // Throws
                    auto img = make_lum_image(img_size); // Throws
                    data = img->get_buffer().data();
                    img_2 = std::move(img);
                    goto matched;
                }
                goto unsupported_channel_masks;
            }
            if (visual_info.c_class == TrueColor || visual_info.c_class == DirectColor) {
                if (ARCHON_UNLIKELY(num_bitplanes != 8))
                    goto unsupported_num_bitplanes;
                if (ARCHON_UNLIKELY(visual_info.colormap_size != 8))
                    goto unexpected_colormap_size;
                BitFields bit_fields = {};
                if (mask_match<image::ChannelPacking_332>(visual_info)) {
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
                if (visual_info.c_class == DirectColor)
                    setup_direct_color_colormap(display, colormap, bit_fields, visual_info); // Throws
                goto matched;
            }
            goto unexpected_visual_class;
        }
        if (depth == 15) {
            if (bits_per_pixel != 16)
                goto unsupported_bits_per_pixel;
            constexpr int bytes_per_pixel = 2;
            if (visual_info.c_class == TrueColor || visual_info.c_class == DirectColor) {
                if (ARCHON_UNLIKELY(num_bitplanes != 15))
                    goto unsupported_num_bitplanes;
                if (ARCHON_UNLIKELY(visual_info.colormap_size != 32))
                    goto unexpected_colormap_size;
                BitFields bit_fields = {};
                if (mask_match<image::ChannelPacking_555>(visual_info)) {
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
                if (visual_info.c_class == DirectColor)
                    setup_direct_color_colormap(display, colormap, bit_fields, visual_info); // Throws
                goto matched;
            }
            goto unexpected_visual_class;
        }
        if (depth == 16) {
            if (bits_per_pixel != 16)
                goto unsupported_bits_per_pixel;
            constexpr int bytes_per_pixel = 2;
            if (visual_info.c_class == TrueColor || visual_info.c_class == DirectColor) {
                if (ARCHON_UNLIKELY(num_bitplanes != 16))
                    goto unsupported_num_bitplanes;
                if (ARCHON_UNLIKELY(visual_info.colormap_size != 64))
                    goto unexpected_colormap_size;
                BitFields bit_fields = {};
                if (mask_match<image::ChannelPacking_565>(visual_info)) {
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
                if (visual_info.c_class == DirectColor)
                    setup_direct_color_colormap(display, colormap, bit_fields, visual_info); // Throws
                goto matched;
            }
            goto unexpected_visual_class;
        }
        if (depth == 24) {
            if (bits_per_pixel != 32)
                goto unsupported_bits_per_pixel;
            constexpr int bytes_per_pixel = 4;
            if (visual_info.c_class == TrueColor || visual_info.c_class == DirectColor) {
                if (ARCHON_UNLIKELY(num_bitplanes != 24))
                    goto unsupported_num_bitplanes;
                if (ARCHON_UNLIKELY(visual_info.colormap_size != 256))
                    goto unexpected_colormap_size;
                BitFields bit_fields = {};
                if (mask_match<image::ChannelPacking_888>(visual_info)) {
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
                if (visual_info.c_class == DirectColor)
                    setup_direct_color_colormap(display, colormap, bit_fields, visual_info); // Throws
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
        logger.error("Unexpected class for visual 0x%s: %s", core::as_hex_int(visual_id),
                     get_visual_class_name(visual_info.c_class)); // Throws
        return EXIT_FAILURE;

      unsupported_channel_masks:
        logger.error("Unsupported channel masks in visual 0x%s: red = %s, green = %s, blue = %s",
                     core::as_hex_int(visual_id), core::as_hex_int(visual_info.red_mask),
                     core::as_hex_int(visual_info.green_mask), core::as_hex_int(visual_info.blue_mask)); // Throws
        return EXIT_FAILURE;

      unsupported_num_bitplanes:
        logger.error("Unsupported visual %s for number of bit-planes: %s", core::as_hex_int(visual_id),
                     num_bitplanes); // Throws
        return EXIT_FAILURE;

      unexpected_colormap_size:
        logger.error("Unexpected colormap size for visual 0x%s: %s", core::as_hex_int(visual_id),
                     visual_info.colormap_size); // Throws
        return EXIT_FAILURE;

      matched:
        img_2->put_image({ 0, 0 }, *img); // Throws
        image::save(*img_2, "/tmp/out.png", locale); // Throws

        // Xlib requires that depth of image matches depth of pixmap. X11 requires that
        // depth of pixmap matches depth of target window (during XCopyArea()). Only ZPixmap
        // format is relevant. With ZPixmap, image data is ordered by pixel rather than by
        // bit-plane, and each scanline unit (word) holds one or more pixels. ZPixamp format
        // supports depths of any offered visual. XPutIamge() can handle byte swapping and
        // changes in row alignment (`scanline_pad` / `bitmap_pad`).
        int scanline_pad = bits_per_pixel;
        XImage img_3;
        img_3.width            = img_size.width;
        img_3.height           = img_size.height;
        img_3.xoffset          = 0;
        img_3.format           = ZPixmap;
        img_3.data             = data;
        img_3.byte_order       = LSBFirst;
        img_3.bitmap_unit      = BitmapUnit(display); // Immaterial
        img_3.bitmap_bit_order = BitmapBitOrder(display); // Immaterial
        img_3.bitmap_pad       = scanline_pad;
        img_3.depth            = depth;
        img_3.bytes_per_line   = 0;
        img_3.bits_per_pixel   = bits_per_pixel;
        img_3.red_mask         = visual_info.red_mask;
        img_3.green_mask       = visual_info.green_mask;
        img_3.blue_mask        = visual_info.blue_mask;
        Status status = XInitImage(&img_3);
        ARCHON_STEADY_ASSERT(status != 0);
        XPutImage(display, img_pixmap, gc, &img_3, 0, 0, 0, 0, unsigned(img_size.width), unsigned(img_size.height));
    }

    // Create window
    int win_width  = img_size.width;
    int win_height = img_size.height;
    XSetWindowAttributes swa;
    swa.event_mask = (KeyPressMask | ExposureMask | StructureNotifyMask);
    swa.colormap = colormap;
    Window window = XCreateWindow(display, root, 0, 0, unsigned(win_width), unsigned(win_height), 0, depth,
                                  InputOutput, visual, CWEventMask | CWColormap, &swa);

    // Set window name
    const char* window_name = "X11 Probe";
    XTextProperty window_name_2;
    Status status = XStringListToTextProperty(const_cast<char **>(&window_name), 1, &window_name_2);
    ARCHON_STEADY_ASSERT(status != 0);
    XSetWMName(display, window, &window_name_2);

    // Set minimum window size
    XSizeHints size_hints;
    size_hints.flags = PMinSize;
    size_hints.min_width  = 128;
    size_hints.min_height = 128;
    XSetWMNormalHints(display, window, &size_hints);

    // Ask X to notify rather than close connection when window is closed
    Atom delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &delete_window, 1);

//    XInstallColormap(display, colormap);        

    // Event loop
    XMapWindow(display, window);
    for (;;) {
        bool redraw = false;
        XEvent ev;
        XPeekEvent(display, &ev);
        for (;;) {
            int n = XEventsQueued(display, QueuedAfterReading);
            if (n == 0)
                break;
            for (int i = 0; i < n; ++i) {
                XNextEvent(display, &ev);
                switch (ev.type) {
                    case KeyPress: {
                        KeySym key_sym = XkbKeycodeToKeysym(display, ev.xkey.keycode, XkbGroup1Index, 0);
                        if (key_sym == XK_Escape)
                            goto quit;
                        break;
                    }
                    case ClientMessage: {
                        bool is_close = (ev.xclient.window == window && ev.xclient.format == 32 &&
                                         Atom(ev.xclient.data.l[0]) == delete_window);
                        if (is_close)
                            goto quit;
                        break;
                    }
                    case Expose:
                        if (ev.xexpose.window == window)
                            redraw = true;
                        break;
                    case ConfigureNotify:
                        if (ev.xconfigure.window == window) {
                            int w = ev.xconfigure.width;
                            int h = ev.xconfigure.height;
                            if (w != win_width || h != win_height) {
                                win_width  = w;
                                win_height = h;
                                redraw = true;
                            }
                        }
                        break;
                }
            }
        }

        if (redraw) {
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
            // Clear top area
            if (top > 0)
                XFillRectangle(display, window, gc, 0, 0, unsigned(win_width), unsigned(top));
            // Clear left area
            if (left > 0)
                XFillRectangle(display, window, gc, 0, top, unsigned(left), unsigned(h));
            // Copy image
            XCopyArea(display, img_pixmap, window, gc, x, y, w, h, left, top);
            // Clear right area
            if (right < win_width)
                XFillRectangle(display, window, gc, right, top, unsigned(win_width - right), unsigned(h));
            // Clear bottom area
            if (bottom < win_height)
                XFillRectangle(display, window, gc, 0, bottom, unsigned(win_width), unsigned(win_height - bottom));
        }
    }

  quit:
    XDestroyWindow(display, window);
    XFreePixmap(display, img_pixmap);
    XFreeColormap(display, colormap);
    XFreeGC(display, gc);
    XCloseDisplay(display);
}


#else // !ARCHON_DISPLAY_HAVE_XLIB


int main()
{
    throw std::runtime_error("No Xlib support");
}


#endif // !ARCHON_DISPLAY_HAVE_XLIB
