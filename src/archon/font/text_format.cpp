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

using namespace std;
using namespace Archon::Core;
using namespace Archon::Math;
using namespace Archon::Font;


namespace Archon {
namespace Font {

Math::Vec2 TextFormatter::get_page_size(int page_index)
{
    flush_inbuf(false);
    const Page& page = pages.at(page_index);
    double w = longest_complete_line;
    double h = page.get_length(this);
    if (current_line && w < current_line->length)
        w = current_line->length;
//      if (current_line && &page == current_page && w < current_line->length)
//          w = current_line->length;
    if (w < min_minor)
        w = min_minor;
    if (h < min_major)
        h = min_major;
    if (0 < max_minor && max_minor < w)
        w = max_minor;
    if (0 < max_major && max_major < h)
        h = max_major;
    return vertical ? Vec2(h,w) : Vec2(w,h);
}


TextFormatter::TextFormatter():
    next_session_page_width(0,0), next_session_page_height(0,0), next_session_alignment(0),
    next_session_word_spacing(0), next_session_letter_spacing(0),
    next_session_horizontal(true), next_session_left_to_right(true),
    next_session_top_to_bottom(true), next_session_grid_fitting(true),
    line_spacing(1), word_wrap(word_wrap_Yes),
    line_wrap(false), page_wrap(false), kerning(true),
    inbuf_end(inbuf + inbuf_size)
{
    reset();
}


void TextFormatter::process_page(int page_index, Vec2 offset, TextHandler& proc)
{
    const int major_coord = vertical ? 0 : 1;
    const int minor_coord = vertical ? 1 : 0;
    const Vec2 page_size = get_page_size(page_index); // Also flushes inbuf
    const double page_major = page_size[major_coord];
    const double page_minor = page_size[minor_coord];
    const int max_glyphs_per_call = 256;
    int glyphs[max_glyphs_per_call];
    float components[max_glyphs_per_call + 1];
    const Page& page = pages[page_index];
    vector<int>::const_iterator    index   = glyph_indices.begin() + page.first_glyph;
    vector<double>::const_iterator advance = advance_comps.begin() + page.first_glyph;
    vector<Chunk>::const_iterator  chunk   = chunks.begin()        + page.first_chunk;
    long left_in_chunk = chunk->num_glyphs - page.first_glyph_in_chunk;
    const int line_end = page.first_line + page.num_lines;
    for (int i=page.first_line; i<line_end; ++i) {
        const Line& line = lines[i];
        double lateral_pos = offset[major_coord];
        {
            double offset = line.lateral_pos - line.lateral_span.begin;
            if (rev_major) {
                lateral_pos += page_major - offset;
            }
            else {
                lateral_pos += offset;
            }
        }
        double line_pos = offset[minor_coord];
        double align = lin_interp(alignment, 0, 1, 0.0, page_minor - line.length);
        if (grid_fitting)
            align = archon_round(align);
        line_pos += rev_minor ? page_minor - align : align;

        int left_on_line = line.num_glyphs;
        do {
            const int n = min(left_on_line, int(min(left_in_chunk, long(max_glyphs_per_call))));
            const vector<int>::const_iterator index_end = index + n;
            copy(index, index_end, glyphs);
            components[0] = lateral_pos;
            const vector<double>::const_iterator advance_end = advance + n;
            const double new_pos = rev_minor ?
                partial_sum_alt(advance, advance_end, components+1, minus<double>(), line_pos) :
                partial_sum_alt(advance, advance_end, components+1, plus<double>(),  line_pos);

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
    const int major_coord = vertical ? 0 : 1;
    const int minor_coord = vertical ? 1 : 0;
    const Vec2 page_size = get_page_size(page_index); // Also flushes inbuf
    const double page_major = page_size[major_coord];
    const double page_minor = page_size[minor_coord];
    const Page& page = pages[page_index];
    long glyph_index = page.first_glyph;
    vector<Chunk>::const_iterator chunk = chunks.begin() + page.first_chunk;
    const vector<Chunk>::const_iterator chunks_end = chunks.end();
    long left_in_chunk = chunk->num_glyphs - page.first_glyph_in_chunk;
    FontCache::FontMetrics info;
    get_style_info(chunk->style_id, vertical, grid_fitting, info);
    const int line_end = page.first_line + page.num_lines;
    for (int i=page.first_line; i<line_end; ++i) {
        const Line& line = lines[i];
        double line_y = offset[major_coord];
        {
            const double offset = line.lateral_pos - line.lateral_span.begin;
            line_y += rev_major ? page_major - offset : offset;
        }
        double line_x = offset[minor_coord];
        double align = lin_interp(alignment, 0, 1, 0.0, page_minor - line.length);
        if (grid_fitting)
            align = archon_round(align);
        line_x += rev_minor ? page_minor - (align+line.length) : align;

        {
            const double q = rev_major ? -line.lateral_span.end : line.lateral_span.begin;
            Vec2 p(line_x, line_y + q), s(line.length, line.lateral_span.get_length());
            if (vertical) {
                swap(p[0], p[1]);
                swap(s[0], s[1]);
            }
            proc.line_box(p,s);
        }

        double glyph_offset = 0;
        int left_on_line = line.num_glyphs;
        do {
            const int n = min(long(left_on_line), left_in_chunk);
            const double y = line_y + info.lateral_span.begin, h = info.lateral_span.get_length();

            for (int j=0; j<n; ++j, ++glyph_index) {
                glyph_offset += advance_comps[glyph_index];
                const bool last_on_line  = j+1 == left_on_line;
                double w;
                if (last_on_line) {
                    w = line.length - glyph_offset;
                    if (glyph_indices[glyph_index] < 0 && w <= 0)
                        w = page_minor - (align+glyph_offset);
                }
                else {
                    w = advance_comps[glyph_index+1];
                }
                if (w < 0)
                    w = 0;
                const double x = line_x + (rev_minor ? line.length - glyph_offset - w : glyph_offset);
                Vec2 p(x,y), s(w,h);
                if (vertical) {
                    swap(p[0], p[1]);
                    swap(s[0], s[1]);
                }
                proc.glyph_box(p,s);
            }

            left_on_line -= n;
            left_in_chunk -= n;
            if (left_in_chunk == 0) {
                if (++chunk != chunks_end) {
                    left_in_chunk = chunk->num_glyphs;
                    get_style_info(chunk->style_id, vertical, grid_fitting, info);
                }
            }
        }
        while (left_on_line);

        {
            const double d = line.lateral_span.begin + line.lateral_span.end;
            proc.baseline(line_y, vertical, rev_major ? d <= 0 : 0 <= d, i == 0 ? 0 : 1);
        }
    }

    {
        double a = offset[minor_coord], b = a + page_minor;
        if (rev_minor)
            swap(a,b);
        const double before = rev_minor ? 0.5 <= alignment : alignment <= 0.5;
        proc.baseline(lin_interp(alignment, 0, 1, a, b), !vertical, before, 2);
    }
}


void TextFormatter::reset()
{
    current_page = 0;
    current_line = 0;
    current_chunk = 0;
    clear_vector(pages, 1, 1);
    clear_vector(lines, 1, 8);
    clear_vector(chunks, 8, 32);
    clear_vector(glyph_indices, 256, 1024);
    clear_vector(advance_comps, 256, 1024);
    clear_vector(word_separators, 1, 8);

    longest_complete_line = 0;

    current_style_id = 0; // Need to reacquire style metrics, since the direction can have changed in the next session.
    in_session = false;

    inbuf_pos = inbuf;
    ignore_inbuf_front = false;
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
    const int inbuf_num = inbuf_pos - inbuf;
    const int num_glyphs = ignore_inbuf_front ? inbuf_num-1 : inbuf_num;
    if (num_glyphs < 1)
        return;

    // Start a session if one has not already been started
    if (!in_session) {
        vertical = !next_session_horizontal;

        min_minor = next_session_page_width.begin;
        min_major = next_session_page_height.begin;
        max_minor = next_session_page_width.end;
        max_major = next_session_page_height.end;
        if (vertical) {
            swap(min_minor, min_major);
            swap(max_minor, max_major);
        }

        alignment = next_session_alignment;

        rev_minor = !next_session_left_to_right;
        rev_major =  next_session_top_to_bottom;
        if (vertical)
            swap(rev_minor, rev_major);

        word_spacing   = next_session_word_spacing;
        letter_spacing = next_session_letter_spacing;

        grid_fitting = next_session_grid_fitting;
        if (grid_fitting) {
            min_minor = ceil(min_minor);
            min_major = ceil(min_major);
            max_minor = floor(max_minor);
            max_major = floor(max_major);
            word_spacing   = archon_round(word_spacing);
            letter_spacing = archon_round(letter_spacing);
        }

        in_session = true;
    }


    // Acquire a style if one has not already been acquired
    // FIXME: Indicate 'request for style refresh' in a different way such that it can be discovered that the new style is the same as the old one.
    // FIXME: We don't seem to need both a current_style_id and a current_chunk. current_chunk might be enough. But see above.
    if (current_style_id == 0) {
        const int style_id = acquire_style();

        FontCache::FontMetrics info;
        get_style_info(style_id, vertical, false, info);
        style_lateral_span = info.lateral_span;
        // FIXME: Negative values of line_spacing gives unexpected
        // results.
        const double a = style_lateral_span.get_length(), b = line_spacing * a;
        style_lateral_span.begin -= (b-a) / 2;
        style_lateral_span.end = style_lateral_span.begin + b;
        if (grid_fitting) {
            // FIXME: In fact the spans do not have to be grid-fitted,
            // instead the baseline advances should be rounded
            // upwards.
            style_lateral_span.begin = floor(style_lateral_span.begin);
            style_lateral_span.end   =  ceil(style_lateral_span.end);
        }
        if (rev_major)
            style_lateral_span.reflect();

        current_style_id = style_id;
        have_space_advance = false;
        if (current_chunk && current_chunk->num_glyphs == 0) {
            current_chunk->style_id = current_style_id;
        }
        else {
            chunks.push_back(Chunk(current_style_id));
            current_chunk = &chunks.back();
        }

        if (current_line) {
            if (0 <= word_start_index) {
                if (word_start_index == long(glyph_indices.size())) {
                    word_lateral_span = style_lateral_span;
                }
                else {
                    word_lateral_span.include(style_lateral_span);
                }
            }
            adjust_lateral_line_span(style_lateral_span, true);
        }
    }


    get_glyph_info(current_style_id, vertical, grid_fitting,
                   kerning ? rev_minor ? FontCache::kern_Dec : FontCache::kern_Inc :
                   FontCache::kern_No, inbuf_num, inbuf, glyph_info);

// Strategy for exception safety is to wrap most inside a try-catch and restore the output vector sizes on error, and maybe also the input consumption state. No - no can do, see write(). More considereation is needed.

    const wchar_t* next_char = inbuf;
    const FontCache::GlyphInfo* info = glyph_info;
    if (ignore_inbuf_front) {
        ++next_char;
        ++info;
    }
    const FontCache::GlyphInfo* const info_end = info + num_glyphs;

    glyph_indices.reserve(glyph_indices.size() + num_glyphs);
    advance_comps.reserve(advance_comps.size() + num_glyphs + 1);

    const bool word_wrap_enabled = 0 < max_minor && word_wrap != word_wrap_No;
    const bool line_wrap_enabled = 0 < max_minor && line_wrap;
    const bool wrap = word_wrap_enabled || line_wrap_enabled;

    do {
        // Provide a current page and line
        open_line(); // May throw

        bool break_out = false;
        do {
            int glyph = info->index;
            double pre_advance = last_minor_advance;
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
                    post_advance += word_spacing;
                    if (word_wrap == word_wrap_Justify)
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

            if (empty_line) {
                last_minor_pos = 0;
                pre_advance = 0;
            }
            else if (0 < post_advance) {
                if (kerning)
                    pre_advance += info->kerning;
                pre_advance += letter_spacing;
                if (pre_advance < 0)
                    pre_advance = 0;
            }

            const double glyph_pos = last_minor_pos + pre_advance;

            if (is_tab) {
                glyph = -1; // No glyph for white space
                if (!have_space_advance)
                    update_space_advance();
                double tab_stop_spacing = 8 * space_advance;
                double next_glyph_pos = glyph_pos + space_advance + word_spacing;
                double num_tab_stops = ceil(next_glyph_pos / tab_stop_spacing);
                double tab_stop_pos = num_tab_stops * tab_stop_spacing;
                post_advance = tab_stop_pos - glyph_pos;
            }

            const double line_length = glyph_pos + post_advance;

            // Check for line overflow
            if (wrap && brk == brk_None && max_minor < line_length) {
                if (word_wrap_enabled) {
                    if (0 <= word_start_index) {
                        do_word_wrap(); // Word wrapping
                        break;
                    }
                    if (line_wrap_enabled && !empty_line) {
                        close_line(); // Simple line wrapping on top of word wrapping
                        break;
                    }
                }
                else if (!empty_line) {
                    close_line(); // Simple line wrapping
                    break;
                }
            }

            if (is_vari_space)
                word_separators.push_back(glyph_indices.size());
            glyph_indices.push_back(glyph);
            advance_comps.push_back(pre_advance);
            // FIXME: Maybe these can be added in chunks.
            ++current_chunk->num_glyphs;
            ++current_line->num_glyphs;

            ++info;
            ++next_char;

            const double prev_line_length = current_line->length;

            last_minor_advance   = post_advance;
            last_minor_pos       = glyph_pos;
            empty_line           = false;
            current_line->length = line_length;

            switch (brk) {
                case brk_None:
                    break;
                case brk_Word:
                    line_length_snapshot  = prev_line_length;
                    lateral_span_snapshot = current_line->lateral_span;
                    word_sep_pos = glyph_pos;
                    word_start_index = glyph_indices.size();
                    if (word_wrap == word_wrap_Justify)
                        num_word_separators = word_separators.size() - 1;
                    word_lateral_span = style_lateral_span;
                    if (max_minor < line_length) {
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
        inbuf_pos = inbuf;
        ignore_inbuf_front = false;
    }
    else {
        // Make last character available for kerning consideration during next flush
        inbuf[0] = inbuf[inbuf_num - 1];
        inbuf_pos = inbuf + 1;
        ignore_inbuf_front = true;
    }
}


void TextFormatter::update_space_advance()
{
    wchar_t space = L' ';
    FontCache::GlyphInfo glyph_info;
    get_glyph_info(current_style_id, vertical, grid_fitting,
                   FontCache::kern_No, 1, &space, &glyph_info);
    space_advance = glyph_info.advance;
    have_space_advance = true;
}


void TextFormatter::open_line()
{
    if (current_line)
        return;

    // Check for page overflow
    double pos;
    if (current_page) {
        pos = current_page->get_length(this);
        if (page_wrap && 0 < max_major && 0 < current_page->num_lines &&
            max_major < pos + style_lateral_span.get_length())
        {
            close_page();
            pos = 0;
        }
    }
    else {
        pos = 0;
    }

    // Open new page if required
    lines.reserve(lines.size()+1);
    open_page();

    // Open new line
    lines.push_back(Line(pos));
    current_line = &lines.back();
    current_line->lateral_span = style_lateral_span;
    ++current_page->num_lines;
    empty_line = true;
    word_start_index = -1;
    word_separators.clear();
}


void TextFormatter::open_page()
{
    if (current_page)
        return;

    pages.push_back(Page(glyph_indices.size(), lines.size(),
                         chunks.size()-1, chunks.back().num_glyphs));
    current_page = &pages.back();
}


void TextFormatter::do_word_wrap()
{
    longest_complete_line = max_minor;
    advance_comps[word_start_index-1] -= word_sep_pos - line_length_snapshot;
    current_line->length = line_length_snapshot;
    if (word_wrap == word_wrap_Justify && 0 < num_word_separators) {
        double diff = max_minor - current_line->length;
        if (grid_fitting)
            diff = archon_round(diff);
        const double f = diff / num_word_separators;
        double prev = 0;
        for (long i=0; i<num_word_separators; ++i) {
            double next = f * (i+1);
            if (grid_fitting)
                next = archon_round(next);
            const double adjust = next - prev;
            advance_comps[word_separators[i]+1] += adjust;
            prev = next;
        }
        current_line->length = max_minor;
    }

    adjust_lateral_line_span(lateral_span_snapshot, false);
    close_line();

    const int n = glyph_indices.size() - word_start_index;
    if (0 < n) {
        last_minor_pos -= word_sep_pos + advance_comps[word_start_index];
        advance_comps[word_start_index] = 0;
        open_line();
        Line* previous_line = &*(lines.end()-2);
        if (current_page->num_lines < 2)
            current_page->move_back_by_glyphs(this, n);
        previous_line->num_glyphs -= n;
        current_line->num_glyphs += n;
        empty_line = false;
        current_line->length = last_minor_pos + last_minor_advance;
        current_line->lateral_span = word_lateral_span;
    }
}


void TextFormatter::adjust_lateral_line_span(const Math::Interval& span, bool include)
{
    const bool wrap = page_wrap && 0 < max_major;
    int way = 0; // -1 for decrease, 1 for increase, 0 for same height.
    if (include && 0 < current_line->num_glyphs) {
        double old;
        if (wrap)
            old = current_line->lateral_span.get_length();
        current_line->lateral_span.include(span);
        if (wrap)
            way = old < current_line->lateral_span.get_length() ? 1 : 0;
    }
    else {
        if (wrap) {
            const double l1 = current_line->lateral_span.get_length();
            const double l2 = span.get_length();
            way = l1 < l2 ? 1 : l2 < l1 ? -1 : 0;
        }
        current_line->lateral_span = span;
    }

    if (wrap) {
        if (way < 0) {
            // Check for page underflow
            if (current_page->num_lines < 2) {
                // WHOOPS: This should also apply for line breaking
                // without word breaking This only applies if the page
                // break was not "hard" NOTE: If pages could have
                // different widths, we would have to check that the
                // truncated current line would fit lenghtwise on the
                // previous page, and also that the untruncated line
                // would NOT fit lengthwise on the previous page,
                // since in that case the current line would not fit
                // latterally on the previous page. PROBLEM: This
                // needs to be rechecked at every extra character
                // added to the wrapped word (including non-breaking
                // spaces)
                ARCHON_ASSERT_1(false, "Page underflow is not implemented"); // FIXME: Implement this!
            }
        }
        else if (0 < way) {
            // Check for page overflow
            if (1 < current_page->num_lines && max_major < current_page->get_length(this)) {
                close_page();
                open_page();
                Page& previous_page = *(pages.end()-2);
                // NOTE: If pages could have different widths, we
                // would have to reprocess the current line, because
                // it may not fit lengthwise on the next page.
                current_page->move_back_one_line(this, &previous_page);
                current_line = &lines.back();
                current_line->lateral_pos = 0;
            }
        }
    }
}


Archon::Core::EnumAssoc TextFormatter::WordWrapSpec::map[] =
{
    { TextFormatter::word_wrap_No,      "no"      },
    { TextFormatter::word_wrap_Yes,     "yes"     },
    { TextFormatter::word_wrap_Justify, "justify" },
    { 0, 0 }
};


} // namespace Font
} // namespace Archon
