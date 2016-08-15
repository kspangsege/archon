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

#ifndef ARCHON_DISPLAY_CONTEXT_HPP
#define ARCHON_DISPLAY_CONTEXT_HPP

#include <stdexcept>

#include <archon/display/drawable.hpp>


namespace archon
{
  namespace display
  {
    struct ContextAlreadyBoundException: std::runtime_error
    {
      ContextAlreadyBoundException(): std::runtime_error("") {}
    };

    struct NestedBindingException: std::runtime_error
    {
      NestedBindingException(): std::runtime_error("") {}
    };


    struct Bind;



    /**
     * The representation of an OpenGL rendering context. This can be
     * thought of as a state-machine or rendering pipeline.
     *
     * For the X11 Library based implementation, this class is a wrapper
     * around a <tt>GLXContext</tt>.
     *
     * Any thread that wishes to call OpenGL rendering primitives,
     * must first be bound to a rendering context and to a drawable
     * such as a window. This is done by instantiating the Bind class.
     *
     * New rendering contexts are created by calling
     * Connection::new_gl_context.
     *
     * \sa Bind
     * \sa Drawable
     * \sa Window
     * \sa Connection::new_gl_context
     * \sa http://www.opengl.org
     * \sa http://www.mesa3d.org
     */
    struct Context
    {
      typedef core::SharedPtr<Context> Ptr;
      typedef Ptr const &Arg;

      /**
       * Tell whether direct rendering is enabled for this
       * context. Direct rendering contexts offers better performance
       * but generally works only on a display of the local host.
       *
       * \note This method is thread safe.
       */
      virtual bool is_direct() const = 0;

      virtual ~Context() {}

    private:
      friend struct Bind;

      virtual void bind(Drawable::Arg, bool block) = 0;

      virtual void unbind() = 0;
    };




    /**
     * Establish a binding between the instantiating thread, a
     * rendering context, and an OpenGL capable drawable such as a
     * window with an appropriate visual.
     *
     * The effect of such a binding is that calls to OpenGL, issued by
     * the instantiating thread, are executed within the bound
     * rendering context causing polygons to be rendered on the bound
     * drawable.
     *
     * This binding is in effect as soon as the Bind object is
     * constructed, and generally lasts as long as the Bind object
     * exists, which is until the end of the scope of that
     * object. Sometimes however it is usefull to be able to unbind
     * and rebind within a single scope, which is possible with the \c
     * release and acquire() methods of this class.
     *
     * A thread can only be bound to one rendering context and vice
     * versa.
     *
     * A drawable however can be bound to multiple rendering contexts,
     * allowing multiple threads to render into the same drawable.
     *
     * \sa Context
     * \sa Drawable
     * \sa Window
     */
    struct Bind
    {
      /**
       * Establish a binding between the calling thread, the specified
       * context, and the specified drawable. This is done by calling
       * acquire() with the same arguments.
       *
       * \sa acquire.
       */
      Bind(Context::Arg c, Drawable::Arg d, bool block = true)
      {
	acquire(c, d, block);
      }

      /**
       * Do not bind anything. Binding can be achieved later, by
       * calling acquire().
       */
      Bind() {}

      ~Bind() { release(); }

      /**
       * Attempt to establish a binding between the instantiating
       * thread, the specified rendering context, and the specified
       * drawable. The context and the drawable must be created with
       * the same screen and visual. Further more, they must belong to
       * the same display implementation, and they must have been
       * created through the same display connection.
       *
       * If the specified context is currently bound to another thread
       * the instantiation will by default block until the context
       * becomes available. If you specify false for the 'block'
       * argument the instantiation will fail in this case.
       *
       * \param c The rendering context to bind.
       *
       * \param d The drawable to bind. This can be a window or an
       * off-screen pixel buffer.
       *
       * \param block If true the instantiation will block until the
       * specified context becomes available. Otherwise the
       * instantiation will fail if the context is in use by another
       * thread.
       *
       * \throw ContextAlreadyBoundException If \a bound was false and
       * the specified context was in use by another thread.
       *
       * \throw NestedBindingException If the calling thread is
       * already bound to a rendering context via a different Bind
       * instance.
       *
       * \throw invalid_argument if the context and the drawable
       * belong to different implementations, are created through
       * different connection, or are tied to different screens or
       * visuals.
       *
       * \note This method is not thread safe.
       */
      void acquire(Context::Arg c, Drawable::Arg d, bool block = true)
      {
	if(context) release();
	c->bind(d, block);
	context = c;
        drawable = d;
      }

      /**
       * Drop this binding.
       *
       * \note This method is not thread safe.
       */
      void release()
      {
	if(!context) return;
	context->unbind();
	context.reset();
        drawable.reset();
      }

    private:
      Context::Ptr context;
      Drawable::Ptr drawable;
    };
  }
}

#endif // ARCHON_DISPLAY_CONTEXT_HPP
