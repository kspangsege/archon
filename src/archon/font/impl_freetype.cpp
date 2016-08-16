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

#include <cstdlib>
#include <limits>
#include <algorithm>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

#include <archon/core/types.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/weak_ptr.hpp>
#include <archon/core/file.hpp>
#include <archon/util/unit_frac.hpp>
#include <archon/font/loader.hpp>


using namespace std;
using namespace archon::core;
using namespace archon::math;
using namespace archon::util;
using namespace archon::image;
using namespace archon::font;


namespace
{
  struct LoaderImpl: FontLoader
  {
    UniquePtr<FontFace> load_default_face(double w, double h) const
    {
      UniquePtr<FontFace> f(load_face(default_file, default_index, w, h).release());
      return f;
    }


    UniquePtr<FontFace> load_face(string, int, double, double) const;


    void load_face_info(string font_file, int face_index, FaceInfo &info) const
    {
      FT_Face f;
      if(FT_New_Face(library, font_file.c_str(), face_index, &f))
        throw BadFontFileException("Failed to load \""+font_file+"\"");
      try
      {
        info.family = f->family_name ? string(f->family_name) : string();
        info.bold   = f->style_flags & FT_STYLE_FLAG_BOLD;
        info.italic = f->style_flags & FT_STYLE_FLAG_ITALIC;
        info.monospace = FT_IS_FIXED_WIDTH(f);
        info.scalable  = FT_IS_SCALABLE(f);
        info.fixed_sizes.reserve(f->num_fixed_sizes);
        for(int i=0; i<f->num_fixed_sizes; ++i)
        {
          FT_Bitmap_Size const &s = f->available_sizes[i];
          info.fixed_sizes.push_back(FaceInfo::FixedSize(1/64.0*s.x_ppem, 1/64.0*s.y_ppem));
        }
      }
      catch(...)
      {
        FT_Done_Face(f);
        throw;
      }
    }


    int check_file(string f) const
    {
      FT_Face face;
      if(FT_New_Face(library, f.c_str(), -1, &face)) return 0;
      ARCHON_ASSERT_1(0 < face->num_faces, "No faces in font file");
      return face->num_faces;
    }


    string get_default_font_file() const { return default_file; }
    int get_default_face_index() const { return default_index; }


    LoaderImpl(FT_Library l, string resource_dir):
      library(l), default_file(resource_dir+"LiberationSerif-Regular.ttf"), default_index(0) {}


    ~LoaderImpl()
    {
      FT_Done_FreeType(library);
    }


    WeakPtr<LoaderImpl> const weak_self;

    FT_Library const library;

  private:
    string const default_file;
    int const default_index;
  };


  struct RenderTarget
  {
    unsigned char *lower_left;
    int width, height;
    RenderTarget(unsigned char *p, int w, int h): lower_left(p), width(w), height(h) {}
  };


  void render_spans(int y, int count, FT_Span const *spans, void *user) throw()
  {
    RenderTarget const *const target = static_cast<RenderTarget *>(user);
    ARCHON_ASSERT_1(0 <= y && y < target->height, "render_spans: Bad y");
    unsigned char *const ptr = target->lower_left + y * target->width;
    for(int i=0; i<count; ++i)
    {
      FT_Span const &span = spans[i];
      int const x1 = span.x, x2 = x1 + span.len;
      ARCHON_ASSERT_1(0 <= x1 && x2 <= target->width, "render_spans: Bad x");
      fill(ptr+x1, ptr+x2,
           frac_adjust_bit_width(span.coverage, 8, numeric_limits<unsigned char>::digits));
    }
  }



  struct FaceImpl: FontFace
  {
    string get_family_name() const
    {
      return face->family_name ? string(face->family_name) : string();
    }


    bool is_bold()      const { return face->style_flags & FT_STYLE_FLAG_BOLD;   }
    bool is_italic()    const { return face->style_flags & FT_STYLE_FLAG_ITALIC; }
    bool is_monospace() const { return FT_IS_FIXED_WIDTH(face); }
    bool is_scalable()  const { return FT_IS_SCALABLE(face);    }


    int get_num_fixed_sizes() const
    {
      return face->num_fixed_sizes;
    }


    Vec2 get_fixed_size(int i) const
    {
      if(i < 0 || face->num_fixed_sizes <= i) throw out_of_range("fixed_size_index");
      return 1/64.0 * Vec2(face->available_sizes[i].x_ppem, face->available_sizes[i].y_ppem);
    }


    void set_fixed_size(int i)
    {
      if(i < 0 || face->num_fixed_sizes <= i) throw out_of_range("fixed_size_index");
      if(FT_Select_Size(face, i) != 0) throw runtime_error("Failed to set fixed size");
      FT_Bitmap_Size const &size = face->available_sizes[i];
      on_size_changed(size.x_ppem, size.y_ppem);
    }


    void set_scaled_size(double width, double height)
    {
      if(!FT_IS_SCALABLE(face)) throw logic_error("Font face is not scalable");
      if(width  <= 0 || 16384 < width ||
         height <= 0 || 16384 < height) throw invalid_argument("Bad font size");
      FT_F26Dot6 const w = width*64, h = height*64;
      if(FT_Set_Char_Size(face, w, h, 0, 0)) throw runtime_error("FT_Set_Char_Size failed");
      on_size_changed(w,h);
    }


    void set_approx_size(double width, double height)
    {
      // Initialize on demand
      if(fixed_sizes.empty())
      {
        if(face->num_fixed_sizes == 0)
        {
          set_scaled_size(width, height);
          return;
        }

        for(int i=0; i<face->num_fixed_sizes; ++i)
        {
          FT_Bitmap_Size const &s = face->available_sizes[i];
          fixed_sizes[1/64.0 * Vec2(s.x_ppem, s.y_ppem)] = i;
        }
      }

      // First check for an exact match
      Vec2 const size(width, height);
      FixedSizes::iterator const i = fixed_sizes.find(size);
      if(i != fixed_sizes.end())
      {
        set_fixed_size(i->second);
        return;
      }

      if(FT_IS_SCALABLE(face))
      {
        set_scaled_size(width, height);
        return;
      }

      // Now search for the best inexact match
      double min = numeric_limits<int>::max();
      int idx = 0;
      FixedSizes::iterator const end = fixed_sizes.end();
      for(FixedSizes::iterator j = fixed_sizes.begin(); j != end; ++j)
      {
        double const diff = sq_dist(j->first, size);
        if(diff < min)
        {
          min = diff;
          idx = j->second;
        }
      }
      set_fixed_size(idx);
    }


    void on_size_changed(FT_F26Dot6 width, FT_F26Dot6 height)
    {
      render_width  = width;
      render_height = height;

      FT_Size_Metrics const &metrics = face->size->metrics;
      double const space_h = 1/64.0 * metrics.height;
      double const space_v = 1/64.0 * metrics.max_advance;
      ARCHON_ASSERT_1(0 < space_h && 0 < space_v, "Zero baseline spacing");
      int const space_h_gf = ceil(space_h);
      int const space_v_gf = ceil(space_v);

      double const min_h = 1/64.0 * metrics.descender, max_h = 1/64.0 * metrics.ascender;
      // Unfortunately Freetype cannot provide appropriate values for
      // the the descender and ascender equivalents in a vertical
      // layout. We are forced to make a guess that can easily be
      // wrong. We will assume that the vertical baseline is centered
      // on the line.
      double const min_v = -0.5 * space_v, max_v = min_v + space_v;

      int const min_h_gf = floor(min_h);
      int const max_h_gf =  ceil(max_h);
      int const min_v_gf = floor(min_v);
      int const max_v_gf =  ceil(max_v);

      hori_baseline_offset     = (space_h - max_h - min_h) / 2;
      hori_baseline_spacing    = space_h;
      vert_baseline_offset     = (space_v - max_v - min_v) / 2;
      vert_baseline_spacing    = space_v;
      hori_baseline_offset_gf  = archon_round((space_h_gf - max_h_gf - min_h_gf) / 2.0);
      hori_baseline_spacing_gf = space_h_gf;
      vert_baseline_offset_gf  = archon_round((space_v_gf - max_v_gf - min_v_gf) / 2.0);
      vert_baseline_spacing_gf = space_v_gf;
    }


    double get_width() const
    {
      return 1/64.0 * render_width;
    }


    double get_height() const
    {
      return 1/64.0 * render_height;
    }


    double get_baseline_spacing(bool vertical, bool grid_fitting) const
    {
      return grid_fitting ?
        vertical ? vert_baseline_spacing_gf : hori_baseline_spacing_gf :
        vertical ? vert_baseline_spacing    : hori_baseline_spacing;
    }


    double get_baseline_offset(bool vertical, bool grid_fitting) const
    {
      return grid_fitting ?
        vertical ? vert_baseline_offset_gf : hori_baseline_offset_gf :
        vertical ? vert_baseline_offset    : hori_baseline_offset;
    }


    int get_num_glyphs() const
    {
      return face->num_glyphs;
    }


    int find_glyph(wchar_t c) const
    {
      return FT_Get_Char_Index(face, c);
    }


    double get_kerning(int glyph1, int glyph2, bool vertical, bool grid_fitting) const
    {
      // FreeType only supports kerning for horizontal layouts
      if(!has_kerning || vertical || glyph1 == 0 || glyph2 == 0) return 0;
      FT_UInt const kern_mode = grid_fitting ? FT_KERNING_DEFAULT : FT_KERNING_UNFITTED;
      FT_Vector v;
      FT_Get_Kerning(face, glyph1, glyph2, kern_mode, &v);
      return v.x / 64.0;
    }


    void load_glyph(int i, bool grid_fitting)
    {
      if(i < 0 || face->num_glyphs <= i) throw out_of_range("glyph_index");
      FT_Int32 flags = FT_LOAD_CROP_BITMAP | FT_LOAD_TARGET_NORMAL;
      if(!grid_fitting) flags |= FT_LOAD_NO_HINTING;
      if(FT_Load_Glyph(face, i, flags)) throw runtime_error("FT_Load_Glyph failed");

      hori_glyph_advance = 1/64.0 * glyph->metrics.horiAdvance;

      // Freetype always loads a glyph such that the origin of the
      // outline description coincides with the bearing point
      // pertaining to a horizontal layout. Therefore, to acheive the
      // direction neutral position where the origin of the outline
      // description is the lower left corner of the bounding box, we
      // need to make a correction.
      double left   = 1/64.0 *  glyph->metrics.horiBearingX;
      double top    = 1/64.0 *  glyph->metrics.horiBearingY;
      double right  = 1/64.0 * (glyph->metrics.horiBearingX + glyph->metrics.width);
      double bottom = 1/64.0 * (glyph->metrics.horiBearingY - glyph->metrics.height);

      // Grid fitting of the glyph metrics will normally already have
      // been done by FreeType, but since that behavior appears to be
      // compile-time configurable, the rounding is repeated
      // here. Fortunately rounding is an idempotent operation.
      if(grid_fitting)
      {
        hori_glyph_advance = archon_round(hori_glyph_advance);
        left   = floor(left);
        bottom = floor(bottom);
        right  = ceil(right);
        top    = ceil(top);
      }

      // Vector from bearing point of vertical layout to bearing point
      // of horizontal layout
      // FIXME: It seems that in some cases such as "Liberation
      // Serif", the vertical metrics are set to appropriate values
      // even when the underlying font face does not provide any. If
      // that were always the case, ther would be no point in
      // emulating those metrics below. Problem is, according to the
      // documentation, the vertical metrics musr be considered
      // unrelibale when FT_HAS_VERTICAL(face) returns false.
      Vec2 v2h;
      if(FT_HAS_VERTICAL(face))
      {
        vert_glyph_advance = 1/64.0 * glyph->metrics.vertAdvance;
        v2h  = 1/64.0 * Vec2(glyph->metrics.vertBearingX - glyph->metrics.horiBearingX,
                             glyph->metrics.vertAdvance -
                             glyph->metrics.vertBearingY - glyph->metrics.horiBearingY);
        if(grid_fitting)
        {
          vert_glyph_advance = archon_round(vert_glyph_advance);
          v2h[0] = archon_round(v2h[0]);
          v2h[1] = archon_round(v2h[1]);
        }
      }
      else // Emulated vertical metrics
      {
        if(grid_fitting)
        {
          vert_glyph_advance = hori_baseline_spacing_gf;
          v2h.set(archon_round(-0.5 * hori_glyph_advance), hori_baseline_offset_gf);
        }
        else
        {
          vert_glyph_advance = hori_baseline_spacing;
          v2h.set(-0.5 * hori_glyph_advance, hori_baseline_offset);
        }
      }

      glyph_size.set(right - left, top - bottom);
      hori_glyph_bearing.set(-left, -bottom);
      vert_glyph_bearing = hori_glyph_bearing - v2h;
      prev_glyph_translation_x = -64 * hori_glyph_bearing[0];
      prev_glyph_translation_y = -64 * hori_glyph_bearing[1];
      glyph_translation.set(0);
    }


    double get_glyph_advance(bool vertical) const
    {
      return vertical ? vert_glyph_advance : hori_glyph_advance;
    }


    Vec2 get_glyph_bearing(bool vertical) const
    {
      return vertical ? vert_glyph_bearing : hori_glyph_bearing;
    }


    Vec2 get_glyph_size() const
    {
      return glyph_size;
    }


    void translate_glyph(Vec2 v)
    {
      glyph_translation += v;
    }


    void get_glyph_pixel_box(int &left, int &right, int &bottom, int &top) const
    {
      if(glyph->format == FT_GLYPH_FORMAT_BITMAP)
      {
        left   = archon_round(glyph_translation[0]);
        bottom = archon_round(glyph_translation[1]);
        right = left   + glyph->bitmap.width;
        top   = bottom + glyph->bitmap.rows;
      }
      else
      {
        left   = floor(glyph_translation[0]);
        bottom = floor(glyph_translation[1]);
        right  = ceil(glyph_translation[0] + glyph_size[0]);
        top    = ceil(glyph_translation[1] + glyph_size[1]);
      }
    }


    void set_target_origin(int x, int y)
    {
      target_origin_x = x;
      target_origin_y = y;
    }


    void render_pixels_to(ImageWriter &image_writer) const
    {
      int left, right, bottom, top;
      get_glyph_pixel_box(left, right, bottom, top);
      image_writer.set_pos(target_origin_x + left, target_origin_y + bottom);

      int const width = right - left, height = top - bottom;

      // First check if we can render the glyph directly and bypass
      // the intermediate buffer
      if(glyph->format == FT_GLYPH_FORMAT_BITMAP &&
         glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY &&
         0 < glyph->bitmap.num_grays &&
         unsigned(glyph->bitmap.num_grays-1) == unsigned(numeric_limits<unsigned char>::max()))
      {
        ssize_t const pitch = 1, stride = -glyph->bitmap.pitch;
        unsigned char const *src =
          stride < 0 ? glyph->bitmap.buffer - (height-1)*stride : glyph->bitmap.buffer;
        image_writer.put_block(src, pitch, stride, width, height, color_space_lum, false);
        return;
      }

      // Make sure our buffer is big enough to hold the affected pixel
      // block
      {
        size_t const s = width * size_t(height);
        if(pix_buf_size < s)
        {
          size_t const t = max(s, pix_buf_size + pix_buf_size/4); // Increase by at least 25%
          Array<unsigned char> b(t);
          pix_buf.swap(b);
          pix_buf_size = t;
        }
      }

      // Clear the buffer
      fill(pix_buf.get(), pix_buf.get()+pix_buf_size, 0);

      // Render into intermediate buffer
      if(glyph->format == FT_GLYPH_FORMAT_BITMAP)
      {
        ssize_t const pitch = 1, stride = -glyph->bitmap.pitch;
        unsigned char const *src =
          stride < 0 ? glyph->bitmap.buffer - (height-1)*stride : glyph->bitmap.buffer;
        unsigned char *dst = pix_buf.get();

        if(glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY)
        {
          int const num_grays = glyph->bitmap.num_grays;
          ARCHON_ASSERT_1(0 < num_grays, "Unexpected number of gray levels");
          for(int y=0; y<height; ++y, src += stride)
            for(int x=0; x<width; ++x, ++dst)
              *dst = frac_adjust_denom<unsigned char>(src[x*pitch], num_grays, 0);
        }
        else if(glyph->bitmap.pixel_mode == FT_PIXEL_MODE_MONO)
        {
          for(int y=0; y<height; ++y, src += stride)
            for(int x=0; x<width; ++x, ++dst)
              *dst = src[(x>>3)*pitch] & 128>>(x&7) ? numeric_limits<unsigned char>::max() : 0;
        }
        else throw runtime_error("Unsupported pixel format of glyph");
      }
      else // Is scalable outline
      {
        // Translate glyph
        {
          FT_Pos const x = 64 * (glyph_translation[0] -   left);
          FT_Pos const y = 64 * (glyph_translation[1] - bottom);
          if(x != prev_glyph_translation_x || y != prev_glyph_translation_y)
          {
            FT_Outline_Translate(&glyph->outline,
                                 x - prev_glyph_translation_x,
                                 y - prev_glyph_translation_y);
            prev_glyph_translation_x = x;
            prev_glyph_translation_y = y;
          }
        }

        RenderTarget target(pix_buf.get(), width, height);
        FT_Raster_Params params;
        params.flags      = FT_RASTER_FLAG_AA | FT_RASTER_FLAG_DIRECT;
        params.gray_spans = &render_spans;
        params.user       = &target;
        FT_Outline_Render(loader->library, &glyph->outline, &params);
      }

      image_writer.put_block(pix_buf.get(), width, height, color_space_lum, false);
    }


    FaceImpl(LoaderImpl const *l, FT_Face f, double w, double h):
      loader(l->weak_self), face(f), glyph(face->glyph), has_kerning(FT_HAS_KERNING(f)),
      color_space_lum(ColorSpace::get_Lum()), target_origin_x(0), target_origin_y(0),
      pix_buf_size(0)
    {
      ARCHON_ASSERT_1(0 < get_num_fixed_sizes() || is_scalable(),
                     "No fixed sizes in non-scalable font");

/*
      if(!face->charmap || face->charmap->encoding != FT_ENCODING_UNICODE)
        throw runtime_error("No Unicode charmap available in font face");
*/

      set_approx_size(w,h);
      static_cast<FontFace *>(this)->load_glyph(0);
    }

    ~FaceImpl()
    {
      FT_Done_Face(face);
    }

  private:
    SharedPtr<LoaderImpl const> const loader;
    FT_Face const face;
    FT_GlyphSlot const glyph;
    bool has_kerning;
    ColorSpace::ConstRef const color_space_lum;

    typedef std::map<Vec2, int> FixedSizes; // Values are fixed size indices.
    FixedSizes mutable fixed_sizes; // Used only by set_approx_size, and initialized on demand.

    FT_F26Dot6 render_width, render_height;

    double hori_baseline_offset, hori_baseline_spacing;
    double vert_baseline_offset, vert_baseline_spacing;

    int hori_baseline_offset_gf, hori_baseline_spacing_gf; // Grid fitted
    int vert_baseline_offset_gf, vert_baseline_spacing_gf; // Grid fitted

    double hori_glyph_advance, vert_glyph_advance;

    Vec2 glyph_size;
    Vec2 hori_glyph_bearing, vert_glyph_bearing;

    FT_Pos mutable prev_glyph_translation_x, prev_glyph_translation_y;
    Vec2 glyph_translation;

    // Position in target image of design tablet origin (in integer
    // pixels)
    int target_origin_x, target_origin_y;

    // These are used only when copying glyphs with one bit per pixel.
    size_t mutable pix_buf_size;
    Array<unsigned char> mutable pix_buf;
  };



  UniquePtr<FontFace> LoaderImpl::load_face(string f, int i, double w, double h) const
  {
    FT_Face face;
    if(FT_New_Face(library, f.c_str(), i, &face))
      throw BadFontFileException("Failed to load \""+f+"\"");
    try
    {
      // Some font files have extra "strap on" files with metrics and
      // kerning information
      string const suffix = file::suffix_of(f);
      if(suffix == "pfa" || suffix == "pfb") // Type 1 fonts (a.k.a. PostScript fonts)
      {
        string const stem = file::dir_of(f)+file::stem_of(f)+".";
        string const afm = stem + "afm";
        // We don't care if it fails, this is entirely opportunistic
        if(file::is_regular(afm)) FT_Attach_File(face, afm.c_str());
      }

      UniquePtr<FontFace> f(new FaceImpl(this, face, w, h));
      return f;
    }
    catch(...)
    {
      FT_Done_Face(face);
      throw;
    }
  }
}


namespace archon
{
  namespace font
  {
    FontLoader::Ptr new_font_loader(string resource_dir)
    {
      FT_Library library;
      if(FT_Init_FreeType(&library)) throw runtime_error("Error initializing FreeType library");
      SharedPtr<LoaderImpl> loader;
      try
      {
        loader.reset(new LoaderImpl(library, resource_dir));
      }
      catch(...)
      {
        FT_Done_FreeType(library);
        throw;
      }
      const_cast<WeakPtr<LoaderImpl> &>(loader->weak_self) = loader;
      return loader;
    }
  }
}
