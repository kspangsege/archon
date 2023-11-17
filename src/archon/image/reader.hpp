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

#ifndef ARCHON_X_IMAGE_X_READER_HPP
#define ARCHON_X_IMAGE_X_READER_HPP

/// \file


#include <cstddef>
#include <algorithm>
#include <memory>
#include <array>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/enum.hpp>
#include <archon/util/color.hpp>
#include <archon/image/geom.hpp>
#include <archon/image/iter.hpp>
#include <archon/image/tray.hpp>
#include <archon/image/bit_medium.hpp>
#include <archon/image/comp_types.hpp>
#include <archon/image/comp_repr.hpp>
#include <archon/image/color_space.hpp>
#include <archon/image/pixel_convert.hpp>
#include <archon/image/pixel_repr.hpp>
#include <archon/image/pixel.hpp>
#include <archon/image/block.hpp>
#include <archon/image/image.hpp>
#include <archon/image/impl/workspace.hpp>


namespace archon::image {


/// \brief Convenience facility for reading pixels from images.
///
/// FIXME: Explain idea of pixel reader: Color space conversion. Introduction / elimination of alpha channel. Conversion between component representations. Palette lookup when reading from indirect-color images. Handle reading from outside image boundary (falloff).           
///
/// FIXME: Make it clear that read operations are not `const` qualified, this means that it is not safe for two threads to read concurrently via the same reader.                                   
///
/// FIXME: Consider allowing for rebinding reader to new image such that allocated memory can be reused.           
///
class Reader {
public:
    /// \brief Construct pixel reader for image.
    ///
    /// This constructor sets up a reader to read from and otherwise operate on the
    /// specified image. The specified image becomes the attached image of the reader.
    ///
    /// The total number of channel components in the specified image (number of channels
    /// per pixel times width of image times height of image) must be representable in
    /// `std::ptrdiff_t` and in `std::size_t`. If the total number of channel components in
    /// the specified image is too large, an exception will be thrown.
    ///
    Reader(const image::Image&);

    ~Reader() noexcept;

    /// \brief Get reference to attached image.
    ///
    /// This function returns a reference to the attached image.
    ///
    auto get_image() const noexcept -> const image::Image&;

    /// \brief Size of associated image.
    ///
    /// This function is a shorthand for calling \ref Image::get_size() on the associated
    /// image, but is more efficient due to the valued being cached in the reader.
    ///
    auto get_image_size() const noexcept -> image::Size;

    /// \brief Native color space of reader.
    ///
    /// This function returns the native color space of the reader. The native color space
    /// of the reader is the same as the color space of the resolved pixel transfer scheme
    /// of the attached image (see class-level documentation for \ref Image).
    ///
    auto get_color_space() const noexcept -> const image::ColorSpace&;

    /// \brief Whether attached image has alpha channel.
    ///
    /// This function returns true when, and only when an alpha channel is present in the
    /// resolved pixel transfer scheme of the attached image. If `r` is a reader, then
    /// `r.has_alpha_channel()` is shorthand for `r.get_transfer_info().has_alpha`.
    ///
    bool has_alpha_channel() const noexcept;

    /// \brief Whether attached image uses indirect color.
    ///
    /// This function returns `true` when, and only when the attached image uses indirect
    /// color. If `r` is a reader, them `r.has_indexed_color()` is identical to
    /// `bool(r.get_palette())`.
    ///
    bool has_indexed_color() const noexcept;

    /// \brief Native component representation scheme of reader.
    ///
    /// This function returns the native component representation scheme of the reader. The
    /// native component representation scheme of the reader is the same as the component
    /// representation scheme of the resolved pixel transfer scheme of the attached image
    /// (see class-level documentation for \ref Image).
    ///
    auto get_comp_repr() const noexcept -> image::CompRepr;

    /// \brief Associated palette for indirect color image.
    ///
    /// This function is a shorthand for calling \ref Image::get_palette() on the attached
    /// image, but is more efficient due to the valued being cached in the reader.
    ///
    auto get_palette() const noexcept -> const image::Image*;

    /// \brief Number of channels (color and alpha) in attached image.
    ///
    /// This function returns the number of channels (color and alpha) in the attached
    /// image. If `r` is a reader, then `r.get_num_channels()` is shorthand for
    /// `r.get_transfer_info().get_num_channels()`.
    ///
    int get_num_channels() const noexcept;

    /// \brief Information on how pixels are transferred into and out of an image.
    ///
    /// This function is a shorthand for calling \ref Image::get_transfer_info() on the
    /// associated image, but is more efficient due to the returned object being cached in
    /// the reader.
    ///
    auto get_transfer_info() const noexcept -> const image::Image::TransferInfo&;

    /// \brief Number of colors in palette.
    ///
    /// If the attached image uses indirect color, this function returns the number of
    /// colors in the associated palette (\ref get_palette()). If the attached image uses
    /// direct color, this function returns zero.
    ///
    /// The number of colors in the palette is the number of pixels in the image used as
    /// palette clamped to the smaller of N and 2^M where N is the maximum representable
    /// value in `std::size_t` and M is the number of available bits in the component
    /// representation used to store color indexes in the attached image
    /// (`image::comp_repr_int_bit_width(image::color_index_repr)`).
    ///
    /// FIXME: Adjust text above when support of varying index representations is added.                                           
    ///
    auto get_palette_size() const noexcept -> std::size_t;

    enum class FalloffMode;

    /// \brief Install new set of custom color space converters.
    ///
    /// This function installs a new set of custom color space converters to be used by this
    /// reader. If the specified registry contains a converter from color space A to color
    /// space B, then all such conversions performed on behalf of this reader (or writer, by
    /// extension) will use that custom converter.
    ///
    /// By default, the set of custom color space converters is empty.
    ///
    /// The application is allowed to modify a registry after installing it in a reader (or
    /// writer), so long as the modification does not execute concurrently with the
    /// execution of any of the member functions of that reader (or writer).
    ///
    auto set_custom_color_space_converters(const image::ColorSpaceConverterRegistry&) noexcept -> Reader&;

    /// \{
    ///
    /// \brief Select behavior when reading outside image boundary.
    ///
    /// These functions set the falloff mode for the horizontal and vertical directions. The
    /// single argument overload sets the same mode for both directions.
    ///
    /// The falloff mode determines the value of pixels when reading outside the boundary of
    /// the image. See \ref FalloffMode for more information.
    ///
    auto set_falloff_mode(FalloffMode mode) noexcept -> Reader&;
    auto set_falloff_mode(FalloffMode horz_mode, FalloffMode vert_mode) noexcept -> Reader&;
    /// \}

    auto set_background_color(util::Color, image::float_type opacity = 1) -> Reader&;
    auto set_foreground_color(util::Color, image::float_type opacity = 1) -> Reader&;
    template<class R> auto set_background_color(const image::Pixel<R>&, image::float_type opacity = 1) -> Reader&;
    template<class R> auto set_foreground_color(const image::Pixel<R>&, image::float_type opacity = 1) -> Reader&;

    auto get_pixel(image::Pos pos) -> util::Color;

    template<class R> auto get_pixel(image::Pos pos, image::Pixel<R>&) -> Reader&;

    using int8_tray_type = image::Tray<image::int8_type>;

    /// \{
    ///
    /// \brief Read block of pixels from image.
    ///
    /// These functions are shorthands for calling \ref get_block() with \p R set to \ref
    /// image::Lum_8, \ref image::LumA_8, \ref image::RGB_8, and \ref image::RGBA_8
    /// respectively.
    ///
    auto get_block_lum(image::Pos pos, const int8_tray_type& tray) -> Reader&;
    auto get_block_luma(image::Pos pos, const int8_tray_type& tray) -> Reader&;
    auto get_block_rgb(image::Pos pos, const int8_tray_type& tray) -> Reader&;
    auto get_block_rgba(image::Pos pos, const int8_tray_type& tray) -> Reader&;
    /// \}

    /// \{
    ///
    /// \brief Read block of pixels from image and convert to given pixel representation.
    ///
    /// These functions read a block of pixels from the specified position (\p pos),
    /// converts the pixels to the specified pixel representation scheme (\p R), and then
    /// places them in the specified \p tray or \p block. The specified pixel representation
    /// scheme (\p R) must be an instance of \ref image::PixelRepr.
    ///
    /// If `block` is an object of type `image::PixelBlock<R>`, then `get_block<R>(pos,
    /// block)` is shorthand for `get_block<R>(pos, block.tray())`.
    ///
    /// If `tray` is an object of type `R::tray_type`, then `get_block<R>(pos, tray)` is
    /// shorthand for `get_block_a<R::comp_repr>(pos, tray, R::get_color_space(),
    /// R::has_alpha)`.
    ///
    template<class R> auto get_block(image::Pos pos, image::PixelBlock<R>& block) -> Reader&;
    template<class R> auto get_block(image::Pos pos, const typename R::tray_type& tray) -> Reader&;
    /// \}

    /// \brief Read block of pixels from image while converting to any color space.
    ///
    /// This function reads a block of pixels from the specified position (\p pos), and
    /// places them in the specified tray (\p tray). The pixels are read from the area
    /// designated by `image::Box(pos, tray.size)`. The pixels are converted to the
    /// specified format (\p color_space, \p has_alpha, and \p R) before they are stored in
    /// the tray. The tray must accommodate for a number of channels per pixel equal to
    /// `color_space.get_num_channels() + int(has_alpha)`.
    ///
    /// This function guarantees lossless operation if all of the following conditions are
    /// met:
    ///
    /// * The component representation scheme in use by the specified block (\p R) is the
    ///   same as the native component representation scheme of this reader (\ref
    ///   get_comp_repr()).
    ///
    /// * The color space in use by the specified block (\p color_space) is the same as the
    ///   native color space of this reader (\ref get_color_space()).
    ///
    /// * The attached image does not have an alpha channel (\ref has_alpha_channel()), or
    ///   the specified block does have an alpha channel (\p has_alpha).
    ///
    /// Under lossless operation, this function guarantees that the pixels passed out of
    /// `get_block_a()` are exactly equal to those passed out of \ref image::Image::read()
    /// of the attached image (or of the image acting as palette in case the attached image
    /// uses indirect color) with the proviso that if the attached image does not have an
    /// alpha channel, but the specified block does, then alpha components with maximum
    /// value (full opacity) will be introduced.
    ///
    template<image::CompRepr R> auto get_block_a(image::Pos pos, const image::tray_type<R>& tray,
                                                 const image::ColorSpace& color_space, bool has_alpha) -> Reader&;

    enum class ColorSlot;

    /// \brief Assign new color to color slot.
    ///
    /// This function assigns a new color to the specified color slot.
    ///
    /// If `r` is a reader, then `r.set_color<R>(slot, color, opacity)` is shorthand for
    /// `r.set_color_a<R::comp_repr>(slot, color.data(), R::get_color_space(), R::has_alpha,
    /// opacity)`.
    ///
    /// \sa \ref get_color()
    /// \sa \ref set_color_a()
    ///
    template<class R> auto set_color(ColorSlot slot, const image::Pixel<R>& color, image::float_type opacity = 1) ->
        Reader&;

    /// \brief Get current color from color slot.
    ///
    /// This function fetches the color currently stored in the specified color slot.
    ///
    /// If `r` is a reader, then `r.get_color<R>(slot, color)` is shorthand for
    /// `r.get_color_a<R::comp_repr>(slot, color.data(), R::get_color_space(),
    /// R::has_alpha)`.
    ///
    /// \sa \ref set_color()
    /// \sa \ref get_color_a()
    ///
    template<class R> auto get_color(ColorSlot slot, image::Pixel<R>& color) -> Reader&;

    /// \brief Assign new color on any form to color slot.
    ///
    /// This function assigns a new color to the specified color slot. The new color can be
    /// specified in any color space (\p color_space); with, or without an alpha component
    /// (\p has_alpha), and in any of the available component representation schemes (\p R).
    ///
    /// The specified color (\p components) must refer to an array of at least N components,
    /// where N is the number of channels of the specified color space (\p color_space) plus
    /// one if the specified color has an alpha channel (\p has_alpha).
    ///
    /// The specified opacity (\p opacity) will be used to modulate the alpha channel of the
    /// specified color.
    ///
    /// This function operates in a lossless manner if all of the following conditions are
    /// met:
    ///
    /// * The color space of the specified color (\p color_space) is the native color space
    ///   of this reader (\ref get_color_space()).
    ///
    /// * The component representation scheme of the specified color (\p R) is the native
    ///   component representation scheme of this reader (\ref get_comp_repr()).
    ///
    /// * The specified opacity (\p opacity) is 1.
    ///
    /// When operating in a lossless manner, this function guarantees that the color is
    /// stored exactly as specified. See \ref image::WritableImage::fill() and \ref
    /// get_color_a() for ways in which this might matter.
    ///
    /// \sa \ref set_color()
    /// \sa \ref get_color_a()
    ///
    template<image::CompRepr R>
    auto set_color_a(ColorSlot slot, const image::comp_type<R>* components, const image::ColorSpace& color_space,
                     bool has_alpha, image::float_type opacity = 1) -> Reader&;

    /// \brief Get current color on any form from color slot.
    ///
    /// This function fetches the color currently stored in the specified color slot (\p
    /// slot). The color is converted to the requested representation (\p R, \p color_space,
    /// \p has_alpha). The components of the converted color are stored in the array pointed
    /// to by \p components, including the alpha channel component when \p has_alpha is
    /// true. The array must be big enough to hold the converted color, which means that its
    /// size must be greater than, or equal to `color_space.get_num_channels() +
    /// int(has_alpha)`.
    ///
    /// This function operates in a lossless manner if all of the following conditions are
    /// met:
    ///
    /// * The requested color (\p slot) was set in a lossless manner (see \ref
    ///   set_color_a()).
    ///
    /// * The requested color (\p slot) was set without an alpha component, or the requested
    ///   color is solid (alpha component at maximum value), or the caller requests an alpha
    ///   component (\p has_alpha).
    ///
    /// When operating in a lossless manner, this function guarantees that the retrieved
    /// component values are exactly equal to those of the color stored in the specified
    /// color slot (\p slot) with the proviso that if the fill color was set without an
    /// alpha component, and the caller requests an alpha component, then an alpha component
    /// with maximum value (full opacity) is introduced.
    ///
    /// \sa \ref get_color()
    /// \sa \ref set_color_a()
    ///
    template<image::CompRepr R> auto get_color_a(ColorSlot slot, image::comp_type<R>* components,
                                                 const image::ColorSpace& color_space, bool has_alpha) -> Reader&;

    /// \brief Fetch color from specific palette entry.
    ///
    /// This function fetches the color from the specified palette entry (\p color_index),
    /// converts it to the requested form (\p R), and then stores the resulting color in \p
    /// color.
    ///
    /// If `r` is a reader, then `r.palette_lookup(color_index, color)` is shorthand for
    /// `r.palette_lookup_a<R::comp_repr>(color_index, color.data(), R::get_color_space(),
    /// R::has_alpha)`.
    ///
    /// \sa \ref palette_lookup_a()
    /// \sa \ref image::Writer::reverse_palette_lookup()
    ///
    template<class R> auto palette_lookup(std::size_t color_index, image::Pixel<R>& color) -> Reader&;

    /// \brief Fetch color on any form from specific palette entry.
    ///
    /// This function fetches the color from the specified palette entry (\p color_index),
    /// converts it to the requested form (\p R, \p color_space, \p has_alpha), and then
    /// stores the resulting channel components in the specified array (\p components).
    ///
    /// If the attached image does not have a palette (because it does not use indirect
    /// color), or if the palette is empty (number of entries is zero), this function
    /// fetches the currently configured background color instead (\ref
    /// set_background_color()).
    ///
    /// \sa \ref palette_lookup()
    /// \sa \ref image::Writer::reverse_palette_lookup_a()
    ///
    template<image::CompRepr R> auto palette_lookup_a(std::size_t color_index, image::comp_type<R>* components,
                                                      const image::ColorSpace& color_space, bool has_alpha) -> Reader&;

private:
    friend class Writer;

    // Default seed memory size for single-pixel workspaces
    static constexpr std::size_t s_default_workspace_seed_size = 5;

    static constexpr int s_num_color_slots = 2;

    template<image::CompRepr R> struct CompReprTag {
        static constexpr image::CompRepr comp_repr = R;
    };

    // The slot is initialized when at least one of the `have_` members are are
    // `true`. `have_restricted_native` implies `have_neutral` or
    // `have_unrestricted_native`.
    struct ColorSlotCtrl {
        bool have_neutral = false;
        bool have_restricted_native = false;
        bool have_unrestricted_native = false;
        bool is_solid = false; // Has meaning only when slot is initialized
    };

    const image::Image& m_image;
    const image::Size m_image_size;
    const image::Image* const m_palette;
    const std::size_t m_palette_size;
    const image::Image::TransferInfo m_transfer_info;
    const int m_num_channels;
    const int m_num_channels_ext; // Always includes an alpha channel

    const image::ColorSpaceConverterRegistry* m_custom_color_space_converters = nullptr;

    FalloffMode m_horz_falloff_mode;
    FalloffMode m_vert_falloff_mode;

    // Each color slot has up to three different representations at any given time: the
    // restricted native representation (`m_color_slots_r`); the unrestricted native
    // representation (`m_color_slots_u`); and the neutral representation
    // (`m_color_slots_f`).
    //
    // The unrestricted native representation uses the native component representation
    // scheme and color space of the reader, and alpha components are always present.
    //
    // The restricted native representation is like the unrestricted one, except that all
    // colors are guaranteed to be solid. If the corresponding unrestricted representation
    // holds a non-solid color, the solid color is obtained by blending with solid black
    // (non-solid color OVER solid black).
    //
    // The neutral representation uses the floating-point component representation scheme
    // and the native color space of the reader, and alpha components are always present.
    //
    // When the restricted native representations are allocated, `m_color_slots_r` points to
    // an array of components of type `image::comp_type<R>` where `R` is the native
    // component representation scheme of the reader.
    //
    // When the unrestricted native representations are allocated, `m_color_slots_u` points
    // to an array of components of type `image::comp_type<R>` where `R` is the native
    // component representation scheme of the reader.
    //
    // The size of the component arrays for all three representations is `s_num_color_slots`
    // times N, where N is `m_num_channels_ext`. Every N components make up one
    // representation of a color.
    //
    void* m_color_slots_r = nullptr;
    void* m_color_slots_u = nullptr;
    std::unique_ptr<image::float_type[]> m_color_slots_f;
    ColorSlotCtrl m_color_slot_ctrls[s_num_color_slots];

    // Two different caching representatrions of the palette may exist at any point in time:
    // the native representation (`m_palette_cache`) and the neutral representation
    // (`m_palette_cache_f`).
    //
    // The native representation uses the native component representation scheme and color
    // space of the reader, and alpha components are always present.
    //
    // The neutral representation uses the floating-point component representation scheme
    // and the native color space of the reader, and alpha components are always present.
    //
    // When the native representations are allocated, `m_color_slots_r` points to an array
    // of components of type `image::comp_type<R>` where `R` is the native component
    // representation scheme of the reader.
    //
    // The size of the component arrays for both representations is `m_palette_size` times
    // N, where N is `m_num_channels_ext`. Every N components make up one representation of
    // a palette entry.
    //
    void* m_palette_cache = nullptr;
    std::unique_ptr<image::float_type[]> m_palette_cache_f;

    // The primary workspace buffer (workspace buffer 1) is intended for contexts where no
    // invoked function clobbers any workspace buffers. The chief examples are
    // Reader::read() and Writer::write().
    //
    // The secondary workspace buffer (workspace buffer 2) is intended for contexts where an
    // invoked function does clobber the primary workspace buffer. The chief examples are
    // Reader::read_g() and Writer::write_b().
    //
    // Buffer memory is guaranteed to be maximally aligned (aligned for at least
    // std::max_align_t).
    //
    core::Buffer<std::byte> m_workspace_buffer_1, m_workspace_buffer_2;

    // Verify that the total number of channel components in the attached image (number of
    // channels per pixel times width of image times height of image) is representable in
    // `std::ptrdiff_t` and in `std::size_t`.
    void verify_max_num_components() const;

    static auto determine_palette_size(const image::Image* palette) noexcept -> std::size_t;

    // Whether current color in color slot is solid. Clobbers primary workspace buffer (may
    // clobber contents, and may reallocate memory).
    bool is_solid_color(ColorSlot);

    // Divide operation into sequence of operations on smaller boxes
    template<class F> void subdivide(const image::Box&, F&& func);

    // Handle conversion to caller's pixel format. Tray size must be bounded as if by
    // subdivision. Clobbers primary and secondary workspace buffers (may clobber contents,
    // and may reallocate memory).
    template<image::CompRepr R> void read_g(image::Pos pos, const image::tray_type<R>& tray,
                                            const image::ColorSpace& color_space, bool has_alpha);

    // Specified box (`image::Box(pos, tray.size)`) is allowed to extend beyond the image
    // area. Escaping parts will be filled according to selected horizontal and vertical
    // falloff modes. Tray size must be bounded as if by subdivision. Clobbers primary
    // workspace buffer (may clobber contents, and may reallocate memory).
    template<image::CompRepr R> void read_e(image::Pos pos, const image::tray_type<R>& tray, bool ensure_alpha);

    // Handles palette lookup. Specified box (`image::Box(pos, tray.size)`) must be confined
    // to image area. Tray size must be bounded as if by subdivision. Clobbers primary
    // workspace buffer (may clobber contents, and may reallocate memory).
    template<image::CompRepr R> void read(image::Pos pos, const image::tray_type<R>& tray, bool ensure_alpha);

    // Helper function for read_e(). Determines extent of progenitor sub-box in one
    // direction (horizontal or vertical), and also shifts read box when necessary.
    bool adjust(FalloffMode mode, int image_size, int& read_pos, int read_size, int& progen_pos,
                int& progen_size) noexcept;

    void delete_color_slots() noexcept;

    // ensure_color_slot_f() and init_color_slot_f() clobber the primary workspace buffer
    // (it may clobber contents, and it may reallocate memory).
    auto ensure_color_slot_f(ColorSlot) -> const image::float_type*;
    auto get_color_slot_f(ColorSlot) noexcept -> image::float_type*;
    void init_color_slot_f(ColorSlot);
    void ensure_color_slots_f();
    void alloc_color_slots_f();

    // ensure_color_slot_r() and init_color_slot_r() clobber the primary workspace buffer
    // (it may clobber contents, and it may reallocate memory).
    template<image::CompRepr R> auto ensure_color_slot_r(ColorSlot) -> const image::comp_type<R>*;
    template<image::CompRepr R> auto get_color_slot_r(ColorSlot) noexcept -> image::comp_type<R>*;
    template<image::CompRepr R> void init_color_slot_r(ColorSlot);
    template<image::CompRepr R> void ensure_color_slots_r();
    template<image::CompRepr R> void alloc_color_slots_r();

    // ensure_color_slot_u() and init_color_slot_u() clobber the primary workspace buffer
    // (it may clobber contents, and it may reallocate memory).
    template<image::CompRepr R> auto ensure_color_slot_u(ColorSlot) -> const image::comp_type<R>*;
    template<image::CompRepr R> auto get_color_slot_u(ColorSlot) noexcept -> image::comp_type<R>*;
    template<image::CompRepr R> void init_color_slot_u(ColorSlot);
    template<image::CompRepr R> void ensure_color_slots_u();
    template<image::CompRepr R> void alloc_color_slots_u();

    auto get_color_slot_ctrl(ColorSlot) -> ColorSlotCtrl&;

    // Clobbers primary workspace buffer (may clobber contents, and may reallocate memory).
    void set_default_color(ColorSlot);

    // Clobbers primary workspace buffer (may clobber contents, and may reallocate memory).
    template<image::CompRepr R> void do_set_color(ColorSlot, const image::comp_type<R>* components,
                                                  const image::ColorSpace&, bool has_alpha, image::float_type opacity);

    auto ensure_palette_cache_f() -> const image::float_type*;
    template<image::CompRepr R> auto ensure_palette_cache() -> const image::comp_type<R>*;
    auto get_palette_cache_f() -> const image::float_type*;
    void instantiate_palette_cache_f();
    void instantiate_palette_cache();
    void delete_palette_cache() noexcept;

    template<image::CompRepr R, image::CompRepr S>
    void convert_1(image::const_tray_type<R> origin, bool origin_has_alpha, image::iter_type<S> destin,
                   bool destin_has_alpha) noexcept;

    // Clobbers primary workspace buffer (may clobber contents, and may reallocate memory)
    template<image::CompRepr R, image::CompRepr S>
    void convert_2(image::const_tray_type<R> origin, const image::ColorSpace& origin_color_space,
                   bool origin_has_alpha, image::iter_type<S> destin, const image::ColorSpace& destin_color_space,
                   bool destin_has_alpha);

    auto find_color_space_converter(const image::ColorSpace& origin, const image::ColorSpace& destin) const noexcept ->
        const image::ColorSpaceConverter*;

    template<class F> auto repr_dispatch_nothrow(F&& func) const noexcept;
    template<class F> auto repr_dispatch(F&& func) const;
};



/// \brief Behavior when reading outside image boundary.
///
/// These modes control the behavior when reading outside the image boundary, for example,
/// using \ref Image::get_block_rgb().
///
/// When both modes (horizontal and vertical) are set to `background`, any pixel outside the
/// image boundary will be read as if the image was extended indefinitely in all directions
/// with pixels of the the background color.
///
/// When both modes (horizontal and vertical) are set to `edge`, any pixel outside the image
/// boundary will be read as if the image was extended indefinitely in all directions with
/// copies of the closest pixel in the image, or with the the background color if the image
/// is empty (contains no pixels).
///
/// When both modes (horizontal and vertical) are set to `repeat`, any pixel outside the
/// image boundary will be read as if the image was repeated indefinitely in all directions.
///
/// When the horizontal mode is set differently from the vertical mode, the effect is as if
/// the extension of the image is first carried out only in the horizontal directions, and
/// according to the horizontal mode, and is then followed by a vertical extension of the
/// entire horizontal extension according to the vertical mode. Crucially, the effect is the
/// same if the the order of directions is reversed, i.e., if the expansion is first done
/// vertically, and then horizontally.
///
///
/// When both modes are set to `background` (0 denotes the background color):
///
///      image
///      -----------
///     | 1   2   3 |   tray               tray after read
///     |           |-------               ---------------
///     | 4   5   6 | .   . |             | 5   6   0   0 |
///     |           |       |             |               |
///     | 7   8   9 | .   . |    ---->    | 8   9   0   0 |
///      -----------        |             |               |
///         | .   .   .   . |             | 0   0   0   0 |
///          ---------------               ---------------
///
///
/// When both modes are set to `edge`:
///
///      image
///      -----------
///     | 1   2   3 |   tray               tray after read
///     |           |-------               ---------------
///     | 4   5   6 | .   . |             | 5   6   6   6 |
///     |           |       |             |               |
///     | 7   8   9 | .   . |    ---->    | 8   9   9   9 |
///      -----------        |             |               |
///         | .   .   .   . |             | 8   9   9   9 |
///          ---------------               ---------------
///
///
/// When both modes are set to `repeat`:
///
///      image
///      -----------
///     | 1   2   3 |   tray               tray after read
///     |           |-------               ---------------
///     | 4   5   6 | .   . |             | 5   6   4   5 |
///     |           |       |             |               |
///     | 7   8   9 | .   . |    ---->    | 8   9   7   8 |
///      -----------        |             |               |
///         | .   .   .   . |             | 2   3   1   2 |
///          ---------------               ---------------
///
/// A specialization of \ref core::EnumTraits is provided, making stream input and output
/// available.
///
enum class Reader::FalloffMode {
    background,
    edge,
    repeat,
};



/// \brief Color slot identifiers.
///
/// These are the identifiers for the available color slots in a reader, and in a writer
/// (\re image::Writer) by extension. Each color slot can store an arbitrary color. The
/// color is stored in terms of the native color space of the associated image (\ref
/// get_color_space()).
///
/// The background color has multiple uses, including:
///
/// * It is the color of pixels when reading outside the image boundary while the relevant
///   falloff mode is `background` (see \ref Reader::FalloffMode).
///
/// * It is the replacement color used when reading from an image that uses indirect color,
///   and the color index is out of range (greater than, or equal to the number of colors in
///   the associated palette).
///
/// * It serves as the background color during stencil operations (\ref
///   image::Writer::put_block_mask()). In this case, it is the color associated with an
///   alpha value of 0.
///
/// The foreground color has multiple uses, including:
///
/// * It is the default color for fill operations (\ref image::Writer::fill()).
///
/// * It serves as the foreground color during stencil operations (\ref
///   image::Writer::put_block_mask()). In this case, it is the color associated with an
///   alpha value of 1.
///
/// Each slot has a default color, which is one of the colors from \ref util::colors
/// converted to the native color space of the associated image:
///
/// | Slot         | Default color
/// |--------------|---------------
/// | `background` | `transparent`
/// | `foreground` | `white`
///
enum class Reader::ColorSlot {
    background,
    foreground,
};








// Implementation


inline Reader::Reader(const image::Image& image)
    : m_image(image)
    , m_image_size(image.get_size())
    , m_palette(image.get_palette())
    , m_palette_size(determine_palette_size(m_palette))
    , m_transfer_info(image.get_transfer_info()) // Throws
    , m_num_channels(m_transfer_info.get_num_channels())
    , m_num_channels_ext(m_num_channels + int(!m_transfer_info.has_alpha))
    , m_horz_falloff_mode(FalloffMode::background)
    , m_vert_falloff_mode(FalloffMode::background)
{
    verify_max_num_components(); // Throws
}


inline Reader::~Reader() noexcept
{
    delete_color_slots();
    delete_palette_cache();
}


inline auto Reader::get_image() const noexcept -> const image::Image&
{
    return m_image;
}


inline auto Reader::get_image_size() const noexcept -> image::Size
{
    return m_image_size;
}


inline auto Reader::get_color_space() const noexcept -> const image::ColorSpace&
{
    return *m_transfer_info.color_space;
}


inline bool Reader::has_alpha_channel() const noexcept
{
    return m_transfer_info.has_alpha;
}


inline bool Reader::has_indexed_color() const noexcept
{
    return bool(get_palette());
}


inline auto Reader::get_comp_repr() const noexcept -> image::CompRepr
{
    return m_transfer_info.comp_repr;
}


inline auto Reader::get_palette() const noexcept -> const image::Image*
{
    return m_palette;
}


inline int Reader::get_num_channels() const noexcept
{
    return m_num_channels;
}


inline auto Reader::get_transfer_info() const noexcept -> const image::Image::TransferInfo&
{
    return m_transfer_info;
}


inline auto Reader::get_palette_size() const noexcept -> std::size_t
{
    return m_palette_size;
}


inline auto Reader::set_custom_color_space_converters(const image::ColorSpaceConverterRegistry& converters) noexcept ->
    Reader&
{
    m_custom_color_space_converters = &converters;
    return *this;
}


inline auto Reader::set_falloff_mode(FalloffMode mode) noexcept -> Reader&
{
    return set_falloff_mode(mode, mode);
}


inline auto Reader::set_falloff_mode(FalloffMode horz_mode, FalloffMode vert_mode) noexcept -> Reader&
{
    m_horz_falloff_mode = horz_mode;
    m_vert_falloff_mode = vert_mode;
    return *this;
}


inline auto Reader::set_background_color(util::Color color, image::float_type opacity) -> Reader&
{
    return set_background_color(image::Pixel(color), opacity); // Throws
}


inline auto Reader::set_foreground_color(util::Color color, image::float_type opacity) -> Reader&
{
    return set_foreground_color(image::Pixel(color), opacity); // Throws
}


template<class R> inline auto Reader::set_background_color(const image::Pixel<R>& color, image::float_type opacity) ->
    Reader&
{
    return set_color(ColorSlot::background, color, opacity); // Throws
}


template<class R> inline auto Reader::set_foreground_color(const image::Pixel<R>& color, image::float_type opacity) ->
    Reader&
{
    return set_color(ColorSlot::foreground, color, opacity); // Throws
}


inline auto Reader::get_pixel(image::Pos pos) -> util::Color
{
    image::Pixel_RGBA_8 pixel;
    get_pixel(pos, pixel); // Throws
    return {
        util::Color::comp_type(image::unpack_int<8>(pixel[0])),
        util::Color::comp_type(image::unpack_int<8>(pixel[1])),
        util::Color::comp_type(image::unpack_int<8>(pixel[2])),
        util::Color::comp_type(image::unpack_int<8>(pixel[3])),
    };
}


template<class R> inline auto Reader::get_pixel(image::Pos pos, image::Pixel<R>& pixel) -> Reader&
{
    std::ptrdiff_t horz_stride = pixel.num_channels;
    std::ptrdiff_t vert_stride = horz_stride;
    image::Iter iter = { pixel.data(), horz_stride, vert_stride };
    image::Tray tray = { iter, image::Size(1) };
    return get_block<R>(pos, tray); // Throws
}


inline auto Reader::get_block_lum(image::Pos pos, const int8_tray_type& tray) -> Reader&
{
    return get_block<image::Lum_8>(pos, tray); // Throws
}


inline auto Reader::get_block_luma(image::Pos pos, const int8_tray_type& tray) -> Reader&
{
    return get_block<image::LumA_8>(pos, tray); // Throws
}


inline auto Reader::get_block_rgb(image::Pos pos, const int8_tray_type& tray) -> Reader&
{
    return get_block<image::RGB_8>(pos, tray); // Throws
}


inline auto Reader::get_block_rgba(image::Pos pos, const int8_tray_type& tray) -> Reader&
{
    return get_block<image::RGBA_8>(pos, tray); // Throws
}


template<class R> inline auto Reader::get_block(image::Pos pos, image::PixelBlock<R>& block) -> Reader&
{
    return get_block<R>(pos, block.tray()); // Throws
}


template<class R> auto Reader::get_block(image::Pos pos, const typename R::tray_type& tray) -> Reader&
{
    return get_block_a<R::comp_repr>(pos, tray, R::get_color_space(), R::has_alpha); // Throws
}


template<image::CompRepr R> auto Reader::get_block_a(image::Pos pos, const image::tray_type<R>& tray,
                                                     const image::ColorSpace& color_space, bool has_alpha) -> Reader&
{
    constexpr image::CompRepr repr = R;
    image::Box box = { pos, tray.size };
    subdivide(box, [&](const image::Box& subbox) {
        read_g<repr>(subbox.pos, tray.subtray(subbox, pos), color_space, has_alpha); // Throws
    }); // Throws
    return *this;
}


template<class R> inline auto Reader::set_color(ColorSlot slot, const image::Pixel<R>& color,
                                                image::float_type opacity) -> Reader&
{
    return set_color_a<R::comp_repr>(slot, color.data(), R::get_color_space(), R::has_alpha, opacity); // Throws
}


template<class R> inline auto Reader::get_color(ColorSlot slot, image::Pixel<R>& color) -> Reader&
{
    return get_color_a<R::comp_repr>(slot, color.data(), R::get_color_space(), R::has_alpha); // Throws
}


template<image::CompRepr R> inline auto Reader::set_color_a(ColorSlot slot, const image::comp_type<R>* components,
                                                            const image::ColorSpace& color_space, bool has_alpha,
                                                            image::float_type opacity) -> Reader&
{
    do_set_color<R>(slot, components, color_space, has_alpha, opacity); // Throws
    return *this;
}


template<image::CompRepr R>
auto Reader::get_color_a(ColorSlot slot, image::comp_type<R>* components, const image::ColorSpace& color_space,
                         bool has_alpha) -> Reader&
{
    // Short circuit mode: The requested color can be read without lossy conversion when the
    // requested format is sufficiently close to the native format of the attached
    // image. This is relevant for performance, and also because it ensures that there is no
    // information lost due to conversion. For this to work, the requested format must use
    // the same component representation scheme and color space as the native
    // format. Additionally, because color slots always carry an alpha component, the
    // requested format must also include an alpha channel, or it must be possible to obtain
    // the correct result simply by throwing away the alpha component. If the requested
    // color is solid, the alpha component can be thrown away regardless of component
    // representation scheme. If it is not solid, the alpha component can still be thrown
    // away if the component representation scheme is `_float` due to alpha
    // premultiplication. Throwing away the alpha component in this case corresponds to
    // blending with black (color OVER black) which is the required behavior when an alpha
    // channel is eliminated.
    constexpr image::CompRepr repr = R;
    using comp_type = image::comp_type<repr>;
    int slot_index = int(slot);
    ARCHON_ASSERT(slot_index >= 0 && slot_index < s_num_color_slots);
    const image::ColorSpace& origin_color_space = *m_transfer_info.color_space;
    bool same_comp_repr = (repr == m_transfer_info.comp_repr);
    bool same_color_space = (&color_space == &origin_color_space);
    bool remove_alpha = !has_alpha;
    bool is_float = (repr == image::CompRepr::float_);
    bool is_solid = is_solid_color(slot); // Throws
    bool short_circuit = (same_comp_repr && same_color_space && (!remove_alpha || is_float || is_solid));
    // NOTE: ensure_color_slot_u() and ensure_color_slot_f() clobber the primary workspace
    // buffer.
    if (ARCHON_LIKELY(short_circuit)) {
        // Source from native slot
        const comp_type* color = ensure_color_slot_u<repr>(slot); // Throws
        int n = m_num_channels_ext - int(!has_alpha);
        std::copy(color, color + n, components);
    }
    else {
        // Source from neutral slot
        const image::float_type* color = ensure_color_slot_f(slot); // Throws
        int origin_num_channels = m_num_channels_ext;
        int destin_num_channels = color_space.get_num_channels() + int(has_alpha);
        std::array<image::float_type, s_default_workspace_seed_size> seed_mem;
        impl::Workspace<image::float_type> workspace(seed_mem, m_workspace_buffer_1,
                                                     std::max(origin_num_channels, destin_num_channels)); // Throws
        constexpr image::CompRepr repr_2 = image::CompRepr::float_;
        bool origin_has_alpha = true;
        image::float_type* interm = workspace.data();
        const image::ColorSpaceConverter* custom_converter =
            find_color_space_converter(origin_color_space, color_space);
        image::pixel_convert_a<repr_2, repr>(color, origin_color_space, origin_has_alpha,
                                             components, color_space, has_alpha,
                                             interm, custom_converter); // Throws
    }
    return *this;
}


template<class R> inline auto Reader::palette_lookup(std::size_t color_index, image::Pixel<R>& color) -> Reader&
{
    return palette_lookup_a<R::comp_repr>(color_index, color.data(), R::get_color_space(), R::has_alpha); // Throws
}


template<image::CompRepr R>
auto Reader::palette_lookup_a(std::size_t color_index, image::comp_type<R>* components,
                              const image::ColorSpace& color_space, bool has_alpha) -> Reader&
{
    if (ARCHON_UNLIKELY(color_index >= m_palette_size))
        return get_color_a<R>(ColorSlot::background, components, color_space, has_alpha); // Throws

    // Short circuit mode: The requested color can be read without lossy conversion when the
    // requested format is sufficiently close to the native format of the attached
    // image. This is relevant for performance, and also because it ensures that there is no
    // information lost due to conversion. For this to work, the requested format must use
    // the same component representation scheme and color space as the native
    // format. Additionally, because palette entries always carry an alpha component, the
    // requested format must also include an alpha channel, or it must be possible to obtain
    // the correct result simply by throwing away the alpha component. If the color in the
    // specified palette entry is solid, the alpha component can be thrown away regardless
    // of component representation scheme. If it is not solid, the alpha component can still
    // be thrown away if the component representation scheme is `_float` due to alpha
    // premultiplication. Throwing away the alpha component in this case corresponds to
    // blending with black (color OVER black) which is the required behavior when an alpha
    // channel is eliminated.
    constexpr image::CompRepr repr = R;
    const image::ColorSpace& origin_color_space = *m_transfer_info.color_space;
    int origin_num_channels = m_num_channels_ext;
    bool same_comp_repr = (repr == m_transfer_info.comp_repr);
    bool same_color_space = (&color_space == &origin_color_space);
    bool maybe_short_circuit = (same_comp_repr && same_color_space);
    if (ARCHON_LIKELY(maybe_short_circuit)) {
        using comp_type = image::comp_type<repr>;
        const comp_type* entries = ensure_palette_cache<repr>(); // Throws
        const comp_type* color = entries + origin_num_channels * color_index;
        bool remove_alpha = !has_alpha;
        bool is_float = (repr == image::CompRepr::float_);
        bool is_solid = (color[origin_num_channels - 1] == image::comp_repr_max<repr>()); // Throws
        bool short_circuit = (!remove_alpha || is_float || is_solid);
        if (ARCHON_LIKELY(short_circuit)) {
            // Short circuit case: Source from native slot
            int n = origin_num_channels - int(!has_alpha);
            std::copy(color, color + n, components);
            return *this;
        }
    }
    // Fallback case: Source from neutral slot
    const image::float_type* entries = ensure_palette_cache_f(); // Throws
    const image::float_type* color = entries + origin_num_channels * color_index;
    int destin_num_channels = color_space.get_num_channels() + int(has_alpha);
    std::array<image::float_type, s_default_workspace_seed_size> seed_mem;
    impl::Workspace<image::float_type> workspace(seed_mem, m_workspace_buffer_1,
                                                 std::max(origin_num_channels, destin_num_channels)); // Throws
    constexpr image::CompRepr repr_2 = image::CompRepr::float_;
    bool origin_has_alpha = true;
    image::float_type* interm = workspace.data();
    const image::ColorSpaceConverter* custom_converter =
        find_color_space_converter(origin_color_space, color_space);
    image::pixel_convert_a<repr_2, repr>(color, origin_color_space, origin_has_alpha,
                                         components, color_space, has_alpha,
                                         interm, custom_converter); // Throws
    return *this;
}


inline void Reader::verify_max_num_components() const
{
    std::ptrdiff_t n = 1;
    core::int_mul(n, m_num_channels); // Throws
    core::int_mul(n, m_image_size.width); // Throws
    core::int_mul(n, m_image_size.height); // Throws
    core::int_cast<std::size_t>(n); // Throws
}


inline bool Reader::is_solid_color(ColorSlot slot)
{
    const ColorSlotCtrl& ctrl = get_color_slot_ctrl(slot);
    if (ARCHON_LIKELY(ctrl.have_neutral || ctrl.have_unrestricted_native))
        goto have;
    // Note: set_default_color() clobbers the primary workspace buffer
    set_default_color(slot); // Throws
  have:
    return ctrl.is_solid;
}


template<class F> void Reader::subdivide(const image::Box& box, F&& func)
{
    constexpr int preferred_block_width  = 64;
    constexpr int preferred_block_height = 64;
    constexpr int preferred_block_area = preferred_block_width * preferred_block_height;
    if (ARCHON_LIKELY(box.size.width >= preferred_block_width)) {
        int y = 0;
        for (;;) {
            int h, max_w;
            {
                int remaining_h = box.size.height - y;
                if (ARCHON_LIKELY(remaining_h >= preferred_block_height)) {
                    h = preferred_block_height;
                    max_w = preferred_block_width;
                }
                else {
                    if (remaining_h == 0)
                        break;
                    h = remaining_h;
                    max_w = preferred_block_area / h;
                }
            }
            int x = 0;
            for (;;) {
                int remaining_w = box.size.width - x;
                int w = std::min(remaining_w, max_w);
                func(image::Box(box.pos + image::Size(x, y), { w, h })); // Throws
                x += w;
                if (ARCHON_LIKELY(x < box.size.width))
                    continue;
                break;
            }
            y += h;
        }
    }
    else if (box.size.width > 0) {
        int w = box.size.width;
        int max_h = preferred_block_area / w;
        int x = 0;
        int y = 0;
        for (;;) {
            int remaining_h = box.size.height - y;
            int h = std::min(remaining_h, max_h);
            func(image::Box(box.pos + image::Size(x, y), { w, h })); // Throws
            y += h;
            if (ARCHON_LIKELY(y < box.size.height))
                continue;
            break;
        }
    }
}


template<image::CompRepr R> void Reader::read_g(image::Pos pos, const image::tray_type<R>& tray,
                                                const image::ColorSpace& color_space, bool has_alpha)
{
    constexpr image::CompRepr repr = R;
    bool same_comp_repr = (repr == get_comp_repr());
    bool same_color_space = (&color_space == &get_color_space());
    bool remove_alpha = (!has_alpha && has_alpha_channel());
    bool lossless = (same_comp_repr && same_color_space && !remove_alpha);
    if (ARCHON_LIKELY(lossless)) {
        bool ensure_alpha = has_alpha;
        read_e<repr>(pos, tray, ensure_alpha); // Throws
        return;
    }

    // Using secondary workspace buffer because read_e() and convert_2() clobber primary
    // workspace buffer.
    core::Buffer<std::byte>& buffer = m_workspace_buffer_2;
    int num_channels_ext = m_num_channels_ext;
    impl::Workspace<image::float_type> workspace(buffer, num_channels_ext, tray.size); // Throws
    image::Tray tray_2 = workspace.tray(num_channels_ext, tray.size);
    constexpr image::CompRepr float_repr = image::CompRepr::float_;
    bool ensure_alpha = true;
    read_e<float_repr>(pos, tray_2, ensure_alpha); // Throws
    bool origin_has_alpha = true;
    convert_2<float_repr, repr>(tray_2, get_color_space(), origin_has_alpha,
                                tray.iter, color_space, has_alpha); // Throws
}


template<image::CompRepr R> void Reader::read_e(image::Pos pos, const image::tray_type<R>& tray, bool ensure_alpha)
{
    // Strategy implemented by this function:
    //
    // If the read box (`image::Box(pos, tray.size)`) happens to be confined to the image
    // area, just forward to `read()`. Otherwise, establish a progenitor sub-box (sub-box of
    // the read box) such that its contents can be sourced directly from the image, and the
    // rest of the read box can either be filled with the background color, or be obtained
    // by replicating pixels from inside the progenitor sub-box.
    //
    // In some cases, it is necessary to shift the read box horizontally and/or vertically
    // in order to obtain a suitable overlap with the image. In particular, if the falloff
    // mode is `edge`, and the read box falls entirely to the left of the image, the read
    // box needs to be shifted to the right in order to obtain a 1-pixel overlap with the
    // image. Similarly, when the falloff mode is `repeat`, and the read box does not
    // intersect the image, the read box needs to be shifted by an integer number of image
    // sizes in the relevant directions (horizontally and/or vertically).
    //
    // When the falloff mode is `repeat`, and the read box is not confined to a single
    // repetition module (imaginary copy of the image to the left or right / above or below
    // the actual image), and the read box is also not entirely covering any single
    // repetition module, it is necessary to source the progenitor sub-box by two, or four
    // separate reads from the image. However, to ensure that a single pixel in the image is
    // only read once as part of an invocation of `read_e(()`, the size of the progenitor
    // sub-box must never exceed the size of the image. Additionally, for the sake of
    // simplicity, it is a requirement that the position of the upper-left corner of the
    // progenitor sub-box is located within the principal repetition module (area of actual
    // image).
    //
    // The following is an illustration of the case where the falloff mode is `repeat` in
    // both directions, and the read box is not confined to a single repetition module, nor
    // is it entirely covering any single repetition module:
    //
    //     |                                                               X-axis
    //  ---|---------------------------------------------------------------------->
    //     | .   .   .   .   .   .   . | .   .   .   .   .   .   . |
    //     |   image                   |   repeat                  |
    //     | .   .   .   .   .   .   . | .   .   .   .   .   .   . |
    //     |                -----------------------------------    |
    //     | .   .   .   . | .   .   . | .   .   .   . | .   . | . |
    //     |               |    q_1    |      q_2      |       |   |
    //     | .   .   .   . | .   .   . | .   .   .   . | .   . | . |
    //     |-------------- | ---------- -------------- | ----- | --
    //     | .   .   .   . | .   .   . | .   .   .   . | .   . | . |
    //     |   repeat      |    q_3    |      q_4      |       |   |
    //     | .   .   .   . | .   .   . | .   .   .   . | .   . | . |
    //     |               |---------------------------        |<------ read box
    //     | .   .   .   . | .   .   . | .   .   .   .   .   . | . |
    //     |                -----------------------------------    |
    //     | .   .   .   .   .   .   . | .   .   .   .   .   .   . |
    //     |--------------------------- ---------------------------
    //     |
    //    \|/  Y-axis
    //
    // Here:
    //
    // * `image` is the area corresponding to the actual image.
    //
    // * `repeat` is a repetition module.
    //
    // * `q_1`, `q_2`, `q_3`, and `q_4` are the four quadrants of the progenitor sub-box,
    //   and the progenitor sub-box is the union of those quadrants, which is a sub-box of
    //   the read box.
    //
    // Note how the upper-left corner of the progenitor sub-box is located in the area of
    // the actual image, as required, and that the size of the progenitor sub-box does not
    // exceed the image size in either direction, as required.
    //

    constexpr image::CompRepr repr = R;
    ARCHON_ASSERT(repr == get_comp_repr() || repr == image::CompRepr::float_);
    using comp_type = image::comp_type<repr>;

    // NOTE: read() clobbers the primary workspace buffer
    image::Size image_size = m_image_size;
    image::Box read_box = { pos, tray.size };
    if (ARCHON_LIKELY(read_box.contained_in(image_size))) {
        // Fast path
        read<repr>(pos, tray, ensure_alpha); // Throws
        return;
    }

    // CAUTION: Note that the integer arithmetic in this function is delicately arranged in
    // order to sidestep any possibility for overflow.

    if (ARCHON_UNLIKELY(tray.is_empty()))
        return;

    int num_channels = (ensure_alpha ? m_num_channels_ext : m_num_channels);
    auto bgcolor_fill = [&](image::Pos rel_pos, image::Size size) {
        const comp_type* color;
        auto slot = ColorSlot::background;
        if constexpr (repr == image::CompRepr::float_) {
            color = ensure_color_slot_f(slot); // Throws
        }
        else {
            if (!ensure_alpha) {
                color = ensure_color_slot_r<repr>(slot); // Throws
            }
            else {
                color = ensure_color_slot_u<repr>(slot); // Throws
            }
        }
        tray.subtray({ rel_pos, size }).fill(color, num_channels); // Throws
    };

    // Progenitor sub-box (position is relative to image origin)
    image::Box progen;

    if (ARCHON_UNLIKELY(image_size.is_empty()))
        goto all_background;

    if (ARCHON_UNLIKELY(!adjust(m_horz_falloff_mode, image_size.width, read_box.pos.x, read_box.size.width,
                                progen.pos.x, progen.size.width)))
        goto all_background;
    if (ARCHON_UNLIKELY(!adjust(m_vert_falloff_mode, image_size.height, read_box.pos.y, read_box.size.height,
                                progen.pos.y, progen.size.height)))
        goto all_background;

    // At this point, the position of the progenitor sub-box must be confined to the image
    // area, it must not be empty, its size must not exceed the size of the image, and it
    // must be confined to the read box.
    ARCHON_ASSERT(image::Box(progen.pos, 0).contained_in(image_size));
    ARCHON_ASSERT(!progen.size.is_empty());
    ARCHON_ASSERT(progen.size.contained_in(image_size));
    ARCHON_ASSERT(progen.contained_in(read_box));

    // Read the 4 quadrants of the progenitor sub-box
    {
        // NOTE: read() clobbers the primary workspace buffer
        image::Size main_quad_size = {
            std::min(image_size.width - progen.pos.x, progen.size.width),
            std::min(image_size.height - progen.pos.y, progen.size.height),
        };
        read<repr>(progen.pos, tray.subtray({ progen.pos, main_quad_size }, read_box.pos), ensure_alpha); // Throws
        if (main_quad_size.width < progen.size.width) {
            image::Pos pos_2 = progen.pos.proj_y();
            image::Size size = { progen.size.width - main_quad_size.width, main_quad_size.height };
            read<repr>(pos_2, tray.subtray({ pos_2 + image_size.proj_x(), size }, read_box.pos),
                       ensure_alpha); // Throws
        }
        if (main_quad_size.height < progen.size.height) {
            image::Pos pos_2 = progen.pos.proj_x();
            image::Size size = { main_quad_size.width, progen.size.height - main_quad_size.height };
            read<repr>(pos_2, tray.subtray({ pos_2 + image_size.proj_y(), size }, read_box.pos),
                       ensure_alpha); // Throws
        }
        if (main_quad_size.width < progen.size.width && main_quad_size.height < progen.size.height) {
            image::Pos pos_2 = { 0, 0 };
            image::Size size = progen.size - main_quad_size;
            read<repr>(pos_2, tray.subtray({ pos_2 + image_size, size }, read_box.pos), ensure_alpha); // Throws
        }
    }

    {
        image::Size diff = progen.pos - read_box.pos;
        image::Pos rel_pos_1 = image::Pos() + diff;
        image::Pos rel_pos_2 = rel_pos_1 + progen.size;

        // Expand in the horizontal direction
        switch (m_horz_falloff_mode) {
            case FalloffMode::background:
                if (ARCHON_UNLIKELY(rel_pos_1.x > 0)) {
                    image::Pos rel_pos = { 0, rel_pos_1.y };
                    image::Size size = { rel_pos_1.x, progen.size.height };
                    bgcolor_fill(rel_pos, size); // Throws
                }
                if (ARCHON_UNLIKELY(read_box.size.width > rel_pos_2.x)) {
                    image::Pos rel_pos = { rel_pos_2.x, rel_pos_1.y };
                    image::Size size = { read_box.size.width - rel_pos_2.x, progen.size.height };
                    bgcolor_fill(rel_pos, size); // Throws
                }
                break;
            case FalloffMode::edge:
                if (ARCHON_UNLIKELY(rel_pos_1.x > 0)) {
                    ARCHON_ASSERT(progen.size.width >= 1);
                    image::Pos rel_pos = { 0, rel_pos_1.y };
                    image::Size size = { 1, progen.size.height };
                    image::Iter iter = tray.iter + diff;
                    for (int i = 0; i < rel_pos_1.x; ++i)
                        tray.subtray({ rel_pos + image::Size(i, 0), size }).copy_from(iter, num_channels); // Throws
                }
                if (ARCHON_UNLIKELY(read_box.size.width > rel_pos_2.x)) {
                    ARCHON_ASSERT(progen.size.width >= 1);
                    image::Pos rel_pos = { rel_pos_2.x, rel_pos_1.y };
                    image::Size size = { 1, progen.size.height };
                    image::Iter iter = tray.iter + (diff + (progen.size - 1).proj_x());
                    for (int i = 0; i < read_box.size.width - rel_pos_2.x; ++i)
                        tray.subtray({ rel_pos + image::Size(i, 0), size }).copy_from(iter, num_channels); // Throws
                }
                break;
            case FalloffMode::repeat:
                if (ARCHON_UNLIKELY(rel_pos_1.x > 0)) {
                    ARCHON_ASSERT(progen.size.width == image_size.width);
                    image::Pos rel_pos = { 0, rel_pos_1.y };
                    image::Size size = { rel_pos_1.x, progen.size.height };
                    image::Iter iter = tray.iter + image::Size(progen.size.width, rel_pos_1.y);
                    tray.subtray({ rel_pos, size }).copy_from(iter, num_channels); // Throws
                }
                if (ARCHON_UNLIKELY(read_box.size.width > rel_pos_2.x)) {
                    ARCHON_ASSERT(progen.size.width == image_size.width);
                    image::Pos rel_pos = { rel_pos_2.x, rel_pos_1.y };
                    image::Iter iter = tray.iter + diff;
                    while (ARCHON_LIKELY(read_box.size.width - rel_pos.x >= progen.size.width)) {
                        tray.subtray({ rel_pos, progen.size }).copy_from(iter, num_channels); // Throws
                        rel_pos += progen.size.proj_x();
                    }
                    image::Size size = { read_box.size.width - rel_pos.x, progen.size.height };
                    tray.subtray({ rel_pos, size }).copy_from(iter, num_channels); // Throws
                }
                break;
        }

        // Expand in the vertical direction
        switch (m_vert_falloff_mode) {
            case FalloffMode::background:
                if (ARCHON_UNLIKELY(rel_pos_1.y > 0)) {
                    image::Pos rel_pos = { 0, 0 };
                    image::Size size = { read_box.size.width, rel_pos_1.y };
                    bgcolor_fill(rel_pos, size); // Throws
                }
                if (ARCHON_UNLIKELY(read_box.size.height > rel_pos_2.y)) {
                    image::Pos rel_pos = { 0, rel_pos_2.y };
                    image::Size size = { read_box.size.width, read_box.size.height - rel_pos_2.y };
                    bgcolor_fill(rel_pos, size); // Throws
                }
                break;
            case FalloffMode::edge:
                if (ARCHON_UNLIKELY(rel_pos_1.y > 0)) {

                    ARCHON_ASSERT(progen.size.height >= 1);
                    image::Pos rel_pos = { 0, 0 };
                    image::Size size = { read_box.size.width, 1 };
                    image::Iter iter = tray.iter + diff.proj_y();
                    for (int i = 0; i < rel_pos_1.y; ++i)
                        tray.subtray({ rel_pos + image::Size(0, i), size }).copy_from(iter, num_channels); // Throws
                }
                if (ARCHON_UNLIKELY(read_box.size.height > rel_pos_2.y)) {
                    ARCHON_ASSERT(progen.size.height >= 1);
                    image::Pos rel_pos = { 0, rel_pos_2.y };
                    image::Size size = { read_box.size.width, 1 };
                    image::Iter iter = tray.iter + (diff + progen.size - 1).proj_y();
                    for (int i = 0; i < read_box.size.height - rel_pos_2.y; ++i)
                        tray.subtray({ rel_pos + image::Size(0, i), size }).copy_from(iter, num_channels); // Throws
                }
                break;
            case FalloffMode::repeat:
                if (ARCHON_UNLIKELY(rel_pos_1.y > 0)) {
                    ARCHON_ASSERT(progen.size.height == image_size.height);
                    image::Pos rel_pos = { 0, 0 };
                    image::Size size = { read_box.size.width, rel_pos_1.y };
                    image::Iter iter = tray.iter + progen.size.proj_y();
                    tray.subtray({ rel_pos, size }).copy_from(iter, num_channels); // Throws
                }
                if (ARCHON_UNLIKELY(read_box.size.height > rel_pos_2.y)) {
                    ARCHON_ASSERT(progen.size.height == image_size.height);
                    image::Pos rel_pos = { 0, rel_pos_2.y };
                    image::Size size_1 = { read_box.size.width, progen.size.height };
                    image::Iter iter = tray.iter + diff.proj_y();
                    while (ARCHON_LIKELY(read_box.size.height - rel_pos.y >= progen.size.height)) {
                        tray.subtray({ rel_pos, size_1 }).copy_from(iter, num_channels); // Throws
                        rel_pos += progen.size.proj_y();
                    }
                    image::Size size_2 = { read_box.size.width, read_box.size.height - rel_pos.y };
                    tray.subtray({ rel_pos, size_2 }).copy_from(iter, num_channels); // Throws
                }
                break;
        }
    }
    return;

  all_background:
    bgcolor_fill({}, read_box.size); // Throws
}


template<image::CompRepr R> void Reader::read(image::Pos pos, const image::tray_type<R>& tray, bool ensure_alpha)
{
    constexpr image::CompRepr repr = R;
    ARCHON_ASSERT(repr == get_comp_repr() || repr == image::CompRepr::float_);

    using comp_type = image::comp_type<repr>;
    constexpr image::CompRepr float_repr = image::CompRepr::float_;
    bool direct_color = !has_indexed_color();
    if (ARCHON_LIKELY(direct_color)) {
        if constexpr (repr == float_repr) {
            if (ARCHON_LIKELY(get_comp_repr() != float_repr)) {
                repr_dispatch([&](auto tag) {
                    constexpr image::CompRepr repr = tag.comp_repr;
                    using comp_type = image::comp_type<repr>;
                    core::Buffer<std::byte>& buffer = m_workspace_buffer_1;
                    int num_channels = m_num_channels;
                    impl::Workspace<comp_type> workspace(buffer, num_channels, tray.size); // Throws
                    image::Tray tray_2 = workspace.tray(num_channels, tray.size);
                    m_image.read(pos, tray_2); // Throws
                    bool origin_has_alpha = has_alpha_channel();
                    bool destin_has_alpha = (origin_has_alpha || ensure_alpha);
                    convert_1<repr, float_repr>(tray_2, origin_has_alpha, tray.iter, destin_has_alpha);
                }); // Throws
                return;
            }
        }
        m_image.read(pos, tray); // Throws
        bool add_alpha = (ensure_alpha && !has_alpha_channel());
        if (add_alpha) {
            int i = m_num_channels_ext - 1;
            comp_type alpha = image::comp_repr_max<repr>();
            for (int y = 0; y < tray.size.height; ++y) {
                for (int x = 0; x < tray.size.width; ++x) {
                    comp_type* pixel = tray(x, y);
                    pixel[i] = alpha;
                }
            }
        }
        return;
    }

    // NOTE: ensure_color_slot_f(), ensure_color_slot_u(), and ensure_color_slot_r() clobber
    // the primary workspace buffer.
    std::size_t palette_size = get_palette_size();
    const comp_type* palette_base = nullptr;
    const comp_type* bgcolor = nullptr;
    if constexpr (repr == float_repr) {
        if (ARCHON_LIKELY(get_comp_repr() != float_repr)) {
            palette_base = ensure_palette_cache_f(); // Throws
            bgcolor = ensure_color_slot_f(ColorSlot::background); // Throws
        }
    }
    if (!palette_base) {
        palette_base = ensure_palette_cache<repr>(); // Throws
        if (ensure_alpha) {
            bgcolor = ensure_color_slot_u<repr>(ColorSlot::background); // Throws
        }
        else {
            bgcolor = ensure_color_slot_r<repr>(ColorSlot::background); // Throws
        }
    }

    constexpr image::CompRepr index_repr = image::color_index_repr; // FIXME: Should be made variable, and be provided through TransferInfo                                  
    using index_comp_type = image::comp_type<index_repr>;
    core::Buffer<std::byte>& buffer = m_workspace_buffer_1;
    int num_index_channels = 1;
    impl::Workspace<index_comp_type> workspace(buffer, num_index_channels, tray.size); // Throws
    image::Tray tray_2 = workspace.tray(num_index_channels, tray.size);
    m_image.read(pos, tray_2); // Throws

    int num_channels = (ensure_alpha ? m_num_channels_ext : m_num_channels);
    int num_channels_ext = m_num_channels_ext;
    for (int y = 0; y < tray.size.height; ++y) {
        for (int x = 0; x < tray.size.width; ++x) {
            const index_comp_type* origin = tray_2(x, y);
            auto index = core::to_unsigned(image::comp_repr_unpack<index_repr>(*origin));
            const comp_type* color = bgcolor;
            if (ARCHON_LIKELY(index < palette_size))
                color = palette_base + index * std::size_t(num_channels_ext);
            comp_type* destin = tray(x, y);
            std::copy(color, color + num_channels, destin);
        }
    }
}


inline auto Reader::ensure_color_slot_f(ColorSlot slot) -> const image::float_type*
{
    const ColorSlotCtrl& ctrl = get_color_slot_ctrl(slot);
    if (ARCHON_LIKELY(ctrl.have_neutral))
        goto have;
    // Note: init_color_slot_f() clobbers the primary workspace buffer
    init_color_slot_f(slot); // Throws
    ARCHON_ASSERT(ctrl.have_neutral);
  have:
    return get_color_slot_f(slot);
}


inline auto Reader::get_color_slot_f(ColorSlot slot) noexcept -> image::float_type*
{
    int slot_index = int(slot);
    ARCHON_ASSERT(slot_index >= 0 && slot_index < s_num_color_slots);
    ARCHON_ASSERT(m_color_slots_f);
    return m_color_slots_f.get() + slot_index * m_num_channels_ext;
}


inline void Reader::ensure_color_slots_f()
{
    if (ARCHON_LIKELY(m_color_slots_f))
        return;
    alloc_color_slots_f(); // Throws
}


template<image::CompRepr R> inline auto Reader::ensure_color_slot_r(ColorSlot slot) -> const image::comp_type<R>*
{
    constexpr image::CompRepr repr = R;
    const ColorSlotCtrl& ctrl = get_color_slot_ctrl(slot);
    if (ARCHON_LIKELY(ctrl.have_restricted_native))
        goto have;
    // Note: init_color_slot_r() clobbers the primary workspace buffer
    init_color_slot_r<repr>(slot); // Throws
  have:
    return get_color_slot_r<repr>(slot);
}


template<image::CompRepr R> inline auto Reader::get_color_slot_r(ColorSlot slot) noexcept -> image::comp_type<R>*
{
    constexpr image::CompRepr repr = R;
    ARCHON_ASSERT(m_color_slots_r);
    using comp_type = image::comp_type<repr>;
    comp_type* components = static_cast<comp_type*>(m_color_slots_r);
    int slot_index = int(slot);
    ARCHON_ASSERT(slot_index >= 0 && slot_index < s_num_color_slots);
    return components + slot_index * m_num_channels_ext;
}


template<image::CompRepr R> void Reader::init_color_slot_r(ColorSlot slot)
{
    constexpr image::CompRepr repr = R;
    using comp_type = image::comp_type<repr>;
    ColorSlotCtrl& ctrl = get_color_slot_ctrl(slot);
    ARCHON_ASSERT(!ctrl.have_restricted_native);
    ensure_color_slots_r<repr>(); // Throws
    comp_type* destin = get_color_slot_r<repr>(slot);
    if (ARCHON_LIKELY(ctrl.is_solid && ctrl.have_unrestricted_native)) {
        const comp_type* origin = get_color_slot_u<repr>(slot);
        std::copy(origin, origin + m_num_channels_ext, destin);
    }
    else {
        // Note: ensure_color_slot_f() clobbers the primary workspace buffer
        constexpr image::CompRepr float_repr = image::CompRepr::float_;
        const image::float_type* origin = ensure_color_slot_f(slot); // Throws
        bool origin_has_alpha = false; // Ignore the alpha component
        bool destin_has_alpha = true;
        int num_color_space_channels = m_num_channels_ext - 1;
        image::pixel_convert<float_repr, repr>(origin, origin_has_alpha, destin, destin_has_alpha,
                                               num_color_space_channels);
    }
    ctrl.have_restricted_native = true;
}


template<image::CompRepr R> inline void Reader::ensure_color_slots_r()
{
    constexpr image::CompRepr repr = R;
    if (ARCHON_LIKELY(m_color_slots_r))
        return;
    alloc_color_slots_r<repr>(); // Throws
}


template<image::CompRepr R> void Reader::alloc_color_slots_r()
{
    constexpr image::CompRepr repr = R;
    using comp_type = image::comp_type<repr>;
    ARCHON_ASSERT(!m_color_slots_r);
    ARCHON_ASSERT(repr == m_transfer_info.comp_repr);
    std::size_t size = 1;
    core::int_mul(size, m_num_channels_ext); // Throws
    core::int_mul(size, s_num_color_slots); // Throws
    m_color_slots_r = new comp_type[size]; // Throws
}


template<image::CompRepr R> inline auto Reader::ensure_color_slot_u(ColorSlot slot) -> const image::comp_type<R>*
{
    constexpr image::CompRepr repr = R;
    const ColorSlotCtrl& ctrl = get_color_slot_ctrl(slot);
    if (ARCHON_LIKELY(ctrl.have_unrestricted_native))
        goto have;
    // Note: init_color_slot_u() clobbers the primary workspace buffer
    init_color_slot_u<repr>(slot); // Throws
  have:
    return get_color_slot_u<repr>(slot);
}


template<image::CompRepr R> inline auto Reader::get_color_slot_u(ColorSlot slot) noexcept -> image::comp_type<R>*
{
    constexpr image::CompRepr repr = R;
    ARCHON_ASSERT(m_color_slots_u);
    using comp_type = image::comp_type<repr>;
    comp_type* components = static_cast<comp_type*>(m_color_slots_u);
    int slot_index = int(slot);
    ARCHON_ASSERT(slot_index >= 0 && slot_index < s_num_color_slots);
    return components + slot_index * m_num_channels_ext;
}


template<image::CompRepr R> void Reader::init_color_slot_u(ColorSlot slot)
{
    constexpr image::CompRepr repr = R;

    ColorSlotCtrl& ctrl = get_color_slot_ctrl(slot);
    ARCHON_ASSERT(!ctrl.have_unrestricted_native);
    if (ARCHON_LIKELY(ctrl.have_neutral))
        goto convert;
    // Note: set_default_color() clobbers the primary workspace buffer
    set_default_color(slot); // Throws
    if (ARCHON_LIKELY(ctrl.have_neutral))
        goto convert;
    ARCHON_ASSERT(ctrl.have_unrestricted_native);
    return;

  convert:
    ensure_color_slots_u<repr>(); // Throws
    using comp_type = image::comp_type<repr>;
    constexpr image::CompRepr float_repr = image::CompRepr::float_;
    image::float_type* origin = get_color_slot_f(slot);
    comp_type* destin = get_color_slot_u<repr>(slot);
    bool has_alpha = true;
    image::comp_repr_convert<float_repr, repr>(origin, destin, m_num_channels_ext, has_alpha);
    ctrl.have_unrestricted_native = true;
}


template<image::CompRepr R> inline void Reader::ensure_color_slots_u()
{
    constexpr image::CompRepr repr = R;
    if (ARCHON_LIKELY(m_color_slots_u))
        return;
    alloc_color_slots_u<repr>(); // Throws
}


template<image::CompRepr R> void Reader::alloc_color_slots_u()
{
    constexpr image::CompRepr repr = R;
    ARCHON_ASSERT(!m_color_slots_u);
    ARCHON_ASSERT(repr == m_transfer_info.comp_repr);
    std::size_t size = 1;
    core::int_mul(size, m_num_channels_ext); // Throws
    core::int_mul(size, s_num_color_slots); // Throws
    using comp_type = image::comp_type<repr>;
    m_color_slots_u = new comp_type[size]; // Throws
}


inline auto Reader::get_color_slot_ctrl(ColorSlot slot) -> ColorSlotCtrl&
{
    int slot_index = int(slot);
    ARCHON_ASSERT(slot_index >= 0 && slot_index < s_num_color_slots);
    return m_color_slot_ctrls[slot_index];
}


template<image::CompRepr R>
void Reader::do_set_color(ColorSlot slot, const image::comp_type<R>* components, const image::ColorSpace& color_space,
                          bool has_alpha, image::float_type opacity)
{
    constexpr image::CompRepr repr = R;
    using comp_type = image::comp_type<repr>;
    const image::ColorSpace& destin_color_space = *m_transfer_info.color_space;
    bool same_color_space = (&color_space == &destin_color_space);
    bool same_comp_repr = (repr == m_transfer_info.comp_repr);
    bool add_alpha = !has_alpha;
    if (ARCHON_LIKELY(same_color_space && same_comp_repr && opacity == 1)) {
        // Store in unrestricted native form
        ensure_color_slots_u<repr>(); // Throws
        comp_type* target = get_color_slot_u<repr>(slot);
        int n = m_num_channels_ext - int(add_alpha);
        std::copy_n(components, n, target);
        bool is_solid = true;
        if (ARCHON_LIKELY(!add_alpha)) {
            ARCHON_ASSERT(has_alpha);
            comp_type alpha = target[n - 1];
            is_solid = (alpha == image::comp_repr_max<repr>());
        }
        else {
            comp_type alpha = image::comp_repr_max<repr>();
            target[n] = alpha;
        }
        ColorSlotCtrl& ctrl = get_color_slot_ctrl(slot);
        ctrl.have_neutral = false;
        ctrl.have_restricted_native = false;
        ctrl.have_unrestricted_native = true;
        ctrl.is_solid = is_solid;
        return;
    }

    // Convert to neutral form
    constexpr image::CompRepr repr_2 = image::CompRepr::float_;
    ensure_color_slots_f(); // Throws
    image::float_type* destin = get_color_slot_f(slot);
    bool destin_has_alpha = true;
    int num_channels = color_space.get_num_channels() + int(has_alpha);
    int destin_num_channels = m_num_channels_ext;
    std::array<image::float_type, s_default_workspace_seed_size> seed_mem;
    impl::Workspace<image::float_type> workspace(seed_mem, m_workspace_buffer_1,
                                                 std::max(num_channels, destin_num_channels)); // Throws
    image::float_type* interm = workspace.data();
    const image::ColorSpaceConverter* custom_converter = find_color_space_converter(color_space, destin_color_space);
    image::pixel_convert_a<repr, repr_2>(components, color_space, has_alpha,
                                         destin, destin_color_space, destin_has_alpha,
                                         interm, custom_converter); // Throws

    // Apply opacity
    for (int i = 0; i < destin_num_channels; ++i)
        destin[i] *= opacity;

    image::float_type alpha = destin[destin_num_channels - 1];
    ColorSlotCtrl& ctrl = get_color_slot_ctrl(slot);
    ctrl.have_neutral = true;
    ctrl.have_restricted_native = false;
    ctrl.have_unrestricted_native = false;
    ctrl.is_solid = (alpha == 1);
}


inline auto Reader::ensure_palette_cache_f() -> const image::float_type*
{
    if (ARCHON_UNLIKELY(!m_palette_cache_f))
        instantiate_palette_cache_f(); // Throws
    return m_palette_cache_f.get();
}


template<image::CompRepr R> inline auto Reader::ensure_palette_cache() -> const image::comp_type<R>*
{
    constexpr image::CompRepr repr = R;
    ARCHON_ASSERT(repr == m_transfer_info.comp_repr);
    if (ARCHON_UNLIKELY(!m_palette_cache))
        instantiate_palette_cache(); // Throws
    using comp_type = image::comp_type<repr>;
    return static_cast<comp_type*>(m_palette_cache);
}


inline auto Reader::get_palette_cache_f() -> const image::float_type*
{
    ARCHON_ASSERT(m_palette_cache_f);
    return m_palette_cache_f.get();
}


template<image::CompRepr R, image::CompRepr S>
void Reader::convert_1(image::const_tray_type<R> origin, bool origin_has_alpha, image::iter_type<S> destin,
                       bool destin_has_alpha) noexcept
{
    int num_color_space_channels = m_num_channels_ext - 1;
    for (int y = 0; y < origin.size.height; ++y) {
        for (int x = 0; x < origin.size.width; ++x) {
            image::pixel_convert<R, S>(origin(x, y), origin_has_alpha, destin(x, y), destin_has_alpha,
                                       num_color_space_channels);
        }
    }
}


template<image::CompRepr R, image::CompRepr S>
void Reader::convert_2(image::const_tray_type<R> origin, const image::ColorSpace& origin_color_space,
                       bool origin_has_alpha, image::iter_type<S> destin, const image::ColorSpace& destin_color_space,
                       bool destin_has_alpha)
{
    int origin_num_channels = origin_color_space.get_num_channels() + int(origin_has_alpha);
    int destin_num_channels = destin_color_space.get_num_channels() + int(destin_has_alpha);
    int num_channels = std::max(origin_num_channels, destin_num_channels);
    std::array<image::float_type, s_default_workspace_seed_size> seed_mem;
    impl::Workspace<image::float_type> workspace(seed_mem, m_workspace_buffer_1, num_channels); // Throws
    image::float_type* interm = workspace.data();
    const image::ColorSpaceConverter* custom_converter =
        find_color_space_converter(origin_color_space, destin_color_space);
    for (int y = 0; y < origin.size.height; ++y) {
        for (int x = 0; x < origin.size.width; ++x) {
            image::pixel_convert_a<R, S>(origin(x, y), origin_color_space, origin_has_alpha,
                                         destin(x, y), destin_color_space, destin_has_alpha,
                                         interm, custom_converter); // Throws
        }
    }
}


inline auto Reader::find_color_space_converter(const image::ColorSpace& origin,
                                               const image::ColorSpace& destin) const noexcept ->
    const image::ColorSpaceConverter*
{
    if (ARCHON_LIKELY(!m_custom_color_space_converters))
        return nullptr;
    return m_custom_color_space_converters->find(origin, destin);
}


template<class F> auto Reader::repr_dispatch_nothrow(F&& func) const noexcept
{
    switch (m_transfer_info.comp_repr) {
        case image::CompRepr::int8:
            static_assert(noexcept(func(CompReprTag<image::CompRepr::int8>())));
            return func(CompReprTag<image::CompRepr::int8>());
        case image::CompRepr::int16:
            static_assert(noexcept(func(CompReprTag<image::CompRepr::int16>())));
            return func(CompReprTag<image::CompRepr::int16>());
        case image::CompRepr::float_:
            static_assert(noexcept(func(CompReprTag<image::CompRepr::float_>())));
            return func(CompReprTag<image::CompRepr::float_>());
    }
    ARCHON_STEADY_ASSERT_UNREACHABLE();
}


template<class F> auto Reader::repr_dispatch(F&& func) const
{
    switch (m_transfer_info.comp_repr) {
        case image::CompRepr::int8:
            return func(CompReprTag<image::CompRepr::int8>()); // Throws
        case image::CompRepr::int16:
            return func(CompReprTag<image::CompRepr::int16>()); // Throws
        case image::CompRepr::float_:
            return func(CompReprTag<image::CompRepr::float_>()); // Throws
    }
    ARCHON_STEADY_ASSERT_UNREACHABLE();
}


} // namespace archon::image

namespace archon::core {

template<> struct EnumTraits<image::Reader::FalloffMode> {
    static constexpr bool is_specialized = true;
    struct Spec {
        static constexpr core::EnumAssoc map[] = {
            { int(image::Reader::FalloffMode::background), "background" },
            { int(image::Reader::FalloffMode::edge),       "edge"       },
            { int(image::Reader::FalloffMode::repeat),     "repeat"     },
        };
    };
    static constexpr bool ignore_case = true;
};

} // namespace archon::core

#endif // ARCHON_X_IMAGE_X_READER_HPP
