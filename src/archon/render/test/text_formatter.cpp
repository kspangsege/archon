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

#include <cstdlib>
#include <string>

#include <GL/gl.h>

#include <archon/core/build_config.hpp>
#include <archon/core/char_enc.hpp>
#include <archon/core/options.hpp>
#include <archon/font/util.hpp>
#include <archon/font/layout_cfg.hpp>
#include <archon/display/keysyms.hpp>
#include <archon/render/text_formatter.hpp>
#include <archon/render/app.hpp>
#include <archon/core/cxx.hpp>


using namespace archon::core;
using namespace archon::math;
using namespace archon::font;
using namespace archon::display;
using namespace archon::render;



/*

Fix font/impl_fallback.cpp.

Page height must go to maximum when page wrapping occurs, but not when done manually.

Become able to navigate more freely.

Allow margin to differ from spacing in RectanglePacker. Then see if everything is fine, if glyphs are adjacent to texture edge.

Make font texture generation truely asynchronous by having it run in a sperarate low-priority thread.

*/


namespace {

struct TextFormatterApp: Application {
    struct Config: Application::Config, LayoutConfig {
        int page_num;
        Vec2 page_size;
        Vec2 font_size;
        bool mipmap;
        bool save_textures;
        Vec4F text_color;
        Vec2 glyph_resol;
        bool add_mixed;

        Config():
            page_num(1), page_size(2), font_size(0.1), mipmap(true),
            save_textures(false), text_color(1), glyph_resol(64), add_mixed(0) {}

        void populate(ConfigBuilder &cfg)
        {
            cfg.add_group(static_cast<Application::Config &>(*this), "win");
            cfg.add_group(static_cast<LayoutConfig &>(*this), "text");
            cfg.add_param("p", "page", page_num, "The number of the initial page to be rendered.");
            cfg.add_param("S", "size", page_size,
                          "Maximum page size (width,height). May be fractional. "
                          "If a component is less than or equal to zero, the page is unbounded in "
                          "that direction.");
            cfg.add_param("s", "font-size", font_size, "Set font size (x,y) where (1,1) "
                          "corresponds to normal size.");
            cfg.add_param("c", "color", text_color, "Set text color.");
            cfg.add_param("M", "add-mixed", add_mixed, "Add extra text using a bouquet of font styles "
                          "and colors.");
        }
    };


    TextFormatterApp(const Config& cfg, FontList& font_list, std::wstring text):
        Application("archon::render::TextFormatter", cfg, std::locale{""}, nullptr, nullptr,
                    new_font_cache(font_list)),
        m_text_formatter{get_font_provider()}
    {
        auto next_page = [this](bool key_down) {
            if (key_down) {
                if (++m_page_index == m_num_pages)
                    m_page_index -= m_num_pages;
                update_page();
                set_int_status(L"Page ", (m_page_index+1));
                return true;
            }
            return false;
        };
        auto prev_page = [this](bool key_down) {
            if (key_down) {
                if (m_page_index-- == 0)
                    m_page_index += m_num_pages;
                update_page();
                set_int_status(L"Page ", (m_page_index+1));
                return true;
            }
            return false;
        };

        bind_key(KeySym_Page_Down, std::move(next_page), "Go to next page.");
        bind_key(KeySym_Page_Up, std::move(prev_page), "Go to previous page.");

        m_text_formatter.set_page_width(Interval(0, cfg.page_size[0]));
        m_text_formatter.set_page_height(Interval(0, cfg.page_size[1]));
        m_text_formatter.set_font_size(cfg.font_size[0], cfg.font_size[1]);
        m_text_formatter.set_text_color(cfg.text_color);
        cfg.apply_to(m_text_formatter);
        m_text_formatter.write(text);

        if (cfg.add_mixed) {
            m_text_formatter.write(L" ");
            m_text_formatter.set_text_color(Vec4F(1,0,0,1));
            m_text_formatter.set_font_size(35/256.0, 35/256.0);
            m_text_formatter.set_font_weight(1); // Bold
            m_text_formatter.write(L"Kristian ");
            m_text_formatter.set_letter_spacing(10/256.0);
            m_text_formatter.write(L"Kristian ");

            m_text_formatter.set_text_color(Vec4F(0,1,0,1));
            m_text_formatter.set_font_size(25/256.0, 25/256.0);
            m_text_formatter.set_font_weight(0); // Not bold
            m_text_formatter.set_font_style(1); // Italic
            m_text_formatter.write(L"Spangsege ");

            m_text_formatter.set_text_color(Vec4F(1,0,1,1));
            m_text_formatter.set_font_size(30/256.0, 30/256.0);
            m_text_formatter.set_font_style(0); // Not italic
            m_text_formatter.set_font_family("URW Palladio L");
            m_text_formatter.write(L"h");
            m_text_formatter.set_line_spacing(2);
            m_text_formatter.write(L"I");
            m_text_formatter.set_line_spacing(1);
            m_text_formatter.write(L"gh ");

            m_text_formatter.set_text_color(Vec4F(0,0,1,1));
            m_text_formatter.set_font_size(45/256.0, 45/256.0);
            m_text_formatter.set_font_family("VL Gothic");
            m_text_formatter.write(L"Mandala");
        }

        m_page_index = cfg.page_num - 1;
        m_num_pages = m_text_formatter.get_num_pages();

        glEnable(GL_LIGHTING);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_NORMALIZE);

        glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
        glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
        glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);

        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 64);

        update_page();
    }


private:
    archon::render::TextFormatter m_text_formatter;
    TextLayout m_text_layout;
    int m_page_index, m_num_pages;

    void render() override
    {
        double w = m_text_layout.get_width(), h = m_text_layout.get_height();
        glTranslated(-0.5*w, -0.5*h, 0);

/*
        glEnable(GL_COLOR_MATERIAL);
        glNormal3f(0, 0, 1);
        glColor3f(0,0.5,0.5);
        Vec2 p0(0,0);
        Vec2 p1(w,h);
        glBegin(GL_QUADS);
        glVertex3d(p1[0], p1[1], -0.001);
        glVertex3d(p0[0], p1[1], -0.001);
        glVertex3d(p0[0], p0[1], -0.001);
        glVertex3d(p1[0], p0[1], -0.001);
        glEnd();
*/

        m_text_layout.render();
    }

    void update_page()
    {
        m_text_formatter.format(m_text_layout, m_page_index);
    }
};

} // unnamed namespace



int main(int argc, const char* argv[])
{
    std::set_terminate(&cxx::terminate_handler);
    try_fix_preinstall_datadir(argv[0], "render/test/");

    TextFormatterApp::Config app_cfg;
    FontConfig font_cfg;
    CommandlineOptions opts;
    opts.add_help("Test application for the texture based text rendering facility "
                  "of archon::render::Application.", "TEXT");
    opts.check_num_args(0,1);
    opts.add_stop_opts();
    opts.add_group(app_cfg);
    opts.add_group(font_cfg, "font");
    if (int stop = opts.process(argc, argv))
        return stop == 2 ? EXIT_SUCCESS : EXIT_FAILURE;

    std::string font_resource_dir = app_cfg.archon_datadir+"font/";
    std::unique_ptr<FontList> font_list = new_font_list(font_resource_dir, font_cfg);
    if (!font_list)
        return EXIT_FAILURE;

    std::wstring text = 1 < argc ? env_decode<wchar_t>(argv[1]) :
        L"The quick brown fox jumps over the lazy dog";

    TextFormatterApp(app_cfg, *font_list, text).run();
}
