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

#ifndef ARCHON_X_IMAGE_X_WRITER_HPP
#define ARCHON_X_IMAGE_X_WRITER_HPP

/// \file


#include <cstddef>
#include <algorithm>
#include <memory>
#include <utility>
#include <array>
#include <optional>
#include <stdexcept>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/buffer.hpp>
#include <archon/util/kdtree.hpp>
#include <archon/util/color.hpp>
#include <archon/image/geom.hpp>
#include <archon/image/iter.hpp>
#include <archon/image/tray.hpp>
#include <archon/image/comp_types.hpp>
#include <archon/image/comp_repr.hpp>
#include <archon/image/color_space.hpp>
#include <archon/image/pixel_repr.hpp>
#include <archon/image/pixel.hpp>
#include <archon/image/block.hpp>
#include <archon/image/transfer_info.hpp>
#include <archon/image/image.hpp>
#include <archon/image/writable_image.hpp>
#include <archon/image/impl/subdivide.hpp>
#include <archon/image/impl/workspace.hpp>
#include <archon/image/reader.hpp>


namespace archon::image {


/// \brief Convenience facility for writing pixels to images.
///
///    
///
class Writer
    : public image::Reader {
public:
    /// \brief Construct pixel writer for image.
    ///
    /// This constructor sets up a writer to read from, and write to the specified
    /// image. The specified image becomes the attached image of the writer.
    ///
    /// The total number of channel components in the specified image (number of channels
    /// per pixel times width of image times height of image) must be representable in
    /// `std::ptrdiff_t` and in `std::size_t`. If the total number of channel components in
    /// the specified image is too large, an exception will be thrown.
    ///
    Writer(image::WritableImage&);

    /// \brief Get reference to attached writable image.
    ///
    /// This function returns a reference to the attached image. It overrides \ref
    /// image::Image::get_image() in order to reflect the fact that the attached image is
    /// writable (of type \ref image::WritableImage).
    ///
    auto get_image() const noexcept -> image::WritableImage&;

    /// \{
    ///
    /// \brief    
    ///
    ///    
    ///
    /// When blending is enabled, a written pixel, that is not fully opaque (alpha component
    /// is not 1 or opacity parameter of writer is not 1), is blended with the original
    /// target pixel (written pixel OVER original target pixel). If there is no alpha channel
    /// in the written pixels, the alpha components are taken to be 1. When blending is
    /// disabled, if the target image does not have an alpha channel, a written pixel, that
    /// is not fully opaque, is blended with fully opaque black (written pixel OVER fully
    /// opaque black). When blending is disabled, and the image has an alpha channel, the
    /// pixel, including its alpha component (modulated by the opacity parameter of writer),
    /// simply replaces the original target pixel.
    ///
    /// When writing of pixels involves both color space conversion and blending, color
    /// space conversion happens before blending. When non-linear color space conversions
    /// are involved, the order does matter.
    ///
    /// Blending always happens in the native color space of the writer (see \ref
    /// get_color_space()), and always in terms of linearly expressed component values (as
    /// opposed to gamma compressed component values).
    ///
    /// Blending is disabled by default.
    ///
    auto enable_blending() noexcept -> Writer&;
    auto disable_blending() noexcept -> Writer&;
    auto set_blending_enabled(bool) noexcept -> Writer&;
    bool get_blending_enabled() noexcept;
    /// \}

    /// \{
    ///
    /// \brief    
    ///
    ///    
    ///
    /// Opacity applies to all operations that write pixels to the underlying image. This
    /// includes both \ref put_block() and \ref fill(). The specified opacity modulates the
    /// alpha component of a written pixel as it is about to be applied to the underlying
    /// image in the manner selected by the blending mode (see \ref set_blending_enabled()).
    ///
    /// The default opacity is 1.
    ///
    /// FIXME: Explain what happens if the specified value lies outside the nominal range
    /// [0,1], and whether it is allowed at all? Unspecified, but not undefined behavior               
    ///
    auto set_opacity(image::float_type) noexcept -> Writer&;
    auto get_opacity() noexcept -> image::float_type;
    /// \}

    /// \{
    ///
    /// \brief Fill area with current color of color slot.
    ///
    /// This function fills the specified area with the current color in the specified color
    /// slot (\ref set_color()), or in the case of the one-argument overload, it fills the
    /// entire image with that color.
    ///
    /// The specified area is allowed to extend beyond, or even lie fully outside the
    /// boundary of the image. Only the part of the image that overlaps with the specified
    /// area will be affected.
    ///
    /// Filling is subject to the selected opacity (\ref set_opacity()) and to blending when
    /// enabled (\ref set_blending_enabled()).
    ///
    /// This function operates in a lossless manner if all of the following conditions are
    /// met:
    ///
    /// * The fill color (\p slot) was set in a lossless manner (see \ref set_color_a()).
    ///
    /// * The fill color (\p slot) was set without an alpha component, or the fill color is
    ///   solid (alpha component at maximum value), or the attached image has an alpha
    ///   channel (\ref has_alpha_channel()).
    ///
    /// * Blending is disabled (it is by default).
    ///
    /// * The configured opacity is 1 (this is the default value).
    ///
    /// * The attached image uses direct color (\ref has_indirect_color()).
    ///
    /// When operating in a lossless manner, this function guarantees that the fill
    /// operation is carried out by way of one or more invocations of \ref
    /// image::WritableImage::fill(), and that the component values of the color passed to
    /// \ref image::WritableImage::fill() are exactly equal to those of the color stored in
    /// the specified color slot (\p slot) with the proviso that if the fill color was set
    /// without an alpha component, and the target image has an alpha channel, then an alpha
    /// component with maximum value (full opacity) is introduced.
    ///
    auto fill(ColorSlot slot = ColorSlot::foreground) -> Writer&;
    auto fill(const image::Box& area, ColorSlot slot = ColorSlot::foreground) -> Writer&;
    /// \}

    /// \brief Write other image to this image.
    ///
    /// This function is a shorthand for calling \ref put_image_a() with a reader
    /// constructed from the specified image (\p img), and a box constructed from the size
    /// of the specified image.
    ///
    /// \sa \ref put_image_a()
    /// \sa \ref image::Reader
    ///
    auto put_image(image::Pos pos, const image::Image& img) -> Writer&;

    /// \brief Write specific part of other image to this image.
    ///
    /// This function extracts a block of pixels from the source image, which is the image
    /// attached to the specified reader (\p reader), and writes that block to the target
    /// image, which is the image attached to this writer, at the specified position (\p
    /// pos) within the target image. The size and position within the source image of the
    /// extracted block is specified by \p box.
    ///
    /// FIXME: Talk about blending     
    ///
    /// FIXME: Talk about aliasing (when source and target image is the same image)      
    ///
    /// This function operates in a lossless manner if all of the following conditions are
    /// met:
    ///
    /// * The specified reader uses the same native component representation scheme as this
    ///   writer (\ref get_comp_repr()).
    ///
    /// * The specified reader uses the same native color space as this writer (\ref
    ///   get_comp_repr()).
    ///
    /// * The image attached to the specified reader does not have an alpha channel, or the
    ///   image attached to this writer does have an alpha channel (\ref
    ///   has_alpha_channel()).
    ///
    /// * Blending is disabled (it is by default).
    ///
    /// * The configured opacity is 1 (this is the default value).
    ///
    /// * The image attached to this writer uses direct color (\ref has_indirect_color()).
    ///
    /// When operating in a lossless manner, this function guarantees that the pixels passed
    /// to \ref image::WritableImage::write() of the image attached to this writer are
    /// exactly equal to those passed out of \ref image::Image::read() of the image attached
    /// to the specified reader with the proviso that if the image attached to the specified
    /// reader does not have an alpha channel, but the image attached to this writer does,
    /// then alpha components with maximum value (full opacity) will be introduced.
    ///
    /// \sa \ref put_image()
    ///
    auto put_image_a(image::Pos pos, image::Reader& reader, image::Box box) -> Writer&;

    auto put_pixel(image::Pos pos, util::Color) -> Writer&;

    template<class R> auto put_pixel(image::Pos pos, image::Pixel<R>) -> Writer&;

    using const_int8_tray_type = image::Tray<const image::int8_type>;

    /// \{
    ///
    /// \brief Write block of pixels to image.
    ///
    /// These functions are shorthands for calling \ref put_block() with \p R set to \ref
    /// image::Lum_8, \ref image::LumA_8, \ref image::RGB_8, and \ref image::RGBA_8
    /// respectively.
    ///
    auto put_block_lum(image::Pos pos, const const_int8_tray_type& tray) -> Writer&;
    auto put_block_luma(image::Pos pos, const const_int8_tray_type& tray) -> Writer&;
    auto put_block_rgb(image::Pos pos, const const_int8_tray_type& tray) -> Writer&;
    auto put_block_rgba(image::Pos pos, const const_int8_tray_type& tray) -> Writer&;
    /// \}

    /// \{
    ///
    /// \brief Write block of pixels to image with flexible pixel representation.
    ///
    /// These functions write a block of pixels to the specified position (\p pos) in the
    /// attached image. The representation of the specified pixels is as specified by \p R,
    /// which must be an instance of \ref image::PixelRepr.
    ///
    /// If `block` is an object of type `image::PixelBlock<R>`, then `put_block<R>(pos,
    /// block)` is a shorthand for `put_block<R>(pos, block.tray())`.
    ///
    /// If `tray` is an object of type `R::const_tray_type`, then `put_block<R>(pos, tray)`
    /// is a shorthand for `put_block_a<R::comp_repr>(pos, tray, R::get_color_space(),
    /// R::has_alpha)`.
    ///
    template<class R> auto put_block(image::Pos pos, const image::PixelBlock<R>& block) -> Writer&;
    template<class R> auto put_block(image::Pos pos, const typename R::const_tray_type& tray) -> Writer&;
    /// \}

    /// \brief Write block of pixels to image with maximally flexible pixel representation.
    ///
    /// This function writes a block of pixels to the specified position (\p pos), taking
    /// them from the specified tray (\p tray). The pixels are written to the area
    /// designated by `image::Box(pos, tray.size)`. The pixels are converted from the
    /// specified format (\p color_space, \p has_alpha, and \p R). The tray must provide a
    /// number of channels per pixel equal to `color_space.get_num_channels() +
    /// int(has_alpha)`.
    ///
    /// This function operates in a lossless manner if all of the following conditions are
    /// met:
    ///
    /// * The component representation scheme in use by the specified block (\p R) is the
    ///   same as the native component representation scheme of this writer (\ref
    ///   get_comp_repr()).
    ///
    /// * The color space in use by the specified block (\p color_space) is the same as the
    ///   native color space of this writer (\ref get_color_space()).
    ///
    /// * The specified block does not have an alpha channel (\p has_alpha), or the attached
    ///   image has an alpha channel (\ref has_alpha_channel()).
    ///
    /// * Blending is disabled (it is by default).
    ///
    /// * The configured opacity is 1 (this is the default value).
    ///
    /// * The attached image uses direct color (\ref has_indirect_color()).
    ///
    /// When operating in a lossless manner, this function guarantees that the pixels passed
    /// to \ref image::WritableImage::write() are exactly equal to those passed to
    /// `put_block_a()` with the proviso that if the specified block does not have an alpha
    /// channel, but the target image does, then alpha components with maximum value (full
    /// opacity) will be introduced.
    ///
    /// \sa \ref put_color_index_block()
    ///
    template<image::CompRepr R> auto put_block_a(image::Pos pos, const image::const_tray_type<R>& tray,
                                                 const image::ColorSpace& color_space, bool has_alpha) -> Writer&;

    /// \brief Perform stencil operation using alpha mask.
    ///
    /// This function performs a stencil operation at the specified position (\p pos) using
    /// the specified mask (\p tray). The targeted pixels are those that fall inside the box
    /// constructed as `image::Box(pos, tray.size)`.
    ///
    /// The alpha components in the specified tray (\p tray) are assumed to be expressed
    /// according to `int8` of \ref image::CompRepr, which means that they are 8-bit
    /// integers, but expressed linearly because they are alpha component and not color
    /// components.
    ///
    /// For each targeted pixel, a preliminary pixel is constructed as `opacity` * ((`mask`
    /// * `fg`) OVER `bg`), where `opacity` is the currently configured opacity (\ref
    /// set_opacity()), `mask` is the alpha value from the corresponding position in the
    /// specified mask (\p tray), `fg` is the currently configured foreground color (\ref
    /// set_foreground_color()), `bg` is the currently configured background color (\ref
    /// set_background_color()), `alpha` * `color` means `color` with opacity scaled by
    /// `alpha`, and OVER refers to the Porter-Duff alpha-compositing operator of that name
    /// (see \sa https://en.wikipedia.org/wiki/Alpha_compositing). Note that the foreground
    /// color and background color carry opacity information even when the target image has
    /// no alpha channel.
    ///
    /// The preliminary pixel is then applied to the target image as if by \ref put_pixel(),
    /// meaning that it will be blended with the preexisting pixel when blending is enabled,
    /// and so forth.
    ///
    auto put_block_mask(image::Pos pos, const const_int8_tray_type& tray) -> Writer&;

    /// \brief Find index of closest color in palette.
    ///
    /// This function searches the associated palette for the entry whose color is closest
    /// to the specified color (\p color), and returns the index of that entry.
    ///
    /// If `w` is a writer, then `w.reverse_palette_lookup(color)` is shorthand for
    /// `w.reverse_palette_lookup_a<R::comp_repr>(color.data(), R::get_color_space(),
    /// R::has_alpha)`.
    ///
    /// \sa \ref reverse_palette_lookup_a()
    /// \sa \ref palette_lookup()
    ///
    template<class R> auto reverse_palette_lookup(const image::Pixel<R>& color) -> std::size_t;

    /// \brief Find index of closest color in palette with color specified on any form.
    ///
    /// This function searches the associated palette for the entry whose color is closest
    /// to the specified color (\p components), and returns the index of that entry. The
    /// format of the specified color is as specified by \p R, \p color_space, and \p
    /// has_alpha.
    ///
    /// If the palette is empty (number of entries is zero) this function returns zero.
    ///
    /// It is an error to call this function when the attached image does not have a palette
    /// (\ref has_indirect_color()). If this function is called in such a case, an exception
    /// is thrown.
    ///
    /// Note that the palette may not include all the pixels in the image that acts as
    /// palette. See \ref image::TransferInfo::determine_palette_size() for details.
    ///
    /// \sa \ref reverse_palette_lookup()
    /// \sa \ref palette_lookup_a()
    ///
    template<image::CompRepr R>
    auto reverse_palette_lookup_a(const image::comp_type<R>* components, const image::ColorSpace& color_space,
                                  bool has_alpha) -> std::size_t;

    /// \brief Compute square distance between two colors.
    ///
    /// This function computes the square distance between the two specified colors (\p a and \p b).
    ///
    /// If `w` is a writer, then `w.color_sqdist(a, b)` is shorthand for
    /// `w.color_sqdist_a<R::comp_repr, S::comp_repr>(a.data(), R::get_color_space(),
    /// R::has_alpha, b.data(), S::get_color_space(), S::has_alpha)`.
    ///
    /// \sa \ref color_sqdist_a()
    ///
    template<class R, class S> auto color_sqdist(const image::Pixel<R>& a, const image::Pixel<S>& b) ->
        image::float_type;

    /// \brief Compute square distance between two colors specified on any form.
    ///
    /// This function computes the square distance between the two specified colors. The
    /// distance is computed with respect to the native color space of the reader (\ref
    /// get_color_space()) with linearly expressed component values (not gamma compressed),
    /// and is the Euclidean distance within that color space (considered as a Euclidean
    /// space).
    ///
    /// FIXME: Make note about a possible alternative comparison color space having been configured, when support for that is added                                             
    ///
    /// The format of the first color, \p components_1, is as specified by \p R, \p
    /// color_space_1, and \p has_alpha_1. The format of the second color, \p components_2,
    /// is as specified by \p S, \p color_space_2, and \p has_alpha_2.
    ///
    /// \sa \ref color_sqdist()
    ///
    template<image::CompRepr R, image::CompRepr S>
    auto color_sqdist_a(const image::comp_type<R>* components_1, const image::ColorSpace& color_space_1,
                        bool has_alpha_1, const image::comp_type<S>* components_2,
                        const image::ColorSpace& color_space_2, bool has_alpha_2) -> image::float_type;

    /// \brief Set color indexes for block of pixels.
    ///
    /// This function first verifies that all the specified color indexes (\p tray) can be
    /// represented in the attached image. They can if, and only if they are non-negative
    /// and less than two to the power of the index depth, which is 8. If verification
    /// succeeds, this function proceeds to store the indexes in the image and then returns
    /// `true`. If verification fails, this function returns `false` and leaves the image
    /// unchanged.
    ///
    /// FIXME: Update paragraph above when support for exact index bit width is added                           
    ///
    /// FIXME: Update paragraph above when support for varying index representation schemes is added.                          
    ///
    /// It is an error if the target area extends beyond the image boundary. If it does,
    /// this function throws. The size of the image is available through \ref
    /// get_image_size().
    ///
    /// It is an error to call this function when the attached image does not have a palette
    /// (\ref has_indirect_color()). If this function is called in such a case, an exception
    /// is thrown.
    ///
    /// \sa \ref put_block_a()
    /// \sa \ref try_get_color_index_block()
    ///
    template<class I> bool try_put_color_index_block(image::Pos pos, const image::Tray<I>& tray);

private:
    // When preset, each entry is a color index, and the entries are sorted by
    // util::kdtree_sort().
    std::unique_ptr<std::size_t[]> m_palette_kdtree;

    bool m_blending_enabled = false;
    image::float_type m_opacity = 1;

    // The tertiary workspace buffer (workspace buffer 3) is intended for contexts where an
    // invoked function does clobber the primary and secondary workspace buffers.
    //
    // Buffer memory must be maximally aligned (aligned for at least / std::max_align_t).
    core::Buffer<std::byte> m_workspace_buffer_3;

    // Handles blending when enabled. Caller must have already applied effect of configured
    // opacity. The alpha channel must be present in the tray. The target box must be
    // confined to the image area. The tray size must be bounded as if by
    // subdivision. Clobbers the primary and secondary workspace buffers (may clobber
    // contents, and may reallocate memory).
    void write_b(image::Pos pos, const image::Tray<const image::float_type>& tray);

    // Handles reverse palette lookup when writing to image with indirect color. The alpha
    // channel must be present in the tray. The target box must be confined to the image
    // area. The tray size must be bounded as if by subdivision. Clobbers the primary
    // workspace buffer (may clobber contents, and may reallocate memory).
    void write(image::Pos pos, const image::Tray<const image::float_type>& tray);

    void ensure_palette_kdtree();
    void instantiate_palette_kdtree();

    // Caller must have already called ensure_palette_kdtree(). Color must be specified in
    // native color space and with an alpha component included.
    //
    // FIXME: Consider allowing use of custom color space (CIELAB) for color comparison                       
    auto do_reverse_palette_lookup(const image::float_type* color) -> std::size_t;

    // The number of components in the specified buffer must not be less than `max(n, m)`,
    // where `n` is the number of channels in the origin color
    // (`color_space.get_num_channels() + int(has_alpha)`) and `m` is the number of channels
    // in the promoted native color (`m_num_channels_ext`).
    template<image::CompRepr R>
    void color_to_promoted_native(const image::comp_type<R>* components, const image::ColorSpace& color_space,
                                  bool has_alpha, image::float_type* buffer);

    auto do_reverse_palette_lookup_a(const image::float_type* color) -> std::size_t;

    auto do_color_sqdist_a(image::float_type* a, image::float_type* b) noexcept -> image::float_type;
};








// Implementation


inline Writer::Writer(image::WritableImage& image)
    : image::Reader(image) // Throws
{
}


inline auto Writer::get_image() const noexcept -> image::WritableImage&
{
    image::Image& image = const_cast<image::Image&>(Reader::get_image());
    return static_cast<image::WritableImage&>(image);
}


inline auto Writer::enable_blending() noexcept -> Writer&
{
    return set_blending_enabled(true);
}


inline auto Writer::disable_blending() noexcept -> Writer&
{
    return set_blending_enabled(false);
}


inline auto Writer::set_blending_enabled(bool val) noexcept -> Writer&
{
    m_blending_enabled = val;
    return *this;
}


inline bool Writer::get_blending_enabled() noexcept
{
    return m_blending_enabled;
}


inline auto Writer::set_opacity(image::float_type val) noexcept -> Writer&
{
    m_opacity = val;
    return *this;
}


inline auto Writer::get_opacity() noexcept -> image::float_type
{
    return m_opacity;
}


inline auto Writer::fill(ColorSlot slot) -> Writer&
{
    image::Box area = { get_image_size() };
    return fill(area, slot); // Throws
}


inline auto Writer::put_image(image::Pos pos, const image::Image& image) -> Writer&
{
    image::Reader reader(image); // Throws
    return put_image_a(pos, reader, image::Box(reader.get_image_size())); // Throws
}


inline auto Writer::put_pixel(image::Pos pos, util::Color color) -> Writer&
{
    return put_pixel(pos, image::Pixel(color)); // Throws
}


template<class R> inline auto Writer::put_pixel(image::Pos pos, image::Pixel<R> pixel) -> Writer&
{
    using repr_type = R;
    std::ptrdiff_t horz_stride = pixel.num_channels;
    std::ptrdiff_t vert_stride = horz_stride;
    image::Iter iter = { pixel.data(), horz_stride, vert_stride };
    image::Tray tray = { iter, image::Size(1) };
    return put_block<repr_type>(pos, tray); // Throws
}


inline auto Writer::put_block_lum(image::Pos pos, const const_int8_tray_type& tray) -> Writer&
{
    return put_block<image::Lum_8>(pos, tray); // Throws
}


inline auto Writer::put_block_luma(image::Pos pos, const const_int8_tray_type& tray) -> Writer&
{
    return put_block<image::LumA_8>(pos, tray); // Throws
}


inline auto Writer::put_block_rgb(image::Pos pos, const const_int8_tray_type& tray) -> Writer&
{
    return put_block<image::RGB_8>(pos, tray); // Throws
}


inline auto Writer::put_block_rgba(image::Pos pos, const const_int8_tray_type& tray) -> Writer&
{
    return put_block<image::RGBA_8>(pos, tray); // Throws
}


template<class R> auto Writer::put_block(image::Pos pos, const image::PixelBlock<R>& block) -> Writer&
{
    return put_block(pos, block.tray()); // Throws
}


template<class R> auto Writer::put_block(image::Pos pos, const typename R::const_tray_type& tray) -> Writer&
{
    return put_block_a<R::comp_repr>(pos, tray, R::get_color_space(), R::has_alpha); // Throws
}


template<image::CompRepr R>
auto Writer::put_block_a(image::Pos pos, const image::const_tray_type<R>& tray, const image::ColorSpace& color_space,
                         bool has_alpha) -> Writer&
{
    constexpr image::CompRepr repr = R;

    image::Box box = { pos, tray.size };
    image::Box boundary = { get_image_size() };
    if (ARCHON_UNLIKELY(!boundary.clip(box)))
        return *this;

    image::TransferInfo info = get_transfer_info();
    bool same_comp_repr = (repr == info.comp_repr);
    bool same_color_space = (&color_space == info.color_space);
    bool add_alpha = (!has_alpha && info.has_alpha);
    bool remove_alpha = (has_alpha && !info.has_alpha);
    bool is_float = (repr == image::CompRepr::float_);
    image::float_type opacity = get_opacity();
    bool blending = (get_blending_enabled() && (has_alpha || opacity != 1));
    bool is_indexed = has_indirect_color();
    bool lossless = (same_comp_repr && same_color_space && (!remove_alpha || is_float) && opacity == 1 && !blending &&
                     !is_indexed);
    if (ARCHON_LIKELY(lossless && !add_alpha)) {
        // Alternative 1/3: Lossless, no introduction of alpha channel
        //
        // FIXME: Explain how this works, the case where alpha channel is removed                                                                             
        get_image().write(box.pos, tray.subtray(box, pos)); // Throws
        return *this;
    }

    impl::subdivide(box, [&](const image::Box& subbox) {
        image::Tray tray_1 = tray.subtray(subbox, pos);
        int num_channels_ext = m_num_channels_ext;
        if (ARCHON_LIKELY(lossless)) {
            // Alternative 2/3: Lossless, introduction of alpha channel
            core::Buffer<std::byte>& buffer = m_workspace_buffer_1;
            using comp_type = image::comp_type<repr>;
            impl::Workspace<comp_type> workspace(buffer, num_channels_ext, subbox.size); // Throws
            image::Tray tray_2 = workspace.tray(num_channels_ext, subbox.size);
            comp_type max_alpha = image::comp_repr_max<repr>();
            for (int y = 0; y < subbox.size.height; ++y) {
                for (int x = 0; x < subbox.size.width; ++x) {
                    const comp_type* origin = tray_1(x, y);
                    comp_type* destin = tray_2(x, y);
                    std::copy(origin, origin + (num_channels_ext - 1), destin);
                    destin[num_channels_ext - 1] = max_alpha;
                }
            }
            get_image().write(subbox.pos, tray_2); // Throws
            return;
        }

        // Alternative 3/3: General
        //
        // Using tertiary workspace buffer because write_b() clobbers primary and secondary
        // workspace buffers. Note also that convert_2() clobbers the primary workspace
        // buffer.
        core::Buffer<std::byte>& buffer = m_workspace_buffer_3;
        impl::Workspace<image::float_type> workspace(buffer, num_channels_ext, subbox.size); // Throws
        image::Tray tray_2 = workspace.tray(num_channels_ext, subbox.size);
        constexpr image::CompRepr float_repr = image::CompRepr::float_;
        bool destin_has_alpha = true;
        convert_2<repr, float_repr>(tray_1, color_space, has_alpha,
                                    tray_2.iter, get_color_space(), destin_has_alpha); // Throws
        if (ARCHON_UNLIKELY(opacity != 1)) {
            std::size_t n = workspace.size();
            for (std::size_t i = 0; i < n; ++i)
                workspace[i] *= opacity; // Throws
        }
        write_b(subbox.pos, tray_2); // Throws
    }); // Throws
    return *this;
}


template<class R> inline auto Writer::reverse_palette_lookup(const image::Pixel<R>& color) -> std::size_t
{
    return reverse_palette_lookup_a<R::comp_repr>(color.data(), R::get_color_space(), R::has_alpha); // Throws
}


template<image::CompRepr R>
auto Writer::reverse_palette_lookup_a(const image::comp_type<R>* components, const image::ColorSpace& color_space,
                                      bool has_alpha) -> std::size_t
{
    int num_channels = color_space.get_num_channels() + int(has_alpha);
    std::array<image::float_type, s_default_workspace_seed_size> seed_mem;
    impl::Workspace<image::float_type> workspace(seed_mem, m_workspace_buffer_1,
                                                 std::max(num_channels, m_num_channels_ext)); // Throws
    image::float_type* buffer = workspace.data();
    color_to_promoted_native<R>(components, color_space, has_alpha, buffer); // Throws
    return do_reverse_palette_lookup_a(buffer); // Throws
}


template<class R, class S> inline auto Writer::color_sqdist(const image::Pixel<R>& a, const image::Pixel<S>& b) ->
    image::float_type
{
    return color_sqdist_a<R::comp_repr, S::comp_repr>(a.data(), R::get_color_space(), R::has_alpha, b.data(),
                                                      S::get_color_space(), S::has_alpha); // Throws
}


template<image::CompRepr R, image::CompRepr S>
auto Writer::color_sqdist_a(const image::comp_type<R>* components_1, const image::ColorSpace& color_space_1,
                            bool has_alpha_1, const image::comp_type<S>* components_2,
                            const image::ColorSpace& color_space_2, bool has_alpha_2) -> image::float_type
{
    int num_channels_1 = color_space_1.get_num_channels() + int(has_alpha_1);
    int num_channels_2 = color_space_2.get_num_channels() + int(has_alpha_2);
    int n_1 = std::max(num_channels_1, m_num_channels_ext);
    int n_2 = std::max(num_channels_2, m_num_channels_ext);
    int n = n_1;
    core::int_add(n, n_2); // Throws
    std::array<image::float_type, 2 * s_default_workspace_seed_size> seed_mem;
    impl::Workspace<image::float_type> workspace(seed_mem, m_workspace_buffer_1, n); // Throws
    image::float_type* buffer_1 = workspace.data();
    image::float_type* buffer_2 = buffer_1 + n_1;
    color_to_promoted_native<R>(components_1, color_space_1, has_alpha_1, buffer_1); // Throws
    color_to_promoted_native<S>(components_2, color_space_2, has_alpha_2, buffer_2); // Throws
    return do_color_sqdist_a(buffer_1, buffer_2);
}


template<class I> bool Writer::try_put_color_index_block(image::Pos pos, const image::Tray<I>& tray)
{
    if (ARCHON_UNLIKELY(!m_transfer_info.palette))
        throw std::runtime_error("Image has no palette");

    constexpr image::CompRepr index_repr = image::color_index_repr; // FIXME: Should be made variable, and be provided through TransferInfo                                  
    using index_comp_type = image::comp_type<index_repr>;
    core::Buffer<std::byte>& buffer = m_workspace_buffer_1;
    impl::Workspace<index_comp_type> workspace(buffer);

    // Verify that all indexes are representable before clobbering the image.
    using unpacked_index_comp_type = image::unpacked_comp_type<index_repr>;
    unpacked_index_comp_type max_index = image::comp_repr_unpacked_max<index_repr>(); // FIXME: Should instead be determined by TransferInfo::index_depth                 
    for (int y = 0; y < tray.size.height; ++y) {
        for (int x = 0; x < tray.size.width; ++x) {
            I index = *tray(x, y);
            if (ARCHON_LIKELY(index >= 0 && core::to_unsigned(index) <= core::to_unsigned(max_index)))
                continue;
            return false;
        }
    }

    image::Box box = { pos, tray.size };
    impl::subdivide(box, [&](const image::Box& subbox) {
        image::Tray tray_2 = tray.subtray(subbox, pos);
        int num_index_channels = 1;
        workspace.reset(num_index_channels, subbox.size); // Throws
        image::Tray tray_3 = workspace.tray(num_index_channels, subbox.size);
        for (int y = 0; y < tray_2.size.height; ++y) {
            for (int x = 0; x < tray_2.size.width; ++x) {
                I index = *tray_2(x, y);
                *tray_3(x, y) = image::comp_repr_pack<index_repr>(index);
            }
        }
        get_image().write(subbox.pos, tray_3); // Throws
    }); // Throws

    return true;
}


inline void Writer::ensure_palette_kdtree()
{
    if (ARCHON_LIKELY(m_palette_kdtree))
        return;
    instantiate_palette_kdtree(); // Throws
}


inline auto Writer::do_reverse_palette_lookup(const image::float_type* color) -> std::size_t
{
    ARCHON_ASSERT(m_palette_kdtree); // Must have called ensure_palette_kdtree()
    const image::float_type* float_components = get_palette_cache_f();
    int num_channels_ext = m_num_channels_ext;
    auto get_comp = [&](std::size_t color_index, int comp_index) noexcept {
        std::size_t i = std::size_t(color_index * num_channels_ext + comp_index);
        return float_components[i];
    };
    int k = get_num_channels();
    auto begin = m_palette_kdtree.get();
    auto end = begin + get_palette_size();
    std::optional<image::float_type> max_dist = {}; // No max dist
    std::size_t index = 0;
    image::float_type dist = 0;
    // If no color is found, because the palette is empty, we will use an index of zero,
    // which is alright, because indexes that are out of range are allowed, and will be
    // resolved to the background color.
    util::kdtree_find(k, begin, end, std::move(get_comp), color, max_dist, index, dist); // Throws
    return index;
}


template<image::CompRepr R>
void Writer::color_to_promoted_native(const image::comp_type<R>* components, const image::ColorSpace& color_space,
                                      bool has_alpha, image::float_type* buffer)
{
    constexpr image::CompRepr repr_1 = R;
    constexpr image::CompRepr repr_2 = image::CompRepr::float_;
    int num_channels = color_space.get_num_channels() + int(has_alpha);
    image::comp_repr_convert<repr_1, repr_2>(components, buffer, num_channels, has_alpha);
    image::float_type alpha = (has_alpha ? buffer[num_channels - 1] : image::float_type(1));
    const image::ColorSpace& destin_color_space = get_color_space();
    const image::ColorSpaceConverter* custom_converter = find_color_space_converter(color_space, destin_color_space);
    image::color_space_convert(buffer, alpha, color_space, destin_color_space, custom_converter); // Throws
}


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_WRITER_HPP
