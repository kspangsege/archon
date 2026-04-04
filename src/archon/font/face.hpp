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

#ifndef ARCHON_X_FONT_X_FACE_HPP
#define ARCHON_X_FONT_X_FACE_HPP

/// \file


#include <cstddef>
#include <string_view>

#include <archon/core/buffer.hpp>
#include <archon/math/vector.hpp>
#include <archon/math/matrix.hpp>
#include <archon/image/geom.hpp>
#include <archon/image/tray.hpp>
#include <archon/image/comp_types.hpp>
#include <archon/image/writer.hpp>
#include <archon/font/size.hpp>
#include <archon/font/code_point.hpp>


namespace archon::font {


/// \brief Loaded font face.
///
/// An instance of this class represents a particular loaded font face. It can be used to
/// examine and render the glyphs of the font face. A font face is a specific stylistic
/// variant of a font family.
///
/// If this font face provides fixed sizes (see \ref get_num_fixed_sizes()), then the
/// initial rendering size is the fixed size that is closest in area to 16 by 16
/// pixels. Otherwise, this font face is scalable, and the initial rendering size is set to
/// exactly 16 by 16 pixels.
///
/// New font face objects are created by a font loader (\ref font::loader). They are created
/// as a result of calling \ref font::loader::load_face() or \ref
/// font::loader::load_default_face(). A font face object is tied to that particular loader
/// object. A font face object must never outlive the loader object that it is tied to.
///
/// The font loader aggregate (see \ref font::loader) that a font face object belongs to
/// must never be accessed by more than one thread at a time. On the other hand, two threads
/// can safely work with separate font loader aggregates.
///
/// \sa \ref font::loader::load_face(), \ref font::loader::load_default_face()
///
///
/// ### Layout process
///
/// In its most basic form, the general layout process for horizontal sequential text might
/// look as follows:
///
///   * Pick a font rendering size (\ref set_approx_size()).
///
///   * Pick an initial cursor position in design space (see below).
///
///   * For each character:
///
///       * Look up the glyph for the character using \ref find_glyph().
///
///       * Load the glyph onto the design tablet (see below) using \ref try_load_glyph().
///
///       * Set the glyph translation to the current cursor position minus the position of
///         the glyph's bearing point. The translation is set using \ref set_translat() and
///         the bearing point position is retrieved using \ref get_glyph_bearing().
///
///       * Render the loaded glyph to the target image using \ref render_glyph_mask().
///
///       * Update the cursor position by adding the glyph advance value to the X
///         coordinate. The glyph advance value is returned by \ref get_glyph_advance().
///
/// FIXME: Explain the nonstandard term "bearing point".
///
/// Note that the font rendering size can be picked in different ways. See \ref
/// set_approx_size(), \ref set_scaled_size(), and \ref set_fixed_size().
///
/// One can render to targets other than an image by using \ref render_glyph_mask_a()
/// instead of \ref render_glyph_mask(). See also "Target medium" below.
///
///
/// ### Design space
///
/// In general, a layout process, such as the one shown above, occurs in design space. The
/// *design space* is a continuous infinite plane that is superimposed on the discrete pixel
/// space of the target medium for the rendered glyphs. The design space has an implied
/// coordinate system where the unit along both axes is pixels. The X-axis points to the
/// right and the Y-axis points upwards. So, (0.2, 0.7) means a fifth of a pixel to the
/// right and seven tenths of a pixel upwards.
///
/// Font face metrics are reported using design space coordinates (\ref
/// get_ascender(), \ref get_descender(), \ref get_baseline_spacing()).
///
/// When a glyph is loaded onto the design tablet (see below), its geometry and metrics are
/// described using design space coordinates (\ref get_glyph_pos(), \ref get_glyph_size(),
/// \ref get_glyph_advance(), get_glyph_bearing()).
///
/// The exact position and size of a loaded glyph in design space depends on the font face
/// and can be examined using \ref get_glyph_pos(), \ref get_glyph_size(). Normally, what
/// matters during a layout process is the glyph's bearing point, which corresponds to the
/// cursor position that precedes the glyph. To get the glyph's bearing point, call \ref
/// get_glyph bearing(). In a horizontal layout, to place the glyph such that its bearing
/// point coincides with a particular desired cursor position, `pos`, in design space, set
/// the glyph translation to `pos - get_glyph_bearing(false)`. For vertical layout, use
/// `pos - get_glyph_bearing(true)`. The glyph translation is set using \ref set_translat().
///
/// The connection between positions in design space and positions in the pixel space of the
/// target medium is controlled by the rendering target point (see "Target medium" below).
///
///
/// ### Design tablet
///
/// Within each font face object, one particular glyph at a time can be loaded onto the
/// *design tablet*. When a glyph is on the design tablet, it can be examined, and it can be
/// rendered to an image or to another type of target medium. To get the metrics of the
/// loaded glyph, call \ref get_glyph_advance() and \ref get_glyph_bearing(). To render the
/// glyph to an image, call \ref render_glyph_mask() or \ref render_glyph_rgba().
///
/// A glyph is loaded onto the design tablet using \ref try_load_glyph(). This loading
/// process also scales the glyph to the selected font size (\ref set_approx_size()) and
/// applies grid fitting / hinting when requested. The bounding box of the resulting glyph
/// description can be obtained by calling \ref get_glyph_pos() and \ref get_glyph_size().
///
/// After a glyph is loaded onto the design tablet, it stays there until another glyph is
/// loaded. Other than being replaced, glyphs on the design tablet, including the glyph's
/// metrics, cannot be mutated in any way. On the other hand, a transient copy of the glyph
/// can be transformed and translated as part of the rendering process. See \ref
/// set_transformation() and \ref set_translat(). The original glyph on the design tablet
/// remains unchanged.
///
/// Changing the rendering size does not affect the glyph on the design tablet. This applies
/// to all ways in which the rendering size can be changed (\ref set_fixed_size(), \ref
/// set_scaled_size(), \ref set_approx_size()).
///
///
/// ### Target medium
///
/// The *target medium* is the pixel medium into, or onto which glyphs are rendered. This
/// can be an image, but it can also be other things. In any case, the target medium is
/// assumed to be associated with a coordinate system. That coordinate system has integer
/// pixel coordinates and a Y-axis that points downwards. This is in contrast to the design
/// space coordinate system which has continuous coordinates and a Y-axis that points
/// upwards. The X-axis points to the right in both cases.
///
/// The connection between positions in design space and positions in the target medium is
/// established through the *rendering target point*, which is a point in, or on the target
/// medium. The position of the design space coordinate system is always such that its
/// origin coincides with the rendering target point. The position of the rendering target
/// point is specified with respect to the target medium coordinate system using \ref
/// set_target_pos(). It is (0, 0) initially. When the target medium is an image and the
/// function used to render the glyph is \ref render_glyph_mask() or \ref
/// render_glyph_rgba(), then the origin of the target medium coordinate system and,
/// therefore, the initial position of the rendering target point is the upper-left corner
/// of the image.
///
/// The full glyph rendering process involves a transformation (e.g. rotation) and
/// positioning step followed by a rasterization step. Rasterization depends on the exact
/// final geometry of the glyph, i.e., geometry specified with sub-pixel precision. First, a
/// copy is made of the glyph on the design tablet. That copy is then transformed using the
/// configured transformation (\ref set_transformation()). This could, for example, be a
/// rotation around the design space origin. The transformed glyph is then translated by the
/// configured translation (\ref set_translat()). Finally, the transformed and translated
/// glyph is rasterized.
///
/// The position of the rendered glyph in, or on the target medium can be determined by
/// taking its position in design space, say the position of its bearing point (\ref
/// get_glyph_bearing()), then apply transformation and translation to that position vector,
/// and finally map it to the coordinate system of the target medium by negating the Y
/// coordinate and adding the result to the the configured position of the rending target
/// point. Note that this involves a shift to integer coordinate space. Call \ref
/// get_target_glyph_box() to get the correct pixel-aligned bounding box of the rasterized
/// glyph with respect to the target medium coordinate system.
///
///
/// ### Color
///
/// A particular font face may or may not provide support for colored glyphs (\ref
/// has_color()). When it does, a particular glyph that is loaded onto the design tablet
/// will have capacity for color (\ref glyph_may_be_colored()) if the font face provides
/// color support for that glyph and color loading is turned on for the font face (\ref
/// set_color_loading_enabled()).
///
/// When a loaded glyph has capacity for color, and the glyph utilizes that capacity, and
/// the glyph is rendered as a block of RGBA pixels, the rendered image of the glyph will be
/// multi-colored. If the loaded glyph does not have capacity for color, the rendered glyph
/// will be black.
///
class face {
public:
    using char_type = font::code_point::char_type;
    using float_type = font::size::comp_type;
    using vector_type = math::Vector<2, float_type>;
    using matrix_type = math::Matrix<2, 2, float_type>;

    /// \brief Get font family name.
    ///
    /// The font family that this font face belongs to, e.g. "Times New Roman". If the font
    /// face does not specify a family name, this function returns the empty string.
    ///
    /// The application must not assume that the referenced memory outlives the font face
    /// object.
    ///
    virtual auto get_family_name() -> std::string_view = 0;

    /// \brief Get font style name.
    ///
    /// The style name of the font face, e.g. "Regular". If the font face does not specify a
    /// style name, this function returns the empty string.
    ///
    /// The application must not assume that the referenced memory outlives the font face
    /// object.
    ///
    virtual auto get_style_name() -> std::string_view = 0;

    /// \brief Whether font face is bold.
    ///
    /// This function returns `true` if, and only if this font face is bold. A bold font
    /// face is one whose glyphs are darker and heavier than normal.
    ///
    virtual bool is_bold() noexcept = 0;

    /// \brief Whether font face is italic.
    ///
    /// This function returns `true` if, and only if this font face is italic/oblique. An
    /// italic, or oblique font face is normally one whose glyphs are slanted to the right.
    ///
    virtual bool is_italic() noexcept = 0;

    /// \brief Whether font face is monospaced.
    ///
    /// This function returns `true` if, and only if this font face is monospaced. In a
    /// monospaced font face, all glyphs have the same size, meaning that the distance
    /// between the prior and posterior cursor positions is the same for all glyphs.
    ///
    virtual bool is_monospace() noexcept = 0;

    /// \brief Whether font face is scalable.
    ///
    /// This function returns `true` if, and only if this font face is scalable. If it is,
    /// any rendering size may be chosen. Otherwise, only a finite set of fixed rendering
    /// sizes are available.
    ///
    /// \sa \ref get_num_fixed_sizes()
    ///
    virtual bool is_scalable() noexcept = 0;

    /// \brief Whether font face has color support.
    ///
    /// If this function returns `true`, the font face has color support. If it returns
    /// `false`, the font face does not have color support.
    ///
    /// When a font face has *color support*, some of its glyphs may be loadable with
    /// capacity for color (\ref glyph_may_be_colored()) so long as color loading has been
    /// turned on (\ref set_color_loading_enabled()). When a font face does not have color
    /// support, no glyphs will be loaded with capacity for color.
    ///
    /// \sa \ref glyph_may_be_colored()
    /// \sa \ref set_color_loading_enabled()
    ///
    virtual bool has_color() noexcept = 0;

    /// \brief Physical resolution of target pixel medium.
    ///
    /// Sets the physical resolution (in pixels per inch) of the target medium (e.g., a
    /// computer monitor or display) that the font renderer should assume. The horizontal
    /// resolution can differ from the vertical resolution.
    ///
    /// The default resolution is 72 pixels per inch horizontally and vertically.
    ///
    /// While resolution often has only subtle effects on standard font rendering, modern
    /// scalable font formats use it to determine the true physical size of the text. To
    /// achieve typographic correctness (such as optical sizing, where the exact shape of a
    /// glyph adapts to its physical size), the application must provide an accurate
    /// physical resolution.
    ///
    /// If the target pixel medium is unknown or unknowable, leaving this at the default
    /// value of 72 pixels per inch is perfectly safe.
    ///
    /// When a scaled size is requested (\ref set_scaled_size(), \ref set_approx_size()),
    /// the currently configured resolution is taken into account. The resolution has no
    /// effect when a fixed size is selected (\ref set_fixed_size()). A change in resolution
    /// has no effect until the next time a scaled size i requested.
    ///
    /// \param resol The horizontal and vertical resolution in pixels per inch.
    ///
    virtual void set_resolution(font::size resol) noexcept = 0;

    /// \brief Number of fixed rendering sizes offered.
    ///
    /// This function returns the number of fixed rendering sizes (bitmap strikes) offered
    /// in this font face.
    ///
    /// Scalable fonts may or may not provide fixed rendering sizes. If they do, it should
    /// be understood as the preferable sizes that lead to the result of highest quality.
    ///
    /// For fonts that are not scalable (\ref is_scalable()), this function is guaranteed to
    /// return at least 1.
    ///
    virtual int get_num_fixed_sizes() noexcept = 0;

    /// \brief Get particular fixed rendering size.
    ///
    /// This function returns the specified fixed rendering size. The fixed rendering size
    /// is identified by its index in the list of offered fixed rendering sizes. The number
    /// of entries in this list is returned by \ref get_num_fixed_sizes().
    ///
    /// \sa \ref set_fixed_size()
    ///
    virtual auto get_fixed_size(int fixed_size_index) -> font::size = 0;

    /// \brief Use specified fixed rendering size.
    ///
    /// This function selects the specified fixed rendering size as the current rendering
    /// size for this font face. The fixed rendering size is identified by its index in the
    /// list of offered fixed rendering sizes. The number of entries in this list is
    /// returned by \ref get_num_fixed_sizes().
    ///
    /// \note There is no guarantee that the rendered size of any particular glyph is
    /// limited to this size. The selected rendering size is merely a nominal size.
    ///
    /// \sa \ref get_fixed_size()
    /// \sa \ref set_scaled_size(), \ref set_approx_size()
    ///
    virtual void set_fixed_size(int fixed_size_index) = 0;

    /// \brief Set rendering size for scalable font.
    ///
    /// If the font face is scalable (\ref is_scalable()), this function sets the current
    /// rendering size as specified. If the font face is not scalable, it is an error to
    /// call this function and an exception will be thrown.
    ///
    /// FIXME: Explain that specified size will be silently clamped to range allowed by
    /// implementation.
    ///
    /// \note There is no guarantee that the rendered size of any particular glyph is
    /// limited to this size. The selected rendering size is merely a nominal size.
    ///
    /// When needed, the corresponding physical font size is determined using the currently
    /// configured resolution. See \ref set_resolution()).
    ///
    /// \sa \ref set_fixed_size(), \ref set_approx_size()
    ///
    virtual void set_scaled_size(font::size size) = 0;

    /// \brief Set rendering size as close to specified size as possible.
    ///
    /// This function sets the current rendering size as close to the specified size as
    /// possible.
    ///
    /// If this font face is not scalable, the fixed size, that is closest to the specified
    /// size, is chosen. For this purpose, the distance between two sizes is calculated as
    /// the square of the difference in width plus the square of the difference in height.
    ///
    /// If this font face is scalable, the specified size is first compared to the list of
    /// offered fixed sizes. If a match is found, given an implementation defined tolerance,
    /// the rendering size is set to that fixed size (as if by invoking \ref
    /// set_fixed_size()). Otherwise the scaled rendering size is set as specified (as if by
    /// invoking \ref set_scaled_size()).
    ///
    /// Note that in some cases, it is possible to set a specific size either as a fixed
    /// size or as a scaled size. In such cases the fixed size may be assumed to produce a
    /// better result, and is therefore always preferred.
    ///
    /// \sa \ref set_fixed_size(), \ref set_scaled_size()
    ///
    virtual void set_approx_size(font::size size) = 0;

    /// \brief Get selected rendering size.
    ///
    /// This function returns the currently selected rendering size for this font face. The
    /// rendering size is the resolution, in pixels, of the EM-square. Even though the
    /// rendering size is measured in number of pixels, it may be fractional.
    ///
    virtual auto get_size() noexcept -> font::size = 0;

    /// \{
    ///
    /// \brief Scaled ascender and descender of font face.
    ///
    /// These functions respectively return the ascender and descender of the font face,
    /// scaled to the selected rendering size. The scaled values are expressed as numbers of
    /// pixels, but the may in general be fractional.
    ///
    /// For a horizontal layout (when \p vertical is false), the *ascender* is the signed
    /// vertical distance from the baseline to the top of the horizontal nominal line
    /// box. The distance is positive when the top of the box is above the baseline, which
    /// it usually is. For a vertical layout, the ascender is the signed horizontal distance
    /// from the baseline to the right size of the vertical nominal line box. The distance
    /// is positive when the right side of the box is to the right of the baseline, which it
    /// usually is.
    ///
    /// For a horizontal layout (when \p vertical is false), the *descender* is the signed
    /// vertical distance from the baseline to the bottom of the horizontal nominal line
    /// box. The distance is negative when the bottom of the box is below the baseline,
    /// which it often is. For a vertical layout, the descender is the signed horizontal
    /// distance from the baseline to the left size of the vertical nominal line box. The
    /// distance is negative when the left side of the box is to the left of the baseline,
    /// which it usually is.
    ///
    /// The *nominal line box* will generally be tall / wide enough to fully contain most of
    /// glyphs of the font face. Any particular glyph, on the other hand, may extend outside
    /// the nominal line box.
    ///
    /// If grid fitting mode is enabled (\p grid_fitting), the returned value is definitely
    /// an integer, otherwise it may be fractional.
    ///
    /// \sa \ref get_baseline_spacing()
    ///
    virtual auto get_ascender(bool grid_fitting = true, bool vertical = false) noexcept -> float_type = 0;
    virtual auto get_descender(bool grid_fitting = true, bool vertical = false) noexcept -> float_type = 0;
    /// \}

    /// \brief Get distance in pixels between two baselines.
    ///
    /// This function returns the distance in pixels between two adjacent baselines. For
    /// horizontal layouts (when \p vertical is false), this is the distance between
    /// horizontal baselines. For vertical layouts, it is the distance between vertical
    /// baselines.
    ///
    /// If grid fitting mode is enabled (\p grid_fitting), the returned value is definitely
    /// an integer, otherwise it may be fractional.
    ///
    /// \sa \ref get_ascender(), \ref get_descender()
    ///
    virtual auto get_baseline_spacing(bool grid_fitting = true, bool vertical = false) noexcept -> float_type = 0;

    /// \brief Turn color loading on or off.
    ///
    /// This function turns color loading on or off. Passing false for \p on turns it
    /// off. Passing true turns it on. It is off by default.
    ///
    /// When *color loading* is turned on, a particular glyph will be loaded with capacity
    /// for color (\ref glyph_may_be_colored()) if the font face provides color support for
    /// that glyph.
    ///
    /// FIXME: Talk about linkage to color parameter of font face query. Generally, the
    /// application should turn color loading on precisely when the font face was produced
    /// by a font query (Fontconfig) where the color parameter was true. This minimizes the
    /// risk of rendering problems due to broken, or pseudo-broken fonts (missing / empty
    /// glyphs).
    ///
    /// \sa \ref has_color()
    /// \sa \ref glyph_may_be_colored()
    ///
    virtual void set_color_loading_enabled(bool on) = 0;

    /// \brief Find glyph for specified character.
    ///
    /// This function looks up the glyph for the specified character. A return value of zero
    /// always refers to the replacement glyph. This indicates that no glyph was available
    /// for the specified character in this font face. Every font face provides a
    /// replacement glyph, and the replacement glyph always occurs at index zero.
    ///
    virtual auto find_glyph(char_type ch) -> std::size_t = 0;

    /// \brief Get kerning adjustment for glyph pair.
    ///
    /// This function returns the appropriate kerning adjustment when the two specified
    /// glyphs occur next to each other with no whitespace between them. For horizontal
    /// layout (when \p vertical is false), the second specified glyph is assumed to occur
    /// to the right of the first specified glyph. For vertical layout, the second specified
    /// glyph is assumed to occur above the first specified glyph. This is despite the fact
    /// that vertical layouts generally flow downwards.
    ///
    /// The kerning adjustment is a correction to the conventional glyph distance obtained
    /// by placing the bearing point of the second glyph on the cursor position immediately
    /// following the first glyph. The kerning adjustment is expressed in number of pixels,
    /// and a positive value means that the two glyphs must be placed further apart. A
    /// negative value means that they must be placed closer together.
    ///
    /// If grid fitting mode is enabled (\p grid_fitting), the returned value is definitely
    /// an integer, otherwise it may be fractional.
    ///
    /// It is guaranteed that if any of the specified glyph indices are zero (the
    /// replacement glyph) then the kerning adjustment is zero.
    ///
    virtual auto get_kerning(std::size_t glyph_index_1, std::size_t glyph_index_2,
                             bool grid_fitting = true, bool vertical = false) -> float_type = 0;

    /// \brief Try to load glyph onto design tablet.
    ///
    /// This function tries to load the specified glyph onto the design tablet. The glyph is
    /// specified by its index in the font face. Use \ref find_glyph() to lookup glyph
    /// indexes. If the glyph format is supported, loading succeeds and the function returns
    /// `true`. Otherwise loading fails and the function returns `false`. Loading will never
    /// fail for the replacement glyph, i.e., when \p glyph_index is zero.
    ///
    /// During the loading process, the glyph is scaled to the selected font size, and grid
    /// fitted if grid fitting is requested (\p grid_fitting). The grid-fitting process
    /// attempts to improve appearance by slightly shifting control points of the glyph
    /// specification into alignment with the target pixel grid.
    ///
    /// If true is passed for \p vertical, grid fitting will be optimized for vertical
    /// layout. Otherwise it will be optimized for horizontal layout. If grid fitting is not
    /// enabled, \p vertical has no effect.
    ///
    /// While the glyph is on the design tablet, its metrics can be retrieved using \ref
    /// get_glyph_advance() and \ref get_glyph_bearing(). A glyph can also only be rendered
    /// while being on the design tablet (\ref render_glyph_mask()). The loaded glyph is not
    /// affected by subsequent changes of the rendering size (\ref set_fixed_size(), \ref
    /// set_scaled_size(), \ref set_approx_size()), or to the state of color loading (\ref
    /// set_color_loading_enabled()).
    ///
    /// The initially loaded glyph is the replacement glyph whose index is zero, and it is
    /// loaded with grid fitting enabled and optimized for horizontal layout.
    ///
    /// A side effect of grid fitting is that all glyph metrics will attain integer
    /// values. This applies to the glyph advance (\ref get_glyph_advance()) and the bearing
    /// point components (\ref get_glyph_bearing()).
    ///
    /// A glyph is loaded with or without capacity for color. See \ref
    /// glyph_may_be_colored() and \ref set_color_loading_enabled().
    ///
    [[nodiscard]] virtual bool try_load_glyph(std::size_t glyph_index, bool grid_fitting = true,
                                              bool vertical = false) = 0;

    /// \brief Get cursor advance distance for selected glyph.
    ///
    /// This function returns the cursor advance distance pertaining to the specified layout
    /// direction (\p vertical) for the glyph on the design tablet. The distance is measured
    /// in number of pixels, but it may be fractional. It is the distance that the cursor
    /// should be moved along the baseline in order to get from the cursor position that
    /// precedes the glyph to the cursor position that succeeds it. In a horizontal layout
    /// (when \p vertical is false), this is a horizontal distance. In a vertical layout, it
    /// is a vertical distance.
    ///
    /// If grid fitting was requested when the glyph was loaded (\ref try_load_glyph()), the
    /// returned value will be an integer.
    ///
    virtual auto get_glyph_advance(bool vertical = false) noexcept -> float_type = 0;

    /// \brief Get bearing point for selected glyph.
    ///
    /// This function returns the position of the bearing point pertaining to the specified
    /// layout direction (\p vertical) for the glyph on the design tablet. The position is
    /// specified with respect to the design space coordinate system.
    ///
    /// Note that in typography, the "bearing" metric is often associated with the
    /// distance from the bearing point to the glyph's bounding box, but in this context,
    /// the bearing metric is the absolute position of the bearing point in design space.
    ///
    /// The glyph bearing point is a point on the baseline and represents the cursor
    /// position that precedes the glyph with respect to the desired layout direction. For
    /// horizontal layout (when \p vertical is false), the returned bearing point position
    /// marks the cursor position to the left of the glyph. To get the cursor position to
    /// the right of the glyph, one must add the glyph's horizontal layout glyph advance
    /// (\ref get_glyph_advance()) to the X component of the horizontal layout bearing
    /// point. For vertical layout, the returned bearing point position marks the cursor
    /// position below the glyph. To get the cursor position above the glyph, one must add
    /// the vertical layout glyph advance to the Y component of the vertical layout bearing
    /// point.
    ///
    /// If grid fitting was requested when the glyph was loaded (\ref try_load_glyph()), the
    /// returned vector will have integer components.
    ///
    virtual auto get_glyph_bearing(bool vertical = false) noexcept -> vector_type = 0;

    /// \brief Get position of lower-left corner of glyph's bounding box.
    ///
    /// This function returns the design space position of the lower-left corner of the
    /// bounding box of the glyph on the design tablet. The returned position is not
    /// affected by glyph transformation or translation (\ref set_transform(), \ref
    /// set_translat()). To get the pixel-aligned bounding box of the transformed,
    /// translated, and rasterized glyph with respect to the target medium coordinate
    /// system, use \ref get_target_glyph_box() instead.
    ///
    /// \sa \ref get_glyph_size()
    /// \sa \ref get_target_glyph_box()
    ///
    virtual auto get_glyph_pos() noexcept -> vector_type = 0;

    /// \brief Get size of glyph's bounding box.
    ///
    /// This function returns the design space size of the bounding box of the glyph on the
    /// design tablet. To get the pixel-aligned bounding box of the rasterized glyph with
    /// respect to the target medium coordinate system, use \ref get_target_glyph_box()
    /// instead.
    ///
    /// \sa \ref get_glyph_pos()
    /// \sa \ref get_target_glyph_box()
    ///
    virtual auto get_glyph_size() noexcept -> vector_type = 0;

    /// \brief Whether glyph on design tablet has capacity for color.
    ///
    /// Returns true if the glyph that is currently on the design tablet has capacity for
    /// color. Returns false otherwise. A glyph that does not have capacity for color is
    /// black when rendered as a block of RGBA pixels (\ref render_glyph_rgba()). A glyph
    /// that does have *capacity for color* will generally produce a glyph that is not just
    /// back when rendered as a block of RGBA pixels.
    ///
    /// If the glyph is rendered as an alpha mask (\ref render_glyph_mask()), the result is
    /// an alpha mask regardless of whether the glyph has capacity for color. Such capacity
    /// is unused in this case.
    ///
    /// One may want to use this function to decide whether to render the currently loaded
    /// glyph using \ref render_glyph_mask() or \ref render_glyph_rgba().
    ///
    /// In order for a glyph to be loaded with capacity for color, the font face must
    /// provide color support for that glyph, so \ref has_color() must return
    /// true. Additionally, color loading needs to be turned on for the font face using \ref
    /// set_color_loading_enabled(). If color loading is not turned on, no glyphs will be
    /// loaded with capacity for color.
    ///
    /// In general, a font face can provide color support for some glyphs and not for
    /// others. This means that a particular glyph may be loaded without capacity for color
    /// even when the font face does have color support and color loading is turned on.
    ///
    /// \sa \ref has_color()
    /// \sa \ref set_color_loading_enabled()
    /// \sa \ref render_glyph_rgba()
    ///
    virtual bool glyph_may_be_colored() noexcept = 0;

    /// \brief Apply transformation to rendered glyph.
    ///
    /// This function sets the transformation that is generally applied to a glyph during
    /// the rendering process. It is applied precisely when the font face is scalable and
    /// the glyph on the design tablet was loaded with grid-fitting disabled (\ref
    /// is_scalable(), \ref try_load_glyph()). The new transformation applies to all
    /// subsequent glyph renderings that satisfy those criteria.
    ///
    /// When the transformation is applied, it is applied before the translation (\ref
    /// set_translat()). Transformation does not modify the glyph on the design tablet. See
    /// \ref font::face for the full explanation of the glyph rendering process.
    ///
    /// \sa \ref set_translat()
    /// \sa \ref reset_transform_translat()
    ///
    virtual void set_transform(const matrix_type& transform) noexcept = 0;

    /// \brief Apply translation to rendered glyph.
    ///
    /// This function sets the translation that is applied to a glyph during the rendering
    /// process. The new translation applies to all subsequent glyph renderings. When
    /// transformation is also applied, transformation is applied before translation (\ref
    /// set_transform()). Translation does not modify the glyph on the design tablet. See
    /// \ref font::face for the full explanation of the glyph rendering process.
    ///
    /// A positive first component moves the glyph towards the right. A positive second
    /// component moves the glyph upwards.
    ///
    /// If the font face is not scalable or the glyph on the design tablet was loaded with
    /// grid-fitting enabled, the effective translation will be the specified translation
    /// with each component rounded to the nearest integer value.
    ///
    /// \sa \ref set_transform()
    /// \sa \ref reset_transform_translat()
    ///
    virtual void set_translat(vector_type translat) noexcept = 0;

    /// \brief Reset glyph transformation and translation.
    ///
    /// This function resets any previously configured transformation and translation back
    /// to the identity matrix and zero respectively. See \ref set_transform() and \ref
    /// set_translat().
    ///
    /// \sa \ref set_transform(), \ref set_translat()
    ///
    virtual void reset_transform_translat() noexcept = 0;

    /// \brief Set position of rendering target point.
    ///
    /// This function sets the position of the rendering target point. It is specified with
    /// respect to the target medium coordinate system. The rendering target point
    /// determines the connection between positions in design space and positions in the
    /// target medium. For further details, see \ref font::face. Initially, the rendering
    /// target point is (0, 0).
    ///
    /// When using \ref render_glyph_mask() or \ref render_glyph_rgba() to render glyphs,
    /// the specified position is a position within the target image, and the initial
    /// position of (0, 0) is at the upper-left corner of the image. This means that useful
    /// cursor positions in design space need to have a negative Y component. To avoid that,
    /// the application can move to the rendering target point to the lower-left corner of
    /// the target image.
    ///
    virtual void set_target_pos(image::Pos pos) noexcept = 0;

    using comp_type = image::int8_type;
    using buffer_type = core::Buffer<comp_type>;
    using tray_type = image::Tray<comp_type>;

    /// \{
    ///
    /// \brief Render glyph into image.
    ///
    /// These functions render the glyph that is currently on the design tablet as a block
    /// of pixels and then writes that block of pixels to the specified target image (\p
    /// writer).
    ///
    /// As part of the rendering process, the glyph is transformed, translated, and
    /// rasterized. The affected target area in the specified image is exactly the area
    /// returned by \ref get_target_glyph_box(). For the full explanation of the general
    /// glyph rendering process, see \ref font::face.
    ///
    /// In the block of pixels generated by `render_glyph_mask()`, each pixel has only one
    /// component, which is an alpha component. That block of pixels is then written to the
    /// image using \ref image::Writer::put_block_mask(). This means that the color of the
    /// glyph in the target image is determined by the image writer's configured foreground
    /// color (\ref image::Reader::set_foreground_color()). Similarly, the background color
    /// for the affected area in the target image is determined by the configured background
    /// color (\ref image::Reader::set_background_color()).
    ///
    /// In the block of pixels generated by `render_glyph_rgba()`, each pixel is a full RGBA
    /// quadruple (red, green, blue, alpha). This pixel block is then written to the target
    /// image using \ref image::Writer::put_block_rgba().
    ///
    /// If the glyph on the design tablet has capacity for color (\ref
    /// glyph_may_be_colored()), and it utilizes that capacity, `render_glyph_rgba()` will
    /// produce a colored glyph image. Otherwise, it will produce a black glyph.
    ///
    /// When rendering multiple sequential glyphs, it is possible for the bounding box of
    /// one glyph to overlap the bounding box the next glyph. When working with
    /// transformations (\ref set_transform()), such overlap is ubiquitous. To avoid having
    /// a glyph partially clobber the previously rendered glyph, blending should be enabled
    /// in the image writer (\ref image::Writer::enable_blending()). Furthermore, when
    /// working with `render_glyph_mask()`, the background color should be made fully
    /// transparent (\ref util::colors::transparent).
    ///
    /// The specified buffer will be used to hold the rendered glyph before it is written to
    /// the image. The capacity of the buffer will be expanded as necessary.
    ///
    /// \sa \ref render_glyph_mask_a(), \ref render_glyph_rgba_a()
    /// \sa \ref glyph_may_be_colored()
    ///
    void render_glyph_mask(image::Writer& writer, buffer_type& buffer);
    void render_glyph_rgba(image::Writer& writer, buffer_type& buffer);
    /// \}

    /// \brief Get bounding box of rasterized glyph.
    ///
    /// This function returns the bounding box of the rasterized glyph with respect to the
    /// target medium coordinate system. To compute it, the design space bounding box of the
    /// transformed and translated glyph is first determined. The positions of the
    /// upper-left and lower-right corners of that bounding box are then snapped to the
    /// pixel grid (floor of X coordinate and ceiling of Y coordinate for the upper-left
    /// corner, ceiling of X coordinate and floor of Y coordinate for the lower-right
    /// corner). Finally, the two positions are mapped to target medium coordinates by
    /// negating the Y coordinate and adding the result to the configured position of the
    /// rendering target point (\ref set_target_pos()). The position of the returned box is
    /// then the mapped position of the upper-left corner and the size is the difference
    /// between the two mapped corners.
    ///
    virtual auto get_target_glyph_box() -> image::Box = 0;

    /// \{
    ///
    /// \brief Render glyph into pixel block.
    ///
    /// These functions render the loaded glyph to the specified pixel block (\p tray). The
    /// loaded glyph is the one that is currently on the design tablet. The target box,
    /// which is formed from the specified position (\p pos) and the size of the specified
    /// tray (\p tray.size), specifies the subsection of a larger imaginary target medium
    /// that coincides with the pixel block. Any part of the rendered glyph that falls
    /// outside the target box is not actually rendered. What the larger medium corresponds
    /// to is irrelevant to these functions, but it could for example correspond to an
    /// actual target image, or to a user interface window.
    ///
    /// The area determined by \ref get_target_glyph_box() is the bounding box of the fully
    /// rasterized glyph within the larger imaginary target medium. In general, however,
    /// only part of the glyph's bounding box will fall inside the target box (\p pos, \p
    /// tray.size), and only that part is actually rendered. If the glyph's bounding box
    /// falls entirely outside the target box, then none of the glyph will be rendered.
    ///
    /// These functions do not generally overwrite all pixels in the specified pixel
    /// block. A pixel in the pixel block is definitely written to if it falls inside the
    /// actual "ink" of the glyph or so close to it that the alpha component becomes nonzero
    /// due to antialiasing. A pixel in the pixel block is definitely not written to if it
    /// falls inside a part of the target box that falls outside the glyph's bounding
    /// box. Any other pixel, which is a pixel that falls inside the glyph's bounding box
    /// but fully outside the glyph's ink, may or may not be written to. It depends on the
    /// implementation (\ref font::implementation) and on the underlying type of font
    /// face. Because of this, in most cases, the application should clear the pixel block
    /// before passing it to these functions.
    ///
    /// The pixel block is specified as a pixel tray (\p tray), which is formed from a pixel
    /// iterator (\p tray.iter) and a block size (\p tray.size). The pixel iterator must
    /// point to the first component of the pixel at the upper-left corner of the block. In
    /// the case of `render_glyph_mask_a()`, each pixel has only one component. In the case
    /// of `render_glyph_rgba_a()`, each pixel has 4 components.
    ///
    /// The orientation and position of the rendered glyph within the imaginary target
    /// medium depends on the configured transformation (\ref set_transform()), the
    /// configured translation (\ref set_translat()), and the configured position of the
    /// rendering target point (\p set_target_pos()). All these parameters are accounted for
    /// in the target area returned by \ref get_target_glyph_box().
    ///
    /// After a copy of the glyph description has been transformed and translated in design
    /// space, the glyph is rasterized, and then the part of the rasterization that overlaps
    /// with the specified target box is written to the specified pixel block.
    ///
    /// In the case of `render_glyph_mask_a()`, the written pixels have only one channel,
    /// and that channel can either be thought of as an alpha channel or as a linearly
    /// expressed intensity channel. In the case of `render_glyph_rgba_a()`, the pixels are
    /// ordinary RGBA quadruples with gamma-compressed color channels (sRGB).
    ///
    /// Channel component values are 8 bits wide. If `comp_type` is wider than 8 bits, only
    /// the first 8 bits are used. Component values are encoded into memory words (objects
    /// of type `comp_type`) using `util::pack_int<comp_type, 8>(value)` (see \ref
    /// util::pack_int()).
    ///
    /// The `_a` suffix in these function names can be thought of as meaning
    /// "alternative". It is there to avoid a name clash with closely related functions.
    ///
    /// \sa \ref render_glyph_mask(), \ref render_glyph_rgba()
    ///
    virtual void render_glyph_mask_a(image::Pos pos, const tray_type& tray) = 0;
    virtual void render_glyph_rgba_a(image::Pos pos, const tray_type& tray) = 0;
    /// \}

    virtual ~face() noexcept = default;
};


} // namespace archon::font

#endif // ARCHON_X_FONT_X_FACE_HPP
