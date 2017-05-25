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

#ifndef ARCHON_FONT_TEXT_FORMAT_HPP
#define ARCHON_FONT_TEXT_FORMAT_HPP

#include <cmath>
#include <algorithm>
#include <vector>
#include <string>

#include <archon/core/enum.hpp>
#include <archon/math/interval.hpp>
#include <archon/math/vector.hpp>
#include <archon/font/cache.hpp>


namespace archon {
namespace font {

/// A powerfull, flexible, and efficient text formatter.
///
/// This is an abstract class that has no clue about how the formatted text is
/// to be rendered. It only knows how to format it.
///
/// The layout is constructed by first assembling characters into words. Words
/// are then assembled into lines, which in turn are assembled into pages.
///
/// Words are separated by white space, which in this context is one of the
/// following three ASCII characters:
///
///     SP  Space     (word break)
///     NL  New line  (line break)
///     FF  Form feed (page break)
///
/// Lines are separated by NL and FF, and pages are separated by FF.
///
/// Not thread-safe.
///
/// FIXME: Implement an adjustable displacement property and consider clipping.
class TextFormatter {
public:
    /// FIXME: Must instead take a StringUtf32 or std::u32string as argument. We
    /// can also provide a version that takes a wide string, but it must then
    /// also take a locale. For now it is the assumption that the specified
    /// string is encoded according to the classic locale.
    void write(std::wstring);

    /// Like write(std::wstring) but adds a trailing newline.
    void writeln(std::wstring);

    void write(const wchar_t* text, std::size_t n);

    /// Discard all previously written text, but keep all formatter settings.
    ///
    /// Note: Some settings do not take effect immediately if a layout session
    /// was already started. In all such cases the setting will take effect on a
    /// layout session initiated after this method is called. Sub-classes should
    /// override this method if they need to do extra clearing, but this method
    /// must be called from the overriding method.
    virtual void clear();

    // Get the number of pages in use by the current layout.
    int get_num_pages();

    // Get the dimensions of the specified page of the current layout.
    // Components will be integers if grid fitting was enabled in this session.
    // The width is always the width of the widest page clamped to the specified width range.
    math::Vec2 get_page_size(int page_index = 0);

    // Set the minimum and maximum page width in pixels. If the maximum component is less than or equal to zero, the page has unbounded width. Otherwise, if the maximum component is less than the minimum component, then the page width will be equal to the maximum component.
    // Grid fitting: Minimum will be rounded upwards, and maximum will be rounded downwards.
    // If a layout session is already initiated, this setting will not have any effect until clear() is called and a new session is started.
    // By default the minimum width is zero, and the maximum is unbounded.
    void set_page_width(const math::Interval& width);

    // Set the minimum and maximum page height in pixels. If the maximum component is less than or equal to zero, the page has unbounded height. Otherwise, if the maximum component is less than the minimum component, then the page height will be equal to the maximum component.
    // Grid fitting: Minimum will be rounded upwards, and maximum will be rounded downwards.
    // If a layout session is already initiated, this setting will not have any effect until clear() is called and a new session is started.
    // By default the minimum height is zero, and the maximum is unbounded.
    void set_page_height(const math::Interval& height);

    // Set the page size in pixels. If a component is less than or equal to zero, the page is unbounded in that direction.
    // This is just a shorthand for calling both set_page_width() and set_page_height() with minimum and maximum being the same.
    void set_page_size(const math::Vec2& size);

    // Specifies how each line is aligned with respect to the edge
    // of the page. For a horizontal layout that runs from left to
    // right, 0 means that the left edge of the line must be aligned
    // with the left edge of the paper, and 1 means that the right
    // edge of the line must coincide with the right edge of the
    // paper. Centering will be achieved with a value of 1/2. For a
    // right to left layout, the meanings of 0 is now
    // right-alignemnt. For a vertical layout that runs from bottom
    // to top, 0 means bottom-alignment, and if it runs instead from
    // top to bottom, zero means top-alignment.
    // Any alignment value is legal, even one that lies outside the
    // interval [0;1].
    // If a layout session is already initiated, this setting will not have any effect until clear() is called and a new session is started.
    // The default alignment is zero.
    void set_alignment(double alignment);

    enum WordWrapMode {
        word_wrap_No,      ///< Do not perform word wrapping
        word_wrap_Yes,     ///< Perform word wrapping
        word_wrap_Justify  ///< Perform word wrapping and justification
    };

    // Is effectively disabled if the page size is unbounded in the primary direction.
    // The default word wrapping mode is `word_wrap_Yes`.
    void set_word_wrap_mode(WordWrapMode mode);

    // Break overlong lines into pieces. If word wrapping is also enabled, line wrapping kicks in afterwards, and breaks words that are too long to fit on a line by themselves.
    // Is effectively disabled if the page size is unbounded in the primary direction.
    // Line wrapping is disabled by default.
    void enable_line_wrapping(bool enabled);

    // Break overlong pages into several pieces.
    // Is effectively disabled if the page size is unbounded in the secondary direction.
    // Page wrapping is disabled by default.
    // FIXME: Must be constant over an entire session.
    void enable_page_wrapping(bool enabled);

    // Modify the line height (or line width for a vertical layout) by the specified factor.
    // \param factor The effective line spacing will be the normal line spacing times the specified factor.
    // The default factor is 1.
    void set_line_spacing(double factor);

    /// Extra spacing for space characters. This affects normal space
    /// (U+0020), non-breaking space (U+00A0), and ideographic space
    /// (U+3000). When disregarding justification, the final width of
    /// a space character is its normal width plus the letter spacing
    /// plus the value set with this method.
    ///
    /// The default value is 0.
    void set_word_spacing(double extra_pixels);

    // Tracking. Spacing is normal spacing plus the specified number of pixels (may be negative and may be fractional)
    // Grid fitting: Value will be rounded to nearest integer
    // The default is 0.
    void set_letter_spacing(double extra_pixels);

    // \param horizontal Set to true if the primary layout direction must be horizontal, otherwise it is vertical. The primary layout direction is the direction in which a single line is read.
    // If a layout session is already initiated, this setting will not have any effect until clear() is called and a new session is started.
    // The default layout direction has horizontal lines which are read from left to right, and has the first line at the top.
    void set_layout_direction(bool horizontal, bool left_to_right = true,
                              bool top_to_bottom = true);

    // Kerning is enabled by default.
    void enable_kerning(bool enabled);

private:
    struct WordWrapSpec { static core::EnumAssoc map[]; };

public:
    using WordWrapEnum = core::Enum<WordWrapMode, WordWrapSpec>;

protected:
    TextFormatter();

    virtual ~TextFormatter() {}

    /// Use this status of grid fitting for the next layout session.
    /// Grid fitting is enabled by default.
    void set_next_session_grid_fitting(bool enabled);

    /// Must be called by the sub-class if it wants to change the
    /// style. In that case, it must be called before the style
    /// changes are made in the sub-class. It is then guaranteed that
    /// as soon as new text is written to the formatter,
    /// acquire_style() will be called to get an updated style ID.
    void request_style_update(bool kerning_barrier)
    {
        flush_inbuf(kerning_barrier);
        m_current_style_id = 0;
    }

    /// Sub-classes must define what a style is. It is then your
    /// responsibility to ensure that the returned value uniquely
    /// identifies the current style. The value zero is reserved, and
    /// indicates absence of information (no style).
    virtual int acquire_style() = 0;

    virtual void get_style_info(int style_id, bool vertical, bool grid_fitting,
                                FontCache::FontMetrics& info) = 0;

    virtual void get_glyph_info(int style_id, bool vertical, bool grid_fitting,
                                FontCache::KernType kern, int num_chars,
                                const wchar_t* chars, FontCache::GlyphInfo* glyphs) = 0;

    struct SessionInfo {
        bool grid_fitting;
        FontCache::Direction layout_direction;
    };

    void get_session_info(SessionInfo &);

    struct TextHandler {
        /// \param glyphs A glyph index may be negative in which case
        /// no glyph should be rendered.
        virtual void handle(int style_id, int num_glyphs,
                            const int* glyphs, const float* components) = 0;
        virtual ~TextHandler() {}
    };

    // Offset is desired coordinates of lower left corner of page
    void process_page(int page_index, math::Vec2 offset, TextHandler&);

    // Both line and glyph boxes are reported in reading order. For
    // each line, the line box is reported first, then all the glyph
    // boxes, and finally the baseline. After all lines are
    // reported, a lateral pseudo baseline is reported, which marks
    // the point of text alignment between lines.
    struct StructHandler {
        virtual void line_box(const math::Vec2& pos, const math::Vec2& size) = 0;
        virtual void glyph_box(const math::Vec2& pos, const math::Vec2& size) = 0;
        // which: 0 means first baseline, 1 means other baselines, 2 means perpendicular alignment marker
        virtual void baseline(double pos, bool vertical, bool before, int which) = 0;
        virtual ~StructHandler() {}
    };

    // Same as process_page() but reports structure instead of text
    void process_page_struct(int page_index, math::Vec2 offset, StructHandler&);

private:
    math::Interval m_next_session_page_width  = {0,0}; // min, max
    math::Interval m_next_session_page_height = {0,0}; // min, max
    double m_next_session_alignment      = 0;
    double m_next_session_word_spacing   = 0;
    double m_next_session_letter_spacing = 0;
    bool m_next_session_horizontal    = true;
    bool m_next_session_left_to_right = true;
    bool m_next_session_top_to_bottom = true;
    bool m_next_session_grid_fitting  = true;

    double m_alignment; // Line stack alignment, 0 = beginning, 0.5 = center, 1 = end
    double m_line_spacing = 1; // Scaling factor for line (vertical ? width : height)
    WordWrapMode m_word_wrap = word_wrap_Yes; // Do word wrapping and optionally justification
    bool m_line_wrap = false; // Break overlong lines into pieces
    bool m_page_wrap = false; // Break overlong pages into pieces
    bool m_kerning = true; // Kerning enabled

    // Settings that must remain constant over an entire layout session
    bool m_grid_fitting; // Integer pixel layout
    bool m_vertical;    // Baselines are vertical
    double m_min_minor; // vertical ? height :  width (strictly positive or unbounded)
    double m_min_major; // vertical ? width  : height (strictly positive or unbounded)
    double m_max_minor; // vertical ? height :  width (strictly positive or unbounded)
    double m_max_major; // vertical ? width  : height (strictly positive or unbounded)
    bool m_rev_minor;   // vertical ? !bottom_to_top :  right_to_left
    bool m_rev_major;   // vertical ?  right_to_left : !bottom_to_top

//    double m_major_advance; // Signed baseline spacing
//    double m_major_offset;  // Signed offset of baseline from "start" of line
//    double m_std_space_advance; // Advance value for space character as reported by style. Never grid fitted.
    double m_word_spacing;   // Number of extra pixels to add to the width of space characters. Is integer if session is grid fitting.
    double m_letter_spacing; // Number of extra pixels between characters. Is integer if session is grid fitting.

    static constexpr int s_inbuf_size = 128; // Must not be less than 2
    wchar_t m_inbuf[s_inbuf_size];
    wchar_t* const m_inbuf_end = m_inbuf + s_inbuf_size;
    wchar_t* m_inbuf_pos;
    bool m_ignore_inbuf_front;
    FontCache::GlyphInfo m_glyph_info[s_inbuf_size];

    bool m_in_session; // True iff the current layout is non-empty

    int m_current_style_id; // Identifier for the currently selected style, or zero if none are currently selected.
    math::Interval m_style_lateral_span;

    bool m_have_space_advance;
    double m_space_advance; // Baseline advance of space character in pixels. Is integer if session is grid fitting.

    // A chunk 'c' consumes 'c.num_glyphs' elements from 'TextFormatter::m_glyph_indices' and the same number of elements from 'TextFormatter::m_advance_comps'.
    // FIXME: Combine TextFormatter::m_glyph_indices and TextFormatter::m_advance_comps into a single vector. Might be unfortunate due to nature of process_page().
    struct Chunk {
        long num_glyphs; // The number of glyphs in this chunk.
        int style_id;
        Chunk(int s): num_glyphs(0), style_id(s) {}
    };

    // A line 'l' consumes 'l.num_glyphs' elements from 'TextFormatter::m_glyph_indices' and the same number of elements from 'TextFormatter::m_advance_comps'.
    // FIXME: Change lateral_pos to lateral_advance!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // FIXME: Change lateral_pos to lateral_advance!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // FIXME: Change lateral_pos to lateral_advance!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // FIXME: Change lateral_pos to lateral_advance!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // FIXME: Change lateral_pos to lateral_advance!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // FIXME: Change lateral_pos to lateral_advance!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // FIXME: Change lateral_pos to lateral_advance!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // FIXME: Change lateral_pos to lateral_advance!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // FIXME: Change lateral_pos to lateral_advance!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // FIXME: Change lateral_pos to lateral_advance!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // FIXME: Change lateral_pos to lateral_advance!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // FIXME: Change lateral_pos to lateral_advance!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // FIXME: Change lateral_pos to lateral_advance!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // FIXME: Change lateral_pos to lateral_advance!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // FIXME: Change lateral_pos to lateral_advance!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // FIXME: Change lateral_pos to lateral_advance!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // FIXME: Change lateral_pos to lateral_advance!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // FIXME: Change lateral_pos to lateral_advance!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    struct Line {
        int num_glyphs = 0; // The number of glyphs on this line.
        double length = 0; // Length along baseline, never negative. Start of line is indicated by the advance of the first glyph.
        math::Interval lateral_span = {0,0}; // Positions of (vertical ? (left and right) : (bottom and top)) edges of line relative to baseline.
        double lateral_pos; // Lateral position of leading edge of line box.
        Line(double p): lateral_pos{p} {}
        double get_lateral_trail_pos() const { return lateral_pos + lateral_span.get_length(); }
    };

    // A page 'p' consumes 'p.num_lines' elements from 'TextFormatter::m_lines'.
    struct Page {
        int num_lines = 0;  // The number of lines on this page.
        long first_glyph; // Index in TextFormatter::m_glyph_indices and TextFormatter::m_advance_comps of the first glyph on this page.
        int first_line;  // Index in TextFormatter::m_lines of the first line on this page.
        int first_chunk; // Index in TextFormatter::m_chunks of the first chunk that intersects this page.
        long first_glyph_in_chunk; // Number of glyphs of first_chunk that belong to pages that preceed this one.
        Page(long g, int l, int c, long i):
            first_glyph(g), first_line(l), first_chunk(c), first_glyph_in_chunk(i) {}
        double get_length(const TextFormatter* f) const
        {
            return (0 < num_lines ?
                    f->m_lines[first_line + num_lines - 1].get_lateral_trail_pos() : 0);
        }
        // Move start of page back by the specified number of glyphs. Must not
        // go behind the start of the preceeding line.
        void move_back_by_glyphs(const TextFormatter* f, int num_glyphs)
        {
            first_glyph -= num_glyphs;
            while (first_glyph_in_chunk < num_glyphs) {
                num_glyphs -= first_glyph_in_chunk;
                first_glyph_in_chunk = f->m_chunks[--first_chunk].num_glyphs;
            }
            first_glyph_in_chunk -= num_glyphs;
        }
        // Move start of this page back by one line.
        void move_back_one_line(const TextFormatter* f, Page* previous_page)
        {
            --previous_page->num_lines;
            ++num_lines;
            move_back_by_glyphs(f, f->m_lines[--first_line].num_glyphs);
        }
    };

    std::vector<Page>   m_pages;
    std::vector<Line>   m_lines;
    std::vector<Chunk>  m_chunks;
    std::vector<int>    m_glyph_indices; // A negative index means 'no glyph'.
    std::vector<double> m_advance_comps; // Components of position advances. For a particular glyph, it expresses the distance from the cursor position of the previous glyph, to the cursor position of this glyph.

    double m_longest_complete_line;

    bool m_empty_line; // Only valid when m_current_line != nullptr
    double m_last_minor_pos; // Position of last glyph on current line. Only valid when m_empty_line == false.
    double m_last_minor_advance; // Advance/width of last glyph on current line. Only valid when m_empty_line == false.
    Page* m_current_page;
    Line* m_current_line;
    Chunk* m_current_chunk;

    long m_word_start_index; // Index within m_advance_comps of first glyph of last word on current line. Less than 0 until a space is seen on the current line. Only valid when m_current_line != nullptr
    double m_word_sep_pos; // Position in primary direction of separator before last word on current line
    double m_line_length_snapshot; // Length of line ending at glyph that preceeds the word separator that caused this snapshot
    math::Interval m_lateral_span_snapshot; // Lateral span of line ending with the word separator that caused this snapshot
    math::Interval m_word_lateral_span; // Lateral span of current word
    std::vector<long> m_word_separators; // Indices within m_advance_comps of white-spaces of current line. This includes non-breaking spaces.
    long m_num_word_separators; // Number of word separators preceeding the one that delimits the last word on the current line

/*
    int m_first_line_glyph; // Index into glyph list of first glyph on current line
    std::vector<int> m_words; // One entry for each complete word on current line, each entry is the number of consecutive glyphs in the glyph list that are consumed by the word

    int m_line_length; // Distance in pixels along the baseline from the bearing point of first glyph to the bearing point of last glyph plus the advance value of last glyph.
*/

    // A space terminates current word and causes a new current word to be initiated.
    // The space is not added immediately to the line not the word.
    // If the word
/*
    double m_word_length; // Length of current word
    double m_cursor; // Position of cursor along baseline
    Line* m_current_line;
    Page* m_current_page;
// No need      math::Box2 line_bbox; // Bounding box of current line
// No need      math::Box2 page_bbox; // Bounding box of text on current page
    Pages m_pages;
*/

    void reset();

    void flush_inbuf(bool kerning_barrier);

//    void flush_word();

    void update_space_advance();

    void open_line();
    void close_line();
    void open_page();
    void close_page();

    void do_word_wrap();

    void adjust_lateral_line_span(const math::Interval& span, bool include);
};




// Implementation

inline void TextFormatter::write(std::wstring s)
{
    write(s.data(), s.size());
}

inline void TextFormatter::writeln(std::wstring s)
{
    write(s);
    write(L"\n");
}

inline void TextFormatter::clear()
{
    reset();
}

inline void TextFormatter::write(const wchar_t* text, std::size_t n)
{
    while (n) {
        std::size_t m = m_inbuf_end - m_inbuf_pos;
        bool not_full = n < m;
        m_inbuf_pos = std::copy(text, text+(not_full?n:m), m_inbuf_pos);
        if (not_full)
            return;
        flush_inbuf(false);
        text += m;
        n -= m;
    }
}

inline int TextFormatter::get_num_pages()
{
    flush_inbuf(false);
    return m_pages.size();
}

inline void TextFormatter::set_page_width(const math::Interval& width)
{
    flush_inbuf(false);
    m_next_session_page_width = width;
}

inline void TextFormatter::set_page_height(const math::Interval& height)
{
    flush_inbuf(false);
    m_next_session_page_height = height;
}

inline void TextFormatter::set_page_size(const math::Vec2& size)
{
    flush_inbuf(false);
    m_next_session_page_width.set(size[0], size[0]);
    m_next_session_page_height.set(size[1], size[1]);
}

inline void TextFormatter::set_alignment(double v)
{
    flush_inbuf(false);
    m_next_session_alignment = v;
}

inline void TextFormatter::set_word_wrap_mode(WordWrapMode mode)
{
    flush_inbuf(false);
    m_word_wrap = mode;
}

inline void TextFormatter::enable_line_wrapping(bool enabled)
{
    flush_inbuf(false);
    m_line_wrap = enabled;
}

inline void TextFormatter::enable_page_wrapping(bool enabled)
{
    flush_inbuf(false);
    m_page_wrap = enabled;
}

inline void TextFormatter::set_line_spacing(double factor)
{
    flush_inbuf(false);
    m_line_spacing = factor;
    m_current_style_id = 0; // Request new style
}

inline void TextFormatter::set_word_spacing(double extra_pixels)
{
    flush_inbuf(false);
    m_next_session_word_spacing = extra_pixels;
    if (m_in_session)
        m_word_spacing = (m_grid_fitting ? std::round(extra_pixels) : extra_pixels);
}

inline void TextFormatter::set_letter_spacing(double extra_pixels)
{
    flush_inbuf(false);
    m_next_session_letter_spacing = extra_pixels;
    if (m_in_session)
        m_letter_spacing = (m_grid_fitting ? std::round(extra_pixels) : extra_pixels);
}

inline void TextFormatter::set_layout_direction(bool horizontal, bool l_to_r, bool t_to_b)
{
    flush_inbuf(false);
    m_next_session_horizontal = horizontal;
    m_next_session_left_to_right = l_to_r;
    m_next_session_top_to_bottom = t_to_b;
}

inline void TextFormatter::enable_kerning(bool enabled)
{
    flush_inbuf(false);
    m_kerning = enabled;
}

inline void TextFormatter::set_next_session_grid_fitting(bool enabled)
{
    flush_inbuf(false);
    m_next_session_grid_fitting = enabled;
}

inline void TextFormatter::get_session_info(SessionInfo& info)
{
    flush_inbuf(true);
    info.grid_fitting     = m_grid_fitting;
    info.layout_direction = m_vertical ?
        m_rev_minor ? FontCache::dir_TopToBottom : FontCache::dir_BottomToTop :
        m_rev_minor ? FontCache::dir_RightToLeft : FontCache::dir_LeftToRight;
}

inline void TextFormatter::close_line()
{
    if (!m_current_line)
        return;
    if (m_longest_complete_line < m_current_line->length)
        m_longest_complete_line = m_current_line->length;
    m_current_line = nullptr;
}

inline void TextFormatter::close_page()
{
    if (!m_current_page)
        return;
    close_line();
    m_current_page = nullptr;
}

} // namespace font
} // namespace archon

#endif // ARCHON_FONT_TEXT_FORMAT_HPP
