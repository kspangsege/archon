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

#ifndef ARCHON_RENDER_DOM_RENDERER_HPP
#define ARCHON_RENDER_DOM_RENDERER_HPP

#include <string>

#include <archon/features.h>
#include <archon/dom/impl/render.hpp>
#include <archon/render/texture_cache.hpp>


namespace archon {
namespace Render {

// FIXME: Overlaps with ModalHudDialogImpl in app.cpp
class DomRenderer: public DomImpl::Renderer {
public:
    DomRenderer(TextureCache& cache, const std::string& resource_dir);

    void set_viewport_height(int);

    void filled_box(int x, int y, int width, int height, Util::PackedTRGB color) override;
    void border_box(int x, int y, int width, int height, const Border* sides) override;

private:
    template<int side_idx>
    void render_border(const Border& side, int s0, int s1, int s2, int s3, int t0, int t1);

    int m_viewport_height = 0;
    TextureDecl const m_dashed_texture_decl;
    TextureUse m_dashed_texture;
    TextureDecl const m_dotted_texture_decl;
    TextureUse m_dotted_texture;

/*
    FontProvider m_font_provider;
    TextFormatter m_text_formatter;
*/
};


inline void DomRenderer::set_viewport_height(int value)
{
    m_viewport_height = value;
}


} // namespace Render
} // namespace archon

#endif // ARCHON_RENDER_DOM_RENDERER_HPP
