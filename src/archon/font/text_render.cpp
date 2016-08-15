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

/**
 * \file
 *
 * \author Kristian Spangsege
 */

#include <cmath>

#include <archon/core/memory.hpp>
#include <archon/font/text_render.hpp>

using namespace std;
using namespace archon::Core;
using namespace archon::Math;
using namespace archon::Util;
using namespace archon::Imaging;
using namespace archon::Font;


namespace archon
{
  namespace Font
  {
    struct TextRenderer::TextProcessor: TextHandler
    {
      void handle(int style_id, int num_glyphs, int const *glyphs, float const *components)
      {
        Style const &s = renderer.styles[style_id-1];
        set_color(s.text_color);
        renderer.cache->render_text(s.font_id, grid_fitting, layout_direction,
                                    num_glyphs, glyphs, components, img_writer);
      }

      TextProcessor(TextRenderer const &r, SessionInfo const &info, ImageWriter &w):
        renderer(r), grid_fitting(info.grid_fitting), layout_direction(info.layout_direction),
        img_writer(w)
      {
        w.enable_blending().enable_color_mapping();
        set_color(PackedTRGB(), true);
      }

      void set_color(PackedTRGB color, bool force = false)
      {
        if(force || color != last_color)
        {
          img_writer.set_background_color(PackedTRGB(color.value() | 0xFF000000));
          img_writer.set_foreground_color(color);
          last_color = color;
        }
      }

      TextRenderer const &renderer;
      bool grid_fitting;
      FontCache::Direction layout_direction;
      ImageWriter &img_writer;
      PackedTRGB last_color;
    };


    struct TextRenderer::StructProcessor: StructHandler
    {
      void line_box(Vec2 const &pos, Vec2 const &size)
      {
        if(size[0] <= 0 || size[1] <= 0) return;
        fill_rect(pos, size, PackedTRGB(even_line ? 0xB0E0FF : 0xFFE0B0));
        even_line = !even_line;
        even_glyph = false;
      }

      void glyph_box(Vec2 const &pos, Vec2 const &size)
      {
        if(size[0] <= 0 || size[1] <= 0) return;
        fill_rect(pos, size, PackedTRGB(even_glyph ? 0xD0FFFF : 0xFFFFD0));
        even_glyph = !even_glyph;
      }

      void baseline(double pos, bool vertical, bool before, int type)
      {
        int x = 0, y = archon_round(pos);
        int w = vertical ? img_writer.get_height() : img_writer.get_width(), h = 1;
        if(before) --y;
        if(vertical)
        {
          swap(x,y);
          swap(w,h);
        }
        unsigned long trgb = type == 0 ? 0xFF0000 : type == 1 ? 0x0000FF : 0x00C000;
        img_writer.set_clip(x,y,w,h).set_foreground_color(PackedTRGB(trgb)).fill();
      }

      void fill_rect(Vec2 const &pos, Vec2 const &size, PackedTRGB color)
      {
        int const x = archon_round(pos[0]);
        int const y = archon_round(pos[1]);
        int const w = int(archon_round(pos[0] + size[0])) - x;
        int const h = int(archon_round(pos[1] + size[1])) - y;
        img_writer.set_clip(x,y,w,h).set_foreground_color(color).fill();
      }

      StructProcessor(ImageWriter &w): img_writer(w), first_line(true), even_line(false) {}

      ImageWriter &img_writer;
      bool first_line, even_line, even_glyph;
    };


    Image::Ref TextRenderer::render(int page_index, bool debug)
    {
      SessionInfo info;
      get_session_info(info);

      Vec2 const page_size = get_page_size(page_index);
      Vec2 offset;
      int width, height;
      if(info.grid_fitting)
      {
        offset[0] = archon_round(padding_left);
        offset[1] = archon_round(padding_bottom);
        width  = offset[0] + page_size[0] + archon_round(padding_right);
        height = offset[1] + page_size[1] + archon_round(padding_top);
      }
      else
      {
        double const w = padding_left   + page_size[0] + padding_right;
        double const h = padding_bottom + page_size[1] + padding_top;
        width  = ceil(w);
        height = ceil(h);
        offset[0] = padding_left   + 0.5*(width  - w);
        offset[1] = padding_bottom + 0.5*(height - h);
      }

      int full_width = width, full_height = height;
      int border_hori = border_left   + border_right;
      int border_vert = border_bottom + border_top;
      bool border = 0 < border_hori || 0 < border_vert;
      if(border)
      {
        offset[0] += border_left;
        offset[1] += border_bottom;
        full_width  += border_hori;
        full_height += border_vert;
      }

      if(full_width < 1 || full_height < 1) return Image::Ref();

      bool has_alpha = background_color.value() & 0xFF000000;
      Image::Ref img =
        Image::new_image(full_width, full_height, ColorSpace::get_RGB(), has_alpha);
      ImageWriter writer(img);

      if(width < 1 || height < 1)
      {
        writer.set_foreground_color(border_color).fill();
        return img;
      }

      writer.set_background_color(background_color).clear();

      if(debug)
      {
        StructProcessor proc(writer);
        process_page_struct(page_index, offset, proc);
        writer.set_clip(); // Reset
      }

      if(border)
      {
        writer.set_foreground_color(border_color);
        if(border_bottom)
          writer.set_clip(0, 0, full_width, border_bottom).fill();
        if(border_top)
          writer.set_clip(0, full_height - border_top, full_width, border_top).fill();
        if(border_left)
          writer.set_clip(0, border_bottom, border_left, height).fill();
        if(border_right)
          writer.set_clip(full_width - border_right, border_bottom, border_right, height).fill();
        writer.set_clip(border_left, border_bottom, width, height);
      }

      {
        TextProcessor proc(*this, info, writer);
        process_page(page_index, offset, proc);
      }

      return img;
    }



    void TextRenderer::clear()
    {
      release_used_fonts();
      clear_vector(used_fonts, 8, 32);
      if(0 <= font_id) used_fonts.push_back(font_id);
      TextFormatter::clear();
    }



    TextRenderer::TextRenderer(FontCache::Arg c):
      cache(c), font_id(-1),
      text_color(0x0), background_color(0xFFFFFF), border_color(0),
      padding_top(4), padding_right(4), padding_bottom(4), padding_left(4),
      border_top(0), border_right(0), border_bottom(0), border_left(0)
    {
      used_fonts.reserve(8);
      FontCache::FontOwner font(c, c->acquire_default_font());
      c->get_font_desc(font.get(), font_desc);
      used_fonts.push_back(font.get());
      default_font = font.release();
    }


    TextRenderer::~TextRenderer()
    {
      font_id = -1; // Release everything
      release_used_fonts();
    }



    // Overriding TextFormatter::acquire_style
    int TextRenderer::acquire_style()
    {
      if(font_id < 0)
      {
        FontCache::FontOwner f(cache, cache->acquire_font(font_desc));
        used_fonts.push_back(f.get());
        font_id = f.release();
      }

      Style const s(font_id, text_color);
      int &id = style_map[s];
      if(id == 0) // New
      {
        styles.push_back(s);
        id = styles.size(); // One plus index
      }

      return id;
    }


    // Overriding TextFormatter::get_style_info
    void TextRenderer::get_style_info(int style_id, bool vertical, bool grid_fitting,
                                      FontCache::FontMetrics &info)
    {
      cache->get_font_metrics(styles[style_id-1].font_id, vertical, grid_fitting, info);
    }


    // Overriding TextFormatter::get_glyph_info
    void TextRenderer::get_glyph_info(int style_id, bool vertical, bool grid_fitting,
                                      FontCache::KernType kern, int num_chars,
                                      wchar_t const *chars, FontCache::GlyphInfo *glyphs)
    {
      cache->get_glyph_info(styles[style_id-1].font_id, vertical, grid_fitting, kern,
                            num_chars, chars, glyphs);
    }


    // Does not release the one that is currently used
    void TextRenderer::release_used_fonts() throw()
    {
      while(!used_fonts.empty())
      {
        int const id = used_fonts.back();
        used_fonts.pop_back();
        if(id != font_id) cache->release_font(id);
      }
    }
  }
}
