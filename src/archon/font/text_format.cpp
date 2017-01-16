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


/*

To be tested:

FIXME: When line spacing is less than 1, structure rendering fails.


To do:

  When grid fitting is enabled and the rendering size is small, the glyphs may grow several pixels in either direction. This fact is not reflected in the calculated line height, and that fact often results in overlapping lines. A simple opportunistic improvement would be to retrieve a few glyphs known to be high and adjust the line height accordingly. A more advanced solution would be to gradually increasing the line height as more and more glyph heights become known, however that would require the glyphs to be loaded during the formatting step. Doing both seems like the optimal solution.


Ideas for the future:

  Work with fixed point fractionals in layout routine.

  Generally have all params that control word spacing be in fractional pixels even when grid fitting mode is enabled

  Glyph caching will be essential for reasonable speed.


Representation of arbitrary glyph layout:

      struct Chunk
      {
        int font_id;
        int num_glyphs;
        FontCache::BearingType bearing_type : numeric_limits<unsigned char>::digits;
        FontCache::CoordType   coord_type   : numeric_limits<unsigned char>::digits;
      };

      struct Layout
      {
        int num_chunks;    ///< Number of glyphs in this layout.
        Chunk *chunks;     ///< 
        int   *glyphs;     ///< Glyph indices.
        float *components; ///< Glyph position components.
      };


Relation to Gecko text block rendering:

In Gecko, letter spacing is added to the right of the last glyph on
each line, meaning that when the line is right aligned, the glyph is
aligned with the right edge of the block. In the Archon formatter,
this is not the case.

 */

#include <cmath>
#include <functional>

#include <archon/core/numeric.hpp>
#include <archon/core/memory.hpp>
#include <archon/math/functions.hpp>
#include <archon/font/text_format.hpp>

using namespace archon::core;
using namespace archon::math;
using namespace archon::font;


namespace archon {
namespace font {

math::Vec2 TextFormatter::get_page_size(int page_index)
{
    flush_inbuf(false);
    const Page& page = m_pages.at(page_index);
    double w = m_longest_complete_line;
    double h = page.get_length(this);
    if (m_current_line && w < m_current_line->length)
        w = m_current_line->length;
//    if (m_current_line && &page == m_current_page && w < m_current_line->length)
//        w = m_current_line->length;
    if (w < m_min_minor)
        w = m_min_minor;
    if (h < m_min_major)
        h = m_min_major;
    if (0 < m_max_minor && m_max_minor < w)
        w = m_max_minor;
    if (0 < m_max_major && m_max_major < h)
        h = m_max_major;
    return (m_vertical ? Vec2{h,w} : Vec2{w,h});
}


TextFormatter::TextFormatter()
{
    reset();
}


void TextFormatter::process_page(int page_index, Vec2 offset, TextHandler& proc)
{
    int major_coord = (m_vertical ? 0 : 1);
    int minor_coord = (m_vertical ? 1 : 0);
    Vec2 page_size = get_page_size(page_index); // Also flushes inbuf
    double page_major = page_size[major_coord];
    double page_minor = page_size[minor_coord];
    const int max_glyphs_per_call = 256;
    int glyphs[max_glyphs_per_call];
    float components[max_glyphs_per_call + 1];
    const Page& page = m_pages[page_index];
    auto index   = m_glyph_indices.cbegin() + page.first_glyph;
    auto advance = m_advance_comps.cbegin() + page.first_glyph;
    auto chunk   =        m_chunks.cbegin() + page.first_chunk;
    long left_in_chunk = chunk->num_glyphs - page.first_glyph_in_chunk;
    int line_end = page.first_line + page.num_lines;
    for (int i = page.first_line; i < line_end; ++i) {
        const Line& line = m_lines[i];
        double lateral_pos = offset[major_coord];
        {
            double offset = line.lateral_pos - line.lateral_span.begin;
            if (m_rev_major) {
                lateral_pos += page_major - offset;
            }
            else {
                lateral_pos += offset;
            }
        }
        double line_pos = offset[minor_coord];
        double align = lin_interp(m_alignment, 0, 1, 0.0, page_minor - line.length);
        if (m_grid_fitting)
            align = std::round(align);
        line_pos += (m_rev_minor ? page_minor - align : align);

        int left_on_line = line.num_glyphs;
        do {
            int n = std::min(left_on_line, int(std::min(left_in_chunk, long(max_glyphs_per_call))));
            auto index_end = index + n;
            std::copy(index, index_end, glyphs);
            components[0] = lateral_pos;
            auto advance_end = advance + n;
            double new_pos = m_rev_minor ?
                partial_sum_alt(advance, advance_end, components+1, std::minus<double>(), line_pos) :
                partial_sum_alt(advance, advance_end, components+1, std::plus<double>(),  line_pos);

            proc.handle(chunk->style_id, n, glyphs, components);

            line_pos = new_pos;
            index    = index_end;
            advance  = advance_end;
            left_on_line -= n;
            left_in_chunk -= n;
            if (left_in_chunk == 0) {
                ++chunk;
                left_in_chunk = chunk->num_glyphs;
            }
        }
        while (left_on_line);
    }
}


void TextFormatter::process_page_struct(int page_index, Vec2 offset, StructHandler& proc)
{
    int major_coord = (m_vertical ? 0 : 1);
    int minor_coord = (m_vertical ? 1 : 0);
    Vec2 page_size = get_page_size(page_index); // Also flushes inbuf
    double page_major = page_size[major_coord];
    double page_minor = page_size[minor_coord];
    const Page& page = m_pages[page_index];
    long glyph_index = page.first_glyph;
    auto chunk = m_chunks.cbegin() + page.first_chunk;
    auto chunks_end = m_chunks.cend();
    long left_in_chunk = chunk->num_glyphs - page.first_glyph_in_chunk;
    FontCache::FontMetrics info;
    get_style_info(chunk->style_id, m_vertical, m_grid_fitting, info);
    int line_end = page.first_line + page.num_lines;
    for (int i = page.first_line; i < line_end; ++i) {
        const Line& line = m_lines[i];
        double line_y = offset[major_coord];
        {
            double offset = line.lateral_pos - line.lateral_span.begin;
            line_y += m_rev_major ? page_major - offset : offset;
        }
        double line_x = offset[minor_coord];
        double align = lin_interp(m_alignment, 0, 1, 0.0, page_minor - line.length);
        if (m_grid_fitting)
            align = std::round(align);
        line_x += m_rev_minor ? page_minor - (align+line.length) : align;

        {
            double q = m_rev_major ? -line.lateral_span.end : line.lateral_span.begin;
            Vec2 p{line_x, line_y + q}, s{line.length, line.lateral_span.get_length()};
            if (m_vertical) {
                std::swap(p[0], p[1]);
                std::swap(s[0], s[1]);
            }
            proc.line_box(p,s);
        }

        double glyph_offset = 0;
        int left_on_line = line.num_glyphs;
        do {
            int n = std::min(long(left_on_line), left_in_chunk);
            double y = line_y + info.lateral_span.begin, h = info.lateral_span.get_length();

            for (int j=0; j<n; ++j, ++glyph_index) {
                glyph_offset += m_advance_comps[glyph_index];
                bool last_on_line  = j+1 == left_on_line;
                double w;
                if (last_on_line) {
                    w = line.length - glyph_offset;
                    if (m_glyph_indices[glyph_index] < 0 && w <= 0)
                        w = page_minor - (align+glyph_offset);
                }
                else {
                    w = m_advance_comps[glyph_index+1];
                }
                if (w < 0)
                    w = 0;
                double x = line_x + (m_rev_minor ? line.length - glyph_offset - w : glyph_offset);
                Vec2 p{x,y}, s{w,h};
                if (m_vertical) {
                    std::swap(p[0], p[1]);
                    std::swap(s[0], s[1]);
                }
                proc.glyph_box(p,s);
            }

            left_on_line -= n;
            left_in_chunk -= n;
            if (left_in_chunk == 0) {
                if (++chunk != chunks_end) {
                    left_in_chunk = chunk->num_glyphs;
                    get_style_info(chunk->style_id, m_vertical, m_grid_fitting, info);
                }
            }
        }
        while (left_on_line);

        {
            double d = line.lateral_span.begin + line.lateral_span.end;
            proc.baseline(line_y, m_vertical, (m_rev_major ? d <= 0 : 0 <= d), (i == 0 ? 0 : 1));
        }
    }

    {
        double a = offset[minor_coord], b = a + page_minor;
        if (m_rev_minor)
            std::swap(a,b);
        bool before = (m_rev_minor ? (0.5 <= m_alignment) : (m_alignment <= 0.5));
        proc.baseline(lin_interp(m_alignment, 0, 1, a, b), !m_vertical, before, 2);
    }
}


void TextFormatter::reset()
{
    m_current_page  = nullptr;
    m_current_line  = nullptr;
    m_current_chunk = nullptr;
    clear_vector(m_pages, 1, 1);
    clear_vector(m_lines, 1, 8);
    clear_vector(m_chunks, 8, 32);
    clear_vector(m_glyph_indices, 256, 1024);
    clear_vector(m_advance_comps, 256, 1024);
    clear_vector(m_word_separators, 1, 8);

    m_longest_complete_line = 0;

    m_current_style_id = 0; // Need to reacquire style metrics, since the direction can have changed in the next session.
    m_in_session = false;

    m_inbuf_pos = m_inbuf;
    m_ignore_inbuf_front = false;
}



// It is important that the currently buffered input is processed
// before changing most of the formatter parameters, such that the
// changes can can be applied starting at the intended position in the
// text.
//
// Set kerning_barrier to true if the input is flushed due to a change
// that would cause a kerning adjustment of the next glyph to be
// meaningless. One example is a change in font face.
//
// FIXME: We can probably do away with the kerning_barrier and simply
// assume that it is never true. But only if there is another way that
// we can forcefully nullify the kerning adjustment when the style
// changes.
//
// FIXME: LINE AND WORD WRAPPING MODES MAY PROBABLY NOT CHANGE DURING
// THE CONSTRUCTION OF THE LINE. MAYBE THE RELEVANT FLAGS SHOULD BE
// SNAPSHOTTED AT EACH OPENING OF A NEW LINE. NO - THIS IDEA IS NOT
// NECESSARY AFTER ALL. INSTEAD IT WILL SUFICE TO KEEP A SNAPSHOT OF
// THE LINE LATERAL SPAN AND UPDATE IT ON EVERY SPACE CHARACTER. ALSO
// A WORD LATERAL SPAN NEEDS TO BE MAINTAINED IN PARELLEL, AND USED
// WHEN THE WORD IS PUSHED TO NEXT LINE. WHEN THAT HAPPENS, THE
// REMAINING LINE MUST BE CLOSED, AND ITS LATERAL SPAN REVERTED TO THE
// LAST SNAPSHOT, WHICH MAY CAUSE IT TO SNAP BACK TO THE PREVIOUS
// PAGE. IN THAT CASE THE NEW LINE MAY OR MAY NOT ALSO FIT ON THE
// PREVIOUS PAGE. IF IT DOES, THE LAST PAGE MUST BE DELETED. SO UNDER
// EXTREEME CIRCUMSTANCES, THE ADDITION OF A GLYPH MAY ACTUALLY REDUCE
// THE NUMBER OP PAGES BY ONE.
void TextFormatter::flush_inbuf(bool kerning_barrier)
{
    int inbuf_num = m_inbuf_pos - m_inbuf;
    int num_glyphs = (m_ignore_inbuf_front ? inbuf_num-1 : inbuf_num);
    if (num_glyphs < 1)
        return;

    // Start a session if one has not already been started
    if (!m_in_session) {
        m_vertical = !m_next_session_horizontal;

        m_min_minor = m_next_session_page_width.begin;
        m_min_major = m_next_session_page_height.begin;
        m_max_minor = m_next_session_page_width.end;
        m_max_major = m_next_session_page_height.end;
        if (m_vertical) {
            std::swap(m_min_minor, m_min_major);
            std::swap(m_max_minor, m_max_major);
        }

        m_alignment = m_next_session_alignment;

        m_rev_minor = !m_next_session_left_to_right;
        m_rev_major =  m_next_session_top_to_bottom;
        if (m_vertical)
            std::swap(m_rev_minor, m_rev_major);

        m_word_spacing   = m_next_session_word_spacing;
        m_letter_spacing = m_next_session_letter_spacing;

        m_grid_fitting = m_next_session_grid_fitting;
        if (m_grid_fitting) {
            m_min_minor = std::ceil(m_min_minor);
            m_min_major = std::ceil(m_min_major);
            m_max_minor = std::floor(m_max_minor);
            m_max_major = std::floor(m_max_major);
            m_word_spacing   = std::round(m_word_spacing);
            m_letter_spacing = std::round(m_letter_spacing);
        }

        m_in_session = true;
    }


    // Acquire a style if one has not already been acquired
    // FIXME: Indicate 'request for style refresh' in a different way such that it can be discovered that the new style is the same as the old one.
    // FIXME: We don't seem to need both a m_current_style_id and a m_current_chunk. m_current_chunk might be enough. But see above.
    if (m_current_style_id == 0) {
        int style_id = acquire_style();

        FontCache::FontMetrics info;
        get_style_info(style_id, m_vertical, false, info);
        m_style_lateral_span = info.lateral_span;
        // FIXME: Negative values of m_line_spacing gives unexpected results.
        double a = m_style_lateral_span.get_length(), b = m_line_spacing * a;
        m_style_lateral_span.begin -= (b-a) / 2;
        m_style_lateral_span.end = m_style_lateral_span.begin + b;
        if (m_grid_fitting) {
            // FIXME: In fact the spans do not have to be grid-fitted,
            // instead the baseline advances should be rounded
            // upwards.
            m_style_lateral_span.begin = std::floor(m_style_lateral_span.begin);
            m_style_lateral_span.end   =  std::ceil(m_style_lateral_span.end);
        }
        if (m_rev_major)
            m_style_lateral_span.reflect();

        m_current_style_id = style_id;
        m_have_space_advance = false;
        if (m_current_chunk && m_current_chunk->num_glyphs == 0) {
            m_current_chunk->style_id = m_current_style_id;
        }
        else {
            m_chunks.push_back(Chunk(m_current_style_id));
            m_current_chunk = &m_chunks.back();
        }

        if (m_current_line) {
            if (0 <= m_word_start_index) {
                if (m_word_start_index == long(m_glyph_indices.size())) {
                    m_word_lateral_span = m_style_lateral_span;
                }
                else {
                    m_word_lateral_span.include(m_style_lateral_span);
                }
            }
            adjust_lateral_line_span(m_style_lateral_span, true);
        }
    }


    get_glyph_info(m_current_style_id, m_vertical, m_grid_fitting,
                   (m_kerning ? (m_rev_minor ? FontCache::kern_Dec : FontCache::kern_Inc) :
                    FontCache::kern_No), inbuf_num, m_inbuf, m_glyph_info);

// Strategy for exception safety is to wrap most inside a try-catch and restore the output vector sizes on error, and maybe also the input consumption state. No - no can do, see write(). More considereation is needed.

    const wchar_t* next_char = m_inbuf;
    const FontCache::GlyphInfo* info = m_glyph_info;
    if (m_ignore_inbuf_front) {
        ++next_char;
        ++info;
    }
    const FontCache::GlyphInfo* info_end = info + num_glyphs;

    m_glyph_indices.reserve(m_glyph_indices.size() + num_glyphs);
    m_advance_comps.reserve(m_advance_comps.size() + num_glyphs + 1);

    bool word_wrap_enabled = (0 < m_max_minor && m_word_wrap != word_wrap_No);
    bool line_wrap_enabled = (0 < m_max_minor && m_line_wrap);
    bool wrap = (word_wrap_enabled || line_wrap_enabled);

    do {
        // Provide a current page and line
        open_line(); // May throw

        bool break_out = false;
        do {
            int glyph = info->index;
            double pre_advance = m_last_minor_advance;
            double post_advance = info->advance;

            enum BreakLevel { brk_None, brk_Word, brk_Line, brk_Page } brk = brk_None;
            bool is_vari_space = false;
            bool is_tab = false;
            switch (*next_char) {
                case L' ':      // Ordinary space
                case L'\x3000': // Ideographic space
                    if (word_wrap_enabled)
                        brk = brk_Word;
                    // Fall-through
                case L'\xA0':   // Non-breaking space
                    glyph = -1; // No glyph for white space
                    post_advance += m_word_spacing;
                    if (m_word_wrap == word_wrap_Justify)
                        is_vari_space = true;
                    break;

                case L'\t':     // Tabulation
                    is_tab = true;
                    break;

                case L'\n':     // New line
                    brk = brk_Line;
                    glyph = -1;
                    post_advance = 0;
                    break;

                case L'\f':     // Form feed (page break)
                    brk = brk_Page;
                    glyph = -1;
                    post_advance = 0;
                    break;

                default:
                    break;
            }

            if (m_empty_line) {
                m_last_minor_pos = 0;
                pre_advance = 0;
            }
            else if (0 < post_advance) {
                if (m_kerning)
                    pre_advance += info->kerning;
                pre_advance += m_letter_spacing;
                if (pre_advance < 0)
                    pre_advance = 0;
            }

            double glyph_pos = m_last_minor_pos + pre_advance;

            if (is_tab) {
                glyph = -1; // No glyph for white space
                if (!m_have_space_advance)
                    update_space_advance();
                double tab_stop_spacing = 8 * m_space_advance;
                double next_glyph_pos = glyph_pos + m_space_advance + m_word_spacing;
                double num_tab_stops = ceil(next_glyph_pos / tab_stop_spacing);
                double tab_stop_pos = num_tab_stops * tab_stop_spacing;
                post_advance = tab_stop_pos - glyph_pos;
            }

            double line_length = glyph_pos + post_advance;

            // Check for line overflow
            if (wrap && brk == brk_None && m_max_minor < line_length) {
                if (word_wrap_enabled) {
                    if (0 <= m_word_start_index) {
                        do_word_wrap(); // Word wrapping
                        break;
                    }
                    if (line_wrap_enabled && !m_empty_line) {
                        close_line(); // Simple line wrapping on top of word wrapping
                        break;
                    }
                }
                else if (!m_empty_line) {
                    close_line(); // Simple line wrapping
                    break;
                }
            }

            if (is_vari_space)
                m_word_separators.push_back(m_glyph_indices.size());
            m_glyph_indices.push_back(glyph);
            m_advance_comps.push_back(pre_advance);
            // FIXME: Maybe these can be added in chunks.
            ++m_current_chunk->num_glyphs;
            ++m_current_line->num_glyphs;

            ++info;
            ++next_char;

            double prev_line_length = m_current_line->length;

            m_last_minor_advance   = post_advance;
            m_last_minor_pos       = glyph_pos;
            m_empty_line           = false;
            m_current_line->length = line_length;

            switch (brk) {
                case brk_None:
                    break;
                case brk_Word:
                    m_line_length_snapshot  = prev_line_length;
                    m_lateral_span_snapshot = m_current_line->lateral_span;
                    m_word_sep_pos = glyph_pos;
                    m_word_start_index = m_glyph_indices.size();
                    if (m_word_wrap == word_wrap_Justify)
                        m_num_word_separators = m_word_separators.size() - 1;
                    m_word_lateral_span = m_style_lateral_span;
                    if (m_max_minor < line_length) {
                        do_word_wrap();
                        break_out = true;
                    }
                    break;
                case brk_Line:
                    close_line();
                    break_out = true;
                    break;
                case brk_Page:
                    close_page();
                    break_out = true;
                    break;
            }

            if (break_out)
                break;
        }
        while (info < info_end);
    }
    while (info < info_end);


    if (kerning_barrier) {
        m_inbuf_pos = m_inbuf;
        m_ignore_inbuf_front = false;
    }
    else {
        // Make last character available for kerning consideration during next flush
        m_inbuf[0] = m_inbuf[inbuf_num - 1];
        m_inbuf_pos = m_inbuf + 1;
        m_ignore_inbuf_front = true;
    }
}


void TextFormatter::update_space_advance()
{
    wchar_t space = L' ';
    FontCache::GlyphInfo glyph_info;
    get_glyph_info(m_current_style_id, m_vertical, m_grid_fitting,
                   FontCache::kern_No, 1, &space, &glyph_info);
    m_space_advance = glyph_info.advance;
    m_have_space_advance = true;
}


void TextFormatter::open_line()
{
    if (m_current_line)
        return;

    // Check for page overflow
    double pos;
    if (m_current_page) {
        pos = m_current_page->get_length(this);
        if (m_page_wrap && 0 < m_max_major && 0 < m_current_page->num_lines &&
            m_max_major < pos + m_style_lateral_span.get_length())
        {
            close_page();
            pos = 0;
        }
    }
    else {
        pos = 0;
    }

    // Open new page if required
    m_lines.reserve(m_lines.size()+1);
    open_page();

    // Open new line
    m_lines.push_back(Line(pos));
    m_current_line = &m_lines.back();
    m_current_line->lateral_span = m_style_lateral_span;
    ++m_current_page->num_lines;
    m_empty_line = true;
    m_word_start_index = -1;
    m_word_separators.clear();
}


void TextFormatter::open_page()
{
    if (m_current_page)
        return;

    m_pages.push_back(Page(m_glyph_indices.size(), m_lines.size(),
                           m_chunks.size()-1, m_chunks.back().num_glyphs));
    m_current_page = &m_pages.back();
}


void TextFormatter::do_word_wrap()
{
    m_longest_complete_line = m_max_minor;
    m_advance_comps[m_word_start_index-1] -= m_word_sep_pos - m_line_length_snapshot;
    m_current_line->length = m_line_length_snapshot;
    if (m_word_wrap == word_wrap_Justify && 0 < m_num_word_separators) {
        double diff = m_max_minor - m_current_line->length;
        if (m_grid_fitting)
            diff = std::round(diff);
        double f = diff / m_num_word_separators;
        double prev = 0;
        for (long i = 0; i < m_num_word_separators; ++i) {
            double next = f * (i+1);
            if (m_grid_fitting)
                next = std::round(next);
            double adjust = next - prev;
            m_advance_comps[m_word_separators[i]+1] += adjust;
            prev = next;
        }
        m_current_line->length = m_max_minor;
    }

    adjust_lateral_line_span(m_lateral_span_snapshot, false);
    close_line();

    int n = m_glyph_indices.size() - m_word_start_index;
    if (0 < n) {
        m_last_minor_pos -= m_word_sep_pos + m_advance_comps[m_word_start_index];
        m_advance_comps[m_word_start_index] = 0;
        open_line();
        Line* previous_line = &*(m_lines.end()-2);
        if (m_current_page->num_lines < 2)
            m_current_page->move_back_by_glyphs(this, n);
        previous_line->num_glyphs -= n;
        m_current_line->num_glyphs += n;
        m_empty_line = false;
        m_current_line->length = m_last_minor_pos + m_last_minor_advance;
        m_current_line->lateral_span = m_word_lateral_span;
    }
}


void TextFormatter::adjust_lateral_line_span(const math::Interval& span, bool include)
{
    bool wrap = m_page_wrap && 0 < m_max_major;
    int way = 0; // -1 for decrease, 1 for increase, 0 for same height.
    if (include && 0 < m_current_line->num_glyphs) {
        double old;
        if (wrap)
            old = m_current_line->lateral_span.get_length();
        m_current_line->lateral_span.include(span);
        if (wrap)
            way = (old < m_current_line->lateral_span.get_length() ? 1 : 0);
    }
    else {
        if (wrap) {
            double l_1 = m_current_line->lateral_span.get_length();
            double l_2 = span.get_length();
            way = (l_1 < l_2 ? 1 : (l_2 < l_1 ? -1 : 0));
        }
        m_current_line->lateral_span = span;
    }

    if (wrap) {
        if (way < 0) {
            // Check for page underflow
            if (m_current_page->num_lines < 2) {
                // WHOOPS: This should also apply for line breaking without word
                // breaking This only applies if the page break was not "hard"
                // NOTE: If m_pages could have different widths, we would have
                // to check that the truncated current line would fit lenghtwise
                // on the previous page, and also that the untruncated line
                // would NOT fit lengthwise on the previous page, since in that
                // case the current line would not fit latterally on the
                // previous page. PROBLEM: This needs to be rechecked at every
                // extra character added to the wrapped word (including
                // non-breaking spaces)
                ARCHON_ASSERT_1(false, "Page underflow is not implemented"); // FIXME: Implement this!
            }
        }
        else if (0 < way) {
            // Check for page overflow
            if (1 < m_current_page->num_lines && m_max_major < m_current_page->get_length(this)) {
                close_page();
                open_page();
                Page& previous_page = *(m_pages.end()-2);
                // NOTE: If m_pages could have different widths, we would have
                // to reprocess the current line, because it may not fit
                // lengthwise on the next page.
                m_current_page->move_back_one_line(this, &previous_page);
                m_current_line = &m_lines.back();
                m_current_line->lateral_pos = 0;
            }
        }
    }
}


archon::core::EnumAssoc TextFormatter::WordWrapSpec::map[] =
{
    { TextFormatter::word_wrap_No,      "no"      },
    { TextFormatter::word_wrap_Yes,     "yes"     },
    { TextFormatter::word_wrap_Justify, "justify" },
    { 0, 0 }
};


} // namespace font
} // namespace archon
