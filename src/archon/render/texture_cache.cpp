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

#include <archon/core/mutex.hpp>
#include <archon/render/load_texture.hpp>
#include <archon/render/texture_cache.hpp>


using namespace archon::core;
using namespace archon::image;
using namespace archon::render;

namespace {

class TextureCacheImpl: public TextureCache {
public:
    std::size_t decl(std::unique_ptr<TextureSource> src, GLenum wrap_s, GLenum wrap_t,
                     FilterMode filter_mode, bool wait, bool fast)
    {
        if (m_unused_slots.empty()) {
            m_unused_slots.reserve(1); // Throws
            std::size_t i = m_textures.size();
            m_textures.resize(i+1); // Throws
            m_unused_slots.push_back(i);
        }
        std::size_t i = m_unused_slots.back();
        m_textures[i].open(std::move(src), wrap_s, wrap_t, filter_mode, wait, fast); // Throws
        m_unused_slots.pop_back();
        return i;
    }


    void obtain_gl_name(std::size_t i)
    {
        Texture* t = &m_textures[i];
        glGenTextures(1, &t->gl_name);
        t->has_name = true;
        if (t->postpone)
            return;
        if (t->image) {
            m_update_queue.push_back(i);
            t->pending_update = true;
        }
        else {
            m_load_queue.push_back(i);
            t->pending_load = true;
        }
        t->show_state();
        m_dirty = true; // signal to update()
    }


    void update_2()
    {
        while (!m_load_queue.empty()) {
            std::size_t i = m_load_queue.back();
            m_load_queue.pop_back();
            Texture* t = &m_textures[i];
            if (!t->pending_load)
                continue;
            t->pending_load = false;
            t->image = t->source->get_image();
            m_update_queue.push_back(i);
            t->pending_update = true;
            t->show_state();
        }
        while (!m_update_queue.empty()) {
            std::size_t i = m_update_queue.back();
            m_update_queue.pop_back();
            Texture* t = &m_textures[i];
            if (!t->pending_update)
                continue;
            {
                GLint prev;
                glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev);
                glBindTexture(GL_TEXTURE_2D, t->gl_name);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, t->wrapping_s);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, t->wrapping_t);
                if (t->filter_mode == FilterMode::mipmap) {
                    load_mipmap(t->image);
                }
                else {
                    bool with_border = false;
                    bool no_interp = (t->filter_mode == FilterMode::nearest);
                    load_texture(t->image, with_border, no_interp);
                }
                glBindTexture(GL_TEXTURE_2D, prev);
            }
            t->pending_update = false;
            t->updated = true;
            if (t->fast_load)
                t->image.reset();
            t->show_state();
        }
        m_dirty = false;
    }


    void refresh_image(std::size_t i)
    {
        Texture* t = &m_textures[i];
        if (t->has_name) {
            if (t->pending_load)
                return;
            t->pending_update = false;
            t->image.reset();
            t->updated = t->postpone = false;

            m_load_queue.push_back(i);
            t->pending_load = true;
            m_dirty = true; // signal to update()
        }
        else {
            t->image.reset();
            t->postpone = false;
        }
        t->show_state();
    }

private:
    std::vector<std::size_t> m_unused_slots;
    std::vector<std::size_t> m_load_queue, m_update_queue;
};

} // unnamed namespace


namespace archon {
namespace render {

std::unique_ptr<TextureCache> make_texture_cache()
{
    std::unique_ptr<TextureCache> c(new TextureCacheImpl);
    return c;
}


void TextureCache::Texture::show_state()
{
/*
    string s;
    switch ((has_name ? 8 : 0) + (image ? 4 : 0) + (updated ? 2 : 0) + (postpone ? 1 : 0)) {
        case 0:
            s = "NeedName";
            break;
        case 1:
            s = "Postponed";
            break;
        case 4:
            s = "ImageOnly";
            break;
        case 8:
            s = "Loading";
            break;
        case 8+1:
            s = "Postponed2";
            break;
        case 8+2:
            s = "NoImage";
            break;
        case 8+4:
            s = "Updating";
            break;
        case 8+4+2:
            s = "Ready";
            break;
        default:
            s = "(invalid)";
            break;
    }
    cerr << "State: '"<<source->get_name()<<"': "<<s << endl;
*/
}

} // namespace render
} // namespace archon
