/*
 * This file is part of the Archon library framework.
 *
 * Copyright (C) 2012  Kristian Spangsege <kristian.spangsege@gmail.com>
 *
 * The Archon library framework is free software: You can redistribute
 * it and/or modify it under the terms of the GNU Lesser General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * The Archon library framework is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the Archon library framework.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/// \file
///
/// \author Kristian Spangsege

#ifndef ARCHON_FONT_FACE_HPP
#define ARCHON_FONT_FACE_HPP

#include <string>

#include <archon/math/vector.hpp>
#include <archon/image/writer.hpp>


namespace archon {
namespace font {

/// This class represents a particular font face, and can be used to render
/// individual glyphs from it.
///
/// If this font face provides fixed sizes (see get_num_fixed_sizes()), then the
/// initial rendering size is the fixed size that is closest in area to 12 by 12
/// pixels. Otherwise this font face is scalable, and the initial rendering size
/// is set to exactly 12 by 12 pixels.
///
/// New font face instances are normally aquired by calling either
/// FontLoader::load_face(), or FontList::load_face().
///
/// A \c FontFace instance is always associated with a \c FontLoader
/// instance. This loader is the one that was used to load the font face, or in
/// case the font face was loaded from a \c FontList instance, it is the loader
/// that was associated with the \c FontList instance.
///
/// The methods of this class are not thread-safe. It is safe, however, for two
/// threads to use this class simultaneously, as long as they access different
/// instances, and those two instances are associated with different \c
/// FontLoader instance. That is, you need one loader instance per thread.
class FontFace {
public:
    /// Get the family name of this font face.
    ///
    /// \return The family name of this font face, e.g. 'Times New Roman'.
    ///
    /// \note The family name can be empty for some fonts.
    virtual std::string get_family_name() const = 0;

    /// Find out whether this is a bold font face.
    ///
    /// \return True if this foint face is bold.
    virtual bool is_bold() const = 0;

    /// Find out whether this is an italic/oblique font face.
    ///
    /// \return True if this foint face is italic.
    virtual bool is_italic() const = 0;

    /// Find out whether this is a monospaced font face. That is, wheter or not
    /// all glyphs have the same width.
    virtual bool is_monospace() const = 0;

    /// Find out whether this is a scalable font face. If it is, any rendering
    /// size may be chosen. Otherwise, only a finite set of fixed rendering
    /// sizes are valid.
    ///
    /// \sa get_num_fixed_sizes
    virtual bool is_scalable() const = 0;

/*
    /// Enable 'forced grid fitting' mode.
    ///
    /// When this mode is enabled (the default), glyphs are modified slightly as
    /// they are loaded, to optimize their appearance with respect to the
    /// selected pixel grid size.
    ///
    /// A side effect of this, is that the width and height of glyphs are
    /// guaranteed to always be integers, and the same is true for most other
    /// font and glyph metrics, in particular those returned by
    /// get_baseline_spacing(), get_baseline_offset(), get_kerning(), and
    /// get_glyph_advance().
    ///
    /// Finally when grid fitting is enabled, one can only translate a glyph by
    /// an integer number of pixels. That is, both translation components are
    /// automatically rounded to the nearest integer.
    ///
    /// Even when grid fitting mode is not enabled, the effect of grid fitting
    /// may still happen. Enabling the mode, simply guarantees that it happens
    /// every time.
    virtual void enable_grid_fitting(bool enabled) = 0;

    /// There are eight possible choices for layout direction. Four of them are
    /// horizontal, and four of them are vertical.
    ///
    /// The default horizontal layout has characters on a line running from left
    /// to right, and lines running from top to bottom. Both of threse
    /// directions can be reversed independently giving rise to the four
    /// different horizontal configurations. The situaltion for vertical layouts
    /// is completely analogous.
    ///
    /// This setting has an immediate effect on the result of
    /// get_baseline_spacing(), get_kerning(), get_glyph_advance(),
    /// get_glyph_pixel_box(), and render_pixels_to().
    virtual void set_layout_direction(bool vertical,
                                      bool right_to_left = 0, bool bottom_to_top = 0) = 0;
*/

    /// Get the number of fixed rendering sizes that this font face offers.
    ///
    /// Scalable fonts may or may not provide fixed rendering sizes. If they do,
    /// it should be understood as the preferrable sizes that lead to the result
    /// of highest quality.
    ///
    /// For fonts that are not scalable, this method is required to return at
    /// least 1.
    ///
    /// \return The number of fixed rendering sizes.
    virtual int get_num_fixed_sizes() const = 0;

    /// Get the specified fixed rendering size. The first component is the
    /// horizontal number of pixels in the EM-square, and the second component
    /// is the same in the vertical direction.
    ///
    /// \param fixed_size_index The index of the fixed rendering size to query.
    ///
    /// \return The horiziontal and vertical resolution of the EM-square in
    /// number of pixels. Note that the components need not be integers.
    virtual math::Vec2 get_fixed_size(int fixed_size_index) const = 0;

    /// Select the specified fixed nominal glyph rendering size.
    ///
    /// \param fixed_size_index The index of the fixed rendering size to use.
    ///
    /// The rendering size, whether specified with this method or with
    /// set_scaled_size(), affects only the glyph loading process (see
    /// load_glyph()). Any previously loaded glyph is not affected.
    ///
    /// \note There is no guarantee that the rendered size of any particular
    /// glyph is limited to this size. It is merely a nominal size.
    ///
    /// \sa set_scaled_size()
    virtual void set_fixed_size(int fixed_size_index) = 0;

    /// Set the nominal glyph rendering size for scalable fonts. The specified
    /// width and height are the number of pixels along the corresponding sides
    /// of the EM-square.
    ///
    /// This method may only be called for scalable fonts. An exception will be
    /// thrown if it is called for a font that is not scalable.
    ///
    /// \param width, height The width and height in pixels of the
    /// EM-square. Note that these values do not need to be integers. They must
    /// both be strictly positive however.
    ///
    /// \note There is no guarantee that the rendered size of any particular
    /// glyph is limited to this size.
    ///
    /// \sa set_fixed_size()
    virtual void set_scaled_size(double width, double height) = 0;

    /// Set the rendering size as close to the specified size as possible.
    ///
    /// If this face is not scalable, then the fixed size, that is closest to
    /// the specified size, is chosen. The distance between two sizes is the
    /// distance between the corresponding points in the plane.
    ///
    /// If this face is scalable, then the specified size is first converted to
    /// the closes possible size allowed by the implementation. Then, if the
    /// result matches a fixed size exactly, that fixed size is used, otherwise
    /// the converted size is used as a scaled size.
    ///
    /// Note that in some cases it is possible to set a specific size either as
    /// a fixed size or as a scaled size. In such cases the fixed size may be
    /// assumed to produce a better result, and is therefore always preferred.
    ///
    /// \param width, height The desired width and height in pixels of the
    /// EM-square. Note that these values do not need to be integers. They must
    /// both be strictly positive however.
    virtual void set_approx_size(double width, double height) = 0;

    /// Get the horizontal component of the currently selected rendering
    /// size. The rendering size is the resolution of the EM-square.
    ///
    /// \return The effective rendering width in number of pixels (may be
    /// fractional).
    virtual double get_width() const = 0;

    /// Get the vertical component of the currently selected rendering size. The
    /// rendering size is the resolution of the EM-square.
    ///
    /// \return The effective rendering height in number of pixels (may be
    /// fractional).
    virtual double get_height() const = 0;

    /// Get the distance in pixels between two baselines.
    ///
    /// The returned value depends on the loaded font face, on the selected
    /// rendering size, on the selected layout direction, and on whether grid
    /// fitting is enabled.
    ///
    /// For horizontal layouts (the default), this is the distance between
    /// adjacent horizontal baselines. If the layout is also left-to-right (the
    /// default), then the returned value will be positive, otherwise it will be
    /// negative.
    ///
    /// For vertical layouts, it is instead the distance between adjacent
    /// vertical baselines. If the layout is top-to-bottom (the default), then
    /// the returned value will be negative, otherwise it will be positive.
    ///
    /// If grid fitting mode is enabled, the returned value is always an
    /// integer, otherwise the value may be fractional.
    virtual double get_baseline_spacing(bool vertical = false,
                                        bool grid_fitting = true) const = 0;

    /// Get the displacement of the baseline relative to the bottom of the line
    /// for a horizontal valyout. If the layout is vertical, it is the
    /// displacement relative to the left side of the line. The value is
    /// normally positive, meaning that the baseline lies within the line
    /// box. If the basline happens to lie outside, then the value will be
    /// negative. It is measured in number of pixels.
    ///
    /// If grid fitting mode is enabled, the returned value is always an
    /// integer, otherwise the value may be fractional.
    virtual double get_baseline_offset(bool vertical = false,
                                       bool grid_fitting = true) const = 0;

    /// Get the number of glyphs provided by this font. Any one of them can be
    /// loaded using \c load_glyph(i) where \c i is a number in the range \c
    /// [0;n-1] where \c n is the value returned by this function.
    virtual int get_num_glyphs() const = 0;

    /// Find the glyph for the specified Unicode character.
    ///
    /// A return value of zero always refer to the replacement glyph, and
    /// indicates that no glyph was available for the specified character in
    /// this font face.
    ///
    /// \param c The Unicode code position of the character to find (UTF-32).
    ///
    /// \return The index of the glyph within this font face.
    virtual int find_glyph(wchar_t c) const = 0;

    /// Get the appropriate kerning adjustment of the distance between the two
    /// specified glyphs when they appear juxtaposed.
    ///
    /// In a horizontal layout, it is assumed that <tt>glyph1</tt> is to the
    /// left of <tt>glyph2</tt>, while in a vertical layout, it is assumed that
    /// <tt>glyph1</tt> is below <tt>glyph2</tt>.
    ///
    /// A positive result means that the two glyphs must be place further apart,
    /// while a negative result means that they must be placed closer together.
    ///
    /// The adjustment is expressed in number of pixels, and may be a fractional
    /// number. However, if \c grid_fitting is true, then it is always an
    /// integer.
    ///
    /// It is guaranteed that if any of the specified glyph indices are zero
    /// (the replacement glyph) then the kerning adjustment is zero.
    ///
    /// \param glyph1_index The index of the left or lower glyph.
    ///
    /// \param glyph2_index The index of the right or upper glyph.
    ///
    /// \param vertical Pass true when the layout is vertical, as opposed to
    /// horizontal.
    ///
    /// \param grid_fitting Pass false to disable grid fitting. When grid
    /// fitting is enabled, the result is guaranteed to always be an integer.
    virtual double get_kerning(int glyph1_index, int glyph2_index,
                               bool vertical = false, bool grid_fitting = true) const = 0;

    /// Load the specified glyph onto the design tablet. After this you can
    /// inspect and modify it, and finally you can render it to produce a block
    /// of pixels.
    ///
    /// As a part of the loading process, a glyph is first scaled according to
    /// the previously specified rendering size (see set_scaled_size()), then,
    /// if requested, the scaled glyph is grid fittet, meaning that it is
    /// modified slightly to improve the appearence of the pixelized image.
    ///
    /// The design tablet has a coordinate system and an origin, and each newly
    /// loaded glyph is placed on the tablet such that the lower left corner of
    /// its axis-aligned bounding box is coincident with the origin of the
    /// tablet. The unit of measurement on both coordinate axes is 'pixels'.
    ///
    /// The initially loaded glyph is the replacement glyph whose index is zero,
    /// and it is loaded with grid fitting enabled.
    ///
    /// A side effect of grid fitting is that all glyph metrics will attain
    /// integer values. This applies to the glyph advance, and the glyph bearing
    /// components.
    ///
    /// \param glyph_index The index of the glyph to load. Use \c find_glyph to
    /// obtain indices of other glyphs.
    ///
    /// \param grid_fitting Pass false to disable grid fitting.
    virtual void load_glyph(int glyph_index, bool grid_fitting = true) = 0;

    /// Get the cursor advance distance for the currently loaded glyph. The
    /// distance is measured in number of pixels, but may be fractional.
    ///
    /// When thinking about cursor positions as occuring between glyphs, the
    /// cursor advance distance is the distance from the cursor position
    /// immadiately before the glyph to the cursor position immediately after
    /// it. For a horizontal layout, it is the distance along an imaginary
    /// horizontal baseline. Respectively it is the distance along a vertical
    /// baseline when the layout is vertical.
    ///
    /// The returned value will be an integer if grid fitting was requested when
    /// the glyph was loaded.
    ///
    /// The returned value always reflect the glyph as it was immediately after
    /// it was loaded. That is, it is not affected by subsequent modifications
    /// of the glyph on the design tablet.
    virtual double get_glyph_advance(bool vertical = false) const = 0;

    /// Get the position on the design tablet of the glyph bearing point
    /// pertaining to the specified layout direction.
    ///
    /// The glyph bearing is a point on the baseline and represents the cursor
    /// position that preceeeds the glyph with respect to the desired layout
    /// direction.
    ///
    /// For a horizontal layout, the bearing point is normally located somewhere
    /// to the left of the glyph, and must always be thought of as the cursor
    /// position immediately to the left of the glyph. Likewise, for a vertical
    /// layout, the bearing point is normally located below the glyph, and must
    /// be thought of as the cursor position immediately below the glyph.
    ///
    /// If, for examaple, you need the bearing point for a right-to-left layout,
    /// add the glyph advance to the first component of the result of this
    /// method.
    ///
    /// Both components of the returned vector will be integers if grid fitting
    /// was requested when the glyph was loaded. Otherwise they may have
    /// fractional values.
    ///
    /// The returned vector always reflect the glyph as it was immediately after
    /// it was loaded. That is, it is not affected by subsequent modifications
    /// of the glyph on the design tablet.
    virtual math::Vec2 get_glyph_bearing(bool vertical = false) const = 0;

    /// Get the size of the axis-aligned bounding box of the glyph currently on
    /// the design tablet.
    ///
    /// Both components of the returned vector will be integers if grid fitting
    /// was requested when the glyph was loaded. Otherwise they may have
    /// fractional values.
    virtual math::Vec2 get_glyph_size() const = 0;

    /// Translate the currently loaded glyph on the design tablet by the
    /// specified amount.
    ///
    /// Tecnically it is not necessary to translate a glyph unless at least one
    /// component has a non-integer value. This is because the resulting block
    /// of pixels will be essentially the same for any integer translation, and
    /// the effect of the translation could as weel be achieved by modifying the
    /// target origin (see set_target_origin()). However, if you are rendeing a
    /// string of glyphs, it is probably easiest to set a fixed target origin,
    /// and then translate each glyph by the sum of the cursor position and the
    /// negative of the relevant glyph bearing point position.
    ///
    /// It is <em>not</em> recommended to translate a grid fitted glyph by a
    /// fractional number of pixels. This is expected to produce a particularly
    /// poor result.
    ///
    /// A positive first component moves the glyph towards the right, and a
    /// positive second component moves the glyph upwards.
    virtual void translate_glyph(math::Vec2 v) = 0;

    /// Determine the edges of the pixel block that will be generated when the
    /// glyph is rendered.
    ///
    /// Superimposed on the design tablet is a pixel grid. Each pixel in the
    /// grid must be thought of as a small rectangle. One of these rectangles
    /// has its lower left corner coincident with the origin of the design
    /// tablet, and its upper right corner at (1,1).
    ///
    /// The returned point (<tt>left</tt>, <tt>bottom</tt>) is the coordinates
    /// of the lower left corner of the lower left pixel of the generated block,
    /// while (<tt>right</tt>, <tt>top</tt>) is the upper right corner of the
    /// upper right pixel of the generated block. Note that this implies that
    /// the horizontal number of generated pixels is <tt>right</tt> -
    /// <tt>left</tt>, and similarly in ther vertical direction.
    ///
    /// When the target origin is set to (0,0) the returned block coincides with
    /// the region of the target image that will be affected during a call to
    /// render_pixels_to().
    ///
    /// It shall be guaranteed that when the target origin is set to (0,0),
    /// render_pixels_to() shall not affect nor access any pixel outside the box
    /// returned by this method.
    ///
    /// The following indicates the correspondance between the box returned by
    /// this method and the value returned by get_glyph_size() assuming the
    /// glyph is translated only by a vector with integer components after being
    /// loaded:
    ///
    ///     size = get_glyph_size()
    ///
    ///     right - left  =  ceil(size.x)
    ///     top - bottom  =  ceil(size.y)
    ///
    virtual void get_glyph_pixel_box(int &left, int &right,
                                     int &bottom, int &top) const = 0;

    /// Set the position of the design tablet origin within the target
    /// image. The initial position is (0,0) which corresponds to the lower left
    /// corner of the image.
    virtual void set_target_origin(int x, int y) = 0;

    /// Render the glyph as a block of pixels and merge the pixels into the
    /// specified image.
    ///
    /// Data is always written in the form of luminance components, and without
    /// any alpha information. It is expected that your image writer is
    /// previously configured for color mapping, such that the selected
    /// foreground color of the image writer determines the color of the
    /// rendered glyph. Likewise it will be the selected background color that
    /// determines the contrast/background color.
    ///
    /// Because glyphs can easily end up overlaping each other, it is
    /// recommended that blending is also enabled in the image writer, and that
    /// the backgound color is made to be transparet. This way the rendered
    /// glyph will be blended nicely into the original contents of the
    /// image. For optimum results, the background color should be the same
    /// color as the foreground only fully transparent.
    ///
    /// The final position of the glyph in the image is determined by matching
    /// the design tablet origin with the target origin set by
    /// set_target_origin(). Thus, the final position of the glyph in the image
    /// is influenced both by the position of the glyph on the design tablet
    /// relative to the design tablet origin, and by the currently selected
    /// target origin.
    ///
    /// \param img A writable interface of the image into which the glyph will
    /// be merged.
    ///
    /// \todo FIXME: It might be attractive to offer alternative blending
    /// modes/functions. One could simply be channel = max(orig, new). Then the
    /// original 'max(orig,new) intencity stuff' can easily be reenabled.
    virtual void render_pixels_to(image::ImageWriter &img) const = 0;

    virtual ~FontFace() {}
};

} // namespace font
} // namespace archon

#endif // ARCHON_FONT_FACE_HPP
