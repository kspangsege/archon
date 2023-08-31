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

#include <archon/core/buffer.hpp>
#include <archon/math/vec.hpp>
#include <archon/image/geom.hpp>
#include <archon/image/iter.hpp>
#include <archon/image/writer.hpp>
#include <archon/font/size.hpp>
#include <archon/font/code_point.hpp>


namespace archon::font {


/// \brief Particular variant of particular font.
///
/// This class represents a particular font face, and can be used to examine and render the
/// glyphs available in that font face. A font face is a particular variant (bold, italic,
/// ...) of a font.
///
/// If this font face provides fixed sizes (see \ref get_num_fixed_sizes()), then the
/// initial rendering size is the fixed size that is closest in area to 12 by 12
/// pixels. Otherwise, this font face is scalable, and the initial rendering size is set to
/// exactly 12 by 12 pixels.
///
/// New font face objects are generally created by calling either \ref                  
/// font::Loader::load_face().
///
/// CAUTION: A font face object must be accessed by at most one thread at a time.
///
class Face {
public:
    using char_type = font::CodePoint::char_type;
    using float_type = font::Size::comp_type;
    using vec_type = math::Vec<2, float_type>;

    virtual ~Face() noexcept = default;

    /// \brief Get font family name.
    ///
    /// This function returns the name of the font family that this font face belongs to,
    /// e.g. "Times New Roman".
    ///
    /// \note The family name can be empty for some fonts.
    ///
    virtual auto get_family_name() -> std::string_view = 0;

    /// \brief Whether font face is bold.
    ///
    /// This function returns `true` if, and only if this font face is bold. A bold font
    /// face is one whose glyphs a darker and heavier than normal.
    ///
    virtual bool is_bold() noexcept = 0;

    /// \brief Whether font face is italic.
    ///
    /// This function returns `true` if, and only if this font face is italic/oblique. An
    /// italic, or oblique font face is normally one whose glyphs a slanted to the right.
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

    /// \brief Number of fixed rendering sizes offered.
    ///
    /// This function returns the number of fixed rendering sizes (bitmap strikes) offered
    /// in this font face.
    ///
    /// Scalable fonts may or may not provide fixed rendering sizes. If they do, it should
    /// be understood as the preferrable sizes that lead to the result of highest quality.
    ///
    /// For fonts that are not scalable (\ref is_scalable()), this funcction is guaranteed
    /// to return at least 1.
    ///
    virtual int get_num_fixed_sizes() = 0;

    /// \brief Get particular fixed rendering size.
    ///
    /// This function returns the specified fixed rendering size. The fixed rendering size
    /// is identified by its index in the list of offered fixed rendering sizes. The number
    /// of entried in this list is returned by \ref get_num_fixed_sizes().
    ///
    /// \sa \ref set_fixed_size()
    ///
    virtual auto get_fixed_size(int fixed_size_index) -> font::Size = 0;

    /// \brief Use specified fixed rendering size.
    ///
    /// This function selects the specified fixed rendering size as the current rendering
    /// size for this font face. The fixed rendering size is identified by its index in the
    /// list of offered fixed rendering sizes. The number of entried in this list is
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
    /// FIXME: Explain that specified size will be silently clamped to range allowed by implementation.                                                     
    ///
    /// If the font face is scalable (\ref is_scalable()), this function sets the current
    /// rendering size as specified. If the font face is not scalbale, it is an error to
    /// call this function and an exception of unspecified type will be thrown.
    ///
    /// \note There is no guarantee that the rendered size of any particular glyph is
    /// limited to this size. The selected rendering size is merely a nominal size.
    ///
    /// \sa \ref set_fixed_size(), \ref set_approx_size()
    ///
    virtual void set_scaled_size(font::Size) = 0;

    /// \brief Set rendering size as close to specified size as possible.
    ///
    /// This function sets the current rendering size as close to the specified size as
    /// possible.
    ///
    /// If this font face is not scalable, the fixed size, that is closest to the specified
    /// size, is chosen. For this purpose, the distance between two sizes is square of the
    /// difference in width plus the square of the difference in height.
    ///
    /// If this font face is scalable, the specified size is first compared to the list of
    /// offered fixed sizes. If a match is found, given an implementation defined tolerance,
    /// the rendering size is set to that fixed size (as if by invoking \ref
    /// set_fixed_size()). Otherwise the scaled rendering size is set as specified (as if by
    /// invoking \re set_Scaled_size()).
    ///
    /// Note that in some cases, it is possible to set a specific size either as a fixed
    /// size or as a scaled size. In such cases the fixed size may be assumed to produce a
    /// better result, and is therefore always preferred.
    ///
    /// \sa \ref set_fixed_size(), \ref set_scaled_size()
    ///
    virtual void set_approx_size(font::Size) = 0;

    /// \brief Get selected rendeing size.
    ///
    /// This function returns the currently selected rendering size for this font face. The
    /// rendering size is the resolution, in pixels, of the EM-square. Even though the
    /// rendering size is measured in number of pixels, it may be fractional.
    ///
    virtual auto get_size() noexcept -> font::Size = 0;

    /// \brief    
    ///
    /// Get the distance in pixels between two baselines.
    ///
    /// The returned value depends on the loaded font face, on the selected rendering size,
    /// on the selected layout direction, and on whether grid fitting is enabled.
    ///
    /// For horizontal layouts (the default), this is the distance between adjacent
    /// horizontal baselines. If the layout is also left-to-right (the default), then the
    /// returned value will be positive, otherwise it will be negative.                                                                                                         
    ///
    /// For vertical layouts, it is instead the distance between adjacent vertical
    /// baselines. If the layout is top-to-bottom (the default), then the returned value
    /// will be negative, otherwise it will be positive.                                               
    ///
    /// If grid fitting mode is enabled, the returned value is always an
    /// integer, otherwise the value may be fractional.
    ///
    virtual auto get_baseline_spacing(bool vertical = false, bool grid_fitting = true) noexcept -> float_type = 0;

    /// \brief    
    ///
    /// Get the displacement of the baseline relative to the bottom of the line for a
    /// horizontal valyout. If the layout is vertical, it is the displacement relative to
    /// the left side of the line. The value is normally positive, meaning that the baseline
    /// lies within the line box. If the basline happens to lie outside, then the value may
    /// be negative (depending on side). It is measured in number of pixels.                           
    ///
    /// If grid fitting mode is enabled, the returned value is always an integer, otherwise
    /// the value may be fractional.
    ///
    virtual auto get_baseline_offset(bool vertical = false, bool grid_fitting = true) noexcept -> float_type = 0;

    /// \brief Find glyph for specified character.
    ///
    /// This function looks up the glyph for the specified character.
    ///
    /// A return value of zero always refers to the replacement glyph, and indicates that no
    /// glyph was available for the specified character in this font face.
    ///
    virtual auto find_glyph(char_type) -> std::size_t = 0;

    /// \brief              
    ///
    /// Get the appropriate kerning adjustment of the distance between the two specified                      
    /// glyphs when they appear juxtaposed.
    ///
    /// In a horizontal layout, it is assumed that <tt>glyph1</tt> is to the left of
    /// <tt>glyph2</tt>, while in a vertical layout, it is assumed that <tt>glyph1</tt> is
    /// above <tt>glyph2</tt>.
    ///
    /// A positive result means that the two glyphs must be place further apart, while a
    /// negative result means that they must be placed closer together.
    ///
    /// The adjustment is expressed in number of pixels, and may be a fractional
    /// number. However, if \c grid_fitting is true, then it is always an integer.
    ///
    /// It is guaranteed that if any of the specified glyph indices are zero (the
    /// replacement glyph) then the kerning adjustment is zero.
    ///
    /// \param glyph1_index The index of the left or upper glyph.
    ///
    /// \param glyph2_index The index of the right or lower glyph.
    ///
    /// \param vertical Pass true when the layout is vertical, as opposed to horizontal.
    ///
    /// \param grid_fitting Pass false to disable grid fitting. When grid fitting is
    /// enabled, the result is guaranteed to always be an integer.
    ///
    virtual auto get_kerning(std::size_t glyph_index_1, std::size_t glyph_index_2,
                             bool vertical = false, bool grid_fitting = true) -> float_type = 0;

    /// \brief Lead glyph onto design tablet.
    ///
    /// FIXME: Explain design tablet in class-level doc.                          
    ///
    /// This function loads the specified glyph onto the design tablet. After this you can
    /// inspect and modify it, and finally you can render it as a block of pixels.
    ///
    /// As a part of the loading process, a glyph is first scaled according to the
    /// previously specified rendering size (see \ref set_scaled_size()), then, if
    /// requested, the scaled glyph is grid fitted, meaning that it is modified slightly to
    /// improve the appearance of the pixelized image.
    ///
    /// The design tablet has a coordinate system and an origin, and each newly loaded glyph
    /// is placed on the tablet such that the lower left corner of its axis-aligned bounding
    /// box is coincident with the origin of the tablet. The unit of measurement on both
    /// coordinate axes is 'pixels'.
    ///
    /// The initially loaded glyph is the replacement glyph whose index is zero, and it is
    /// loaded with grid fitting enabled.
    ///
    /// A side effect of grid fitting is that all glyph metrics will attain integer
    /// values. This applies to the glyph advance, and the glyph bearing components.
    ///
    /// \param glyph_index The index of the glyph to load. Use \c find_glyph to obtain
    /// indexes of glyphs. Specify zero to load the replacement glyph.
    ///
    /// \param grid_fitting Pass `false` to disable grid fitting.
    ///
    virtual void load_glyph(std::size_t glyph_index, bool grid_fitting = true) = 0;

    /// \brief Get cursor advance distance for glyph.
    ///
    /// This function returns the cursor advance distance for the currently loaded
    /// glyph. The distance is measured in number of pixels, but may be fractional.
    ///
    /// When thinking about cursor positions as occurring between glyphs, the cursor advance
    /// distance is the distance from the cursor position immediately before the glyph to
    /// the cursor position immediately after it. For a horizontal layout, it is the
    /// distance along an imaginary horizontal baseline. Respectively it is the distance
    /// along a vertical baseline when the layout is vertical.
    ///
    /// The returned value will be an integer if grid fitting was requested when the glyph
    /// was loaded (\ref load_glyph()).
    ///
    /// The returned value will always reflect the glyph as it was immediately after it was
    /// loaded. That is, it is not affected by subsequent modifications of the glyph on the
    /// design tablet.
    ///
    virtual auto get_glyph_advance(bool vertical = false) noexcept -> float_type = 0;

    /// \brief Get preceding cursor position for glyph.
    ///
    /// This function returns the position on the design tablet of the glyph bearing point
    /// pertaining to the specified layout direction.
    ///
    /// The glyph bearing point is a point on the baseline and represents the cursor
    /// position that precedes the glyph with respect to the desired layout direction.
    ///
    /// For a horizontal layout, the bearing point is normally located somewhere to the left
    /// of the glyph, and must always be thought of as the cursor position immediately to
    /// the left of the glyph. Likewise, for a vertical layout, the bearing point is
    /// normally located below the glyph, and must be thought of as the cursor position
    /// immediately below the glyph.
    ///
    /// If you need the bearing point for a right-to-left layout, add the horizontal glyph
    /// advance (\ref get_glyph_advance()) to the first component of the vector returned by
    /// this function.
    ///
    /// Both components of the returned vector will be integers if grid fitting was
    /// requested when the glyph was loaded (\ref load_glyph()). Otherwise they may have
    /// fractional values.
    ///
    /// The returned vector will always reflect the glyph as it was immediately after it was
    /// loaded. That is, it is not affected by subsequent modifications of the glyph on the
    /// design tablet.
    ///
    virtual auto get_glyph_bearing(bool vertical = false) noexcept -> vec_type = 0;

    /// \brief Change position of glyph on design tablet.
    ///
    /// This function translates the position of the currently loaded glyph on the design
    /// tablet by the specified amount.
    ///
    /// FIXME: Explain interaction with \ref set_origin_pos().                          
    ///
    /// It is *not* recommended to translate a grid-fitted glyph by a fractional number of
    /// pixels. This is expected to produce a particularly poor result.
    ///
    /// A positive first component moves the glyph towards the right. A positive second
    /// component moves the glyph upwards.
    ///
    virtual void translate_glyph(vec_type) = 0;

    /// \brief Get size of glyph's pixel-aligned bounding box.
    ///
    /// This function determines the size of the pixel block that will be generated when the
    /// glyph is rendered.
    ///
    /// This function is a shorthand for calling \ref get_glyph_pa_box() and then
    /// calculating the width as `right - left` and height as `top - bottom`.
    ///
    auto get_glyph_pa_size() -> image::Size;

    /// \brief Determine glyph's pixel-aligned bounding box.
    ///
    /// This function determines the pixel-aligned bounding box of the currently loaded
    /// glyph given its current translation. The size of this box is the size of the block
    /// of pixels that would be generated if the glygh was rendered now (\ref
    /// render_glyph_mask()). See also \ref get_target_glyph_box().
    ///
    /// Superimposed on the design tablet is a pixel grid. Each pixel in the grid must be                   
    /// thought of as a small rectangle. One of these rectangles has its lower left corner
    /// coincident with the origin of the design tablet, and its upper right corner at
    /// (1,1).
    ///
    /// The first of the returned points, \p left, \p bottom, is the design table                      
    /// coordinates of the lower left corner of the lower left pixel of the generated
    /// block. The second point, \p right, \p top, is the upper right corner of the upper
    /// right pixel of the generated block. Note that this implies that the horizontal
    /// number of generated pixels is `right - left`, and similarly in the vertical
    /// direction.
    ///
    /// When the target origin is set to (0,0) the returned block coincides with the region              
    /// of the target image that will be affected during a call to render_pixels_to().
    ///
    /// It shall be guaranteed that when the target origin is set to (0,0), \ref               
    /// render_pixels_to() shall not affect nor access any pixel outside the box returned by
    /// this method.
    ///
    /// The following equations reflect the correspondence between the box returned by this                   
    /// function and the value returned by \ref get_glyph_size(), assuming the glyph is
    /// translated only by integer values after being loaded:
    ///
    /// ```
    ///   size = get_glyph_size()
    ///
    ///   right - left  =  ceil(size.x)
    ///   top - bottom  =  ceil(size.y)
    /// ```
    ///
    void get_glyph_pa_box(int& left, int& right, int& bottom, int& top);

    /// \brief    
    ///
    /// Set the position of the origin of the design tablet within the target image. The
    /// initial position is (0,0) which corresponds to the upper left corner of the image.
    ///
    /// For normal fonts, this position will need to be moved down. If it is not moved down,
    /// most glyphs will be situated above to top of the image, and will therefore not show
    /// up.
    ///
    /// FIXME: Explain briefly how to obtain a good value for this position?                                          
    ///
    void set_target_pos(image::Pos) noexcept;

    /// \brief    
    ///
    ///    
    ///
    auto get_target_glyph_box() -> image::Box;

    /// \{
    ///
    /// \brief Write glyph to image.
    ///
    /// Render the glyph as a block of pixels and merge the pixels into the specified image.
    ///
    /// Data is always written in the form of luminance components, and without any alpha              
    /// information. It is expected that your image writer is previously configured for
    /// color mapping, such that the selected foreground color of the image writer
    /// determines the color of the rendered glyph. Likewise it will be the selected
    /// background color that determines the contrast/background color.
    ///
    /// Because glyphs can easily end up overlaping each other, it is recommended that               
    /// blending is also enabled in the image writer, and that the backgound color is made
    /// to be transparet. This way the rendered glyph will be blended nicely into the
    /// original contents of the image. For optimum results, the background color should be
    /// the same color as the foreground only fully transparent.
    ///
    /// The final position of the glyph in the image is determined by matching the design                
    /// tablet origin with the target origin set by set_target_origin(). Thus, the final
    /// position of the glyph in the image is influenced both by the position of the glyph
    /// on the design tablet relative to the design tablet origin, and by the currently
    /// selected target origin.
    ///
    /// \param img A writable interface of the image into which the glyph will be merged.            
    ///
    /// \todo FIXME: It might be attractive to offer alternative blending             
    /// modes/functions. One could simply be channel = max(orig, new). Then the original
    /// 'max(orig,new) intensity stuff' can easily be reenabled.
    ///
    void render_glyph_mask(image::Writer&);
    void render_glyph_rgba(image::Writer&);
    /// \}

    using comp_type = image::int8_type;
    using iter_type = image::Iter<comp_type>;

    /// \{
    ///
    /// \brief   
    ///
    ///   
    ///
    void render_glyph_mask_a(const iter_type& iter, image::Size size);
    void render_glyph_rgba_a(const iter_type& iter, image::Size size);
    /// \}

protected:
    /// \brief   
    ///
    ///   
    ///
    virtual void do_get_glyph_pa_box(int& left, int& right, int& bottom, int& top) = 0;

    /// \{
    ///
    /// \brief   
    ///
    /// FIXME: Explain: \p pos is the position of the origin of the design tablet within the target tray (iter, size), and should generally be set to the currently configured target position (\ref ....)              
    ///
    /// FIXME: Combine \p iter \p size into single tray argument.            
    ///
    virtual void do_render_glyph_mask(image::Pos pos, const iter_type& iter, image::Size size) = 0;
    virtual void do_render_glyph_rgba(image::Pos pos, const iter_type& iter, image::Size size) = 0;
    /// \}

private:
    core::Buffer<comp_type> m_render_buffer;
    image::Pos m_target_pos = { 0, 0 };
};








// Implementation


inline auto Face::get_glyph_pa_size() -> image::Size
{
    int left = 0, right = 0, bottom = 0, top = 0;
    do_get_glyph_pa_box(left, right, bottom, top); // Throws
    int width  = right - left;
    int height = top - bottom;
    return { width, height };
}


inline void Face::get_glyph_pa_box(int& left, int& right, int& bottom, int& top)
{
    do_get_glyph_pa_box(left, right, bottom, top); // Throws
}


inline void Face::set_target_pos(image::Pos pos) noexcept
{
    m_target_pos = pos;
}


inline auto Face::get_target_glyph_box() -> image::Box
{
    int left = 0, right = 0, bottom = 0, top = 0;
    do_get_glyph_pa_box(left, right, bottom, top); // Throws
    int width  = right - left;
    int height = top - bottom;
    image::Pos pos = m_target_pos;
    // Note the inversion of the Y-axis
    core::int_add(pos.x, left); // Throws
    core::int_sub(pos.y, top); // Throws
    image::Size size = { width, height };
    return { pos, size };
}


inline void Face::render_glyph_mask_a(const iter_type& iter, image::Size size)
{
    do_render_glyph_mask(m_target_pos, iter, size); // Throws
}


inline void Face::render_glyph_rgba_a(const iter_type& iter, image::Size size)
{
    do_render_glyph_rgba(m_target_pos, iter, size); // Throws
}


} // namespace archon::font

#endif // ARCHON_X_FONT_X_FACE_HPP
