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
#include <cmath>
#include <algorithm>
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

#include <archon/core/functions.hpp>
#include <archon/core/weak_ptr.hpp>
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
using namespace archon::Render;
namespace dom = archon::dom;
namespace DomImpl = archon::DomImpl;


namespace {

typedef archon::Render::TextFormatter TextFormatter; // Resolving ambiguity


const double zoom_step = std::pow(2, 1.0 / 8); // 8 steps to double
const double zoom_min  = 0.1;
const double zoom_max  = 32;

const double camera_dist_step = std::pow(2, 1.0 / 8); // 8 steps to double

const long status_hud_linger_millis = 1000;



class DialogImpl;


class PrivateApplicationState {
public:
    void open_help_hud()
    {
        Dialog::Ptr dialog = new_modal_hud_dialog();
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

    Dialog::Ptr new_modal_hud_dialog();

    void open_dialog(const SharedPtr<DialogImpl>&);
    void close_dialog(const SharedPtr<DialogImpl>&);

    bool has_open_dialogs() const { return !open_dialogs.empty(); }

    void render_hud(int viewport_width, int viewport_height);

    void on_resize();

    /// Can by called at any time, also when an OpenGL context is not
    /// bound.
    void recycle_display_list(GLuint disp_list)
    {
        available_display_lists.push_back(disp_list);
    }

    TextureCache& get_texture_cache()
    {
        ensure_texture_cache();
        return *m_texture_cache;
    }

    TextureDecl declare_texture(std::string image_path, bool repeat, TextureCache::FilterMode f)
    {
        ensure_texture_cache();
        UniquePtr<TextureSource> src(new TextureFileSource(image_path));
        GLenum wrap = repeat ? GL_REPEAT : GL_CLAMP;
        return m_texture_cache->declare(src, wrap, wrap, f);
    }

    TextureDecl declare_texture(Image::ConstRefArg img, std::string name, bool repeat,
                                TextureCache::FilterMode f)
    {
        ensure_texture_cache();
        UniquePtr<TextureSource> src(new TextureImageSource(img, name));
        GLenum wrap = repeat ? GL_REPEAT : GL_CLAMP;
        return m_texture_cache->declare(src, wrap, wrap, f);
    }

    FontProvider* get_font_provider()
    {
        if (!font_provider) {
            ensure_font_cache();
            ensure_texture_cache();
            font_provider.reset(new FontProvider(font_cache, *m_texture_cache, glyph_resolution,
                                                 glyph_mipmapping, save_glyph_textures));
        }
        return font_provider.get();
    }

    TextFormatter& get_text_formatter()
    {
        if (!text_formatter)
            text_formatter.reset(new TextFormatter(get_font_provider()));
        return *text_formatter;
    }

    TextureDecl get_dashed_texture_decl()
    {
        if (!dashed_texture_decl) {
            unsigned char buffer[4] = {
                std::numeric_limits<unsigned char>::max(),
                std::numeric_limits<unsigned char>::max(),
                0, 0
            };
            Image::Ref img = Image::copy_image_from(buffer, 2, 1, ColorSpace::get_Lum(), true);
            dashed_texture_decl = declare_texture(img, "Dashed pattern", true,
                                                  TextureCache::filter_mode_Nearest);
        }
        return dashed_texture_decl;
    }

    TextureDecl get_dotted_texture_decl()
    {
        if (!dotted_texture_decl) {
            Image::Ref img = Image::load(resource_dir + "render/dotted.png");
            dotted_texture_decl = declare_texture(img, "Dotted pattern", true,
                                                  TextureCache::filter_mode_Mipmap);
        }
        return dotted_texture_decl;
    }

    // Called with bound OpenGL context
    void update()
    {
        typedef std::vector<GLuint>::iterator iter;
        iter end = available_display_lists.end();
        for (iter i = available_display_lists.begin(); i != end; ++i) {
std::cout << "*";
            glDeleteLists(*i, 1);
        }

        if (m_texture_cache)
            m_texture_cache->update();
    }

    TextLayout status_hud_text_layout;

    DomImpl::HTMLImplementation* get_dom_impl()
    {
        if (!dom_impl)
            dom_impl.reset(new DomImpl::HTMLImplementation);
        return dom_impl.get();
    }

    PrivateApplicationState(const Application::Config& cfg, const std::locale& l,
                            TextureCache* texture_cache, FontCache::Arg font):
        resource_dir(cfg.archon_datadir),
        loc(l),
        utf16_string_codec(loc),
        m_texture_cache(texture_cache),
        font_cache(font),
        glyph_resolution(cfg.glyph_resol),
        glyph_mipmapping(cfg.glyph_mipmap),
        save_glyph_textures(cfg.glyph_save)
    {
    }

    virtual ~PrivateApplicationState()
    {
        status_hud_text_layout.clear();
    }

    const WeakPtr<PrivateApplicationState>& get_weak_self()
    {
        return weak_self;
    }

protected:
    WeakPtr<PrivateApplicationState> weak_self;

private:
    typedef std::list<SharedPtr<DialogImpl> > OpenDialogs;
    OpenDialogs open_dialogs;

    std::vector<GLuint> available_display_lists;

    StringUtf16 u16(const std::string& s) const
    {
        StringUtf16 t;
        utf16_string_codec.encode_narrow(s,t);
        return t;
    }

    StringUtf16 u16(const std::wstring& s) const
    {
        StringUtf16 t;
        if (!utf16_string_codec.encode(s,t))
            throw std::runtime_error("UTF-16 encode");
        return t;
    }

    std::string narrow_from_u16(const StringUtf16& s) const
    {
        std::string t;
        if (!utf16_string_codec.decode_narrow(s,t))
            throw std::runtime_error("UTF-16 decode");
        return t;
    }

    void ensure_font_cache()
    {
        if (!font_cache) {
            FontLoader::Ptr loader = new_font_loader(resource_dir + "font/");
            FontList::Ptr list = new_font_list(loader);
            font_cache = new_font_cache(list);
        }
    }

    void ensure_texture_cache()
    {
        if (!m_texture_cache) {
            m_texture_cache_owner = make_texture_cache(); // Throws
            m_texture_cache = m_texture_cache_owner.get();
        }
    }

    const std::string resource_dir;
    const std::locale loc;
    const CharEnc<CharUtf16> utf16_string_codec;

    TextureCache* m_texture_cache = nullptr;
    FontCache::Ptr font_cache;
    const Vec2F glyph_resolution;
    const bool glyph_mipmapping, save_glyph_textures;
    UniquePtr<FontProvider> font_provider;
    UniquePtr<TextFormatter> text_formatter;
    TextureDecl dashed_texture_decl, dotted_texture_decl;

    dom::ref<DomImpl::HTMLImplementation> dom_impl;

    std::unique_ptr<TextureCache> m_texture_cache_owner;
};



class DialogImpl: public Dialog {
public:
    DialogImpl(PrivateApplicationState* s):
        state(s->get_weak_self())
    {
    }

    ~DialogImpl()
    {
        if (disp_list) {
            if (SharedPtr<PrivateApplicationState> s = state.lock())
                s->recycle_display_list(disp_list);
        }
    }

protected:
    void mark_dirty() { dirty = true; }

    virtual void render(TextFormatter&, int viewport_width, int viewport_height) = 0;

    const WeakPtr<PrivateApplicationState> state;

private:
    friend class PrivateApplicationState;

    bool is_open = false;
    bool dirty = true;

    // Name of the OpenGL display list that renders this HUD dialog,
    // or zero if no list has been created yet.
    GLuint disp_list = 0;
};



void PrivateApplicationState::open_dialog(const SharedPtr<DialogImpl>& d)
{
    if (d->is_open)
        return;
    open_dialogs.push_back(d);
    d->is_open = true;
}

void PrivateApplicationState::close_dialog(const SharedPtr<DialogImpl>& d)
{
    if (!d->is_open)
        return;
    open_dialogs.remove(d);
    d->is_open = false;
}


void PrivateApplicationState::render_hud(int viewport_width, int viewport_height)
{
    typedef OpenDialogs::iterator iter;
    iter end = open_dialogs.end();
    for (iter i = open_dialogs.begin(); i != end; ++i) {
        DialogImpl* dlg = i->get();
        if (dlg->dirty) {
            if (!dlg->disp_list) {
                dlg->disp_list = glGenLists(1);
                if (!dlg->disp_list)
                    throw std::runtime_error("Failed to create a new OpenGL display list");
            }

            glNewList(dlg->disp_list, GL_COMPILE_AND_EXECUTE);
            dlg->render(get_text_formatter(), viewport_width, viewport_height);
            glEndList();

            dlg->dirty = false;
        }
        else {
            glCallList(dlg->disp_list);
        }
    }
}


void PrivateApplicationState::on_resize()
{
    typedef OpenDialogs::iterator iter;
    iter end = open_dialogs.end();
    for (iter i = open_dialogs.begin(); i != end; ++i)
        i->get()->dirty = true;
}








// FIXME: Overlaps with DomRenderer in dom_renderer.hpp
class ModalHudDialogImpl: public DialogImpl, public DomImpl::Renderer {
public:
    void show() override
    {
        SharedPtr<DialogImpl> d(this->weak_self);
        if (SharedPtr<PrivateApplicationState> s = state.lock())
            s->open_dialog(d);
    }

    void hide() override
    {
        SharedPtr<DialogImpl> d(this->weak_self);
        if (SharedPtr<PrivateApplicationState> s = state.lock())
            s->close_dialog(d);
    }

    dom::ref<dom::html::HTMLDocument> get_dom() override
    {
        return dom_doc;
    }

    static SharedPtr<ModalHudDialogImpl> create(PrivateApplicationState* s, double dpcm)
    {
        SharedPtr<ModalHudDialogImpl> d(new ModalHudDialogImpl(s, dpcm));
        d->weak_self = d;
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
        int y2 = viewport_height - y;
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
        int oy2 = viewport_height - y;
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
    dom::ref<DomImpl::HTMLDocument> create_dom(PrivateApplicationState* s, double dpcm,
                                               PackedTRGB::CssLevel css_level);

    void render(TextFormatter&, int viewport_width, int viewport_height);

    template<int side_idx> void render_border(const Border& side,
                                              int s0, int s1, int s2, int s3, int t0, int t1)
    {
        if (!side.width || side.style == DomImpl::borderStyle_None)
            return;

        Vec4F rgba;
        side.color.unpack_rgba(rgba);
        glColor4f(rgba[0], rgba[1], rgba[2], rgba[3]);

        if (side.style == DomImpl::borderStyle_Solid) {
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
        if (side.style == DomImpl::borderStyle_Dashed) {
            if (!dashed_texture)
                dashed_texture = dashed_texture_decl.acquire();
            dashed_texture.bind();
            len = 2*3*side.width;
        }
        else {
            if (!dotted_texture)
                dotted_texture = dotted_texture_decl.acquire();
            dotted_texture.bind();
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
        DialogImpl(s), dom_doc(create_dom(s, dpcm, PackedTRGB::css3)),
        dashed_texture_decl(s->get_dashed_texture_decl()),
        dotted_texture_decl(s->get_dotted_texture_decl()) {}

    dom::ref<DomImpl::HTMLDocument> dom_doc;

    int viewport_height;
    const TextureDecl dashed_texture_decl;
    TextureUse dashed_texture;
    const TextureDecl dotted_texture_decl;
    TextureUse dotted_texture;

    WeakPtr<ModalHudDialogImpl> weak_self;
};



dom::ref<DomImpl::HTMLDocument> ModalHudDialogImpl::create_dom(PrivateApplicationState* s, double /*dpcm*/,
                                                               PackedTRGB::CssLevel /*css_level*/)
{
    using namespace DomImpl;
    dom::ref<HTMLDocument> doc(new HTMLDocument(s->get_dom_impl(),
                                                HTMLDocument::mode_HTML_Strict));
    dom::ref<dom::Element> root = doc->createElement(dom::str_from_cloc(L"HTML"));
    root->appendChild(doc->createElement(dom::str_from_cloc(L"BODY")));
    doc->appendChild(root);
    return doc;
}



void ModalHudDialogImpl::render(TextFormatter& /*formatter*/,
                                int viewport_width, int viewport_height)
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
    dom_doc->update_render_tree(viewport_width, viewport_height, shrink_to_fit);
    int width  = dom_doc->get_root_box_width();
    int height = dom_doc->get_root_box_height();
    int x = (viewport_width  - width)  / 2;
    int y = (viewport_height - height) / 2;
    this->viewport_height = viewport_height;
    dashed_texture = dotted_texture = TextureUse(); // Clear
    dom_doc->render(this, x, y);
}



Dialog::Ptr PrivateApplicationState::new_modal_hud_dialog()
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
namespace Render {

class Application::PrivateState: public PrivateApplicationState {
public:
    static archon::core::SharedPtr<PrivateState>
    create(const Config& cfg, const std::locale& loc,
           TextureCache* texture_cache, FontCache::Arg font_cache)
    {
        SharedPtr<PrivateState> s(new PrivateState(cfg, loc, texture_cache, font_cache));
        s->weak_self = s;
        return s;
    }

private:
    PrivateState(const Config& cfg, const std::locale& loc,
                 TextureCache* texture_cache, FontCache::Arg font_cache):
        PrivateApplicationState(cfg, loc, texture_cache, font_cache)
    {
    }
};


void Application::set_window_size(int w, int h)
{
    win->set_size(w,h);
}


void Application::set_window_pos(int x, int y)
{
    win->set_position(x,y);
    win_x = x;
    win_y = y;
    win_pos_set = true;
}


void Application::set_fullscreen_enabled(bool enable)
{
    fullscreen_mode = enable;
    win->set_fullscreen_enabled(enable);
}


void Application::set_headlight_enabled(bool enable)
{
    headlight = enable;
}


void Application::set_frame_rate(double r)
{
    frame_rate = r;
    time_per_frame.set_as_seconds_float(1/frame_rate);
//    std::cout << "Setting desired frame rate (f/s): " << frame_rate << std::endl;
}


void Application::set_scene_orientation(math::Rotation3 rot)
{
    trackball.set_orientation(rot);
}


void Application::set_scene_spin(math::Rotation3 rot)
{
    trackball.set_spin(rot, Time::now());
}


void Application::set_detail_level(double level)
{
    detail_level = level;
}


void Application::set_interest_size(double diameter)
{
    interest_size = diameter;
    projection_needs_update = true;
}


void Application::set_zoom_factor(double zoom)
{
    proj.zoom_factor = clamp(zoom, zoom_min, zoom_max);
    projection_needs_update = true;
}


void Application::set_eye_screen_dist(double dist)
{
    proj.view_dist = dist;
    projection_needs_update = true;
}


void Application::set_screen_dpcm(double horiz, double vert)
{
    if (0 < horiz)
        proj.horiz_dot_pitch = 0.01 / horiz;
    if (0 < vert)
        proj.vert_dot_pitch  = 0.01 / vert;
    if (0 < horiz || 0 < vert)
        projection_needs_update = true;
/*
    std::cout << "Horizontal dot pitch = " << proj.horiz_dot_pitch << " m/px  (" << proj.get_horiz_resol_dpi() << " dpi)" << std::endl;
    std::cout << "Vertical dot pitch   = " << proj.vert_dot_pitch  << " m/px  (" << proj.get_vert_resol_dpi()  << " dpi)" << std::endl;
*/
}


void Application::set_depth_of_field(double ratio)
{
    proj.far_to_near_clip_ratio = ratio;
    projection_needs_update = true;
}


void Application::set_wireframe_enabled(bool enable)
{
    wireframe_mode = enable;
}


void Application::set_axes_display_enabled(bool enable)
{
    axes_display = enable;
}


void Application::set_global_ambience(double intencity)
{
    global_ambience = intencity;
    need_misc_update = true;
}


void Application::set_background_color(Vec4 rgba)
{
    background_color = rgba;
    need_misc_update = true;
}



void Application::run()
{
    if (first_run) {
        initial_rotation = trackball.get_orientation(Time::now());
        initial_interest_size = interest_size;
        initial_zoom_factor = proj.zoom_factor;
        first_run = false;
    }

    win->show();
    if (win_pos_set || fullscreen_mode) {
        conn->flush_output();
        if (win_pos_set)
            win->set_position(win_x, win_y);
        if (fullscreen_mode)
            win->set_fullscreen_enabled(true);
    }

//    RateMeter rate_meter("Frame rate (f/s): ", 10000);
    bool lagging_frames = false;
    Time time = Time::now();
    for (;;) {
//        rate_meter.tick();

        if(need_misc_update) {
            GLfloat params[] = { GLfloat(global_ambience),
                                 GLfloat(global_ambience),
                                 GLfloat(global_ambience), 1 };
            glLightModelfv(GL_LIGHT_MODEL_AMBIENT, params);
            glClearColor(background_color[0], background_color[1],
                         background_color[2], background_color[3]);
            need_misc_update = false;
        }

        if (projection_needs_update) {
            update_gl_projection();
            projection_needs_update = false;
        }

        // The distance is not known acurately until
        // update_gl_projection() has been called.
        if (status_hud_activate_cam_dist) {
            set_float_status(L"DIST = ", proj.camera_dist, 2, L"", status_hud_activate_cam_dist_timeout);
            status_hud_activate_cam_dist = false;
        }

        render_frame(time);
        win->swap_buffers(); // Implies glFlush

        private_state->update();

        while (0 < max_gl_errors) {
            GLenum error = glGetError();
            if (!error)
                break;
            emit_gl_error(error, --max_gl_errors == 0);
        }

        time += time_per_frame;

        Time now = Time::now();
        if (time < now) {
            time = now;
            if (!lagging_frames) {
//                std::cout << "Lagging frames" << std::endl;
                lagging_frames = true;
            }
        }
        else {
            lagging_frames = false;
        }

        try {
            event_proc->process(time);
        }
        catch (InterruptException&) {
            if (terminate)
                break;
            time = Time::now();
        }
    }

    win_pos_set = false;
    win->hide();
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
        Rotation3 r = trackball.get_orientation(Time::now());
        r.neg();
        r.get_matrix(rot);
    }
    eye = proj.camera_dist * rot.col(2);

    // Describe the 2-D screen coordinate system relative to the 3-D
    // view coordinate system
    screen.basis.col(0).set(proj.get_near_clip_width(), 0, 0);
    screen.basis.col(1).set(0, proj.get_near_clip_height(), 0);
    screen.origin.set(0, 0, -proj.get_near_clip_dist());
    screen.translate(Vec2(-0.5));

    // Rotate and translate the screen to reflect the actual viewing
    // position and direction direction
    screen.pre_mult(CoordSystem3x3(rot, eye));
}



TextureDecl Application::declare_texture(std::string image_path, bool repeat, bool mipmap)
{
    TextureCache::FilterMode filter_mode =
        mipmap ? TextureCache::filter_mode_Mipmap : TextureCache::filter_mode_Interp;
    return private_state->declare_texture(image_path, repeat, filter_mode);
}


TextureCache& Application::get_texture_cache()
{
    return private_state->get_texture_cache();
}


FontProvider* Application::get_font_provider()
{
    return private_state->get_font_provider();
}


Application::Application(std::string title, const Config& cfg, const std::locale& loc,
                         Connection::Arg c, TextureCache* texture_cache,
                         FontCache::Arg font_cache):
    conn(c),
    private_state(PrivateState::create(cfg, loc, texture_cache, font_cache))
{
    if (title.empty())
        title = "Archon";
    if (!conn)
        conn = archon::display::get_default_implementation()->new_connection();

    int vis = conn->choose_gl_visual();

    int width = cfg.win_size[0], height = cfg.win_size[1];
    win = conn->new_window(width, height, -1, vis);
    win->set_title(title);

    cursor_normal =
        conn->new_cursor(::Image::load(cfg.archon_datadir+"render/viewer_interact.png"),   7,  6);
    cursor_trackball =
        conn->new_cursor(::Image::load(cfg.archon_datadir+"render/viewer_trackball.png"), 14, 14);
    win->set_cursor(cursor_normal);

    event_proc = conn->new_event_processor(this);
    event_proc->register_window(win);

    ctx = conn->new_gl_context(-1, vis, cfg.direct_render);
//    std::cout << "Direct rendering context: " << (ctx->is_direct() ? "Yes" : "No") << std::endl;

    set_viewport_size(width, height);

    set_headlight_enabled(cfg.headlight);
    set_frame_rate(cfg.frame_rate);
    if (0 <= cfg.win_pos[0] && 0 <= cfg.win_pos[1])
        set_window_pos(cfg.win_pos[0], cfg.win_pos[1]);
    set_screen_dpcm(cfg.scr_dpcm[0] < 1 ? 0.01 / conn->get_horiz_dot_pitch() : cfg.scr_dpcm[0],
                    cfg.scr_dpcm[1] < 1 ? 0.01 / conn->get_vert_dot_pitch()  : cfg.scr_dpcm[1]);
    set_eye_screen_dist(cfg.eye_scr_dist);
    set_depth_of_field(cfg.depth_of_field);
    set_interest_size(cfg.interest_size);
    set_zoom_factor(cfg.zoom);
    set_detail_level(cfg.detail_level);
    set_fullscreen_enabled(cfg.fullscreen);
    set_global_ambience(cfg.ambience);
    set_background_color(cfg.bgcolor);

    gl_binding.acquire(ctx, win);
}


Application::~Application()
{
    if (one_axis_dpy_list)
        glDeleteLists(one_axis_dpy_list, 2);
    if (quadric)
        gluDeleteQuadric(quadric);
    if (status_hud_disp_list)
        glDeleteLists(status_hud_disp_list, 1);
}


void Application::set_viewport_size(int w, int h)
{
    viewport_width  = w;
    viewport_height = h;
    projection_needs_update = true;
}



void Application::update_gl_projection()
{
    update_proj_and_trackball();

    double view_plane_dist  = proj.get_near_clip_dist();
    double view_plane_right = proj.get_near_clip_width()  / 2;
    double view_plane_top   = proj.get_near_clip_height() / 2;
    double far_clip_dist    = proj.get_far_clip_dist();

/*
    std::cout << "Camera distance     = " << proj.camera_dist << " obj" << std::endl;
    std::cout << "Near clip distance  = " << view_plane_dist  << " obj" << std::endl;
    std::cout << "Far clip distance   = " << far_clip_dist    << " obj" << std::endl;
*/

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-view_plane_right, view_plane_right,
              -view_plane_top, view_plane_top, view_plane_dist, far_clip_dist);

    glViewport(0, 0, viewport_width, viewport_height);
}


void Application::update_proj_and_trackball()
{
    proj.set_viewport_size_pixels(viewport_width, viewport_height);
    proj.auto_dist(interest_size, proj.get_min_field_factor());
    trackball.set_viewport_size(viewport_width, viewport_height);
}



void Application::render_frame(Time now)
{
    // Handle headlight feature
    if (!headlight_blocked && headlight != headlight_prev) {
        GLboolean params[1];
        glGetBooleanv(GL_LIGHT0, params);
        GLfloat pos_params[4];
        glGetLightfv(GL_LIGHT0, GL_POSITION, pos_params);
        GLfloat pos_on_params[4]  = { 0, 0, 0, 1 };
        GLfloat pos_off_params[4] = { 0, 0, 1, 0 };
        if (params[0] != (headlight_prev ? GL_TRUE : GL_FALSE) ||
            !std::equal(pos_params, pos_params+4, headlight ? pos_off_params : pos_on_params)) {
            std::cout << "Warning: Headlight feature blocked due to conflict with application." << std::endl;
            headlight_blocked = true;
        }
        else {
            // Make the headlight a point light source
            glLightfv(GL_LIGHT0, GL_POSITION, headlight ? pos_on_params : pos_off_params);
            if (headlight) {
                glEnable(GL_LIGHT0);
            }
            else {
                glDisable(GL_LIGHT0);
            }
            headlight_prev = headlight;
        }
    }

    // Handle wireframe mode
    if (!wireframe_mode_blocked && wireframe_mode != wireframe_mode_prev) {
        GLint params[2];
        glGetIntegerv(GL_POLYGON_MODE, params);
        if (wireframe_mode_prev ?
            params[0] != GL_LINE || params[1] != GL_LINE :
            params[0] != GL_FILL || params[1] != GL_FILL) {
            std::cout << "Warning: Wireframe mode blocked due to conflict with application." << std::endl;
            wireframe_mode_blocked = true;
        }
        else {
            glPolygonMode(GL_FRONT_AND_BACK, wireframe_mode ? GL_LINE : GL_FILL);
            wireframe_mode_prev = wireframe_mode;
        }
    }

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    update_observer(now);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (axes_display) {
        if (axes_display_first) {
            axes_display_first = false;
            if (!quadric) {
                quadric = gluNewQuadric();
                if (!quadric)
                    throw std::bad_alloc();
            }
            one_axis_dpy_list = glGenLists(2);
            all_axes_dpy_list = one_axis_dpy_list+1;
            if (!one_axis_dpy_list)
                throw std::runtime_error("glGenLists failed");

            double back_len = 0.1, head_len = 0.1, shaft_radius = 0.005, head_radius = 0.022;
            int shaft_slices = adjust_detail(8, 3), head_slices = adjust_detail(16, 3),
                shaft_stacks = adjust_detail(10, 1);

            glNewList(one_axis_dpy_list, GL_COMPILE);
            glTranslated(0, 0, -back_len);
            gluQuadricOrientation(quadric, GLU_INSIDE);
            gluDisk(quadric, 0, shaft_radius, shaft_slices, 1);
            gluQuadricOrientation(quadric, GLU_OUTSIDE);
            gluCylinder(quadric, shaft_radius, shaft_radius, 1, shaft_slices, shaft_stacks);
            glTranslated(0, 0, 1+back_len-head_len);
            gluQuadricOrientation(quadric, GLU_INSIDE);
            gluDisk(quadric, 0, head_radius, head_slices, 1);
            gluQuadricOrientation(quadric, GLU_OUTSIDE);
            gluCylinder(quadric, head_radius, 0, head_len, head_slices, 1);
            glTranslated(0, 0, -1+head_len);
            glEndList();

            glNewList(all_axes_dpy_list, GL_COMPILE_AND_EXECUTE);
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
            glCallList(one_axis_dpy_list);
            glRotated(-90, 0, 1, 0);
            // Y-axis
            glColor3f(0.2, 0.9, 0.2);
            glRotated(90, -1, 0, 0);
            glCallList(one_axis_dpy_list);
            glRotated(-90, -1, 0, 0);
            // Z-axis
            glColor3f(0.2, 0.2, 0.9);
            glCallList(one_axis_dpy_list);
            glPopAttrib();
            glEndList();
        }
        else {
            glCallList(all_axes_dpy_list);
        }
    }

    render_scene();

    glPopMatrix();

    if (status_hud_active || private_state->has_open_dialogs()) {
        render_hud();
        if (status_hud_timeout <= now)
            status_hud_active = false;
    }
}



// Render "head-up display"
void Application::render_hud()
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, viewport_width, 0, viewport_height, -1, 1);
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

    private_state->render_hud(viewport_width, viewport_height);

    if (status_hud_active) {
        if (status_hud_dirty) {
            TextFormatter& text_formatter = private_state->get_text_formatter();
            text_formatter.set_font_size(28);
            text_formatter.set_font_boldness(1);
            text_formatter.set_text_color(Vec4F(0.1, 0, 0.376, 1));
            text_formatter.write(status_hud_text);
            text_formatter.format(private_state->status_hud_text_layout);
            text_formatter.clear();

            int margin = 16, padding_h = 4, padding_v = 1;
            int width  = ceil(private_state->status_hud_text_layout.get_width())  + 2*padding_h;
            int height = ceil(private_state->status_hud_text_layout.get_height()) + 2*padding_v;
            int x = viewport_width - margin - width;
            int y = margin;

            if (!status_hud_disp_list) {
                status_hud_disp_list = glGenLists(1);
                if (!status_hud_disp_list)
                    throw std::runtime_error("Failed to create a new OpenGL display list");
            }

            glNewList(status_hud_disp_list, GL_COMPILE_AND_EXECUTE);
            glTranslatef(x,y,0);
            glColor4f(1,1,0,0.7);
            glBegin(GL_QUADS);
            glVertex2i(-padding_h, -padding_v);
            glVertex2i(width,      -padding_v);
            glVertex2i(width,      height);
            glVertex2i(-padding_h, height);
            glEnd();
            private_state->status_hud_text_layout.render();
            glEndList();

            status_hud_dirty = false;
        }
        else {
            glCallList(status_hud_disp_list);
        }
    }

    glBindTexture(GL_TEXTURE_2D, prev_tex);
    glPopAttrib();
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
}



void Application::update_observer(Time now)
{
    glTranslated(0, 0, -proj.camera_dist);

    Rotation3 rot = trackball.get_orientation(now);
    if (rot.angle)
        glRotated(180/M_PI*rot.angle, rot.axis[0], rot.axis[1], rot.axis[2]);
}



void Application::modify_zoom(int diff)
{
    int level = archon_round(log(proj.zoom_factor) / log(zoom_step));
    set_zoom_factor(pow(zoom_step, level + diff));
    set_float_status(L"ZOOM = ", proj.zoom_factor, 2, L"x");
}


void Application::modify_dist(int diff)
{
    // The distance modification comes about indirectly. We modify the
    // size of the sphere of interest, and the auto-distance feature
    // makes the corresponding change in distance.
    int level = archon_round(log(interest_size) / log(camera_dist_step));
    set_interest_size(pow(camera_dist_step, level + diff));
    status_hud_activate_cam_dist = true;
    status_hud_activate_cam_dist_timeout = get_status_hud_timout();
}


void Application::set_status(std::wstring text, Time timeout)
{
    if (!status_hud_enabled)
        return;
    status_hud_text = text;
    status_hud_dirty = true;
    activate_status(timeout);
    status_hud_activate_cam_dist = false;
}

void Application::set_int_status(std::wstring prefix, int value, std::wstring suffix, Time timeout)
{
    if (!status_hud_enabled)
        return;
    std::wostringstream out; // FIXME: Must be imbued with a proper locale, or a reusable stream should be used
    out << prefix << value << suffix;
    set_status(out.str(), timeout);
}

void Application::set_float_status(std::wstring prefix, double value, int precision,
                                   std::wstring suffix, Time timeout)
{
    if (!status_hud_enabled)
        return;
    std::wostringstream out; // FIXME: Must be imbued with a proper locale, or a reusable stream should be used
    out << std::fixed << std::setprecision(precision) << prefix << value << suffix;
    set_status(out.str(), timeout);
}

void Application::set_on_off_status(std::wstring prefix, bool value, Time timeout)
{
    if (!status_hud_enabled)
        return;
    std::wostringstream out; // FIXME: Must be imbued with a proper locale, or a reusable stream should be used
    out << prefix << L" IS " << (value ? "ON" : "OFF");
    set_status(out.str(), timeout);
}

void Application::activate_status(Time timeout)
{
    if (!status_hud_enabled)
        return;
    status_hud_active = true;
    if (!timeout)
        timeout = get_status_hud_timout();
    if (status_hud_timeout < timeout)
        status_hud_timeout = timeout;
}

Time Application::get_status_hud_timout()
{
    return Time::now() + Time(status_hud_linger_millis, Time::millis);
}



void Application::on_resize(const SizeEvent& e)
{
    set_viewport_size(e.width, e.height);
    need_refresh = true;
    private_state->on_resize();
}


void Application::on_close(const Event&)
{
    terminate = true;
    throw InterruptException();
}


void Application::on_keydown(const KeyEvent& e)
{
    switch(e.key_sym) {
        case KeySym_Shift_L: // Modifier
            shift_left_down = true;
            break;

        case KeySym_q:
        case KeySym_Escape: // Quit event loop
            on_close(e);
            break;

        case KeySym_space:  // Reset camera configuration
            trackball.set_orientation(initial_rotation);
            set_interest_size(initial_interest_size);
            set_zoom_factor(initial_zoom_factor);
            set_status(L"RESET VIEW");
            need_refresh = true;
            break;

        case KeySym_KP_Add: // Increase frame rate
            set_frame_rate(frame_rate * 2);
            set_float_status(L"FRAME RATE = ", frame_rate);
            need_refresh = true;
            break;

        case KeySym_KP_Subtract:   // Decrease frame rate
            set_frame_rate(frame_rate / 2);
            set_float_status(L"FRAME RATE = ", frame_rate);
            need_refresh = true;
            break;

        case KeySym_h:      // Open help window
            private_state->open_help_hud();
            break;

        case KeySym_l:      // Toggle headlight
            set_on_off_status(L"HEADLIGHT", headlight ^= true);
            need_refresh = true;
            break;

        case KeySym_f:      // Toggle fullscreen mode
            win->set_fullscreen_enabled(fullscreen_mode ^= true);
            need_refresh = true;
            break;

        case KeySym_w:      // Toggle wireframe mode
            set_on_off_status(L"WIREFRAME", wireframe_mode ^= true);
            need_refresh = true;
            break;

        case KeySym_a:      // Toggle X,Y,Z axes display
            set_on_off_status(L"AXES", axes_display ^= true);
            need_refresh = true;
            break;

        case KeySym_s:      // Toggle status HUD enable
            if (status_hud_enabled) {
                set_on_off_status(L"STATUS", false);
                status_hud_enabled = false;
            }
            else {
                status_hud_enabled = true;
                set_on_off_status(L"STATUS", true);
            }
            need_refresh = true;
            break;

        default: {
            KeyHandlers::iterator i = key_handlers.find(e.key_sym);
            if (i != key_handlers.end() && i->second.first->handle(this, true))
                need_refresh = true;
            break;
        }
    }
}


void Application::on_keyup(const KeyEvent& e)
{
    switch(e.key_sym) {
        case KeySym_Shift_L: // Modifier
            shift_left_down = false;
            break;
        default: {
            KeyHandlers::iterator i = key_handlers.find(e.key_sym);
            if (i != key_handlers.end() && i->second.first->handle(this, false))
                need_refresh = true;
        }
    }
}


void Application::on_mousedown(const MouseButtonEvent& e)
{
    if (e.button == 1) {
        but1_down = true;
        win->set_cursor(cursor_trackball);
        trackball.acquire(Time::now());
        trackball.track(e.x, e.y, e.time);
    }
    if (e.button == 4) { // Mouse wheel scroll up -> approach
        if (shift_left_down) {
            modify_zoom(+1);
        }
        else {
            modify_dist(-1);
        }
        need_refresh = true;
    }
    if (e.button == 5) { // Mouse wheel scroll down -> recede
        if (shift_left_down) {
            modify_zoom(-1);
        }
        else {
            modify_dist(+1);
        }
        need_refresh = true;
    }
}


void Application::on_mouseup(const MouseButtonEvent& e)
{
    if (e.button == 1) {
        trackball.track(e.x, e.y, e.time);
        trackball.release(Time::now());
        win->set_cursor(cursor_normal);
        but1_down = false;
    }
}


void Application::on_mousemove(const MouseEvent& e)
{
    if (but1_down)
        trackball.track(e.x, e.y, e.time);
}


void Application::on_show(const Event& )
{
//    std::cerr << "SHOW\n";
}


void Application::on_hide(const Event&)
{
//    std::cerr << "HIDE\n";
}


void Application::on_damage(const AreaEvent&)
{
    need_refresh = true;
}


void Application::before_sleep()
{
    if (need_refresh) {
        need_refresh = false;
        throw InterruptException();
    }
}


void Application::register_key_handler(KeySym key, UniquePtr<KeyHandlerBase> handler,
                                       std::string descr)
{
    std::pair<KeyHandlers::iterator, bool> r =
        key_handlers.insert(std::make_pair(key, std::make_pair(handler.get(), descr)));
    if (!r.second)
        throw KeyHandlerConflictException("Multiple registrations for key '"+
                                          event_proc->get_key_sym_name(key)+"'");
    try {
        key_handler_owner.push_back(handler);
    }
    catch (...) {
        key_handlers.erase(r.first);
    }
}



Application::Config::Config():
    archon_datadir(get_value_of(build_config_param_DataDir))
{
    std::string v = Sys::getenv("ARCHON_DATADIR");
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


} // namespace Render
} // namespace archon
