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

#ifndef ARCHON_RENDER_TEXTURE_CACHE_HPP
#define ARCHON_RENDER_TEXTURE_CACHE_HPP

#include <memory>
#include <stdexcept>
#include <vector>
#include <string>

#include <GL/gl.h>

#include <archon/core/bind_ref.hpp>
#include <archon/core/unique_ptr.hpp>
#include <archon/image/image.hpp>


namespace archon {
namespace Render {

class TextureCache;
class TextureSource;
class TextureFileSource;
class TextureDecl;
class TextureUse;


std::unique_ptr<TextureCache> make_texture_cache();



/**
 * Notes on sharing texture objects between multiple OpenGL contexts: Texture
 * objects (names) created in one OpenGL context is available in another
 * context, if, and only if the two contexts are configured to share display
 * lists and texture objects. Also, deleting a texture object in either context
 * will delete it from both contexts.
 *
 *
 *
 * \todo FIXME: Currently, GL texture names are never freed.
 *
 * \todo FIXME: Test that a texture can be defined in one OpenGL rendering
 * context and then afterwards used in multiple contexts that are configured to
 * share textures.
 */
class TextureCache {
public:
    enum FilterMode {
        filter_mode_Nearest,
        filter_mode_Interp,
        filter_mode_Mipmap
    };


    /**
     * Declare a source of texture data. This process simply registers the
     * source and assigns a handle to it. It does not invoke OpenGL at all. To
     * allocate an OpenGL texture name for this texture source, you must call
     * the acquire() method on the returned handle object.
     *
     * The calling thread need not be bound to an OpenGL rendering context.
     *
     * This method may be called during the building of an OpenGL display list.
     *
     * \param wrap_s, wrap_t The texture coordinate wrapping modes in the
     * primary (horizontal) and secondary (vertical) directions
     * respectively. Pass \c GL_REPEAT to produce an infinite repetition of the
     * base texture, or GL_CLAMP to extend the base texture by its edge
     * colors. See the decumentation of glTexParameter() under \c
     * GL_TEXTURE_WRAP_S for further details.
     *
     * \param wait_for_refresh If true, the retrieval of the image from the
     * texture source will be postponed until some time after the
     * TextureDecl::refresh() has been called at least once on the returned
     * handle. Otherwise the image will be retrieved as soon as somebody
     * acquires the texture by calling TextureDecl::acquire().
     */
    TextureDecl declare(Core::UniquePtr<TextureSource>,
                        GLenum wrap_s = GL_REPEAT, GLenum wrap_t = GL_REPEAT,
                        FilterMode = filter_mode_Mipmap, bool wait_for_refresh = false,
                        bool fast_image_retrieval = true);


    /**
     * Carry out any outstanding duties relating to the texture image updating.
     *
     * It is intended that this method be called regularly, for example once per
     * frame in a frame based renderer.
     *
     * The calling thread must be bound to an OpenGL rendering context.
     *
     * Do not call this method during the building of an OpenGL display list.
     */
    void update()
    {
        if (dirty)
            update2();
    }


    TextureCache():
        dirty(false)
    {
    }

    virtual ~TextureCache()
    {
    }

protected:
    virtual std::size_t decl(Core::UniquePtr<TextureSource>, GLenum h_wrap, GLenum v_wrap,
                             FilterMode f, bool wait, bool fast) = 0;
    virtual void obtain_gl_name(std::size_t i) = 0; // Requires bound OpenGL context. Assumes Texture::has_name is false.
    virtual void update2() = 0; // Requires bound OpenGL context.
    virtual void refresh_image(std::size_t i) = 0;

    friend class TextureDecl;
    friend class TextureUse;

    class Texture;
    Texture* get_tex(std::size_t i)
    {
        return &textures[i];
    }

    struct Ref {
        TextureCache* cache = nullptr;
        std::size_t index = 0;
        Ref(TextureCache* c, std::size_t i):
            cache(c),
            index(i)
        {
        }
        Ref()
        {
        }
        bool operator==(const Ref& r) const
        {
            return cache == r.cache && index == r.index;
        }
        bool operator!=(const Ref& r) const
        {
            return cache != r.cache || index != r.index;
        }
        operator bool() const
        {
            return cache;
        }
        Texture* get_tex() const
        {
            return cache->get_tex(index);
        }
        Texture* operator->() const
        {
            return get_tex();
        }
    };

    std::vector<Texture> textures;

    bool dirty; // Set to true when update() has work to do
};



class TextureSource {
public:
    /**
     * Get the name of the source. If the source id a file system path, then
     * that path is the name. If it is a URL, it is that URL.
     */
    virtual std::string get_name() const = 0;

    /**
     * Must be thread-safe if the cache instance is used by more than one thread
     * (also counting the threads that access it indirectly through texture
     * binders).
     */
    virtual Imaging::Image::ConstRef get_image() = 0;

    virtual ~TextureSource() {}
};



class TextureFileSource: public TextureSource {
public:
    TextureFileSource(std::string path):
        path(path)
    {
    }
    std::string get_name() const
    {
        return path;
    }
    Imaging::Image::ConstRef get_image();

private:
    std::string path;
};



class TextureImageSource: public TextureSource {
public:
    TextureImageSource(Imaging::Image::ConstRefArg img, std::string name);
    std::string get_name() const
    {
        return name;
    }
    Imaging::Image::ConstRef get_image()
    {
        return image;
    }

private:
    Imaging::Image::ConstRef image;
    std::string name;
};



/**
 * A handle to a declared/registered source of texture data. The existance of
 * such a handle does not imply that the texture currently has an allocated
 * OpenGL texture name associated with it, however, through the acquire() method
 * it provides a means of acquiring an OpenGL texture name for this source.
 */
class TextureDecl {
public:
    /**
     * This method ensures that there is a unique OpenGL texture name assocated
     * with this texture source. A texture name may already have been allocated,
     * otherwise it is allocated now. The returned handle represents the
     * availability of the texture name. If this method is called multiple times
     * for the same texture source, all the returned handles will refer to the
     * same texture, but not necessarily to the same OpenGL texture name. In any
     * case, when all the returned handles have been destroyed (including all
     * the copies of those handles,) the texture cache may free/reuse the
     * allocated texture names.
     *
     * This method may or may not also initiate the image loading process
     * depending on the arguments passed to TextureCache::declare(), however,
     * the actual loading will never be done directly by this method. That is
     * supposed to happens as a consequence of repeatedly calling
     * TextureCache::update().
     *
     * This method must be called by a thread that is bound to an OpenGL
     * rendering context.
     *
     * This method may be called during the building of an OpenGL display list,
     * since it is guaranteed to never call an OpenGL function that can enter a
     * display list.
     */
    TextureUse acquire() const;

    /**
     * Discard any previously obtained image obtained from the texture
     * source. If the texture is already acquired, a new image will be retrieved
     * immediately, otherwise a new image will be retrieved when the texture is
     * acquired.
     */
    void refresh() const;

    std::string get_source_name() const;

    /**
     * Creates a null declaration.
     */
    TextureDecl()
    {
    }

    /**
     * Test if this is a proper declaration, that is, not a null declaration.
     *
     * \return False iff this is a null reference.
     */
    explicit operator bool() const throw()
    {
        return ref;
    }

private:
    friend class TextureCache;
    typedef TextureCache::Ref Ref;
    TextureDecl(const Ref& r):
        ref(r)
    {
    }
    struct Traits {
        static void bind(const Ref&) throw();
        static void unbind(const Ref&);
    };
    Core::BindRef<Ref, Traits> ref;
};



/**
 * This is a handle to a texture that has an allocated OpenGL texture name
 * associated with it. The OpenGL texture name can be retrieved using
 * get_gl_name(). When the handle and all its copies are destroyed, the OpenGL
 * texture name is freed, and thus must no longer be considered valid.
 */
class TextureUse {
public:
    // Must be called by a thread that is bound to an OpenGL rendering context.
    // May be called during the building of an OpenGL display list.
    void bind() const
    {
        glBindTexture(GL_TEXTURE_2D, get_gl_name());
    }

    GLuint get_gl_name() const;

    void clear()
    {
        ref.reset();
    }

    /**
     * Creates a null texture use.
     */
    TextureUse()
    {
    }

    /**
     * Test if this is a proper texture use, that is, not a null texture nuse.
     *
     * \return False iff this is a null reference.
     */
    explicit operator bool() const throw()
    {
        return ref;
    }

private:
    friend class TextureDecl;
    typedef TextureCache::Ref Ref;
    TextureUse(const Ref& r):
        ref(r)
    {
    }
    struct Traits {
        static void bind(const Ref&) throw();
        static void unbind(const Ref&);
    };
    Core::BindRef<Ref, Traits> ref;
};




// Implementation

/**
 * <pre>
 *
 *   has_name   has_image  updated   postpone    State
 *  ------------------------------------------------------------------
 *   no         no         no         no         NeedName   (static)
 *   no         no         no         yes        Postponed  (static)
 *   no         yes        no         no         ImageOnly  (static)
 *   yes        no         no         no         Loading
 *   yes        no         no         yes        Postponed2 (static)
 *   yes        no         yes        no         NoImage    (static)
 *   yes        yes        no         no         Updating
 *   yes        yes        yes        no         Ready      (static)
 *
 *
 *   Event            State transitions
 *  -------------------------------------------------------------------
 *   init             -->  NeedName | Postponed
 *
 *   refresh          Postponed | ImageOnly  -->  NeedName
 *                    Loading                -->  NeedName -->  Loading   (abort loading, start loading)
 *                    Updating                             -->  Loading   (abort updating, start loading)
 *                    Postponed2 | NoImage | Ready         -->  Loading   (start loading)
 *
 *   obtain_name      NeedName   -->  Loading    (start loading)
 *                    Postponed  -->  Postponed2
 *                    ImageOnly  -->  Updating   (start updating)
 *
 *   discard_name     Postponed2  -->  Postponed
 *                    Ready       -->  NeedName | ImageOnly
 *                    Updating    -->  NeedName | ImageOnly  (abort updating)
 *                    NoImage     -->  NeedName
 *                    Loading     -->  NeedName (abort loading)
 *
 *   finish_load      Loading  -->  Updating
 *
 *   finish_update    Updating  -->  Ready | NoImage
 *
 *   (Keep the image if it is considered a slow or heavy operation to optain it)
 *
 * </pre>
 */
class TextureCache::Texture {
public:
    void open(Core::UniquePtr<TextureSource> s, GLenum wrap_s, GLenum wrap_t,
              FilterMode f, bool wait, bool fast)
    {
        source = s.release();
        wrapping_s = wrap_s;
        wrapping_t = wrap_t;
        filter_mode = f;
        fast_load = fast;
        has_name = updated = false;
        postpone = wait;
        image.reset();
        pending_load = pending_update = false;
        decl_count = use_count = 0;
    }

    void close()
    {
        delete source;
        source = 0;
    }

    GLuint get_gl_name() const
    {
        return gl_name;
    }

    void decl_count_up()
    {
        ++decl_count;
    }
    void decl_count_down()
    {
        --decl_count;
    }

    void use_count_up()
    {
        ++use_count;
    }
    void use_count_down()
    {
        --use_count;
    }

    void show_state();

    TextureSource* source = nullptr; // Null when unused
    GLenum wrapping_s, wrapping_t; // Constant while used
    FilterMode filter_mode;
    bool fast_load; // Constant while used

    bool has_name, updated, postpone; // State flags (see above)
    Imaging::Image::ConstRef image;

    bool pending_load, pending_update; // On load_queue or update_queue and not yet processed

    GLuint gl_name; // Valid only when 'has_name' is true

    int decl_count, use_count;
};


inline TextureDecl TextureCache::declare(Core::UniquePtr<TextureSource> src,
                                         GLenum wrap_s, GLenum wrap_t,
                                         FilterMode f, bool wait, bool fast)
{
    std::size_t index = decl(src, wrap_s, wrap_t, f, wait, fast);
    return TextureDecl(Ref(this, index));
}

inline TextureImageSource::TextureImageSource(Imaging::Image::ConstRefArg img,
                                              std::string name):
    image(img),
    name(name)
{
}

inline TextureUse TextureDecl::acquire() const
{
    Ref r = ref.get();
    if (!r->has_name)
        r.cache->obtain_gl_name(r.index);
    return TextureUse(r);
}

inline void TextureDecl::refresh() const
{
    Ref r = ref.get();
    r.cache->refresh_image(r.index);
}

inline std::string TextureDecl::get_source_name() const
{
    return ref->source->get_name();
}

inline GLuint TextureUse::get_gl_name() const
{
    return ref->get_gl_name();
}

inline void TextureDecl::Traits::bind(const Ref& r) throw()
{
    r->decl_count_up();
}

inline void TextureDecl::Traits::unbind(const Ref& r)
{
    r->decl_count_down();
}

inline void TextureUse::Traits::bind(const Ref& r) throw()
{
    r->use_count_up();
}

inline void TextureUse::Traits::unbind(const Ref& r)
{
    r->use_count_down();
}

inline Imaging::Image::ConstRef TextureFileSource::get_image()
{
    return Imaging::Image::load(path);
}

} // namespace Render
} // namespace archon

#endif // ARCHON_RENDER_TEXTURE_CACHE_HPP
