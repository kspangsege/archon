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
#include <new>
#include <list>
#include <iomanip>
#include <sstream>
#include <iostream>

#include <archon/platform.hpp> // Never include this one in header files

#include <GL/gl.h>
#ifdef ARCHON_HAVE_GLU
#  include <GL/glu.h>
#endif

#include <archon/features.h>
#include <archon/core/functions.hpp>
#include <archon/core/sys.hpp>
#include <archon/core/build_config.hpp>
#include <archon/core/enum.hpp>
#include <archon/core/text.hpp>
#include <archon/util/ticker.hpp>
#include <archon/image/image.hpp>
#include <archon/dom/impl/html.hpp>
#include <archon/display/implementation.hpp>
#include <archon/render/dialog.hpp>
#include <archon/render/app.hpp>


using namespace archon::core;
using namespace archon::math;
using namespace archon::util;
using namespace archon::image;
using namespace archon::font;
using namespace archon::display;
using namespace archon::render;
namespace dom = archon::dom;
namespace dom_impl = archon::dom_impl;


namespace {

typedef archon::render::TextFormatter TextFormatter; // Resolving ambiguity


constexpr double g_zoom_step = std::pow(2, 1.0 / 8); // 8 steps to double
constexpr double g_zoom_min  = 0.1;
constexpr double g_zoom_max  = 32;

constexpr double g_camera_dist_step = std::pow(2, 1.0 / 8); // 8 steps to double

constexpr long g_status_hud_linger_millis = 1000;



class DialogImpl;


class PrivateApplicationState {
public:
    void open_help_hud()
    {
        std::shared_ptr<Dialog> dialog = new_modal_hud_dialog();
        using namespace dom;
        ref<html::HTMLDocument> doc = dialog->get_dom();
        ref<html::HTMLElement> body = doc->getBody();
        ref<css::CSSStyleDeclaration> body_style =
            dynamic_pointer_cast<css::ElementCSSInlineStyle>(body)->getStyle();
        body_style->setProperty(u16(L"background-color"), u16("white"), u16(""));
        body_style->setProperty(u16("color"), u16("red"), u16(""));
        body_style->setProperty(u16("border"), u16("2px solid lime"), u16(""));
        body_style->setProperty(u16("padding"), u16("8px"), u16(""));
        body->appendChild(doc->createTextNode(u16("Help me!")));
        body->appendChild(doc->createTextNode(u16(" Now!")));
/*
        ref<Element> elem = doc->createElement(u16("B"));
        ref<css::CSSStyleDeclaration> elem_style =
            dynamic_pointer_cast<css::ElementCSSInlineStyle>(elem)->getStyle();
        elem_style->setProperty(u16("color"), u16("black"), u16(""));
        elem_style->setProperty(u16("font-weight"), u16("bolder"), u16(""));
        elem_style->setProperty(u16("font-size"), u16("smaller"), u16(""));
        elem_style->setProperty(u16("height"), u16("100px"), u16(""));
        elem_style->setProperty(u16("background-color"), u16("lightblue"), u16(""));
        elem_style->setProperty(u16("padding"), u16("1em"), u16(""));
        elem_style->setProperty(u16("border"), u16("1px dashed red"), u16(""));
        elem->appendChild(doc->createTextNode(u16(" FISSE :-)")));
        body->appendChild(elem);
        ref<Element> elem2 = doc->createElement(u16("I"));
        ref<css::CSSStyleDeclaration> elem2_style =
            dynamic_pointer_cast<css::ElementCSSInlineStyle>(elem2)->getStyle();
        elem2_style->setProperty(u16("color"), u16("purple"), u16(""));
        elem2_style->setProperty(u16("font-style"), u16("italic"), u16(""));
        elem2_style->setProperty(u16("font-size"), u16("larger"), u16(""));
        elem2_style->setProperty(u16("border-left-width"), u16("thick"), u16(""));
        elem2_style->setProperty(u16("border-color"), u16("teal"), u16(""));
        elem2->appendChild(doc->createTextNode(u16("Barnach!?")));
        body->appendChild(elem2);
*/
        dialog->show();
/*
        std::cerr << "Body style #2: " << narrow_from_u16(body_style->getCssText()) << "\n";
        std::cerr << "Elem style #2: " << narrow_from_u16(elem_style->getCssText()) << "\n";
        std::cerr << "Elem2 style #2: " << narrow_from_u16(elem2_style->getCssText()) << "\n";
*/
    }

    std::shared_ptr<Dialog> new_modal_hud_dialog();

    void open_dialog(const std::shared_ptr<DialogImpl>&);
    void close_dialog(const std::shared_ptr<DialogImpl>&);

    bool has_open_dialogs() const { return !m_open_dialogs.empty(); }

    void render_hud(int viewport_width, int viewport_height);

    void on_resize();

    /// Can by called at any time, also when an OpenGL context is not
    /// bound.
    void recycle_display_list(GLuint disp_list)
    {
        m_available_display_lists.push_back(disp_list);
    }

    TextureCache& get_texture_cache()
    {
        ensure_texture_cache();
        return *m_texture_cache;
    }

    TextureDecl declare_texture(std::string image_path, bool repeat, TextureCache::FilterMode f)
    {
        ensure_texture_cache();
        std::unique_ptr<TextureSource> src = std::make_unique<TextureFileSource>(image_path);
        GLenum wrap = repeat ? GL_REPEAT : GL_CLAMP;
        return m_texture_cache->declare(std::move(src), wrap, wrap, f);
    }

    TextureDecl declare_texture(Image::ConstRefArg img, std::string name, bool repeat,
                                TextureCache::FilterMode f)
    {
        ensure_texture_cache();
        std::unique_ptr<TextureSource> src = std::make_unique<TextureImageSource>(img, name);
        GLenum wrap = repeat ? GL_REPEAT : GL_CLAMP;
        return m_texture_cache->declare(std::move(src), wrap, wrap, f);
    }

    FontProvider& get_font_provider()
    {
        if (!m_font_provider) {
            ensure_font_cache();
            ensure_texture_cache();
            m_font_provider =
                std::make_unique<FontProvider>(*m_font_cache, *m_texture_cache,
                                               m_glyph_resolution, m_glyph_mipmapping,
                                               m_save_glyph_textures); // Throws
        }
        return *m_font_provider;
    }

    TextFormatter& get_text_formatter()
    {
        if (!m_text_formatter)
            m_text_formatter = std::make_unique<TextFormatter>(get_font_provider()); // Throws
        return *m_text_formatter;
    }

    TextureDecl get_dashed_texture_decl()
    {
        if (!m_dashed_texture_decl) {
            unsigned char buffer[4] = {
                std::numeric_limits<unsigned char>::max(),
                std::numeric_limits<unsigned char>::max(),
                0, 0
            };
            Image::Ref img = Image::copy_image_from(buffer, 2, 1, ColorSpace::get_Lum(), true);
            m_dashed_texture_decl = declare_texture(img, "Dashed pattern", true,
                                                    TextureCache::FilterMode::nearest);
        }
        return m_dashed_texture_decl;
    }

    TextureDecl get_dotted_texture_decl()
    {
        if (!m_dotted_texture_decl) {
            Image::Ref img = Image::load(m_resource_dir + "render/dotted.png");
            m_dotted_texture_decl = declare_texture(img, "Dotted pattern", true,
                                                    TextureCache::FilterMode::mipmap);
        }
        return m_dotted_texture_decl;
    }

    // Called with bound OpenGL context
    void update()
    {
        for (GLuint list: m_available_display_lists) {
std::cout << "*";
            glDeleteLists(list, 1);
        }

        if (m_texture_cache)
            m_texture_cache->update();
    }

    TextLayout status_hud_text_layout;

    dom_impl::HTMLImplementation* get_dom_impl()
    {
        if (!m_dom_impl)
            m_dom_impl.reset(new dom_impl::HTMLImplementation);
        return m_dom_impl.get();
    }

    PrivateApplicationState(const Application::Config& cfg, const std::locale& locale,
                            TextureCache* texture_cache, std::unique_ptr<FontCache> font_cache):
        m_resource_dir{cfg.archon_datadir},
        m_locale{locale},
        m_utf16_string_codec{m_locale},
        m_texture_cache{texture_cache},
        m_font_cache{std::move(font_cache)},
        m_glyph_resolution{cfg.glyph_resol},
        m_glyph_mipmapping{cfg.glyph_mipmap},
        m_save_glyph_textures{cfg.glyph_save}
    {
    }

    virtual ~PrivateApplicationState()
    {
        status_hud_text_layout.clear();
    }

    const std::weak_ptr<PrivateApplicationState>& get_weak_self()
    {
        return m_weak_self;
    }

protected:
    std::weak_ptr<PrivateApplicationState> m_weak_self;

private:
    std::list<std::shared_ptr<DialogImpl>> m_open_dialogs;

    std::vector<GLuint> m_available_display_lists;

    const std::string m_resource_dir;
    const std::locale m_locale;
    const CharEnc<CharUtf16> m_utf16_string_codec;

    TextureCache* m_texture_cache = nullptr;
    std::unique_ptr<FontList> m_font_list;
    std::unique_ptr<FontCache> m_font_cache;
    const Vec2F m_glyph_resolution;
    const bool m_glyph_mipmapping, m_save_glyph_textures;
    std::unique_ptr<FontProvider> m_font_provider;
    std::unique_ptr<TextFormatter> m_text_formatter;
    TextureDecl m_dashed_texture_decl, m_dotted_texture_decl;

    dom::ref<dom_impl::HTMLImplementation> m_dom_impl;

    std::unique_ptr<TextureCache> m_texture_cache_owner;

    StringUtf16 u16(const std::string& s) const
    {
        StringUtf16 t;
        m_utf16_string_codec.encode_narrow(s,t);
        return t;
    }

    StringUtf16 u16(const std::wstring& s) const
    {
        StringUtf16 t;
        if (!m_utf16_string_codec.encode(s,t))
            throw std::runtime_error("UTF-16 encode");
        return t;
    }

    std::string narrow_from_u16(const StringUtf16& s) const
    {
        std::string t;
        if (!m_utf16_string_codec.decode_narrow(s,t))
            throw std::runtime_error("UTF-16 decode");
        return t;
    }

    void ensure_font_cache()
    {
        if (!m_font_cache) {
            std::shared_ptr<FontLoader> loader =
                new_font_loader(m_resource_dir + "font/"); // Throws
            m_font_list = new_font_list(std::move(loader)); // Throws
            m_font_cache = new_font_cache(*m_font_list); // Throws
        }
    }

    void ensure_texture_cache()
    {
        if (!m_texture_cache) {
            m_texture_cache_owner = make_texture_cache(); // Throws
            m_texture_cache = m_texture_cache_owner.get();
        }
    }
};



class DialogImpl: public Dialog {
public:
    DialogImpl(PrivateApplicationState* s):
        m_state{s->get_weak_self()}
    {
    }

    ~DialogImpl()
    {
        if (m_disp_list) {
            if (std::shared_ptr<PrivateApplicationState> s = m_state.lock())
                s->recycle_display_list(m_disp_list);
        }
    }

protected:
    void mark_dirty()
    {
        m_dirty = true;
    }

    virtual void render(TextFormatter&, int viewport_width, int viewport_height) = 0;

    const std::weak_ptr<PrivateApplicationState> m_state;

private:
    friend class PrivateApplicationState;

    bool m_is_open = false;
    bool m_dirty = true;

    // Name of the OpenGL display list that renders this HUD dialog,
    // or zero if no list has been created yet.
    GLuint m_disp_list = 0;
};



void PrivateApplicationState::open_dialog(const std::shared_ptr<DialogImpl>& d)
{
    if (d->m_is_open)
        return;
    m_open_dialogs.push_back(d);
    d->m_is_open = true;
}

void PrivateApplicationState::close_dialog(const std::shared_ptr<DialogImpl>& d)
{
    if (!d->m_is_open)
        return;
    m_open_dialogs.remove(d);
    d->m_is_open = false;
}


void PrivateApplicationState::render_hud(int viewport_width, int viewport_height)
{
    for (const auto& dialog: m_open_dialogs) {
        if (dialog->m_dirty) {
            if (!dialog->m_disp_list) {
                dialog->m_disp_list = glGenLists(1);
                if (!dialog->m_disp_list)
                    throw std::runtime_error("Failed to create a new OpenGL display list");
            }

            glNewList(dialog->m_disp_list, GL_COMPILE_AND_EXECUTE);
            dialog->render(get_text_formatter(), viewport_width, viewport_height);
            glEndList();

            dialog->m_dirty = false;
        }
        else {
            glCallList(dialog->m_disp_list);
        }
    }
}


void PrivateApplicationState::on_resize()
{
    for (const auto& dialog: m_open_dialogs)
        dialog->m_dirty = true;
}








// FIXME: Overlaps with DomRenderer in dom_renderer.hpp
class ModalHudDialogImpl: public DialogImpl, public dom_impl::Renderer {
public:
    void show() override
    {
        std::shared_ptr<DialogImpl> d(m_weak_self);
        if (std::shared_ptr<PrivateApplicationState> s = m_state.lock())
            s->open_dialog(d);
    }

    void hide() override
    {
        std::shared_ptr<DialogImpl> d(m_weak_self);
        if (std::shared_ptr<PrivateApplicationState> s = m_state.lock())
            s->close_dialog(d);
    }

    dom::ref<dom::html::HTMLDocument> get_dom() override
    {
        return m_dom_doc;
    }

    static std::shared_ptr<ModalHudDialogImpl> create(PrivateApplicationState* s, double dpcm)
    {
        std::shared_ptr<ModalHudDialogImpl> d{new ModalHudDialogImpl{s, dpcm}};
        d->m_weak_self = d;
        return d;
    }

    void filled_box(int x, int y, int width, int height, PackedTRGB color) override
    {
        std::cerr << "rect(" << x << ", " << y << ", " << width << ", " << height << ")" << std::endl;
        Vec4F rgba;
        color.unpack_rgba(rgba);
        glColor4f(rgba[0], rgba[1], rgba[2], rgba[3]);
        int x1 = x;
        int x2 = x1 + width;
        int y2 = m_viewport_height - y;
        int y1 = y2 - height;
        glBegin(GL_QUADS);
        glVertex2i(x1, y1);
        glVertex2i(x2, y1);
        glVertex2i(x2, y2);
        glVertex2i(x1, y2);
        glEnd();
    }

    void border_box(int x, int y, int width, int height, const Border* sides) override
    {
        int ox1 = x;
        int ox2 = ox1 + width;
        int oy2 = m_viewport_height - y;
        int oy1 = oy2 - height;

        const Border& top    = sides[0];
        const Border& right  = sides[1];
        const Border& bottom = sides[2];
        const Border& left   = sides[3];

        int ix1 = ox1 + left.width;
        int ix2 = ox2 - right.width;
        int iy1 = oy1 + bottom.width;
        int iy2 = oy2 - top.width;

        render_border<0>(top, ox1, ix1, ix2, ox2, oy2, iy2);
        render_border<1>(right, oy2, iy2, iy1, oy1, ox2, ix2);
        render_border<2>(bottom, ox2, ix2, ix1, ox1, oy1, iy1);
        render_border<3>(left, oy1, iy1, iy2, oy2, ox1, ix1);
    }

private:
    dom::ref<dom_impl::HTMLDocument> m_dom_doc;

    int m_viewport_height;
    const TextureDecl m_dashed_texture_decl;
    TextureUse m_dashed_texture;
    const TextureDecl m_dotted_texture_decl;
    TextureUse m_dotted_texture;

    std::weak_ptr<ModalHudDialogImpl> m_weak_self;

    dom::ref<dom_impl::HTMLDocument> create_dom(PrivateApplicationState* s, double dpcm,
                                                PackedTRGB::CssLevel css_level);

    void render(TextFormatter&, int viewport_width, int viewport_height);

    template<int side_idx> void render_border(const Border& side,
                                              int s0, int s1, int s2, int s3, int t0, int t1)
    {
        if (!side.width || side.style == dom_impl::borderStyle_None)
            return;

        Vec4F rgba;
        side.color.unpack_rgba(rgba);
        glColor4f(rgba[0], rgba[1], rgba[2], rgba[3]);

        if (side.style == dom_impl::borderStyle_Solid) {
            glBegin(GL_QUADS);
            if (side_idx == 0 || side_idx == 2) {
                glVertex2i(s0, t0);
                glVertex2i(s1, t1);
                glVertex2i(s2, t1);
                glVertex2i(s3, t0);
            }
            else {
                glVertex2i(t0, s0);
                glVertex2i(t1, s1);
                glVertex2i(t1, s2);
                glVertex2i(t0, s3);
            }
            glEnd();
            return;
        }

        double len;
        if (side.style == dom_impl::borderStyle_Dashed) {
            if (!m_dashed_texture)
                m_dashed_texture = m_dashed_texture_decl.acquire();
            m_dashed_texture.bind();
            len = 2*3*side.width;
        }
        else {
            if (!m_dotted_texture)
                m_dotted_texture = m_dotted_texture_decl.acquire();
            m_dotted_texture.bind();
            len = 2*side.width;
        }
        glEnable(GL_TEXTURE_2D);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBegin(GL_QUADS);
        switch (side_idx) {
            case 0: // Top
                glTexCoord2f(0, 1);
                glVertex2i(s0, t0);
                glTexCoord2f((s1-s0)/len, 0);
                glVertex2i(s1, t1);
                glTexCoord2f((s2-s0)/len, 0);
                glVertex2i(s2, t1);
                glTexCoord2f((s3-s0)/len, 1);
                glVertex2i(s3, t0);
                break;
            case 1: // Right
                glTexCoord2f(0, 1);
                glVertex2i(t0, s0);
                glTexCoord2f((s0-s1)/len, 0);
                glVertex2i(t1, s1);
                glTexCoord2f((s0-s2)/len, 0);
                glVertex2i(t1, s2);
                glTexCoord2f((s0-s3)/len, 1);
                glVertex2i(t0, s3);
                break;
            case 2: // Bottom
                glTexCoord2f(0, 1);
                glVertex2i(s0, t0);
                glTexCoord2f((s0-s1)/len, 0);
                glVertex2i(s1, t1);
                glTexCoord2f((s0-s2)/len, 0);
                glVertex2i(s2, t1);
                glTexCoord2f((s0-s3)/len, 1);
                glVertex2i(s3, t0);
                break;
            case 3: // Left
                glTexCoord2f(0, 1);
                glVertex2i(t0, s0);
                glTexCoord2f((s1-s0)/len, 0);
                glVertex2i(t1, s1);
                glTexCoord2f((s2-s0)/len, 0);
                glVertex2i(t1, s2);
                glTexCoord2f((s3-s0)/len, 1);
                glVertex2i(t0, s3);
                break;
        }
        glEnd();
        glDisable(GL_TEXTURE_2D);
    }

    ModalHudDialogImpl(PrivateApplicationState* s, double dpcm):
        DialogImpl{s},
        m_dom_doc{create_dom(s, dpcm, PackedTRGB::css3)},
        m_dashed_texture_decl{s->get_dashed_texture_decl()},
        m_dotted_texture_decl{s->get_dotted_texture_decl()}
    {
    }
};



dom::ref<dom_impl::HTMLDocument> ModalHudDialogImpl::create_dom(PrivateApplicationState* s, double /*dpcm*/,
                                                                PackedTRGB::CssLevel /*css_level*/)
{
    using namespace dom_impl;
    dom::ref<HTMLDocument> doc(new HTMLDocument(s->get_dom_impl(),
                                                HTMLDocument::mode_HTML_Strict));
    dom::ref<dom::Element> root = doc->createElement(dom::str_from_cloc(L"HTML"));
    root->appendChild(doc->createElement(dom::str_from_cloc(L"BODY")));
    doc->appendChild(root);
    return doc;
}



void ModalHudDialogImpl::render(TextFormatter&, int viewport_width, int viewport_height)
{
/*
    Each Element may or may not have an associcated Box.
    The box is owned by the Element.
    The box also has a pointer to the owning element.
    The box has a parent unless it is the root box.
    When the box is destroyed, it removes itself from the parent.

    PROBLEM with anonymous boxes: Boxes are owned by their parent. If
    a box has a reference to an Element and is destroyed, it will
    first nullify the elements reference to it.

    A box stores its structural layout (left, top, width, height). The
    combination of this information and the computed style is always
    enough to efficiently derive the used style for any element.
*/

    bool shrink_to_fit = true;
    m_dom_doc->update_render_tree(viewport_width, viewport_height, shrink_to_fit);
    int width  = m_dom_doc->get_root_box_width();
    int height = m_dom_doc->get_root_box_height();
    int x = (viewport_width  - width)  / 2;
    int y = (viewport_height - height) / 2;
    m_viewport_height = viewport_height;
    m_dashed_texture = m_dotted_texture = TextureUse(); // Clear
    m_dom_doc->render(this, x, y);
}



std::shared_ptr<Dialog> PrivateApplicationState::new_modal_hud_dialog()
{
    // FIXME: The calculation below is in accordance with CSS2.1, but
    // we should also support the true value which can be obtained
    // from the display connection.
    double ptpd  = 0.75; // Points per dot  (a dot is the same as a pixel)
    double ptpin = 72;   // Points per inch
    double cmpin = 2.54; // Centimeters per inch
    double dpcm = ptpin / cmpin / ptpd; // Dots per centimeter
    return ModalHudDialogImpl::create(this, dpcm);
}

} // anonymous namespace



namespace archon {
namespace render {

class Application::PrivateState: public PrivateApplicationState {
public:
    static std::shared_ptr<PrivateState> create(const Config& cfg, const std::locale& locale,
                                                TextureCache* texture_cache,
                                                std::unique_ptr<FontCache> font_cache)
    {
        std::shared_ptr<PrivateState> s =
            std::make_shared<PrivateState>(cfg, locale, texture_cache,
                                           std::move(font_cache)); // Throws
        s->m_weak_self = s;
        return s;
    }

    PrivateState(const Config& cfg, const std::locale& locale,
                 TextureCache* texture_cache, std::unique_ptr<FontCache> font_cache):
        PrivateApplicationState{cfg, locale, texture_cache, std::move(font_cache)}
    {
    }
};


void Application::set_window_size(int w, int h)
{
    m_win->set_size(w,h);
}


void Application::set_window_pos(int x, int y)
{
    m_win->set_position(x,y);
    m_win_x = x;
    m_win_y = y;
    m_win_pos_set = true;
}


void Application::set_fullscreen_enabled(bool enable)
{
    m_fullscreen_mode = enable;
    m_win->set_fullscreen_enabled(enable);
}


void Application::set_headlight_enabled(bool enable)
{
    m_headlight = enable;
}


void Application::set_frame_rate(double r)
{
    m_frame_rate = r;
    auto nanos_per_frame = std::chrono::nanoseconds::rep(std::floor(1E9 / m_frame_rate));
    auto time_per_frame = std::chrono::nanoseconds(nanos_per_frame);
    m_time_per_frame = std::chrono::duration_cast<clock::duration>(time_per_frame);
//    std::cout << "Setting desired frame rate (f/s): " << m_frame_rate << std::endl;
}


void Application::set_scene_orientation(math::Rotation3 orientation)
{
    m_orientation = orientation;
    m_trackball.set_orientation(orientation);
    m_need_redraw = true;
}


void Application::set_scene_spin(math::Rotation3 rot)
{
    m_trackball.set_spin(rot, clock::now());
}


void Application::set_detail_level(double level)
{
    m_detail_level = level;
}


void Application::set_interest_size(double diameter)
{
    m_interest_size = diameter;
    m_projection_needs_update = true;
    m_need_redraw = true;
}


void Application::set_zoom_factor(double zoom)
{
    m_proj.zoom_factor = clamp(zoom, g_zoom_min, g_zoom_max);
    m_projection_needs_update = true;
    m_need_redraw = true;
}


void Application::set_eye_screen_dist(double dist)
{
    m_proj.view_dist = dist;
    m_projection_needs_update = true;
    m_need_redraw = true;
}


void Application::set_screen_dpcm(double horiz, double vert)
{
    if (0 < horiz)
        m_proj.horiz_dot_pitch = 0.01 / horiz;
    if (0 < vert)
        m_proj.vert_dot_pitch  = 0.01 / vert;
    if (0 < horiz || 0 < vert) {
        m_projection_needs_update = true;
        m_need_redraw = true;
    }
/*
    std::cout << "Horizontal dot pitch = " << m_proj.horiz_dot_pitch << " m/px  (" << m_proj.get_horiz_resol_dpi() << " dpi)" << std::endl;
    std::cout << "Vertical dot pitch   = " << m_proj.vert_dot_pitch  << " m/px  (" << m_proj.get_vert_resol_dpi()  << " dpi)" << std::endl;
*/
}


void Application::set_depth_of_field(double ratio)
{
    m_proj.far_to_near_clip_ratio = ratio;
    m_projection_needs_update = true;
    m_need_redraw = true;
}


void Application::set_wireframe_enabled(bool enable)
{
    m_wireframe_mode = enable;
}


void Application::set_axes_display_enabled(bool enable)
{
    m_axes_display = enable;
}


void Application::set_global_ambience(double intencity)
{
    m_global_ambience = intencity;
    m_need_misc_update = true;
    m_need_redraw = true;
}


void Application::set_background_color(Vec4 rgba)
{
    m_background_color = rgba;
    m_need_misc_update = true;
    m_need_redraw = true;
}


void Application::run()
{
    if (m_first_run) {
        m_initial_orientation = m_orientation;
        m_initial_interest_size = m_interest_size;
        m_initial_zoom_factor = m_proj.zoom_factor;
        m_first_run = false;
    }

    m_win->show();
    if (m_win_pos_set || m_fullscreen_mode) {
        m_conn->flush_output();
        if (m_win_pos_set)
            m_win->set_position(m_win_x, m_win_y);
        if (m_fullscreen_mode)
            m_win->set_fullscreen_enabled(true);
    }

//    RateMeter rate_meter("Frame rate (f/s): ", 10000);
//    want_redraw = true;
    using time_point = clock::time_point;
    time_point wakeup_time = clock::now();
    time_point next_tick_time = wakeup_time + m_time_per_frame;
    for (;;) {
//        rate_meter.tick();

        // The distance is not known accurately until update_gl_projection() has
        // been called.
        if (ARCHON_UNLIKELY(m_status_hud_activate_cam_dist)) {
            set_float_status(L"DIST = ", m_proj.camera_dist, 2, L"",
                             m_status_hud_activate_cam_dist_timeout);
            m_status_hud_activate_cam_dist = false;
        }

        if (ARCHON_UNLIKELY(m_status_hud_active && m_status_hud_timeout <= wakeup_time)) {
            m_status_hud_active = false;
            m_need_redraw = true;
        }

        if (m_need_redraw) {
            redraw();
            m_need_redraw = false;
        }

        // Event handlers can request `want_redraw` or even `want_redraw_immediately`. If they want redraw immediately, before_sleep() must terminate the event processing immediately.
        m_event_proc->process(next_tick_time);
        if (m_terminate)
            break;
        wakeup_time = clock::now();

        if (wakeup_time < next_tick_time)
            continue;

        next_tick_time += m_time_per_frame;
        while (next_tick_time + m_time_per_frame < wakeup_time) {
            next_tick_time += m_time_per_frame;
            ++m_num_dropped_frames;
            if (m_num_dropped_frames >= m_num_dropped_frames_report) {
                auto n = m_num_dropped_frames;
                const char* frames_text = (n == 1 ? "frame" : "frames");
                std::cerr << ""<<n<<" "<<frames_text<<" dropped\n";
                m_num_dropped_frames = 0;
                m_num_dropped_frames_report *= 2;
            }
        }

        internal_tick(wakeup_time);

        if (tick(wakeup_time))
            m_need_redraw = true;
    }

    m_win_pos_set = false;
    m_win->hide();
}


void Application::redraw()
{
    if (ARCHON_UNLIKELY(m_need_misc_update)) {
        GLfloat params[] = { GLfloat(m_global_ambience),
                             GLfloat(m_global_ambience),
                             GLfloat(m_global_ambience), 1 };
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, params);
        glClearColor(m_background_color[0], m_background_color[1],
                     m_background_color[2], m_background_color[3]);
        m_need_misc_update = false;
    }

    if (ARCHON_UNLIKELY(m_projection_needs_update)) {
        update_gl_projection();
        m_projection_needs_update = false;
    }

    render_frame();
    m_win->swap_buffers(); // Implies glFlush

    m_private_state->update();

    while (0 < m_max_gl_errors) {
        GLenum error = glGetError();
        if (!error)
            break;
        emit_gl_error(error, --m_max_gl_errors == 0);
    }
}


void Application::internal_tick(clock::time_point time)
{
    Rotation3 orientation = m_trackball.get_orientation(time);
    if (orientation != m_orientation) {
        m_orientation = orientation;
        m_need_redraw = true;
    }
}


/// \todo FIXME: Lacks propper transcoding from ISO Latin 1 to the wide
/// character encoding of 'std::wcout'. Not so easy though, because there seems
/// to be no way to query for the encoding of 'std::wcout'.
///
/// \todo FIXME: Emit to an abstract logger instead.
void Application::emit_gl_error(GLenum error, bool last)
{
#ifdef ARCHON_HAVE_GLU
    const GLubyte* ptr = gluErrorString(error);
    const GLubyte* end = ptr;
    while (*end)
        ++end;
    std::string latin1_str;
    latin1_str.reserve(end - ptr);
    while (ptr < end)
        latin1_str += char(static_cast<unsigned char>(*ptr++));
    std::cerr << "OpenGL error: " << latin1_str << "\n";
#else
    std::cerr << "OpenGL error: " << error << "\n";
#endif
    if (last)
        std::cerr << "No more OpenGL errors will be reported\n";
}


void Application::get_current_view(math::Vec3& eye, CoordSystem3x2& screen)
{
    update_proj_and_trackball();

    Mat3 rot;
    {
        Rotation3 r = m_orientation;
        r.neg();
        r.get_matrix(rot);
    }
    eye = m_proj.camera_dist * rot.col(2);

    // Describe the 2-D screen coordinate system relative to the 3-D view
    // coordinate system
    screen.basis.col(0).set(m_proj.get_near_clip_width(), 0, 0);
    screen.basis.col(1).set(0, m_proj.get_near_clip_height(), 0);
    screen.origin.set(0, 0, -m_proj.get_near_clip_dist());
    screen.translate(Vec2(-0.5));

    // Rotate and translate the screen to reflect the actual viewing position
    // and direction direction
    screen.pre_mult(CoordSystem3x3(rot, eye));
}



TextureDecl Application::declare_texture(std::string image_path, bool repeat, bool mipmap)
{
    using FilterMode = TextureCache::FilterMode;
    FilterMode filter_mode = (mipmap ? FilterMode::mipmap : FilterMode::interp);
    return m_private_state->declare_texture(image_path, repeat, filter_mode);
}


TextureCache& Application::get_texture_cache()
{
    return m_private_state->get_texture_cache();
}


FontProvider& Application::get_font_provider()
{
    return m_private_state->get_font_provider();
}


Application::Application(std::string title, const Config& cfg, const std::locale& locale,
                         Connection::Arg c, TextureCache* texture_cache,
                         std::unique_ptr<FontCache> font_cache):
    m_conn{c},
    m_private_state{PrivateState::create(cfg, locale, texture_cache, std::move(font_cache))}
{
    int key_handler_index =
        register_builtin_key_handler(&Application::key_func_shift_modifier,
                                     "Shift modifier mode",
                                     BuiltinKeyHandler::shift_modifier); // Throws
    bind_key(display::KeySym_Shift_L, key_handler_index); // Throws
    bind_key(display::KeySym_Shift_R, key_handler_index); // Throws

    key_handler_index =
        register_builtin_key_handler(&Application::key_func_quit,
                                     "Quit",
                                     BuiltinKeyHandler::quit); // Throws
    bind_key(display::KeySym_Escape, key_handler_index); // Throws
    bind_key(display::KeySym_q, key_handler_index); // Throws

    key_handler_index =
        register_builtin_key_handler(&Application::key_func_reset_view,
                                     "Reset view",
                                     BuiltinKeyHandler::reset_view); // Throws
    bind_key(display::KeySym_space, key_handler_index); // Throws

    key_handler_index =
        register_builtin_key_handler(&Application::key_func_inc_frame_rate,
                                     "Increase frame rate",
                                     BuiltinKeyHandler::inc_frame_rate); // Throws
    bind_key(display::KeySym_KP_Add, key_handler_index); // Throws

    key_handler_index =
        register_builtin_key_handler(&Application::key_func_dec_frame_rate,
                                     "Decrease frame rate",
                                     BuiltinKeyHandler::dec_frame_rate); // Throws
    bind_key(display::KeySym_KP_Subtract, key_handler_index); // Throws

    key_handler_index =
        register_builtin_key_handler(&Application::key_func_show_help,
                                     "Show help",
                                     BuiltinKeyHandler::show_help); // Throws
    bind_key(display::KeySym_h, key_handler_index); // Throws

    key_handler_index =
        register_builtin_key_handler(&Application::key_func_toggle_headlight,
                                     "Toggle headlight",
                                     BuiltinKeyHandler::toggle_headlight); // Throws
    bind_key(display::KeySym_l, key_handler_index); // Throws

    key_handler_index =
        register_builtin_key_handler(&Application::key_func_toggle_fullscreen,
                                     "Toggle fullscreen mode",
                                     BuiltinKeyHandler::toggle_fullscreen); // Throws
    bind_key(display::KeySym_f, key_handler_index); // Throws

    key_handler_index =
        register_builtin_key_handler(&Application::key_func_toggle_wireframe,
                                     "Toggle wireframe mode",
                                     BuiltinKeyHandler::toggle_wireframe); // Throws
    bind_key(display::KeySym_w, key_handler_index); // Throws

    key_handler_index =
        register_builtin_key_handler(&Application::key_func_toggle_show_axes,
                                     "Toggle X,Y,Z axes display",
                                     BuiltinKeyHandler::toggle_show_axes); // Throws
    bind_key(display::KeySym_a, key_handler_index); // Throws

    key_handler_index =
        register_builtin_key_handler(&Application::key_func_toggle_status_hud,
                                     "Toggle status HUD enable",
                                     BuiltinKeyHandler::toggle_status_hud); // Throws
    bind_key(display::KeySym_s, key_handler_index); // Throws

    if (title.empty())
        title = "Archon";
    if (!m_conn)
        m_conn = archon::display::get_default_implementation()->new_connection();

    int vis = m_conn->choose_gl_visual();

    int width = cfg.win_size[0], height = cfg.win_size[1];
    m_win = m_conn->new_window(width, height, -1, vis);
    m_win->set_title(title);

    m_cursor_normal =
        m_conn->new_cursor(::Image::load(cfg.archon_datadir+"render/viewer_interact.png"),   7,  6);
    m_cursor_trackball =
        m_conn->new_cursor(::Image::load(cfg.archon_datadir+"render/viewer_trackball.png"), 14, 14);
    m_win->set_cursor(*m_cursor_normal);

    m_event_proc = m_conn->new_event_processor(this);
    m_event_proc->register_window(m_win);

    m_ctx = m_conn->new_gl_context(-1, vis, cfg.direct_render);
//    std::cout << "Direct rendering context: " << (m_ctx->is_direct() ? "Yes" : "No") << std::endl;

    set_viewport_size(width, height);

    set_headlight_enabled(cfg.headlight);
    set_frame_rate(cfg.frame_rate);
    if (0 <= cfg.win_pos[0] && 0 <= cfg.win_pos[1])
        set_window_pos(cfg.win_pos[0], cfg.win_pos[1]);
    set_screen_dpcm(cfg.scr_dpcm[0] < 1 ? 0.01 / m_conn->get_horiz_dot_pitch() : cfg.scr_dpcm[0],
                    cfg.scr_dpcm[1] < 1 ? 0.01 / m_conn->get_vert_dot_pitch()  : cfg.scr_dpcm[1]);
    set_eye_screen_dist(cfg.eye_scr_dist);
    set_depth_of_field(cfg.depth_of_field);
    set_interest_size(cfg.interest_size);
    set_zoom_factor(cfg.zoom);
    set_detail_level(cfg.detail_level);
    set_fullscreen_enabled(cfg.fullscreen);
    set_global_ambience(cfg.ambience);
    set_background_color(cfg.bgcolor);

    m_gl_binding.acquire(m_ctx, m_win);
}


Application::~Application()
{
    if (m_one_axis_dpy_list)
        glDeleteLists(m_one_axis_dpy_list, 2);
    if (m_quadric)
        gluDeleteQuadric(m_quadric);
    if (m_status_hud_disp_list)
        glDeleteLists(m_status_hud_disp_list, 1);
}


void Application::render()
{
}


bool Application::tick(clock::time_point)
{
    return false;
}


void Application::set_viewport_size(int w, int h)
{
    m_viewport_width  = w;
    m_viewport_height = h;
    m_projection_needs_update = true;
    m_need_redraw = true;
}


void Application::update_gl_projection()
{
    update_proj_and_trackball();

    double view_plane_dist  = m_proj.get_near_clip_dist();
    double view_plane_right = m_proj.get_near_clip_width()  / 2;
    double view_plane_top   = m_proj.get_near_clip_height() / 2;
    double far_clip_dist    = m_proj.get_far_clip_dist();

/*
    std::cout << "Camera distance     = " << m_proj.camera_dist << " obj" << std::endl;
    std::cout << "Near clip distance  = " << view_plane_dist  << " obj" << std::endl;
    std::cout << "Far clip distance   = " << far_clip_dist    << " obj" << std::endl;
*/

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-view_plane_right, view_plane_right,
              -view_plane_top, view_plane_top, view_plane_dist, far_clip_dist);

    glViewport(0, 0, m_viewport_width, m_viewport_height);
}


void Application::update_proj_and_trackball()
{
    m_proj.set_viewport_size_pixels(m_viewport_width, m_viewport_height);
    m_proj.auto_dist(m_interest_size, m_proj.get_min_field_factor());
    m_trackball.set_viewport_size(m_viewport_width, m_viewport_height);
}


void Application::render_frame()
{
    // Handle headlight feature
    if (!m_headlight_blocked && m_headlight != m_headlight_prev) {
        GLboolean params[1];
        glGetBooleanv(GL_LIGHT0, params);
        GLfloat pos_params[4];
        glGetLightfv(GL_LIGHT0, GL_POSITION, pos_params);
        GLfloat pos_on_params[4]  = { 0, 0, 0, 1 };
        GLfloat pos_off_params[4] = { 0, 0, 1, 0 };
        if (params[0] != (m_headlight_prev ? GL_TRUE : GL_FALSE) ||
            !std::equal(pos_params, pos_params+4, m_headlight ? pos_off_params : pos_on_params)) {
            std::cout << "Warning: Headlight feature blocked due to conflict with application." << std::endl;
            m_headlight_blocked = true;
        }
        else {
            // Make the headlight a point light source
            glLightfv(GL_LIGHT0, GL_POSITION, m_headlight ? pos_on_params : pos_off_params);
            if (m_headlight) {
                glEnable(GL_LIGHT0);
            }
            else {
                glDisable(GL_LIGHT0);
            }
            m_headlight_prev = m_headlight;
        }
    }

    // Handle wireframe mode
    if (!m_wireframe_mode_blocked && m_wireframe_mode != m_wireframe_mode_prev) {
        GLint params[2];
        glGetIntegerv(GL_POLYGON_MODE, params);
        if (m_wireframe_mode_prev ?
            params[0] != GL_LINE || params[1] != GL_LINE :
            params[0] != GL_FILL || params[1] != GL_FILL) {
            std::cout << "Warning: Wireframe mode blocked due to conflict with application." << std::endl;
            m_wireframe_mode_blocked = true;
        }
        else {
            glPolygonMode(GL_FRONT_AND_BACK, m_wireframe_mode ? GL_LINE : GL_FILL);
            m_wireframe_mode_prev = m_wireframe_mode;
        }
    }

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    glTranslated(0, 0, -m_proj.camera_dist);
    glRotated(180/M_PI*m_orientation.angle, m_orientation.axis[0],
              m_orientation.axis[1], m_orientation.axis[2]);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_axes_display) {
        if (m_axes_display_first) {
            m_axes_display_first = false;
            if (!m_quadric) {
                m_quadric = gluNewQuadric();
                if (!m_quadric)
                    throw std::bad_alloc();
            }
            m_one_axis_dpy_list = glGenLists(2);
            m_all_axes_dpy_list = m_one_axis_dpy_list+1;
            if (!m_one_axis_dpy_list)
                throw std::runtime_error("glGenLists failed");

            double back_len = 0.1, head_len = 0.1, shaft_radius = 0.005, head_radius = 0.022;
            int shaft_slices = adjust_detail(8, 3), head_slices = adjust_detail(16, 3),
                shaft_stacks = adjust_detail(10, 1);

            glNewList(m_one_axis_dpy_list, GL_COMPILE);
            glTranslated(0, 0, -back_len);
            gluQuadricOrientation(m_quadric, GLU_INSIDE);
            gluDisk(m_quadric, 0, shaft_radius, shaft_slices, 1);
            gluQuadricOrientation(m_quadric, GLU_OUTSIDE);
            gluCylinder(m_quadric, shaft_radius, shaft_radius, 1, shaft_slices, shaft_stacks);
            glTranslated(0, 0, 1+back_len-head_len);
            gluQuadricOrientation(m_quadric, GLU_INSIDE);
            gluDisk(m_quadric, 0, head_radius, head_slices, 1);
            gluQuadricOrientation(m_quadric, GLU_OUTSIDE);
            gluCylinder(m_quadric, head_radius, 0, head_len, head_slices, 1);
            glTranslated(0, 0, -1+head_len);
            glEndList();

            glNewList(m_all_axes_dpy_list, GL_COMPILE_AND_EXECUTE);
            glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_POLYGON_BIT);
            glEnable(GL_LIGHTING);
            glEnable(GL_COLOR_MATERIAL);
            glDisable(GL_TEXTURE_2D);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
            glEnable(GL_CULL_FACE);
            glShadeModel(GL_SMOOTH);
            {
                GLfloat params[4] = { 0.5, 0.5, 0.5, 1 };
                glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, params);
            }
            {
                GLfloat params[4] = { 0.4, 0.4, 0.4, 1 };
                glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, params);
            }
            glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 32);
            // X-axis
            glColor3f(0.9, 0.2, 0.2);
            glRotated(90, 0, 1, 0);
            glCallList(m_one_axis_dpy_list);
            glRotated(-90, 0, 1, 0);
            // Y-axis
            glColor3f(0.2, 0.9, 0.2);
            glRotated(90, -1, 0, 0);
            glCallList(m_one_axis_dpy_list);
            glRotated(-90, -1, 0, 0);
            // Z-axis
            glColor3f(0.2, 0.2, 0.9);
            glCallList(m_one_axis_dpy_list);
            glPopAttrib();
            glEndList();
        }
        else {
            glCallList(m_all_axes_dpy_list);
        }
    }

    render();

    glPopMatrix();

    if (m_status_hud_active || m_private_state->has_open_dialogs())
        render_hud();
}



// Render "head-up display"
void Application::render_hud()
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, m_viewport_width, 0, m_viewport_height, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT | GL_POLYGON_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    GLint prev_tex;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev_tex);

    m_private_state->render_hud(m_viewport_width, m_viewport_height);

    if (m_status_hud_active) {
        if (m_status_hud_dirty) {
            TextFormatter& text_formatter = m_private_state->get_text_formatter();
            text_formatter.set_font_size(28);
            text_formatter.set_font_weight(1);
            text_formatter.set_text_color(Vec4F(0.1, 0, 0.376, 1));
            text_formatter.write(m_status_hud_text);
            text_formatter.format(m_private_state->status_hud_text_layout);
            text_formatter.clear();

            int margin = 16, padding_h = 4, padding_v = 1;
            int width  = ceil(m_private_state->status_hud_text_layout.get_width())  + 2*padding_h;
            int height = ceil(m_private_state->status_hud_text_layout.get_height()) + 2*padding_v;
            int x = m_viewport_width - margin - width;
            int y = margin;

            if (!m_status_hud_disp_list) {
                m_status_hud_disp_list = glGenLists(1);
                if (!m_status_hud_disp_list)
                    throw std::runtime_error("Failed to create a new OpenGL display list");
            }

            glNewList(m_status_hud_disp_list, GL_COMPILE_AND_EXECUTE);
            glTranslatef(x,y,0);
            glColor4f(1,1,0,0.7);
            glBegin(GL_QUADS);
            glVertex2i(-padding_h, -padding_v);
            glVertex2i(width,      -padding_v);
            glVertex2i(width,      height);
            glVertex2i(-padding_h, height);
            glEnd();
            m_private_state->status_hud_text_layout.render();
            glEndList();

            m_status_hud_dirty = false;
        }
        else {
            glCallList(m_status_hud_disp_list);
        }
    }

    glBindTexture(GL_TEXTURE_2D, prev_tex);
    glPopAttrib();
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
}


void Application::modify_zoom(int diff)
{
    int level = std::round(std::log(m_proj.zoom_factor) / std::log(g_zoom_step));
    set_zoom_factor(std::pow(g_zoom_step, level + diff));
    set_float_status(L"ZOOM = ", m_proj.zoom_factor, 2, L"x");
}


void Application::modify_dist(int diff)
{
    // The distance modification comes about indirectly. We modify the
    // size of the sphere of interest, and the auto-distance feature
    // makes the corresponding change in distance.
    int level = std::round(std::log(m_interest_size) / std::log(g_camera_dist_step));
    set_interest_size(std::pow(g_camera_dist_step, level + diff));
    m_status_hud_activate_cam_dist = true;
    m_status_hud_activate_cam_dist_timeout = get_status_hud_timout();
}


void Application::set_status(std::wstring text, clock::time_point timeout)
{
    if (!m_status_hud_enabled)
        return;
    m_status_hud_text = text;
    m_status_hud_dirty = true;
    activate_status(timeout);
    m_status_hud_activate_cam_dist = false;
}

void Application::set_int_status(std::wstring prefix, int value, std::wstring suffix, clock::time_point timeout)
{
    if (!m_status_hud_enabled)
        return;
    std::wostringstream out; // FIXME: Must be imbued with a proper locale, or a reusable stream should be used
    out << prefix << value << suffix;
    set_status(out.str(), timeout);
}

void Application::set_float_status(std::wstring prefix, double value, int precision,
                                   std::wstring suffix, clock::time_point timeout)
{
    if (!m_status_hud_enabled)
        return;
    std::wostringstream out; // FIXME: Must be imbued with a proper locale, or a reusable stream should be used
    out << std::fixed << std::setprecision(precision) << prefix << value << suffix;
    set_status(out.str(), timeout);
}

void Application::set_on_off_status(std::wstring prefix, bool value, clock::time_point timeout)
{
    if (!m_status_hud_enabled)
        return;
    std::wostringstream out; // FIXME: Must be imbued with a proper locale, or a reusable stream should be used
    out << prefix << L" IS " << (value ? "ON" : "OFF");
    set_status(out.str(), timeout);
}

void Application::activate_status(clock::time_point timeout)
{
    if (!m_status_hud_enabled)
        return;
    m_status_hud_active = true;
    m_need_redraw = true;
    if (timeout.time_since_epoch().count() == 0)
        timeout = get_status_hud_timout();
    if (m_status_hud_timeout < timeout)
        m_status_hud_timeout = timeout;
}


int Application::get_builtin_key_handler(BuiltinKeyHandler ident) const noexcept
{
    return m_builtin_key_handlers.at(ident);
}


int Application::register_key_handler(std::function<bool(bool down)> callback,
                                      std::string description)
{
    auto size = m_key_handlers.size();
    if (size > std::numeric_limits<int>::max())
        throw std::length_error("Too many key handlers");
    int handler_index = int(size);
    m_key_handlers.emplace_back(KeyHandler{std::move(callback), std::move(description)}); // Throws
    return handler_index;
}


void Application::on_resize(const SizeEvent& e)
{
    set_viewport_size(e.width, e.height);
    m_need_immediate_redraw = true;
    m_private_state->on_resize();
}


void Application::on_close(const Event&)
{
    m_terminate = true;
}


void Application::on_keydown(const KeyEvent& e)
{
    bool down = true;
    on_key_down_or_up(KeyIdent::from_key_sym(e.key_sym), down, e.timestamp); // Throws
}


void Application::on_keyup(const KeyEvent& e)
{
    bool down = false;
    on_key_down_or_up(KeyIdent::from_key_sym(e.key_sym), down, e.timestamp); // Throws
}


void Application::on_mousedown(const MouseButtonEvent& e)
{
    if (e.button == 1) {
        m_but1_down = true;
        m_win->set_cursor(*m_cursor_trackball);
        m_trackball.acquire(clock::now());
        m_trackball.track(e.x, e.y, e.timestamp);
        return;
    }
    if (e.button == 4) { // Mouse wheel scroll up -> approach
        if (m_key_modifier == KeyModifier::shift) {
            modify_zoom(+1);
        }
        else {
            modify_dist(-1);
        }
        m_need_redraw = true;
        return;
    }
    if (e.button == 5) { // Mouse wheel scroll down -> recede
        if (m_key_modifier == KeyModifier::shift) {
            modify_zoom(-1);
        }
        else {
            modify_dist(+1);
        }
        m_need_redraw = true;
        return;
    }
    bool down = true;
    on_key_down_or_up(KeyIdent::from_button_number(e.button), down, e.timestamp); // Throws
}


void Application::on_mouseup(const MouseButtonEvent& e)
{
    if (e.button == 1) {
        m_trackball.track(e.x, e.y, e.timestamp);
        m_trackball.release(clock::now());
        m_win->set_cursor(*m_cursor_normal);
        m_but1_down = false;
        return;
    }
    bool down = false;
    on_key_down_or_up(KeyIdent::from_button_number(e.button), down, e.timestamp); // Throws
}


void Application::on_mousemove(const MouseEvent& e)
{
    if (m_but1_down)
        m_trackball.track(e.x, e.y, e.timestamp);
}


void Application::on_show(const Event&)
{
//    std::cerr << "SHOW\n";
}


void Application::on_hide(const Event&)
{
//    std::cerr << "HIDE\n";
}


void Application::on_damage(const AreaEvent&)
{
    m_need_immediate_redraw = true;
}


bool Application::before_sleep()
{
    if (ARCHON_UNLIKELY(m_terminate))
        return false;
    if (ARCHON_UNLIKELY(m_need_immediate_redraw)) {
        m_need_immediate_redraw = false;
        m_need_redraw = true;
        return false;
    }
    return true;
}


Application::clock::time_point Application::get_status_hud_timout()
{
    return clock::now() + std::chrono::milliseconds(g_status_hud_linger_millis);
}


void Application::on_key_down_or_up(KeyIdent key, bool down, KeySlot::Timestamp time)
{
    auto i = m_key_bindings.find(key);
    if (i == m_key_bindings.end())
        return; // Key not bound
    KeySlot& key_slot = i->second;
    int handler_index = -1;
    if (down) {
        if (key_slot.down_handler_index >= 0)
            return; // Key already down (a prior key-up event was missed)
        ++key_slot.press_count;
        KeySlot::Timestamp max_multipress_period = std::chrono::milliseconds{300};
        bool connected = (key_slot.prev_press_multiplicity > 0 &&
                          time - key_slot.prev_press_time <= max_multipress_period);
        key_slot.prev_press_time = time;
        int press_multiplicity = (connected ? key_slot.prev_press_multiplicity : 0) + 1;
        key_slot.prev_press_multiplicity = press_multiplicity;
        auto j = key_slot.modifiers.find(m_key_modifier);
        if (j == key_slot.modifiers.end())
            return; // Key not bound for modifier
        KeyModifierSlot& mod_slot = j->second;
        auto curr_press_count = key_slot.press_count;
        auto press_count_offset = curr_press_count - press_multiplicity;
        auto rend = mod_slot.multiplicities.rend();
        for (auto k = mod_slot.multiplicities.rbegin(); k != rend; ++k) {
            KeyPressMultiplicitySlot& mul_slot = k->second;
            if (mul_slot.press_count_at_last_press > press_count_offset)
                press_count_offset = mul_slot.press_count_at_last_press;
            int effective_multiplicity = int(curr_press_count - press_count_offset);
            if (k->first <= effective_multiplicity) {
                mul_slot.press_count_at_last_press = curr_press_count;
                handler_index = mul_slot.handler_index;
                break;
            }
        }
        if (handler_index == -1)
            return; // Key not bound for press multiplicity
    }
    else { // up
        if (key_slot.down_handler_index == -1)
            return; // Key already up (a prior key-down event was missed)
        handler_index = key_slot.down_handler_index;
    }
    const KeyHandler& handler = m_key_handlers[handler_index];
    if (handler.callback(down))
        m_need_redraw = true;
    if (down) {
        key_slot.down_handler_index = handler_index;
    }
    else {
        key_slot.down_handler_index = -1;
        if (key_slot.modifiers.empty())
            m_key_bindings.erase(i);
    }
}


int Application::register_builtin_key_handler(bool (Application::*handler)(bool down),
                                              std::string description, BuiltinKeyHandler ident)
{
    auto handler_2 = [this, handler](bool down) {
        return (this->*handler)(down); // Throws
    };
    std::function<bool(bool down)> handler_3{std::move(handler_2)}; // Throws (type erasure)
    int handler_index =
        register_key_handler(std::move(handler_3), std::move(description)); // Throws
    m_builtin_key_handlers[ident] = handler_index; // Throws
    return handler_index;
}


void Application::do_bind_key(KeyIdent key, KeyModifier modifier, int press_multiplicity,
                              int handler_index)
{
    KeySlot& key_slot = m_key_bindings[key]; // Throws
    key_slot.prev_press_multiplicity = 0; // Multipress barrier
    KeyModifierSlot& mod_slot = key_slot.modifiers[modifier]; // Throws
    KeyPressMultiplicitySlot& mul_slot =
        mod_slot.multiplicities[press_multiplicity]; // Throws
    mul_slot.handler_index = handler_index;
}


int Application::do_unbind_key(KeyIdent key, KeyModifier modifier,
                               int press_multiplicity) noexcept
{
    auto i = m_key_bindings.find(key);
    if (i == m_key_bindings.end())
        return -1; // Key not bound
    KeySlot& key_slot = i->second;
    auto j = key_slot.modifiers.find(modifier);
    if (j == key_slot.modifiers.end())
        return -1; // Key not bound for modifier
    KeyModifierSlot& mod_slot = j->second;
    auto k = mod_slot.multiplicities.find(press_multiplicity);
    if (k == mod_slot.multiplicities.end())
        return -1; // Key not bound for multiplicity
    const KeyPressMultiplicitySlot& mul_slot = k->second;
    int handler_index = mul_slot.handler_index;
    mod_slot.multiplicities.erase(k);
    if (mod_slot.multiplicities.empty())
        key_slot.modifiers.erase(j);
    if (key_slot.modifiers.empty() && key_slot.down_handler_index < 0)
        m_key_bindings.erase(i);
    key_slot.prev_press_multiplicity = 0; // Multipress barrier
    return handler_index;
}


int Application::do_get_key_binding(KeyIdent key, KeyModifier modifier,
                                    int press_multiplicity) noexcept
{
    auto i = m_key_bindings.find(key);
    if (i == m_key_bindings.end())
        return -1; // Key not bound
    const KeySlot& key_slot = i->second;
    auto j = key_slot.modifiers.find(modifier);
    if (j == key_slot.modifiers.end())
        return -1; // Key not bound for modifier
    const KeyModifierSlot& mod_slot = j->second;
    auto k = mod_slot.multiplicities.find(press_multiplicity);
    if (k == mod_slot.multiplicities.end())
        return -1; // Key not bound for multiplicity
    const KeyPressMultiplicitySlot& mul_slot = k->second;
    return mul_slot.handler_index;
}


bool Application::key_func_shift_modifier(bool down)
{
    m_key_modifier = (down ? KeyModifier::shift : KeyModifier::none);
    return false;
}


bool Application::key_func_quit(bool down)
{
    if (down)
        m_terminate = true;
    return false;
}


bool Application::key_func_reset_view(bool down)
{
    if (down) {
        m_trackball.set_orientation(m_initial_orientation);
        set_interest_size(m_initial_interest_size);
        set_zoom_factor(m_initial_zoom_factor);
        set_status(L"RESET VIEW");
        return true; // Need refresh
    }
    return false;
}


bool Application::key_func_inc_frame_rate(bool down)
{
    if (down) {
        set_frame_rate(m_frame_rate * 2);
        set_float_status(L"FRAME RATE = ", m_frame_rate);
        return true; // Need refresh
    }
    return false;
}


bool Application::key_func_dec_frame_rate(bool down)
{
    if (down) {
        set_frame_rate(m_frame_rate / 2);
        set_float_status(L"FRAME RATE = ", m_frame_rate);
        return true; // Need refresh
    }
    return false;
}


bool Application::key_func_show_help(bool down)
{
    if (down) {
        m_private_state->open_help_hud();
        return false; // FIXME: Should this be true?
    }
    return false;
}


bool Application::key_func_toggle_headlight(bool down)
{
    if (down) {
        set_on_off_status(L"HEADLIGHT", m_headlight ^= true);
        return true; // Need refresh
    }
    return false;
}


bool Application::key_func_toggle_fullscreen(bool down)
{
    if (down) {
        m_win->set_fullscreen_enabled(m_fullscreen_mode ^= true);
        return true; // Need refresh
    }
    return false;
}


bool Application::key_func_toggle_wireframe(bool down)
{
    if (down) {
        set_on_off_status(L"WIREFRAME", m_wireframe_mode ^= true);
        return true; // Need refresh
    }
    return false;
}


bool Application::key_func_toggle_show_axes(bool down)
{
    if (down) {
        set_on_off_status(L"AXES", m_axes_display ^= true);
        return true; // Need refresh
    }
    return false;
}


bool Application::key_func_toggle_status_hud(bool down)
{
    if (down) {
        if (m_status_hud_enabled) {
            set_on_off_status(L"STATUS", false);
            m_status_hud_enabled = false;
        }
        else {
            m_status_hud_enabled = true;
            set_on_off_status(L"STATUS", true);
        }
        return true; // Need refresh
    }
    return false;
}



Application::Config::Config():
    archon_datadir(get_value_of(build_config_param_DataDir))
{
    std::string v = sys::getenv("ARCHON_DATADIR");
    if (!v.empty()) {
        archon_datadir = v;
        if (v[v.size()-1] != '/')
            archon_datadir += "/";
    }
}


void Application::Config::populate(core::ConfigBuilder& cfg)
{
    cfg.add_param("f", "frame-rate", frame_rate,
                  "The initial frame rate. The frame rate marks the upper limit of frames "
                  "per second");
    cfg.add_param("s", "win-size", win_size,
                  "The initial size (width, height) in pixels of the windows contents area");
    cfg.add_param("p", "win-pos", win_pos,
                  "The initial position (x,y) in pixels of the upper left corner of "
                  "the outside window frame, relative to the upper left corner of the screen.\n"
                  "If any of the two coordinates are negative, both coordinates are ignored, "
                  "and the window manager will choose the initial position");
    cfg.add_param("r", "scr-dpcm", scr_dpcm,
                  "The resolution (horizontal, vertical) of the target screen in dots per "
                  "centimeter. If the value in one direction is zero or negative, then the "
                  "effective value in that direction will be determinaed automatically, "
                  "which may, or may not yield an accurate result.\n"
                  "To translate from dots per inch (dpi) to dots per centimeter, divide by "
                  "2.54 cm/in.\n"
                  "Specifying the wrong values here will produce the wrong field of view, "
                  "which in turn will produce the wrong aspect ratio between the Z-axis and "
                  "the X-Y-plane, which in turn leads to the depth effect appearing either "
                  "stretched or squeezed. It may also produce the wrong aspect ratio between "
                  "the X and Y-axes, which will lead to circles in the X-Y-plane appearing "
                  "egg-shaped");
    cfg.add_param("e", "eye-scr-dist", eye_scr_dist,
                  "The initial physical distance in meters between your eyes and the screen. "
                  "Specifying the wrong distance here will produce the wrong field of view, "
                  "which in turn will produce the wrong aspect ratio between the Z-axis "
                  "and the X-Y plane, which in turn leads to the depth effect appearing "
                  "either stretched or squeezed");
    cfg.add_param("d", "depth-of-field", depth_of_field,
                  "The initial depth of field. The depth of field is the ratio between the "
                  "depth of the near and the far clipping planes. It must be greater than 1. "
                  "Smaller values produce more accurate depth tests but makes it more likely "
                  "that your scene will be clipped");
    cfg.add_param("i", "interest-size", interest_size,
                  "The diameter of the initial sphere of interest in global modelview "
                  "coordinates. By default, the viewing frustum will be made as narrow as "
                  "possible while it still contains the sphere of interest completely.");
    cfg.add_param("z", "zoom", zoom, "Set the zoom factor. When you double the zoom factor, "
                  "you double the size of the on-screen projections of scene features.");
    cfg.add_param("l", "detail-level", detail_level,
                  "The initial level of detail. The level of detail controls the general "
                  "quality of the rendering, for example, by adjusting the number of faces "
                  "used to render a curved surface. A value of 1 corresponds to the normal "
                  "level of detail, while a value of 2 corresponds to twice the normal level "
                  "of detail. Any value is allowed");
    cfg.add_param("D", "direct-render", direct_render,
                  "Attempt to create a direct rendering contexts to gain performance. "
                  "This may fail, in which case, there will be a silent fallback to indirect "
                  "rendering");
    cfg.add_param("F", "fullscreen", fullscreen, "Open all windows in fullscreen mode.");
    cfg.add_param("H", "headlight", headlight, "Turn on the headlight.");
    cfg.add_param("a", "ambience", ambience,
                  "The global ambient intencity. For each shaded pixel, this value times the "
                  "ambient color of the material is aded to the final color of the pixel");
    cfg.add_param("b", "bgcolor", bgcolor, "The background color specified as a RGBA quadruple");
    cfg.add_param("R", "glyph-resol", glyph_resol, "Set an alternative glyph resolution to be "
                  "used by the default font provider. This is actually the resulution of "
                  "the EM-square, and fractional values are allowed.");
    cfg.add_param("M", "glyph-mipmap", glyph_mipmap, "Enable mipmapping on glyph textures "
                  "generated by the default font provider.");
    cfg.add_param("T", "glyph-save", glyph_save,
                  "Save all glyph textures generated by the default font provider as images.");
    cfg.add_param("", "archon-datadir", archon_datadir, "The path to the directory in which "
                  "the idiosyncratic read-only architecture-independent data objects used by "
                  "the Archon libraries are installed. It must be specified with a trailing "
                  "slash.");
}


} // namespace render
} // namespace archon
