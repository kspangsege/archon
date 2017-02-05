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

#ifndef ARCHON_FONT_CACHE_HPP
#define ARCHON_FONT_CACHE_HPP

#include <memory>
#include <string>

#include <archon/math/interval.hpp>
#include <archon/image/writer.hpp>
#include <archon/font/list.hpp>


namespace archon {
namespace font {

class FontCache;

std::unique_ptr<FontCache> new_font_cache(std::shared_ptr<FontList>);



/// Thread safety: Instances are not thread safe, but the class is, as long as
/// each thread accesses a different instance, and no two instances are
/// associated with the same FontLoader instance via the associated FontList
/// instances. That is, you also need one FontList instance, and one FontLoader
/// instance per thread. See \ref ThreadSafety.
///
/// \todo FIXME: The grid_fitting flag should be a fixed property of a
/// particular font and part of the font selection parameters along with the
/// font size. Thus, there shall be one grid fitted font, and another one that
/// is the same in all respects except that it is not grid fitted.
class FontCache {
public:
    /// Fetch the default font. Guarantees to not cause the associated list to
    /// scan through the font path for further font files.
    ///
    /// Font IDs are never negative. The returned ID must be released when no
    /// longer needed (see FontOwner).
    virtual int acquire_default_font() = 0;

    /// Fetch a font of the specified size from the default family and with the
    /// default style. Will always succeed. The returned font, however, may not
    /// be the exact size you requested. Guarantees to not cause the associated
    /// list to scan through the font path for further font files. On the other
    /// hand, it does not guarantee to find a matching fixed size, if that size
    /// is only available after scanning the font path.
    ///
    /// Font IDs are never negative. The returned ID must be released when no
    /// longer needed (see FontOwner).
    virtual int acquire_default_font(double width, double height) = 0;

    struct FontDesc {
        std::string family;
        double boldness, italicity;
        math::Vec2 size;
        bool operator==(const FontDesc&) const noexcept;
    };

    /// Will always succeed. The returned font, however, may not be exactly what
    /// you requested. To the greatest possible extent, it will be the best
    /// match among the available fonts.
    ///
    /// Font IDs are never negative. The returned ID must be released when no
    /// longer needed (see FontOwner).
    virtual int acquire_font(const FontDesc&) = 0;

    virtual void release_font(int font_id) = 0;

    /// Returns FontDesc::size as returned by get_font_desc().
    virtual math::Vec2 get_font_size(int font_id) = 0;

    /// Get the descriptor for the specified font. This can always be done
    /// without loading the face proper.
    ///
    /// The returned font size reflects the actual rendering size of the
    /// specified font, which may or may not be the same as the size that was
    /// originally requested.
    virtual void get_font_desc(int font_id, FontDesc&) = 0;

    /// RAII scheme (Resource acquisition is initialization) for font IDs.
    struct FontOwner {
        FontOwner(FontCache& font_cache, int font_id = -1) noexcept:
            m_font_cache{font_cache},
            m_font_id{font_id}
        {
        }
        int get() const noexcept
        {
            return m_font_id;
        }
        int release() noexcept
        {
            int f = m_font_id;
            m_font_id = -1;
            return f;
        }
        void reset(int font_id = -1);
        ~FontOwner()
        {
            if (0 <= m_font_id)
                m_font_cache.release_font(m_font_id);
        }
    private:
        FontOwner(const FontOwner&);
        FontCache& m_font_cache;
        int m_font_id;
    };

    struct FontInfo {
        std::string name; // Descriptive name of the specified font. Each font_id should map to a unique name, but it is recommended to not rely in it.
        int num_glyphs; // Number of glyphs provided by the specified font.
    };

    virtual void get_font_info(int font_id, FontInfo& info) = 0;

    struct FontMetrics {
        math::Interval lateral_span; // For horizontal layouts, this is known as the fonts descender and ascender.
    };

    virtual void get_font_metrics(int font_id, bool vertical, bool grid_fitting,
                                  FontMetrics& metrics) = 0;

    struct GlyphInfo {
        int index;
        double advance; // Distance along the baseline from preceeding to succeeding bearing point. Normally always positive.
        double kerning; // Additional space between this glyph and the glyph of the preceeding character in the list. Is negative if the glyphs should be moved closer together.
    };

    enum KernType {
        kern_No,  ///< No kerning information is retreived. All are set to zero.
        kern_Inc, ///< Kerning information is retrieved, and the first character in the list is assumed to be the leftmost/bottom-most one.
        kern_Dec  ///< Kerning information is retrieved, and the first character in the list is assumed to be the rightmost/top-most one.
    };

    // \param glyps A buffer with at least as many elements as the number of characters passed in.
    // Must also handle the curious case of "T.V.Smith" where T and V could get too close due to kerning.
    virtual void get_glyph_info(int font_id, bool vertical, bool grid_fitting, KernType kern,
                                int num_chars, const wchar_t* chars, GlyphInfo* glyphs) = 0;

    enum Direction {
        dir_LeftToRight,
        dir_TopToBottom,
        dir_RightToLeft,
        dir_BottomToTop
    };

    /// \param glyphs Glyph indices. If a negative index is encountered, it will
    /// be skipped, and so will the corresponding entry in <tt>components</tt>.
    ///
    /// \param components Glyph position components. If \c coord_type is
    /// coord_HoriLine then the first component is the common Y coordinate. If
    /// \c coord_type is coord_VertLine then the first component is the common X
    /// coordinate.
    ///
    /// \param img_writer The configured clipping region is respected. Blending
    /// setting is respected, but should generally be enabled, because glyphs
    /// can easily overlap.
    void render_text(int font_id, bool grid_fitting, Direction direction,
                     int num_glyphs, const int* glyphs, const float* components,
                     image::ImageWriter& img_writer);

    enum BearingType {
        bearing_None,  ///< Bearing point is at the lower left corner of the bounding box of the glyph
        bearing_Above, ///< Bearing point is above the glyph on a vertical baseline
        bearing_Right, ///< Bearing point is on the right side of the glyph on a horizontal baseline
        bearing_Below, ///< Bearing point is below the glyph on a vertical baseline
        bearing_Left   ///< Bearing point is on the left side of the glyph on a horizontal baseline
    };

    enum CoordType {
        coord_Cloud, ///< Each glyph consumes both an X and an Y coordinate.
        coord_Hori,  ///< Each glyph consumes only an X coordinate (horizontal baseline).
        coord_Vert   ///< Each glyph consumes only an Y coordinate (vertical baseline).
    };

    /// A slightly more flexible version of render_text().
    virtual void render_glyphs(int font_id, bool grid_fitting,
                               BearingType bearing_type, CoordType coord_type,
                               int num_glyphs, const int* glyphs, const float* components,
                               image::ImageWriter& img_writer) = 0;

    /// Holds information about the size and position of the axis aligned
    /// bounding box containing the glyph. The position is relative to the
    /// cursor position and also depends on the layout direction as follows:
    ///
    /// <pre>
    ///
    ///     Direction        Vector from cursor position to
    ///     of layout        lower left corner of glyph box
    ///   ---------------------------------------------------
    ///     left to right    hori_pos
    ///     right to left    (rev_pos[0], hori_pos[1])
    ///     bottom to top    vert_pos
    ///     top to bottom    (vert_pos[0], rev_pos[1])
    ///
    /// </pre>
    struct GlyphBoxInfo {
        math::Vec2F size, hori_pos, vert_pos, rev_pos;
    };

    virtual void get_glyph_box_info(int font_id, bool grid_fitting, int num_glyphs,
                                    const int* glyphs, GlyphBoxInfo* info) = 0;

    virtual ~FontCache() {}
};




// Implementation

inline bool FontCache::FontDesc::operator==(const FontDesc& d) const throw()
{
    return family == d.family && boldness == d.boldness &&
        italicity == d.italicity && size == d.size;
}

inline void FontCache::FontOwner::reset(int font_id)
{
    int f = m_font_id;
    m_font_id = font_id;
    if (0 <= f)
        m_font_cache.release_font(f);
}

inline void FontCache::render_text(int font_id, bool grid_fitting, Direction dir,
                                   int num_glyphs, const int* glyphs, const float* components,
                                   image::ImageWriter& img_writer)
{
    BearingType bearing_type;
    CoordType coord_type;
    switch (dir) {
        case dir_LeftToRight:
            bearing_type = bearing_Left;
            coord_type   = coord_Hori;
            goto render;
        case dir_RightToLeft:
            bearing_type = bearing_Right;
            coord_type   = coord_Hori;
            goto render;
        case dir_BottomToTop:
            bearing_type = bearing_Below;
            coord_type   = coord_Vert;
            goto render;
        case dir_TopToBottom:
            bearing_type = bearing_Above;
            coord_type   = coord_Vert;
            goto render;
    }
    return;

  render:
    render_glyphs(font_id, grid_fitting, bearing_type, coord_type,
                  num_glyphs, glyphs, components, img_writer);
}

} // namespace font
} // namespace archon

#endif // ARCHON_FONT_CACHE_HPP
